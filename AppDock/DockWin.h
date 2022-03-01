#pragma once
#include <wx/wx.h>
#include "Layout/Layout.h"
#include "Layout/LProps.h"
#include "BarDrop.h"
#include <set>

class TopDockWin;
class Node;
class BarDrag;

/// <summary>
/// 
/// </summary>
class DockWin : public wxWindow
{
public:
	/// <summary>
	/// The different types of mouse drag operations.
	/// </summary>
	enum class MouseState
	{
		// Not dragging
		Normal,
		// Dragging a sash
		DragSash,
		// Anticipate dragging a window. As soon as the node's tab starts
		// being dragged, the state will transition to DragWin.
		DragWin_Anticipate,
		// Dragging a window
		DragWin
	};

protected:
	/// <summary>
	/// When dragging a node to drop somewhere else, this is
	/// the graphical element that follow the cursor to give
	/// visual feedback of being in the drag mode.
	/// </summary>
	static BarDrag* barDrag;

	/// <summary>
	/// When dragging a node to drop somewhere else, this is
	/// the preview of where the window will be dropped.
	/// </summary>
	static BarDrop* dropPreviewWin;

	/// <summary>
	/// When dragging a node to another window, this is the 
	/// current node the cursor on top is stored here until 
	/// the mouse is released.
	/// </summary>
	static TopDockWin* winDraggedOnto;

	/// <summary>
	/// When dragging a node, this is current location to drop
	/// the node into.
	/// </summary>
	static DropResult droppedDst;

	/// <summary>
	/// When ripping a tab out, the Node is removed, but
	/// kept around in memory until the drag is finished
	/// in case the user cancels. If the user cancels, the
	/// info in dragUndo contains instructions on how to
	/// reinsert the node(s) and restore them.
	/// </summary>
	static std::vector<Layout::ForgetUndo> dragUndo;

	/// <summary>
	/// The node that is being dragged and dropped.
	/// </summary>
	static Node* nodeDragged;

	/// <summary>
	/// The nodes that were removed in a drag-node operation.
	/// 
	/// If a drag is not undone and commited to (i.e., the user
	/// drops a node somewhere instead of pressing escape to bail
	/// from the drag node operation) any nodes that were removed
	/// and no longer relevant are deleted.
	/// </summary>
	static std::set<Node*> dragNodesInvolved;

	/// <summary>
	/// When dragging a sash, this is the parent of the
	/// Nodes being resized by the sash.
	/// </summary>
	static Node* sashDraggedParent;

	/// <summary>
	/// When dragging a sash, this contains the pre-dragged
	/// children proportions in case the user bails (presses
	/// escape) to restore the window sizes.
	/// </summary>
	static std::vector<float> sashPreDragProps;

	/// <summary>
	/// The last position left clicked. This is used during drag
	/// operations to make sure the mouse as actually moved a 
	/// certain ammount before comming to a drag/window-tear.
	/// </summary>
	static wxPoint lastLeftClick;

	static TabBar* tabBarDrag;

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

	/// <summary>
	/// The last known mouse position. Used for debugging
	/// and calculating mouse deltas.
	/// </summary>
	wxPoint lastMouse; 

	/// <summary>
	/// The current mouse state - all except Normal are mouse
	/// drag types.
	/// </summary>
	MouseState mouseState = MouseState::Normal;

	/// <summary>
	/// The sash being dragged.
	/// </summary>
	Sash* draggingSash = nullptr;

	/// <summary>
	/// The local position of a sash, where it was clicked and
	/// dragged.
	/// </summary>
	wxPoint dragOffset;
	
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

	Node* AddToLayout(HWND hwnd, Node* reference, Node::Dest refDock);
	void StealRoot(Node* n);
	bool StealToLayout(Node* n, Node* reference, Node::Dest refDock);
	Sash* SashAtPoint(const wxPoint& pt);
	Node* GetNodeTab(const wxPoint& pt);

	Node* AddRootHwnd(HWND hwnd);
	void ResizeLayout(bool refresh = true, bool rebuildSashes = true);

	void MaintainNodesTabBar(Node* pn);

	void ClearDocked();
	void ReleaseAll(bool clear = true);

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

	bool ReleaseNodeWin(Node* pn);
	bool DettachNodeWin(Node* pn);
	bool CloseNodeWin(Node* pn);
	bool CloneNodeWin(Node* pn);

	void TabClickStart(TabBar* tbInvoker, Node* node, Node* tabOwner);
	void TabClickMotion();
	void TabClickEnd();
	void TabClickCancel();

	void FinishMouseDrag();

	void OnDelegatedEscape();
	void _CancelDrag();
	void _ConfirmDrag();
	void _ReactToNodeRemoval();

public:
	
	static void SetDropPreviewWin( 
		const wxPoint& pt, 
		const wxSize& sz,
		DockWin* dwInvoker, 
		DockWin* dwDst, 
		bool raise = true);

	static void RemDropPreviewWin(bool destroy = true);

protected:
	DECLARE_EVENT_TABLE();

public:
	bool _TestValidity();
};