#include <Windows.h>
#include <ostream>
#include "Logger.h"

#define FILELOG(x) Logger::LogInst(x);

#define KBM_API __declspec(dllexport) __stdcall __cdecl


// some data will be shared across all
// instances of the DLL
//
#pragma comment(linker, "/SECTION:.SHARED,RWS")
#pragma data_seg(".SHARED")
int iKeyCount = 0;
HHOOK hKeyboardHook = 0;
HHOOK hMouseHook = 0;
#pragma data_seg()

//
// instance specific data
//
HMODULE hInstance = 0;

//
// DLL load/unload entry point
//
BOOL APIENTRY DllMain(HANDLE hModule, 
    DWORD  dwReason, 
    LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH :
        FILELOG("DLL_PROCESS_ATTACH");
        hInstance = (HINSTANCE) hModule;
        break;

    case DLL_THREAD_ATTACH :
        FILELOG("DLL_THREAD_ATTACH");
        break;

    case DLL_THREAD_DETACH :
        FILELOG("DLL_THREAD_DETACH");
        break;

    case DLL_PROCESS_DETACH :
        FILELOG("DLL_PROCESS_DETACH");
        break;
    }
    return TRUE;
}

//
// keyboard hook
//
LRESULT CALLBACK KeyboardProc(int code,       // hook code
    WPARAM wParam,  // virtual-key code
    LPARAM lParam)  // keystroke-message information
{
    if ((lParam & 0x80000000) != 0)
    {
        ++iKeyCount;
    }
    return CallNextHookEx(hKeyboardHook, code, wParam, lParam);
}

//
// mouse hook
//
LRESULT CALLBACK MouseProc(int code,       // hook code
    WPARAM wParam,  // message identifier
    LPARAM lParam)  // mouse coordinates
{
    switch (wParam){
    case WM_LBUTTONDOWN :
        FILELOG("\tMouseProc WM_LBUTTONDOWN");
    case WM_MBUTTONDOWN :
        FILELOG("\tMouseProc WM_MBUTTONDOWN");
    case WM_RBUTTONDOWN :
        FILELOG("\tMouseProc WM_RBUTTONDOWN");
    case WM_LBUTTONDBLCLK :
        FILELOG("\tMouseProc WM_LBUTTONDBLCLK");
    case WM_MBUTTONDBLCLK :
        FILELOG("\tMouseProc WM_MBUTTONDBLCLK");
    case WM_RBUTTONDBLCLK :
        FILELOG("\tMouseProc WM_RBUTTONDBLCLK");
        MessageBeep(0);
        break;
    }
    return CallNextHookEx(hMouseHook, code, wParam, lParam);
}

extern "C"
{

//
// install keyboard/mouse hooks
//
void KBM_API InstallHooks(void)
{
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInstance, 0);
    hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, hInstance, 0);
}

//
// remove keyboard/mouse hooks
//
void KBM_API RemoveHooks(void)
{
    UnhookWindowsHookEx(hKeyboardHook);
    UnhookWindowsHookEx(hMouseHook);
    hKeyboardHook = hMouseHook = 0;
}

//
// retrieve number of keystrokes
//
int KBM_API FetchKeyCount(bool bClear)
{
    int kc = iKeyCount;
    if (bClear)
        iKeyCount = 0;
    return kc;
}

}