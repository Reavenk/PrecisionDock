#pragma once
#include <wx/wx.h>

/// <summary>
/// Misc wxWidget utilities.
/// 
/// These are various utilities that are collected in a single location. This
/// should not be confused with AppUtils, which are utilities designed to create
/// consistent app behaviour.
/// </summary>
namespace mywxUtils
{
	/// <summary>
	/// Load Windows resource ID and generated a wxBitmap from it.
	/// </summary>
	wxBitmap LoadFromResourceID(int resourceID);

	/// <summary>
	/// wxStaticText has an issue that it intercepts and eats UI events. If
	/// we want to use a wxStaticText just for visual elements (that can also be
	/// used with wxSizers), this function can setup mouse events to dummy functions
	/// that delegate the events to the wxStaticText's parent windows.
	/// 
	/// For the most part this works, but note there may be edge cases.
	/// </summary>
	/// <param name="stxt">
	/// The wxStaticText that shouldn't eat up UI events.
	/// </param>
	void SetupStaticTextMousePassthrough(wxStaticText* stxt);

	/// <summary>
	/// Make a window visible and obvious to the user.
	/// </summary>
	/// <param name="win">The window to raise.</param>
	void RaiseWindowToAttention(wxTopLevelWindow* topLevelWin);
}