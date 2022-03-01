#include "DlgIntro.h"
#include "../Utils/AppUtils.h"

wxBEGIN_EVENT_TABLE(DlgIntro, wxDialog)
	EVT_BUTTON(wxID_OK,		DlgIntro::OnButtonOK)
	EVT_CLOSE(DlgIntro::OnClose)
wxEND_EVENT_TABLE()

DlgIntro::DlgIntro()
	: wxDialog(nullptr, wxID_ANY, "Notice", wxDefaultPosition, wxSize(400, 400))
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(mainSizer);

	wxStaticText* noticeText = 
		new wxStaticText(
			this, 
			wxID_ANY, 
			"TODO: WARNING HERE\nSome thing about manipulating windows screw up apps, and that this is pre-alpha.");

	mainSizer->Add(noticeText, 1, wxALL, 10);

	wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
	okBtn->SetMinSize(wxSize(-1, 50));
	mainSizer->Add(okBtn, 0, wxGROW|wxALL, 2);

	AppUtils::SetDefaultIcons(this);
}

DlgIntro::~DlgIntro()
{
}

void DlgIntro::OnButtonOK(wxCommandEvent& evt)
{
	this->EndModal(wxID_OK);
}

void DlgIntro::OnClose(wxCloseEvent& evt)
{
	this->EndModal(wxID_CANCEL);
}