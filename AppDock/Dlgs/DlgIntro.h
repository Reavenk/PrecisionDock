#pragma once

#include <wx/wx.h>

class DlgIntro : public wxDialog
{
public:
	DlgIntro();
	~DlgIntro();

	void OnButtonOK(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
protected:
	DECLARE_EVENT_TABLE()
};