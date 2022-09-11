#include "mywxUtils.h"

// TODO: Refactor namespace to have correct caps
namespace mywxUtils
{
	wxBitmap LoadFromResourceID(int resourceID)
	{
		//https://forums.wxwidgets.org/viewtopic.php?t=45918
		HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(resourceID), L"PNG");
		size_t _size = SizeofResource(nullptr, hResource);
		HGLOBAL hMemory = LoadResource(nullptr, hResource);
		LPVOID ptr = LockResource(hMemory);
		wxBitmap my_bitmap = wxBitmap::NewFromPNGData(ptr, _size);
		return my_bitmap;
	}

	static void _PassthroughHandler(wxEvent& evt)
	{
		wxWindow* parent = ((wxWindow*)evt.GetEventObject())->GetParent();
		wxPostEvent(parent, evt);
	}

	void SetupStaticTextMousePassthrough(wxStaticText* stxt)
	{
		// Note there is a danger of frustration by doing this, specifically with the
		// mouse position - as we will be re-passing an event with a mouse local to the
		// StaticText for something else to use. This could cause confusion if they're
		// treated as local to what we're delegating to.
		wxWindow* parent = stxt->GetParent();
		wxEvtHandler* evthParent = parent->GetEventHandler();

		stxt->Bind(wxEVT_LEFT_DOWN,		_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		stxt->Bind(wxEVT_LEFT_UP,		_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		stxt->Bind(wxEVT_LEFT_DCLICK,	_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		//
		stxt->Bind(wxEVT_RIGHT_DOWN,	_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		stxt->Bind(wxEVT_RIGHT_UP,		_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		stxt->Bind(wxEVT_RIGHT_DCLICK,	_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		//
		stxt->Bind(wxEVT_MIDDLE_DOWN,	_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		stxt->Bind(wxEVT_MIDDLE_UP,		_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		stxt->Bind(wxEVT_MIDDLE_DCLICK,	_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		//
		stxt->Bind(wxEVT_ENTER_WINDOW,	_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		stxt->Bind(wxEVT_LEAVE_WINDOW,	_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		//
		// stxt->Bind(wxEVT_MOUSEWHEEL,	_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		// stxt->Bind(wxEVT_MOTION,		_PassthroughHandler, wxID_ANY, wxID_ANY, nullptr);
		// Pass through more events as needed.
	}

	void RaiseWindowToAttention(wxTopLevelWindow* topLevelWin)
	{
		assert(topLevelWin != nullptr);
		topLevelWin->Show(true);
		topLevelWin->Raise();
		topLevelWin->RequestUserAttention();
	}
}