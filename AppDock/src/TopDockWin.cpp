#include "TopDockWin.h"
#include "DockWin.h"
#include "AppDock.h"
#include "Utils/AppUtils.h"

int TopDockWin::_InstCtr = 0;

wxBEGIN_EVENT_TABLE(TopDockWin, wxFrame)
    EVT_MENU(wxID_EXIT,  TopDockWin::OnExit)
    EVT_MENU((int)CMDID::ToggleStatusbar,       TopDockWin::OnMenu_ToggleStatusbar  )
    EVT_MENU((int)CMDID::ReleaseAll,            TopDockWin::OnMenu_ReleaseAll       )
    EVT_MENU((int)CMDID::DetachAll,             TopDockWin::OnMenu_DetachAll        )
    EVT_MENU((int)CMDID::CloseAll,              TopDockWin::OnMenu_CloseAll         )
wxEND_EVENT_TABLE()

TopDockWin::TopDockWin(const wxPoint& pos, const wxSize& size)
    :   wxFrame(NULL, wxID_ANY, "PrecisionDock", pos, size)
{
    this->cachedHWND = this->GetHWND();
    AppDock::GetApp().RegisterToplevelOwned(this->cachedHWND);

    ++_InstCtr;

    //this->Connect((int)CMDID::ReleaseAll, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(TopDockWin::OnMenu_ReleaseAll), nullptr, this);

    HMENU sysMenu = (HMENU )GetSystemMenu((HWND)this->m_hWnd,  FALSE);
    int menuPosIns = 0;
    if (sysMenu != NULL)
    {
        // System Menu: MANAGE ALL
        //////////////////////////////////////////////////
        HMENU subMenuManip = CreatePopupMenu();
        if (subMenuManip != NULL)
        {
            InsertMenu(sysMenu, menuPosIns, MF_BYPOSITION | MF_POPUP, (UINT_PTR)subMenuManip, TEXT("Manage All"));
            ++menuPosIns;
            //
            InsertMenu(subMenuManip, -1, MF_BYPOSITION | MF_POPUP, (int)CMDID::ReleaseAll,  TEXT("Release All"));
            InsertMenu(subMenuManip, -1, MF_BYPOSITION | MF_POPUP, (int)CMDID::DetachAll,   TEXT("Detach All All"));
            InsertMenu(subMenuManip, -1, MF_BYPOSITION | MF_POPUP, (int)CMDID::CloseAll,    TEXT("Close All"));
            InsertMenu(subMenuManip, -1, MF_BYPOSITION | MF_POPUP, wxID_EXIT,               TEXT("Force Close All"));
        }

        // System Menu: VIEW
        //////////////////////////////////////////////////
        HMENU subMenuView = CreatePopupMenu();
        if (subMenuView != NULL)
        {
            InsertMenu(sysMenu, menuPosIns, MF_BYPOSITION | MF_POPUP, (UINT_PTR)subMenuView, TEXT("View"));
            ++menuPosIns;
            //
            InsertMenu(subMenuView, -1, MF_BYPOSITION | MF_POPUP, (int)CMDID::ToggleStatusbar,  TEXT("Toggle Statusbar"));
        }

        // System Menu: HELP
        //////////////////////////////////////////////////
        HMENU subMenuHelp = CreatePopupMenu();
        if (subMenuHelp != NULL)
        {
            InsertMenu(sysMenu, menuPosIns, MF_BYPOSITION | MF_POPUP, (UINT_PTR)subMenuHelp, TEXT("Help"));
            ++menuPosIns;
            //
            InsertMenu(subMenuHelp, -1, MF_BYPOSITION | MF_POPUP, wxID_ANY,  TEXT("Homepage")); // TODO:
            InsertMenu(subMenuHelp, -1, MF_BYPOSITION | MF_POPUP, wxID_ANY,  TEXT("About")); // TODO:
        }

        // Menu horizontal separator before the "Close" at the bottom of the system menu.
        InsertMenu(sysMenu, 3, MF_BYPOSITION | MF_SEPARATOR, (UINT_PTR)subMenuHelp, TEXT(""));
    }

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
    this->UpdateTitlebar(false);
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

void TopDockWin::CloseAllHWNDs()
{
    std::set<HWND> dockedHwnds = this->GetDockWin()->AllDockedWindows();
    for (HWND hwnd : dockedHwnds)
        SendMessage(hwnd, WM_CLOSE, 0, 0);
}

void TopDockWin::UpdateTitlebar(bool maybeDeleted)
{
    std::string titlebarStr = "PrecisionDock";

    const Node* rootNode = this->dockWin->GetRoot();
    if (rootNode != nullptr && rootNode->type == Node::Type::Window)
    {
        if (maybeDeleted && rootNode->Hwnd() == NULL)
            return;

        titlebarStr = std::string("PrDok | ") + rootNode->GetPreferredTabTitlebar();
    }

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
    this->UpdateTitlebar(false);
}

bool TopDockWin::MSWTranslateMessage(WXMSG* msg)
{
    if (msg->message == WM_SYSCOMMAND)
    {
        switch(msg->wParam)
        {
            // For every wparam documented for WM_SYSCOMMAND, we leave alone and 
            // let the default implementation do whatever with it.
            // https://learn.microsoft.com/en-us/windows/win32/menurc/wm-syscommand
        case SC_CLOSE:
        case SC_CONTEXTHELP:
        case SC_DEFAULT:
        case SC_HOTKEY:
        case SC_HSCROLL:
        // case SCF_ISSECURE: 
        //  Except this, t'is a troublemaker!
        case SC_KEYMENU:
        case SC_MAXIMIZE:
        case SC_MINIMIZE:
        case SC_MONITORPOWER:
        case SC_MOUSEMENU:
        case SC_MOVE:
        case SC_NEXTWINDOW:
        case SC_PREVWINDOW:
        case SC_RESTORE:
        case SC_SCREENSAVE:
        case SC_SIZE:
        case SC_TASKLIST:
        case SC_VSCROLL:
            break;

        default:
            // Everything else, we're claiming as ID space we're processing ourselves.
            // We convert to a menu command
            wxCommandEvent evt(wxEVT_MENU, msg->wParam);
            this->ProcessEvent(evt);
            return true;
        }
        
    }
    
    return wxFrame::MSWTranslateMessage(msg);
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

void TopDockWin::OnWindowTitlebarModified(HWND win)
{
    // See comments at titlebarsToUpdate for more information.
    if(this->dockWin->RefreshWindowTitlebar(win))
        this->UpdateTitlebar(false);
}

void TopDockWin::OnDockWin_Added(HWND hwnd, Node* n)
{
	AppDock::GetApp()._RegisterCapturedHWND(this, hwnd);
    this->UpdateTitlebar(false);
}

void TopDockWin::OnDockWin_Removed(HWND hwnd, LostReason lr)
{
	AppDock::GetApp()._UnregisterCapturedHWND(hwnd);
    this->UpdateTitlebar(true);
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

void TopDockWin::OnMenu_CloseAll(wxCommandEvent& evt)
{
    this->CloseAllHWNDs();
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