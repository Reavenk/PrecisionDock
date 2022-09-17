#pragma once
#include <wx/wx.h>
#include "Layout/Layout.h"
#include "Layout/LProps.h"
#include "BarDrop.h"
#include <set>
#include "Utils/json.hpp"
#include "DragHelperMgr.h"

using json = nlohmann::json;

class TopDockWin;
class Node;
class DragPreviewOlyWin;

/// <summary>
/// The location in a TopDockWin where the layout of docked windows
/// will be placed.
/// </summary>
class DockWin : public wxWindow
{
	friend DragHelperMgr;

	static int instCtr;
public:
	/// <summary>
	/// The different types of mouse drag operations.
	/// </summary>
	enum class MouseState
	{
		// Not dragging
		Normal,
		// Draggin a tab (usually through delegation of a TabsBar)
		DragTab,
		// Dragging a sash
		DragSash,
		// Anticipate dragging a window. As soon as the node's tab starts
		// being dragged, the state will transition to DragWin.
		DragWin_Anticipate,
		// Dragging a window
		DragWin
	};

	static DragHelperMgrPtr dragggingMgr;

protected:
	
	/// <summary>
	/// The layout of the grid.
	/// </summary>
	::Layout layout;

	/// <summary>
	/// The layout properties.
	/// </summary>
	LProps lprops;

	/// <summary>
	/// The owner window.
	/// </summary>
	TopDockWin* owner;

protected:
	/// <summary>
	/// Called when ANY type of mouse drag operation is finished.
	/// </summary>
	void FinishMouseDrag();

public:
	DockWin(
		wxWindow *parent,
		TopDockWin* owner,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxString& name = wxPanelNameStr);

	~DockWin();

	const LProps& GetLayoutProps() const
	{return this->lprops;}

	inline bool HasRoot() const
	{return this->layout.root != nullptr;}

	/// <summary>
	/// Add a HWND to dock the layout.
	/// </summary>
	/// <param name="hwnd">The HWND dock.</param>
	/// <param name="reference">
	/// The Node used as a reference point of where to dock.
	/// </param>
	/// <param name="refDock">
	/// The location of where in respect to the reference to dock the Window.
	/// </param>
	/// <returns>
	/// If true, the HWND will be docked in the layout system. 
	/// Else if false, an issue was detected and the request will be ignored.
	/// </returns>
	Node* AddToLayout(HWND hwnd, Node* reference, Node::Dest refDock);

	/// <summary>
	/// Take a Node* that used to be a part of another (DockWin) layout, as set it
	/// as the root.
	/// </summary>
	/// <param name="n">The Node to steal as the root of the invoking DockWin's layout.</param>
	void StealRoot(Node* n);

	/// <summary>
	/// Take a Node* that used to be a part of another (DockWin) layout, and add it
	/// to a specific location in the invoking DockWin's layout.
	/// </summary>
	/// <param name="n">The node to steal.</param>
	/// <param name="reference">
	/// The location of where in respect to the reference to dock the Window.
	/// </param>
	/// <param name="refDock">
	/// The location of where in respect to the reference to dock the Window.
	/// </param>
	/// <returns></returns>
	bool StealToLayout(Node* n, Node* reference, Node::Dest refDock);

	/// <summary>
	/// Retrieve a sash who's area covers a specific location.
	/// </summary>
	/// <param name="pt">The location to query.</param>
	/// <returns>
	/// The Sash found at the specified location, or nullptr if none was found.
	/// </returns>
	Sash* SashAtPoint(const wxPoint& pt);

	/// <summary>
	/// Given a point in the entire layout, check if a position is the
	/// notebook tab for 
	/// </summary>
	/// <param name="pt"></param>
	/// <returns></returns>
	Node* GetNodeTab(const wxPoint& pt);

	/// <summary>
	/// Dock a HWND as the layout's root.
	/// </summary>
	/// <param name="hwnd">The HWND to add.</param>
	/// <returns>
	/// The created root Window Node, or nullptr if an issue was encountered.
	/// </returns>
	Node* AddRootHwnd(HWND hwnd);

	/// <summary>
	/// Resize the layout to match the DockWin's current dimensions.
	/// 
	/// This should be called every time the DockWin is resized.
	/// </summary>
	/// <param name="refresh">
	/// If true, redraw the layout. This will probably signal a redraw for all 
	/// docked contents.
	/// </param>
	/// <param name="rebuildSashes">
	/// Rebuild all sashes.
	/// </param>
	// !TODO: Create an explicit ResizeLayout(const wxSize&) function and delegate to that
	void ResizeLayout(bool refresh = true, bool rebuildSashes = true);

	/// <summary>
	/// Ensured the TabsBar for the Node is correct. This includes removing
	/// it when it's no needed, and creating it if it's missing.
	/// </summary>
	/// <param name="pn">The Node to process the TabsBar for.</param>
	void MaintainNodesTabsBar(Node* pn);

	/// <summary>
	/// Clear the layout information. Note that this is only responsible
	/// for clearing the Node information. It will not clean up or close
	/// UI elements for them.
	/// </summary>
	void ClearDocked();

	/// <summary>
	/// Release all contained Window Nodes. This will result in an empty
	/// layout.
	/// </summary>
	/// <param name="clear">
	/// If true, also clear out the layout information.
	/// 
	/// This parameter may be removed (and always assumed true) in the future.
	/// </param>
	void ReleaseAll(bool clear = true);

	/// <summary>
	/// Release a specific Window Node.
	/// </summary>
	/// <param name="pn">The Window Node to remove.</param>
	/// <returns>True if successful.</returns>
	bool ReleaseNodeWin(Node* pn);

	/// <summary>
	/// Detach a Window node.
	/// 
	/// This will result in the Window node being its own
	/// TopDockWin as the root..
	/// </summary>
	/// <param name="pn">
	/// The Window Node to remove and give its own TopDockWin to.
	/// </param>
	/// <returns>True if successful.</returns>
	bool DettachNodeWin(Node* pn);

	/// <summary>
	/// Close a Window node.
	/// </summary>
	/// <param name="pn">The Window Node to close.</param>
	/// <returns>True if successfully closed.</returns>
	bool CloseNodeWin(Node* pn);

	/// <summary>
	/// Create a Node spawned from the exact same command as another node.
	/// 
	/// If successful, the created Window Node will added into a Tab node with
	/// the specified Window node.
	/// </summary>
	/// <param name="pn">The Window node to create a duplicate process of.</param>
	/// <returns>True if the node was cloned.</returns>
	bool CloneNodeWin(Node* pn);

	/// <summary>
	/// Handle when a tab is clicked.
	/// 
	/// Includes logic to prepare for a tab dragging operation.
	/// </summary>
	/// <param name="tbInvoker">
	/// The TabsBar that is delegating the call.
	/// </param>
	/// <param name="node">
	/// The node of the tab being dragged.
	/// </param>
	/// <param name="tabOwner">
	/// The TabNode that is the parent of the Node being draggged.
	/// </param>
	/// <param name="closePressed">
	/// True if the click was on the tab's close button.
	/// </param>
	void TabClickStart(TabsBar* tbInvoker, Node* node, Node* tabOwner, bool closePressed);

	/// <summary>
	/// Handles when the mouse is moved during a tab drag.
	/// 
	/// This will be called via delegation when the delegated object is deletects
	/// a mouse move (while it has the mouse capture).
	/// </summary>
	void TabClickMotion();

	/// <summary>
	/// Handle when a tab drag has completed.
	/// 
	/// This will be called via delegation when the delegated object is no longer
	/// being dragged.
	/// </summary>
	void TabClickEnd();

	/// <summary>
	/// Called from a contained UI element that's in a drag operation, that is
	/// cancelling the drag operation - most likely because the escape key was
	/// pressed.
	/// </summary>
	void TabClickCancel();

	/// <summary>
	/// Handles canceling a drag operation when being cancelled from delegation.
	/// </summary>
	void OnDelegatedEscape(); // TODO: Encapsulate - and expect delegation from TabClickCancel

	inline void RebuildSashes()
	{
		this->layout.RebuildSashes(this->lprops);
	}

	void DelegateFinishMouseDrag()
	{
		// We could just make FinishMouseDrag() public, the only advantage this
		// gives us is that we have a bottleneck of where all outside code
		// is calling FinishMouseDrag().
		this->FinishMouseDrag();
	}

	/// <summary>
	/// Called to handle when a node is removed. This includes,
	/// * When the Window for a node is closed.
	/// * When a Window node is released.
	/// * When a Node is moved/stolen.
	/// </summary>
	void _ReactToNodeRemoval();

	//////////////////////////////////////////////////
	//
	//	wxWidget EVENT HANDLERS
	//
	//////////////////////////////////////////////////
	void OnDraw(wxPaintEvent& paint);
	void OnSize(wxSizeEvent& evt);
	void OnClose(wxCloseEvent& evt);

	void OnMouseLDown(wxMouseEvent& evt);
	void OnMouseLUp(wxMouseEvent& evt);
	void OnMouseMDown(wxMouseEvent& evt);
	void OnMouseMUp(wxMouseEvent& evt);
	void OnMouseRDown(wxMouseEvent& evt);
	void OnMouseRUp(wxMouseEvent& evt);
	void OnMouseMotion(wxMouseEvent& evt);
	void OnMouseCaptureChanged(wxMouseCaptureChangedEvent& evt);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);

	void OnKeyDown(wxKeyEvent& evt);
	void OnKeyUp(wxKeyEvent& evt);

protected:
	DECLARE_EVENT_TABLE();

public:
	json _JSONRepresentation();
	bool _TestValidity();
};