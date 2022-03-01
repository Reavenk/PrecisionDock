
#include "BarDrag.h"
#include "DockWin.h"

wxBEGIN_EVENT_TABLE(BarDrag, wxTopLevelWindow)
	EVT_KEY_DOWN(BarDrag::OnKeyDown)
wxEND_EVENT_TABLE();

BarDrag::BarDrag(wxWindow * parent, DockWin* escapeSink)
	: wxTopLevelWindow(
		parent, 
		-1, 
		"BarDrag", 
		wxDefaultPosition, 
		wxSize(60, 15), 
		wxBORDER_NONE|wxCLIP_CHILDREN)
{
	this->escapeSink = escapeSink;

	this->SetCursor(*wxCROSS_CURSOR);
}

void BarDrag::OnKeyDown(wxKeyEvent& evt)
{
	if(evt.GetKeyCode() == WXK_ESCAPE)
		this->escapeSink->OnDelegatedEscape();
}