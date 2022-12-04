#include "Node.h"
#include "Layout.h"
#include "LProps.h"
#include "TabsBar.h"


int Node::IDCT = 0;

Node::Node()
{
	this->id = IDCT;
	++IDCT;
}

Node::~Node()
{
	if(this->type == Node::Type::Window)
		assert(this->children.empty());
}

bool Node::ContainsPoint(const wxPoint& pt)
{
	return InBounds(this->cachePos, this->cacheSize, pt);
}

void Node::DeleteChildren(bool recurse)
{
	for(Node* pn : this->children)
	{
		if(recurse)
			pn->DeleteChildren(true);

		delete pn;
	}
}


void Node::NormalizeChildren()
{
	if(this->children.empty())
		return;

	// Gather accumulated proportion. If we have any zeros or
	// end up with all zeros, we need to take note of that.
	bool anyZeros = false;
	float total = 0.0f;
	int ct = 0;
	for(Node* pn : this->children)
	{
		if(pn->proportion == 0.0f)
			anyZeros = true;
		else
		{
			++ct;
			total += pn->proportion;
		}
	}

	// If we have all zeros, just average everything.
	if(ct == 0)
	{
		float avg = 1.0f/(float)this->children.size();
		for(Node* pn : this->children)
			pn->proportion = avg;

		return;
	}

	// If we have any zeros, set it as the average
	if (anyZeros)
	{
		// Nothing to fully resolve zeros, just try to do something
		// "reasonable" by giving the zero items the average proportion
		// of the other non-zeros.
		float avg = total / ct;
		for(Node* pn : this->children)
		{
			if(pn->proportion == 0.0f)
			{ 
				pn->proportion = avg;
				total += avg;
			}

		}
	}

	// Normalize so the sum of all proportions is 1.0.
	for(Node* pn : this->children)
		pn->proportion /= total;
}

bool Node::DestroyHWND()
{
	if(this->win == NULL)
		return false;

	DestroyWindow(this->win);
	this->win = NULL;
	return true;
}

bool Node::ForgetHWND()
{
	if(this->win == NULL)
		return false;

	this->win = NULL;

	return true;
}

bool Node::ForgetHWND(ForgetAction fa)
{
	if(this->win == NULL)
		return false;

	switch(fa)
	{
	case ForgetAction::Void:
		// Do nothing
		return true;

	case ForgetAction::Forget:
		this->ForgetHWND();
		return true;

	case ForgetAction::Hide:
		this->HideWindow();
		return true;

	case ForgetAction::Delete:
		this->DestroyHWND();
		return true;

	default:
		assert(!"Unhandled case for ForgetHWND.");
		break;
	}
	return false;
}

bool Node::ReleaseHWND(bool forget)
{
	if(this->win == NULL)
		return false;

	//HWND hwndDesktop = GetDesktopWindow();
	//SetParent(this->win, hwndDesktop);
	SetParent(this->win, NULL);
	SetWindowLongPtrA(this->win, GWL_STYLE, this->origStyle);
	SetWindowLongPtrA(this->win, GWL_EXSTYLE, this->origStyleEx);
	//
	RECT r;
	GetWindowRect(this->win, &r);
	SetWindowPos(
		this->win,
		NULL,
		r.left,
		r.right,
		this->origSize.x,
		this->origSize.y,
		/*SWP_FRAMECHANGED|SWP_DRAWFRAME|*/SWP_NOZORDER|SWP_SHOWWINDOW);
	

	if(forget)
		this->ForgetHWND();

	return true;
}

void Node::CacheFromProps(const LProps& lp)
{
	this->cachedTab.x			= this->cachePos.x;
	this->cachedTab.y			= this->cachePos.y + lp.tabPadTop;
	this->cachedTab.height		= lp.tabHeight;
	this->cachedTab.width		= lp.tabWidth;

	this->cachedTabLcl.x	= this->cachedTab.x - this->cachePos.x;
	this->cachedTabLcl.y	= this->cachedTab.y - this->cachePos.y;
	this->cachedTabLcl.SetSize(this->cachedTab.GetSize());

	int cutFromTop				= lp.tabPadTop + lp.tabPadBot + lp.tabHeight;
	this->cachedClient.x		= this->cachePos.x;
	this->cachedClient.y		= this->cachePos.y + cutFromTop;
	this->cachedClient.width	= this->cacheSize.x;
	this->cachedClient.height	= this->cacheSize.y - cutFromTop;
}


wxSize Node::CalculateMinSize(const LProps& lp)
{
	switch(this->type)
	{
	case Node::Type::Horizontal:
	{
		wxSize ret;
		for(int i = 0; i < this->children.size(); ++i)
		{
			wxSize childSz = this->children[i]->CalculateMinSize(lp);
			ret.x += childSz.x;
			ret.y = std::max(ret.y, childSz.y);

			if(i != 0)
				ret.x += lp.sashWidth;
		}
		return ret;

	}
	case Node::Type::Vertical:
	{
		wxSize ret;
		for(int i = 0; i < this->children.size(); ++i)
		{
			wxSize childSz = this->children[i]->CalculateMinSize(lp);
			ret.x = std::max(ret.x, childSz.x);
			ret.y += childSz.y;

			if(i != 0)
				ret.y += lp.sashHeight;
		}
		return ret;
	}
	case Node::Type::Window:
	case Node::Type::Tabs:
		return 
			wxSize(
				lp.minClientWidth, 
				lp.minClientHeight + lp.tabHeight + lp.tabPadTop + lp.tabPadBot);
	}


	return wxSize(0, 0);
}

void Node::CalculateChildProportions_Prop()
{
	// Note there are no safteys against edge cases
	// such as 0 sized children, or 0 length children.
	
	float total = 0.0f;
	for(Node* pn : this->children)
		total += pn->proportion;

	for(Node* pn : this->children)
		pn->proportion /= total;

}

void Node::CalculateChildProportions_Cache()
{
	float total = 0.0f;

	if(this->type == Node::Type::Horizontal)
	{ 
		for(Node* pn : this->children)
			total += pn->cacheSize.x;

		for(Node* pn : this->children)
			pn->proportion = pn->cacheSize.x / total;
	}
	else if(this->type == Node::Type::Vertical)
	{
		for(Node* pn : this->children)
			total += pn->cacheSize.y;

		for(Node* pn : this->children)
			pn->proportion = pn->cacheSize.y / total;
	}
}

void Node::ResizeChildrenByProportions(const LProps& lp)
{
	float totalProp = 0.0f;
	for(Node* child : this->children)
		totalProp += child->proportion;


	if(this->type == Node::Type::Horizontal)
	{
		int allSash = lp.sashWidth * (this->children.size() - 1);
		int spaceNodes = this->cacheSize.x - allSash;
		// We move across with a float instead of an int so
		// we don't start loosing pixels to float truncation.
		float x = this->cachePos.x;

		for(int i = 0; i < this->children.size(); ++i)
		{
			Node* c = this->children[i];
			c->proportion /= totalProp;
			int w = spaceNodes * c->proportion;

			Layout::Resize(
				c,
				wxPoint((int)x, this->cachePos.y),
				wxSize(w, this->cacheSize.y),
				lp);

			x += w;
			x += lp.sashWidth;
		}
	}
	else if(this->type == Node::Type::Vertical)
	{
		int allSash = lp.sashHeight * (this->children.size() - 1);
		int spaceNodes = this->cacheSize.y - allSash;
		float y = this->cachePos.y;

		for(int i = 0; i < this->children.size(); ++i)
		{
			Node* c = this->children[i];
			c->proportion /= totalProp;
			int h = spaceNodes * c->proportion;

			Layout::Resize(
				c, 
				wxPoint(this->cachePos.x, y), 
				wxSize(this->cacheSize.x, h), 
				lp);

			y += h;
			y += lp.sashHeight;
		}
	}

}

void Node::ResizeChildrenByProportions(std::vector<float> vecp, const LProps& lp)
{
	assert(vecp.size() == this->children.size());

	for(size_t i = 0; i < vecp.size(); ++i)
		this->children[i]->proportion = vecp[i];

	this->ResizeChildrenByProportions(lp);
}

bool Node::HideWindow()
{
	return this->ShowWindow(false);
}

bool Node::ShowWindow(bool show)
{
	if(this->win == NULL)
		return false;

	if(show == true)
		::ShowWindow(this->win, SW_SHOW);
	else
		::ShowWindow(this->win, SW_HIDE);

	return true;
}

int Node::Depth() const
{
	int ret = 0;
	for(Node* it = this->parent; it != nullptr; it = it->parent)
		++ret;

	return ret;
}

bool Node::SelectTab(int idx, bool updateWinVisibility)
{
	assert(this->type == Node::Type::Tabs);
	if(idx < 0 || idx > this->children.size())
		return false;

	this->selectedTabIdx = idx;

	if(updateWinVisibility)
		this->UpdateTabWindowVisibility();

	return true;
}

void Node::UpdateTabWindowVisibility()
{
	assert(this->type == Node::Type::Tabs);
	for(size_t i = 0; i < this->children.size(); ++i)
	{
		Node* child = this->children[i];
		ASSERT_ISNODEWIN(child);

		child->ShowWindow(i == this->selectedTabIdx);
	}
}

bool Node::SelectTab(Node* winChild)
{
	for(size_t i = 0; i < this->children.size(); ++i)
	{
		if(this->children[i] == winChild)
			return SelectTab(i);
	}
	return false;
}

void Node::ClearTabsBar()
{
	if(this->tabsBar == nullptr)
		return;

	delete this->tabsBar;
	this->tabsBar = nullptr;
}

Node* Node::ChildOtherThan(Node* n)
{
	if(this->children.empty())
		return nullptr;

	for(Node* nCmp : this->children)
	{
		if(n != nCmp)
			return nCmp;
	}

	return nullptr;
}

void Node::SetTabsBar(TabsBar* tb)
{
	assert(this->tabsBar == nullptr);
	this->tabsBar = tb;
}

void Node::ForgetTabsBar()
{
	this->tabsBar = nullptr;
}

void Node::ResetTabsBarLayout(const LProps& lp)
{
	if(this->tabsBar == nullptr)
		return;

	assert(this->tabsBar != nullptr);
	assert(this->type == Node::Type::Tabs || this->type == Node::Type::Window);

	this->tabsBar->SetPosition(this->cachedTab.GetPosition());
	this->tabsBar->SetSize(wxSize(this->cacheSize.x, lp.tabPadBot + lp.tabHeight));
}

void Node::UpdateWindowTitlebarCache()
{
	ASSERT_ISNODEWIN(this);

	// We're leaving the project to where the windows API is using a WCHAR
	// for strings, and std::string doesn't play nice with it, so we're using
	// wxString as a utility to handle string type conversions.
	wxString title = "";

	const int titleBuffLen = 128;
	WCHAR szBuff[titleBuffLen];
	if (GetWindowText(this->Hwnd(), szBuff, titleBuffLen) != 0)
		title = szBuff;

	this->cachedTitlebar = title;
}

bool Node::UsesCustomTabTitlebar() const
{
	ASSERT_ISNODEWIN(this);
	
	return this->titlebarType == TabNameType::Custom && !this->customTabName.empty();
}

Node* Node::GetTabsParent()
{
	ASSERT_ISNODEWIN(this);

	if (
		this->parent == nullptr ||
		this->parent->type != Node::Type::Tabs)
	{
		return nullptr;
	}

	return this->parent;
}

std::string Node::GetPreferredTabTitlebar() const
{
	ASSERT_ISNODEWIN(this);

	switch (this->titlebarType)
	{
	case TabNameType::Custom:
		if(!this->customTabName.empty())
			return this->customTabName;
		break;
	case TabNameType::Command:
		// This conversion pipeline is pretty janky - 
		// May need to find something more elegant.
		return (std::string)(wxString)this->cmdLine;
	}
	
	return this->cachedTitlebar;
}