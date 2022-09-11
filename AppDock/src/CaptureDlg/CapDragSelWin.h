#include <wx/wx.h>

class CaptureDlg;

/// <summary>
/// The (+) icon on the capture window. This UI element can be
/// dragged and dropped onto a window to add the window it was
/// dropped onto, into the AppDock system.
/// 
/// The mode of selection is similar to Spy++'s Finder tool.
/// </summary>
class CapDragSelWin : public wxWindow
{
public:
	/// <summary>
	/// The size the render the element. Depending on the size,
	/// it will be drawn differently.
	/// </summary>
	enum class Style
	{
		/// <summary>
		/// A compact icon.
		/// </summary>
		Small,

		/// <summary>
		/// A large icon. This will be 
		/// </summary>
		Large
	};

// TODO: Encapsulation
// private: 

	/// <summary>
	/// The icon to use when rendering the small style.
	/// </summary>
	static wxBitmap reticuleSmall;

	/// <summary>
	/// The icon to use when rendering the large style.
	/// </summary>
	static wxBitmap reticuleLarge;

	// TODO: Encapsulation
public:
	/// <summary>
	/// When dragging, the hovered window that is currently the 
	/// canidate being dragged on will have a rectangle around
	/// it. 
	/// 
	/// This is done as top-level window that is resized
	/// as large the target window, placed on top in the z-order.
	/// </summary>
	wxFrame* previewOly = nullptr;

	/// <summary>
	/// The window that the icon was last dragged over.
	/// </summary>
	HWND lastDraggedOver = NULL;

	/// <summary>
	/// Large style rendering will also have additional 
	/// text for help and context.
	/// 
	/// When dragging, it will also show the text
	/// </summary>
	wxStaticText* infoText = nullptr;

	/// <summary>
	/// The current style the element is set to render as.
	/// </summary>
	Style style = Style::Large;

	/// <summary>
	/// The CaptureDlg that the window is a child of.
	/// </summary>
	CaptureDlg* owner = nullptr;

	/// <summary>
	/// Counter to check if the mouse is over this window. This is a counter
	/// instead of a bool because multiple things will increment this -
	/// the other thing being the static text (captureLabel).
	/// </summary>
	int hoverCounter = 0;

public: // Public methods
	/// <summary>
	/// Constructor.
	/// </summary>
	/// <param name="parent">
	/// Parent window. This will either be a CaptureDlg, or a window
	/// child in a CaptureDlg's UI hierarchy.
	/// </param>
	/// <param name="owner">
	/// The CaptureDlg that is managing the CapDragSelWin.
	/// </param>
	CapDragSelWin(wxWindow* parent, CaptureDlg* owner);

	~CapDragSelWin();

	/// <summary>
	/// Change the style of rendering.
	/// </summary>
	void SetStyle(Style newStyle);

	/// <summary>
	/// Set the info text infoText to its default.
	/// </summary>
	// TODO: Rename to ResetInfoText
	void ResetHelpText();

	/// <summary>
	/// Set the info text to a name of a program.
	/// </summary>
	/// <param name="programName"></param>
	// TODO: Rename to SetInfoTextToProgram
	void SetProgramLabel(const wxString& programName);

	/// <summary>
	/// Update the child info text size whenever the parent
	/// CapDragSelWin changes.
	/// </summary>
	void UpdateInfoTextSize();

	/// <summary>
	/// Reset the background color.
	/// 
	/// Normally you don't call this directly with a color, but
	/// instead call UpdateHoverBackground(), which will then
	/// analyze the object's state and decide the correct color.
	/// </summary>
	/// <param name="c">The new background color.</param>
	void ChangeDragStateBackground(const wxColour& c);

	/// <summary>
	/// Updates the object's color.
	/// </summary>
	void UpdateHoverBackground();

	/// <summary>
	/// Clears dragging state.
	/// </summary>
	void EndDrag();

	//////////////////////////////////////////////////
	//
	//	wxWidget EVENT HANDLERS
	//
	//////////////////////////////////////////////////

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

public: // Public statics

	/// <summary>
	/// Should be called when the application is being closed.
	/// 
	/// Handles lingering global logic that needs to be handled
	/// for any/all CapDragSelWin objects.
	/// </summary>
	static void Shutdown();

protected:
	DECLARE_EVENT_TABLE()
};