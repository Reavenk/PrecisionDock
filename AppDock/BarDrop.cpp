#include "BarDrop.h"
#include "DockWin.h"

wxBEGIN_EVENT_TABLE(BarDrop, wxTopLevelWindow)
	EVT_KEY_DOWN(BarDrop::OnKeyDown)
wxEND_EVENT_TABLE()

BarDrop::BarDrop(wxWindow* parent, DockWin* escapeSink)
	: wxTopLevelWindow(
		parent, 
		-1, 
		"", 
		wxDefaultPosition, 
		wxDefaultSize, 
		wxBORDER_NONE|wxCLIP_CHILDREN)
{
	this->escapeSink = escapeSink;
	this->SetBackgroundColour(wxColor(0, 255, 0));
}

void BarDrop::OnKeyDown(wxKeyEvent& evt)
{
	if(evt.GetKeyCode() == WXK_ESCAPE)
		escapeSink->OnDelegatedEscape();
}