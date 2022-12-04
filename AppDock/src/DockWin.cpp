#include "DockWin.h"
#include <queue>
#include "AppDock.h"
#include "Layout/Sash.h"
#include "Layout/TabsBar.h"
#include "DragPreviewOlyWin.h"
#include "TopDockWin.h"

void _DockObserverBus::PublishOnLost(HWND hwnd, LostReason lr)
{
	if (this->eventOnLost)
		this->eventOnLost(hwnd, lr);
}

void _DockObserverBus::PublishOnAdded(HWND hwnd, Node* node)
{
	if(this->eventOnAdded)
		this->eventOnAdded(hwnd, node);
}

void _DockObserverBus::PublishTitleModified(Node* node)
{
    if(this->eventOnTitleMod)
        this->eventOnTitleMod(node);
}

void _DockObserverBus::SetEventOnLost(fntyOnLost fn)
{
	this->eventOnLost = fn;
}

void _DockObserverBus::SetEventOnAdded(fntyOnAdded fn)
{
	this->eventOnAdded = fn;
}

void _DockObserverBus::SetEventTitleModified(fntyOnTitleMod fn)
{
    this->eventOnTitleMod = fn;
}

int DockWin::instCtr = 0;

BEGIN_EVENT_TABLE(DockWin, wxWindow)
    EVT_PAINT                   (DockWin::OnDraw                )
    EVT_SIZE                    (DockWin::OnSize                )
    EVT_LEFT_DOWN               (DockWin::OnMouseLDown          )
    EVT_LEFT_UP                 (DockWin::OnMouseLUp            )
    EVT_MIDDLE_DOWN             (DockWin::OnMouseMDown          )
    EVT_MIDDLE_UP               (DockWin::OnMouseMUp            )
    EVT_RIGHT_DOWN              (DockWin::OnMouseRDown          )
    EVT_RIGHT_UP                (DockWin::OnMouseRUp            )
    EVT_MOTION                  (DockWin::OnMouseMotion         )
    EVT_MOUSE_CAPTURE_CHANGED   (DockWin::OnMouseCaptureChanged )
    EVT_MOUSE_CAPTURE_LOST      (DockWin::OnMouseCaptureLost    )
    EVT_ENTER_WINDOW            (DockWin::OnMouseEnter          )
    EVT_LEAVE_WINDOW            (DockWin::OnMouseLeave          )
    EVT_KEY_DOWN                (DockWin::OnKeyDown             )
    EVT_KEY_UP                  (DockWin::OnKeyUp               )
END_EVENT_TABLE()

DragHelperMgrPtr DockWin::dragggingMgr;

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
{
    --instCtr;
    assert(instCtr >= 0);
}

std::set<HWND> DockWin::AllDockedWindows() const
{
    std::set<HWND> ret;
    for (auto it : this->layout.hwndLookup)
        ret.insert(it.first);
    return ret;
}

Node * DockWin::AddToLayout(HWND hwnd, Node* reference, Node::Dest refDock)
{
    Node* n = this->layout.Add(hwnd, reference, refDock);
	assert(n != nullptr);

	this->PublishOnAdded(hwnd, n);
	
    this->MaintainNodesTabsBar(n);
    return n;
}

bool DockWin::StealToLayout(Node* n, Node* reference, DockWin* stolenFrom, Node::Dest refDock)
{
    // We cannot be dragging with a mouse capture if here,
    // because Steal() will force the OS to exit it.
    ASSERT_ISNODEWIN(n);
    assert(this->dragggingMgr == nullptr || this->dragggingMgr->dragFlaggedAsFinished);

    n->ClearTabsBar();

    bool ret = 
        this->layout.Steal(
            n, 
            reference, 
            refDock, 
            this->lprops);

    if(ret == false)
        return false;

    stolenFrom->PublishOnLost(n->Hwnd(), LostReason::ManualMoved);
	this->PublishOnAdded(n->Hwnd(), n);

    if(n->IsTabChild() == true)
    {
        Node* nTabSys   = n->parent;
        bool sel        = nTabSys->SelectTab(n);
        assert(sel == true);

        this->MaintainNodesTabsBar(nTabSys);
        nTabSys->GetTabsBar()->Refresh();
    }
    else
        this->MaintainNodesTabsBar(n);

    this->ResizeLayout(true, true);
    return true;
}

void DockWin::StealRoot(Node* steal, DockWin* stolenFrom)
{
    assert(this->layout.root == nullptr);
    ASSERT_ISNODEWIN(steal);

    HWND hwnd = steal->Hwnd();
    if(hwnd != NULL)
    { 
        ::SetParent(hwnd, this->GetHWND());
        this->layout.hwndLookup[hwnd] = steal;
    }

    this->layout.root = steal;
    steal->parent = nullptr;
    steal->ClearTabsBar();
    this->MaintainNodesTabsBar(steal);

    stolenFrom->PublishOnLost(steal->Hwnd(), LostReason::ManualMoved);
	this->PublishOnAdded(steal->Hwnd(), steal);
	
    steal->ShowWindow();
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

void DockWin::ResizeLayout(bool refresh, bool rebuildSashes)
{
    this->ResizeLayout(
        this->GetClientSize(), 
        refresh, 
        rebuildSashes);
}

void DockWin::ResizeLayout(const wxSize& sz, bool refresh, bool rebuildSashes)
{
    this->layout.Resize(sz, this->lprops);

    if(refresh == true)
        this->Refresh();

    if(rebuildSashes == true)
        this->layout.RebuildSashes(this->lprops);
}

void DockWin::MaintainNodesTabsBar(Node* pn)
{
    assert(pn != nullptr);
    if(
        pn->type == Node::Type::Horizontal || 
        pn->type == Node::Type::Vertical)
    {
        // Shouldn't ever do anything, sanity operation.
        pn->ClearTabsBar();
        return;
    }

    if(!pn->IsTabChild())
    {
        if (pn->GetTabsBar() == nullptr)
        {
            TabsBar* tb = new TabsBar(this, pn);
            pn->SetTabsBar(tb);
        }
    }
    else
    {
        // For windows that belong to a tab. They don't own the tab bar,
        // the parent tab system does.
        //
        // This isn't expected to do anything - sanity guard.
        pn->ClearTabsBar();
    }
}

void DockWin::ClearDocked(std::optional<LostReason> lr)
{
	if(lr.has_value())
    { 
	    std::vector<HWND> hwnds = this->layout.CollectHWNDs();
        this->layout.Clear();
	    for (HWND hwndRm : hwnds)
		    this->PublishOnLost(hwndRm, lr.value());
    }
    else
        this->layout.Clear();
}

void DockWin::ReleaseAll(bool clear)
{
    std::vector<Node*> wndNodes;
    this->layout.CollectHWNDNodes(wndNodes);

    for(Node* n : wndNodes)
    { 
        if(n->ReleaseHWND())
			this->PublishOnLost(n->Hwnd(), LostReason::ManualReleased);
    }

    if(clear == true)
        this->ClearDocked();
}

void DockWin::OnDraw(wxPaintEvent& paint)
{
    // Debug code to render layouts lots
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
        this->dragggingMgr = DragHelperMgrPtr(new DragHelperMgr(this, olySash, pos));
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
    if(
        this->dragggingMgr != nullptr && 
        this->dragggingMgr->dragType == DragHelperMgr::DragType::Sash)
    {
        this->dragggingMgr->CancelSashDragging(false);
        this->FinishMouseDrag();
    }
}

void DockWin::OnMouseRUp(wxMouseEvent& evt)
{
}

void DockWin::OnMouseMotion(wxMouseEvent& evt)
{
    if(this->dragggingMgr != nullptr)
    {
        // If dragging, the DraggingMgr will be responsible for updating the
        // mouse cursor graphic ...
        assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Sash);
        this->dragggingMgr->HandleMouseMove();
    }
    else
    {
        this->UpdateCursorFromMouseOverPoint(evt.GetPosition());
    }
}

void DockWin::OnMouseEnter(wxMouseEvent& evt)
{
    this->UpdateCursorFromMouseOverPoint(evt.GetPosition());
}

void DockWin::OnMouseLeave(wxMouseEvent& evt)
{
    this->SetCursor(wxNullCursor);
}

void DockWin::OnMouseCaptureChanged(wxMouseCaptureChangedEvent& evt)
{
    if(this->dragggingMgr != nullptr)
    { 
        if(!this->dragggingMgr->dragFlaggedAsFinished)
        {
            assert(this->dragggingMgr->dragType != DragHelperMgr::DragType::Invalid);

            if(this->dragggingMgr->dragType == DragHelperMgr::DragType::Sash)
            {
                // > ☐ SASH_DRAG_ba0d1d458dcc: Doing an alt-tab while dragging cancels the drag.
                // > ☐ SASH_DRAG_1e45f4aeb499: Cancelling a sash drag reset the sash to where it was before the drag.
                this->dragggingMgr->CancelSashDragging(true);
            }
            else if(this->dragggingMgr->dragType == DragHelperMgr::DragType::Tab)
                this->dragggingMgr->CancelTabDragging(true);
            else
                assert(!"Illegal unhandled case in DockWin::OnMouseCaptureChanged()");
        }
        this->FinishMouseDrag();
    }
}

void DockWin::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{}

void DockWin::OnKeyDown(wxKeyEvent& evt)
{
    if(evt.GetKeyCode() == WXK_ESCAPE)
    {
        if(this->HasCapture())
        {
            assert(this->dragggingMgr != nullptr);
            assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Sash);
			
            this->dragggingMgr->CancelSashDragging(false);
            this->FinishMouseDrag();
            this->Refresh(false);
        }
        else if(
            this->dragggingMgr != nullptr && 
            this->dragggingMgr->dragType == DragHelperMgr::DragType::Tab)
        {
            this->TabClickCancel();
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
            n->SelectTab(n->selectedTabIdx);
        }
        else if(
            n->type == Node::Type::Window && 
            (n->parent == nullptr || n->parent->type != Node::Type::Tabs))
        {
            dw->MaintainNodesTabsBar(n);
        }
    }
}

bool DockWin::ReleaseNodeWin(Node* pn)
{
    ASSERT_ISNODEWIN(pn);
	HWND origHwnd = pn->Hwnd();
	
    std::set<Node*> involved;
    if(this->layout.ReleaseWindow(pn, &involved) == false)
        return false;

    _ProcessInvolvedFromRem(involved, this);
    this->_ReactToNodeRemoval();
	this->PublishOnLost(origHwnd, LostReason::ManualReleased);
    return true;
}

bool DockWin::DetachNodeWin(Node* pn)
{
    ASSERT_ISNODEWIN(pn);
    
    HWND hwnd = pn->Hwnd();
    if(hwnd == NULL)
        return false;

    std::set<Node*> involved;
    if(this->layout.ReleaseWindow(pn, &involved) == false)
        return false;

    this->PublishOnLost(hwnd, LostReason::ManualMoved);
    AppDock::GetApp().CreateWindowFromHwnd(hwnd);

    _ProcessInvolvedFromRem(involved, this);
    this->_ReactToNodeRemoval();
    return true;
}

bool DockWin::DetachNodeWin(HWND hwnd)
{
    auto itFind = this->layout.hwndLookup.find(hwnd);
    if (itFind == this->layout.hwndLookup.end())
        return false;

    Node* nodeDetaching = itFind->second;
    assert(nodeDetaching != nullptr);

    return this->DetachNodeWin(nodeDetaching);
}

bool DockWin::CloseNodeWin(Node* pn)
{
    assert(this->layout.root != nullptr);
    ASSERT_ISNODEWIN(pn);

    HWND origHwnd = pn->Hwnd();

    std::set<Node*> involved;
    if(this->layout.DeleteWindow(pn, & involved) == false)
        return false;

    _ProcessInvolvedFromRem(involved, this);
    this->_ReactToNodeRemoval();
	this->PublishOnLost(origHwnd, LostReason::ManualClose);
    return true;
}

Node* DockWin::CloseNodeWin(HWND hwnd)
{
	Node* n = this->layout.GetNodeFrom(hwnd);
	if (n == nullptr)
		return nullptr;

	if (this->CloseNodeWin(n) == false)
    {
		assert(!"Failed to close node");
		return nullptr;
    }
	
	return n;
}

Node* DockWin::CloneNodeWin(Node* pn)
{
    if(pn->win == NULL)
        return nullptr;

    HWND hwnd = 
        AppDock::CreateSpawnedWindow(pn->cmdLine);

    if(hwnd == NULL)
        return nullptr;

    Node* nClone = this->AddToLayout( hwnd, pn, Node::Dest::Into);
    nClone->cmdLine = pn->cmdLine;
    this->ResizeLayout(); // Mainly to force redraw

    // Creating a window and placing it onto a window will always result
    // in the parent being a tabs group.
    Node* parent = nClone->parent;
    ASSERT_ISNODETABS(parent);
    parent->UpdateTabWindowVisibility();

    return nClone;
}

void DockWin::TabClickStart(TabsBar* tbInvoker, Node* node, Node* tabOwner, bool closePressed)
{
    assert(this->dragggingMgr == nullptr);

    tbInvoker->CaptureMouse();
    this->dragggingMgr = DragHelperMgrPtr(new DragHelperMgr(this, tbInvoker, closePressed, node, tabOwner));
    this->dragggingMgr->_AssertIsDraggingTabCorrectly(true);
}

void DockWin::TabClickMotion()
{
    assert(this->dragggingMgr != nullptr);
    assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Tab);

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

void DockWin::UpdateCursorFromMouseOverPoint(const wxPoint& pt)
{
    bool inSash = false;
    for (Sash* s : this->layout.sashes)
    {
        if (s->Contains(pt))
        {
            inSash = true;
            break;
        }
    }
    if (inSash)
        this->SetCursor(*wxCROSS_CURSOR);
    else
        this->SetCursor(wxNullCursor);

}

void DockWin::OnDelegatedEscape()
{
    assert(this->dragggingMgr);
    assert(this->dragggingMgr->dragType == DragHelperMgr::DragType::Tab);

    this->dragggingMgr->CancelTabDragging(false);
    this->FinishMouseDrag();
}

void DockWin::RefreshWindowTitlebar(HWND hwnd)
{
    Node* n = this->layout.GetNodeFrom(hwnd);
    ASSERT_ISNODEWIN(n);
    if(n == nullptr)
        return;

    if (n->IsTabChild())
    {
        n = n->parent;
        ASSERT_ISNODETABS(n);
    }
	
    TabsBar* tb = n->GetTabsBar();
    if (tb == nullptr)
    {
        this->MaintainNodesTabsBar(n);
        tb = n->GetTabsBar();
    }
    assert(tb != nullptr);

    tb->Refresh(false);
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
                if(n->GetTabsBar() != nullptr)
                    jsRet["tab"] = n->GetTabsBar()->id;
                break;
                
            case Node::Type::Horizontal:
                jsRet["ty"] = "horiz";
                break;

            case Node::Type::Vertical:
                jsRet["ty"] = "vert";
                break;

            case Node::Type::Tabs:
                jsRet["ty"] = "tabs";
                jsRet["sel"] = n->selectedTabIdx;
                if(n->GetTabsBar() != nullptr)
                    jsRet["tab"] = n->GetTabsBar()->id;
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