#pragma once
#include <wx/wx.h>
#include <set>
#include "Layout/Layout.h"
class DockWin;
class TopDockWin;
class DragPreviewOlyWin;
class Node;
class BarDrop;

/// <summary>
/// A helper class to manage the drag and drop mechanics,
/// validity and state-keeping of DockWins.
/// 
/// This mainly handles two parts, the dragging of sashes,
/// and the dragging of tabs.
/// </summary>
class DragHelperMgr
{
public:
	enum class DragType
	{
		Invalid,
		Sash,
		Tab
	};

public:

	DragType dragType = DragType::Invalid;

	bool alreadyToreOffTab = false;

	bool dragFlaggedAsFinished = false;

	wxWindow* winWithMouseCapture = nullptr;

	// It's assumed that only one drag operation can occur at a time. This
	// may not be 100% correct if we introduce the idea of multitouch. For
	// now we don't consider issues like that. 

	/// <summary>
	/// When dragging a node to drop somewhere else, this is
	/// the graphical element that follow the cursor to give
	/// visual feedback of being in the drag mode.
	/// </summary>
	DragPreviewOlyWin* draggingCursorGraphic = nullptr;

	/// <summary>
	/// When dragging a node to drop somewhere else, this is
	/// the preview of where the window will be dropped.
	/// </summary>
	BarDrop* dropPreviewWin = nullptr;

	/// <summary>
	/// When dragging a node to another window, this is the 
	/// current node the cursor on top of.
	/// </summary>
	TopDockWin* winDraggedOnto = nullptr;

	/// <summary>
	/// When dragging a node, this is current location to drop
	/// the node into.
	/// </summary>
	DropResult tabDropDst;

	/// <summary>
	/// When ripping a tab out, the Node is removed, but
	/// kept around in memory until the drag is finished
	/// in case the user cancels. If the user cancels, the
	/// info in dragUndo contains instructions on how to
	/// reinsert the node(s) and restore them.
	/// </summary>
	std::vector<Layout::ForgetUndo> dragUndo;

	/// <summary>
	/// The node that is being dragged and dropped.
	/// </summary>
	Node* nodeDragged = nullptr;

	/// <summary>
	/// The nodes that were removed in a drag-node operation.
	/// 
	/// If a drag is not undone and commited to (i.e., the user
	/// drops a node somewhere instead of pressing escape to bail
	/// from the drag node operation) any nodes that were removed
	/// and no longer relevant are deleted.
	/// </summary>
	std::set<Node*> dragNodesInvolved;

	/// <summary>
	/// When dragging a sash, this is the parent of the
	/// Nodes being resized by the sash.
	/// </summary>
	Node* sashDraggedParent = nullptr;

	/// <summary>
	/// When dragging a sash, this contains the pre-dragged
	/// children proportions in case the user bails (presses
	/// escape) to restore the window sizes.
	/// </summary>
	std::vector<float> preDragSashProps;

	/// <summary>
	/// When dragging a window from a TabsBar, what was that TabsBar?
	/// </summary>
	TabsBar* tabsBarDrag = nullptr;

	/// <summary>
	/// The node that originally owned TabsBar before the drag operation.
	/// </summary>
	Node* tabDragOwner = nullptr;

	// The node who's window should be shown, or whos tab order should
	// be reprocessed after a drag operation
	Node* toUpdateAfterDrag = nullptr;

	/// <summary>
	/// The local position of a sash, where it was clicked and
	/// dragged.
	/// </summary>
	wxPoint dragOffset;

	/// <summary>
	/// The last position left clicked. This is used during drag
	/// operations to make sure the mouse as actually moved a 
	/// certain ammount before comming to a drag/window-tear.
	/// </summary>
	wxPoint lastLeftClick;

	/// <summary>
	/// The sash being dragged.
	/// </summary>
	Sash* draggingSash = nullptr;

	wxPoint lastKnownGlobalMouse;

	/// <summary>
	/// The DockWin that has the mouse capture during a tab drag.
	/// </summary>
	DockWin* winWhereDragged;

	/// <summary>
	/// If a dragging tab, this is if it was initiated by clicking
	/// on the close button.
	/// </summary>
	bool clickedClose = false;

public:
	DragHelperMgr(DockWin* winDragged, Sash* sashDragged, const wxPoint& winMousePos);
	DragHelperMgr(DockWin* winDragged, TabsBar* tbInvoker, bool clickedClose, Node* node, Node* tabOwner);
	~DragHelperMgr();

	/// <summary>
	/// Sets the data of dropPreviewWin to specific values.
	/// </summary>
	/// <param name="pt">Sets the (top left) position of the window.</param>
	/// <param name="sz">Set the size of the window.</param>
	/// <param name="dwInvoker">The DockWin that called SetDropPreiewWin.</param>
	/// <param name="dwDst">The DockWin that is the target of the call.</param>
	/// <param name="raise">If true, raises the window to the top of the Z-order.</param>
	void SetDropPreviewWin( 
		const wxPoint& pt, 
		const wxSize& sz,
		DockWin* dwInvoker, 
		DockWin* dwDst, 
		bool raise = true);

	bool RemoveDropPreviewWin();
	bool RemoveDraggingCursor();

	/// <summary>
	/// Run assert checks to see if all expectations for dragging a sash are correct.
	/// </summary>
	/// <param name="shouldHaveCapture">
	/// True if the DockWin is expected to have the mouse capture.
	/// </param>
	void _AssertIsDraggingSashCorrectly(bool shouldHaveCapture);

	/// <summary>
	/// Run assert checks to see if all expectations for dragging a tab are correct.
	/// </summary>
	/// <param name="shouldHaveCapture">
	/// True if the DockWin is expected to have the mouse capture.
	/// </param>
	void _AssertIsDraggingTabCorrectly(bool shouldHaveCapture);

	/// <summary>
	/// Run assert checks to see that all expected variables are null or empty. This
	/// is because our codebase expects these things to be manually emptied to ensure
	/// proper things are left untouched, or have explicitly been touched (and then
	/// emptied).
	/// </summary>
	void _AssertIsNeutralized();

	void _ResolveToUpdateAfterDrag();

	void FinishSuccessfulTabDragging();
	void CancelTabDragging(bool fromCaptureLoss);

	void FinishSuccessfulSashDragging();
	void CancelSashDragging(bool fromCaptureLoss);

	void HandleMouseMove();
	void _HandleMouseMoveSash(const wxPoint& delta);
	void _HandleMouseMoveTab(const wxPoint& delta);

	void ResumeCapture(wxWindow* requester);

	/// <summary>
	/// Called when the mouse capture should be released.
	/// 
	/// Even if the mouse capture was taken, this function should be called to
	/// end the drag context.
	/// </summary>
	/// <param name="requester">
	/// The window requesting the mouse capture release.
	/// This should be the same window that has the mouse capture. This is simple
	/// a safeguard check (for asserts) and is non-functional.
	/// </param>
	/// <param name="fromCaptureLoss">
	/// Set to true if StopCapture was called after the mouse capture was lost from
	/// an OS event that forced us to loose mouse capture. Else, set to false.
	/// </param>
	void StopCapture(wxWindow* requester, bool fromCaptureLoss);

	void SetDragPreviewOlyWin(const wxPoint& whereAt);
};

typedef std::shared_ptr<DragHelperMgr> DragHelperMgrPtr;