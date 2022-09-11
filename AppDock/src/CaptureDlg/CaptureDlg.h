#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <vector>
#include <set>

class CaptureDlg;
class CapDragSelWin;
class CaptureListItem;


/// <summary>
/// 
/// </summary>
class CaptureDlg : public wxFrame
{
public:
	// Icons for the help toggle button
	static wxBitmap questionSmall;
	static wxBitmap questionLarge;

	// Sizes for the icons
	const wxSize szButtonsCompact = wxSize(24, 24);
	const wxSize szButtonsFull = wxSize(40, 60);

	enum class CMDID
	{
		Btn_Capture,
		Btn_CaptureAndClose,
		CkB_AutoClose,
		Btn_ToggleHelp
	};
	/// <summary>
	/// Singleton instance
	/// </summary>
	static CaptureDlg* inst;
	
public:

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
	/// 
	/// </summary>
	wxBoxSizer* scrollWinSizer = nullptr;

	/// <summary>
	/// The container holdering the Process UI elements
	/// and help info.
	/// </summary>
	wxWindow* processCont = nullptr;

	/// <summary>
	/// 
	/// </summary>
	wxStaticText* processHelpText = nullptr;

	/// <summary>
	/// 
	/// </summary>
	wxBoxSizer* helpTextSizer = nullptr;

	/// <summary>
	/// The help button taht toggles help-mode.
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
	/// 
	/// </summary>
	std::vector<wxSizerItem*> helpModeSizerPadding;

	/// <summary>
	/// 
	/// </summary>
	std::vector<CaptureListItem*> listedItems;

	/// <summary>
	/// 
	/// </summary>
	std::set<HWND> setListed;

private:


	/// <summary>
	/// 
	/// </summary>
	/// <param name="parent"></param>
	/// <param name="pos"></param>
	/// <param name="size"></param>
	CaptureDlg(wxWindow* parent, const wxPoint& pos, const wxSize& size);

	void _EnsureScrollWinSizer();
	void _ClearScrollWinSizer();
	static const char * _GetHelpText();
public:

	

	/// <summary>
	/// Toggle the help mode
	/// </summary>
	/// <param name="showHelp"></param>
	/// <param name="force"></param>
	void ToggleHelpMode(bool showHelp, bool force = false);

	void SetListSelectionAll(bool val, bool refreshDisplay = true);
	void ClearListSelection(bool refreshDisplay = true);

	int GetSelectionCount();

	std::vector<CaptureListItem*> GetSelected();
	std::vector<HWND> GetSelectedHWND();

	int Capture(std::vector<HWND> toCapture);
	void CaptureSelected();

	void RebuildListed();
	void ClearListed();

	void UpdateListedSelectionColors();
	void UpdateListColor(CaptureListItem* cli, bool refresh = true);
	void UpdateListColor(CaptureListItem* cli, bool sel, bool refresh = true);
	void UpdateCaptureButtonsEnabled();

	CaptureListItem* AddListItem(HWND topWin);

	void OnButton_Capture(wxCommandEvent& evt);
	void OnButton_CaptureCloseDlg(wxCommandEvent& evt);
	void OnCheckbox_AutoClose(wxCommandEvent& evt);
	void OnButton_ToggleHelpMode(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void OnSize(wxSizeEvent& evt);

	void OnInputDelegated_LeftMouseDown(CaptureListItem* item, wxMouseEvent& evt);
	void OnInputDelegated_DLeftMouseDown(CaptureListItem* item, wxMouseEvent& evt);
	void DoSelection(CaptureListItem* item, bool confirm, bool ctrl);

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

	virtual ~CaptureDlg();

protected:
	DECLARE_EVENT_TABLE();
};
