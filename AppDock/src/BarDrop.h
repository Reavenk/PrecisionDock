#pragma once

#include <wx/wx.h>

class DockWin;

class BarDrop : public wxTopLevelWindow
{
public:
	DockWin* escapeSink = nullptr;

public:
	BarDrop(wxWindow* parent, DockWin* escapeSink);

	void OnKeyDown(wxKeyEvent& evt);

	DECLARE_EVENT_TABLE();
};