﻿// AppDock.cpp : Defines the entry point for the application.
#include "TopDockWin.h"
#include "Utils/OSUtils.h"

#include <wx/msw/private.h>
#include <wx/config.h>

#include "Taskbar.h"

#include "AppDock.h"
#include "Icon.xpm"
#include <fstream>
#include <sstream>

#include "Layout/Node.h"
#include "CaptureDlg/CaptureDlg.h"
#include "Dlgs/DlgInfo.h"
#include "Utils/mywxUtils.h"

//HHOOK hookListener = NULL;

void UnsubscribeWinSysHook(HWINEVENTHOOK& hook)
{
    if(hook == NULL)
        return;

    UnhookWinEvent(hook);
    hook = NULL;
}

HWINEVENTHOOK SubscribeWinSysHook(DWORD event, WINEVENTPROC callback, std::vector<HWINEVENTHOOK>* pvec)
{

    HWINEVENTHOOK hook = 
        SetWinEventHook(
            event, 
            event, 
            NULL, 
            callback, 
            NULL, 
            NULL, 
            WINEVENT_SKIPOWNPROCESS|WINEVENT_SKIPOWNTHREAD);

    if(pvec)
        pvec->push_back(hook);

    return hook;
}

wxBEGIN_EVENT_TABLE(AppDock, wxApp)
    EVT_TIMER(-1, AppDock::MaintenanceLoop)
wxEND_EVENT_TABLE()

//LRESULT CALLBACK CBTProc(
//    _In_ int    nCode,
//    _In_ WPARAM wParam,
//    _In_ LPARAM lParam
//)
//{
//    int ret = CallNextHookEx(hookListener, nCode, wParam, lParam);
//
//    std::string strCode     = std::to_string(nCode);
//    std::string strwParam   = std::to_string(wParam);
//    std::string strlParam   = std::to_string(lParam);
//
//    switch (nCode)
//    {
//    case HCBT_ACTIVATE:
//        strCode = "HCBT_ACTIVATE";
//        break;
//    case HCBT_CLICKSKIPPED:
//        strCode = "HCBT_CLICKSKIPPED";
//        break;
//    case HCBT_CREATEWND:
//        strCode = "HCBT_CREATEWND";
//        break;
//    case HCBT_DESTROYWND:
//        strCode = "HCBT_DESTROYWND";
//        break;
//    case HCBT_KEYSKIPPED:
//        strCode = "HCBT_KEYSKIPPED";
//        break;
//    case HCBT_MINMAX:
//        strCode = "HCBT_MINMAX";
//        break;
//    case HCBT_MOVESIZE:
//        strCode = "HCBT_MOVESIZE";
//        break;
//    case HCBT_QS:
//        strCode = "HCBT_QS";
//        break;
//    case HCBT_SETFOCUS:
//        strCode = "HCBT_SETFOCUS";
//        break;
//    case HCBT_SYSCOMMAND:
//        strCode = "HCBT_SYSCOMMAND";
//        break;
//    }
//    
//    std::cout << "HCBT: " << strCode << " with lParam " << strlParam << " and wParam " << strwParam << std::endl;
//    return ret;
//}

HINSTANCE hGetProcIDDLL = NULL;

VOID WindowSysEventCallback(
    HWINEVENTHOOK hWinEventHook,
    DWORD         event,
    HWND          hwnd,
    LONG          idObject,
    LONG          idChild,
    DWORD         idEventThread,
    DWORD         dwmsEventTime)
{
 
    if (event == EVENT_OBJECT_DESTROY)
    {
        AppDock::GetApp().OnHook_WindowClosed(hwnd);
    }
    else if (event == EVENT_OBJECT_NAMECHANGE)
    {
		// The text of the titlebar changed
        if (idObject == OBJID_WINDOW)
            AppDock::GetApp().OnHook_WindowNameChanged(hwnd);
    }
    else if (event == EVENT_OBJECT_CREATE)
    {
        if (idObject == OBJID_WINDOW)
            AppDock::GetApp().OnHook_WindowCreated(hwnd);
    }
    else if (event == EVENT_OBJECT_LOCATIONCHANGE)
    {
        if (idObject == OBJID_WINDOW)
        {
			// If a window moved, check if it's the window we're
			// tracking as the one being dragged by its titlebar.
			AppDock::GetApp().OnHook_MoveSize(hwnd);
        }
    }
    else if (EVENT_SYSTEM_MOVESIZESTART)
    {
        wxLogDebug("EVENT_SYSTEM_MOVESIZESTART");
        wxPoint mousePt = wxGetMousePosition();
        LRESULT checkClick = SendMessage(hwnd, WM_NCHITTEST, 0, MAKELPARAM(mousePt.x, mousePt.y));

        // Used to handle unexpected behaviour where EVENT_SYSTEM_MOVESIZEEND will not be
        // sent for a window when dragging stops, but instead another EVENT_SYSTEM_MOVESIZESTART
        // is sent.
        HWND capHWND = ::GetCapture();
        if (DragHelperMgr::IsInstType(DragHelperMgr::DragType::NativeWin))
        {
            if (capHWND != DragHelperMgr::GetInst()->nativeDragged)
            {
                AppDock::GetApp().OnHook_EndMoveSize(hwnd);
                return;
            }
        }

        // We want to track when the titlebar starts to get dragged, but that's
        // mixed in with the size events too. So we only track events that involve
        // the mouse being over the titlebar and assume the MOVESIZESTART was triggered
        // by a titlebar drag.
        if (checkClick == HTCAPTION)
            AppDock::GetApp().OnHook_StartedMoveSize(hwnd);
    }
    else if (EVENT_SYSTEM_MOVESIZEEND)
    {
        wxLogDebug("EVENT_SYSTEM_MOVESIZEEND");
        AppDock::GetApp().OnHook_EndMoveSize(hwnd);
    }

}

wxIMPLEMENT_APP(AppDock);
bool AppDock::OnInit()
{
    wxInitAllImageHandlers();

    this->singleInstChecker = new wxSingleInstanceChecker;
    if ( this->singleInstChecker->IsAnotherRunning() )
    {
        wxLogError(_("Another program instance is already running, aborting."));
        delete this->singleInstChecker; 
        this->singleInstChecker = nullptr;
        return false;
    }

    if (AppDock::IsNoticeConfirmed() == false)
    {
        // Note that if the user doesn't agree to the dialog's terms, the app
        // will be forced closed.
        ShowNotificationDlgModal();
    }

    this->SpawnEmpty(true);
    HINSTANCE hInst = wxGetInstance();
    ////hookListener = SetWindowsHookExA(WH_CBT, CBTProc, GetModuleHandle(NULL), GetCurrentThreadId() );
    //DWORD err = GetLastError();
    //HINSTANCE hGetProcIDDLL = LoadLibrary(L"AppDockEventSpy.dll");
    //err = GetLastError();
    //
    //
    //if (!hGetProcIDDLL) {
    //    std::cout << "could not load the dynamic library" << std::endl;
    //    return EXIT_FAILURE;
    //}
    //typedef void (__stdcall *f_funci)();
    //// resolve function address here
    //f_funci funci = (f_funci)GetProcAddress(hGetProcIDDLL, "InstallHooks");
    ////err = GetLastError();
    //if (!funci) {
    //    std::cout << "could not locate the function" << std::endl;
    //    return EXIT_FAILURE;
    //}
    //funci();

    this->taskbar = new Taskbar();

    if (!taskbar->SetIcon(
        wxIcon(statusIcon),
        "PrecisionDock\n"
        "Double click for capture window.\n"
        "Right click for more options."))
    {
        //wxLogError("Could not set icon.");
    }

    this->ReloadAppRefs();
    this->maintenenceTimer.Start(2000);
    
    const std::vector<AppRef>& v = AppDock::GetApp().ReferencedApps();
    // For debugging, spawn some dummy programs right-off-the-bat.
    // Note that this will crash if there aren't any spawnnable things.
    // 
    // NOTE: This is slated to be removed when feature-complete is hit.
    AppDock::GetApp().LaunchAppRef(v[0]);
    AppDock::GetApp().LaunchAppRef(v[0]);
    AppDock::GetApp().LaunchAppRef(v[0]);

    // https://learn.microsoft.com/en-us/windows/win32/winauto/event-constants
    SubscribeWinSysHook(EVENT_OBJECT_DESTROY,           WindowSysEventCallback, &this->winHooks);
    SubscribeWinSysHook(EVENT_OBJECT_NAMECHANGE,        WindowSysEventCallback, &this->winHooks);
    SubscribeWinSysHook(EVENT_OBJECT_CREATE,            WindowSysEventCallback, &this->winHooks);
    SubscribeWinSysHook(EVENT_OBJECT_LOCATIONCHANGE,    WindowSysEventCallback, &this->winHooks);
    SubscribeWinSysHook(EVENT_SYSTEM_MOVESIZESTART,     WindowSysEventCallback, &this->winHooks);
    SubscribeWinSysHook(EVENT_SYSTEM_MOVESIZEEND,       WindowSysEventCallback, &this->winHooks);

    return true;
}

int AppDock::OnExit()
{
    //typedef void (__stdcall *f_funci)();
    //// resolve function address here
    //f_funci funci = (f_funci)GetProcAddress(hGetProcIDDLL, "RemoveHooks");
    //funci();

    CaptureDlg::Shutdown();

    assert(this->singleInstChecker != nullptr);
    delete this->singleInstChecker;
    this->singleInstChecker = nullptr;

    if(this->taskbar != nullptr)
        delete this->taskbar;

    for(HWINEVENTHOOK wevth : this->winHooks)
        UnsubscribeWinSysHook(wevth);
    this->winHooks.clear();
	
    return 0;
}

AppDock & AppDock::GetApp()
{
    return wxGetApp();
}

AppDock::AppDock()
    : maintenenceTimer(this)
{}

TopDockWin* AppDock::CreateSpawned(
    const std::wstring& cmd, 
    bool giveAttention)
{
    return this->CreateSpawned(
        cmd,
        AppRef::_default.msBeforeAttach,
        AppRef::_default.closeIfFail,
        AppRef::_default.startShown, 
        giveAttention);
}

TopDockWin* AppDock::CreateSpawned(
    const std::wstring& cmd, 
    int warmup,
    bool closeIfFail,
    bool startShown,
    bool giveAttention)
{
    HWND hwndCreateWin = 
        CreateSpawnedWindow(
            cmd,
            warmup,
            closeIfFail,
            startShown,
            true);
    

    if(hwndCreateWin == NULL)
        return nullptr;

    TopDockWin* createdTopDockWin = 
        new TopDockWin(
            wxDefaultPosition, 
            wxDefaultSize);

    Node* rootNode = createdTopDockWin->SetRoot(hwndCreateWin);
    assert(rootNode != nullptr);
    rootNode->cmdLine = cmd;
    createdTopDockWin->Show();

    if(giveAttention)
        MyWxUtils::RaiseWindowToAttention(createdTopDockWin);
    
    return createdTopDockWin;
}

TopDockWin* AppDock::SpawnEmpty(bool giveAttention)
{
    TopDockWin* createdTopDockWin = 
        new TopDockWin(
            wxDefaultPosition,
            wxSize(400, 300));

    createdTopDockWin->Show();

    if(giveAttention)
        MyWxUtils::RaiseWindowToAttention(createdTopDockWin);

    return createdTopDockWin;
}

TopDockWin* AppDock::CreateTorn(
    Node* pn,
	DockWin* originalOwner,
    bool giveAttention)
{
    TopDockWin* createdTopDockWin = 
        new TopDockWin(
            wxGetMousePosition(),
            pn->cacheSize);

    createdTopDockWin->StealRoot(pn, originalOwner);
    createdTopDockWin->Show();

    if(giveAttention)
        MyWxUtils::RaiseWindowToAttention(createdTopDockWin);

    return createdTopDockWin;
}

TopDockWin* AppDock::CreateWindowFromHwnd(HWND hwnd, bool giveAttention)
{
    TopDockWin* createdTopDockWin = 
        new TopDockWin(
            wxDefaultPosition,
            wxDefaultSize);

    createdTopDockWin->Show();

    Node* createRoot = createdTopDockWin->SetRoot(hwnd);
    assert(createRoot != nullptr);

    if(giveAttention)
        MyWxUtils::RaiseWindowToAttention(createdTopDockWin);

    return createdTopDockWin;
}

TopDockWin* AppDock::GetDockWin(HWND hwnd)
{
    for(auto it : this->dockWins)
    {
        if(it.first->GetHWND() == hwnd)
            return it.first;
    }
    return nullptr;
}

bool AppDock::RegisterTopWin(TopDockWin* win, HWND hwnd)
{
    auto itFind = this->dockWins.find(win);
    if(itFind != this->dockWins.end())
    {
        assert(!"Failed to register TopWin in RegisterTopWin().");
        return false;
    }

    WinProcessTrack tracking;
    tracking.win = win;
    tracking.rootHwnd = hwnd;
    //
    this->dockWins[win] = tracking;
    return true;
}

bool AppDock::UnregisterTopWin(TopDockWin* win)
{
    auto itFind = this->dockWins.find(win);
    if(itFind == this->dockWins.end())
        return false;

    this->dockWins.erase(itFind);
    return true;
}

void AppDock::ForceCloseAll()
{
    std::vector<TopDockWin*> tdw;
    for(auto it : this->dockWins)
        tdw.push_back(it.first);

    for(TopDockWin* del : tdw)
        delete del;
}

void AppDock::ReleaseAll()
{
    std::vector<TopDockWin*> tdw;
    for(auto it : this->dockWins)
        it.first->ReleaseAll();

    this->ForceCloseAll();
}

void AppDock::DetachAll()
{
    for(auto it : this->dockWins)
        it.first->DetachAll();

}

void AppDock::MaintenanceLoop(wxTimerEvent& evt)
{
    // Do maintenence on sparse intervals

    for(auto it = dockWins.begin(); it != dockWins.end(); )
    {
        // Check if the dockWins needs to be pruned. While windows will
        // unregister themselves upon closing, this can happen if windows 
        // crash.
        if(IsWindow(it->second.rootHwnd) == false)
            it = dockWins.erase(it);
        else
            ++it;
    }
}

void AppDock::ReloadAppRefs()
{
    this->launchRefs.clear();
    std::ifstream refFiles(GetAppListFilename());
    if(!refFiles.is_open())
        return;

    std::stringstream inputDataStream;
    inputDataStream << refFiles.rdbuf();
    std::string jsText = inputDataStream.str();
    json inputJSON = json::parse(jsText);

    if(
        inputJSON.contains("apps") == true && 
        inputJSON["apps"].is_array() == true)
    { 
        json entries = inputJSON["apps"];

        for(json& jse : entries)
        {
            AppRef ad(jse);
            if(ad.IsValid() == false)
                continue;

            this->launchRefs.push_back(ad);
        }
    }

    refFiles.close();
}

bool AppDock::LaunchAppRef(int idx)
{
    if(idx < 0 || idx >= this->launchRefs.size())
        return false;
    wxFrame wxf;
    
    return this->LaunchAppRef(this->launchRefs[idx]);
}

bool AppDock::LaunchAppRef(const AppRef& aref)
{
    return 
        this->CreateSpawned(
            aref.command,
            //aref.label,
            aref.msBeforeAttach,
            aref.closeIfFail,
            aref.startShown);
}

void AppDock::OnHook_WindowClosed(HWND hwnd)
{
    TopDockWin* twd = nullptr;
    {
        std::lock_guard<std::mutex> guardOwned(this->capturedWinsMutex);
        auto itFindCaptured = this->capturedWinsToTopDock.find(hwnd);
        if(itFindCaptured == this->capturedWinsToTopDock.end())
            return;

        twd = itFindCaptured->second;
    }

    assert(twd != nullptr);
    twd->OnDetectLostWindow(hwnd, LostWindowReason::Destroyed);
}

void AppDock::OnHook_WindowNameChanged(HWND hwnd)
{
    TopDockWin* twd = nullptr;
    {
        std::lock_guard<std::mutex> guardOwned(this->capturedWinsMutex);
        auto itFindCaptured = this->capturedWinsToTopDock.find(hwnd);
        if(itFindCaptured == this->capturedWinsToTopDock.end())
            return;

        twd = itFindCaptured->second;
    }

    assert(twd != nullptr);

    // Check validity because I've seen an edge case where the 
    // TopDockWin (twd) is closed and destructed before 
    // UpdateWindowTitlebar() is called.
    if(IsWindow(hwnd))
        twd->OnWindowTitlebarModified(hwnd);
}

void AppDock::OnHook_WindowCreated(HWND hwnd)
{
	// Sanity check it's a toplevel window.
    if (!OSUtils::IsToplevel(hwnd))
        return;

	// Lots of new windows are constantly being created.
	// So what we want to do is only handle things toplevels
	// with titlebars.
	LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if ((style & WS_CAPTION) == 0)
        return;
	
    AppDock& adInst = AppDock::GetApp();
    if (adInst.IsStealingNew())
    {
        this->CreateWindowFromHwnd(hwnd, false);

        // TODO: Set window size and position to
        // match the hwnd before it was captured
    }
}

void AppDock::OnHook_StartedMoveSize(HWND hwnd)
{
    if (DragHelperMgr::IsInstType(DragHelperMgr::DragType::NativeWin))
    {
        // If we've gotten a signal to drag the window we're already
        // dragging - this can happen spurriously by the system right
        // before an EndMoveSize happens.
        if (DragHelperMgr::GetInst()->nativeDragged == hwnd)
            return;
    }
	// Only called if started from a titlebar drag.
    DragHelperMgr::SetInst(new DragHelperMgr(hwnd));
}

void AppDock::OnHook_EndMoveSize(HWND hwnd)
{
	if(!DragHelperMgr::IsInstType(DragHelperMgr::DragType::NativeWin))
		return;

    DragHelperMgrPtr dragMgrInst = DragHelperMgr::GetInst();
    if (dragMgrInst->winDraggedOnto != nullptr)
    {
        Node::Dest whereAdd = dragMgrInst->tabDropDst.WhereToNodeDest();

        Node * nodeAdded = 
            dragMgrInst->winDraggedOnto->GetDockWin()->AddToLayout(
                dragMgrInst->nativeDragged,
                dragMgrInst->tabDropDst.topOf,
                whereAdd);

        if(nodeAdded)
            dragMgrInst->winDraggedOnto->GetDockWin()->ResizeLayout();
    }
		
    dragMgrInst->dragFlaggedAsFinished = true;
	DragHelperMgr::ReleaseInst();
}

void AppDock::OnHook_MoveSize(HWND hwnd)
{
    if(!DragHelperMgr::IsInstType(DragHelperMgr::DragType::NativeWin))
		return;

    DragHelperMgr::GetInst()->_HandleMouseMoveTopHWND();
}

std::vector<TopDockWin*> AppDock::_GetWinList()
{
    std::vector<TopDockWin*> ret;
    for(auto it : this->dockWins)
        ret.push_back(it.first);

    return ret;
}

std::set<HWND> AppDock::_GetToplevelDockHWNDs() const
{
    std::set<HWND> ret;
    for (auto it : this->dockWins)
        ret.insert(it.second.rootHwnd);

    return ret;
}

bool AppDock::AppOwnsTopLevelWindow(HWND hwnd) const
{
    std::lock_guard<std::mutex> guard(this->ownedWinMutex);
    return this->ownedWins.find(hwnd) != this->ownedWins.end();
}

bool AppDock::IsStealingNew() const
{
    return this->stealAllNewOSToplevels;
}

void AppDock::SetStealingNew(bool steal)
{
    this->stealAllNewOSToplevels = steal;
}

HWND AppDock::CreateSpawnedWindow(const std::wstring & cmd)
{
    return CreateSpawnedWindow(
        cmd, 
        AppRef::_default.msBeforeAttach,
        AppRef::_default.closeIfFail,
        AppRef::_default.startShown,
        true);
}

HWND AppDock::CreateSpawnedWindow(
    const std::wstring & cmd, 
    int warmup,
    bool closeIfFail,
    bool startShown,
    bool reportOnFail)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    std::wstring wcmd(cmd.begin(), cmd.end());

    if(startShown == false)
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow |= SW_HIDE;
    }

    bool create =
        CreateProcessW( 
            NULL,           // No module name (use command line)
            &wcmd[0],       // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi );          // Pointer to PROCESS_INFORMATION structure 

    Sleep(warmup);
    //
    ////https://stackoverflow.com/questions/49185312/how-to-start-an-exe-in-c-and-get-its-hwnd-window-handle-for-sending-messages
    HWND hwndFind = NULL;
    for(
        hwndFind = FindWindowEx(NULL, NULL, NULL, NULL);
        hwndFind != NULL;
        hwndFind = FindWindowEx(NULL, hwndFind, NULL, NULL))
    {
        DWORD procID = 0;
        GetWindowThreadProcessId(hwndFind, &procID);
        if(procID == pi.dwProcessId)
        { 
            HWND topWin = hwndFind;//::GetTopWindow(hwndFind);

            if(topWin == NULL)                
                continue;

            while(true)
            {
                HWND parent = GetParent(topWin);
                if(parent == NULL)
                    break;

                topWin = parent;
            }
            return topWin;
        }
    }

    if(reportOnFail == true)
    {
        AppDock::GetApp().taskbar->ShowBalloon(
            "No window found.",
            "Could not find window from spawned process.");
    }

    if(closeIfFail == true)
        TerminateProcess(pi.hProcess, 1);

    return NULL;
}

bool AppDock::RegisterToplevelOwned(HWND hwnd)
{
    std::lock_guard<std::mutex> guard(this->ownedWinMutex);

    if(this->ownedWins.find(hwnd) != this->ownedWins.end())
        return false;

    this->ownedWins.insert(hwnd);
    return true;
}

bool AppDock::UnregisterToplevelOwned(HWND hwnd)
{
    std::lock_guard<std::mutex> guard(this->ownedWinMutex);

    if(this->ownedWins.find(hwnd) == this->ownedWins.end())
        return false;

    this->ownedWins.erase(hwnd);
    return true;
}

bool AppDock::IsToplevelOwned(HWND hwnd)
{
    std::lock_guard<std::mutex> guard(this->ownedWinMutex);
    return this->ownedWins.find(hwnd) != this->ownedWins.end();
}

void AppDock::ShowNotificationDlgModal()
{

    if (!AppDock::IsNoticeConfirmed())
    {
        DlgInfo dlgInfo(
            DlgInfo::CreateFlag::HasCancelConfirm |
            DlgInfo::CreateFlag::HasGotchaConfirm |
            DlgInfo::CreateFlag::HasIntro);

        dlgInfo.ShowModal();
    }
    else
    {
        DlgInfo dlgInfo(DlgInfo::CreateFlag::Default, 1);
        dlgInfo.ShowModal();
    }
}

void AppDock::ShowAboutDlgModal()
{
    // > ☐ ABOUT_MAIN_54ef27967daf: Application has an About dialog
    // > ☐ ABOUT_MAIN_e45b1a70f019: About dialog is modal.

	// !TODO:
    DlgInfo dlgInfo;
    dlgInfo.ShowModal();
}

const char* CONFIGNAME = "PrecisionDock";

bool AppDock::IsNoticeConfirmed()
{
    wxConfig config(CONFIGNAME);
    bool ret;
    config.Read("confirmed", &ret, false);
    return ret;
}

void AppDock::ConfirmNotice()
{
    wxConfig config(CONFIGNAME);
    config.Write("confirmed", true);
}

void AppDock::ClearNoticeConfirm()
{
    wxConfig config(CONFIGNAME);
    config.Write("confirmed", false);
}

void AppDock::RaiseTODO(const wxString& msg)
{
    AppDock::GetApp().taskbar->ShowBalloon("TODO", msg);
}

bool AppDock::_RegisterCapturedHWND(TopDockWin* owner, HWND winCaptured)
{
    // This function does more than just register, it also needs to 
    // check if things are being re-registered and send the proper
    // event notifications.

    std::lock_guard<std::mutex> capGuard(this->capturedWinsMutex);
    
    auto itFind = this->capturedWinsToTopDock.find(winCaptured);
    if(itFind != this->capturedWinsToTopDock.end())
    {
        if(itFind->second == owner)
        {
            // This shouldn't ever happen, but checked for sanity.
            assert(false);
            return false;
        }
        itFind->second->OnDetectLostWindow(winCaptured, LostWindowReason::Recaptured);
    }
    this->capturedWinsToTopDock[winCaptured] = owner;
    return true;
}

bool AppDock::_UnregisterCapturedHWND(HWND winCaptured)
{
    std::lock_guard<std::mutex> capGuard(this->capturedWinsMutex);
    auto itFind = this->capturedWinsToTopDock.find(winCaptured);
    if(itFind == this->capturedWinsToTopDock.end())
        return false;

    this->capturedWinsToTopDock.erase(itFind);
    return true;
}

bool AppDock::_UnregisterCapturedHWND(std::initializer_list<HWND> winsCaptured)
{
    bool any = false;
    std::lock_guard<std::mutex> capGuard(this->capturedWinsMutex);
    for(HWND hwnd : winsCaptured)
    {
        auto itFind = this->capturedWinsToTopDock.find(hwnd);
        if(itFind == this->capturedWinsToTopDock.end())
            return false;

        this->capturedWinsToTopDock.erase(itFind);
        any = true;
    }
    return any;
}

bool AppDock::RegisterCaptureDlg(CaptureDlg* dlg)
{
	std::lock_guard<std::mutex> capsGuard(this->captureDlgsMutex);
	this->captureDialogs.insert(dlg);
    return true; // TODO: Re-eval use of return value
}

bool AppDock::UnregisterCaptureDlg(CaptureDlg* dlg)
{
    std::lock_guard<std::mutex> capsGuard(this->captureDlgsMutex);
	this->captureDialogs.erase(dlg);
    return true; // TODO: Re-eval use of return value
}

bool AppDock::_TestValidity()
{

    // The constructor and destructor counter of the class should
    // match how many TopDockWins we have on record.
    assert(TopDockWin::ClassInstCtr() == dockWins.size());

    // Check window validity. If they were closed they should have
    // exited. If they crashed, hopefully the polling checks in
    // AppDock::MaintenanceLoop() will have cleaned it up.
    for(const auto& it : dockWins)
        assert(IsWindow(it.second.rootHwnd));

    // Make sure the cached version in the value matches the key as expected.
    for(const auto& it : dockWins)
        assert(it.first == it.second.win);

    // Make sure the app's cached HWND matches the true value.
    for(const auto& it : dockWins)
        assert(it.first->GetHWND() == it.second.rootHwnd);

    // Delegate validating of individual AppDocks.
    for(const auto& it : dockWins)
        it.first->_TestValidity();

    return true;
}