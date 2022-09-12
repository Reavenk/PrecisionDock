#include "TabBar.h"
#include "../DockWin.h"
#include "../AppDock.h"
#include <wx/dcbuffer.h>

#include "TabCloseBtn.xpm"

BEGIN_EVENT_TABLE(TabBar, wxWindow)
	EVT_PAINT					(TabBar::OnDraw			)
	EVT_LEFT_DOWN				(TabBar::OnMouseLDown	)
	EVT_LEFT_UP					(TabBar::OnMouseLUp		)
	EVT_MOTION					(TabBar::OnMouseMotion	)
	EVT_RIGHT_DOWN				(TabBar::OnMouseRDown	)
	EVT_SIZE					(TabBar::OnSize			)
	EVT_MOUSE_CAPTURE_CHANGED	(TabBar::OnMouseChanged	)
	EVT_MOUSE_CAPTURE_LOST		(TabBar::OnMouseCaptureLost)

	EVT_MENU((int)CmdIds::Menu_CloneWin,    TabBar::OnMenu_RClick_Clone        )
	EVT_MENU((int)CmdIds::Menu_RenameWin,   TabBar::OnMenu_RClick_Release      )  
	EVT_MENU((int)CmdIds::Menu_ReleaseWin,  TabBar::OnMenu_RClick_Release      )
	EVT_MENU((int)CmdIds::Menu_CloseWin,    TabBar::OnMenu_RClick_CloseWin     )
	EVT_MENU((int)CmdIds::Menu_DettachWin,  TabBar::OnMenu_RClick_DettachWin   )
	EVT_MENU((int)CmdIds::Menu_SystemMenu,  TabBar::OnMenu_RClick_SystemMenu   )
END_EVENT_TABLE()

TabBar::TabBar(DockWin* win, Node* node)
	: wxWindow(win, wxID_ANY, wxDefaultPosition, wxDefaultSize, WS_EX_COMPOSITED)
{
	this->owner = win;
	this->node = node;
	this->SetBackgroundStyle(wxBackgroundStyle::wxBG_STYLE_PAINT);
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

void DrawCloseButton(wxDC& dc, const wxRect& wxrTab, const LProps& lp)
{
	int closeButtonRad;
	wxPoint closeBtnCenter;
	CalculateCloseButtonInfo(wxrTab, lp, closeButtonRad, closeBtnCenter);

	// Draw the circle for the close button
	static wxBrush brushBtn = wxBrush(wxColour(220, 220, 220));	
	dc.SetBrush(brushBtn);
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

void DrawTab(wxDC& dc, Node* node, const LProps& lp, bool isSelected)
{
	const int iconPad = 20;

	static wxBrush brushSel    = wxBrush(wxColour(255, 255, 255));	// The color of selected tabs
	static wxBrush brushUnsel  = wxBrush(wxColour(180, 180, 180));	// The color of unselected tabs

	const wxRect& tabRect = node->cachedTabLcl;

	// Draw the body of the tab
	std::vector<wxPoint> tabPts = 
		GenerateTabPoints(
			isSelected, 
			node->cachedTab.GetSize(), 
			tabRect.x,
			node->cacheSize.x,
			lp);

	dc.SetBrush(isSelected ? brushSel : brushUnsel);
	dc.DrawPolygon(tabPts.size(), &tabPts[0], 0, 0);


	// Draw the internals
	wxCoord textHgt = dc.GetCharHeight();
	dc.DrawText(
		"Massive headphones", 
		wxPoint(
			tabRect.x + iconPad, 
			tabRect.y + (tabRect.height - textHgt) * 0.5f));

	DrawTabIcon(dc.GetTempHDC().GetHDC(), node->win, node->cachedTabLcl, lp);
	DrawCloseButton(dc, tabRect, lp);
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

	if(this->node->type == Node::Type::Window)
	{
		DrawTab(dc, this->node, lp, true);
	}
	else if(this->node->type == Node::Type::Tabs)
	{
		for(size_t i = 0; i < this->node->children.size(); ++i)
		{
			Node* tinner = this->node->children[i];
			bool isSelected = (this->node->selTab == i);
			DrawTab(dc, tinner, lp, isSelected);
		}
	}
	else
		assert(!"Unhandled node type for TabBar::OnDraw");

}

void TabBar::OnMouseLDown(wxMouseEvent& evt)
{
	if(this->node->type == Node::Type::Window)
	{
		if(this->node->cachedTabLcl.Contains(evt.GetPosition()) == true)
			this->owner->TabClickStart(this, this->node, this->node);
	}
	else if(this->node->type == Node::Type::Tabs)
	{
		for(size_t i = 0; i < this->node->children.size(); ++i)
		{
			Node* pn = this->node->children[i];
			if(pn->cachedTabLcl.Contains(evt.GetPosition()) == true)
			{ 
				this->owner->TabClickStart(this, pn, this->node);
				break;
			}

		}
	}
}

void TabBar::OnMouseLUp(wxMouseEvent& evt)
{
	if(this->HasCapture() == true)
	{
		this->ReleaseMouse();
		this->owner->TabClickEnd();
	}
}

void TabBar::OnMouseMotion(wxMouseEvent& evt)
{
	if(this->HasCapture() == true)
		this->owner->TabClickMotion();

	Node * pnTabOver = this->GetTabAtPoint(evt.GetPosition());

	if(pnTabOver != nullptr)
		this->SetToolTip(pnTabOver->cmdLine);
	else
		this->SetToolTip("");
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
	this->owner->TabClickEnd();
}

void TabBar::OnMouseChanged(wxMouseCaptureChangedEvent& evt)
{
	if(this->HasCapture() == true)
		this->ReleaseMouse();
	
	this->owner->TabClickEnd();
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