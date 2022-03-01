#include "DockWin.h"
#include <queue>
#include "AppDock.h"
#include "Layout/Sash.h"
#include "Layout/TabBar.h"
#include "BarDrag.h"

#include "TopDockWin.h"

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

BarDrag*    DockWin::barDrag            = nullptr;
BarDrop*    DockWin::dropPreviewWin     = nullptr;
TopDockWin* DockWin::winDraggedOnto     = nullptr;
DropResult  DockWin::droppedDst;
Node*       DockWin::nodeDragged        = nullptr;
Node*       DockWin::sashDraggedParent  = nullptr;
wxPoint     DockWin::lastLeftClick;
TabBar*     DockWin::tabBarDrag         = nullptr;
std::set<Node*>     DockWin::dragNodesInvolved;
std::vector<float>  DockWin::sashPreDragProps;
std::vector<Layout::ForgetUndo> DockWin::dragUndo;

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
    //AllocConsole();
    this->owner = owner;
    this->SetBackgroundColour(wxColour(200, 200, 200));
    this->layout._SetContentWin(this->m_hWnd);
}

DockWin::~DockWin()
{}

Node * DockWin::AddToLayout(HWND hwnd, Node* reference, Node::Dest refDock)
{
    Node* n = this->layout.Add(hwnd, reference, refDock);
    this->MaintainNodesTabBar(n);
    return n;
}

bool DockWin::StealToLayout(Node* n, Node* reference, Node::Dest refDock)
{
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
    wxPoint pos = evt.GetPosition();

    Sash* olySash = this->SashAtPoint(pos);
    if(olySash != nullptr)
    {
        this->draggingSash = olySash;
        this->dragOffset = pos - olySash->pos;
        
        this->mouseState = MouseState::DragSash;
        // Both parents should be the same
        sashDraggedParent = this->draggingSash->r[0]->parent; 

        sashPreDragProps.clear();
        for(size_t i = 0; i < sashDraggedParent->children.size(); ++i)
        { 
            sashPreDragProps.push_back(
                sashDraggedParent->children[i]->proportion);
        }

        this->CaptureMouse();
        return;
    }
}

void DockWin::OnMouseLUp(wxMouseEvent& evt)
{
    if(this->mouseState == MouseState::DragSash)
    { 
        // Nothing needs to be done
    }

    this->FinishMouseDrag();
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
    wxPoint pt = evt.GetPosition();
    wxPoint delta = pt - this->lastMouse;
    this->lastMouse = pt;

    bool olaySash = false;
    wxPoint mousePos = evt.GetPosition();

    if(this->mouseState == MouseState::DragSash)
    {
        if(this->draggingSash->dir == Sash::Dir::Horiz)
        {
            Node* nleft = this->draggingSash->left;
            Node* nright = this->draggingSash->right;
            wxSize minLeft = nleft->CalculateMinSize(this->lprops);
            wxSize minRight = nright->CalculateMinSize(this->lprops);

            // Check if there's even enough space to do a 
            // dragging operation.
            if(nleft->cacheSize.x + nright->cacheSize.x <= minLeft.x + minRight.x)
                return;

            this->draggingSash->SlideHoriz(delta.x);
            this->layout.Resize(nleft, this->lprops);
            this->layout.Resize(nright, this->lprops);
            nleft->parent->CalculateChildProportions_Cache();
            this->Refresh();
            
        }
        else if(this->draggingSash->dir == Sash::Dir::Vert)
        {
            Node* ntop = this->draggingSash->top;
            Node* nbot = this->draggingSash->bot;
            wxSize minTop = ntop->CalculateMinSize(this->lprops);
            wxSize minBot = nbot->CalculateMinSize(this->lprops);

            if(ntop->cacheSize.y + nbot->cacheSize.y < minTop.y + minBot.y)
                return;

            this->draggingSash->SlideVert(delta.y);
            this->layout.Resize(ntop, this->lprops);
            this->layout.Resize(nbot, this->lprops);
            ntop->parent->CalculateChildProportions_Cache();
            this->Refresh();
        }
    }

    if(barDrag != nullptr)
        barDrag->Raise();

    Sash* olySash = this->SashAtPoint(mousePos);
    if(olySash != nullptr)
        this->SetCursor(*wxCROSS_CURSOR);
    else
        this->SetCursor(wxNullCursor);
}

void DockWin::OnMouseCaptureChanged(wxMouseCaptureChangedEvent& evt)
{
    this->FinishMouseDrag();
}

void DockWin::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
    this->FinishMouseDrag();
}

void DockWin::OnKeyDown(wxKeyEvent& evt)
{
    Node * n = this->layout.GetNodeAt(this->lastMouse);

    if(evt.GetKeyCode() == 'W')
    {
        n = this->AddToLayout(NULL, n, Node::Dest::Above);
    }
    else if(evt.GetKeyCode() == 'A')
    {
        n = this->AddToLayout(NULL, n, Node::Dest::Left);
    }
    else if(evt.GetKeyCode() == 'S')
    {
        n = this->AddToLayout(NULL, n, Node::Dest::Below);
    }
    else if(evt.GetKeyCode() == 'D')
    {
        n = this->AddToLayout(NULL, n, Node::Dest::Right);
    }
    else if(evt.GetKeyCode() == 'Q')
    {
        // For debugging
        DropResult dr = this->layout.ScanForDrop(evt.GetPosition(), this->lprops);
    }
    else if(evt.GetKeyCode() == WXK_DELETE)
    {
        if(n != nullptr && n->type == Node::Type::Window)
        {
            if(this->layout.DeleteWindow(n) == true)
                this->ResizeLayout();
  
            return;
        }
    }
    else if(evt.GetKeyCode() == WXK_ESCAPE)
    {
        if(this->mouseState == MouseState::DragSash)
        {
            if(sashDraggedParent != nullptr)
            {
                // Restore the children sizes in the parent the sash modifies
                sashDraggedParent->ResizeChildrenByProportions(
                    sashPreDragProps, 
                    this->lprops);

                this->Refresh();
            }
        }
        else if(this->mouseState == MouseState::DragWin)
        {
            // Handled in OnDelegatedEscape.
        }

        if(this->HasCapture())
            this->ReleaseMouse();
    }

    if(n != nullptr)
    {
        this->ResizeLayout();
        this->Refresh();
    }
}

void DockWin::OnKeyUp(wxKeyEvent& evt)
{
}

bool DockWin::ReleaseNodeWin(Node* pn)
{
    if(this->layout.ReleaseWindow(pn) == false)
        return false;

    this->_ReactToNodeRemoval();
    return true;
}

bool DockWin::DettachNodeWin(Node* pn)
{
    HWND hwnd = pn->Hwnd();
    if(hwnd == NULL)
        return false;

    if(this->layout.ReleaseWindow(pn) == false)
        return false;

    AppDock::GetApp().CreateWindowFromHwnd(hwnd);

    this->_ReactToNodeRemoval();
    return true;
}

bool DockWin::CloseNodeWin(Node* pn)
{
    if(this->layout.DeleteWindow(pn) == false)
        return false;

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

void DockWin::TabClickStart(TabBar* tbInvoker, Node* node, Node* tabOwner)
{
    this->barDrag = new BarDrag(this, this);
    this->barDrag->SetPosition(wxGetMousePosition());
    //this->barDrag->Show();

    this->lastMouse     = this->ScreenToClient(wxGetMousePosition());
    this->lastLeftClick = this->lastMouse;
    // Was any tab clicked? If so, prepare it to be dragged.
    //Node* nTabClicked = this->GetNodeTab(pos);
    
    nodeDragged = node;
    // Set to anticipate, don't do anything yet (that MouseState::DragWin
    // would do) until they actually start dragging.
    this->mouseState = MouseState::DragWin_Anticipate;

    // The way we have the TabBar managing the mouse capture, but 
    // also need to show the tabs, except it needs to be removed 
    // because its tabs are removed ... sigh...
    // So we need to keep it around for the end of the drag, but
    // it also can't be tied to the node anymore (we're generalizing
    // also for tab nodes for now - if another one is needed, then
    // another one will be recreated.
    //
    // TODO: Word better.
    tabBarDrag = tbInvoker; // Will be cleaned up after drag.
    tabOwner->ForgetTabBar();
    //
    this->MaintainNodesTabBar(tabOwner);
    TabBar* tbRepl = tabOwner->GetTabBar();
    tbRepl->SetPosition(tabBarDrag->GetPosition());
    tbRepl->SetSize(tabBarDrag->GetSize());
    tbRepl->Show();
    //
    tbInvoker->SetPosition(wxPoint(0, -1000));

    lastLeftClick = this->ScreenToClient(wxGetMousePosition());
    tbInvoker->CaptureMouse();
}

void DockWin::TabClickMotion()
{
    this->lastMouse = this->ScreenToClient(wxGetMousePosition());

    if(this->mouseState == MouseState::DragWin_Anticipate)
    {
        const float tearDist = 3.0f;
        //
        float difX = lastLeftClick.x - this->lastMouse.x;
        float difY = lastLeftClick.y - this->lastMouse.y;
        float pxDragDst = sqrt(difX * difX + difY * difY);

        bool doLayout = false;
        if(pxDragDst > tearDist)
        { 
            if(this->layout._ForgetWindow(
                nodeDragged, 
                Node::ForgetAction::Hide, 
                dragUndo,
                dragNodesInvolved, 
                true))
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
                    this->MaintainNodesTabBar(fundo.node);
                    doLayout = true;
                }
            }

            this->barDrag->Show();
            this->SetCursor(*wxCROSS_CURSOR);
            this->mouseState = MouseState::DragWin;        
        }
        if(doLayout == true)
            this->ResizeLayout();
    }
    else if(this->mouseState == MouseState::DragWin)
    {
        wxPoint globMouse = wxGetMousePosition();
        if(barDrag != nullptr)
            barDrag->SetPosition(globMouse);

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
                    this, 
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
                        this, 
                        winDraggedOnto->GetDockWin());

                    foundTarg = true;
                }
                else
                {
                    //SetDropPreviewWin(--)


                    droppedDst = winDraggedOnto->GetDockWin()->layout.ScanForDrop(othMouse, this->lprops);
                    if(droppedDst.where == DropResult::Where::Left)
                    {
                        SetDropPreviewWin(
                            droppedDst.topOf->cachePos, 
                            wxSize(
                                this->lprops.dropEdgeWidth, 
                                droppedDst.topOf->cacheSize.y),
                            this,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(droppedDst.where == DropResult::Where::Right)
                    {
                        SetDropPreviewWin(
                            wxPoint(
                                droppedDst.topOf->CacheRight() - this->lprops.dropEdgeWidth,
                                droppedDst.topOf->cachePos.y), 
                            wxSize(
                                this->lprops.dropEdgeWidth, 
                                droppedDst.topOf->cacheSize.y),
                            this,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(droppedDst.where == DropResult::Where::Top)
                    {
                        SetDropPreviewWin(
                            droppedDst.topOf->cachePos,
                            wxSize(
                                droppedDst.topOf->cacheSize.x, 
                                this->lprops.dropEdgeWidth),
                            this,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                    else if(droppedDst.where == DropResult::Where::Bottom)
                    {
                        SetDropPreviewWin(
                            wxPoint(
                                droppedDst.topOf->cachePos.x,
                                droppedDst.topOf->CacheBot() - this->lprops.dropEdgeWidth),
                            wxSize(
                                droppedDst.topOf->cacheSize.x,
                                this->lprops.dropEdgeWidth),
                            this,
                            winDraggedOnto->GetDockWin());


                        foundTarg = true;
                    }
                    else if(droppedDst.where == DropResult::Where::Onto)
                    {
                        SetDropPreviewWin(
                            droppedDst.dropRgnPt,
                            droppedDst.dropRgnSz,
                            this,
                            winDraggedOnto->GetDockWin());

                        foundTarg = true;
                    }
                }
            }
        }

        if(foundTarg == false)
        {
            // Hide it, but don't destroy it because we might end up
            // recreating it soon as the user continues to drag the
            // mouse.
            RemDropPreviewWin(false);
        }
    }
}

void DockWin::TabClickEnd()
{
    do
    { 
        if(this->mouseState == MouseState::DragWin_Anticipate)
        {
            // If we're here, the user clicked on a tab but never dragged
            // it (which would have transitioned the state to MouseState::DragWin).
            // Thus, a tab was clicked.
            if(nodeDragged != nullptr && nodeDragged->IsTabChild())
            {
                // Cache in case we loose it from keyboard focus changing
                Node* nCur = nodeDragged;
                Node* nTabSys = nCur->parent;

                assert(nTabSys != nullptr);
                assert(nTabSys->type == Node::Type::Tabs);
                bool sel = nTabSys->SelectTab(nCur);
                assert(sel);

                this->layout.Resize(nTabSys, this->lprops);
                this->Refresh();
            }
        }
        else if(this->mouseState == MouseState::DragWin)
        {
            if(winDraggedOnto == nullptr)
            {
                // If we didn't drag into a window, and it was the only
                // thing previously in the DockWin (leaving nothing left
                // after it was dragged off) then just restore the window.
                if(this->HasRoot() == false)
                    this->_CancelDrag();
                else
                {
                    // Or else it has multiple items and we tore an item out.
                    // In that case, detach what the user tore off.
                    dragNodesInvolved.erase(this->nodeDragged);
                    AppDock::GetApp().CreateTorn(this->nodeDragged, "Headphones");
                }

                break;
            }

            if(droppedDst.where == DropResult::Where::Void)
                break;

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

            // This gets a little weird, but we need to cache the variables
            // involved with dragging the mouse before we confirm it, because
            // the operation shuffles around some windows, which can force
            // an early mouse-capture loss which will clear the drag variables
            // we need before we're done with them.
            Node* oldNodeDragged    = nodeDragged;
            Node* oldTopOf          = droppedDst.topOf;
            this->_ConfirmDrag();
            oldNodeDragged->parent = nullptr; // To not trigger asserts
            winDraggedOnto->GetDockWin()->StealToLayout(
                oldNodeDragged,
                oldTopOf,
                whereAdd);

            if(this->HasRoot() == false)
                this->owner->Destroy();
        }
    }
    while(false);

    this->FinishMouseDrag();
}

void DockWin::TabClickCancel()
{
    this->OnDelegatedEscape();
}

void DockWin::FinishMouseDrag()
{
    if(barDrag != nullptr)
    {
        delete barDrag;
        barDrag = nullptr;
    }

    RemDropPreviewWin(true);

    if(this->HasCapture() == true)
        this->ReleaseMouse();

    winDraggedOnto = nullptr;

    this->droppedDst.Invalidate();

    // Anything involved and removed from a drag node that we 
    // want kept should have been deleted before FinishMouseDrag 
    // was called.
    for(Node* n : dragNodesInvolved)
    { 
        n->ForgetHWND(Node::ForgetAction::Delete);
        delete n;
    }
    dragNodesInvolved.clear();

    sashDraggedParent   = nullptr;
    nodeDragged         = nullptr;
    sashPreDragProps.clear();
    dragUndo.clear();

    this->SetCursor(wxNullCursor);

    TabBar* cachedTB = this->tabBarDrag;
    this->tabBarDrag = nullptr;
    if(cachedTB != nullptr)
    { 
        if(cachedTB->HasCapture() == true)
            cachedTB->ReleaseMouse();
        //
        if(cachedTB != nullptr)
        {
            // We could be here from a TabBar call when it releases mouse capture - 
            // which is not allowed to deleted itself at that time. So instead
            // we Close() to queue deletion.
            cachedTB->Close(true);
        }
    }

    this->mouseState = MouseState::Normal;
}

void DockWin::OnDelegatedEscape()
{
    if(this->mouseState == MouseState::DragWin)
    {
        // If we're dragging a node around and the user presses escape,
        // restore the layout to how it looked before dragging.
        if(this->nodeDragged != nullptr)
            this->_CancelDrag();
    }

    if(this->HasCapture() == true)
        this->ReleaseMouse();
}

void DockWin::_CancelDrag()
{
    assert(dragUndo.size() > 0);
    dragNodesInvolved.clear();
    this->layout.UndoForget(dragUndo, this->lprops);
    this->nodeDragged->ShowWindow();
    this->ResizeLayout();
}

void DockWin::_ConfirmDrag()
{
    if(this->nodeDragged == nullptr)
        return;

    dragNodesInvolved.erase(this->nodeDragged);
    nodeDragged = nullptr;
    dragUndo.clear();
}

void DockWin::_ReactToNodeRemoval()
{
    if(this->HasRoot() == false)
        this->Close();
    else
        this->ResizeLayout();
}

void DockWin::SetDropPreviewWin(
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

void DockWin::RemDropPreviewWin(bool destroy)
{
    if(dropPreviewWin == nullptr)
        return;

    if(destroy == true)
    {
        // Cache and delete, or else interupting stuff
        // could send us in here twice due to obscure
        // UI shenanigans.
        BarDrop * bd = dropPreviewWin;
        dropPreviewWin = nullptr;
        delete bd;
    }
    else
    {
        dropPreviewWin->Hide();
    }
}

bool DockWin::_TestValidity()
{
    return true;
}