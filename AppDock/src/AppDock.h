#pragma once

#include "resource.h"
#include <map>
#include <set>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <thread>
#include <mutex>

#include <wx/snglinst.h>
#include <wx/timer.h>
#include "Taskbar.h"
#include "AppRef.h"

class TopDockWin;
class DockWin;
class Node;
class CaptureDlg;

/// <summary>
/// The application instance.
/// </summary>
class AppDock: public wxApp
{
    /// <summary>
    /// Data related to a TockDockWin instance current running.
    /// </summary>
    struct WinProcessTrack
    {
        /// <summary>
        /// A cached copy of the TopDockWin.
        /// </summary>
        TopDockWin * win;

        /// <summary>
        /// The root HWND of the file. While this is the same as 
        /// win->GetHWND(), we can't access that value once win
        /// is deleted. The Reason we would want to keep the hwnd 
        /// around afterwards is to see if win is still valid, 
        /// without directly touching it.
        /// </summary>
        HWND rootHwnd;
    };

public:
    /// <summary>
    /// Get the singleton.
    /// </summary>
    /// <returns>The singleton reference.</returns>
    static AppDock& GetApp();

private:
    /// <summary>
    /// Makes sure only 1 instance of the application can ever run
    /// at a time on the machine.
    /// </summary>
    wxSingleInstanceChecker* singleInstChecker = nullptr;

    /// <summary>
    /// A collection of all TopDockWin on the machine. (there should
    /// only be one instance of AppDock running on the machine).
    /// </summary>
    std::map<TopDockWin*, WinProcessTrack> dockWins;
    // TODO: We probably need to wrap dockWins in a mutex since this 
    // can change if something external closes the last window of a 
    // DockWin which forces the TopDockWin to close outside the app's
    // program thread.

    /// <summary>
    /// Timer to run background maintenence logic at 
    /// occasional intervals.
    /// </summary>
    wxTimer maintenenceTimer;

    /// <summary>
    /// The list of loaded applications that can be loaded
    /// from the taskbar.
    /// </summary>
    std::vector<AppRef> launchRefs;

    Taskbar * taskbar = nullptr;

    std::vector<HWINEVENTHOOK> winHooks;

public:
    virtual bool OnInit();
    virtual int OnExit();

protected:
    /// <summary>
    /// Top-level HWNDs (TopDockWin) owned by the application. We need to make sure
    /// these aren't captured.
    /// </summary>
    std::set<HWND> ownedWins;

    /// <summary>
    /// Thread protection for ownedWins.
    /// </summary>
    mutable std::mutex ownedWinMutex;

    /// <summary>
    /// The Captured HWND objects
    /// </summary>
    std::map<HWND, TopDockWin*> capturedWinsToTopDock;

    /// <summary>
    /// The mutex for capturedWinsToTopDock;
    /// </summary>
    std::mutex capturedWinsMutex;

    std::set<CaptureDlg*> captureDialogs;
	
    std::mutex captureDlgsMutex;

protected:
    void MaintenanceLoop(wxTimerEvent& evt);

public:
    AppDock();

    /// <summary>
    /// Spawn a TopDockWin created from a TopLevel window of an
    /// arbitrarily started command line.
    /// 
    /// Simplified the call compares to overload by using default
    /// parameters.
    /// </summary>
    /// <param name="cmd">The command line to capture the window of.</param>
    /// <param name="giveAttention">Make the window visable and obvious to the user.</param>
    /// <returns>The created captured window.</returns>
    TopDockWin* CreateSpawned(
        const std::wstring& cmd, 
        bool giveAttention = true);

    /// <summary>
    /// Spawn a TopDockWin created from a TopLevel window of an
    /// arbitrarily started command line.
    /// </summary>
    /// <param name="cmd">The command line to capture the window of.</param>
    /// <param name="warmup">
    /// The amount of time to wait for a program to create its toplevel 
    /// window before attempting to capture it.
    /// </param>
    /// <param name="closeIfFail">
    /// If true, close the program if the 
    /// </param>
    /// <param name="startShown">
    /// If false, try to start the program as hidden until it's captured.
    /// </param>
    /// <returns>The created captured window.</returns>
    TopDockWin* CreateSpawned(
        const std::wstring& cmd, 
        int warmup,
        bool closeIfFail,
        bool startShown,
        bool giveAttention = true);

    /// <summary>
    /// Spawn an empty TopDockWin.
    /// </summary>
    /// <param name="giveAttention">Make the window visable and obvious to the user.</param>
    /// <returns>The created empty TopDockWin.</returns>
    TopDockWin* SpawnEmpty(
        bool giveAttention = true);

    /// <summary>
    /// Repurpose a Node owned by a TopDockWin, into a new
    /// TopDockWin.
    /// </summary>
    /// <param name="pn">The node to use at the new root.</param>
    /// <param name="originalOwner">The DockWin that originally owned the Node.</param>
    /// <param name="giveAttention">Make the window visable and obvious to the user.</param>
    /// <returns>The newly created TopDockWin.</returns>
    TopDockWin* CreateTorn(
        Node* pn,
        DockWin* originalOwner,
        bool giveAttention = true);

    /// <summary>
    /// Capture a toplevel HWND.
    /// </summary>
    /// <param name="hwnd">The HWND to capture.</param>
    /// <param name="giveAttention">Make the window visable and obvious to the user.</param>
    /// <returns>
    /// The captured HWND window, or nullptr if the capture
    /// fails.
    /// </returns>
    TopDockWin* CreateWindowFromHwnd(
        HWND hwnd,
        bool giveAttention = true);

    /// <summary>
    /// Get an existing TopDockWin based off its HWND.
    /// </summary>
    /// <param name="hwnd">
    /// The HWND of the TopDockWin to capture.
    /// </param>
    /// <returns>
    /// The TopDockWin the HWND belongs to, or nullptr if
    /// there was no match.
    /// </returns>
    TopDockWin* GetDockWin(HWND hwnd);

    const std::vector<AppRef>& ReferencedApps() const 
    { return this->launchRefs; }

    bool RegisterTopWin(TopDockWin* win, HWND hwnd);
    bool UnregisterTopWin(TopDockWin* win);

    /// <summary>
    /// Close all TopDockWins.
    /// </summary>
    void CloseAll();

    /// <summary>
    /// Release all windows contained in all TopDockWins,
    /// restoring them back as windows.
    /// </summary>
    void ReleaseAll();

    /// <summary>
    /// Detach all windows contained in all TopDockWins,
    /// So each captured HWND will be the root of their own TopDockWin.
    /// </summary>
    void DetachAll();

    /// <summary>
    /// Gets the path where reference launch applications are
    /// specified. These are a list of authored program names
    /// that can be launched from the taskbar.
    /// </summary>
    /// <returns></returns>
    static inline std::string GetAppListFilename()
    {return "Applist.dock"; }

    // Functions to handle loaded presets.
    void ReloadAppRefs();
    bool LaunchAppRef(int idx);
    bool LaunchAppRef(const AppRef& aref);

    void OnHook_WindowClosed(HWND hwnd);
    void OnHook_WindowNameChanged(HWND hwnd);
    void OnHook_WindowCreated(HWND hwnd);

    std::vector<TopDockWin*> _GetWinList();
    std::set<HWND> _GetToplevelDockHWNDs() const;

    bool AppOwnsTopLevelWindow(HWND hwnd) const;

public:
    // Utility functions - may be moved to deciated utility library(s) later.

    // Register owned functions so capture systems know not 
    // to capture them.
    bool RegisterToplevelOwned(HWND hwnd);
    bool UnregisterToplevelOwned(HWND hwnd);
    bool IsToplevelOwned(HWND hwnd);

    static HWND CreateSpawnedWindow(
        const std::wstring & cmd);

    static HWND CreateSpawnedWindow(
        const std::wstring & cmd, 
        int warmup,
        bool closeIfFail,
        bool startShown,
        bool reportOnFail = true);

    static bool ShowNotificationDlgModal();
    static void ShowAboutDlgModal();

    static bool IsNoticeConfirmed();
    static void ConfirmNotice();
    static void ClearNoticeConfirm();

    /// <summary>
    /// Leave TODOs in the the code in the form of window popups.
    /// </summary>
    /// <param name="msg">The TODO message to show.</param>
    static void RaiseTODO(const wxString& msg);

    bool _RegisterCapturedHWND(TopDockWin* owner, HWND winCaptured);

    bool _UnregisterCapturedHWND(HWND winCaptured);
    bool _UnregisterCapturedHWND(std::initializer_list<HWND> winsCaptured);
	
	bool RegisterCaptureDlg(CaptureDlg* dlg);
	bool UnregisterCaptureDlg(CaptureDlg* dlg);

protected:
    DECLARE_EVENT_TABLE();

public:
    bool _TestValidity();
};