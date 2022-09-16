
#include "DragPreviewOlyWin.h"
#include "DockWin.h"

wxBEGIN_EVENT_TABLE(DragPreviewOlyWin, wxTopLevelWindow)
	EVT_KEY_DOWN(DragPreviewOlyWin::OnKeyDown)
wxEND_EVENT_TABLE();

DragPreviewOlyWin::DragPreviewOlyWin(wxWindow * parent, DockWin* escapeSink)
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

void DragPreviewOlyWin::OnKeyDown(wxKeyEvent& evt)
{
	if(evt.GetKeyCode() == WXK_ESCAPE)
		this->escapeSink->OnDelegatedEscape();
}