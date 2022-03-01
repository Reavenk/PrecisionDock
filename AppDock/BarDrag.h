#pragma once

#include <wx/wx.h>

class DockWin;

class BarDrag : public wxTopLevelWindow
{
public:

	DockWin* escapeSink;

public:
	BarDrag(wxWindow * parent, DockWin* escapeSink);

	void OnKeyDown(wxKeyEvent& evt);

	DECLARE_EVENT_TABLE();
};