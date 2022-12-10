#pragma once

#include <wx/wx.h>

class DockWin;

/// <summary>
/// When dragging a tab to drop somewhere, a BarDrag will represent the
/// graphical representation of the application, which will be placed
/// next to the mouse.
/// 
/// </summary>
class DragPreviewOlyWin : public wxFrame
{
public:

	/// <summary>
	/// The DockWin to notify if the Escape key was pressed.
	/// </summary>
	DockWin* escapeSink;

public:
	/// <summary>
	/// Constructor.
	/// </summary>
	/// <param name="parent">
	/// The parent window.
	/// </param>
	/// <param name="escapeSink">
	/// The DockWin to notify if the Escape key was pressed during the operation.
	/// </param>
	DragPreviewOlyWin(wxWindow * parent, DockWin* escapeSink);

	/// <summary>
	/// Keyboard presses to handle if the user presses Escape
	/// while dragging.
	/// 
	/// This is set up this way because while dragging, the BarDrop needs mouse
	/// capture focus which forces it to also have keyboard focus, but it is not
	/// responsible for managing high-level aspects of the drag operation - so it
	/// must detect the Escape key press, but also needs to notify/delegate the
	/// manager.
	/// </summary>
	void OnKeyDown(wxKeyEvent& evt);

	DECLARE_EVENT_TABLE();
};