#include "BarDrop.h"
#include "DockWin.h"
#include "Utils/AppUtils.h"

wxBEGIN_EVENT_TABLE(BarDrop, wxFrame)
	EVT_KEY_DOWN(BarDrop::OnKeyDown)
wxEND_EVENT_TABLE()

BarDrop::BarDrop(wxWindow* parent, DockWin* escapeSink)
	: wxFrame (
		parent, 
		-1, 
		"", 
		wxDefaultPosition, 
		wxDefaultSize, 
		wxBORDER_NONE|wxCLIP_CHILDREN|wxFRAME_NO_TASKBAR)
{
	this->escapeSink = escapeSink;
	this->SetBackgroundColour(wxColor(0, 255, 0));
	AppUtils::SetWindowTransparency(this, 100);
}

void BarDrop::OnKeyDown(wxKeyEvent& evt)
{
	if(evt.GetKeyCode() == WXK_ESCAPE)
		escapeSink->OnDelegatedEscape();
}