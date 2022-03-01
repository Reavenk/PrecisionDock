#pragma once
#include <wx/wx.h>

namespace mywxUtils
{
	wxBitmap LoadFromResourceID(int resourceID);
	void SetupStaticTextMousePassthrough(wxStaticText* stxt);
}