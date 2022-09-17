#include "CaptureListItem.h"
#include "CaptureDlg.h"
#include "../Utils/mywxUtils.h"

const wxSize CaptureListItem::iconRegionSize = wxSize(16, 16);

wxBEGIN_EVENT_TABLE(CaptureListItem, wxWindow)
	EVT_PAINT		(CaptureListItem::OnPaint		)
	EVT_LEFT_DOWN	(CaptureListItem::OnLeftDown	)
	EVT_LEFT_DCLICK	(CaptureListItem::OnDoubleClick	)
	EVT_ENTER_WINDOW(CaptureListItem::OnMouseEnter	)
	EVT_LEAVE_WINDOW(CaptureListItem::OnMouseLeave	)
	EVT_SET_FOCUS	(CaptureListItem::OnSetFocus	)
	EVT_KILL_FOCUS	(CaptureListItem::OnKillFocus	)
	EVT_KEY_DOWN	(CaptureListItem::OnKeydown		)
wxEND_EVENT_TABLE()

CaptureListItem::CaptureListItem(
	CaptureDlg* owner,
	wxWindow* parent, 
	HWND hwnd, 
	HICON icon, 
	const wxString& title)
	: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE)
{
	this->owner = owner;
	this->captureIcon = icon;
	this->captureHWND = hwnd;
	this->captureLabel = new wxStaticText(this, wxID_ANY, title);
	MyWxUtils::SetupStaticTextMousePassthrough(this->captureLabel);

	wxBoxSizer* thisSizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(thisSizer);
	this->iconSpace = 
		thisSizer->Add(
			iconRegionSize.x, 
			iconRegionSize.y, 
			0, 
			wxALIGN_CENTER_VERTICAL|wxLEFT,
			border);
	//
	thisSizer->Add(
		this->captureLabel, 
		1, 
		wxALL|wxALIGN_CENTER_VERTICAL, 
		border);
}

void CaptureListItem::UpdateBackgroundColor(bool refresh)
{
	bool hovered = this->hoverCounter <= 0;

	if(this->isSel == true)
	{
		if(hovered == true)
			this->SetBackgroundColour(wxColour(220, 255, 220));
		else
			this->SetBackgroundColour(wxColour(235, 255, 235));
	}
	else
	{
		if(hovered == true)
			this->SetBackgroundColour(wxColour(220, 220, 220));
		else
			this->SetBackgroundColour(wxColour(200, 200, 200));
	}
	
	if(refresh == true)
		this->Refresh();
}

void CaptureListItem::OnPaint(wxPaintEvent& evt)
{
	wxPaintDC dc(this);

	if(this->HasFocus())
	{
		wxPen selBrush = wxPen(wxColor(100, 100, 100), 1, wxPENSTYLE_DOT);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(selBrush);

		wxSize sz = this->GetClientSize();
		int pad = 2;
			dc.DrawRectangle(
			wxRect(
				pad, 
				pad, 
				sz.x - pad * 2, 
				sz.y - pad * 2));
	}

	if(
		this->captureIcon != NULL && 
		this->iconSpace != nullptr) // Sanity check
	{
		wxRect r = this->iconSpace->GetRect();

		DrawIconEx(
			dc.GetHDC(),
			r.x,
			r.y,
			this->captureIcon,
			r.width,
			r.height,
			0,
			NULL,
			DI_NORMAL);
	}
}

void CaptureListItem::OnLeftDown(wxMouseEvent& evt)
{
	this->SetFocus();
	this->owner->OnInputDelegated_LeftMouseDown(this, evt);
}

void CaptureListItem::OnDoubleClick(wxMouseEvent& evt)
{
	this->owner->OnInputDelegated_DLeftMouseDown(this, evt);
}

void CaptureListItem::OnMouseEnter(wxMouseEvent& evt)
{
	++this->hoverCounter;
	this->UpdateBackgroundColor();
}

void CaptureListItem::OnMouseLeave(wxMouseEvent& evt)
{
	--this->hoverCounter;
	this->UpdateBackgroundColor();
}

void CaptureListItem::OnSetFocus(wxFocusEvent& evt)
{
	this->Refresh();
}

void CaptureListItem::OnKillFocus(wxFocusEvent& evt)
{
	this->Refresh();
}

void CaptureListItem::OnKeydown(wxKeyEvent& evt)
{
	switch(evt.GetKeyCode())
	{
	case WXK_NUMPAD_ENTER:
	case WXK_SPACE:
	case WXK_RETURN:
		this->owner->DoSelection(this, true, evt.ControlDown());
		break;

	default:
		evt.Skip();
		break;
	}
}

/// <summary>
/// Check if a top-level HWND is deserving enough to be
/// placed in the capture dialog.
/// </summary>
/// <param name="icon"></param>
/// <param name="appTitle"></param>
/// <returns></returns>
bool CaptureListItem::IsValid(HICON icon, wxString& appTitle)
{
	// If it doesn't have and defining features the user
	// can recognize if it was placed in the list - it's
	// pointless to list it, and possibly even dangerous.
	return 
		icon != NULL ||
		appTitle.empty() == false;
}