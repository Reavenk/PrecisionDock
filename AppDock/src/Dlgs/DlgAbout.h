#pragma once

#include <wx/wx.h>

/// <summary>
/// The about dialog.
/// </summary>
class DlgAbout : public wxDialog
{
private: // Private members

	/// <summary>
	/// The window to the about dialog, to make sure the application
	/// cannot try to select and dock it.
	/// 
	/// We cache this instead of always using this->GetHWND() because
	/// it may be the case that the HWND could be invalid 
	/// </summary>
	HWND cachedHWND = NULL;

public: // Public methods

	DlgAbout();
	~DlgAbout();

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