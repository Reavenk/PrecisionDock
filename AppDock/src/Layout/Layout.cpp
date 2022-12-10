#include "Layout.h"
#include "LProps.h"
#include "Sash.h"
#include "TabsBar.h"

#include <queue>

DropResult::DropResult()
{
	this->where = DropResult::Where::Void;
	this->topOf = nullptr;
	this->sash	= nullptr;
}

DropResult::DropResult(Where where)
{
	this->where = where;
	this->topOf = nullptr;
	this->sash	= nullptr;
}

DropResult::DropResult(Where where, Node* topOf, Sash* sash, const wxPoint& dropRgnPt, const wxSize& dropRgnSz)
{
	this->where = where;
	this->topOf = topOf;
	this->sash	= sash;

	this->dropRgnPt = dropRgnPt;
	this->dropRgnSz = dropRgnSz;
}

void DropResult::Invalidate()
{
	this->sash	= nullptr;
	this->topOf = nullptr;
	this->where = DropResult::Where::None;
}

Node::Dest DropResult::WhereToNodeDest() const
{
	switch(this->where)
	{
	case DropResult::Where::Left:
		return Node::Dest::Left;
		break;
	case DropResult::Where::Right:
		return Node::Dest::Right;
		break;
	case DropResult::Where::Top:
		return Node::Dest::Above;
		break;
	case DropResult::Where::Bottom:
		return Node::Dest::Below;
		break;
	case DropResult::Where::Onto:
		return  Node::Dest::Into;
	}
	return Node::Dest::Invalid;
}


Layout::ForgetUndo Layout::ForgetUndo::MakeProportion(Node* pn)
{
	ForgetUndo ret;
	ret.ty = ForgetUndo::Type::Proportions;
	ret.node = pn;
	ret.prvParent = nullptr;
	ret.prvIndex = -1;

	for(Node* child : pn->children)
		ret.childrenProps.push_back(child->proportion);

	return ret;
}

Layout::ForgetUndo Layout::ForgetUndo::MakeStitching(Node* pn)
{
	Node* parent = pn->parent;

	ForgetUndo ret;
	ret.ty = ForgetUndo::Type::Stitching;
	ret.node = pn;
	ret.prvParent = parent;

	if(parent != nullptr)
	{
		ret.prvIndex = -1;
		for(size_t i = 0; i < parent->children.size(); ++i)
		{
			if(parent->children[i] == pn)
			{ 
				ret.prvIndex = i;
				break;
			}
		}
	}
	return ret;
}

Layout::ForgetUndo Layout::ForgetUndo::MakeTab(Node* pn)
{
	assert(pn->type == Node::Type::Tabs);

	ForgetUndo ret;
	ret.ty		= ForgetUndo::Type::TabIndex;
	ret.node	= pn;
	ret.idx		= pn->selectedTabIdx;
	return ret;
}

Layout::InsertWinLoc::InsertWinLoc(bool valid, Node * parent, int childIdx)
{
	this->valid = valid;
	this->parent = parent;
	this->childIdx = childIdx;
}

Layout::InsertWinLoc Layout::InsertWinLoc::AtInvalid()
{
	return Layout::InsertWinLoc(false, nullptr, -1);
}

Layout::InsertWinLoc Layout::InsertWinLoc::AtRoot()
{
	return Layout::InsertWinLoc(true, nullptr, -1);
}

Layout::InsertWinLoc Layout::InsertWinLoc::AtNodeChild(Node* parent, int childIdx)
{
	return Layout::InsertWinLoc(true, parent, childIdx);
}

void Layout::_RegisterHwndToNode(
	HWND hwnd, 
	Node* pn, 
	bool embeddedHwnd, 
	bool cacheWinState)
{
	if(hwnd == nullptr)
		return;

	pn->win = hwnd;
	if(cacheWinState == true)
	{ 
		RECT r;
		GetWindowRect(hwnd, &r);
		pn->origSize.x = r.right - r.left;
		pn->origSize.y = r.bottom - r.top;
		//
		pn->origStyle = GetWindowLong(hwnd, GWL_STYLE);
		pn->origStyleEx = GetWindowLongA(hwnd, GWL_EXSTYLE);
	}

	assert(this->hwndLookup.find(hwnd) == this->hwndLookup.end());
	this->hwndLookup[hwnd] = pn;

	if(this->contentWin != NULL && embeddedHwnd == true)
	{
		SetParent(hwnd, this->contentWin);
		SendMessage(hwnd, WM_CHANGEUISTATE ,UIS_CLEAR,  UISF_ACTIVE);
		
		LONG newStyle = pn->origStyle & ~(WS_BORDER|WS_DLGFRAME|WS_TILEDWINDOW| WS_SIZEBOX|WS_POPUP|WS_DLGFRAME);
		SetWindowLongA(hwnd, GWL_STYLE, newStyle);

		ShowWindow(hwnd, SW_NORMAL);
		//SetWindowPos(hwnd, NULL,)
	}
}

bool Layout::_Replace(Node* n, Node* swapWith)
{
	if(n->parent == nullptr)
	{
		assert(this->root == n);
		swapWith->parent		= nullptr;
		swapWith->proportion	= 1.0f;
		this->root				= swapWith;
		return true;
	}

	std::vector<Node*>& parChilds = n->parent->children;
	for(size_t i = 0; i < parChilds.size(); ++i)
	{
		if(parChilds[i] == n)
		{ 
			parChilds[i]		= swapWith;
			swapWith->parent	= n->parent;
			swapWith->proportion = n->proportion;
			return true;
		}
	}
	return false;
}

bool Layout::_ForgetWindow(
	Node* targ, 
	Node::ForgetAction fa, 
	std::vector<ForgetUndo>& undo,
	std::set<Node*>& rmInvolved,
	bool updateTabs,
	bool updateTabVisibility)
{
	if(targ->type != Node::Type::Window)
		return false;

	targ->ClearTabsBar();

	// Unregister the HWND (if any)
	if(targ->win != NULL)
	{
		auto it = this->hwndLookup.find(targ->win);

		if(it == this->hwndLookup.end())
			return false;

		this->hwndLookup.erase(targ->win);
	}

	if(this->root == targ)
	{
		undo.push_back(ForgetUndo::MakeStitching(targ));
		rmInvolved.insert(targ);
		targ->ForgetHWND(fa);
		this->root = nullptr;
		return true;
	}

	assert(targ != nullptr);			// Sanity check
	assert(targ->parent != nullptr);	// Should have been from the root processing

	Node* oldParent = targ->parent;

	auto it = std::find(
		oldParent->children.begin(), 
		oldParent->children.end(),
		targ);

	bool changedTab = false;
	int newTabIdx = -1;
	Node* tabParentToUpdate = nullptr;
	if(updateTabs == true)
	{
		if(oldParent->type == Node::Type::Tabs)
		{
			// The old index, in case the parent was a tab and we need to adjust.
			int oldIdx = it - oldParent->children.begin();
			tabParentToUpdate = oldParent;

			if(oldIdx <= oldParent->selectedTabIdx)
			{
				// Reset to a new tab, and update the selected tab
				// to something valid.
				newTabIdx = std::max(0, oldParent->selectedTabIdx - 1);
				// We can't update the tab window just yet until the removal is done,
				// so we just flag it for removal.
				// The reason this is done so high up in the function though, is because 
				// we need to record the Undo before the removal.
				
				undo.push_back(ForgetUndo::MakeTab(oldParent));
				changedTab = true;
			}
		}
	}
	// Save undo information
	undo.push_back(ForgetUndo::MakeProportion(targ->parent));
	undo.push_back(ForgetUndo::MakeStitching(targ));
	rmInvolved.insert(targ);

	assert(it != oldParent->children.end());

	oldParent->children.erase(it);
	targ->ForgetHWND(fa);
	// Avoid hitting some asserts later down the line.
	targ->parent = nullptr; 

	if(oldParent->type == Node::Type::Tabs &&  updateTabVisibility == true)
		oldParent->SelectTab(oldParent->selectedTabIdx, !updateTabs);


	// Check if we need to remove parents.
	//
	// This happens if parent's don't have enough children to
	// justify their existence.
	while(true)
	{
		// If the container ends up being a 
		// - horizontal in a horizontal parent, or
		// - a vertical in a vertical parent,
		// Then we need to just dump the lower-depth container's
		// contents in the parent where it was in the parent.
		if(oldParent->parent != nullptr && (oldParent->type == oldParent->parent->type))
		{
			// If we have a window in a window, or a tabset in a tabset, something
			// elsewhere has corrupted the state.
			assert(oldParent->type != Node::Type::Window);
			assert(oldParent->type != Node::Type::Tabs);

			Node* toRem = oldParent;
			Node* remPar = oldParent; // The same value as oldParent, but a more relevant name.
			oldParent = oldParent->parent;

			undo.push_back(ForgetUndo::MakeProportion(remPar));
			undo.push_back(ForgetUndo::MakeStitching(toRem));

			toRem->ClearTabsBar();

			size_t swapidx = -1;
			for(size_t i = 0; i < toRem->children.size(); ++i)
			{
				if(remPar->children[i] == toRem)
				{
					swapidx = i;
					break;
				}
			}
			remPar->children.erase(remPar->children.begin() + swapidx);
			toRem->parent = nullptr;
			rmInvolved.insert(toRem);

			while(toRem->children.empty() == false)
			{
				// Remove from parent, from last to first, but don't change the insertion
				// point, which will end up maintaining the order.
				Node* nMvChild = toRem->children.back();
				undo.push_back(ForgetUndo::MakeStitching(nMvChild));
				toRem->children.pop_back();
				nMvChild->parent = remPar;
				remPar->children.insert(remPar->children.begin() + swapidx, nMvChild);
			}

			assert(swapidx != -1);
		}
		// It the container has 2 or more children, it serves
		// a purpose and can keep existing.
		else if(oldParent->children.size() < 2)
		{ 
			// Or else it needs to be collapsed.
		
			bool isroot = (oldParent == this->root);

			// Save undo information
			if(isroot == false)
				undo.push_back(ForgetUndo::MakeProportion(oldParent->parent));

			undo.push_back(ForgetUndo::MakeStitching(oldParent));
			rmInvolved.insert(oldParent);
			// Make a reparent stiching, but it's not deleted (so it doesn't get
			// put into the removed involved nodes set) because it takes the place
			// of its former parent.
			undo.push_back(ForgetUndo::MakeStitching(oldParent->children[0]));

			Node * toRem = oldParent;
			oldParent = toRem->parent;

			toRem->ClearTabsBar();

			this->_Replace(toRem, toRem->children[0]);

			toRem->children.clear();

			if(isroot == true)
				break;
		}
		else
			break;
	}

	// If we had a tab container, it could have been collapsed if it only 
	// had 1 child left afterwards, In which case it's no longer going to 
	// be on the current layout.
	if(changedTab && tabParentToUpdate->children.size() >= 2)
	{
		assert(tabParentToUpdate != nullptr);
		assert(tabParentToUpdate->type == Node::Type::Tabs);
		assert(newTabIdx != -1);

		tabParentToUpdate->SelectTab(newTabIdx, false);
	}


	return true;
}

void Layout::UndoForget(std::vector<ForgetUndo>& undo, const LProps& props)
{
	if(undo.empty() == true)
		return;

	for(size_t i = undo.size() - 1; ;)
	{
		ForgetUndo & forgu = undo[i];

		if(forgu.ty == ForgetUndo::Type::Proportions)
		{
			//forgu.childrenProps
			const size_t childCt = forgu.node->children.size();
			assert(childCt == forgu.childrenProps.size());

			forgu.node->ResizeChildrenByProportions(forgu.childrenProps, props);
		}
		else if(forgu.ty == ForgetUndo::Type::Stitching)
		{
			assert(forgu.node != nullptr);

			if(forgu.node->parent != nullptr)
			{
				// If the stitching involves a node that wasn't removed, but 
				// moved to a different location on the layout - then remove
				// it from its current location before readding it to where 
				// it was before.

				Node* par = forgu.node->parent;
				auto it = 
					std::find(
						par->children.begin(),
						par->children.end(),
						forgu.node);

				
				if(it != par->children.end())
				{ 
					par->children.erase(it);
					forgu.node->parent = nullptr;
				}
			}

			if(forgu.prvParent == nullptr)
			{
				assert(this->root == nullptr);
				this->root = forgu.node;
				this->root->parent = nullptr;
			}
			else
			{
				assert(forgu.prvParent != nullptr);

				forgu.prvParent->children.insert( 
					forgu.prvParent->children.begin() + forgu.prvIndex,
					forgu.node);

				forgu.node->parent = forgu.prvParent;
				if(forgu.prvParent->type == Node::Type::Tabs)
					forgu.node->ClearTabsBar();

				// The root should be repaired by the end of processing
				// all the undos. But we set the root to null if needed
				// so certain assertions can pass.
				if(forgu.node == this->root)
					this->root = nullptr;
			}
			HWND hwnd = forgu.node->Hwnd();
			if(hwnd != NULL)
				this->hwndLookup[hwnd] = forgu.node;

			// Don't show window anywhere inside of UndoForget(). In practice,
			// only 1 window will be re-shown from an undo chain, and that can
			// be done afterwards, outside this function. If we do a ShowWindow(),
			// it will steal focus and cause a mouse release event to instantly
			// trigger.
			//
			//forgu.node->ShowWindow();
		}
		else if(forgu.ty == ForgetUndo::Type::TabIndex)
		{
			assert(forgu.node != nullptr);
			assert(forgu.node->type == Node::Type::Tabs);
			assert(forgu.node->children.size() >= 2);
			assert(forgu.idx >= 0);
			assert(forgu.idx < forgu.node->children.size());

			forgu.node->SelectTab(forgu.idx, true);
		}
		else
			assert(!"Unhandled ForgetUndo::Type");

		if(i == 0)
			return;

		--i;
	}
}

void Layout::_SetContentWin(HWND cWin)
{
	this->contentWin = cWin;
}

Layout::InsertWinLoc Layout::_ScanAndPrepAddLoc(Node* targ, Node::Dest whereAroundTarg)
{
	if(this->root == nullptr)
	{
		if(targ != nullptr)
			return InsertWinLoc::AtInvalid();

		return InsertWinLoc::AtRoot();
		//Node * pnNew = new Node();
		//pnNew->proportion = 1.0f;
		//this->root = pnNew;
		//
		//this->_RegisterHwndToNode(hwnd, pnNew);
		//return pnNew;
	}

	if(targ == nullptr)
		InsertWinLoc::AtInvalid();

	if(targ->type == Node::Type::Horizontal ||
		targ->type == Node::Type::Vertical)
	{
		// If it's at the far beginning of a grained container.
		if(whereAroundTarg == Node::Dest::Left || whereAroundTarg == Node::Dest::Above)
		{
			//Node* pnAdd = new Node();
			//targ->children.insert(targ->children.begin(), pnAdd);
			//pnAdd->parent = targ;
			//pnAdd->type = Node::Type::Window;
			//pnAdd->win = hwnd;
			//
			//this->_RegisterHwndToNode(hwnd, pnAdd);
			//
			//return pnAdd;
			return InsertWinLoc::AtNodeChild(targ, 0);
		}

		// If it's at the far end of a grained container.
		if(whereAroundTarg == Node::Dest::Right || whereAroundTarg == Node::Dest::Below) 
		{
			//Node* pnAdd = new Node();
			//targ->children.push_back(pnAdd);
			//pnAdd->parent = targ;
			//pnAdd->type = Node::Type::Window;
			//pnAdd->win = hwnd;
			//
			//this->_RegisterHwndToNode(hwnd, pnAdd);
			//
			//return pnAdd;
			return InsertWinLoc::AtNodeChild(targ, targ->children.size());
		}
	}

	// The UI may detect a drop into the inner tabbed 
	// window instead of the tab container.
	if(targ->parent != nullptr && targ->parent->type == Node::Type::Tabs)
		targ = targ->parent;

	if(whereAroundTarg == Node::Dest::Into)
	{
		assert(
			targ->type == Node::Type::Window || 
			targ->type == Node::Type::Tabs);

		if(targ->type == Node::Type::Tabs)
		{ 
			return InsertWinLoc::AtNodeChild(
				targ, targ->children.size()); 
		}
		else // if(targ->type == Node::Type::Window)
		{

			Node* nTab			= new Node();
			nTab->type			= Node::Type::Tabs;
			targ->ClearTabsBar();
			this->_Replace(targ, nTab);
			nTab->children.push_back(targ);
			targ->proportion	= 1.0f;
			targ->parent		= nTab;
			return InsertWinLoc::AtNodeChild(nTab, 0); 
		}
	}

	// Was it placed onto a window that now needs to
	// turn into a grain?
	if(
		targ->type == Node::Type::Window || 
		targ->type == Node::Type::Tabs)
	{
		// Are we creating a split at the root?
		if(targ->parent == nullptr)
		{ 
			assert(this->root == targ);

			if(
				whereAroundTarg == Node::Dest::Above	||
				whereAroundTarg == Node::Dest::Below	||
				whereAroundTarg == Node::Dest::Left	|| 
				whereAroundTarg == Node::Dest::Right)
			{
				// Figure out the grain.
				Node::Type grain = Node::Type::Vertical;
				if(whereAroundTarg == Node::Dest::Left || whereAroundTarg == Node::Dest::Right)
					grain = Node::Type::Horizontal;

				// Figure out who gets what index in the split container
				bool insAsFirst = 
					(whereAroundTarg == Node::Dest::Above) || 
					(whereAroundTarg == Node::Dest::Left);

				// Create new grained container
				Node* pnSplit = new Node();
				pnSplit->type = grain;
				this->root = pnSplit;

				// Initialize and manage references.
				targ->parent	= pnSplit;

				// Insert at the first index?
				pnSplit->children.push_back(targ);
				if(insAsFirst) 
					return InsertWinLoc::AtNodeChild(pnSplit, 0);
				else
					return InsertWinLoc::AtNodeChild(pnSplit, 1);
			}
		}
		// Splitting at anything else that's NOT the root.
		else
		{
			Node::Type parentTy = targ->parent->type;

			// Figure out the grain.
			Node::Type grain = Node::Type::Vertical;
			if(whereAroundTarg == Node::Dest::Left || whereAroundTarg == Node::Dest::Right)
				grain = Node::Type::Horizontal;

			if(parentTy == grain)
			{
				// If we're adding to a window that's a part of a container with the
				// same grain (i.e., we're adding above/below a vertical system, or 
				// left/right of a horizontal system) we can just add the new node
				// directly as part of the system.

				for(size_t i = 0; i < targ->parent->children.size(); ++i)
				{
					Node* nIt = targ->parent->children[i];
					if(nIt == targ)
					{
						bool insAsFirst = 
							(whereAroundTarg == Node::Dest::Above) || 
							(whereAroundTarg == Node::Dest::Left);

						if(insAsFirst)
							return InsertWinLoc::AtNodeChild(targ->parent, 0);
						else
							return InsertWinLoc::AtNodeChild(targ->parent, 1);
					}
				}
				assert(!"Should not have reached here");
			}
			else
			{
				// If we're adding to a window that's part of a container with a
				// different grain than how we're adding (i.e., adding vertically to
				// a horizontal system, or adding horizontally to a vertical system)
				// directly as part of the system.
				Node* pnSplit	= new Node();
				if(parentTy == Node::Type::Vertical)
					pnSplit->type = Node::Type::Horizontal;
				else if(parentTy == Node::Type::Horizontal)
					pnSplit->type = Node::Type::Vertical;

				this->_Replace(targ, pnSplit);

				bool instBefore = 
					(whereAroundTarg == Node::Dest::Above) || 
					(whereAroundTarg == Node::Dest::Left);

				targ->parent	= pnSplit;
				if(instBefore)
				{
					pnSplit->children.push_back(targ);
					return InsertWinLoc::AtNodeChild(pnSplit, 0);
				}
				else
				{
					pnSplit->children.push_back(targ);
					return InsertWinLoc::AtNodeChild(pnSplit, 1);
				}
			}

		}
	}

	return InsertWinLoc::AtInvalid();
}

Node* Layout::Add(HWND hwnd, Node* targ, Node::Dest whereAroundTarg)
{
	InsertWinLoc ins = this->_ScanAndPrepAddLoc(targ, whereAroundTarg);
	if(ins.valid == false)
		return nullptr;

	Node * pnNew = new Node();
	if(this->Integrate(ins, pnNew) == true)
	{ 
		this->_RegisterHwndToNode(hwnd, pnNew);
		ASSERT_ISNODEWIN(pnNew);
		pnNew->UpdateWindowTitlebarCache();
	}
	else
	{ 
		delete pnNew;
		return nullptr;
	}

	return pnNew;
}

bool Layout::Steal(Node* n, Node* targ, Node::Dest whereAroundTarg, const LProps& /*props*/)
{
	assert(n->parent == nullptr);
	ASSERT_ISNODEWIN(n);

	InsertWinLoc ins = this->_ScanAndPrepAddLoc(targ, whereAroundTarg);
	if(ins.valid == false)
		return false;

	if(this->Integrate(ins, n) == true)
	{

		n->proportion = 0.0f;
		//if(n->parent != nullptr)
		//	n->parent->ResizeChildrenByProportions(props);

		HWND hwnd = n->Hwnd();
		if(hwnd != NULL)
		{
			this->_RegisterHwndToNode(n->Hwnd(), n, true, false);
			this->hwndLookup[hwnd] = n;
		}
	

		return true;
	}
	else
	{ 
		delete n;
		return false;
	}

}

bool Layout::Integrate(InsertWinLoc ins, Node* nodeToIntegrate)
{
	if(ins.valid == false)
		return false;

	// Pretty much everywhere we have a Window node, the hwnd
	// should be set, EXCEPT here, where the Node MAY NOT be fully
	// set up yet (since this function is part of the setup process).
	assert(nodeToIntegrate->type == Node::Type::Window);

	if(ins.parent == nullptr)
	{
		assert(this->root == nullptr);
		assert(nodeToIntegrate->parent == nullptr);
		
		this->root = nodeToIntegrate;
		return true;
	}

	// There shouldn't be any inserting into windows, they're
	// leaf nodes.
	assert(ins.parent->type != Node::Type::Window);
	assert(ins.childIdx >= 0 && ins.childIdx <= ins.parent->children.size());

	ins.parent->children.insert(
		ins.parent->children.begin() + ins.childIdx,
		nodeToIntegrate);

	// If we integrate into this parent, it should
	// end up with 2 or more children, or else there
	// shouldn't have been a container.
	assert(ins.parent->children.empty() == false);

	nodeToIntegrate->parent = ins.parent;
	return true;
}

bool Layout::DeleteWindow(HWND hwnd, std::set<Node*>* involved)
{
	if(hwnd == NULL)
		return false;

	auto it = this->hwndLookup.find(hwnd);

	if(it == this->hwndLookup.end())
		return false;

	return this->DeleteWindow(it->second, involved);
}

bool Layout::_CleanupWindowNodeRemoval(
	Node* targ,
	Node::ForgetAction fa,
	std::set<Node*>* involvedOut)
{
	std::vector<Layout::ForgetUndo> involved;
	std::set<Node*> removed;
	if(this->_ForgetWindow(targ, fa, involved, removed, true, true) == false)
		return false;

	// Return back other things that were involved with the operation
	if(involvedOut != nullptr)
	{
		for(Layout::ForgetUndo i : involved)
		{
			// Only report things that aren't about to be deleted.
			if(removed.find(i.node) != removed.end())
				continue;

			involvedOut->insert(i.node);
		}
	}

	for(Node* toRm : removed)
		delete toRm;

	return true;
}

bool Layout::DeleteWindow(Node* targ, std::set<Node*>* involvedOut)
{
	HWND hwnd = targ->Hwnd();

	if (hwnd != NULL)
	{
		SendMessage(hwnd, WM_DESTROY, 0, 0);
		targ->ForgetHWND();
	}

	return this->_CleanupWindowNodeRemoval(
		targ,
		Node::ForgetAction::Delete, 
		involvedOut);
}

bool Layout::ReleaseWindow(Node* targ, std::set<Node*>* involvedOut, bool delNode)
{
	HWND hwnd = targ->Hwnd();
	if(hwnd != NULL)
		targ->ReleaseHWND(false);

	return this->_CleanupWindowNodeRemoval(
		targ,
		Node::ForgetAction::Forget,
		involvedOut);
}

bool Layout::ReleaseWindow(HWND hwnd, std::set<Node*>* involved, bool delNode)
{
	auto it = this->hwndLookup.find(hwnd);
	if(it == this->hwndLookup.end())
		return false;

	return this->ReleaseWindow(it->second, involved);
}

void Layout::ClearSashes()
{
	for(size_t i = 0; i < this->sashes.size(); ++i)
		delete this->sashes[i];

	this->sashes.clear();
}

void Layout::RebuildSashes(const LProps& lp)
{
	this->ClearSashes();

	// If the root isn't a horizontal or vertical container,
	// there's no hierarchy to process because will not
	// be any sashes.
	if(
		this->root == nullptr || 
		this->root->type == Node::Type::Window ||
		this->root->type == Node::Type::Tabs)
	{ 
		return;
	}

	// Start processing sash hierarchy.
	std::vector<Node*> toProcess;
	toProcess.push_back(this->root);

	while(!toProcess.empty())
	{
		Node* pn = toProcess.back();
		toProcess.pop_back();

		if(pn->type == Node::Type::Horizontal)
		{ 
			for(size_t i = 0; i < pn->children.size(); ++i)
			{
				Node* child = pn->children[i];

				// There will always be one less sash to make than
				// there are divisions.
				if(i < pn->children.size() - 1)
				{ 
					Sash* s		= new Sash();
					s->left		= child;
					s->right	= pn->children[i + 1];
					s->pos.x	= child->cachePos.x + child->cacheSize.x;
					s->pos.y	= child->cachePos.y;
					s->size.x	= lp.sashWidth;
					s->size.y	= child->cacheSize.y;
					s->dir		= Sash::Dir::Horiz;
					//
					this->sashes.push_back(s);
				}

				// Only queue calculation of sashes for things that can
				// actually have sashes.
				if(
					child->type == Node::Type::Window || 
					child->type == Node::Type::Tabs)
				{
					continue;
				}
				else
					toProcess.push_back(child);
			}
		}
		else //if(pn->type == Node::Type::Vertical)
		{
			for(size_t i = 0; i < pn->children.size(); ++i)
			{
				Node* child = pn->children[i];

				// There will always be one less sash to make than
				// there are divisions.
				if(i < pn->children.size() - 1)
				{ 
					Sash* s		= new Sash();
					s->top		= child;
					s->bot		= pn->children[i + 1];
					s->pos.x	= child->cachePos.x;
					s->pos.y	= child->cachePos.y + child->cacheSize.y;
					s->size.x	= child->cacheSize.x;
					s->size.y	= lp.sashHeight;
					s->dir		= Sash::Dir::Vert;
					//
					this->sashes.push_back(s);
				}

				if (
					child->type == Node::Type::Window || 
					child->type == Node::Type::Tabs)
				{
					continue;
				}
				else
					toProcess.push_back(child);
			}
		}
	}
}

void Layout::RefreshSashes(const LProps& lp)
{
	for(Sash* s : this->sashes)
	{
		if(s->dir == Sash::Dir::Horiz)
		{
			s->pos.x = s->left->cachePos.x + s->left->cacheSize.x;
			s->pos.y = s->left->cachePos.y;
			s->size.x = lp.sashWidth;
			s->size.y = s->left->cacheSize.y;
		}
		else
		{
			s->pos.x = s->top->cachePos.x;
			s->pos.y = s->top->cachePos.y + s->top->cacheSize.y;
			s->size.x = s->top->cacheSize.x;
			s->size.y = lp.sashHeight;
		}
	}
}

Node* Layout::GetNodeAt( const wxPoint& pt, bool stopAtTabs)
{
	if(this->root == nullptr)
		return nullptr;

	if(this->root->ContainsPoint(pt) == false)
		return nullptr;

	Node * lastIn = this->root;

	while(lastIn->children.size() > 0)
	{
		if(stopAtTabs == true && lastIn->type == Node::Type::Tabs)
			return lastIn;

		bool cont = false;
		for(Node* pn : lastIn->children)
		{
			if(pn->ContainsPoint(pt) == true)
			{
				cont = true;
				lastIn = pn;
				break;
			}
		}

		if(cont == false)
			break;
	}

	return lastIn;
}

Sash* Layout::GetSashAt(const wxPoint& pt)
{
	for(Sash* s : this->sashes)
	{
		if(s->Contains(pt) == true)
			return s;
	}
	return nullptr;
}

void Layout::Resize(
	const wxSize& newSz, 
	const LProps& lp, 
	bool applyPadding)
{
	if(this->root == nullptr)
		return;

	Lot lroot;
	lroot.pn = this->root;
	lroot.x = 0;
	lroot.y = 0;
	lroot.w = newSz.x;
	lroot.h = newSz.y;

	if(applyPadding == true)
	{
		lroot.x += lp.paddLeft;
		lroot.y += lp.paddTop;
		lroot.w -= lp.paddLeft + lp.paddRight;
		lroot.h -= lp.paddTop + lp.paddBottom;
	}

	_ResizeFromLot(lroot, lp);
}

void Layout::Resize(
	Node* pn, 
	const wxPoint& newPt, 
	const wxSize& newSz, 
	const LProps& lp)
{
	if(pn == nullptr)
		return;

	Lot lot;
	lot.pn = pn;
	lot.x = newPt.x;
	lot.y = newPt.y;
	lot.w = newSz.x;
	lot.h = newSz.y;

	_ResizeFromLot(lot, lp);
}

void Layout::Resize(Node* pn, const LProps& lp)
{
	if(pn == nullptr)
		return;

	Lot lot;
	lot.pn = pn;
	lot.x = pn->cachePos.x;
	lot.y = pn->cachePos.y;
	lot.w = pn->cacheSize.x;
	lot.h = pn->cacheSize.y;

	_ResizeFromLot(lot, lp);
}

void Layout::_ResizeFromLot(const Lot& lroot, const LProps& lp)
{
	std::vector<Lot> toCalc;
	toCalc.push_back(lroot);

	while(toCalc.empty() == false)
	{
		Lot l = toCalc.back();
		toCalc.pop_back();

		l.pn->cachePos = wxPoint(l.x, l.y);
		l.pn->cacheSize = wxSize(l.w, l.h);

		if(l.pn->type == Node::Type::Window)
		{
			l.pn->cachePos = wxPoint(l.x, l.y);
			l.pn->cacheSize = wxSize(l.w, l.h);

			l.pn->CacheFromProps(lp);

			if(l.pn->win != NULL)
			{
				SetWindowPos(
					l.pn->win, 
					NULL,
					l.pn->cachedClient.x,
					l.pn->cachedClient.y,
					l.pn->cachedClient.width,
					l.pn->cachedClient.height,
					SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
			}
			l.pn->ResetTabsBarLayout(lp);
		}
		else if(l.pn->type == Node::Type::Tabs)
		{
			l.pn->CacheFromProps(lp);

			int xTab = 0;
			// For now, all children window in a tabs region will
			// be the same size as the tabs region, but this will
			// probably be changed to be a smaller region inside
			// later.
			for(size_t tabIdx = 0; tabIdx < l.pn->children.size(); ++tabIdx)
			{
				Node* pnChild = l.pn->children[tabIdx];

				pnChild->cachePos		= wxPoint(l.x, l.y);
				pnChild->cacheSize		= wxSize(l.w, l.h);
				pnChild->cachedClient	= l.pn->cachedClient;
				pnChild->cachedTab		= l.pn->cachedTab;
				pnChild->cachedTab.x += xTab;
				pnChild->cachedTabLcl.x = xTab;
				pnChild->cachedTabLcl.y = 0;
				pnChild->cachedTabLcl.SetSize( pnChild->cachedTab.GetSize());
				xTab += pnChild->cachedTab.width;
				pnChild->proportion		= 1.0f;

				if(l.pn->selectedTabIdx ==tabIdx)
				{ 
					SetWindowPos(
						pnChild->win, 
						NULL,
						l.pn->cachedClient.x,
						l.pn->cachedClient.y,
						l.pn->cachedClient.width,
						l.pn->cachedClient.height,
						SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
				}
			}
			l.pn->ResetTabsBarLayout(lp);
			
		}
		else if(l.pn->type == Node::Type::Horizontal)
		{
			// The amount of space that will be taken up with
			// resize sashes.
			int padSpace = (l.pn->children.size() - 1) * lp.sashWidth;
			// The amount of space available when excluding the 
			// width of resize sashes.
			int freeWidth = l.pn->cacheSize.x - padSpace;
			l.pn->NormalizeChildren();

			float x = l.x;
			for(Node* pnChild : l.pn->children)
			{
				// Queue the work on a future iteration.
				Lot cl;
				cl.pn = pnChild;
				cl.x = (int)x;
				cl.w = (int)(freeWidth * pnChild->proportion);
				cl.y = l.y;
				cl.h = l.h;

				x += freeWidth * pnChild->proportion + lp.sashWidth;
				toCalc.push_back(cl);
			}
		}
		else if(l.pn->type == Node::Type::Vertical)
		{
			// The amount of space that will be taken up with
			// resize sashes.
			int padSpace = (l.pn->children.size() - 1) * lp.sashHeight;
			// The amount of space available when excluding the 
			// width of resize sashes.
			int freeHeight = l.pn->cacheSize.y - padSpace;
			l.pn->NormalizeChildren();

			float y = l.y;
			for(Node* pnChild : l.pn->children)
			{
				// Queue the work on a future iteration.
				Lot cl;
				cl.pn = pnChild;
				cl.x = l.x;
				cl.w = l.w;
				cl.y = (int)y;
				cl.h = (int)(freeHeight * pnChild->proportion);

				y += freeHeight * pnChild->proportion + lp.sashWidth;
				toCalc.push_back(cl);
			}
		}
	}
}

void Layout::Clear()
{
	if(this->root == nullptr)
		return;

	std::vector<Node*> toDel;
	toDel.push_back(this->root);
	while(toDel.empty() == false)
	{
		Node* n = toDel.back();
		toDel.pop_back();

		for(Node* child : n->children)
			toDel.push_back(child);

		delete n;
	}
	this->root = nullptr;
	this->ClearSashes();
	this->hwndLookup.clear();
}

DropResult Layout::ScanForDrop(const wxPoint& pt, const LProps& lp)
{
	if(this->root == nullptr)
		return DropResult(DropResult::Where::None);

	// Find the deepest node we're on.
	std::queue<Node*> toScan;
	toScan.push(this->root);

	// The innermost the point was in.
	Node* inner = nullptr;
	while(!toScan.empty())
	{
		Node* pn = toScan.front();
		toScan.pop();

		if(pn->ContainsPoint(pt) == false)
			continue;

		inner = pn;
		for(Node* child : pn->children)
			toScan.push(child);
	}

	if(inner == nullptr)
		return DropResult(DropResult::Where::None);

	std::vector<DropResult> topSt;
	std::vector<DropResult> leftSt;
	std::vector<DropResult> botSt;
	std::vector<DropResult> rightSt;

	topSt.push_back(	
		DropResult(
			DropResult::Where::Top,		
			inner, 
			nullptr, 
			wxPoint(inner->cachePos), 
			wxSize(inner->cacheSize.x, lp.dropEdgeWidth)));
	leftSt.push_back(	
		DropResult(
			DropResult::Where::Left,		
			inner, nullptr, 
			wxPoint(inner->cachePos), 
			wxSize(lp.dropEdgeWidth, inner->cacheSize.y)));
	botSt.push_back(	
		DropResult(
			DropResult::Where::Bottom,	
			inner, 
			nullptr,
			wxPoint(inner->cachePos.x, inner->CacheBot() - lp.dropEdgeWidth),
			wxSize(inner->cacheSize.x, lp.dropEdgeWidth)));
	rightSt.push_back(	
		DropResult(
			DropResult::Where::Right,	
			inner, 
			nullptr,
			wxPoint(inner->CacheRight() - lp.dropEdgeWidth, inner->cachePos.y),
			wxSize(lp.dropEdgeWidth, inner->cacheSize.y)));

	int leftEdge	= inner->cachePos.x;
	int rightEdge	= inner->cachePos.x  + inner->cacheSize.x;
	int topEdge		= inner->cachePos.y;
	int botEdge		= inner->cachePos.y + inner->cacheSize.y;

	// Find how many items each side has. For each item the side 
	// gets pushed inwards. Only after pushing all sides can we
	// also determine the center for the onto.
	int leftFar		= leftEdge	+ leftSt.size()	* lp.dropEdgeWidth;
	int rightFar	= rightEdge - rightSt.size()* lp.dropEdgeWidth;
	int topFar		= topEdge	+ topSt.size()	* lp.dropEdgeWidth;
	int botFar		= botEdge	- botSt.size()	* lp.dropEdgeWidth;
	wxPoint dropCenter = 
		wxPoint(
			(leftFar+ rightFar	)/2, 
			(topFar	+ botFar	)/2);

	int dstLeft		= pt.x		- leftFar;	
	int dstRight	= rightFar	- pt.x;
	int dstTop		= pt.y		- topFar;	
	int dstBot		= botFar	- pt.y;
	int dstCenter =						
		std::max(
			std::abs(pt.x - dropCenter.x), 
			std::abs(pt.y - dropCenter.y));

	enum class Side
	{
		Invalid,
		ELeft,
		ERight,
		ETop,
		EBot,
		ECenter
	};

	Side side = Side::Invalid;
	int dst = std::numeric_limits<int>::max();

	// Gauntlet to find what feature the mouse point is
	// closest to.
	if(dst > dstLeft)
	{
		side = Side::ELeft;
		dst = dstLeft;
	}
	if(dst > dstRight)
	{
		side = Side::ERight;
		dst = dstRight;
	}
	if(dst > dstTop)
	{
		side = Side::ETop;
		dst = dstTop;
	}
	if(dst > dstBot)
	{
		side = Side::EBot;
		dst = dstBot;
	}
	if(dst > dstCenter)
	{
		side = Side::ECenter;
		dst = dstCenter;
	}

	if(side == Side::ELeft)
	{
		return leftSt[0];
	}
	else if(side == Side::ERight)
	{
		return rightSt[0];
	}
	else if(side == Side::ETop)
	{
		return topSt[0];
	}
	else if(side == Side::EBot)
	{
		return botSt[0];
	}
	else if(side == Side::ECenter)
	{
		return 
			DropResult(
				DropResult::Where::Onto, 
				inner, 
				nullptr, 
				wxPoint(dropCenter.x - lp.dropIntoRad, dropCenter.y - lp.dropIntoRad),
				wxSize(lp.dropIntoRad * 2, lp.dropIntoRad * 2));
	}

	return DropResult(DropResult::Where::None, inner, nullptr, wxPoint(0,0), wxSize(0,0));
}

Node* Layout::GetNodeFrom(HWND hwnd)
{
	auto itFind = this->hwndLookup.find(hwnd);
	if (itFind == this->hwndLookup.end())
		return nullptr;

	return itFind->second;
}

std::vector<HWND> Layout::CollectHWNDs() const
{
	std::vector<HWND> ret;
	this->CollectHWNDs(ret);
	return ret;
}

void Layout::CollectHWNDs(std::vector<HWND>& dst) const
{
	std::queue<Node*> toScan;
	toScan.push(this->root);

	while (!toScan.empty())
	{
		Node* pn = toScan.front();
		toScan.pop();

		if (pn->type == Node::Type::Window)
		{ 
			assert(pn->Hwnd() != NULL);
			dst.push_back(pn->Hwnd());
		}

		for (Node* child : pn->children)
			toScan.push(child);
	}
}

void Layout::CollectHWNDNodes(std::vector<Node*>& outVec)
{
	std::queue<Node*> toScan;
	toScan.push(this->root);

	while (!toScan.empty())
	{
		Node* pn = toScan.front();
		toScan.pop();

		if (pn->type == Node::Type::Window)
		{
			assert(pn->Hwnd() != NULL);
			outVec.push_back(pn);
		}

		for (Node* child : pn->children)
			toScan.push(child);
	}
}

bool InBounds(wxPoint r_pos, wxSize r_sz, wxPoint pt)
{
	if(pt.x < r_pos.x)
		return false;

	if(pt.y < r_pos.y)
		return false;

	if(pt.x > r_pos.x + r_sz.x)
		return false;

	if(pt.y > r_pos.y + r_sz.y)
		return false;

	return true;
}

bool InBounds(wxRect r, wxPoint pt)
{
	return InBounds(
		r.GetPosition(), 
		r.GetSize(),
		pt);
}

int Layout::_CountInstancedTabsBarsInHierarchy()
{
	if(this->root == nullptr)
		return 0;

	int counter = 0;
	std::vector<Node*> toScan = {this->root};
	while(!toScan.empty())
	{
		Node* curNode = toScan.back();
		toScan.pop_back();

		if(curNode->GetTabsBar() != nullptr)
			++counter;

		for(Node* childToProcess : toScan)
			toScan.push_back(childToProcess);
	}
	return counter;
}

int Layout::_CountNodesInHierarchy()
{
	if(this->root == nullptr)
		return 0;

	int counter = 0;
	std::vector<Node*> toScan = {this->root};
	while(!toScan.empty())
	{
		++counter;
		Node* curNode = toScan.back();
		toScan.pop_back();

		for(Node* childToProcess : toScan)
			toScan.push_back(childToProcess);
	}
	return counter;
}

bool Layout::_TestValidity()
{
	// Checklistion
	// >t3$t {Layout.cpp}
	// ✅t3$t_b408 Are all embedded HWNDs accounted for in the hwndLookup?
	// ✅t3$t_a4af Do sashes point to the right thing?
	// ✅t3$t_a4af Do sashes point to the right thing?
	// ✅t3$t_a2eb Is the root propery maintained?
	// ✅t3$t_49fb Is parent-child hierarchy propery maintained?
	// ✅t3$t_4454 Is there no extra crud?
	// ✅t3$t_4c90 Are all hierarchy items accounted for?

	std::vector<Node*> toScan = {this->root};
	
	// All the nodes found in the hierarchy
	std::set<Node*> encounteredNodes;
	std::set<HWND> encounteredHWNDs;

	// The order we process these elements isn't important, 
	// as long as the entire hierarchy is processed.
	while(!toScan.empty())
	{
		toScan.pop_back();
		Node* n = toScan.back();

		// Track encountered nodes, but also make sure there
		// aren't repeats.
		assert(encounteredNodes.find(n) == encounteredNodes.end());
		encounteredNodes.insert(n);

		// Queue children to be scanned
		for(size_t i = 0; i < n->children.size(); ++i)
			toScan.push_back(n->children[i]);

		// Sanity check
		assert(n != nullptr);

		// The root Node is the only node without a parent.
		if(n->parent == nullptr)
			assert(this->root == n);
		else
			assert(this->root != n);

		// Leaf (Window/HWND) Nodes should not have any children;
		// all other Nodes, containers, should have at least 2 children.
		if(n->type == Node::Type::Window)
		{ 
			assert(n->children.size() == 0);

			HWND hwnd = n->Hwnd();
			if(hwnd != NULL)
			{
				// HWNDs should only be embedded into any one Node.
				// This is true across the entire AppDock system, but there isn't
				// currently a way to test that within the confines of how the
				// _TestValidity() system is set up.
				assert(encounteredHWNDs.find(hwnd) == encounteredHWNDs.end());
				encounteredHWNDs.insert(hwnd);

				// Everything that has a valid HWND window was from spawning from
				// the command line, and that command should be cached.
				assert(!n->cmdLine.empty());
			}
		}
		else
		{ 
			assert(n->children.size() >= 2);

			// A horizontal cannot have horizontal children.
			// (Or else the contents of the child horizontal container
			// should just be in the parent contain).
			if(n->type == Node::Type::Horizontal)
			{ 
				for(size_t i = 0; i < n->children.size(); ++i)
					assert(n->children[i]->type != Node::Type::Horizontal);
			}

			// Similarly...
			// A vertical cannot have vertical children.
			// (Or else the contents of the child vertical container
			// should just be in the parent contain).
			if(n->type == Node::Type::Vertical)
			{
				for(size_t i = 0; i < n->children.size(); ++i)
					assert(n->children[i]->type != Node::Type::Vertical);
			}

			// tabs can only have window children.
			if(n->type == Node::Type::Tabs)
			{
				for(size_t i = 0; i < n->children.size(); ++i)
					assert(n->children[i]->type == Node::Type::Window);
			}
		}

		// Check topology correctness
		if(n->parent != nullptr)
		{
			for(size_t i = 0; i < n->children.size(); ++i)
				assert(n->children[i]->parent = n);
		}
	}

	// All encountered HWNDs should match the registered HWNDs
	auto hwndLeft = encounteredHWNDs;
	//
	std::set<Node*> hwndLookupVals;
	for(auto itHwndLUp : this->hwndLookup)
	{
		// Every HWND is unique, and mapped to a specific Node, so the Node
		// items in the lookup should also be unique.
		assert(hwndLookupVals.find(itHwndLUp.second) == hwndLookupVals.end());
		hwndLookupVals.insert(itHwndLUp.second);

		// The Node registered should be in the hiererachy;
		assert(encounteredNodes.find(itHwndLUp.second) != encounteredNodes.end());

		// Test that the lookup is mapped correctly.
		assert(itHwndLUp.second->Hwnd() == itHwndLUp.first);

		// If the HWND was found in a node, it should be registered in the lookup;
		assert(hwndLeft.find(itHwndLUp.first) != hwndLeft.end());
		hwndLeft.erase(itHwndLUp.first);
	}
	// If it was found, it should have been registered, there shouldn't be extra
	// HWND crud the Layout doesn't know about in its lookup.
	assert(hwndLeft.empty() == false);

	return true;
}