#pragma once
#include <wx/wx.h>
#include <set>

class DockWin;
class Node;

class TopDockWin: public wxFrame
{
private:
    enum class CMDID
    {
        ToggleStatusbar,
        ReleaseAll,
        DettachAll
    };

    static int      _InstCtr;

private:
    DockWin*        dockWin;
    wxStatusBar*    statusBar = nullptr;

    HWND cachedHWND = NULL;

public:
    TopDockWin(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~TopDockWin();

    Node* SetRoot(HWND hwnd);
    void StealRoot(Node* pn);
    bool HasRoot() const;

    inline DockWin * GetDockWin()
    { return this->dockWin; }

    void ReleaseAll();


    bool _TestValidity();
public:
    static inline int ClassInstCtr(){return _InstCtr;}
    static TopDockWin * GetWinAt(const wxPoint& screenMouse, const std::set<TopDockWin*>& ignores);
    static TopDockWin * GetWinAt(const wxPoint& screenMouse, TopDockWin* ignore);
    static TopDockWin * GetWinAt(const wxPoint& screenMouse);

private:
    void OnMenu_ToggleStatusbar(wxCommandEvent& evt);
    void OnMenu_ReleaseAll(wxCommandEvent& evt);
    void OnExit(wxCommandEvent& event);

protected:
    wxDECLARE_EVENT_TABLE();
};