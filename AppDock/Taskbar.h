#pragma once
#include <wx/taskbar.h>
class Taskbar : public wxTaskBarIcon
{
public:
	enum CMDID
	{
		
		Ex_StartRange = 0,
		Ex_EndRange = 500,
		Ex_SpawnEmpty,
		Ex_Reload,
		OpenLaunchList,
		PU_EXIT,

		Dlg_About,
		Dlg_Attach,

		CloseAll,
		ReleaseAll,
		RunTests,
		ClearNotification,
		ShowNotification,
		ShowAbout
	};

public:
	Taskbar();

	void OnLeftButtonDClick(wxTaskBarIconEvent& evt);
	void OnMenuRestore(wxCommandEvent& evt);
	void OnMenuExit(wxCommandEvent& evt);
	void OnMenuSetNewIcon(wxCommandEvent& evt);
	void OnMenuCheckmark(wxCommandEvent& evt);
	void OnMenuUICheckmark(wxUpdateUIEvent& evt);
	void OnMenuSub(wxCommandEvent& evt);

	void OnMenu_ExecuteLaunchID(wxCommandEvent& evt);
	void OnMenu_ReloadLaunches(wxCommandEvent& evt);
	void OnMenu_OpenLaunchList(wxCommandEvent& evt);
	void OnMenu_SpawnEmpty(wxCommandEvent& evt);
	

	void OnMenu_DlgAttach(wxCommandEvent& evt);

	void OnMenu_CloseAllDocked(wxCommandEvent& evt);
	void OnMenu_ReleaseAllDocked(wxCommandEvent& evt);
	void OnMenu_RunTests(wxCommandEvent& evt);
	void OnMenu_ClearNotification(wxCommandEvent& evt);
	void OnMenu_ShowNotification(wxCommandEvent& evt);
	void OnMenu_ShowAbout(wxCommandEvent& evt);
	void OnMenu_Quit(wxCommandEvent&);

	virtual wxMenu *CreatePopupMenu() wxOVERRIDE;

	wxDECLARE_EVENT_TABLE();
};