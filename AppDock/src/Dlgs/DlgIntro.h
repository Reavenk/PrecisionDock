#pragma once

#include <wx/wx.h>

/// <summary>
/// The dialog that shows up the first time the application is run.
/// </summary>
class DlgIntro : public wxDialog
{
public: // public methods

	DlgIntro();
	~DlgIntro();

	//////////////////////////////////////////////////
	//
	//	wxWidgets EVENT HANDLERS
	//
	//////////////////////////////////////////////////
	void OnButtonOK(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);

protected:
	DECLARE_EVENT_TABLE()
};