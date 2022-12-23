#include "AppUtils.h"
#include "../resource.h"

namespace AppUtils
{ 
    void SetIcon(wxWindow* win, int resourceID, bool large)
    {
        // As a Windows exclusive program, it's a lot easier to get
        // high quality icons by using Windows (Visual Studio IDE's) 
        // resource system than wxWidgets.

        HICON hIcon = 
            (HICON)LoadImage(
                GetModuleHandle(0), 
                MAKEINTRESOURCE(resourceID), 
                IMAGE_ICON, 
                0, 
                0, 
                LR_LOADTRANSPARENT); 

        // https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-seticon
        SendMessage(
            win->GetHWND(), 
            WM_SETICON, 
            large ? ICON_BIG : ICON_SMALL, 
            (LPARAM)hIcon);
    }

    void SetDefaultIcons(wxWindow* win)
    {
        SetIcon(win, IDI_ICON1, false);
        SetIcon(win, IDI_ICON4, true);
    }

    void SetWindowTransparency(wxWindow* win, int alpha)
    {
        HWND hwnd = win->GetHWND();
        SetWindowTransparency(hwnd, alpha);
    }

    void SetWindowTransparency(HWND hwnd, int alpha)
    {
        LONG style = GetWindowLong(hwnd,GWL_EXSTYLE);
        SetWindowLong(hwnd,GWL_EXSTYLE,style|WS_EX_LAYERED);

        if (! SetLayeredWindowAttributes(hwnd,0, alpha, LWA_ALPHA) )
        {
            //DWORD error = GetLastError();
            wxLogDebug("Could not set window transparency for DragPreviewOlyWin");
        }
    }
}