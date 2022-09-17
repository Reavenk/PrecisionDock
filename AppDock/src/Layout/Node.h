#pragma once

#include <wx/wx.h>

class LProps;
class TabsBar;

/// <summary>
/// A container to hold a window in the layout.
/// </summary>
class Node
{
	/// <summary>
	/// The counter to generate Node::id values.
	/// </summary>
	static int IDCT;
public:

	enum class ForgetAction
	{
		Void,
		Forget,
		Hide,
		Delete
	};

public:

	/// <summary>
	/// The types of Node.
	/// </summary>
	enum class Type
	{
		/// <summary>
		/// The node contains an embedded HWND window.
		/// </summary>
		Window,

		/// <summary>
		/// The node contains a horizontal array of nodes 
		/// within this->children.
		/// </summary>
		Horizontal,

		/// <summary>
		/// The node contains a vertical array of nodes
		/// within this->children.
		/// </summary>
		Vertical,

		/// <summary>
		/// The node contains a set of nodes in a tab region
		/// within this->children.
		/// </summary>
		Tabs
	};


	// Very similar to DropResult::Where. We may merge these together 
	// later, but for now we'll keep them separate.
	enum class Dest
	{
		Invalid = -1,
		Above,
		Below,
		Into,
		Left,
		Right
	};

private:
	/// <summary>
	/// If the node is a notebooked container, then this will be the GUI
	/// that has the notebook tabs.
	/// 
	/// Else, this will be nullptr.
	/// </summary>
	TabsBar* tabsBar = nullptr;

public:

	/// <summary>
	/// The ID of the node.
	/// 
	/// This value will be unique as an incremented counter value (see 
	/// usage of Node::IDCT for details), which can be used for debugging, as
	/// the same steps will deterministically reproduce the same id values.
	/// </summary>
	int id;

	/// <summary>
	/// The type of content the node is containing.
	/// </summary>
	Type type;

	/// <summary>
	/// Parent node.
	/// </summary>
	Node * parent;

	/// <summary>
	/// The children nodes contains within this node.
	/// </summary>
	std::vector<Node*> children;

	/// <summary>
	/// The position of node in the layout from the last 
	/// resize calculation.
	/// </summary>
	wxPoint cachePos;

	/// <summary>
	/// The size of the node in the layout from the last 
	/// resize calculation.
	/// </summary>
	wxSize cacheSize;

	/// <summary>
	/// The command line used to invoke the process. Only
	/// used for Window nodes.
	/// </summary>
	std::wstring cmdLine;

	inline int CacheRight() const
	{ return this->cachePos.x + this->cacheSize.x;}

	inline int CacheBot() const
	{ return this->cachePos.y + this->cacheSize.y;}

	/// <summary>
	/// The tab region of the node in the layout from the last
	/// resize calculation.
	/// </summary>
	wxRect cachedTab;

	/// <summary>
	/// tab region of the node in the layout from the last
	/// resize calculation; local to the cached region.
	/// </summary>
	wxRect cachedTabLcl;

	/// <summary>
	/// The client region of the node in the layout from the last
	/// resize calculation.
	/// </summary>
	wxRect cachedClient;

	//wxSize cachedMinSize;

	/// <summary>
	/// The percentage of the (available) space the child takes up
	/// of its parent space.
	/// </summary>
	float proportion = 0.0f;

	/// <summary>
	/// The original size of the docked window. This is stored so
	/// if the window is undocked, it can be re-given its previous
	/// size.
	/// </summary>
	wxSize origSize;

	/// <summary>
	/// The original style of the contained HWND before it's 
	/// contained in the node. It's stored so it can be restored 
	/// if it's released from the HWND.
	/// </summary>
	LONG origStyle = 0;

	/// <summary>
	/// The original styleEx of the contained HWND before it's
	/// contained in the node. It's stored so it can be restored
	/// if it's released from the HWND.
	/// </summary>
	LONG origStyleEx = 0;

	/// <summary>
	/// Handle to the window being embedded.
	/// 
	/// Note that (for now) Window nodes are allowed to have empty
	/// HWND values. While there is no practical reason for this, it
	/// makes it easier to set up situations to develop and test if
	/// we're not forced to constantly find/create HWNDs to test a
	/// situation.
	/// </summary>
	HWND win = NULL;

	/// <summary>
	/// If the node is a notebook tab, what tab is currently active?
	/// 
	/// This represents an index into this->children.
	/// </summary>
	int selectedTabIdx = 0;


	// Currently UNUSED: This is a custom label that can be assigned
	// to a window node's tab.
	std::string customTabName;

	inline HWND Hwnd() const
	{ return this->win; }

public:
	Node();
	~Node();

	inline bool IsTabChild() const
	{ 
		return 
			this->parent != nullptr && 
			this->parent->type == Node::Type::Tabs;
	}

	inline TabsBar* GetTabsBar()
	{
		return this->tabsBar;
	}

	/// <summary>
	/// Check if the node contains point within its alloted region.
	/// </summary>
	/// <param name="pt">The point to query.</param>
	/// <returns>True if point is within region. Else, false.</returns>
	bool ContainsPoint(const wxPoint& pt);

	/// <summary>
	/// Delete all Node children.
	/// </summary>
	/// <param name="recurse">
	/// If true, the children nodes will also delete their children, 
	/// and so on.
	/// </param>
	void DeleteChildren(bool recurse = true);

	/// <summary>
	/// Recalculate the proportion of the Node's children 
	/// so they keep the same proportions with each other, 
	/// but sum up to 1.0.
	/// </summary>
	void NormalizeChildren();

	/// <summary>
	/// Destroy the window contained in the node.
	/// </summary>
	/// <returns>
	/// True if there was a window and successfully destroyed,
	/// else false.
	/// </returns>
	bool DestroyHWND();

	/// <summary>
	/// Remove knowledge of the HWND.
	/// </summary>
	/// <returns>True, if successful.</returns>
	bool ForgetHWND();

	bool ForgetHWND(ForgetAction fa);

	/// <summary>
	/// Moved the HWND back to the desktop.
	/// </summary>
	/// <param name="forget">
	/// If true, also forget the reference to the HWND.
	/// </param>
	/// <returns>True, if successful.</returns>
	bool ReleaseHWND(bool forget = true);

	/// <summary>
	/// Calculate UI elements such as that tab and client
	/// region, from the node's outer cached info.
	/// </summary>
	/// <param name="lp">The layout properties.</param>
	void CacheFromProps(const LProps& lp);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="lp">The layout properties.</param>
	/// <returns></returns>
	wxSize CalculateMinSize(const LProps& lp);

	// Recalculate child proportions by normalizing the
	// children's current proportions.
	void CalculateChildProportions_Prop();

	/// <summary>
	/// Recalculate child proportions by comparing the
	/// cached sizes of the children.
	/// </summary>
	void CalculateChildProportions_Cache();

	/// <summary>
	/// Resize the children, using the cached space, 
	/// distributing the space by proportion.
	/// </summary>
	/// <param name="lp">The layout properties.</param>
	void ResizeChildrenByProportions(const LProps& lp);

	/// <summary>
	/// Resize the children, used specific proportion values.
	/// </summary>
	/// <param name="vecp">
	/// The proportions to used for the layout. This will map to the Node's
	/// children, meaning the size of vecp should be the same size as 
	/// this->children.
	/// </param>
	/// <param name="lp">The layout properties.</param>
	void ResizeChildrenByProportions(std::vector<float> vecp, const LProps& lp);

	/// <summary>
	/// Hide the window.
	/// </summary>
	/// <returns>True, if the window was successfully hidden.</returns>
	bool HideWindow();

	/// <summary>
	/// Show the window.
	/// </summary>
	/// <param name="show">
	/// Whether to show or hide the window.
	/// </param>
	/// <returns>
	/// True, if the Window's visibility state matches the specified state when the
	/// function exits.
	/// </returns>
	bool ShowWindow(bool show = true);

	/// <summary>
	/// The depth from the root of the layout.
	/// 
	/// This function may be unused, but is kept as a diagnostic utility.
	/// </summary>
	/// <returns></returns>
	int Depth() const;

	/// <summary>
	/// Check if a tab index is selected.
	/// 
	/// Only valid for tab collection nodes.
	/// </summary>
	/// <param name="idx">The tab to check</param>
	/// <param name="updateWinVisibility">
	/// If true, update the windows visibility. Sometimes we want to avoid doing this
	/// because it can force a redraw or loss of mouse focus.
	/// </param>
	/// <returns>True if the specifed index is selected.</returns>
	bool SelectTab(int idx, bool updateWinVisibility = true);

	void UpdateTabWindowVisibility();

	/// <summary>
	/// Check if a node is the selected tab.
	/// 
	/// Only valid for tab collection nodes.
	/// </summary>
	/// <param name="winChild">
	/// The node to check against.
	/// The node is expected to be a child of the invoking node, and a Window node.
	/// </param>
	/// <returns>True if the specified node is selected.</returns>
	bool SelectTab(Node* winChild);

	/// <summary>
	/// Destroy the windows bar tab.
	/// </summary>
	void ClearTabsBar();

	/// <summary>
	/// Set the value of the tab bar. 
	/// 
	/// This is expected to be a tab bar that was previously forgotten.
	/// </summary>
	/// <param name="tb">The tab bar to set.</param>
	void SetTabsBar(TabsBar* tb);

	Node* ChildOtherThan(Node* n);

	/// <summary>
	/// Get rid knowledge of the Windows bar tab.
	/// </summary>
	void ForgetTabsBar();

	void ResetTabsBarLayout(const LProps& lp);
};