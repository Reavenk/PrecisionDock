#pragma once

#include <wx/wx.h>

class DlgAbout : public wxDialog
{
private:
	HWND cachedHWND = NULL;

public:
	DlgAbout();
	~DlgAbout();

	void OnButtonOK(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
protected:
	DECLARE_EVENT_TABLE()
};