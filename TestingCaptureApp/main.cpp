// wxWidgets "Hello world" Program
// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
class MyApp: public wxApp
{
public:
    virtual bool OnInit();
};
class MyFrame: public wxFrame
{
public:
    wxTextCtrl* inputTitle;
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
private:
    void OnHello(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    void OnBtn_ChangeTitle(wxCommandEvent& event);
    void OnBtn_Close(wxCommandEvent& event);
    wxDECLARE_EVENT_TABLE();
};
enum
{
    ID_Hello = 1,
    ID_ChangeTitle,
    ID_Close
};
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(ID_Hello,          MyFrame::OnHello)
EVT_MENU(wxID_EXIT,         MyFrame::OnExit)
EVT_MENU(wxID_ABOUT,        MyFrame::OnAbout)
EVT_BUTTON(ID_ChangeTitle,  MyFrame::OnBtn_ChangeTitle)
EVT_BUTTON(ID_Close,        MyFrame::OnBtn_Close)
wxEND_EVENT_TABLE()
wxIMPLEMENT_APP(MyApp);
bool MyApp::OnInit()
{
    MyFrame *frame = new MyFrame( "Hello World", wxPoint(50, 50), wxSize(450, 340) );
    frame->Show( true );
    return true;
}
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, title, pos, size)
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
        "Help string shown in status bar for this menu item");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, "&File" );
    menuBar->Append( menuHelp, "&Help" );
    SetMenuBar( menuBar );
    CreateStatusBar();
    SetStatusText( "Welcome to wxWidgets!" );

    wxBoxSizer * bsMain = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(bsMain);

    this->inputTitle = new wxTextCtrl(this, -1, "Test titlename");
    wxButton* btnChangeTitle = new wxButton(this, ID_ChangeTitle, "Change Title");
    wxBoxSizer* bsTitleChange = new wxBoxSizer(wxHORIZONTAL);
    bsTitleChange->Add(this->inputTitle, 1, wxGROW);
    bsTitleChange->Add(btnChangeTitle, 0, 0);
    bsMain->Add(bsTitleChange, 0, wxGROW);

    wxButton* btnClose = new wxButton(this, ID_Close, "Close");
    bsMain->Add(btnClose, 0, wxGROW);

    this->Layout();
}

void MyFrame::OnExit(wxCommandEvent& event)
{
    Close( true );
}

void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox( "This is a wxWidgets' Hello world sample",
        "About Hello World", wxOK | wxICON_INFORMATION );
}

void MyFrame::OnBtn_ChangeTitle(wxCommandEvent& event)
{
    this->SetTitle(this->inputTitle->GetValue());
}

void MyFrame::OnBtn_Close(wxCommandEvent& event)
{
    this->Close();
}

void MyFrame::OnHello(wxCommandEvent& event)
{
    wxLogMessage("Hello world from wxWidgets!");
}