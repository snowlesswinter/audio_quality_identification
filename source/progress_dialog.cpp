#include "progress_dialog.h"

#include <cassert>

#include <afxdialogex.h>

namespace {
enum
{
    kMessageInitializing = WM_USER + 100,
    kMessageDone
};
}

IMPLEMENT_DYNAMIC(ProgressDialog, CDialog)

ProgressDialog::ProgressDialog(CWnd* parent)
    : CDialog(ProgressDialog::IDD, parent)
    , currentFile_()
    , progress_()
    , total_(0)
    , finished_(0)
    , cancelFlag_(false)
{
}

ProgressDialog::~ProgressDialog()
{
}

void ProgressDialog::Initializing(int totalFiles)
{
    PostMessage(kMessageInitializing, totalFiles, 0);
}

bool ProgressDialog::Progress(const std::wstring& current)
{
    currentFile_.SetWindowText(current.c_str());
    finished_++;
    progress_.SetPos(finished_);
    return !cancelFlag_;
}

void ProgressDialog::Done()
{
    PostMessage(kMessageDone, 0, 0);
}

void ProgressDialog::DoDataExchange(CDataExchange* dataExch)
{
    CDialog::DoDataExchange(dataExch);
    DDX_Control(dataExch, IDC_STATIC_CURRENT_FILE, currentFile_);
    DDX_Control(dataExch, IDC_PROGRESS1, progress_);
}

BEGIN_MESSAGE_MAP(ProgressDialog, CDialog)
    ON_MESSAGE(kMessageInitializing, OnInitializing)
    ON_MESSAGE(kMessageDone, OnDone)
END_MESSAGE_MAP()

void ProgressDialog::OnCancel()
{
    cancelFlag_ = true;
}

BOOL ProgressDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetWindowText(L"Processing...");
    currentFile_.SetWindowText(L"Calculating the number of files...");
    progress_.SetRange32(0, total_);
    return TRUE;
}

LRESULT ProgressDialog::OnInitializing(WPARAM w, LPARAM l)
{
    assert(w >= 0);

    total_ = w;
    finished_ = 0;
    if (progress_.GetSafeHwnd())
        progress_.SetRange32(0, total_);

    return 0;
}

LRESULT ProgressDialog::OnDone(WPARAM w, LPARAM l)
{
    total_ = 0;
    finished_ = 0;
    cancelFlag_ = false;
    EndDialog(IDOK);
    return 0;
}