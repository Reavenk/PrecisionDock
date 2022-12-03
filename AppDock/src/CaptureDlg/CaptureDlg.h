#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <vector>
#include <set>

class CaptureDlg;
class CapDragSelWin;
class CaptureListItem;


/// <summary>
/// A modeless dialog that list all known windows currently running. It 
/// allows docking these windows to the system.
/// 
/// It also has a draggable icon that can be dropped windows to add them.
/// 
/// The dialog is currently opened by double clicking in the taskbar icon.
/// </summary>
class CaptureDlg : public wxFrame
{
public:
	// wxWidget Command IDs
	enum class CMDID
	{
		Btn_Capture,				// See OnButton_Capture()
		Btn_CaptureAndClose,		// See OnButton_CaptureCloseDlg()
		CkB_AutoClose,				// See OnCheckbox_AutoClose()
		Btn_ToggleHelp				// See OnButton_ToggleHelpMode()
	};

public:
	// Icons for the help toggle button
	static wxBitmap questionSmall;
	static wxBitmap questionLarge;

	// Sizes for the icons
	const wxSize szButtonsCompact = wxSize(24, 24);
	const wxSize szButtonsFull = wxSize(40, 60);

	/// <summary>
	/// Singleton instance
	/// </summary>
	static CaptureDlg* inst;
	
public:
	/// <summary>
	/// The cached width of the dialog. Used to check if the width has
	/// changed, and if the text wrapping in processHelpText needs to
	/// be reset.
	/// </summary>
	int wrapWidth = -1;

	/// <summary>
	/// A copy of the window's HWND that last past the subclass' 
	/// destruction.
	/// </summary>
	HWND cachedHwnd = NULL;

	/// <summary>
	/// The scrollable window with listed top-level
	/// window content inside.
	/// </summary>
	wxScrolledWindow * scrollWin = nullptr;

	/// <summary>
	/// The sizer containing all the Window listings in scrollWin.
	/// </summary>
	wxBoxSizer* scrollWinSizer = nullptr;

	/// <summary>
	/// The container holdering the Process UI elements
	/// and help info.
	/// </summary>
	wxWindow* processCont = nullptr;

	/// <summary>
	/// A help text shown below the dialog's title.
	/// </summary>
	wxStaticText* processHelpText = nullptr;

	/// <summary>
	/// The sizer used to hold processHelpText, which can be updated
	/// when the help mode changes.
	/// </summary>
	wxBoxSizer* helpTextSizer = nullptr;

	/// <summary>
	/// The help button that toggles help-mode.
	/// 
	/// Help mode shows help text for certain things.
	/// </summary>
	wxButton* helpBtn = nullptr;

	/// <summary>
	/// The window that can be dragged.
	/// </summary>
	CapDragSelWin* dragTarg = nullptr;

	/// <summary>
	/// The window that can be dragged in mini-mode
	/// </summary>
	CapDragSelWin* dragTargMin = nullptr;

	/// <summary>
	/// The "Capture" button
	/// </summary>
	wxButton* captureBtn = nullptr;

	/// <summary>
	/// The "Capture & Close Dlg" button
	/// </summary>
	wxButton* captureOKBtn = nullptr;

	/// <summary>
	/// If checked, capturing windows will automatically
	/// close the dialog.
	/// </summary>
	wxCheckBox* autoCloseCheck = nullptr;

	/// <summary>
	/// Cached state of if helpmode is used.
	/// </summary>
	bool usingHelpMode = true;

	/// <summary>
	/// When in help mode, UI items don't care too much about compactness, 
	/// and will padd themselves vertically the visually seperate out the
	/// clutter. 
	/// 
	/// To automate this process, we just keep a list of things to change
	/// the sizer padding for (in a mechanical/consistent) way.
	/// </summary>
	std::vector<wxSizerItem*> helpModeSizerPadding;

	/// <summary>
	/// The list of known Windowed processes.
	/// </summary>
	std::vector<CaptureListItem*> listedItems;

	/// <summary>
	/// The list of all HWNDs in listedItems. Used to quickly check if a
	/// HWND is known to the dialog (as a Windowed process).
	/// </summary>
	std::set<HWND> setListed;

private:


	/// <summary>
	/// Constructor
	/// </summary>
	CaptureDlg(wxWindow* parent, const wxPoint& pos, const wxSize& size);

	virtual ~CaptureDlg();

	/// <summary>
	/// Ensures scrollWinSizer is non-null and set up correctly.
	/// </summary>
	void _EnsureScrollWinSizer();

	/// <summary>
	/// Creates a new scrollWinSizer.
	/// </summary>
	void _ClearScrollWinSizer();

	/// <summary>
	/// The authority on what the text is in the help region.
	/// </summary>
	/// <returns>The text region.</returns>
	static const char * _GetHelpText();
public:

	

	/// <summary>
	/// Toggle the help mode
	/// </summary>
	/// <param name="showHelp">
	/// If true, set the dialog to help mode.
	/// If false, set the dialog to a more streamlined and compact UI mode.
	/// </param>
	/// <param name="force">
	/// If true, do not ignore setting up the state for the help mode, even if
	/// the application is already using the specified help mode.
	/// </param>
	void ToggleHelpMode(bool showHelp, bool force = false);

	/// <summary>
	/// Set the selection state for all known processes.
	/// </summary>
	/// <param name="val"></param>
	/// <param name="refreshDisplay">If true, redraw the UI.</param>
	void SetListSelectionAll(bool val, bool refreshDisplay = true);

	/// <summary>
	/// Clear the selection of processes.
	/// </summary>
	/// <param name="refreshDisplay"></param>
	void ClearListSelection(bool refreshDisplay = true);

	/// <summary>
	/// Get the number of listed processes.
	/// </summary>
	/// <returns></returns>
	int GetSelectionCount();

	/// <summary>
	/// Get a copy of all the selected entries of Windowed processes.
	/// </summary>
	std::vector<CaptureListItem*> GetSelected();

	/// <summary>
	/// Get a vector of all the HWNDs of selected Windowed processes.
	/// </summary>
	std::vector<HWND> GetSelectedHWND();

	/// <summary>
	/// Have the application dock selected HWNDs, and modify the
	/// dialog as needed to reflect those changes.
	/// </summary>
	/// <param name="toCapture">The HWNDs to active</param>
	/// <returns></returns>
	// TODO: Use a better function name, the term Capture() could be improved upon
	int Capture(std::vector<HWND> toCapture);

	/// <summary>
	/// Have the application dock the selected windows.
	/// </summary>
	void CaptureSelected();

	/// <summary>
	/// Clears the listed process and remakes them.
	/// </summary>
	void RebuildListed();

	/// <summary>
	/// Clear all the listed processes. This will destroy their UI, 
	/// as well as remove the knowledge of them from the application.
	/// </summary>
	void ClearListed();


	void UpdateListedSelectionColors();
	void UpdateListColor(CaptureListItem* cli, bool refresh = true);
	void UpdateListColor(CaptureListItem* cli, bool sel, bool refresh = true);
	void UpdateCaptureButtonsEnabled();

	CaptureListItem* AddListItem(HWND topWin);
	
	bool RemoveListItem(HWND topWin);

	/// <summary>
	/// Change the selection status of a Windowed process.
	/// </summary>
	/// <param name="item"></param>
	/// <param name="confirm"></param>
	/// <param name="ctrl"></param>
	void DoSelection(CaptureListItem* item, bool confirm, bool ctrl);

	//	
	//	Delegated CaptureListItem functions
	//////////////////////////////////////////////////

	// These functions are called from CaptureListItem, to delegate their
	// input to their manager.
	//
	// This makes the organization of code responsibilities and encapsulation
	// easier.

	void OnInputDelegated_LeftMouseDown(CaptureListItem* item, wxMouseEvent& evt);
	void OnInputDelegated_DLeftMouseDown(CaptureListItem* item, wxMouseEvent& evt);

	//////////////////////////////////////////////////
	//	
	//	System HWND Events Handlers
	//
	//////////////////////////////////////////////////
	
	void OnTopLevelRenamed(HWND hwnd);
	void OnTopLevelCreated(HWND hwnd);
	void OnTopLevelDestroyed(HWND hwnd);

	//////////////////////////////////////////////////
	//	
	//	wxWidgets CALLBACKS
	//
	//////////////////////////////////////////////////

	void OnButton_Capture(wxCommandEvent& evt);
	void OnButton_CaptureCloseDlg(wxCommandEvent& evt);
	void OnCheckbox_AutoClose(wxCommandEvent& evt);
	void OnButton_ToggleHelpMode(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void OnSize(wxSizeEvent& evt);


public: // Static functions

	/// <summary>
	/// Create the singleton window if needed, and return
	/// a pointer to it.
	/// </summary>
	/// <returns>The CaptureDlg singleton instance.</returns>
	static CaptureDlg* GetInstance(bool raise = true);

	/// <summary>
	/// Shutdown the singleton instance.
	/// </summary>
	/// <returns>
	/// If true, a dialog was closed.
	/// If false, there was no dialog to close.
	/// </returns>
	static bool Shutdown();

protected:
	DECLARE_EVENT_TABLE();
};
