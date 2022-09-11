#include "CaptureDlg.h"
#include "CapDragSelWin.h"
#include "CaptureListItem.h"
#include "../AppDock.h"
#include <wx/dcbuffer.h>
#include "../Utils/mywxUtils.h"
#include "../Utils/AppUtils.h"
#include "../resource.h"


CaptureDlg* CaptureDlg::inst = nullptr;


wxBitmap CaptureDlg::questionSmall;
wxBitmap CaptureDlg::questionLarge;



wxBEGIN_EVENT_TABLE(CaptureDlg, wxFrame)
	EVT_BUTTON(		(int)CMDID::Btn_Capture,		CaptureDlg::OnButton_Capture		)
	EVT_BUTTON(		(int)CMDID::Btn_CaptureAndClose,CaptureDlg::OnButton_CaptureCloseDlg)
	EVT_BUTTON(		(int)CMDID::Btn_ToggleHelp,		CaptureDlg::OnButton_ToggleHelpMode	)
	EVT_CHECKBOX(	(int)CMDID::CkB_AutoClose,		CaptureDlg::OnCheckbox_AutoClose	)
	EVT_CLOSE		(CaptureDlg::OnClose)
	EVT_SIZE		(CaptureDlg::OnSize	)	
wxEND_EVENT_TABLE()

CaptureDlg::CaptureDlg(wxWindow* parent, const wxPoint& pos, const wxSize& size)
	: wxFrame(parent, wxID_ANY, "AppDock Capture", pos, size)
{
	this->cachedHwnd = this->GetHWND();
	AppDock::GetApp().RegisterToplevelOwned(this->cachedHwnd);

	if(questionSmall.IsNull())
		questionSmall = mywxUtils::LoadFromResourceID(IDB_QUESTION_SMALL);

	if(questionLarge.IsNull())
		questionLarge = mywxUtils::LoadFromResourceID(IDB_QUESTION_LARGE);

	const int SectionSepHeight = 20;
	
	assert(inst == nullptr);
	inst = this;
	
	wxPanel* thisPanel = new wxPanel(this, wxID_ANY);
	wxBoxSizer * sizerMain = new wxBoxSizer(wxVERTICAL);
	thisPanel->SetSizer(sizerMain);

	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(rootSizer);
	rootSizer->Add(thisPanel, 1, wxEXPAND);
	
	this->processCont = new wxWindow(thisPanel, wxID_ANY);
	this->processCont->SetBackgroundColour(wxColour(200, 200, 255));
	wxBoxSizer* processSizer = new wxBoxSizer(wxHORIZONTAL);
	this->helpTextSizer = new wxBoxSizer(wxVERTICAL);
	
	this->helpBtn = 
		new wxButton(
			this->processCont, 
			(int)CMDID::Btn_ToggleHelp, 
			"", 
			wxDefaultPosition,
			szButtonsFull);

	this->helpBtn->SetToolTip("Click to toggle displaying help.");

	this->helpBtn->SetBitmap(questionLarge);

	this->dragTargMin = new CapDragSelWin(this->processCont, this);
	this->dragTargMin->SetMinSize(szButtonsCompact);
	this->dragTargMin->SetStyle( CapDragSelWin::Style::Small );
	this->dragTargMin->Hide();
	
	// The drag targ can placed in two places, depending on
	// the help mode. We'll default it to where the default 
	// help mode (on) would place it.
	this->dragTarg = new CapDragSelWin(thisPanel, this);
	this->dragTarg->SetMinSize(wxSize(-1, szButtonsFull.y));
	
	sizerMain->Add(this->processCont, 0, wxEXPAND);
	this->processCont->SetSizer(processSizer);
	processSizer->Add(this->dragTargMin, 0, wxALL, 5);
	processSizer->Add(this->helpTextSizer, 1, wxEXPAND);
	{
		wxStaticText* intrTitle = new wxStaticText(this->processCont, wxID_ANY, "Capture Window");
		intrTitle->SetFont(wxFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
		int ext_x, ext_y;
		intrTitle->GetTextExtent("Capture Window", &ext_x, &ext_y);
		this->helpTextSizer->Add(intrTitle, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 5);
		intrTitle->SetMaxSize(wxSize(-1, ext_y + 4));
	
		this->processHelpText = new wxStaticText(this->processCont, wxID_ANY, _GetHelpText());
		this->helpTextSizer->Add(this->processHelpText, 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
	}
	processSizer->Add(this->helpBtn, 0, wxALL, 5);
	
	this->helpModeSizerPadding.push_back(sizerMain->AddSpacer(SectionSepHeight));
	sizerMain->Add(this->dragTarg, 0, wxEXPAND|wxLEFT|wxRIGHT, 5);	
	
	this->scrollWin = new wxScrolledWindow(
		thisPanel, 
		wxID_ANY, 
		wxDefaultPosition, 
		wxDefaultSize, 
		wxScrolledWindowStyle|wxBORDER_SUNKEN);
	//
	this->helpModeSizerPadding.push_back(sizerMain->AddSpacer(SectionSepHeight));
	sizerMain->Add(this->scrollWin, 1, wxEXPAND|wxLEFT|wxRIGHT, 2);
	
	const int minButtonSz = 40;
	wxBoxSizer* confirmSizer = new wxBoxSizer(wxHORIZONTAL);
	sizerMain->Add(confirmSizer, 0, wxEXPAND);
	this->captureBtn = new wxButton(thisPanel, (int)CMDID::Btn_Capture, "Capture");
	this->captureBtn->SetMinSize(wxSize(-1, minButtonSz));
	confirmSizer->Add(this->captureBtn, 1, wxEXPAND|wxALL, 2);
	this->captureOKBtn = new wxButton(thisPanel, (int)CMDID::Btn_CaptureAndClose, "Capture &&& Close Dlg");
	this->captureOKBtn->SetMinSize(wxSize(-1, minButtonSz));
	confirmSizer->Add(this->captureOKBtn, 3, wxEXPAND|wxALL, 2);
	
	sizerMain->AddSpacer(SectionSepHeight);
	
	this->autoCloseCheck = new wxCheckBox(thisPanel, (int)CMDID::CkB_AutoClose, "Auto-Close on Capture");
	sizerMain->Add(this->autoCloseCheck, 0, wxEXPAND|wxLEFT|wxBOTTOM, 2);
	this->ToggleHelpMode(this->usingHelpMode);
	
	this->RebuildListed();
	
	this->scrollWin->SetScrollRate(10, 10);
	this->UpdateCaptureButtonsEnabled();

	AppUtils::SetDefaultIcons(this);
}

CaptureDlg::~CaptureDlg()
{
	assert(this == inst);
	inst = nullptr;

	AppDock::GetApp().UnregisterToplevelOwned(this->cachedHwnd);
}

void CaptureDlg::_EnsureScrollWinSizer()
{
	if(this->scrollWinSizer != nullptr)
		return;

	this->scrollWinSizer = new wxBoxSizer(wxVERTICAL);
	this->scrollWin->SetSizer(this->scrollWinSizer);
}

void CaptureDlg::_ClearScrollWinSizer()
{
	if(this->scrollWinSizer == nullptr)
		return;

	assert(this->listedItems.size() == 0);
	assert(this->setListed.size() == 0);

	delete this->scrollWinSizer;
	this->scrollWinSizer = nullptr;
	this->scrollWin->SetSizer(nullptr, false);
}

const char * CaptureDlg::_GetHelpText()
{
	static const char * helpString = 
		"This dialog allows adding windows to the AppDock. "
		"Either click and drag the target on to the window "
		"you wish to capture, or select a window below. \n\n"
		"Click on the question mark button to disable help text.";

	return helpString;
}

void CaptureDlg::ToggleHelpMode(bool showHelp, bool force)
{
	if(this->usingHelpMode == showHelp && force == false)
		return;

	this->usingHelpMode = showHelp;

	if(this->usingHelpMode == true)
	{
		this->helpBtn->SetMinSize(szButtonsFull);
		this->helpBtn->SetBitmap(questionLarge);
		this->processHelpText->Show(true);
		this->dragTarg->Show(true);
		this->dragTargMin->Show(false);

		if(this->helpTextSizer->GetItemCount() < 2)
			this->helpTextSizer->Add(this->processHelpText, 0, wxEXPAND|wxLEFT|wxRIGHT, 10);

		//if(this->sizerLocHelpText )
		//this->sizerLocHelpText
	}
	else
	{
		this->helpBtn->SetMinSize(szButtonsCompact);
		this->helpBtn->SetBitmap(questionSmall);
		this->processHelpText->Show(false);
		this->dragTarg->Show(false);
		this->dragTargMin->Show(true);

		if(this->helpTextSizer->GetItemCount() > 1)
			this->helpTextSizer->Remove(1);
	}

	for(wxSizerItem* s : this->helpModeSizerPadding)
		s->Show(showHelp);

	this->Layout();
}

void CaptureDlg::SetListSelectionAll(bool val, bool refreshDisplay)
{
	for(CaptureListItem* item : this->listedItems)
		item->isSel = val;

	if(refreshDisplay == true)
		this->UpdateListedSelectionColors();

}

void CaptureDlg::ClearListSelection(bool refreshDisplay)
{
	this->SetListSelectionAll(false, refreshDisplay);
}

int CaptureDlg::GetSelectionCount()
{
	return this->GetSelectedHWND().size();
}

std::vector<CaptureListItem*> CaptureDlg::GetSelected()
{
	std::vector<CaptureListItem*> ret;
	for(auto it : listedItems)
	{
		if(it->isSel == false)
			continue;

		ret.push_back(it);
	}

	return ret;
}

std::vector<HWND> CaptureDlg::GetSelectedHWND()
{
	std::vector<HWND> ret;
	for(auto it : listedItems)
	{
		if(it->isSel == false)
			continue;

		ret.push_back(it->captureHWND);
	}
	return ret;
}

int CaptureDlg::Capture(std::vector<HWND> toCapture)
{
	int ret = 0;

	std::set<HWND> setSuccessful;

	// Try to spawn all selected windows.
	for(size_t i = 0; i < toCapture.size(); ++i)
	{
		HWND toSpawn = toCapture[i];
		setSuccessful.insert(toSpawn);

		TopDockWin* twd = AppDock::GetApp().CreateWindowFromHwnd(toSpawn);
		if(twd != nullptr)
			setSuccessful.insert(toSpawn);
	}

	// Remove successfully captured windows from UI
	bool anyRm = false;
	for(size_t rmIdx = 0; rmIdx < this->listedItems.size(); )
	{
		CaptureListItem* cli = this->listedItems[rmIdx];
		if(setSuccessful.find(cli->captureHWND) == setSuccessful.end())
		{
			++rmIdx;
			continue;
		}

		this->listedItems.erase(this->listedItems.begin() + rmIdx);
		this->setListed.erase(cli->captureHWND);
		delete cli;
		anyRm = true;
		++ret;
	}

	if(anyRm == true)
		this->Layout();

	return ret;
}

void CaptureDlg::CaptureSelected()
{
	std::vector<HWND> vecHwnd = this->GetSelectedHWND();
	this->Capture(vecHwnd);
}

void CaptureDlg::RebuildListed()
{
	this->ClearListed();

	// https://gist.github.com/blewert/b6e7b11c565cf82e7d700c609f22d023

	this->_EnsureScrollWinSizer();

	std::vector<HWND> vecWins;
	HWND desktWin = GetDesktopWindow();
	for(
		HWND it = GetTopWindow(desktWin); 
		it != NULL; 
		it = GetWindow(it, GW_HWNDNEXT))
	{
		CaptureListItem* added = this->AddListItem(it);
	}

	this->scrollWin->Layout();
	this->UpdateListedSelectionColors();
	this->UpdateCaptureButtonsEnabled();
}

void CaptureDlg::ClearListed()
{
	// The children of the scroll win should match with
	// listedItems, but the UI API of scrollWin is the 
	// true authority of the UI we care about destroying.
	wxWindowList listWins = this->scrollWin->GetChildren();
	for(auto it : listWins)
		it->Destroy();
	
	listedItems.clear();
	setListed.clear();

	this->_ClearScrollWinSizer();
}

void CaptureDlg::UpdateListedSelectionColors()
{
	for(auto it : this->listedItems)
	{
		this->UpdateListColor(it);
	}
}

void CaptureDlg::UpdateListColor(CaptureListItem* cli, bool refresh)
{
	this->UpdateListColor(cli, cli->isSel, refresh);
}

void CaptureDlg::UpdateListColor(CaptureListItem* cli, bool sel, bool refresh)
{
	cli->UpdateBackgroundColor(refresh);
}

void CaptureDlg::UpdateCaptureButtonsEnabled()
{
	bool anySel = false;
	for(size_t i = 0; i < this->listedItems.size(); ++i)
	{
		if(this->listedItems[i]->isSel == true)
		{
			anySel =  true;
			break;
		}
	}

	this->captureBtn->Enable(anySel);
	this->captureOKBtn->Enable(anySel);
}

CaptureListItem* CaptureDlg::AddListItem(HWND topWin)
{
	if(topWin == NULL)
		return nullptr;

	if(IsWindowVisible(topWin) == FALSE)
		return nullptr;

	HICON icon = 
		(HICON)GetClassLongPtr(
			topWin, 
			GCLP_HICON /* or GCLP_HICONSM */);

	wxString title;
	int labelCt = GetWindowTextLength(topWin) + 1;
	if(labelCt != 0)
	{ 
		std::vector<WCHAR> label;
		label.resize(labelCt);
		GetWindowText(topWin, &label[0], labelCt);
		title = wxString(&label[0]);
	}
	if(CaptureListItem::IsValid(icon, title) == false)
		return nullptr;

	CaptureListItem* cli = 
		new CaptureListItem(
			this,
			this->scrollWin, 
			topWin, 
			icon, 
			title);

	CaptureListItem* ret = cli;
	this->listedItems.push_back(ret);
	this->setListed.insert(topWin);

	this->_EnsureScrollWinSizer();
	this->scrollWinSizer->Add(cli, 0, wxEXPAND);

	return ret;
}

void CaptureDlg::OnButton_Capture(wxCommandEvent& evt)
{
	this->CaptureSelected();
}

void CaptureDlg::OnButton_CaptureCloseDlg(wxCommandEvent& evt)
{
	this->CaptureSelected();
	this->Close();
}

void CaptureDlg::OnCheckbox_AutoClose(wxCommandEvent& evt)
{
	if(this->autoCloseCheck->GetValue() == true)
		this->captureBtn->Hide();
	else
		this->captureBtn->Show();

	this->Layout();
}

void CaptureDlg::OnButton_ToggleHelpMode(wxCommandEvent& evt)
{
	this->ToggleHelpMode(!this->usingHelpMode);
}

void CaptureDlg::OnClose(wxCloseEvent& evt)
{
	this->Destroy();
}

void CaptureDlg::OnSize(wxSizeEvent& evt)
{
	wxSize sz = this->GetClientSize();

	this->Freeze();
	this->Layout();

	if(wrapWidth != sz.x)
	{
		wrapWidth = sz.x;
		// Wrap() will insert newlines, which means the text 
		// will only get thinner unless we reset the text 
		// before rewrapping.
		this->processHelpText->SetLabel(_GetHelpText());
		this->processHelpText->Wrap(this->processHelpText->GetSize().x);
		this->Layout();
	}
	this->Thaw();
}

void CaptureDlg::OnInputDelegated_LeftMouseDown(
	CaptureListItem* item, 
	wxMouseEvent& evt)
{
	this->DoSelection(item, false, evt.ControlDown());
}

void CaptureDlg::OnInputDelegated_DLeftMouseDown(CaptureListItem* item, wxMouseEvent& evt)
{
	this->DoSelection(item, true, evt.ControlDown());
}

void CaptureDlg::DoSelection(CaptureListItem* item, bool confirm, bool ctrl)
{
	if(confirm == true)
	{
		if(ctrl == true)
		{ 
			this->DoSelection(item, false, true);
			return;
		}

		std::vector<HWND> vec({item->captureHWND});
		int caped = this->Capture(vec);

		if(caped == 0)
			return;

		if(this->autoCloseCheck->GetValue())
			this->Close();

		this->UpdateCaptureButtonsEnabled();
	}
	else
	{
		if(ctrl == true)
		{ 
			item->isSel = !item->isSel;
		}
		else
		{ 
			this->ClearListSelection(true);
			item->isSel = true;
		}
		this->UpdateListColor(item);
		this->UpdateCaptureButtonsEnabled();
	}
	
}

CaptureDlg* CaptureDlg::GetInstance(bool raise)
{
	if(inst != nullptr)
	{
		assert(::IsWindow(inst->GetHWND()));

		if(raise == true)
			inst->Raise();

		return inst;
	}


	static const wxSize arbitraryStartSz = wxSize(400, 600);
	inst = new CaptureDlg(nullptr, wxDefaultPosition, arbitraryStartSz);
	inst->Show();
	return inst;
}

bool CaptureDlg::Shutdown()
{
	if(inst == nullptr)
		return false;

	inst->Destroy();
	// Destroy() should call dialog destructor, which
	// should null out the singleton instance.
	assert(inst == nullptr);

	CapDragSelWin::Shutdown();

	return true;
}