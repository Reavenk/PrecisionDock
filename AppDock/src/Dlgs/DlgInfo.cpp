#include "DlgInfo.h"
#include "../Utils/AppUtils.h"
#include "../AppDock.h"
#include "../DrawUtils.h"
#include <wx/statline.h>

wxDEFINE_EVENT(TAB_CLICK_EVENT, wxCommandEvent);

wxBEGIN_EVENT_TABLE(DlgInfoTabs, wxWindow)
	EVT_MOTION(			DlgInfoTabs::OnMotion		)
	EVT_LEFT_DOWN(		DlgInfoTabs::OnMouseLeftDown)
	EVT_LEFT_UP(		DlgInfoTabs::OnMouseLeftUp	)
	EVT_ENTER_WINDOW(	DlgInfoTabs::OnMouseEnter	)
	EVT_LEAVE_WINDOW(	DlgInfoTabs::OnMouseLeave	)
	EVT_PAINT(			DlgInfoTabs::OnDraw			)
wxEND_EVENT_TABLE()



DlgInfoTabs::DlgInfoTabs(wxWindow* parent, int id, const wxPoint& pt, const wxSize& sz)
	: wxWindow(parent, id, pt, sz)
{
	this->SetMinSize(wxSize(-1, tabHeight + tabBottom));
	
}

bool DlgInfoTabs::_IsIndexValid(int idx)
{
	return idx >= 0 && idx < this->entries.size();
}

int DlgInfoTabs::IndexOver(const wxPoint& pt)
{
	for (int i = 0; i < this->entries.size(); ++i)
	{
		wxRect r(wxPoint(i * tabWidth, 0), wxSize(tabWidth, tabHeight));
		if (r.Contains(pt))
			return i;
	}
	return -1;
}

int DlgInfoTabs::AddEntry(int id, const wxString& label)
{
	int ret = (int)this->entries.size();
	this->entries.push_back({ id, label });
	return ret;
}

bool DlgInfoTabs::RemoveIndex(int idxToRem)
{
	if (!this->_IsIndexValid(idxToRem))
		return false;

	this->entries.erase(this->entries.begin() + idxToRem);
	this->Refresh(false);
	return true;
}

void DlgInfoTabs::SelectId(int id)
{
	for (int i = 0; i < this->entries.size(); ++i)
	{
		if (this->entries[i].id == id)
		{
			this->SelectIndex(i);
			return;
		}
	}
}

void DlgInfoTabs::SelectIndex(int idx)
{
	if (!this->_IsIndexValid(idx))
		return;

	this->selIdx = idx;
	this->Refresh(false);

	if (this->_IsIndexValid(this->selIdx))
	{
		wxCommandEvent evt(TAB_CLICK_EVENT, this->entries[idx].id);
		evt.Skip(true);
		wxPostEvent(this, evt);
	}
}

bool DlgInfoTabs::RemoveEntry(int idToRem)
{
	for (int i = 0; i < this->entries.size(); ++i)
	{
		if (this->entries[i].id == idToRem)
			return this->RemoveIndex(i);
	}
	return false;
}

void DlgInfoTabs::OnMotion(wxMouseEvent& evt)
{
	int oldHover = this->hoverIdx;
	this->hoverIdx = this->IndexOver(evt.GetPosition());
	if (this->hoverIdx != oldHover)
		this->Refresh(false);
}

void DlgInfoTabs::OnMouseLeftDown(wxMouseEvent& evt)
{
	this->hoverIdx = this->IndexOver(evt.GetPosition());

	// The clicked tab is only a consideration if we're not clicking
	// on what's already the selected tab.
	if(this->hoverIdx != -1 && this->selIdx != this->hoverIdx)
		this->clickCanidate = this->hoverIdx;

	this->Refresh(false);
}

void DlgInfoTabs::OnMouseLeftUp(wxMouseEvent& evt)
{
	int oldHover = this->hoverIdx;
	this->hoverIdx = this->IndexOver(evt.GetPosition());

	if (this->clickCanidate != -1 && this->hoverIdx == this->clickCanidate)
		this->SelectIndex(this->clickCanidate);

	this->clickCanidate = -1;
	this->Refresh(false);

}

void DlgInfoTabs::OnMouseEnter(wxMouseEvent& evt)
{
	this->hoverIdx = this->IndexOver(evt.GetPosition());
	if (this->hoverIdx != -1)
		this->Refresh(false);
}

void DlgInfoTabs::OnMouseLeave(wxMouseEvent& evt)
{
	if (this->hoverIdx != -1)
	{
		this->hoverIdx = -1;
		this->Refresh(false);
	}	
}

void DlgInfoTabs::OnDraw(wxPaintEvent& evt)
{
	wxPaintDC dc(this);
	if (this->entries.empty())
		return;


	dc.SetPen(*wxTRANSPARENT_PEN);
	wxSize clientSz = this->GetClientSize();

	static wxBrush bgBrush(wxColour(100, 100, 100));
	dc.SetBrush(bgBrush);
	dc.DrawRectangle(wxRect(0, 0, clientSz.x, clientSz.y));

	for (int i = 0; i < this->entries.size(); ++i)
	{
		std::vector<wxPoint> tabPts =
			GenerateTabPoints(
				i == selIdx,
				tabBottom,
				wxSize(tabWidth, tabHeight),
				tabWidth * i,
				clientSz.x,
				10);

		const wxString& label = this->entries[i].label;
		wxSize labelExtent = dc.GetTextExtent(label);

		if (i == this->selIdx)
		{
			if (i == this->hoverIdx)
				dc.SetBrush(*wxWHITE_BRUSH);
			else
			{
				static wxBrush bgSelHover(wxColour(245, 245, 245));
				dc.SetBrush(bgSelHover);
			}
		}
		else
		{
			if (i == this->hoverIdx)
			{
				static wxBrush bgUnsel(wxColour(200, 200, 200));
				dc.SetBrush(bgUnsel);
			}
			else
			{
				static wxBrush bgUnselHover(wxColour(185, 185, 185));
				dc.SetBrush(bgUnselHover);
			}
		}

		dc.DrawPolygon(tabPts.size(), &tabPts[0], 0, 0);
		dc.DrawText(
			label,
			tabWidth * i + (this->tabWidth - labelExtent.x) / 2,
			(this->tabHeight - labelExtent.y) / 2);
	}
}

// Currently just a placeholder.

wxBEGIN_EVENT_TABLE(DlgInfo, wxDialog)
	EVT_BUTTON(wxID_OK,						DlgInfo::OnButtonOK)
	EVT_BUTTON((int)CmdIDs::BtnBailIntro,	DlgInfo::OnButtonCancel)
	EVT_BUTTON((int)CmdIDs::BtnGotcha,		DlgInfo::OnButtonGotcha)
	wx__DECLARE_EVT1(TAB_CLICK_EVENT, (int)CmdIDs::TabAbout, &DlgInfo::OnTab_About)
	wx__DECLARE_EVT1(TAB_CLICK_EVENT, (int)CmdIDs::TabIntro, &DlgInfo::OnTab_Intro)
	EVT_CLOSE(DlgInfo::OnDlgClose)
wxEND_EVENT_TABLE()

DlgInfo::DlgInfo(int createFlags, int startingIndex)
	:	wxDialog(nullptr, wxID_ANY,"About", wxDefaultPosition, wxSize(400, 400))
{
	// > ☐ ABOUT_MAIN_a9e233bac5e8: About dialog shows the date the application was built.
	// > ☐ ABOUT_MAIN_c026ec87e65e: About dialog shows the application version.
	// > ☐ ABOUT_MAIN_5eb90c1bc078: Application's version is a semantic version.
	// > ☐ ABOUT_MAIN_897148b6d678: About's application version is correct.


	this->cachedHWND = this->GetHWND();
	AppDock::GetApp().RegisterToplevelOwned(this->cachedHWND);
	AppUtils::SetDefaultIcons(this);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(mainSizer);

	// Tab region
	//////////////////////////////////////////////////
	this->tabs = new DlgInfoTabs(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	mainSizer->Add(this->tabs, 0, wxGROW, 0);

	if((createFlags&CreateFlag::HasAbout) != 0)
		this->tabs->AddEntry((int)CmdIDs::TabAbout, "About");

	if((createFlags&CreateFlag::HasIntro) != 0)
		this->tabs->AddEntry((int)CmdIDs::TabIntro, "Intro");

	assert(this->tabs->TabCount() != 0);

	// Content Region
	//////////////////////////////////////////////////
	this->contentRegion = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	mainSizer->Add(this->contentRegion, 1, wxGROW, 5);
	
	// The bottom
	//////////////////////////////////////////////////
	mainSizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxGROW|wxALL, 5);
	wxBoxSizer* sizerBottomBtnRow = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(sizerBottomBtnRow, 0, wxALL | wxGROW, 5);

	if ((createFlags & CreateFlag::HasOKConfirm) != 0)
	{
		wxButton* okBtn = new wxButton(this, wxID_OK, "Close");
		sizerBottomBtnRow->Add(okBtn, 2, wxGROW, 0);
	}

	if ((createFlags & CreateFlag::HasGotchaConfirm) != 0)
	{
		wxButton* gotchaBtn = new wxButton(this, (int)CmdIDs::BtnGotcha, "Gotcha!");
		sizerBottomBtnRow->Add(gotchaBtn, 2, wxGROW, 0);
	}

	if ((createFlags & CreateFlag::HasCancelConfirm) != 0)
	{
		wxButton* gotchaBtn = new wxButton(this, (int)CmdIDs::BtnBailIntro, "Cancel");
		sizerBottomBtnRow->Add(gotchaBtn, 2, wxGROW, 5);
	}

	// Finish
	//////////////////////////////////////////////////
	this->tabs->SelectIndex(startingIndex);
}

DlgInfo::~DlgInfo()
{
	AppDock::GetApp().UnregisterToplevelOwned(this->cachedHWND);
}

void DlgInfo::SetTab(CmdIDs tmode, bool force)
{
	switch (tmode)
	{
	case CmdIDs::TabAbout:
		this->_AddContentsForAbout();
		break;

	case CmdIDs::TabIntro:
		this->_AddContentsForIntro();
		break;
	}
}


void InsertInfoToGrid(wxWindow* parent, wxFlexGridSizer* fgsInfoGrid, const char* label, const char* value)
{
	const int gridSpace = 10;

	fgsInfoGrid->Add(new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxST_NO_AUTORESIZE), wxGROW | wxALIGN_RIGHT);
	fgsInfoGrid->AddSpacer(gridSpace);
	fgsInfoGrid->Add(new wxStaticText(parent, wxID_ANY, value), wxALIGN_RIGHT);
}

void InsertHorizontalSplit(wxWindow* parent, wxBoxSizer* sizer)
{
	sizer->Add(new wxStaticLine(parent, wxID_ANY), 0, wxGROW|wxALL, 5);
}

void DlgInfo::_AddContentsForIntro()
{
	this->ClearContentRegion();

	wxBoxSizer* regionMainSizer = new wxBoxSizer(wxVERTICAL);
	this->contentRegion->SetSizer(regionMainSizer, true);

	wxStaticText* tabTitle = new wxStaticText(this->contentRegion, wxID_ANY, "Welcome to\nPrecisionDock");
	regionMainSizer->Add(tabTitle, 0, wxALIGN_CENTER_HORIZONTAL);
	tabTitle->SetFont(tabTitle->GetFont().Scale(2.0f));

	InsertHorizontalSplit(this->contentRegion, regionMainSizer);

	const char* warning = 
		"This application does wonky thing with Windows, use at your own risk. "
		"This is not only because of the application's current stability, but also"
		"because it allows you to do things that the OS windowing system wasn't "
		"designed to accomodate.\n\n"
		"If you are not comfortable with your windows randomly closing and behaving "
		"in unexpected ways, close this program and live your life without it.";

	wxStaticText* warningText = new wxStaticText(this->contentRegion, wxID_ANY, warning);
	regionMainSizer->Add(warningText, 0, wxALIGN_CENTER_HORIZONTAL);
	warningText->Wrap(this->GetClientSize().x - 10);

}

void DlgInfo::_AddContentsForAbout()
{
	this->ClearContentRegion();

	wxBoxSizer* regionMainSizer = new wxBoxSizer(wxVERTICAL);
	this->contentRegion->SetSizer(regionMainSizer, true);

	wxStaticText* appTitle = new wxStaticText(this->contentRegion, wxID_ANY, "PrecisionDock");
	regionMainSizer->Add(appTitle, 0, wxALIGN_CENTER_HORIZONTAL);
	appTitle->SetFont(appTitle->GetFont().Scale(3.0f));

	InsertHorizontalSplit(this->contentRegion, regionMainSizer);
	
	wxFlexGridSizer* dataGridSizer = new wxFlexGridSizer(3);
	dataGridSizer->AddGrowableCol(2);
	regionMainSizer->Add(dataGridSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	//
	InsertInfoToGrid(this->contentRegion, dataGridSizer, "Version",			"--");
	InsertInfoToGrid(this->contentRegion, dataGridSizer, "Build Commit",	"--");
	InsertInfoToGrid(this->contentRegion, dataGridSizer, "Build Date",		"--");

	InsertHorizontalSplit(this->contentRegion, regionMainSizer);

	wxStaticText* bottomText = new wxStaticText(this->contentRegion, wxID_ANY, "Pixel Precision 2022");
	regionMainSizer->Add(bottomText, 0, wxALL, 5);
}

void DlgInfo::ClearContentRegion()
{
	this->contentRegion->DestroyChildren();
}

void DlgInfo::OnButtonOK(wxCommandEvent& evt)
{
	this->EndModal(wxID_OK);
}

void DlgInfo::OnButtonCancel(wxCommandEvent& evt)
{
	AppDock::GetApp().Exit();
	this->EndModal(wxID_OK);
}

void DlgInfo::OnButtonGotcha(wxCommandEvent& evt)
{
	AppDock::ConfirmNotice();
	this->EndModal(wxID_OK);
}

void DlgInfo::OnTab_About(wxCommandEvent& evt)
{
	this->Freeze();
	{
		this->SetTab(CmdIDs::TabAbout);
		this->Layout();
	}
	this->Thaw();

	this->SetTitle("Application Info");
}

void DlgInfo::OnTab_Intro(wxCommandEvent& evt)
{
	this->Freeze();
	{
		this->SetTab(CmdIDs::TabIntro);
		this->Layout();
	}
	this->Thaw();

	this->SetTitle("Introduction");
}

void DlgInfo::OnDlgClose(wxCloseEvent& evt)
{
	// If they close the dialog without confirming the intro
	// agreement, they're out. There's no other non-intro case
	// where it shouldn't have been confirmed so we don't need
	// to be surgical in our checks of IsNoticeConfirmed().
	if (!AppDock::IsNoticeConfirmed())
		AppDock::GetApp().Exit();
}