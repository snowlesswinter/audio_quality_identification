#ifndef _MAIN_DIALOG_H_
#define _MAIN_DIALOG_H_

#include "mfc_predefine.h"
#include <afxwin.h>

#include "resource/resource.h" // main symbols
#include "dir_traversing.h"

class MainDialog : public CDialogEx
{
public:
    MainDialog(CWnd* parent = NULL);    // standard constructor
    virtual ~MainDialog();

    enum { IDD = IDD_AUDIO_QUALITY_IDENTIFICATION_DIALOG };

protected:
    DECLARE_MESSAGE_MAP()

private:
    virtual void DoDataExchange(CDataExchange* dataExch);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnCbnKillfocusComboAudioDir();
    afx_msg void OnCbnKillfocusComboResultDir();
    afx_msg void OnBnClickedButtonBrowseAudioDir();
    afx_msg void OnBnClickedButtonBrowseResultDir();
    afx_msg void OnBnClickedButtonStart();
    afx_msg void OnBnClickedButtonAnalyze();

    HICON icon_;
    CComboBox audioDir_;
    CButton browseAudio_;
    CComboBox resultDir_;
    CButton browseResult_;
    DirTraversingProxy dirTraversing_;
};

#endif  // _MAIN_DIALOG_H_