#pragma once

#include <wx/wx.h>

class DockWin;

/// <summary>
/// Visual representation of where a window would be dropped 
/// into a layout.
/// </summary>
class BarDrop : public wxTopLevelWindow
{
public:
	// TODO: Remove? Is this ever used? (Do not confused with BarDrag::escapeSink)
	DockWin* escapeSink = nullptr;

public:
	
	BarDrop(wxWindow* parent, DockWin* escapeSink);

	// TODO: Remove? Is this ever used? (Do not confused with BarDrag::OnKeyDown)
	void OnKeyDown(wxKeyEvent& evt);

	DECLARE_EVENT_TABLE();
};