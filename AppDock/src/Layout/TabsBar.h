#pragma once
#include <wx/wx.h>
#include "Node.h"

class DockWin;

/// <summary>
/// The window containing the droppable tabs for nodes.
/// 
/// This is in its own window to localize where the drawings
/// are, and so that tab drawing can be double buffered without
/// forcing the main DockWin canvas to be double buffered.
/// </summary>
class TabsBar : public wxWindow
{
public:
	// wxWidget Command IDs
	enum class CmdIds
	{
		Menu_ShowTBarCustom,
		Menu_ShowTBarOriginal,
		Menu_ShowTBarCmdLine,
		Menu_CloneWin,			// See OnMenu_RClick_Clone()
		Menu_RenameWin,			// See OnMenu_RClick_Rename()
		Menu_ReleaseWin,		// See OnMenu_RClick_Release()
		Menu_CloseWin,			// See OnMenu_RClick_CloseWin()
		Menu_DetachWin,			// See OnMenu_RClick_DetachWin()
		Menu_SystemMenu			// See OnMenu_RClick_SystemMenu()
	};

private:
	/// <summary>
	/// The counter used to create debug IDs for each TabsBar created.
	/// </summary>
	static int dbgCtr;

	// Count how many of these objects there are.
	static int instCtr;

public: // TODO: Encapsulate better.

	int id;

	/// <summary>
	/// A reference to the Tab node that owns us.
	/// </summary>
	Node* node				= nullptr;

	/// <summary>
	/// A reference to the Dock win that owns the layout the 
	/// TabsBar object is in.
	/// </summary>
	DockWin* owner			= nullptr;

	/// <summary>
	/// When a node tab is right clicked, this stores the
	/// node selected so if it's operated on the right-click
	/// menu, the menu callback knows the what node to target.
	/// </summary>
	Node* nodeRightClicked	= nullptr;

	/// <summary>
	/// The node that the mouse was last known to be over.
	/// </summary>
	Node * nodeOfTabHoveredOver		= nullptr;

	/// <summary>
	/// If true, the mouse was last known to be over the close button of hoverOver.
	/// </summary>
	bool hoveringOverClose		= false;

protected:
	void ClearHover();
	bool UpdateMouseOver(const wxPoint& mousePt);

public: // Public methods

	/// <summary>
	/// Constructor.
	/// </summary>
	/// <param name="win">
	/// The DockWin containing the Tab Node that the TabsBar is 
	/// managing.
	/// </param>
	/// <param name="node">
	/// The TabNode that the TabsBar is responsible for.
	/// </param>
	TabsBar(DockWin* win, Node* node);

	~TabsBar();

	/// <summary>
	/// If the tabs have been moved to a different window, this can
	/// be used to update this->owner.
	/// </summary>
	/// <param name="win">
	/// The DockWin that now contains the Tab Node that the TabsBar
	/// is responsible for.
	/// </param>
	void SwapOwner(DockWin* win);

	/// <summary>
	/// Check to see what tab is at a local screen point.
	/// </summary>
	/// <param name="pt">The local point to check.</param>
	/// <returns>
	/// The Node whos tab is at the specified pt. Or nullptr if
	/// none was detected.
	/// </returns>
	Node* GetTabAtPoint(const wxPoint& pt);

	/// <summary>
	/// Event, called when the TabsBar is holding a window that was torn.
	/// </summary>
	void OnWindowTorn();

	/// <summary>
	/// Event, called when the TabsBar is holding a tab, whos child window
	/// was torn.
	/// </summary>
	/// <param name="nodeTorn">
	/// The window that was torn. The node will have been a child, but 
	/// was recently removed.
	/// </param>
	void OnTabTorn(Node* nodeTorn);

	/// <summary>
	/// Change what kind of titlebar is displayed for a node displayed in the
	/// TabsBar's system.
	/// </summary>
	/// <param name="node">
	/// The node to change the titlebar type for. This should only be a node that
	/// is in the node (or a child of the node) that the TabsBar is for.
	/// </param>
	/// <param name="tbarTy">
	/// The titlebar type to display.
	/// </param>
	/// <param name="force">
	/// If true, the operation will always happen. Else if tbarTy is the same as
	/// node's current titlebar type, the request will be ignored.
	/// </param>
	/// <return>If modified, true.</return>
	bool ChangeTBarType(Node* node, Node::TabNameType tbarTy, bool force = false);

	/// <summary>
	/// Open a docked window's system menu below the tab.
	/// The function also handles redirecting the system menu to the application.
	/// The menu will be modal.
	/// </summary>
	/// <param name="node">The docked window to open the menu for.</param>
	void OpenSystemMenu(Node* node);

	//////////////////////////////////////////////////
	//
	//	wxWidgets EVENT HANDLERS
	//
	//////////////////////////////////////////////////
	void OnDraw(wxPaintEvent& evt);
	void OnMouseLDown(wxMouseEvent& evt);
	void OnMouseLUp(wxMouseEvent& evt);
	void OnMouseMotion(wxMouseEvent& evt);
	void OnMouseRDown(wxMouseEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
	void OnMouseChanged(wxMouseCaptureChangedEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void OnMouseExit(wxMouseEvent& evt);

	void OnMenu_RClick_Clone(wxCommandEvent& evt);
	void OnMenu_RClick_Rename(wxCommandEvent& evt);
	void OnMenu_RClick_Release(wxCommandEvent& evt);
	void OnMenu_RClick_CloseWin(wxCommandEvent& evt);
	void OnMenu_RClick_DetachWin(wxCommandEvent& evt);
	void OnMenu_RClick_SystemMenu(wxCommandEvent& evt);
	void OnMenu_RClick_ShowTBarCustom(wxCommandEvent& evt);
	void OnMenu_RClick_ShowTBarOriginal(wxCommandEvent& evt);
	void OnMenu_RClick_ShowTBarCmdLine(wxCommandEvent& evt);

protected:
	DECLARE_EVENT_TABLE();

public: // Static public methods
	const static wxBitmap& GetCloseBtnBitmap();

public:
	bool _TestValidity();
};

