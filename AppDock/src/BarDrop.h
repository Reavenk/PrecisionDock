#pragma once

#include <wx/wx.h>

class DockWin;

/// <summary>
/// Visual representation of where a window would be dropped 
/// into a layout.
/// </summary>
class BarDrop : public wxFrame
{
public:	DockWin* escapeSink = nullptr;

public:
	
	BarDrop(wxWindow* parent, DockWin* escapeSink);
	void OnKeyDown(wxKeyEvent& evt);

	DECLARE_EVENT_TABLE();
};