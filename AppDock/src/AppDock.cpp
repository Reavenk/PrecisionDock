// AppDock.cpp : Defines the entry point for the application.
#include "TopDockWin.h"

#include <wx/msw/private.h>
#include <wx/config.h>

#include "Taskbar.h"

#include "AppDock.h"
#include "Icon.xpm"
#include <fstream>
#include <sstream>

#include "Layout/Node.h"
#include "CaptureDlg/CaptureDlg.h"
#include "Dlgs/DlgIntro.h"
#include "Dlgs/DlgAbout.h"
#include "Utils/mywxUtils.h"

//HHOOK hookListener = NULL;

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
        if(ShowNotificationDlgModal() == true)
            AppDock::ConfirmNotice();
        else
            return false;
    }

    this->SpawnEmpty("AppDock", true);
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

    // TODO: Assign proper icon
    if (!taskbar->SetIcon(
        wxIcon(statusIcon),
        "AppDock\n"
        "Double click for capture window.\n"
        "Right click for more options."))
    {
        //wxLogError("Could not set icon.");
    }

    this->ReloadAppRefs();
    this->maintenenceTimer.Start(2000);
    

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
    const wxString& title,
    bool giveAttention)
{
    return this->CreateSpawned(
        cmd,
        title,
        AppRef::_default.msBeforeAttach,
        AppRef::_default.closeIfFail,
        AppRef::_default.startShown, 
        giveAttention);
}

TopDockWin* AppDock::CreateSpawned(
    const std::wstring& cmd, 
    const wxString& title,
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
            title, 
            wxDefaultPosition, 
            wxDefaultSize);

    Node* rootNode = createdTopDockWin->SetRoot(hwndCreateWin);
    assert(rootNode != nullptr);
    rootNode->cmdLine = cmd;
    createdTopDockWin->Show();

    if(giveAttention)
        mywxUtils::RaiseWindowToAttention(createdTopDockWin);
    
    return createdTopDockWin;
}

TopDockWin* AppDock::SpawnEmpty(const wxString& title, bool giveAttention)
{
    TopDockWin* createdTopDockWin = 
        new TopDockWin(
            title, 
            wxDefaultPosition,
            wxSize(400, 300));

    createdTopDockWin->Show();

    if(giveAttention)
        mywxUtils::RaiseWindowToAttention(createdTopDockWin);

    return createdTopDockWin;
}

TopDockWin* AppDock::CreateTorn(
    Node* pn,
    const wxString& title,
    bool giveAttention)
{
    
    TopDockWin* createdTopDockWin = 
        new TopDockWin(
            title, 
            wxGetMousePosition(),
            pn->cacheSize);

    createdTopDockWin->StealRoot(pn);
    createdTopDockWin->Show();

    if(giveAttention)
        mywxUtils::RaiseWindowToAttention(createdTopDockWin);

    return createdTopDockWin;
}

TopDockWin* AppDock::CreateWindowFromHwnd(HWND hwnd, bool giveAttention)
{
    TopDockWin* createdTopDockWin = 
        new TopDockWin(
            "Untitled",
            wxDefaultPosition,
            wxDefaultSize);

    createdTopDockWin->Show();

    // TODO: Check registration
    createdTopDockWin->SetRoot(hwnd);

    if(giveAttention)
        mywxUtils::RaiseWindowToAttention(createdTopDockWin);

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
        // TODO: How to error handle?
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

void AppDock::CloseAll()
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

    this->CloseAll();
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
            aref.label,
            aref.msBeforeAttach,
            aref.closeIfFail,
            aref.startShown);
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

bool AppDock::ShowNotificationDlgModal()
{
    DlgIntro dlgIntro;
    int ret = dlgIntro.ShowModal();

    return (ret == wxID_OK);
}

void AppDock::ShowAboutDlgModal()
{
    DlgAbout dlgAbout;
    dlgAbout.ShowModal();
}

bool AppDock::IsNoticeConfirmed()
{
    wxConfig config("AppDock");
    bool ret;
    config.Read("confirmed", &ret, false);
    return ret;
}

void AppDock::ConfirmNotice()
{
    wxConfig config("AppDock");
    config.Write("confirmed", true);
}

void AppDock::ClearNoticeConfirm()
{
    wxConfig config("AppDock");
    config.Write("confirmed", false);
}

void AppDock::RaiseTODO(const wxString& msg)
{
    AppDock::GetApp().taskbar->ShowBalloon("TODO", msg);
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