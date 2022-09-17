#pragma once
#include <wx/wx.h>

#include <vector>
#include <set>
#include <map>

#include "Node.h"

class Sash;

// TODO: Move into own .h/.cpp
/// <summary>
/// Description of where an application was inserted into a layout.
/// </summary>
struct DropResult
{
public: // Public data

	enum class Where
	{
		Void,
		None,
		Onto,
		Top,
		Left,
		Bottom,
		Right
	};

public: // Public members
	/// <summary>
	/// What location to drop on was detected.
	/// </summary>
	Where where;

	/// <summary>
	/// The node to operate on.
	/// </summary>
	Node* topOf;

	/// <summary>
	/// The sash representing the drop location.
	/// </summary>
	Sash* sash;

	wxPoint dropRgnPt;

	wxSize dropRgnSz;

public: // Public methods

	DropResult();
	DropResult(Where where);
	DropResult(Where where, Node* topOf, Sash* sash, const wxPoint& dropRgnPt, const wxSize& dropRgnSz);

	/// <summary>
	/// Set references to null, and the state to be invalid.
	/// </summary>
	void Invalidate();
};

/// <summary>
/// 
/// </summary>
class Layout
{
public: // Public data

	/// <summary>
	/// A region in the layout that's reserved for a Node to be placed.
	/// 
	/// This struct is used for queueing regions to process while performing
	/// a layout.
	/// </summary>
	struct Lot
	{
		Node * pn;
		int x;	// x position (left)
		int y;	// y position (top)
		int w;	// width
		int h;	// height
	};

	/// <summary>
	/// When inserting nodes, this specifies the location to add
	/// a new window node. Note that for functions that return
	/// InsertWinLoc, a window MUST be inserted to maintain a 
	/// proper datastructure. This is because the datastructure may
	/// have been modified to provide a proper location that the
	/// InsertWinLoc points to.
	/// </summary>
	struct InsertWinLoc
	{
	public:
		bool valid = true;

		/// <summary>
		/// The parent to add the window to. If the value is null,
		/// it represents setting the node to the root.
		/// </summary>
		Node * parent = nullptr;

		/// <summary>
		/// The child indext of parent to insert the node into. Ignore
		/// if parent is null.
		/// </summary>
		int childIdx = -1;

	public:
		InsertWinLoc(bool valid, Node * parent, int childIdx);
		static InsertWinLoc AtInvalid();
		static InsertWinLoc AtRoot();
		static InsertWinLoc AtNodeChild(Node* parent, int childIdx);
	};

	class ForgetUndo
	{
	public:
		enum Type
		{
			/// <summary>
			/// The ForgetUndo is recording past proportion values.
			/// </summary>
			Proportions,

			/// <summary>
			/// The ForgetUndo is recording past datastructure topology manipulation.
			/// (i.e., node reparenting)
			/// </summary>
			Stitching,

			/// <summary>
			/// Change the active index of the tab. Only used for Tab
			/// nodes.
			/// </summary>
			TabIndex,
		};

	public:
		/// <summary>
		/// The type of undo operation.
		/// </summary>
		Type ty;


		/// <summary>
		/// For propotion, the node having its children reproportioned.
		/// For Stitching, the node added as a child into prvParent.
		/// </summary>
		Node* node;

		/// <summary>
		/// When restitching, the type of parent.
		/// </summary>
		Node* prvParent;

		/// <summary>
		/// When restitching, the index the node is in the parent's
		/// children list.
		/// </summary>
		int prvIndex;

		/// <summary>
		/// Saved proportion value to restore.
		/// </summary>
		std::vector<float> childrenProps;

		/// <summary>
		/// Int value, such as tab index.
		/// </summary>
		int idx = -1;

	public:
		/// <summary>
		/// Constructor to copy the node's proportions.
		/// </summary>
		/// <param name="childrenToCpy"></param>
		static ForgetUndo MakeProportion(Node* pn);

		/// <summary>
		/// Constructor to copy the node's stitching data.
		/// </summary>
		static ForgetUndo MakeStitching(Node* pn);

		static ForgetUndo MakeTab(Node* pn);
	};

public:
	// TODO: Sort out encapsulation

	/// <summary>
	/// The root note of the entire layout hierarchy.
	/// </summary>
	Node * root;

	/// <summary>
	/// A registry of all HWNDs embedded in the layout.
	/// </summary>
	std::map<HWND, Node*> hwndLookup;

	/// <summary>
	/// The HWND of the parent DockWin
	/// 
	/// Note that this is NOT an embedded external window like 
	/// every other HWND in the app. Instead, this is the destination
	/// where all those other HWNDs would be parented to when
	/// they're embedded.
	/// </summary>
	HWND contentWin;

	/// <summary>
	/// A cache of where all the sashes in the layout are.
	/// </summary>
	std::vector<Sash*> sashes;

private: // Private methods

	/// <summary>
	/// Utility function called after adding a Node, to prepare the 
	/// embedded HWND And register that HWND with the system.
	/// </summary>
	/// <param name="hwnd">The HWND to embed into the window node.</param>
	/// <param name="pn">A Window Node that will embed the HWND.</param>
	/// <param name="embeded">
	/// If true, cache the HWNDs restore properties and modify its 
	/// visual style for how embedded windows consistently look in 
	/// the app.</param>
	void _RegisterHwndToNode(
		HWND hwnd, 
		Node* pn, 
		bool embeddedHwnd = true,
		bool cacheWinState = true);

	/// <summary>
	/// Replace a node in its parent listing with another swapped node. 
	/// If the node is the root, the swapped node will take the place
	/// of the new node.
	/// </summary>
	/// <param name="n"></param>
	/// <param name="swapWith"></param>
	/// <returns></returns>
	bool _Replace(Node* n, Node* swapWith);

public: // Public utility methods

	/// <summary>
	/// Delete a node propery from the layout datastructure.
	/// </summary>
	/// <param name="targ"></param>
	/// /// <param name="fa"></param>
	/// <param name="undo">A list of how to undo the removal.</param>
	/// <param name="updateTabVisibility">
	/// If tabs are modified, and if true, update the visibility state 
	/// of the child windows so only the active tab's window is visible.
	/// </param>
	/// <returns></returns>
	bool _ForgetWindow(
		Node* targ, 
		Node::ForgetAction fa, 
		std::vector<ForgetUndo>& undo, 
		std::set<Node*>& rmInvolved,
		bool updateTabs,
		bool updateTabVisibility);

	void UndoForget(std::vector<ForgetUndo>& undo, const LProps& props);

public: // Public methods

	/// <summary>
	/// Set the contentWin variable.
	/// </summary>
	/// <param name="cWin">The new contentWin value.</param>
	void _SetContentWin(HWND cWin);

	// TODO: Should this be public? Consider encapsulation as protected.
	// TODO: Change Node name to "location" to be more consistent
	// TODO: change all location variables to dstLocation
	// TODO: Change all "dest" to "whereAroundDst"
	/// <summary>
	/// Prepare for a node to be added to the layout.
	/// 
	/// This function does multiple things in preparation for adding a node to
	/// the layout.
	/// * Checks if the request is valid.
	/// * Creates a location for the node to be added if a new container needs to be created.
	/// 
	/// Note that if the operation is valid, the operation MUST occur in order
	/// to keep the layout datastructures correct - calling this function starts
	/// the processes of adding the node, and THERE IS NO OPTION TO CANCELL after 
	/// the call.
	/// </summary>
	/// <param name="targ">The location a node would be added to the layout.</param>
	/// <param name="dest">
	/// Where in respect to the location to place the node in the layout.
	/// </param>
	/// <returns>
	/// The results of the preparation, including whether the insertion request is 
	/// valid and is allowed to continue.
	/// </returns>
	InsertWinLoc _ScanAndPrepAddLoc(Node* targ, Node::Dest dest);

	/// <summary>
	/// Add a node to the layout.
	/// </summary>
	/// <param name="hwnd">The node to dock.</param>
	/// <param name="targ">
	/// The location to place the docked node in the layout.
	/// </param>
	/// <param name="dest">
	/// Where in respect to the location to place the node in the layout.
	/// </param>
	/// <returns>The created node.</returns>
	Node* Add(HWND hwnd, Node* targ, Node::Dest dest);

	/// <summary>
	/// Take a node that exists in another layout, and place it in the
	/// invoking object.
	/// </summary>
	/// <param name="n">The node being taken.</param>
	/// <param name="targ">
	/// The location to place the docked node in the invoking layout.
	/// </param>
	/// <param name="dest">
	/// Where in respect to the location to place the node in the layout.
	/// </param>
	/// <param name="props">
	/// The layout property that the invoking layout is using.
	/// </param>
	/// <returns></returns>
	bool Steal(Node* n, Node* targ, Node::Dest dest, const LProps& props);

	/// <summary>
	/// Integrate a node into a InsertWinLoc (an insertion
	/// site that's been prepared and expecting an insertion).
	/// </summary>
	bool Integrate(InsertWinLoc ins, Node* n);

	/// <summary>
	/// Delete a window from the layout datastructure; 
	/// identified by its HWND.
	/// </summary>
	bool DeleteWindow(HWND hwnd, std::set<Node*>* involved);

	/// <summary>
	/// Delete a window from the layout datastructure; 
	/// identified by its Node*.
	/// </summary>
	bool DeleteWindow(Node* targ, std::set<Node*>* involved);

	/// <summary>
	/// Releases a contained window from being embedded,
	/// but also deletes that window's node from the 
	/// layout datastructure. With the target being the node
	/// in the layout.
	/// </summary>
	bool ReleaseWindow(Node* targ, std::set<Node*>* involved, bool delNode = true);

	/// <summary>
	/// Releases a contained window from being embedded,
	/// but also deletes that window's node from the 
	/// layout datastructure. With the target being addressed
	/// by the HWND its containing.
	/// </summary>
	bool ReleaseWindow(HWND hwnd, std::set<Node*>* involved, bool delNode = true);

	/// <summary>
	/// Delete all sash data.
	/// Note that this does not destroy or modify the layout
	/// datastructure. It only gets rid of adjacency data.
	/// </summary>
	void ClearSashes();

	/// <summary>
	/// Deletes all sashes and rebuilds them from scratch.
	/// </summary>
	void RebuildSashes(const LProps& lp);

	/// <summary>
	/// Refreshes cached sash dimensions to match their referencing
	/// Nodes.
	/// </summary>
	void RefreshSashes(const LProps& lp);

	/// <summary>
	/// Given a point in the layout, return the overlapping node. 
	/// This will return the node with the lowest depth. The return
	/// value can be a container node if the point is over padding
	/// reserved for a sash.
	/// </summary>
	Node* GetNodeAt(const wxPoint& pt, bool stopAtTabs = false);

	/// <summary>
	/// Get a sash at a specific point.
	/// </summary>
	/// <param name="pt">The point to get the sash at.</param>
	/// <returns>
	/// The sash at the specified point, or nullptr if one was NOT found at the point.
	/// </returns>
	Sash* GetSashAt(const wxPoint& pt);

	/// <summary>
	/// Resize the layout.
	/// </summary>
	void Resize(const wxSize& newSz, const LProps& lp, bool applyPadding = true);
	/// <summary>
	/// Clear all the nodes in the layout.
	/// </summary>
	void Clear();

	/// <summary>
	/// Get a list of all contained HWNDs in the layout.
	/// </summary>
	void CollectHWNDs(std::vector<HWND>& outVec);

	/// <summary>
	/// Get a list of all window (leaf) Node items in the 
	///  layout.
	/// </summary>
	void CollectHWNDNodes(std::vector<Node*>& outVec);

	/// <summary>
	/// Given a (mouse) point, scan the layout for where a node 
	/// would be inserted.
	/// </summary>
	DropResult ScanForDrop(const wxPoint& pt, LProps& lp);

public: // Public static methods

	/// <summary>
	/// Resize a specific node, as well as its hierarchy.
	/// </summary>
	static void Resize(Node* pn, const wxPoint& newPt, const wxSize& newSz, const LProps& lp);

	/// <summary>
	/// Resize a node using its current cached size. Note 
	/// this will not modify the parameter's proportion but
	/// will recursively modify the children's proportions.
	/// </summary>
	/// <param name="pn">The node to resize.</param>
	/// <param name="lp">The layout properties.</param>
	static void Resize(Node* pn, const LProps& lp);

	/// <summary>
	/// Resize a Layout Lot, and then recursively resize its 
	/// hierarchy.
	/// </summary>
	static void _ResizeFromLot(const Lot& lroot, const LProps& lp);

public:
	int _CountInstancedTabsBarsInHierarchy();
	int _CountNodesInHierarchy();
	bool _TestValidity();
};

/// <summary>
/// Check if a point is within a rectangle, with the rectangle
/// defined by a point and size.
/// </summary>
/// <param name="r_pos">The origin (top left) of the rectangle.</param>
/// <param name="r_sz">The size of the rectangle.</param>
/// <param name="pt">The point to query overlap with the rectangle.</param>
/// <returns>If true, the point is in the rectangle.</returns>
bool InBounds(wxPoint r_pos, wxSize r_sz, wxPoint pt);

/// <summary>
/// Check if a point is within a rectangle.
/// </summary>
/// <param name="r">The rectangle.</param>
/// <param name="pt">The point.</param>
/// <returns>If true, the point is in the rectangle.</returns>
bool InBounds(wxRect r, wxPoint pt);
