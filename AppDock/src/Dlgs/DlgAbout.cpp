#include "DlgAbout.h"
#include "../Utils/AppUtils.h"
#include "../AppDock.h"

// Currently just a placeholder.

wxBEGIN_EVENT_TABLE(DlgAbout, wxDialog)
	EVT_BUTTON(wxID_OK, DlgAbout::OnButtonOK)
	EVT_CLOSE(DlgAbout::OnClose)
wxEND_EVENT_TABLE()

DlgAbout::DlgAbout()
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

	wxBoxSizer* horizSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(horizSizer, 0, wxEXPAND|wxALL, 5);
	wxStaticText* stxtVer	= new wxStaticText(this, wxID_ANY, "Version #.#.#");
	wxStaticText* stxtBuild	= new wxStaticText(this, wxID_ANY, "Built: --");
	horizSizer->Add(stxtVer,	0, wxLEFT);
	horizSizer->Add(stxtBuild,	0, wxRIGHT);

	wxTextCtrl* infoField = new wxTextCtrl(this, wxID_ANY, "");
	mainSizer->Add(infoField, 1, wxALL|wxEXPAND, 5);

	wxButton* okBtn = new wxButton(this, wxID_OK, "Gotcha!");
	mainSizer->Add(okBtn, 0, wxALL, 5);
}

DlgAbout::~DlgAbout()
{
	AppDock::GetApp().UnregisterToplevelOwned(this->cachedHWND);
}

void DlgAbout::OnButtonOK(wxCommandEvent& evt)
{
	this->EndModal(wxID_OK);
}

void DlgAbout::OnClose(wxCloseEvent& evt)
{
	this->EndModal(wxID_OK);
}