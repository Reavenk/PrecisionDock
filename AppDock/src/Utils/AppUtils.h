#pragma once
#include <wx/wx.h>

/// <summary>
/// Misc utilities shared amonst different parts of the application.
/// 
/// These utilities specifically focus on making different parts of 
/// the application consistent.
/// </summary>
namespace AppUtils
{ 
	/// <summary>
	/// Sets the Window icon set for a Window.
	/// </summary>
	/// <param name="win">The window to set the icons for.</param>
	/// <param name="resourceID">The icon's resource ID.</param>
	/// <param name="large">Specify whether to use the big or small icon in the set.</param>
	void SetIcon(wxWindow* win, int resourceID, bool large);

	/// <summary>
	/// The function is used to set Window icons to the same set across
	/// the entire application.
	/// </summary>
	/// <param name="win">The window to apply the icon set for.</param>
	void SetDefaultIcons(wxWindow* win);

	void SetWindowTransparency(wxWindow* win, int alpha);

	void SetWindowTransparency(HWND hwnd, int alpha);
}