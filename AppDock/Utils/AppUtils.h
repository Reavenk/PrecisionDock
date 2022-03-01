#pragma once
#include <wx/wx.h>

namespace AppUtils
{ 
	void SetIcon(wxWindow* win, int resourceID, bool large);
	void SetDefaultIcons(wxWindow* win);
}