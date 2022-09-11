#pragma once
#include <wx/wx.h>

class CaptureDlg;

/// <summary>
/// A selectable item in the CaptureDlg's scroll window 
/// that represents a toplevel HWND that can be captured
/// into a TopDockWin.
/// </summary>
class CaptureListItem : public wxWindow
{
public: // Public static members

	/// <summary>
	/// The size for drawn icons.
	/// </summary>
	static const wxSize iconRegionSize;

	/// <summary>
	/// The sizer border on all edges.
	/// </summary>
	static const int border = 10;

public: // Public members

	/// <summary>
	/// The owner to delegate events to.
	/// </summary>
	CaptureDlg* owner = nullptr;

	/// <summary>
	/// The toplevel HWND to capture if selected.
	/// </summary>
	HWND captureHWND = NULL;

	/// <summary>
	/// Cached HICON of the capturable window's taskbar icon.
	/// </summary>
	HICON captureIcon = NULL;

	/// <summary>
	/// If true, the item is selected in the dialog.
	/// </summary>
	bool isSel = false;

	/// <summary>
	/// The reserved space in the sizer where the icon should be drawn.
	/// </summary>
	wxSizerItem* iconSpace = nullptr;

	/// <summary>
	/// Text label.
	/// </summary>
	wxStaticText* captureLabel = nullptr;

	/// <summary>
	/// Counter to check if the mouse is over this window. This is a counter
	/// instead of a bool because multiple things will increment this -
	/// the other thing being the static text (captureLabel).
	/// </summary>
	int hoverCounter = 0;

public:
	CaptureListItem(CaptureDlg* owner, wxWindow* parent, HWND hwnd, HICON icon, const wxString& title);

	/// <summary>
	/// Update the background color based on if the item is selected,
	/// and if the mouse is hovering over the item.
	/// </summary>
	/// <param name="refresh">
	/// If true, also redraw the item after updating its background color.
	/// </param>
	void UpdateBackgroundColor(bool refresh = true);

	//////////////////////////////////////////////////
	//
	//	wxWidget CALLBACKS
	//
	//////////////////////////////////////////////////
	void OnPaint(wxPaintEvent& evt);
	void OnLeftDown(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnSetFocus(wxFocusEvent& evt);
	void OnKillFocus(wxFocusEvent& evt);
	void OnKeydown(wxKeyEvent& evt);

public: // Public methods

	/// <summary>
	/// Filter the properties of a HWND to see if it should be an element
	/// that populated the capture window.
	/// </summary>
	/// <param name="icon">The toplevel's taskbar icon.</param>
	/// <param name="appTitle">The toplevel's label.</param>
	/// <returns>
	/// If true, the parameters are valid and can be used as an element in the GUI.
	/// </returns>
	static bool IsValid(HICON icon, wxString& appTitle);

protected:
	DECLARE_EVENT_TABLE()
};