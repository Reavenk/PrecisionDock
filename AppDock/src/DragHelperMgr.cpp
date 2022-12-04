#include "DragHelperMgr.h"

#include "Layout/Sash.h"
#include "DockWin.h"
#include "TopDockWin.h"
#include "Layout/TabsBar.h"
#include "AppDock.h"
#include "DragPreviewOlyWin.h"

DragHelperMgr::DragHelperMgr(DockWin* winDragged, Sash* sashDragged, const wxPoint& winMousePos)
{
    assert(winDragged   != nullptr);
    assert(sashDragged  != nullptr);

    assert(winDragged->HasCapture());
    this->winWithMouseCapture = winDragged;

    this->lastKnownGlobalMouse  = wxGetMousePosition();
    this->lastLeftClick         = wxGetMousePosition();
    this->dragType              = DragType::Sash;

    this->draggingSash          = sashDragged;
    this->dragOffset            = winMousePos - sashDragged->pos;

    this->winWhereDragged = winDragged;

    // Both parents should be the same
    this->sashDraggedParent = this->draggingSash->r[0]->parent; 

    this->preDragSashProps.clear();
    for(size_t i = 0; i < sashDraggedParent->children.size(); ++i)
    { 
        preDragSashProps.push_back(
            sashDraggedParent->children[i]->proportion);
    }

    this->_AssertIsDraggingSashCorrectly(true);
}

DragHelperMgr::DragHelperMgr(DockWin* winDragged, TabsBar* tbInvoker, bool clickedClose, Node* node, Node* tabOwner)
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
    this->tabsBarDrag        = tbInvoker;
    this->nodeDragged       = node;
    this->tabDragOwner      = tabOwner;
    this->clickedClose      = clickedClose;


    this->_AssertIsDraggingTabCorrectly(true);
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
    if(this->draggingCursorGraphic == nullptr)
        return false;

    delete this->draggingCursorGraphic;
    this->draggingCursorGraphic = nullptr;
    return true;
}

void DragHelperMgr::FinishSuccessfulTabDragging()
{
    assert(this->dragType == DragType::Tab);
    this->_AssertIsDraggingTabCorrectly(true);

    if(!this->alreadyToreOffTab)
    { 
        this->StopCapture(this->winWithMouseCapture, false);

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
            //
            // Although if we don't refresh it here, it won't get a chance to redraw
            // itself correctly without the mouse down. We can't rely on the TabsBar::OnMouseLUp()
            // to handle it because it doesn't know if it's closing - so it can't access the
            // Dockwin or node after delegating to DragHelperMgr.
            this->tabsBarDrag->Refresh();
        }
        this->winWhereDragged = nullptr;
        this->nodeDragged = nullptr;
        this->tabDragOwner = nullptr;
        this->tabsBarDrag = nullptr;
        this->nodeDragged = nullptr;
        this->dragType = DragHelperMgr::DragType::Invalid;
        this->_AssertIsNeutralized();
        return;
    }

    if( 
        (this->winDraggedOnto == nullptr && this->winWhereDragged->HasRoot() == false)
        //|| droppedDst.where == DropResult::Where::Void
        )
    {
        // If we didn't drag into a window, and it was the only
        // thing previously in the DockWin (leaving nothing left
        // after it was dragged off) then just restore the window.

        this->CancelTabDragging(false);
        this->_AssertIsNeutralized();
        return;
    }

    this->StopCapture(this->winWithMouseCapture, false);

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
        // TODO: Figure out title change
        // It should no longer be the dummy name "Headphones"
        AppDock::GetApp().CreateTorn(
            this->nodeDragged, 
            this->winWhereDragged, 
            "Headphones");

        this->_ResolveToUpdateAfterDrag();
        this->nodeDragged = nullptr;
        this->dragType = DragType::Invalid;
        this->tabDragOwner = nullptr;
        this->winWhereDragged = nullptr;
        this->_AssertIsNeutralized();
        return;
    }

    assert(tabDropDst.where != DropResult::Where::Void);

    Node::Dest whereAdd = Node::Dest::Invalid;

    // NOTE: We may just want to get rid of one of the enums.
    switch(tabDropDst.where)
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
        this->tabDropDst.topOf,
		this->winWhereDragged,
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
    this->tabDropDst.Invalidate();

    if(!this->alreadyToreOffTab)
    {
        // If nothin happened, restore it, and don't hold on to it
        // so it won't get deleted.
        this->tabsBarDrag = nullptr;
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

void DragHelperMgr::CancelTabDragging(bool fromCaptureLoss)
{
    assert(!this->dragFlaggedAsFinished);
    assert(this->winWithMouseCapture != nullptr);
    assert(this->dragType == DragType::Tab);

    Layout* layout = &this->winWhereDragged->layout;
    LProps& lp = this->winWhereDragged->lprops;

    if(this->alreadyToreOffTab)
    { 
        assert(!this->dragUndo.empty());
        assert(!this->dragNodesInvolved.empty());

        this->StopCapture(this->winWithMouseCapture, fromCaptureLoss);
        this->dragNodesInvolved.clear();
        layout->UndoForget(this->dragUndo, lp);

        this->dragUndo.clear();
        this->nodeDragged->ShowWindow();

        // We leave tabsBarDrag non-null so that it will be deleted in the destructor
        this->tabDragOwner->ClearTabsBar();
        this->winWhereDragged->MaintainNodesTabsBar(tabDragOwner);
    }
    else
    {
        // If we entered into mouse capture from a mouse click, but never
        // mouse to mouse to start a tear operation.
        assert(this->dragUndo.empty());
        assert(this->dragNodesInvolved.empty());

        this->StopCapture(this->winWithMouseCapture, fromCaptureLoss);
    }

    this->nodeDragged = nullptr;

    this->tabsBarDrag = nullptr;
    this->tabDragOwner = nullptr;
    this->toUpdateAfterDrag = nullptr;
    this->winWhereDragged->ResizeLayout();
    this->winWhereDragged->SetCursor(wxNullCursor);
    this->winWhereDragged = nullptr;
    this->winDraggedOnto = nullptr;
    this->dragType = DragHelperMgr::DragType::Invalid;

    this->_AssertIsNeutralized();
}

void DragHelperMgr::_AssertIsDraggingSashCorrectly(bool shouldHaveCapture)
{
    assert(this->dragType == DragType::Sash);
    assert(this->winWhereDragged != nullptr);
    assert(!this->dragFlaggedAsFinished);
    assert(this->winWithMouseCapture != nullptr);

    if(shouldHaveCapture)
        assert(this->winWithMouseCapture->HasCapture());

    // Assertion checks that the wrong data wasn't touched
    assert(this->draggingCursorGraphic  == nullptr);
    assert(this->dropPreviewWin     == nullptr);
    assert(this->winDraggedOnto     == nullptr);
    assert(this->nodeDragged        == nullptr);
    assert(this->tabsBarDrag         == nullptr);
    assert(this->tabDragOwner       == nullptr);
    assert(this->dragUndo.empty());
    assert(this->dragNodesInvolved.empty());
    assert(this->toUpdateAfterDrag == nullptr);
    assert(this->alreadyToreOffTab == false);

    // Assertion checks that expected stuff is filled out.
    assert(this->preDragSashProps.size() > 0);
    assert(this->draggingSash       != nullptr);
    assert(this->sashDraggedParent  != nullptr);
    assert(!this->clickedClose);
}

void DragHelperMgr::_AssertIsDraggingTabCorrectly(bool shouldHaveCapture)
{
    assert(!this->dragFlaggedAsFinished);
    assert(this->winWithMouseCapture != nullptr);

    if(shouldHaveCapture)
        assert(this->winWithMouseCapture->HasCapture());

    assert(this->dragType == DragType::Tab);
    assert(this->winWhereDragged != nullptr);

    // Assertion checks that expected stuff is filled out.
    assert(this->preDragSashProps.empty());
    assert(this->draggingSash       == nullptr);
    assert(this->sashDraggedParent  == nullptr);

    assert((this->draggingCursorGraphic != nullptr) == this->alreadyToreOffTab);
    //assert((this->dropPreviewWin != nullptr) == this->startedDraggingTab);
    assert(this->nodeDragged != nullptr);
    assert(this->tabsBarDrag != nullptr  || this->alreadyToreOffTab);
    assert(this->tabDragOwner != nullptr);
    assert(this->dragUndo.empty()           != this->alreadyToreOffTab);
    assert(this->dragNodesInvolved.empty()  != this->alreadyToreOffTab);
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
    assert( this->preDragSashProps.empty());
    //assert( this->TabsBarDrag            == nullptr);
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

void DragHelperMgr::FinishSuccessfulSashDragging()
{
    // Not much to do except stop
    this->_AssertIsDraggingSashCorrectly(true);
    this->StopCapture(this->winWhereDragged, false);

    this->draggingSash = nullptr;

    this->preDragSashProps.clear();
    this->sashDraggedParent = nullptr;

    this->winWhereDragged->SetCursor(wxNullCursor);
    this->winWhereDragged = nullptr;

    this->dragType = DragType::Invalid;
    this->_AssertIsNeutralized();
}

void DragHelperMgr::CancelSashDragging(bool fromCaptureLoss)
{
    this->_AssertIsDraggingSashCorrectly(!fromCaptureLoss);

    // Restore the children sizes in the parent the sash modifies
    this->sashDraggedParent->ResizeChildrenByProportions(
        this->preDragSashProps, 
        this->winWhereDragged->lprops);

    this->winWhereDragged->RebuildSashes();

    this->preDragSashProps.clear();
    this->draggingSash = nullptr;
    this->winWhereDragged->Refresh();
    this->winWhereDragged->SetCursor(wxNullCursor);

    this->sashDraggedParent = nullptr;

    this->StopCapture(this->winWhereDragged, fromCaptureLoss);
}

DragHelperMgr::~DragHelperMgr()
{
    this->RemoveDropPreviewWin();
    this->RemoveDraggingCursor();

    // This may be left non-null to delete AFTER a tab drag operation
    // is completed. This is because it needs to be managed after the
    // entire drag operation because it (tabsBarDrag) will have had
    // the mouse capture so it has to stay alive through the entire 
    // process.
    if(this->tabsBarDrag != nullptr)
        delete this->tabsBarDrag;

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
    this->_AssertIsDraggingSashCorrectly(true);

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
    this->_AssertIsDraggingTabCorrectly(true);

    if(!this->alreadyToreOffTab)
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
                tabDragOwner->GetTabsBar()->Hide();
                // Very hackey, but we can't let the original TabsBar that was dragged
                // be destroyed while we're dragging.
                // 
                // We'll set it back later if nothing happened, or we'll destroy it if
                // the tab was moved around (in which case it will create another
                // TabsBar for itself and we can get rid of the original one.
                tabDragOwner->ForgetTabsBar();
                // Keep it around but hide it by moving it way off the window
                tabsBarDrag->SetPosition(wxPoint(0, -100000));
            }
            else if(tabDragOwner->type == Node::Type::Tabs)
            {
                if(tabDragOwner->children.size() > 2)
                { 
                    toUpdateAfterDrag = tabDragOwner;
                    tabsBarDrag = nullptr; // Don't delete it in ~DragHelperMgr
                }
                else
                {
                    toUpdateAfterDrag = tabDragOwner->ChildOtherThan(nodeDragged);

                    tabDragOwner->GetTabsBar()->Hide();
                    tabDragOwner->ForgetTabsBar();
                    tabsBarDrag->SetPosition(wxPoint(0, -100000));
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
                    this->winWhereDragged->MaintainNodesTabsBar(fundo.node);
                    doLayout = true;
                }
            }


            this->alreadyToreOffTab = true;      
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
                tabDropDst = 
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
                    tabDropDst = 
                        DropResult(
                            DropResult::Where::Onto, 
                            nullptr, 
                            nullptr, 
                            wxPoint(0,0), 
                            otherClientsz);

                    SetDropPreviewWin(
                        tabDropDst.dropRgnPt, 
                        tabDropDst.dropRgnSz, 
                        this->winWhereDragged, 
                        winDraggedOnto->GetDockWin());

                    foundTarg = true;
                }
                else
                {
                    //SetDropPreviewWin(--)


                    tabDropDst = 
                        this->winDraggedOnto->GetDockWin()->layout.ScanForDrop(
                            othMouse, 
                            this->winWhereDragged->lprops);

                    if(tabDropDst.where == DropResult::Where::Left)
                    {
                        SetDropPreviewWin(
                            tabDropDst.topOf->cachePos, 
                            wxSize(
                                this->winWhereDragged->lprops.dropEdgeWidth, 
                                tabDropDst.topOf->cacheSize.y),
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(tabDropDst.where == DropResult::Where::Right)
                    {
                        SetDropPreviewWin(
                            wxPoint(
                                tabDropDst.topOf->CacheRight() - this->winWhereDragged->lprops.dropEdgeWidth,
                                tabDropDst.topOf->cachePos.y), 
                            wxSize(
                                this->winWhereDragged->lprops.dropEdgeWidth, 
                                tabDropDst.topOf->cacheSize.y),
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(tabDropDst.where == DropResult::Where::Top)
                    {
                        SetDropPreviewWin(
                            tabDropDst.topOf->cachePos,
                            wxSize(
                                tabDropDst.topOf->cacheSize.x, 
                                this->winWhereDragged->lprops.dropEdgeWidth),
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(tabDropDst.where == DropResult::Where::Bottom)
                    {
                        SetDropPreviewWin(
                            wxPoint(
                                tabDropDst.topOf->cachePos.x,
                                tabDropDst.topOf->CacheBot() - this->winWhereDragged->lprops.dropEdgeWidth),
                            wxSize(
                                tabDropDst.topOf->cacheSize.x,
                                this->winWhereDragged->lprops.dropEdgeWidth),
                            this->winWhereDragged,
                            winDraggedOnto->GetDockWin());


                        foundTarg = true;
                    }
                    else if(tabDropDst.where == DropResult::Where::Onto)
                    {
                        SetDropPreviewWin(
                            tabDropDst.dropRgnPt,
                            tabDropDst.dropRgnSz,
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

void DragHelperMgr::StopCapture(wxWindow* requester, bool fromCaptureLoss)
{
    assert(requester != nullptr);
    assert(this->winWithMouseCapture == requester);
    assert(!this->dragFlaggedAsFinished);

    if(fromCaptureLoss)
    {
        assert(!this->winWithMouseCapture->HasCapture());
        this->winWithMouseCapture = nullptr;
        this->dragFlaggedAsFinished = true;
    }
    else
    {
        // wxWindow::HasCapture() has some weird semantics that aren't 
        // 100% reliable here.
        assert(this->winWithMouseCapture->HasCapture());

        // Release the mouse will invoke callbacks which may reference this object, so
        // we need to neutralize this stuff before releasing the mouse capture.
        wxWindow* toRelease = this->winWithMouseCapture;
        this->winWithMouseCapture = nullptr;
        this->dragFlaggedAsFinished = true;
        toRelease->ReleaseMouse();
    }
}

void DragHelperMgr::SetDragPreviewOlyWin(const wxPoint& whereAt)
{
    assert(this->dragType == DragType::Tab);

    if(this->draggingCursorGraphic == nullptr)
    {
        this->draggingCursorGraphic = new DragPreviewOlyWin(this->winWhereDragged, this->winWhereDragged);
        this->draggingCursorGraphic->SetCursor(*wxCROSS_CURSOR);
        this->draggingCursorGraphic->Show();
    }
    this->draggingCursorGraphic->SetPosition(whereAt);
}