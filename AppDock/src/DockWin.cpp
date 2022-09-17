#include "DockWin.h"
#include <queue>
#include "AppDock.h"
#include "Layout/Sash.h"
#include "Layout/TabBar.h"
#include "DragPreviewOlyWin.h"

#include "TopDockWin.h"

DragHelperMgr::DragHelperMgr(DockWin* winDragged, Sash* sashDragged, const wxPoint& winMousePos)
{
    assert(winDragged   != nullptr);
    assert(sashDragged  != nullptr);

    assert(winDragged->HasCapture());
    this->winWithMouseCapture = winDragged;

    this->lastKnownGlobalMouse = wxGetMousePosition();
    this->lastLeftClick = wxGetMousePosition();
    this->dragType = DragType::Sash;

    this->draggingSash = sashDragged;
    this->dragOffset = winMousePos - sashDragged->pos;

    this->winWhereDragged = winDragged;

    // Both parents should be the same
    this->sashDraggedParent = this->draggingSash->r[0]->parent; 

    this->sashPreDragProps.clear();
    for(size_t i = 0; i < sashDraggedParent->children.size(); ++i)
    { 
        sashPreDragProps.push_back(
            sashDraggedParent->children[i]->proportion);
    }

    this->_AssertIsDraggingSashCorrectly();
}

DragHelperMgr::DragHelperMgr(DockWin* winDragged, TabBar* tbInvoker, bool clickedClose, Node* node, Node* tabOwner)
{
    assert(winDragged   != nullptr);
    assert(tbInvoker    != nullptr);
    assert(node         != nullptr);
    assert(tabOwner     != nullptr);
    assert(node == tabOwner || node->parent == tabOwner);
    assert(node->type == Node::Type::Window || node->type == Node::Type::Tabs);

    assert(tbInvoker->HasCapture());
    this->winWithMouseCapture = tbInvoker;

    this->lastKnownGlobalMouse = wxGetMousePosition();
    this->lastLeftClick     = wxGetMousePosition();
    this->dragType          = DragType::Tab;

    this->winWhereDragged   = winDragged;
    this->tabBarDrag        = tbInvoker;
    this->nodeDragged       = node;
    this->tabDragOwner      = tabOwner;
    this->clickedClose      = clickedClose;


    this->_AssertIsDraggingTabCorrectly();
}

bool DragHelperMgr::RemoveDropPreviewWin()
{
    if(this->dropPreviewWin == nullptr)
        return false;

    delete this->dropPreviewWin;
    this->dropPreviewWin = nullptr;
    return true;
}

bool DragHelperMgr::RemoveDraggingCursor()
{
    if(this->dragPreviewOlyWin == nullptr)
        return false;

    delete this->dragPreviewOlyWin;
    this->dragPreviewOlyWin = nullptr;
    return true;
}

bool DragHelperMgr::FinishSuccessfulTabDragging() // TODO: Take out return value
{
    assert(this->dragType == DragType::Tab);
    this->_AssertIsDraggingTabCorrectly();

    if(!this->startedDraggingTab)
    { 
        this->StopCapture(this->winWithMouseCapture);

        if(this->clickedClose)
        {
            this->winWhereDragged->CloseNodeWin(this->nodeDragged);

        }
        else if(this->nodeDragged->IsTabChild())
        {
            // TAB CLICK:
            // If we're here, the user clicked on a tab but never dragged
            // it (which would have transitioned the state to MouseState::DragWin).
            // Thus, a tab was clicked.

            // Cache in case we loose it from keyboard focus changing
            Node* nTabSys = this->nodeDragged->parent;

            assert(nTabSys != nullptr);
            assert(nTabSys->type == Node::Type::Tabs);
            bool sel = nTabSys->SelectTab(this->nodeDragged);
            assert(sel);

            this->winWhereDragged->layout.Resize(nTabSys, this->winWhereDragged->lprops);
            this->winWhereDragged->Refresh();
            
        }
        else
        {
            // If we're here, a simple window tab was clicked (without being dragged)
            // which doesn't do anything.
        }
        this->winWhereDragged = nullptr;
        this->nodeDragged = nullptr;
        this->tabDragOwner = nullptr;
        this->tabBarDrag = nullptr;
        this->nodeDragged = nullptr;
        this->dragType = DragHelperMgr::DragType::Invalid;
        this->_AssertIsNeutralized();
        return true;
    }

    if( 
        (this->winDraggedOnto == nullptr && this->winWhereDragged->HasRoot() == false) //||
        //droppedDst.where == DropResult::Where::Void
        )
    {
        // If we didn't drag into a window, and it was the only
        // thing previously in the DockWin (leaving nothing left
        // after it was dragged off) then just restore the window.

        this->CancelTabDragging();
        this->_AssertIsNeutralized();
        return true;
    }
   
    this->StopCapture(this->winWithMouseCapture);

    // At this point, some action will happen that we're commiting to,
    // so we can get rid of the undo information.
    //
    // Anything involved and removed from a drag node that we 
    // want kept should have been deleted before FinishMouseDrag 
    // was called.
    dragNodesInvolved.erase(this->nodeDragged);
    for(Node* n : this->dragNodesInvolved)
    { 
        n->ForgetHWND(Node::ForgetAction::Delete);
        delete n;
    }
    this->dragNodesInvolved.clear();
    this->dragUndo.clear();

    // If we tore it out of a window but didn't drop it on anything, but
    // the window still has stuff after we ripped out items (or else it would
    // have exited earlier from a previous if handler) then that will mean the
    // user wanted to rip it out as a new TopDockWin.
    if(this->winDraggedOnto == nullptr)
    {
        AppDock::GetApp().CreateTorn(this->nodeDragged, "Headphones");

        this->_ResolveToUpdateAfterDrag();
        this->nodeDragged = nullptr;
        this->dragType = DragType::Invalid;
        this->tabDragOwner = nullptr;
        this->winWhereDragged = nullptr;
        this->_AssertIsNeutralized();
        return true;
    }

    assert(droppedDst.where != DropResult::Where::Void);

    Node::Dest whereAdd = Node::Dest::Invalid;

    // NOTE: We may just want to get rid of one of the enums.
    switch(droppedDst.where)
    {
    case DropResult::Where::Left:
        whereAdd = Node::Dest::Left;
        break;
    case DropResult::Where::Right:
        whereAdd = Node::Dest::Right;
        break;
    case DropResult::Where::Top:
        whereAdd = Node::Dest::Above;
        break;
    case DropResult::Where::Bottom:
        whereAdd = Node::Dest::Below;
        break;
    case DropResult::Where::Onto:
        whereAdd = Node::Dest::Into;
    }
            

    this->nodeDragged->parent = nullptr; // To not trigger asserts
    winDraggedOnto->GetDockWin()->StealToLayout(
        this->nodeDragged,
        this->droppedDst.topOf,
        whereAdd);
    //
    this->nodeDragged = nullptr;

    if(this->winWhereDragged->HasRoot())
        this->tabDragOwner = nullptr;
    
    //////////////////////////////////////////////////
    //
    //  CLEANUP
    //
    //////////////////////////////////////////////////

    this->winDraggedOnto = nullptr;
    this->droppedDst.Invalidate();

    if(!this->startedDraggingTab)
    {
        // If nothin happened, restore it, and don't hold on to it
        // so it won't get deleted.
        this->tabBarDrag = nullptr;
    }
    
    this->nodeDragged         = nullptr;
    this->tabDragOwner        = nullptr;

    this->_ResolveToUpdateAfterDrag();
    

    if(this->winWhereDragged->layout.root == nullptr)
        this->winWhereDragged->owner->Close(true);
    else
        this->winWhereDragged->SetCursor(wxNullCursor);
    //
    this->winWhereDragged = nullptr;

    this->dragType = DragHelperMgr::DragType::Invalid;
    this->_AssertIsNeutralized();
    return true;
}

void DragHelperMgr::_ResolveToUpdateAfterDrag()
{
    assert(this->dragType == DragType::Tab);

    if(this->toUpdateAfterDrag != nullptr)
    {
        if(this->toUpdateAfterDrag->type == Node::Type::Window)
            this->toUpdateAfterDrag->ShowWindow();
        else if(this->toUpdateAfterDrag->type == Node::Type::Tabs)
            this->toUpdateAfterDrag->UpdateTabWindowVisibility();

        this->toUpdateAfterDrag = nullptr;
    }
}

bool DragHelperMgr::CancelTabDragging()
{
    assert(!this->dragFlaggedAsFinished);
    assert(this->winWithMouseCapture != nullptr);
    assert(this->dragType == DragType::Tab);
    //
    // The only time we should be cancelling a drag, when dealing with tabs, 
    // is while we're already dragging.
    assert(this->startedDraggingTab);

    this->StopCapture(this->winWithMouseCapture);

    Layout* layout = &this->winWhereDragged->layout;
    LProps& lp = this->winWhereDragged->lprops;

    this->dragNodesInvolved.clear();
    layout->UndoForget(this->dragUndo, lp);
    this->dragUndo.clear();
    this->nodeDragged->ShowWindow();
    this->nodeDragged = nullptr;
    
    // We leave tabBarDrag non-null so that it will be deleted in the destructor
    this->tabDragOwner->ClearTabBar();
    this->winWhereDragged->MaintainNodesTabBar(tabDragOwner);

    this->tabBarDrag = nullptr;
    this->tabDragOwner = nullptr;
    this->toUpdateAfterDrag = nullptr;
    this->winWhereDragged->ResizeLayout();
    this->winWhereDragged->SetCursor(wxNullCursor);
    this->winWhereDragged = nullptr;
    this->winDraggedOnto = nullptr;
    this->dragType = DragHelperMgr::DragType::Invalid;

    this->_AssertIsNeutralized();
    return true;
}

void DragHelperMgr::_AssertIsDraggingSashCorrectly()
{
    assert(this->dragType == DragType::Sash);
    assert(this->winWhereDragged != nullptr);
    assert(!this->dragFlaggedAsFinished);
    assert(this->winWithMouseCapture != nullptr);
    assert(this->winWithMouseCapture->HasCapture());

    // Assertion checks that the wrong data wasn't touched
    assert(this->dragPreviewOlyWin        == nullptr);
    assert(this->dropPreviewWin == nullptr);
    assert(this->winDraggedOnto == nullptr);
    assert(this->nodeDragged    == nullptr);
    assert(this->tabBarDrag     == nullptr);
    assert(this->tabDragOwner   == nullptr);
    assert(this->dragUndo.empty());
    assert(this->dragNodesInvolved.empty());
    assert(this->toUpdateAfterDrag == nullptr);
    assert(this->startedDraggingTab == false);

    // Assertion checks that expected stuff is filled out.
    assert(this->sashPreDragProps.size() > 0);
    assert(this->draggingSash       != nullptr);
    assert(this->sashDraggedParent  != nullptr);
    assert(!this->clickedClose);
}

void DragHelperMgr::_AssertIsDraggingTabCorrectly()
{
    assert(!this->dragFlaggedAsFinished);
    assert(this->winWithMouseCapture != nullptr);
    assert(this->winWithMouseCapture->HasCapture());
    assert(this->dragType == DragType::Tab);
    assert(this->winWhereDragged != nullptr);

    // Assertion checks that expected stuff is filled out.
    assert(this->sashPreDragProps.empty());
    assert(this->draggingSash       == nullptr);
    assert(this->sashDraggedParent  == nullptr);

    assert((this->dragPreviewOlyWin != nullptr) == this->startedDraggingTab);
    //assert((this->dropPreviewWin != nullptr) == this->startedDraggingTab);
    assert(this->nodeDragged != nullptr);
    assert(this->tabBarDrag != nullptr  || this->startedDraggingTab);
    assert(this->tabDragOwner != nullptr);
    assert(this->dragUndo.empty()           != this->startedDraggingTab);
    assert(this->dragNodesInvolved.empty()  != this->startedDraggingTab);
    //assert(this->toUpdateAfterDrag != nullptr);
}


void DragHelperMgr::_AssertIsNeutralized()
{
    // When we're at the point that this function is called,
    // either all this stuff was never set (depending on the
    // type of drag and what happened during the drag) or
    // it should have been manually cleared out and disabled.

    assert( this->dragType == DragType::Invalid);
    assert( this->winDraggedOnto        == nullptr);
    assert( this->dragUndo.empty());
    assert( this->nodeDragged           == nullptr);
    assert( this->dragNodesInvolved.empty());
    assert( this->sashDraggedParent     == nullptr);
    assert( this->sashPreDragProps.empty());
    //assert( this->tabBarDrag            == nullptr);
    assert( this->tabDragOwner          == nullptr);
    assert( this->toUpdateAfterDrag     == nullptr);
    assert( this->draggingSash          == nullptr);
    assert( this->winWhereDragged       == nullptr);

    assert(this->dragFlaggedAsFinished);
    assert(this->winWithMouseCapture    == nullptr);

    // These are cleaned up, if needed, in the destructor
    // assert( this->dragPreviewOlyWin     == nullptr); 
    // assert( this->dropPreviewWin        == nullptr);
}

bool DragHelperMgr::FinishSuccessfulSashDragging()
{
    // Not much to do except stop
    this->_AssertIsDraggingSashCorrectly();
    this->StopCapture(this->winWhereDragged);
    
    this->draggingSash = nullptr;

    this->sashPreDragProps.clear();
    this->sashDraggedParent = nullptr;

    this->winWhereDragged->SetCursor(wxNullCursor);
    this->winWhereDragged = nullptr;

    this->dragType = DragType::Invalid;
    this->_AssertIsNeutralized();
    return true;
}

bool DragHelperMgr::CancelSashDragging()
{
    this->_AssertIsDraggingSashCorrectly();

    // Restore the children sizes in the parent the sash modifies
    this->sashDraggedParent->ResizeChildrenByProportions(
            this->sashPreDragProps, 
            this->winWhereDragged->lprops);

    this->winWhereDragged->Refresh();
    
    this->sashDraggedParent = nullptr;
    this->winWhereDragged = nullptr;

    this->winWhereDragged->SetCursor(wxNullCursor);
    this->dragType = DragType::Invalid;

    this->_AssertIsNeutralized();
    return true;
}

DragHelperMgr::~DragHelperMgr()
{
    this->RemoveDropPreviewWin();
    this->RemoveDraggingCursor();

    // This may be left non-null to delete AFTER a tab drag operation
    // is completed. This is because it needs to be managed after the
    // entire drag operation because it (tabBarDrag) will have had
    // the mouse capture so it has to stay alive through the entire 
    // process.
    if(this->tabBarDrag != nullptr)
        delete this->tabBarDrag;

    // If these asserts fail, it means the drag-and-drop didn't go through
    // the proper channels (or the proper channels have a bug).
    assert(this->dragFlaggedAsFinished);
    assert(this->winWithMouseCapture == nullptr);
}

void DragHelperMgr::HandleMouseMove()
{
    assert(this->dragType != DragType::Invalid);

    wxPoint curGlobalPt = wxGetMousePosition();
    wxPoint delta = curGlobalPt - this->lastKnownGlobalMouse;

    if(this->dragType == DragType::Sash)
        this->_HandleMouseMoveSash(delta);
    else if(this->dragType == DragType::Tab)
        this->_HandleMouseMoveTab(delta);
    else
        assert(!"Unhandled mouse move type");

    this->lastKnownGlobalMouse = curGlobalPt;
}

void DragHelperMgr::_HandleMouseMoveSash(const wxPoint& delta)
{
    this->_AssertIsDraggingSashCorrectly();

    wxPoint pt = wxGetMousePosition();

    this->winWhereDragged->SetCursor(*wxCROSS_CURSOR);

    LProps& lp = this->winWhereDragged->lprops;
    Layout* layout = &this->winWhereDragged->layout;

    if(this->draggingSash->dir == Sash::Dir::Horiz)
    {
        Node* nleft = this->draggingSash->left;
        Node* nright = this->draggingSash->right;
        wxSize minLeft = nleft->CalculateMinSize(lp);
        wxSize minRight = nright->CalculateMinSize(lp);

        // Check if there's even enough space to do a 
        // dragging operation.
        if(nleft->cacheSize.x + nright->cacheSize.x <= minLeft.x + minRight.x)
            return;

        this->draggingSash->SlideHoriz(delta.x);
        layout->Resize(nleft, lp);
        layout->Resize(nright, lp);
        nleft->parent->CalculateChildProportions_Cache();

    }
    else if(this->draggingSash->dir == Sash::Dir::Vert)
    {
        Node* ntop = this->draggingSash->top;
        Node* nbot = this->draggingSash->bot;
        wxSize minTop = ntop->CalculateMinSize(lp);
        wxSize minBot = nbot->CalculateMinSize(lp);

        if(ntop->cacheSize.y + nbot->cacheSize.y < minTop.y + minBot.y)
            return;

        this->draggingSash->SlideVert(delta.y);
        layout->Resize(ntop, lp);
        layout->Resize(nbot, lp);
        ntop->parent->CalculateChildProportions_Cache();
    }
    else
    {
        assert("Invalid container type for sash dragging.");
    }
    this->winWhereDragged->Refresh();
}

void DragHelperMgr::_HandleMouseMoveTab(const wxPoint& delta)
{
    this->_AssertIsDraggingTabCorrectly();

    if(!this->startedDraggingTab)
    {
        const float tearDist = 3.0f;
        //
        float difX = this->lastLeftClick.x - this->lastKnownGlobalMouse.x;
        float difY = this->lastLeftClick.y - this->lastKnownGlobalMouse.y;
        float pxDragDst = sqrt(difX * difX + difY * difY);

        bool doLayout = false;
        if(pxDragDst > tearDist)
        { 
            if(tabDragOwner->type == Node::Type::Window)
            {
                // Don't allow the old tab bar to draw.
                tabDragOwner->GetTabBar()->Hide();
                // Very hackey, but we can't let the original TabBar that was dragged
                // be destroyed while we're dragging.
                // 
                // We'll set it back later if nothing happened, or we'll destroy it if
                // the tab was moved around (in which case it will create another
                // TabBar for itself and we can get rid of the original one.
                tabDragOwner->ForgetTabBar();
                // Keep it around but hide it by moving it way off the window
                tabBarDrag->SetPosition(wxPoint(0, -100000));
            }
            else if(tabDragOwner->type == Node::Type::Tabs)
            {
                if(tabDragOwner->children.size() > 2)
                { 
                    toUpdateAfterDrag = tabDragOwner;
                    tabBarDrag = nullptr; // Don't delete it in ~DragHelperMgr
                }
                else
                {
                    toUpdateAfterDrag = tabDragOwner->ChildOtherThan(nodeDragged);

                    tabDragOwner->GetTabBar()->Hide();
                    tabDragOwner->ForgetTabBar();
                    tabBarDrag->SetPosition(wxPoint(0, -100000));
                }
            }

            if(this->winWhereDragged->layout._ForgetWindow(
                nodeDragged, 
                Node::ForgetAction::Hide, 
                dragUndo,
                dragNodesInvolved, 
                true,
                false))
            {
                doLayout = true;
            }

            // If anything was moved out of a tab region (because the
            // tab was collapsed) give it a tab region.

            for(Layout::ForgetUndo& fundo : dragUndo)
            {
                if(fundo.ty != Layout::ForgetUndo::Type::Stitching)
                    continue;

                if(fundo.node->type != Node::Type::Window)
                    continue;

                if(fundo.prvParent == nullptr)
                    continue;

                if(fundo.prvParent->type == Node::Type::Tabs)
                { 
                    this->winWhereDragged->MaintainNodesTabBar(fundo.node);
                    doLayout = true;
                }
            }


            this->startedDraggingTab = true;      
            this->SetDragPreviewOlyWin(wxGetMousePosition());
        }
        if(doLayout == true)
            this->winWhereDragged->ResizeLayout();
    }
    else
    {
        wxPoint globMouse = wxGetMousePosition();
        this->SetDragPreviewOlyWin(globMouse);

        bool foundTarg = false;
        winDraggedOnto = TopDockWin::GetWinAt(globMouse);
        if(winDraggedOnto != nullptr)
        { 
            wxPoint othMouse = winDraggedOnto->GetDockWin()->ScreenToClient(globMouse);

            // DRAGGING ONTO A SASH?
            //////////////////////////////////////////////////
            Sash* s = winDraggedOnto->GetDockWin()->layout.GetSashAt(othMouse);

            if(s != nullptr)
            {
                droppedDst = 
                    DropResult(
                        s->dir == Sash::Dir::Horiz ?
                        DropResult::Where::Right : 
                        DropResult::Where::Bottom,
                        s->r[0],
                        s,
                        s->pos,
                        s->size);

                SetDropPreviewWin(
                    s->pos,
                    s->size,
                    this->winWhereDragged, 
                    winDraggedOnto->GetDockWin());

                foundTarg = true;
            }
            else
            {
                // DRAGGING ONTO A NODE IN THE LAYOUT?
                //////////////////////////////////////////////////
                Node* pn = winDraggedOnto->GetDockWin()->layout.GetNodeAt(othMouse);

                if(pn == nullptr && winDraggedOnto->GetDockWin()->layout.root == nullptr)
                {
                    // If there's no node found in the region, we're going to
                    // assume it's the root
                    //
                    // NOTE: This is NOT 100% correct because there may be a border
                    // outside of the grid.
                    wxSize otherClientsz = winDraggedOnto->GetClientSize();
                    droppedDst = 
                        DropResult(
                            DropResult::Where::Onto, 
                            nullptr, 
                            nullptr, 
                            wxPoint(0,0), 
                            otherClientsz);

                    SetDropPreviewWin(
                        droppedDst.dropRgnPt, 
                        droppedDst.dropRgnSz, 
                        this->winWhereDragged, 
                        winDraggedOnto->GetDockWin());

                    foundTarg = true;
                }
                else
                {
                    //SetDropPreviewWin(--)


                    droppedDst = 
                        this->winDraggedOnto->GetDockWin()->layout.ScanForDrop(
                            othMouse, 
                            this->winWhereDragged->lprops);

                    if(droppedDst.where == DropResult::Where::Left)
                    {
                        SetDropPreviewWin(
                            droppedDst.topOf->cachePos, 
                            wxSize(
                                this->winWhereDragged->lprops.dropEdgeWidth, 
                                droppedDst.topOf->cacheSize.y),
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(droppedDst.where == DropResult::Where::Right)
                    {
                        SetDropPreviewWin(
                            wxPoint(
                                droppedDst.topOf->CacheRight() - this->winWhereDragged->lprops.dropEdgeWidth,
                                droppedDst.topOf->cachePos.y), 
                            wxSize(
                                this->winWhereDragged->lprops.dropEdgeWidth, 
                                droppedDst.topOf->cacheSize.y),
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(droppedDst.where == DropResult::Where::Top)
                    {
                        SetDropPreviewWin(
                            droppedDst.topOf->cachePos,
                            wxSize(
                                droppedDst.topOf->cacheSize.x, 
                                this->winWhereDragged->lprops.dropEdgeWidth),
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(droppedDst.where == DropResult::Where::Bottom)
                    {
                        SetDropPreviewWin(
                            wxPoint(
                                droppedDst.topOf->cachePos.x,
                                droppedDst.topOf->CacheBot() - this->winWhereDragged->lprops.dropEdgeWidth),
                            wxSize(
                                droppedDst.topOf->cacheSize.x,
                                this->winWhereDragged->lprops.dropEdgeWidth),
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());


                        foundTarg = true;
                    }
                    else if(droppedDst.where == DropResult::Where::Onto)
                    {
                        SetDropPreviewWin(
                            droppedDst.dropRgnPt,
                            droppedDst.dropRgnSz,
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                }
            }
        }

        if(foundTarg == false && this->dropPreviewWin != nullptr)
        {
            // Hide it, but don't destroy it because we might end up
            // recreating it soon as the user continues to drag the
            // mouse.
            this->dropPreviewWin->Hide();
        }
    }
}

void DragHelperMgr::SetDropPreviewWin(
    const wxPoint& pt, 
    const wxSize& sz, 
    DockWin* dwInvoker, 
    DockWin* dwDst, 
    bool raise )
{
    if(dropPreviewWin == nullptr)
    { 
        dropPreviewWin = new BarDrop(nullptr, dwInvoker);

        // Creating the new window will steal keyboard focus, but we
        // still want it to detect pressing the escape key.
    }
    dropPreviewWin->Show();

    wxPoint dstScreenOrigin = 
        dwDst->ClientToScreen(wxPoint(0,0));

    dropPreviewWin->SetPosition(dstScreenOrigin + pt);
    dropPreviewWin->SetSize(sz);

    if(raise == true)
        dropPreviewWin->Raise();
}

void DragHelperMgr::ResumeCapture(wxWindow* requester)
{
    // Some of these asserts are redundant, but it helps describe all the constraints.
    assert(this->winWithMouseCapture != nullptr);
    assert(this->winWithMouseCapture == requester);
    assert(!requester->HasCapture());
    assert(GetCapture() == NULL); // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getcapture
    assert(!this->dragFlaggedAsFinished);

    // When resuming, we already know we're going to resume this->winWithMouseCapture,
    // the parameter is just a saftey precaution that ResumeCapture is being called
    // 
    this->winWithMouseCapture->GetCapture();
    //SetCapture(this->winWithMouseCapture->GetHWND());

    assert(this->winWithMouseCapture->HasCapture());
}

void DragHelperMgr::StopCapture(wxWindow* requester)
{
    assert(requester != nullptr);
    assert(this->winWithMouseCapture == requester);
    assert(this->winWithMouseCapture->HasCapture());
    assert(!this->dragFlaggedAsFinished);

    // Release the mouse will invoke callbacks which may reference this object, so
    // we need to neutralize this stuff before releasing the mouse capture.
    wxWindow* toRelease = this->winWithMouseCapture;
    this->winWithMouseCapture = nullptr;
    this->dragFlaggedAsFinished = true;
    toRelease->ReleaseMouse();
}

void DragHelperMgr::SetDragPreviewOlyWin(const wxPoint& whereAt)
{
    assert(this->dragType == DragType::Tab);

    if(this->dragPreviewOlyWin == nullptr)
    {
        this->dragPreviewOlyWin = new DragPreviewOlyWin(this->winWhereDragged, this->winWhereDragged);
        this->dragPreviewOlyWin->SetCursor(*wxCROSS_CURSOR);
        this->dragPreviewOlyWin->Show();
    }
    this->dragPreviewOlyWin->SetPosition(whereAt);
}

int DockWin::instCtr = 0;

BEGIN_EVENT_TABLE(DockWin, wxWindow)
    EVT_PAINT       (DockWin::OnDraw        )
    EVT_SIZE        (DockWin::OnSize        )
    EVT_LEFT_DOWN   (DockWin::OnMouseLDown  )
    EVT_LEFT_UP     (DockWin::OnMouseLUp    )
    EVT_MIDDLE_DOWN (DockWin::OnMouseMDown  )
    EVT_MIDDLE_UP   (DockWin::OnMouseMUp    )
    EVT_RIGHT_DOWN  (DockWin::OnMouseRDown  )
    EVT_RIGHT_UP    (DockWin::OnMouseRUp    )
    EVT_MOTION      (DockWin::OnMouseMotion )
    EVT_MOUSE_CAPTURE_CHANGED   (DockWin::OnMouseCaptureChanged )
    EVT_MOUSE_CAPTURE_LOST      (DockWin::OnMouseCaptureLost    )
    EVT_KEY_DOWN    (DockWin::OnKeyDown     )
    EVT_KEY_UP      (DockWin::OnKeyUp       )
END_EVENT_TABLE()

TabDraggingMgrPtr DockWin::dragggingMgr;

DockWin::DockWin(
        wxWindow *parent,
        TopDockWin* owner,
        wxWindowID id,
        const wxPoint& pos,
        const wxSize& size,
        long style,
        const wxString& name)
    : wxWindow(parent, id, pos, size, style, name)
{
    ++instCtr;

    //AllocConsole();
    this->owner = owner;
    this->SetBackgroundColour(wxColour(200, 200, 200));
    this->layout._SetContentWin(this->m_hWnd);
}

DockWin::~DockWin()
{}

Node * DockWin::AddToLayout(HWND hwnd, Node* reference, Node::Dest refDock)
{
    --instCtr;
    assert(instCtr >= 0);

    Node* n = this->layout.Add(hwnd, reference, refDock);
    this->MaintainNodesTabBar(n);
    return n;
}

bool DockWin::StealToLayout(Node* n, Node* reference, Node::Dest refDock)
{
    // We cannot be dragging with a mouse capture if here,
    // because Steal() will force the OS to exit it.
    assert(this->dragggingMgr == nullptr || this->dragggingMgr->dragFlaggedAsFinished);

    n->ClearTabBar();

    bool ret = 
        this->layout.Steal(
            n, 
            reference, 
            refDock, 
            this->lprops);

    if(ret == false)
        return false;

    if(n->IsTabChild() == true)
    {
        Node* nTabSys   = n->parent;
        bool sel        = nTabSys->SelectTab(n);
        assert(sel == true);

        this->MaintainNodesTabBar(nTabSys);
        nTabSys->GetTabBar()->Refresh();
    }
    else
        this->MaintainNodesTabBar(n);

    this->ResizeLayout(true, true);
    return true;
}

void DockWin::StealRoot(Node* n)
{
    assert(this->layout.root == nullptr);
    assert(n->type == Node::Type::Window);

    HWND hwnd = n->Hwnd();
    if(hwnd != NULL)
    { 
        ::SetParent(hwnd, this->GetHWND());
        this->layout.hwndLookup[hwnd] = n;
    }


    this->layout.root = n;
    n->parent = nullptr;
    n->ClearTabBar();
    this->MaintainNodesTabBar(n);
    n->ShowWindow();
    this->ResizeLayout();
}

Sash* DockWin::SashAtPoint(const wxPoint& pt)
{
    for(Sash* s : this->layout.sashes)
    {
        if(s->Contains(pt) == true)
            return s;
    }
    return nullptr;
}

Node* DockWin::GetNodeTab(const wxPoint& pt)
{
    // Find which tab was clicked
    //
    // Note the GetNodeAt function has the stopAtTabs set to true.
    Node* n = this->layout.GetNodeAt(pt, true);
    if(n != nullptr)
    {
        if(n->type == Node::Type::Window)
        {
            if(n->cachedTab.Contains(pt))
                return n;
        }
        else if(n->type == Node::Type::Tabs)
        {
            for(size_t i = 0; i < n->children.size(); ++i)
            {
                Node* nt = n->children[i];
                if(nt->cachedTab.Contains(pt))
                    return nt;
            }
        }
    }
    return nullptr;
}

Node* DockWin::AddRootHwnd(HWND hwnd)
{
    if(this->layout.root != nullptr)
        return nullptr;

    Node* n = 
        this->AddToLayout(
            hwnd, 
            nullptr, 
            Node::Dest::Above);

    if(n == nullptr)
        return nullptr;

    this->ResizeLayout();

    return n;
}

void DockWin::ResizeLayout(bool refresh, bool rebuildSahes)
{
    this->layout.Resize(this->GetClientSize(), this->lprops);

    if(refresh == true)
        this->Refresh();

    if(rebuildSahes == true)
        this->layout.RebuildSashes(this->lprops);
}

void DockWin::MaintainNodesTabBar(Node* pn)
{
    assert(pn != nullptr);
    if(
        pn->type == Node::Type::Horizontal || 
        pn->type == Node::Type::Vertical)
    {
        // Shouldn't ever do anything, sanity operation.
        pn->ClearTabBar();
        return;
    }

    if(pn->GetTabBar() == nullptr)
    {
        TabBar* tb = new TabBar(this, pn);
        pn->SetTabBar(tb);
    }
}

void DockWin::ClearDocked()
{
    this->layout.Clear();
}

void DockWin::ReleaseAll(bool clear)
{
    std::vector<Node*> wndNodes;
    this->layout.CollectHWNDNodes(wndNodes);

    for(Node* n : wndNodes)
        n->ReleaseHWND();

    if(clear == true)
        this->ClearDocked();
}





void DockWin::OnDraw(wxPaintEvent& paint)
{
    //wxPaintDC dc(this);
    //
    //std::queue<Node*> qn;
    //
    //dc.SetPen(*wxBLACK_PEN);
    //
    //if(this->layout.root == nullptr)
    //    return;
    //
    //qn.push(this->layout.root);
    //
    //while(qn.empty() == false)
    //{
    //    Node * cur = qn.front();
    //    qn.pop();
    //
    //    LProps& lp = this->lprops;
    //
    //    dc.DrawRectangle(
    //        wxRect(
    //            cur->cachePos.x,
    //            cur->cachePos.y,
    //            cur->cacheSize.x, 
    //            cur->cacheSize.y));
    //
    //        for(Node * child : cur->children)
    //            qn.push(child);
    //}
}

void DockWin::OnSize(wxSizeEvent& evt)
{
    this->ResizeLayout();
}

void DockWin::OnClose(wxCloseEvent& evt)
{
    //FreeConsole();
}

void DockWin::OnMouseLDown(wxMouseEvent& evt)
{
    this->dragggingMgr.reset();

    wxPoint pos = evt.GetPosition();
    Sash* olySash = this->SashAtPoint(pos);
    if(olySash != nullptr)
    {
        this->CaptureMouse();
        this->dragggingMgr = TabDraggingMgrPtr(new DragHelperMgr(this, olySash, pos));
        return;
    }
}

void DockWin::OnMouseLUp(wxMouseEvent& evt)
{
    if(this->dragggingMgr != nullptr)
    {
        assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Sash);
        this->dragggingMgr->FinishSuccessfulSashDragging();
        this->FinishMouseDrag();
    }
}

void DockWin::OnMouseMDown(wxMouseEvent& evt)
{
}

void DockWin::OnMouseMUp(wxMouseEvent& evt)
{
}

void DockWin::OnMouseRDown(wxMouseEvent& evt)
{
}

void DockWin::OnMouseRUp(wxMouseEvent& evt)
{
}

void DockWin::OnMouseMotion(wxMouseEvent& evt)
{
    if(this->dragggingMgr != nullptr)
    {
        assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Sash);
        this->dragggingMgr->HandleMouseMove();
    }
}

void DockWin::OnMouseCaptureChanged(wxMouseCaptureChangedEvent& evt)
{
    if(this->dragggingMgr != nullptr)
    { 
        if(!this->dragggingMgr->dragFlaggedAsFinished)
            this->dragggingMgr->CancelSashDragging();
    }
}

void DockWin::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
}

void DockWin::OnKeyDown(wxKeyEvent& evt)
{
    if(evt.GetKeyCode() == WXK_ESCAPE)
    {
        if(this->HasCapture())
        {
            assert(this->dragggingMgr != nullptr);
            assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Sash);
            this->dragggingMgr->CancelSashDragging();
            this->ReleaseMouse();
            this->Refresh();
        }
    }
}

void DockWin::OnKeyUp(wxKeyEvent& evt)
{
}

// Whenever we detect collateral manipulations from a node removal,
// make sure the nodes are managed correctly. Mainly in respect to
// their tab bar.
void _ProcessInvolvedFromRem(std::set<Node*>& involved, DockWin* dw)
{
    for(Node* n : involved)
    {
        if(n->type == Node::Type::Tabs)
        { 
            n->SelectTab(n->selTab);
        }
        else if(
            n->type == Node::Type::Window && 
            (n->parent == nullptr || n->parent->type != Node::Type::Tabs))
        {
            dw->MaintainNodesTabBar(n);
        }
    }
}

bool DockWin::ReleaseNodeWin(Node* pn)
{
    std::set<Node*> involved;
    if(this->layout.ReleaseWindow(pn, &involved) == false)
        return false;

    _ProcessInvolvedFromRem(involved, this);
    this->_ReactToNodeRemoval();
    return true;
}

bool DockWin::DettachNodeWin(Node* pn)
{
    HWND hwnd = pn->Hwnd();
    if(hwnd == NULL)
        return false;

    std::set<Node*> involved;
    if(this->layout.ReleaseWindow(pn, &involved) == false)
        return false;

    AppDock::GetApp().CreateWindowFromHwnd(hwnd);

    _ProcessInvolvedFromRem(involved, this);
    this->_ReactToNodeRemoval();
    return true;
}

bool DockWin::CloseNodeWin(Node* pn)
{
    assert(this->layout.root != nullptr);

    std::set<Node*> involved;
    if(this->layout.DeleteWindow(pn, & involved) == false)
        return false;

    _ProcessInvolvedFromRem(involved, this);
    this->_ReactToNodeRemoval();
    return true;
}

bool DockWin::CloneNodeWin(Node* pn)
{
    if(pn->win == NULL)
        return false;

    HWND hwnd = 
        AppDock::CreateSpawnedWindow(pn->cmdLine);

    if(hwnd == NULL)
        return false;

    Node* nClone = this->AddToLayout( hwnd, pn, Node::Dest::Into);
    nClone->cmdLine = pn->cmdLine;
    this->ResizeLayout(); // Mainly to force redraw

    return true;
}

void DockWin::TabClickStart(TabBar* tbInvoker, Node* node, Node* tabOwner, bool closePressed)
{
    assert(this->dragggingMgr == nullptr);

    tbInvoker->CaptureMouse();
    this->dragggingMgr = TabDraggingMgrPtr(new DragHelperMgr(this, tbInvoker, closePressed, node, tabOwner));
    this->dragggingMgr->_AssertIsDraggingTabCorrectly();
}

void DockWin::TabClickMotion()
{
    assert(this->dragggingMgr != nullptr);
    assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Tab);

    // TODO: Return to let the invoker know if it needs to mouse capture
    this->dragggingMgr->HandleMouseMove();
}

void DockWin::TabClickEnd()
{
    assert(this->dragggingMgr != nullptr);
    assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Tab);

    this->dragggingMgr->FinishSuccessfulTabDragging();
    this->FinishMouseDrag();
}

void DockWin::TabClickCancel()
{
    this->OnDelegatedEscape();
}

void DockWin::FinishMouseDrag()
{
    this->dragggingMgr.reset();
}

void DockWin::OnDelegatedEscape()
{
    assert(this->dragggingMgr);
    assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Tab);

    this->dragggingMgr->CancelTabDragging();
    this->FinishMouseDrag();
}


void DockWin::_ReactToNodeRemoval()
{
    if(this->HasRoot() == false)
    {
        // If no more windows, close down the entire thing. Note that we should
        // only do this if we're not in the middle of a drag, because that can
        // still be undone if the drag is cancelled.
        this->owner->Close();
    }
    else
        this->ResizeLayout();
}

json DockWin::_JSONRepresentation()
{
    if(this->layout.root == nullptr)
        return json::object();

    struct AnonUtil
    {
        static json $(Node* n)
        {
            json jsRet = json::object();
            jsRet["id"] = n->id;

            switch(n->type)
            {
            case Node::Type::Window:
                jsRet["ty"] = "win";
                jsRet["cmd"] = n->cmdLine;
                if(n->GetTabBar() != nullptr)
                    jsRet["tab"] = n->GetTabBar()->id;
                break;
                
            case Node::Type::Horizontal:
                jsRet["ty"] = "horiz";
                break;

            case Node::Type::Vertical:
                jsRet["ty"] = "vert";
                break;

            case Node::Type::Tabs:
                jsRet["ty"] = "tabs";
                jsRet["sel"] = n->selTab;
                if(n->GetTabBar() != nullptr)
                    jsRet["tab"] = n->GetTabBar()->id;
                break;

            default:
                assert(false);
            }

            if(!n->children.empty())
            {
                json jsChildren = json::array();

                for(Node* nChild : n->children)
                    jsChildren.push_back($(nChild));

                jsRet["children"] = jsChildren;
            }

            return jsRet;
        }
    };

    return AnonUtil::$(this->layout.root);
}

bool DockWin::_TestValidity()
{
    return true;
}