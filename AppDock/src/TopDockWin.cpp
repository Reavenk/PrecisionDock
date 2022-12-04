#include "TopDockWin.h"
#include "DockWin.h"
#include "AppDock.h"
#include "Utils/AppUtils.h"

int TopDockWin::_InstCtr = 0;

wxBEGIN_EVENT_TABLE(TopDockWin, wxFrame)
    EVT_MENU(wxID_EXIT,  TopDockWin::OnExit)
    EVT_MENU((int)CMDID::ToggleStatusbar,   TopDockWin::OnMenu_ToggleStatusbar  )
    EVT_MENU((int)CMDID::ReleaseAll,        TopDockWin::OnMenu_ReleaseAll       )
    EVT_MENU((int)CMDID::DetachAll,         TopDockWin::OnMenu_DetachAll       )
wxEND_EVENT_TABLE()

TopDockWin::TopDockWin(const wxPoint& pos, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, "PrecisionDock", pos, size)
{
    this->cachedHWND = this->GetHWND();
    AppDock::GetApp().RegisterToplevelOwned(this->cachedHWND);

    ++_InstCtr;

    wxMenu* menuFile = new wxMenu;
    {
        menuFile->Append((int)CMDID::ReleaseAll,    "Release All",  "Release all windows in this collection");
        menuFile->Append((int)CMDID::DetachAll,     "Detach All",   "Detach all windows in this collection into their own collections.");
        menuFile->Append(wxID_EXIT,                 "Close All",    "Close all windows in this collection.");
    }
    wxMenu* menuView = new wxMenu;
    {
        menuView->Append((int)CMDID::ToggleStatusbar, "Toggle Statusbar", "Show and hide the status bar (this bar down below)");
    }
    wxMenu* menuHelp = new wxMenu;
    {
        menuHelp->Append(wxID_ANY, "Homepage"); // Homepage
        menuHelp->Append(wxID_ANY, "About"); // About
    }

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile,  "&File"  );
    menuBar->Append( menuView,  "&View"  );
    menuBar->Append( menuHelp,  "&Help"  );
    SetMenuBar( menuBar );

    this->dockWin = 
        new DockWin(
            this, 
            this, 
            -1, 
            wxDefaultPosition, 
            wxDefaultSize, 
            wxCLIP_CHILDREN);

    this->dockWin->SetEventOnAdded([this](HWND hwnd, Node*n){this->OnDockWin_Added(hwnd, n);});
    this->dockWin->SetEventOnLost([this](HWND hwnd, LostReason lr){this->OnDockWin_Removed(hwnd, lr);});
    this->dockWin->SetEventTitleModified([this](Node* n){this->OnDockWin_TitleModified(n);});
    AppDock::GetApp().RegisterTopWin(this, this->GetHWND());

    AppUtils::SetDefaultIcons(this);
    this->UpdateTitlebar();
}

TopDockWin::~TopDockWin()
{
    --_InstCtr;
    AppDock::GetApp().UnregisterTopWin(this);
    AppDock::GetApp().UnregisterToplevelOwned(this->cachedHWND);
}

Node* TopDockWin::SetRoot(HWND hwnd)
{
    Node* retRoot = this->dockWin->AddRootHwnd(hwnd);
    if( retRoot == nullptr)
        return nullptr;

    return retRoot;
}

void TopDockWin::StealRoot(Node* pn, DockWin* stolenFrom)
{
    ASSERT_ISNODEWIN(pn);
    this->dockWin->StealRoot(pn, stolenFrom);
}

bool TopDockWin::HasRoot() const
{
    return this->dockWin->HasRoot();
}

void TopDockWin::ReleaseAll()
{
    this->dockWin->ReleaseAll();
}

void TopDockWin::DetachAll()
{
    // There's probably a cheaper way to do this by trashing the
    // layout - which isn't a problem because we'd close the (Top)DockWin
    // right after detaching everything. 
    //
    // But for now we'll do the more expensive thing (out of simplicity)
    // and cleanly remove every individual captured HWND one by one.
    std::set<HWND> dockedHwnds = this->GetDockWin()->AllDockedWindows();
	
	// Detaching only makes sense if we have more than one window, or else
	// we would just create a TopDockWin that had the exact same contents.
    if (dockedHwnds.size() <= 1)
        return;

	// We'll let the first window (the root) stay in this TopDockWin.
    for (auto it = ++dockedHwnds.begin(); it != dockedHwnds.end(); ++it)
        this->dockWin->DetachNodeWin(*it);

    this->GetDockWin()->Layout();
}

void TopDockWin::UpdateTitlebar()
{
    std::string titlebarStr = "PrecisionDock";

    const Node* rootNode = this->dockWin->GetRoot();
    if (rootNode != nullptr && rootNode->type == Node::Type::Window)
        titlebarStr += std::string(" | ") + rootNode->GetPreferredTabTitlebar();

    this->SetTitle(titlebarStr.c_str());
}

TopDockWin * TopDockWin::GetWinAt(const wxPoint& screenMouse, const std::set<TopDockWin*>& ignores)
{
    HWND deskHwnd = GetDesktopWindow();
    bool foundTarg = false;
    TopDockWin* ret = nullptr;

    for(
        HWND it = GetWindow(deskHwnd, GW_CHILD); 
        it != NULL; 
        it = GetNextWindow(it, GW_HWNDNEXT))
    {
        if(IsWindowVisible(it) == FALSE)
            continue;

        RECT r;
        if(GetWindowRect(it, &r) == FALSE)
            continue;

        wxRect rect = wxRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
        if(rect.Contains(screenMouse) == false)
            continue;

        ret = AppDock::GetApp().GetDockWin(it);
        if(ret == nullptr)
            continue;
        
        if(ignores.find(ret) != ignores.end())
            continue;

        return ret;
    }
    return nullptr;
}

void TopDockWin::OnDockWin_TitleModified(Node* n)
{
    ASSERT_ISNODEWIN(n);
    this->UpdateTitlebar();
}

TopDockWin * TopDockWin::GetWinAt(const wxPoint& screenMouse, TopDockWin* ignore)
{
    std::set<TopDockWin*> ignores;
    ignores.insert(ignore);
    return GetWinAt(screenMouse, ignores);
}

TopDockWin * TopDockWin::GetWinAt(const wxPoint& screenMouse)
{
    std::set<TopDockWin*> ignores;
    return GetWinAt(screenMouse, ignores);
}


void TopDockWin::OnDetectLostWindow(HWND win, LostWindowReason why)
{
	if (why == LostWindowReason::Destroyed)
	    this->dockWin->CloseNodeWin(win);
}

void TopDockWin::UpdateWindowTitlebar(HWND win)
{
    this->dockWin->RefreshWindowTitlebar(win);
}

void TopDockWin::OnDockWin_Added(HWND hwnd, Node* n)
{
	AppDock::GetApp()._RegisterCapturedHWND(this, hwnd);
    this->UpdateTitlebar();
}

void TopDockWin::OnDockWin_Removed(HWND hwnd, LostReason lr)
{
	AppDock::GetApp()._UnregisterCapturedHWND(hwnd);
    this->UpdateTitlebar();
}

void TopDockWin::OnExit(wxCommandEvent& event)
{
    Close( true );
}


void TopDockWin::OnMenu_ToggleStatusbar(wxCommandEvent& evt)
{
    if(this->statusBar == nullptr)
    {
        this->statusBar = this->CreateStatusBar();
    }
    else
    {
        delete this->statusBar;
        this->statusBar = nullptr;
    }
}

void TopDockWin::OnMenu_DetachAll(wxCommandEvent& evt)
{
    this->DetachAll();
}

void TopDockWin::OnMenu_ReleaseAll(wxCommandEvent& evt)
{
    this->dockWin->ReleaseAll();
    // NOTE: Eventually we may want to release them in an ordered way.
    // But for now when we release them, we'll just leave them wherever
    // they happen to end up.
    this->Close(true);
}

json TopDockWin::_JSONRepresentation()
{
    return this->dockWin->_JSONRepresentation();
}

bool TopDockWin::_TestValidity()
{
    // TODO:
    assert(this->dockWin->_TestValidity() == true);
    return true;
}