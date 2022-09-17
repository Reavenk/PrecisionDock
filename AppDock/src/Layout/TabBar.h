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
	// wxWidget Command IDs
	enum class CmdIds
	{
		Menu_CloneWin,		// See OnMenu_RClick_Clone()
		Menu_RenameWin,		// See OnMenu_RClick_Rename()
		Menu_ReleaseWin,	// See OnMenu_RClick_Release()
		Menu_CloseWin,		// See OnMenu_RClick_CloseWin()
		Menu_DettachWin,	// See OnMenu_RClick_DettachWin()
		Menu_SystemMenu		// See OnMenu_RClick_SystemMenu()
	};

public: // TODO: private static members
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
	/// TabBar object is in.
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
	Node * tabHoveringOver		= nullptr;

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
	/// The DockWin containing the Tab Node that the TabBar is 
	/// managing.
	/// </param>
	/// <param name="node">
	/// The TabNode that the TabBar is responsible for.
	/// </param>
	TabBar(DockWin* win, Node* node);

	~TabBar();

	/// <summary>
	/// If the tabs have been moved to a different window, this can
	/// be used to update this->owner.
	/// </summary>
	/// <param name="win">
	/// The DockWin that now contains the Tab Node that the TabBar
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
	/// Event, called when the TabBar is holding a window that was torn.
	/// </summary>
	void OnWindowTorn();

	/// <summary>
	/// Event, called when the TabBar is holding a tab, whos child window
	/// was torn.
	/// </summary>
	/// <param name="nodeTorn">
	/// The window that was torn. The node will have been a child, but 
	/// was recently removed.
	/// </param>
	void OnTabTorn(Node* nodeTorn);

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
	void OnMenu_RClick_DettachWin(wxCommandEvent& evt);
	void OnMenu_RClick_SystemMenu(wxCommandEvent& evt);

protected:
	DECLARE_EVENT_TABLE();

public: // Static public methods
	const static wxBitmap& GetCloseBtnBitmap();

public:
	bool _TestValidity();
};

