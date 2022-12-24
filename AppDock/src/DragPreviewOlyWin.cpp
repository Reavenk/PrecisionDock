
#include "DragPreviewOlyWin.h"
#include "DockWin.h"
#include "Utils/AppUtils.h"
// > ☐ DRAGOLY_MAIN_735bd5a48d12: Dragging a window in the layout shows a representation of the drag near the cursor.
// > ☐ DRAGOLY_MAIN_6893e998f2be: Drag overlay to shop showing when the drag operation is complete.
// > ☐ DRAGOLY_MAIN_6397bbd27efe: Drag overlay to shop showing when the drag operation is cancelled.

wxBEGIN_EVENT_TABLE(DragPreviewOlyWin, wxFrame)
	EVT_KEY_DOWN(DragPreviewOlyWin::OnKeyDown)
	EVT_PAINT(DragPreviewOlyWin::OnPaint)
wxEND_EVENT_TABLE();

DragPreviewOlyWin::DragPreviewOlyWin(wxWindow * parent, HWND appBeingDragged, DockWin* escapeSink)
	: wxFrame(
		parent, 
		-1, 
		"BarDrag", 
		wxDefaultPosition, 
		wxSize(60, 30), 
		wxBORDER_NONE|wxCLIP_CHILDREN|wxFRAME_NO_TASKBAR)
{
	this->escapeSink = escapeSink;
	this->appBeingDragged = appBeingDragged;

	// > ☐ DRAGOLY_DISP_133e4748509e: Drag overlay to show the window's label of the window being dragged.
	// > ☐ DRAGOLY_DISP_8610471ec7fe: Drag overlay to show the icon of the window being dragged.

	this->SetCursor(*wxCROSS_CURSOR);

	AppUtils::SetWindowTransparency(this, 100);
}

void DragPreviewOlyWin::OnKeyDown(wxKeyEvent& evt)
{
	// > ☐ DOCKWIN_DRAG_c06d29ee0c7e: Dragging a window can be canelled by pressing escape during the drag.
	if(evt.GetKeyCode() == WXK_ESCAPE)
		this->escapeSink->OnDelegatedEscape();
}

void DragPreviewOlyWin::OnPaint(wxPaintEvent& evt)
{
	wxPaintDC dc(this);
	wxSize clientSz = this->GetClientSize();

	HICON winIcon = (HICON)GetClassLongPtr(this->appBeingDragged, GCLP_HICON /* or GCLP_HICONSM */);
	int iconDrawDim = 20;
	int iconDrawLeftPadding = 5;

	if (winIcon)
	{
		DrawIconEx(
			dc.GetHDC(),
			iconDrawLeftPadding,
			(clientSz.y - iconDrawDim)/2,
			winIcon,
			iconDrawDim,
			iconDrawDim,
			0,
			NULL,
			DI_NORMAL);
	}
}