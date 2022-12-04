#pragma once
#include <wx/taskbar.h>

// > ☐ TSKBR_MAIN_189de4b32a00: Application has an icon in the taskbar.

/// <summary>
/// Subsystem that handles the taskbar icon.
/// </summary>
class Taskbar : public wxTaskBarIcon
{
public:

	/// <summary>
	/// wxWidget Command IDs
	/// </summary>
	enum CMDID
	{
		
		/// <summary>
		/// The IDs from 0 to 500 are reserved for OnMenu_ExecuteLaunchID(),
		/// to launch a program. This means we can only support up to 500 programs
		/// ATM. And while we can raise this value as needed, at a certain point
		/// we will run out of reservable numbers because then we will start colliding 
		/// into IDs reserved for wxWidgets. If we ever get to that point, we will need
		/// to figure out another system.
		/// 
		/// For more details, see wxID_LOWEST in wx\defs.h
		/// </summary>
		Ex_StartRange = 0,
		Ex_EndRange = 500,

		Ex_SpawnEmpty,			// See OnMenu_SpawnEmpty()
		Ex_Reload,				// See OnMenu_ReloadLaunches()
		OpenLaunchList,			// See OnMenu_OpenLaunchList()
		PU_EXIT,

		Dlg_Attach,				// TODO: Remove? Used for empty fn OnMenu_DlgAttach()

		ForceCloseAll,			// See OnMenu_CloseAllDocked()
		ReleaseAll,				// See OnMenu_ReleaseAllDocked()
		DetachAll,				// See OnMenu_DetachAllDocked()
		RunTests,				// See OnMenu_RunTests()
		SaveState,				// See OnMenu_SaveState()
		ClearNotification,		// See OnMenu_ClearNotification()
		ShowNotification,		// See OnMenu_ShowNotification()
		ShowAbout,				// See OnMenu_ShowAbout()
		_DBG_Many
	};

public:
	Taskbar();


	/// <summary>
	/// Creates the popup menu when the taskbar icon is
	/// right clicked.
	/// </summary>
	virtual wxMenu *CreatePopupMenu() wxOVERRIDE;

	//////////////////////////////////////////////////
	//
	//	wxWidgets EVENT HANDLERS
	//
	//////////////////////////////////////////////////

	/// <summary>
	/// Handler for when the icon is double clicked.
	/// </summary>
	void OnLeftButtonDClick(wxTaskBarIconEvent& evt);

	// TODO: Remove NOTE when no longer necessary
	// NOTE: Some functions are set to TODO: Remove, because 

	void OnMenuRestore(wxCommandEvent& evt);
	void OnMenuExit(wxCommandEvent& evt);				// Exit application
	void OnMenuSetNewIcon(wxCommandEvent& evt);			// TODO: Remove?
	void OnMenuCheckmark(wxCommandEvent& evt);			// TODO: Remove?
	void OnMenuUICheckmark(wxUpdateUIEvent& evt);		// TODO: Remove?
	void OnMenu_ExecuteLaunchID(wxCommandEvent& evt);
	void OnMenu_ReloadLaunches(wxCommandEvent& evt);	// Refresh launch list from JSON
	void OnMenu_OpenLaunchList(wxCommandEvent& evt);	// Open launch list menu
	void OnMenu_SpawnEmpty(wxCommandEvent& evt);		// Open empty dock window
	
	void OnMenu_DlgAttach(wxCommandEvent& evt);			// TODO: Remove? Forget what this was for

	// Close all dock windows. This will close the contained windows.
	void OnMenu_CloseAllDocked(wxCommandEvent& evt);	
	// Close all dock window - BUT, release the contained windows first.
	void OnMenu_ReleaseAllDocked(wxCommandEvent& evt);
	// Detach all dock windows so that each windows in the system will be a root in its own TopDockWin
	void OnMenu_DetachAllDocked(wxCommandEvent& evt);
	//
	// Run saftey/sanity/state-health checks (debug only)
	void OnMenu_RunTests(wxCommandEvent& evt);			
	void OnMenu_SaveState(wxCommandEvent& evt);			

	void OnMenu_ClearNotification(wxCommandEvent& evt);
	void OnMenu_ShowNotification(wxCommandEvent& evt);
	void OnMenu_ShowAbout(wxCommandEvent& evt);
	void OnMenu_Quit(wxCommandEvent&);					// TODO: Remove

	void OnMenu_DBG_Many(wxCommandEvent&);

	wxDECLARE_EVENT_TABLE();
};