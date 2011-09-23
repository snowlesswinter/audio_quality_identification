#include "main_dialog.h"

#include <afxdialogex.h>

#include "my_app.h"

MainDialog::MainDialog(CWnd* parent)
    : CDialogEx(MainDialog::IDD, parent)
    , icon_(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
{
}

void MainDialog::DoDataExchange(CDataExchange* dataExch)
{
    CDialogEx::DoDataExchange(dataExch);
}

BEGIN_MESSAGE_MAP(MainDialog, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

BOOL MainDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(icon_, TRUE);            // Set big icon
    SetIcon(icon_, FALSE);        // Set small icon

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void MainDialog::OnPaint()
{
    if (IsIconic()) {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND,
                    reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, icon_);
    } else {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR MainDialog::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(icon_);
}