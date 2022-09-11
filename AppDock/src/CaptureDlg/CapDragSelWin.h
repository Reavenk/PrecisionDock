#include <wx/wx.h>

class CaptureDlg;

/// <summary>
/// 
/// </summary>
class CapDragSelWin : public wxWindow
{
public:
	enum class Style
	{
		Small,
		Large
	};

	static wxBitmap reticuleSmall;
	static wxBitmap reticuleLarge;

public:
	/// <summary>
	/// 
	/// </summary>
	wxFrame* previewOly = nullptr;

	/// <summary>
	/// 
	/// </summary>
	HWND lastDraggedOver = NULL;

	/// <summary>
	/// 
	/// </summary>
	wxStaticText* infoText = nullptr;

	Style style = Style::Large;

	CaptureDlg* owner = nullptr;

	int hoverCounter = 0;

public:
	/// <summary>
	/// 
	/// </summary>
	/// <param name="parent"></param>
	/// <param name="owner"></param>
	CapDragSelWin(wxWindow* parent, CaptureDlg* owner);

	~CapDragSelWin();

	void SetType(Style newStyle, bool force = false);

	void ResetHelpText();
	void SetProgramLabel(const wxString& programName);
	void UpdateInfoTextSize();

	void ChangeDragStateBackground(const wxColour& c);
	void UpdateHoverBackground();

	void OnPaint(wxPaintEvent& evt);
	void OnLeftMouseDown(wxMouseEvent& evt);
	void OnLeftMouseUp(wxMouseEvent& evt);
	void OnMotion(wxMouseEvent& evt);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
	void OnMouseCaptureChanged(wxMouseCaptureChangedEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnResize(wxSizeEvent& evt);
	void OnKeydown(wxKeyEvent& evt);

	/// <summary>
	/// 
	/// </summary>
	void EndDrag();

	void SetStyle(Style newStyle);

	static void Shutdown();

protected:
	DECLARE_EVENT_TABLE()
};