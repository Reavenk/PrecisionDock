#pragma once

#include <wx/wx.h>
#include <wx/event.h>
#include <vector>

wxDECLARE_EVENT(TAB_CLICK_EVENT, wxCommandEvent);

class DlgInfoTabs : public wxWindow
{
	// The scope of DlgInfoTags is in a wonkey limbo right now. 
	// While it's only used for DlgInfo, it's also being built
	// semi-robust so that it can be seperated into its own tab
	// system.
	//
	// The reason we want our own tab system is to match the style
	// and code of what we're using for the main application for 
	// design language cohesion.
public:
	struct TabEntry
	{
		int id;
		wxString label;
	};

private:
	const int tabHeight = 30;
	const int tabWidth = 150;
	const int tabBottom = 20;

	int selIdx			= -1;
	int hoverIdx		= -1;
	int clickCanidate	= -1;

	std::vector<TabEntry> entries;

private:
	bool _IsIndexValid(int idx);

public:
	DlgInfoTabs(wxWindow* parent, int id, const wxPoint& pt, const wxSize& sz);

	inline int GetSelection() const
	{ return this->selIdx; }

	inline size_t TabCount() const
	{ return this->entries.size(); }

	int IndexOver(const wxPoint& pt);

	int AddEntry(int id, const wxString& label);
	bool RemoveIndex(int idxToRem);
	bool RemoveEntry(int idToRem);

	void SelectId(int id);
	void SelectIndex(int idx);

	void OnMotion(wxMouseEvent& evt);
	void OnMouseLeftDown(wxMouseEvent& evt);
	void OnMouseLeftUp(wxMouseEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnDraw(wxPaintEvent& evt);

protected:
	DECLARE_EVENT_TABLE()
};

/// <summary>
/// The about dialog.
/// </summary>
class DlgInfo : public wxDialog
{
public:
	enum class CmdIDs
	{
		TabAbout,
		TabIntro,
		BtnGotcha,
		BtnBailIntro
	};

	enum CreateFlag
	{
		HasAbout			= 1 << 0,
		HasIntro			= 1 << 1,
		HasOKConfirm		= 1 << 2,
		HasGotchaConfirm	= 1 << 3,
		HasCancelConfirm	= 1 << 4,
		Default				= HasAbout|HasIntro|HasOKConfirm
	};

private:

	DlgInfoTabs* tabs = nullptr;

	/// <summary>
	/// The window to the about dialog, to make sure the application
	/// cannot try to select and dock it.
	/// 
	/// We cache this instead of always using this->GetHWND() because
	/// it may be the case that the HWND could be invalid 
	/// </summary>
	HWND cachedHWND = NULL;

	wxScrolledWindow* contentRegion = nullptr;

protected:
	void _AddContentsForIntro();
	void _AddContentsForAbout();
	void ClearContentRegion();

public: // Public methods

	DlgInfo(int createFlags = CreateFlag::Default, int startingIndex = 0);
	~DlgInfo();

	void SetTab(CmdIDs tmode, bool force = false);

	//////////////////////////////////////////////////
	//
	//	wxWidgets EVENT HANDLERS
	//
	//////////////////////////////////////////////////
	void OnButtonOK(wxCommandEvent& evt);
	void OnButtonCancel(wxCommandEvent& evt);
	void OnButtonGotcha(wxCommandEvent& evt);

	void OnTab_About(wxCommandEvent& evt);
	void OnTab_Intro(wxCommandEvent& evt);

	void OnDlgClose(wxCloseEvent& evt);

protected:
	DECLARE_EVENT_TABLE()
};