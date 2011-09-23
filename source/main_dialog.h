#ifndef _MAIN_DIALOG_H_
#define _MAIN_DIALOG_H_

#include "mfc_predefine.h"
#include <afxwin.h>

#include "resource/resource.h" // main symbols

class MainDialog : public CDialogEx
{
public:
    MainDialog(CWnd* parent = NULL);    // standard constructor

    enum { IDD = IDD_AUDIO_QUALITY_IDENTIFICATION_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* dataExch);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

private:
    HICON icon_;
};

#endif  // _MAIN_DIALOG_H_