#pragma once
#include <wx/wx.h>
#include <set>
#include "Utils/json.hpp"
#include "DockWin.h"

using json = nlohmann::json;

class Node;

/// <summary>
/// Reasons why a TopDockWin would get a notification that a window was lost.
/// </summary>
enum LostWindowReason
{
    /// <summary>
    /// The window was captured by another TopDockWin.
    /// </summary>
    Recaptured,

    /// <summary>
    /// The window was closed. Either by the app, or by external systems.
    /// </summary>
    Destroyed
};

/// <summary>
/// The top-level window where contained (previously) top-level windows
/// will be placed.
/// 
/// This mostly handles the top-level window functionality and high-level
/// management. For the lower-level layout and UI containment, see the
/// dockWin member.
/// </summary>
class TopDockWin: public wxFrame
{
private:

    /// <summary>
    /// wxWidgets Command IDs
    /// </summary>
    enum class CMDID
    {
        ToggleStatusbar,
        ReleaseAll,
        DetachAll
    };

    static int      _InstCtr;

private:
    /// <summary>
    /// The DockWin containing the region where docked windows will be placed,
    /// alongwith the layout information.
    /// </summary>
    DockWin*        dockWin;

    /// <summary>
    /// The status bar. This may be null, in which case the top-level doesn't
    /// have a status bar.
    /// </summary>
    wxStatusBar*    statusBar = nullptr;

    /// <summary>
    /// The HWND of the window.
    /// 
    /// Upon creation, the TopDockWin is registered to make sure the application
    /// doesn't try to capture in a layout. This cached HWND is used to properly 
    /// unregister the node from the system after the TopDockWin is closed.
    /// window is closed.
    /// </summary>
    HWND cachedHWND = NULL;

public:
    TopDockWin(const wxPoint& pos, const wxSize& size);
    ~TopDockWin();

    /// <summary>
    /// Set the HWND for the root node of the layout system.
    /// 
    /// Should only be called when the layout system is empty.
    /// </summary>
    /// <param name="hwnd">
    /// The HWND of a top-level Window to set as the root of the layout.
    /// </param>
    /// <returns>True, if successful.</returns>
    Node* SetRoot(HWND hwnd);

    /// <summary>
    /// Set the root as a Node* that used to be part of another dock layout.
    ///
    /// This is done when taking a Node in a layout and "plucking" it out to
    /// make it its own TopDockWin.
    /// </summary>
    /// <param name="pn">The Node to take.</param>
    /// <param name="stolenFrom">The DockWin that originally owned the Node.<param>
    void StealRoot(Node* pn, DockWin* stolenFrom);

    /// <summary>
    /// Check if the root node is non-null.
    /// </summary>
    bool HasRoot() const;

    inline DockWin * GetDockWin()
    { return this->dockWin; }

    /// <summary>
    /// Release all Docked Windows in the layout.
    /// 
    /// This will restore all docked windows as undocked, and clear the layout.
    /// </summary>
    void ReleaseAll();

    void DetachAll();

    void UpdateTitlebar();

public:
    static inline int ClassInstCtr()
    { return _InstCtr;}

    static TopDockWin * GetWinAt(const wxPoint& screenMouse, const std::set<TopDockWin*>& ignores);
    static TopDockWin * GetWinAt(const wxPoint& screenMouse, TopDockWin* ignore);
    static TopDockWin * GetWinAt(const wxPoint& screenMouse);

    //////////////////////////////////////////////////
    //
    //  System Event Handlers
    //
    //////////////////////////////////////////////////
    void OnDetectLostWindow(HWND win, LostWindowReason why);
    void UpdateWindowTitlebar(HWND win);

	void OnDockWin_Added(HWND hwnd, Node* n);
	void OnDockWin_Removed(HWND hwnd, LostReason lr);
    void OnDockWin_TitleModified(Node* n);
private:


    //////////////////////////////////////////////////
    //
    //  wxWidget EVENT HANDLERS
    //
    //////////////////////////////////////////////////

    void OnMenu_ToggleStatusbar(wxCommandEvent& evt);
    void OnMenu_DetachAll(wxCommandEvent& evt);
    void OnMenu_ReleaseAll(wxCommandEvent& evt);
    void OnExit(wxCommandEvent& event);

public:
    json _JSONRepresentation();
    bool _TestValidity();

protected:
    wxDECLARE_EVENT_TABLE();
};