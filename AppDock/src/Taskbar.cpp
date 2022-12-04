#include "Taskbar.h"
#include <wx/menu.h>
#include "AppDock.h"
#include "CaptureDlg/CaptureDlg.h"
#include "TopDockWin.h"
#include <fstream>

wxBEGIN_EVENT_TABLE(Taskbar, wxTaskBarIcon)
	EVT_MENU(CMDID::PU_EXIT,			Taskbar::OnMenuExit)
	EVT_TASKBAR_LEFT_DCLICK (			Taskbar::OnLeftButtonDClick)

	EVT_MENU_RANGE(CMDID::Ex_StartRange, CMDID::Ex_EndRange, Taskbar::OnMenu_ExecuteLaunchID)
	EVT_MENU(CMDID::Ex_Reload,			Taskbar::OnMenu_ReloadLaunches      )
	EVT_MENU(CMDID::Ex_SpawnEmpty,		Taskbar::OnMenu_SpawnEmpty          )
	//EVT_MENU(CMDID::Dlg_About,		Taskbar::OnMenu_About)
	EVT_MENU(CMDID::Dlg_Attach,			Taskbar::OnMenu_DlgAttach           )
	EVT_MENU(CMDID::ReleaseAll,			Taskbar::OnMenu_ReleaseAllDocked    )
	EVT_MENU(CMDID::DetachAll,			Taskbar::OnMenu_DetachAllDocked     )
	EVT_MENU(CMDID::ForceCloseAll,		Taskbar::OnMenu_CloseAllDocked      )
	EVT_MENU(CMDID::OpenLaunchList,		Taskbar::OnMenu_OpenLaunchList		)
	EVT_MENU(CMDID::RunTests,			Taskbar::OnMenu_RunTests			)
	EVT_MENU(CMDID::SaveState,			Taskbar::OnMenu_SaveState			)
	EVT_MENU(CMDID::ClearNotification,  Taskbar::OnMenu_ClearNotification   )
	EVT_MENU(CMDID::ShowNotification,	Taskbar::OnMenu_ShowNotification	)
	EVT_MENU(CMDID::ShowAbout,			Taskbar::OnMenu_ShowAbout			)

	EVT_MENU(CMDID::_DBG_Many,			Taskbar::OnMenu_DBG_Many			)
wxEND_EVENT_TABLE()

Taskbar::Taskbar()
{
}

void Taskbar::OnMenuRestore(wxCommandEvent& )
{
	//gs_dialog->Show(true);
}

void Taskbar::OnMenuExit(wxCommandEvent& )
{
	// > ☐ TSKBR_MAIN_b2287bf38ad2: Taskbar has option to close the system.
	// > ☐ TSKBR_MAIN_10ded6c7be77: Taskbar close option works as expected.

	//gs_dialog->Close(true);
	AppDock::GetApp().ForceCloseAll();
	AppDock::GetApp().Exit();
}

static bool check = true;

void Taskbar::OnMenuCheckmark(wxCommandEvent& )
{
	check = !check;
}

void Taskbar::OnMenuUICheckmark(wxUpdateUIEvent &event)
{
	event.Check(check);
}

void Taskbar::OnMenuSetNewIcon(wxCommandEvent&)
{
	//wxIcon icon(smile_xpm);
	//
	//if (!SetIcon(wxBitmapBundle::FromBitmaps(
	//    wxBitmap(smile_xpm),
	//    wxBitmap(smile2_xpm)
	//),
	//    "wxTaskBarIcon Sample - a different icon"))
	//    wxMessageBox("Could not set new icon.");
}

void Taskbar::OnMenu_ExecuteLaunchID(wxCommandEvent& evt)
{
	const std::vector<AppRef>& v = AppDock::GetApp().ReferencedApps();
	int idx = evt.GetId() - CMDID::Ex_StartRange;
	if(idx >= 0 && idx < v.size())
		AppDock::GetApp().LaunchAppRef(v[idx]);
}

void Taskbar::OnMenu_ReloadLaunches(wxCommandEvent&)
{
	AppDock::GetApp().ReloadAppRefs();
}

void Taskbar::OnMenu_OpenLaunchList(wxCommandEvent&)
{
	std::string launchPath = AppDock::GetAppListFilename();
	std::string cmd = std::string("notepad ") + launchPath;
	system(cmd.c_str());
}

void Taskbar::OnMenu_SpawnEmpty(wxCommandEvent&)
{
	AppDock::GetApp().SpawnEmpty("");
}

void Taskbar::OnMenu_DlgAttach(wxCommandEvent&)
{
}

void Taskbar::OnMenu_CloseAllDocked(wxCommandEvent&)
{
	AppDock::GetApp().ForceCloseAll();
}

void Taskbar::OnMenu_ReleaseAllDocked(wxCommandEvent&)
{
	AppDock::GetApp().ReleaseAll();
}

void Taskbar::OnMenu_DetachAllDocked(wxCommandEvent& evt)
{
	AppDock::GetApp().DetachAll();
}

void Taskbar::OnMenu_RunTests(wxCommandEvent&)
{
	if(AppDock::GetApp()._TestValidity() == true)
		this->ShowBalloon("Tests Passed!", "The application passed the full test suite.");
}

void Taskbar::OnMenu_SaveState(wxCommandEvent& evt)
{
	json jsrepr = json::array();
	std::vector<TopDockWin*> wins = AppDock::GetApp()._GetWinList();
	for(TopDockWin* tdw : wins)
	{
		if(!tdw->HasRoot())
			continue;

		jsrepr.push_back(tdw->_JSONRepresentation());
	}
	std::ofstream ofs("SavedState.json");
	ofs << jsrepr.dump(4);
	ofs.close();

}

void Taskbar::OnMenu_ClearNotification(wxCommandEvent& evt)
{
	AppDock::ClearNoticeConfirm();
}

void Taskbar::OnMenu_ShowNotification(wxCommandEvent& evt)
{
	AppDock::ShowNotificationDlgModal();
}

void Taskbar::OnMenu_ShowAbout(wxCommandEvent& evt)
{
	// > ☐ TSKBR_OPTS_5fd68ae5d901: Taskbar to have an option to show the About dialog.
	// > ☐ TSKBR_OPTS_7a3f6f0fadfe: Taskbar's About works as expected.

	AppDock::ShowAboutDlgModal();
}

void Taskbar::OnMenu_Quit(wxCommandEvent&)
{
}

// Overridables
wxMenu *Taskbar::CreatePopupMenu()
{
	wxMenu *menu = new wxMenu;

	wxMenu* submSpawn = new wxMenu;
	{
		const std::vector<AppRef>& refs = AppDock::GetApp().ReferencedApps();

		submSpawn->Append(CMDID::Ex_Reload,			"Reload References");
		submSpawn->Append(CMDID::OpenLaunchList,	"View References");
		submSpawn->AppendSeparator();

		for(size_t i = 0; i < refs.size(); ++i)
			submSpawn->Append(CMDID::Ex_StartRange + i, refs[i].label);
	}
	menu->Append(-1, "Spawn", submSpawn);
	menu->Append(CMDID::Ex_SpawnEmpty,  "Spawn Empty Window");
	
	menu->AppendSeparator();

	menu->Append(CMDID::ReleaseAll,			"Release All Docked");
	menu->Append(CMDID::DetachAll,			"Detach All");
	menu->Append(CMDID::ForceCloseAll,		"Force Close All Docked");

	menu->AppendSeparator();

#if _DEBUG
	menu->Append(CMDID::RunTests,			"Run Debug Tests");
	menu->Append(CMDID::SaveState,			"Save States");
#endif
	menu->Append(CMDID::ClearNotification,	"Clear Notification");
	menu->Append(CMDID::ShowNotification,	"Warning");
	menu->Append(CMDID::ShowAbout,			"About");
	menu->Append(PU_EXIT,					"E&xit");

	return menu;
}

void Taskbar::OnLeftButtonDClick(wxTaskBarIconEvent& evt)
{
	CaptureDlg* capDlg = CaptureDlg::GetInstance();
	capDlg->Freeze();

	wxPoint pt = wxGetMousePosition();
	wxSize dlgSz = capDlg->GetSize();
	capDlg->SetPosition(
		wxPoint(
			pt.x - dlgSz.x, 
			pt.y - dlgSz.y - 10));
	capDlg->Thaw();
}

void Taskbar::OnMenu_DBG_Many(wxCommandEvent&)
{
	const std::vector<AppRef>& v = AppDock::GetApp().ReferencedApps();
	AppDock::GetApp().LaunchAppRef(v[0]);
	AppDock::GetApp().LaunchAppRef(v[0]);
	AppDock::GetApp().LaunchAppRef(v[0]);
}