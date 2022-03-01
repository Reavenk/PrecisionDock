#pragma once

#include <wx/wx.h>

class LProps;
class TabBar;

/// <summary>
/// A container to hold a window in the layout.
/// </summary>
class Node
{
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
	/// 
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
	TabBar* tabsBar = nullptr;

public:

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

	int selTab = 0;

	std::string tabName;

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

	inline TabBar* GetTabBar()
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

	void ResizeChildrenByProportions(std::vector<float> vecp, const LProps& lp);

	bool HideWindow();

	bool ShowWindow(bool show = true);

	int Depth() const;

	bool SelectTab(int idx);

	bool SelectTab(Node* winChild);

	void ClearTabBar();

	void SetTabBar(TabBar* tb);

	void ForgetTabBar();
};