#pragma once
#include <wx/wx.h>

class Node;
class DockWin;

/// <summary>
/// The window containing the droppable tabs for nodes.
/// 
/// This is in its own window to localize where the drawings
/// are, and so that tab drawing can be double buffered without
/// forcing the main DockWin canvas to be double buffered.
/// </summary>
class TabBar : public wxWindow
{
public:
	enum class CmdIds
	{
		Menu_CloneWin,
		Menu_RenameWin,
		Menu_ReleaseWin,
		Menu_CloseWin,
		Menu_DettachWin,
		Menu_SystemMenu
	};

public:
	Node* node				= nullptr;
	DockWin* owner			= nullptr;

	/// <summary>
	/// When a node tab is right clicked, this stores the
	/// node selected so if it's operated on the right-click
	/// menu, the menu callback knows the what node to target.
	/// </summary>
	Node* nodeRightClicked	= nullptr;

public:
	TabBar(DockWin* win, Node* node);

	void SwapOwner(DockWin* win);

	Node* GetTabAtPoint(const wxPoint& pt);

	void OnDraw(wxPaintEvent& evt);
	void OnMouseLDown(wxMouseEvent& evt);
	void OnMouseLUp(wxMouseEvent& evt);
	void OnMouseMotion(wxMouseEvent& evt);
	void OnMouseRDown(wxMouseEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
	void OnMouseChanged(wxMouseCaptureChangedEvent& evt);

	void OnMenu_RClick_Clone(wxCommandEvent& evt);
	void OnMenu_RClick_Rename(wxCommandEvent& evt);
	void OnMenu_RClick_Release(wxCommandEvent& evt);
	void OnMenu_RClick_CloseWin(wxCommandEvent& evt);
	void OnMenu_RClick_DettachWin(wxCommandEvent& evt);
	void OnMenu_RClick_SystemMenu(wxCommandEvent& evt);

protected:
	DECLARE_EVENT_TABLE();

public:
	bool _TestValidity();
};

