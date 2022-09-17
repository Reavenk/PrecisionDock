#include "CapDragSelWin.h"
#include "../resource.h"
#include "../Utils/mywxUtils.h"
#include "CaptureDlg.h"
#include "../AppDock.h"

wxBitmap CapDragSelWin::reticuleSmall;
wxBitmap CapDragSelWin::reticuleLarge;

static wxColour bgColorNormal(200, 255, 200);
static wxColour bgColorHover(175, 255, 175);
static wxColour bgColorDrag(240, 255, 150);

wxBEGIN_EVENT_TABLE(CapDragSelWin, wxWindow)
	EVT_PAINT				(CapDragSelWin::OnPaint					)
	EVT_LEFT_DOWN			(CapDragSelWin::OnLeftMouseDown			)
	EVT_LEFT_UP				(CapDragSelWin::OnLeftMouseUp			)
	EVT_MOTION				(CapDragSelWin::OnMotion				)
	EVT_MOUSE_CAPTURE_LOST	(CapDragSelWin::OnMouseCaptureLost		)
	EVT_MOUSE_CAPTURE_CHANGED(CapDragSelWin::OnMouseCaptureChanged	)
	EVT_ENTER_WINDOW		(CapDragSelWin::OnMouseEnter			)
	EVT_LEAVE_WINDOW		(CapDragSelWin::OnMouseLeave			)	
	EVT_SIZE				(CapDragSelWin::OnResize				)
	EVT_KEY_DOWN			(CapDragSelWin::OnKeydown				)
wxEND_EVENT_TABLE()

CapDragSelWin::CapDragSelWin(wxWindow* parent, CaptureDlg* owner)
	: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
	this->owner = owner;

	if(reticuleSmall.IsNull() == true)
		reticuleSmall = MyWxUtils::LoadFromResourceID(IDB_RETICULE_SMALL);

	if(reticuleLarge.IsNull() == true)
		reticuleLarge = MyWxUtils::LoadFromResourceID(IDB_RETICULE_LARGE);

	this->SetBackgroundColour(wxColour(200, 255, 200));

	this->infoText = new wxStaticText(this, wxID_ANY, "");
	this->ResetInfoText();

	MyWxUtils::SetupStaticTextMousePassthrough(this->infoText);

	this->SetToolTip("Click and drag on to a window.");

}

CapDragSelWin::~CapDragSelWin()
{
	if(this->previewOly != nullptr)
	{
		delete this->previewOly;
		this->previewOly = nullptr;
	}
}

void CapDragSelWin::ResetInfoText()
{
	this->infoText->SetLabel(
		"Click and drag this region onto a top-level window you want to capture.");

	this->UpdateInfoTextSize();
}

void CapDragSelWin::SetInfoTextToProgram(const wxString& programName)
{
	this->infoText->SetLabel(programName);
	this->UpdateInfoTextSize();
}

void CapDragSelWin::UpdateInfoTextSize()
{
	if(this->infoText->IsShown())
	{ 
		wxSize sz = this->GetSize();
		const int leftIconPad = 50;
		const int vPadding = 10;
		this->infoText->SetPosition( wxPoint(leftIconPad, vPadding));
		this->infoText->SetSize(wxSize(sz.x - (leftIconPad + 10), sz.y - vPadding * 2));
	}
}

void CapDragSelWin::ChangeDragStateBackground(const wxColour& c)
{
	this->SetBackgroundColour(c);
	this->Refresh();
}

void CapDragSelWin::OnPaint(wxPaintEvent& evt)
{
	wxRect r = GetClientRect();

	wxPaintDC dc(this);

	// Pretend to do transparent edges. To do this, we're 
	// going to assume the parent's background is a solid 
	// color and use that for our transparent areas.
	wxColour parentBgCol = this->GetParent()->GetBackgroundColour();
	wxBrush wxbParBg(parentBgCol);
	wxSize clientSz = this->GetClientSize();
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxbParBg);
	dc.DrawRectangle(wxRect(wxPoint(), clientSz));

	wxBrush wxbFill(this->GetBackgroundColour());
	dc.SetBrush(wxbFill);

	if(this->style == Style::Small)
	{
		dc.DrawEllipse(r);

		dc.DrawBitmap(
			reticuleSmall, 
			wxPoint(
				(r.width - 16) / 2,
				(r.height - 16) / 2), 
			true);
	}
	else if(this->style == Style::Large)
	{
		// We're going to assume a horizontal capsule is 
		// always drawn.
		int halfHeight = r.height/2;
		dc.DrawCircle(wxPoint(halfHeight, halfHeight), r.height/2);
		dc.DrawCircle(wxPoint(r.width - halfHeight, halfHeight), r.height/2);
		dc.DrawRectangle(wxRect(halfHeight, 0, r.width - 2 * halfHeight, r.height));

		dc.DrawBitmap(
			reticuleLarge, 
			wxPoint(10, (r.height - 32) / 2), 
			true);
	}
}

void CapDragSelWin::OnLeftMouseDown(wxMouseEvent& evt)
{
	this->lastDraggedOver = NULL;
	this->CaptureMouse();

	this->ChangeDragStateBackground(bgColorDrag);
	this->SetFocus();
}

void CapDragSelWin::OnLeftMouseUp(wxMouseEvent& evt)
{
	if(this->lastDraggedOver != NULL)
	{
	}

	this->EndDrag();

	this->SetFocus();
}

void CapDragSelWin::OnMotion(wxMouseEvent& evt)
{
	if(this->HasCapture() == false)
		return;

	if(this->previewOly == nullptr)
	{
		this->previewOly = 
			new wxFrame(
				nullptr, 
				wxID_ANY, 
				"", 
				wxDefaultPosition,
				wxDefaultSize,
				wxFRAME_NO_TASKBAR|wxSTAY_ON_TOP);

		this->Show();
		this->previewOly->SetBackgroundColour(
			wxColour(255, 200, 200));
	}

	AppDock& app = AppDock::GetApp();

	wxPoint globalMouse = 
		this->ClientToScreen(evt.GetPosition());

	HWND ownerHWND = this->owner->GetHWND();
	this->lastDraggedOver = NULL;
	HWND desktWin = GetDesktopWindow();
	for(
		HWND it = GetTopWindow(desktWin); 
		it != NULL; 
		it = GetWindow(it, GW_HWNDNEXT))
	{
		if(IsWindowVisible(it) == FALSE)
			continue;

		RECT r;
		if(GetWindowRect(it, &r) == FALSE)
			continue;

		wxRect wxr = 
			wxRect(
				r.left, 
				r.top, 
				r.right - r.left, 
				r.bottom - r.top);

		if(wxr.Contains(globalMouse) == true)
		{
			if(app.IsToplevelOwned(it) == true)
				break;

			int strLen = GetWindowTextLength(it) + 1;
			if(strLen == 1)
				continue;

			this->previewOly->Show();
			this->previewOly->SetPosition(wxr.GetPosition());
			this->previewOly->SetSize(wxr.GetSize());
			lastDraggedOver = it;

			std::vector<WCHAR> winText;
			winText.resize(strLen);
			GetWindowText(it, &winText[0], strLen);
			this->SetInfoTextToProgram(&winText[0]);
			break;
		}
	}

	if(this->lastDraggedOver == NULL)
		this->previewOly->Hide();

	this->SetFocus();
}

void CapDragSelWin::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
	this->EndDrag();
}

void CapDragSelWin::OnMouseCaptureChanged(wxMouseCaptureChangedEvent& evt)
{
	this->EndDrag();
}

void CapDragSelWin::OnMouseEnter(wxMouseEvent& evt)
{
	++this->hoverCounter;
	this->SetCursor(wxStockCursor::wxCURSOR_HAND);

	if(this->HasCapture() == false)
		this->UpdateHoverBackground();
}

void CapDragSelWin::OnMouseLeave(wxMouseEvent& evt)
{
	--this->hoverCounter;

	if(this->HasCapture() == false)
		this->UpdateHoverBackground();
}

void CapDragSelWin::OnResize(wxSizeEvent& evt)
{
	this->Refresh();
	this->UpdateInfoTextSize();
}

void CapDragSelWin::OnKeydown(wxKeyEvent& evt)
{
	switch(evt.GetKeyCode())
	{
	case WXK_ESCAPE:
		{
			if(this->HasCapture())
				this->ReleaseMouse();
		}
		break;
	}
}

void CapDragSelWin::SetStyle(Style newStyle)
{
	this->style = newStyle;

	if(newStyle == Style::Small)
	{
		if(this->infoText != nullptr)
			this->infoText->Show(false);
	}
	else if(newStyle == Style::Large)
	{
		if(this->infoText != nullptr)
			this->infoText->Show(true);
	}
}

void CapDragSelWin::EndDrag()
{
	if(this->previewOly != nullptr)
	{
		delete previewOly;
		this->previewOly = nullptr;
	}

	if(this->HasCapture() == true)
		this->ReleaseMouse();

	this->lastDraggedOver = NULL;
	this->ResetInfoText();

	//wxPoint mousePos = this->ScreenToClient(wxGetMousePosition());
	//if(this->GetClientRect().Contains(mousePos))
	//	this->hoverCounter = std::max(this->hoverCounter, 1);

	this->UpdateHoverBackground();
}

void CapDragSelWin::UpdateHoverBackground()
{
	if(this->hoverCounter > 0)
		this->ChangeDragStateBackground(bgColorHover);
	else
		this->ChangeDragStateBackground(bgColorNormal);
}

void CapDragSelWin::Shutdown()
{
}