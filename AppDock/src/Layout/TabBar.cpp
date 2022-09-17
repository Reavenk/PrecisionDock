#include "TabBar.h"
#include "../DockWin.h"
#include "../AppDock.h"
#include <wx/dcbuffer.h>

#include "TabCloseBtn.xpm"

// TODO: Rename to TabsBar - nomenclature should be that a grouping
// of tabs be explicitly plural.

BEGIN_EVENT_TABLE(TabBar, wxWindow)
	EVT_PAINT					(TabBar::OnDraw			)
	EVT_LEFT_DOWN				(TabBar::OnMouseLDown	)
	EVT_LEFT_UP					(TabBar::OnMouseLUp		)
	EVT_MOTION					(TabBar::OnMouseMotion	)
	EVT_RIGHT_DOWN				(TabBar::OnMouseRDown	)
	EVT_SIZE					(TabBar::OnSize			)
	EVT_MOUSE_CAPTURE_CHANGED	(TabBar::OnMouseChanged	)
	EVT_MOUSE_CAPTURE_LOST		(TabBar::OnMouseCaptureLost)
	EVT_ENTER_WINDOW			(TabBar::OnMouseEnter	)
	EVT_LEAVE_WINDOW			(TabBar::OnMouseExit	)

	EVT_MENU((int)CmdIds::Menu_CloneWin,    TabBar::OnMenu_RClick_Clone        )
	EVT_MENU((int)CmdIds::Menu_RenameWin,   TabBar::OnMenu_RClick_Release      )  
	EVT_MENU((int)CmdIds::Menu_ReleaseWin,  TabBar::OnMenu_RClick_Release      )
	EVT_MENU((int)CmdIds::Menu_CloseWin,    TabBar::OnMenu_RClick_CloseWin     )
	EVT_MENU((int)CmdIds::Menu_DettachWin,  TabBar::OnMenu_RClick_DettachWin   )
	EVT_MENU((int)CmdIds::Menu_SystemMenu,  TabBar::OnMenu_RClick_SystemMenu   )
END_EVENT_TABLE()

int TabBar::dbgCtr = 0;
int TabBar::instCtr = 0;

bool InCircle(float x, float y, float r)
{
	// Cheaper to square r than to square root the left side.
	return x * x + y * y <= r * r;
}

void CalculateCloseButtonInfo(const wxRect& wxrTab, const LProps& lp, int& outRad, wxPoint& outCenter)
{
	// These may eventually get put into LProps
	const int closeButtonRad = 8;
	const int closeRightPad = 2;

	outCenter = 
		wxPoint(
			wxrTab.x + wxrTab.width - closeButtonRad - closeRightPad, 
			wxrTab.height / 2 + lp.tabPadTop);

	// A simple transfer, but that allows this function to be 
	// authority on the button radius.
	outRad = closeButtonRad;
}

bool InCloseButton(TabBar* tb, Node* n, const wxPoint mousePt)
{
	int closeButtonRad;
	wxPoint closeBtnCenter;
	CalculateCloseButtonInfo( n->cachedTabLcl, tb->owner->GetLayoutProps(), closeButtonRad, closeBtnCenter);
	return InCircle(mousePt.x - closeBtnCenter.x, mousePt.y - closeBtnCenter.y, closeButtonRad);
}

TabBar::TabBar(DockWin* win, Node* node)
	: wxWindow(win, wxID_ANY, wxDefaultPosition, wxDefaultSize, WS_EX_COMPOSITED)
{
	++instCtr;
	this->id = ++dbgCtr;

	this->owner = win;
	this->node = node;
	this->SetBackgroundStyle(wxBackgroundStyle::wxBG_STYLE_PAINT);
}

TabBar::~TabBar()
{
	--instCtr;
	assert(instCtr >= 0);
}

void TabBar::SwapOwner(DockWin* win)
{
	this->Reparent(win);
	this->owner = win;
	this->Show(true);
}

Node* TabBar::GetTabAtPoint(const wxPoint& pt)
{
	if(this->node->type == Node::Type::Window)
	{
		if(this->node->cachedTabLcl.Contains(pt) == true)
			return this->node;
	}
	else if(this->node->type == Node::Type::Tabs)
	{
		for(Node* childWin : this->node->children)
		{
			if(childWin->cachedTabLcl.Contains(pt) == true)
				return childWin;
		}
	}
	return nullptr;
}

void TabBar::ClearHover()
{
	this->tabHoveringOver = nullptr;
	this->hoveringOverClose = false;
	this->UnsetToolTip();
	this->Refresh(false);
}

bool TabBar::UpdateMouseOver(const wxPoint& mousePt)
{
	Node * pnTabOver = this->GetTabAtPoint(mousePt);

	// If the tab being hovered over has changed
	bool tabHoveredChanged = false;
	// If something has changed to where a redraw is necessary.
	// This is basically (tabHoveredChanged || (Mouse over close button changed))
	bool redraw = false;

	// Check if hovering over a different tab
	// This includes 
	// - null->something or 
	// - something->null
	//
	// If it has, we need to change the tooltip and tab background colors.
	if(pnTabOver != this->tabHoveringOver)
	{
		redraw = true;
		tabHoveredChanged = true;
		this->hoveringOverClose = false; // We will recheck this
		this->tabHoveringOver = pnTabOver;
	}

	// Check the state of the mouse being over a close button has changed.
	//
	// If it has, we need to redraw to change the close button colors.
	if(this->tabHoveringOver != nullptr)
	{
		bool oldHoverClose = this->hoveringOverClose;
		this->hoveringOverClose = InCloseButton(this, this->tabHoveringOver, mousePt);

		if(oldHoverClose != this->hoveringOverClose)
			redraw = true;
	}
	else
		this->hoveringOverClose = false;

	if(redraw)
		this->Refresh(false);

	// May as well handle this while we're at it because it's a good
	// bottlenecked position for this logic.
	if(tabHoveredChanged)
	{
		if(this->tabHoveringOver != nullptr)
		{
			// > ☐ TABS_TLTP_a82959419809: Tabs can be hovered over to show tooptips.
			// > ☐ TABS_TLTP_c564222ff91b: Tooltips are relevant to the application hovered over.

			wxToolTip* tooltip = 
				new wxToolTip(
					this, 
					-1, 
					this->tabHoveringOver->cmdLine,
					this->tabHoveringOver->cachedTab);

			this->SetToolTip(tooltip);
		}
		else
			this->UnsetToolTip();
	}

	return tabHoveredChanged;
}

void TabBar::OnWindowTorn()
{
	assert(this->node->type == Node::Type::Window);
	this->ClearHover();
}

void TabBar::OnTabTorn(Node* nodeTorn)
{
	assert(this->node->type == Node::Type::Tabs);
	assert(nodeTorn != nullptr);

	if(this->tabHoveringOver == nodeTorn)
		this->ClearHover();
}

/// <summary>
/// Generate points, local to where a node region is drawn, for tabs.
/// </summary>
/// <param name="drawBot">
/// If true, generate points for the bottom of a selected tab.
/// </param>
/// <param name="tabRect">The rectangle for tab.</param>
/// <param name="tabX">The X offset to generate the points at</param>
/// <param name="barRgnWidth"></param>
/// <param name="lp"></param>
/// <returns></returns>
std::vector<wxPoint> GenerateTabPoints(
	bool drawBot, 
	wxSize tabSz, 
	int tabX, 
	int barRgnWidth, 
	const LProps& lp)
{
	const float myPI = 3.14159f;

	int tabWidth = tabSz.x;
	int tabHeight = tabSz.y;

	const int tabBaseline = tabHeight;
	const int tabPadShank = tabHeight - lp.bevelRad;

	std::vector<wxPoint> tabPts = 
	{
		wxPoint(tabX, tabBaseline - 0),
		wxPoint(tabX, tabBaseline - tabPadShank),
		wxPoint(tabX + lp.bevelRad + (int)(cos(myPI * 7.0f / 8.0f) * lp.bevelRad),   tabBaseline - (tabPadShank + (int)(sin(myPI * 7.0f / 8.0f) * lp.bevelRad))),
		wxPoint(tabX + lp.bevelRad + (int)(cos(myPI * 6.0f / 8.0f) * lp.bevelRad),   tabBaseline - (tabPadShank + (int)(sin(myPI * 6.0f / 8.0f) * lp.bevelRad))),
		wxPoint(tabX + lp.bevelRad + (int)(cos(myPI * 5.0f / 8.0f) * lp.bevelRad),   tabBaseline - (tabPadShank + (int)(sin(myPI * 5.0f / 8.0f) * lp.bevelRad))),
		wxPoint(tabX + lp.bevelRad + (int)(cos(myPI * 4.0f / 8.0f) * lp.bevelRad),   tabBaseline - (tabPadShank + (int)(sin(myPI * 4.0f / 8.0f) * lp.bevelRad))),
		//
		wxPoint(tabX + tabWidth - lp.bevelRad + (int)(cos(myPI * 4.0f / 8.0f) * lp.bevelRad),  tabBaseline - (tabPadShank + (int)(sin(myPI * 4.0f / 8.0f) * lp.bevelRad))),
		wxPoint(tabX + tabWidth - lp.bevelRad + (int)(cos(myPI * 3.0f / 8.0f) * lp.bevelRad),  tabBaseline - (tabPadShank + (int)(sin(myPI * 3.0f / 8.0f) * lp.bevelRad))),
		wxPoint(tabX + tabWidth - lp.bevelRad + (int)(cos(myPI * 2.0f / 8.0f) * lp.bevelRad),  tabBaseline - (tabPadShank + (int)(sin(myPI * 2.0f / 8.0f) * lp.bevelRad))),
		wxPoint(tabX + tabWidth - lp.bevelRad + (int)(cos(myPI * 1.0f / 8.0f) * lp.bevelRad),  tabBaseline - (tabPadShank + (int)(sin(myPI * 1.0f / 8.0f) * lp.bevelRad))),
		wxPoint(tabX + tabWidth, tabBaseline - tabPadShank),
		wxPoint(tabX + tabWidth, tabBaseline - 0),
	};

	if(drawBot)
	{
		tabPts.push_back(wxPoint(barRgnWidth,  tabBaseline));
		tabPts.push_back(wxPoint(barRgnWidth,  tabBaseline + lp.tabPadBot));
		tabPts.push_back(wxPoint(0,            tabBaseline + lp.tabPadBot));
		tabPts.push_back(wxPoint(0,            tabBaseline));
	}

	return tabPts;
}

void DrawTabIcon(HDC hdc, HWND hwnd, const wxRect& wxrTab, const LProps& lp)
{
	if(hwnd == NULL)
		return;

	HICON winIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON /* or GCLP_HICONSM */);
	if(winIcon != NULL)
	{
		// The width and height of the square region to draw the icon.
		const int iconDim = 16;

		DrawIconEx(
			hdc,
			wxrTab.x + 2,
			wxrTab.y + (wxrTab.height - iconDim) / 2,
			winIcon,
			iconDim,
			iconDim,
			0,
			NULL,
			DI_NORMAL);
	}
}

void DrawCloseButton(wxDC& dc, const wxRect& wxrTab, const wxBrush& bgColor, const LProps& lp)
{
	int closeButtonRad;
	wxPoint closeBtnCenter;
	CalculateCloseButtonInfo(wxrTab, lp, closeButtonRad, closeBtnCenter);

	// Draw the circle for the close button
	dc.SetBrush(bgColor);
	dc.DrawCircle(closeBtnCenter.x, closeBtnCenter.y, closeButtonRad);

	// Draw the X image for the close button
	const wxBitmap& closeBtnBmp = TabBar::GetCloseBtnBitmap();
	dc.DrawBitmap(
		closeBtnBmp, 
		wxPoint(
			closeBtnCenter.x - closeBtnBmp.GetWidth() / 2,
			closeBtnCenter.y - closeBtnBmp.GetHeight() / 2), 
		true);
}

void DrawTab(
	wxDC& dc, 
	Node* node, 
	const LProps& lp, 
	bool isSelectedTab, 
	bool hoveringOverTab, 
	bool hoveringOverCloseBtn,
	bool leftBtnDown)
{
	const int iconPad = 20;
	// > ☐ TABS_DSPLY_41f7d1875e9e: Tabs to display the application name.
	// > ☐ TABS_DSPLY_8890a1b90661: Tabs to emphasized what the selected tab is.
	// > ☐ TABS_DSPLY_11b7985c4553: Tabs to change colors when their mouse-over state changes.
	// > ☐ TABS_DSPLY_6e357d1e5e9c: Tabs to draw a close button to the right.
	// > ☐ TABS_DSPLY_52d8388e8c21: Tabs close button to change colors when its mouse hover state changes.
	// > ☐ TABS_DSPLY_8cc1ac9f23a3: Tabs to show the application's icon.


	static wxBrush brushSel				= wxBrush(wxColour(255, 255, 255));	// The color of selected tabs
	static wxBrush brushSelHover		= wxBrush(wxColour(240, 240, 255));
	static wxBrush brushUnsel			= wxBrush(wxColour(180, 180, 180));	// The color of unselected tabs
	static wxBrush brushUnselHover		= wxBrush(wxColour(170, 170, 200));
	static wxBrush brushSelMouseDown	= wxBrush(wxColour(150, 150, 255));

	static wxBrush closeBtnBrushNorm	= wxBrush(wxColour(220, 220, 220));	
	static wxBrush closeBtnBrushHover	= wxBrush(wxColour(230, 210, 210));
	static wxBrush closeBtnBrushDown	= wxBrush(wxColour(255, 200, 200));

	const wxRect& tabRect = node->cachedTabLcl;

	// Draw the body of the tab
	std::vector<wxPoint> tabPts = 
		GenerateTabPoints(
			isSelectedTab, 
			node->cachedTab.GetSize(), 
			tabRect.x,
			node->cacheSize.x,
			lp);

	if(leftBtnDown && !hoveringOverCloseBtn && hoveringOverTab)
		dc.SetBrush(brushSelMouseDown);
	else if(isSelectedTab)
	{
		if(hoveringOverTab)
			dc.SetBrush(brushSelHover);
		else
			dc.SetBrush(brushSel);
	}
	else
		dc.SetBrush(hoveringOverTab ? brushUnselHover : brushUnsel);

	dc.DrawPolygon(tabPts.size(), &tabPts[0], 0, 0);


	// Draw the internals
	wxCoord textHgt = dc.GetCharHeight();

	const char* szTitle = "Massive headphones";
#define DEBUG_TITLES 1
#if DEBUG_TITLES
	std::string debugTitle = std::string("DBG: ") + std::to_string(node->id);
	szTitle = debugTitle.c_str();
#endif

	dc.DrawText(
		szTitle, 
		wxPoint(
			tabRect.x + iconPad, 
			tabRect.y + (tabRect.height - textHgt) * 0.5f));

	DrawTabIcon(dc.GetTempHDC().GetHDC(), node->win, node->cachedTabLcl, lp);

	wxBrush* closeBrush = &closeBtnBrushNorm;
	if(hoveringOverCloseBtn && hoveringOverTab)
	{
		if(leftBtnDown)
			closeBrush = &closeBtnBrushDown;
		else
			closeBrush = &closeBtnBrushHover;
	}
	DrawCloseButton(dc, tabRect, *closeBrush, lp);
}

void TabBar::OnDraw(wxPaintEvent& evt)
{
	assert(this->node != nullptr);
	assert(this->owner != nullptr);

	wxBufferedPaintDC dc(this);
	wxSize szClient = this->GetClientSize();

	// Background of the entire tab area
	static wxBrush brushBG     = wxBrush(wxColour(200, 200, 240));	
	dc.SetBrush(brushBG);
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(wxPoint(), szClient);

	const LProps& lp = this->owner->GetLayoutProps();

	// > ☐ TABS_DSPLY_37ab301a73f2: Windows in the layout show the tab region.
	// > ☐ TABS_DSPLY_400ecb65205b: Tab collections in the layout show all tabs in a notebook tab UI.
	// > ☐ TABS_DSPLY_e8fc077e8376: Tab collections to show the correct entries.
	// > ☐ TABS_DSPLY_8ee9a555cc7c: Tab collections to show which is the active tab displayed.

	if(this->node->type == Node::Type::Window)
	{
		bool isHovering = (this->node == this->tabHoveringOver);
		DrawTab(
			dc, 
			this->node, 
			lp, 
			true, 
			isHovering, 
			this->hoveringOverClose, 
			DockWin::dragggingMgr != nullptr);
	}
	else if(this->node->type == Node::Type::Tabs)
	{
		for(size_t i = 0; i < this->node->children.size(); ++i)
		{
			Node* tinner = this->node->children[i];
			bool isSelected = (this->node->selTab == i);
			bool isHovering = (tinner == this->tabHoveringOver);
			DrawTab(
				dc, 
				tinner, 
				lp, 
				isSelected, 
				isHovering, 
				this->hoveringOverClose,
				DockWin::dragggingMgr != nullptr);
		}
	}
	else
		assert(!"Unhandled node type for TabBar::OnDraw");

}

void TabBar::OnMouseLDown(wxMouseEvent& evt)
{
	// > ☐ TABS_CTRL_6c3f9ca8d8df: Tabs close button closes window.
	// > ☐ TABS_CTRL_6c1f2893152f: Tabs can be clicked to initiate dragging the tab.
	// > ☐ TABS_CTRL_b3631964179a: Tabs in the notebook tab UI can be clicked to toggle the active tab.

	if(this->node->type == Node::Type::Window)
	{
		if(this->node->cachedTabLcl.Contains(evt.GetPosition()) == true)
		{
			bool closePress = InCloseButton(this, this->node, evt.GetPosition());
			this->owner->TabClickStart(this, this->node, this->node, closePress);
		}
	}
	else if(this->node->type == Node::Type::Tabs)
	{
		for(size_t i = 0; i < this->node->children.size(); ++i)
		{
			Node* pn = this->node->children[i];
			if(pn->cachedTabLcl.Contains(evt.GetPosition()) == true)
			{ 
				bool closePress = InCloseButton(this, pn, evt.GetPosition());
				this->owner->TabClickStart(this, pn, this->node, closePress);
				break;
			}

		}
	}

	this->Refresh(false);
}

void TabBar::OnMouseLUp(wxMouseEvent& evt)
{
	if(this->HasCapture() == true)
	{
		assert(DockWin::dragggingMgr != nullptr);
		this->owner->TabClickEnd();
	}
}

void TabBar::OnMouseMotion(wxMouseEvent& evt)
{
	if(this->HasCapture() == true)
		this->owner->TabClickMotion();

	if(this->UpdateMouseOver(evt.GetPosition()))
	{
		if(this->tabHoveringOver != nullptr)
			this->SetToolTip(this->tabHoveringOver->cmdLine);
		else
			this->SetToolTip("");
	}
}

void TabBar::OnMouseRDown(wxMouseEvent& evt)
{
	if(this->HasCapture() == true)
		return;

	this->nodeRightClicked = nullptr;


	wxPoint mousePt = evt.GetPosition();
	this->nodeRightClicked = this->GetTabAtPoint(mousePt);


	if(this->nodeRightClicked == nullptr)
		return;

	// > ☐ TABS_RMENU_09b5a49bfc1d: Tabs can be right clicked to bring up a dropdown menu.
	// > ☐ TABS_RMENU_bfa93a347a32: Tabs dropdown menu references the tab that was right clicked to invoke the menu.
	// > ☐ TABS_RMENU_5d0bfc4f7500: Tabs dropdown menu has a "Release" option
	// > ☐ TABS_RMENU_cba5c12ace6b: Tabs dropdown menu "Release" option properly releases the window.
	// > ☐ TABS_RMENU_4e986e5e95d0: Tabs dropdown menu has a "Dettach" option.
	// > ☐ TABS_RMENU_449a32913b62: Tabs dropdown menu "Dettach" option properly detaches the window.
	// > ☐ TABS_RMENU_80f542c9cbc4: Tabs dropdown menu has a "Close" option
	// > ☐ TABS_RMENU_80f542c9cbc4: Tabs dropdown menu "Close" option closes the tab properly

	wxMenu tabPopupMenu;
	//
	tabPopupMenu.Append((int)CmdIds::Menu_CloneWin,     "Clone"     );
	tabPopupMenu.Append((int)CmdIds::Menu_RenameWin,    "Rename Window");
	tabPopupMenu.Append((int)CmdIds::Menu_SystemMenu,   "System Menu");
	tabPopupMenu.AppendSeparator();
	tabPopupMenu.Append((int)CmdIds::Menu_ReleaseWin,   "Release"   );
	tabPopupMenu.Append((int)CmdIds::Menu_DettachWin,   "Dettach"   );
	tabPopupMenu.AppendSeparator();
	tabPopupMenu.Append((int)CmdIds::Menu_CloseWin,     "Close"     );
	//
	this->PopupMenu(&tabPopupMenu);
}

void TabBar::OnSize(wxSizeEvent& evt)
{
	this->Refresh();
}

void TabBar::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
}

void TabBar::OnMouseChanged(wxMouseCaptureChangedEvent& evt)
{	
	// This can be nullptr if we right click the tab and bring up the system menu.
	if(this->owner->dragggingMgr != nullptr)
	{ 
		if(!this->owner->dragggingMgr->dragFlaggedAsFinished)
			this->owner->dragggingMgr->CancelTabDragging();
	}
}

void TabBar::OnMouseEnter(wxMouseEvent& evt)
{
	assert(this->node != nullptr);

	if(this->node->type != Node::Type::Tabs)
		return;

	this->UpdateMouseOver(evt.GetPosition());
}

void TabBar::OnMouseExit(wxMouseEvent& evt)
{
	assert(this->node != nullptr);

	if(this->tabHoveringOver != nullptr)
		this->ClearHover();
}

void TabBar::OnMenu_RClick_Clone(wxCommandEvent& evt)
{
	if(this->nodeRightClicked == nullptr)
		return;

	this->owner->CloneNodeWin(this->nodeRightClicked);
}

void TabBar::OnMenu_RClick_Rename(wxCommandEvent& evt)
{
	if(this->nodeRightClicked == nullptr)
		return;

}

void TabBar::OnMenu_RClick_Release(wxCommandEvent& evt)
{
	if(this->nodeRightClicked == nullptr)
		return;

	this->owner->ReleaseNodeWin(this->nodeRightClicked);
}

void TabBar::OnMenu_RClick_CloseWin(wxCommandEvent& evt)
{
	if(this->nodeRightClicked == nullptr)
		return;

	this->owner->CloseNodeWin(this->nodeRightClicked);
}

void TabBar::OnMenu_RClick_DettachWin(wxCommandEvent& evt)
{
	if(this->nodeRightClicked == nullptr)
		return;

	this->owner->DettachNodeWin(this->nodeRightClicked);
}

void TabBar::OnMenu_RClick_SystemMenu(wxCommandEvent& evt)
{
	if(this->nodeRightClicked == nullptr)
		return;

	HWND hwnd = this->nodeRightClicked->Hwnd();
	if(hwnd == NULL)
		return;

	HMENU hMenuSys = GetSystemMenu(hwnd, FALSE);
	static const UINT menuFlags = TPM_LEFTALIGN|TPM_TOPALIGN;

	// GetSystemMenu doesn't work on all windows, best guess is
	// the function only gives a menu if it has a non-default
	// system menu.
	if(hMenuSys == NULL)
		AppDock::RaiseTODO("Missing System menu, perform fallback menu");

	wxPoint tabCornerOnScr = 
		this->ClientToScreen(
			this->nodeRightClicked->cachedTabLcl.GetPosition());

	BOOL popup = 
		TrackPopupMenu(
			hMenuSys,
			menuFlags,
			tabCornerOnScr.x,
			tabCornerOnScr.y,
			0,
			this->GetHWND(),
			NULL);

	// We're not allowed to open the menu on the actual HWND, probably because
	// it doesn't have a titlebar to display it at. 
	// What we need to do instead is intercept the message on a dummy window, and
	// redirect it via SendMessage to the window. Pretty sure that would work.
	AppDock::RaiseTODO("Create delegation system of message to HWND");
}

const wxBitmap& TabBar::GetCloseBtnBitmap()
{
	static wxBitmap closeBtnBitmap = wxBitmap(pszTabCloseBtn);
	return closeBtnBitmap;
}

bool TabBar::_TestValidity()
{
	return true;
}