#ifndef _PROGRESS_DIALOG_H_
#define _PROGRESS_DIALOG_H_

#include <string>
#include <memory>

#include "mfc_predefine.h"
#include <afxwin.h>
#include <afxcmn.h>

#include "resource/resource.h" // main symbols
#include "dir_traversing.h"
#include "third_party/chromium/base/synchronization/cancellation_flag.h"

class ProgressDialog : public CDialog, public DirTraversing::Callback
{
public:
    enum { IDD = IDD_DIALOG_PROGRESS };

    ProgressDialog(CWnd* parent);
    virtual ~ProgressDialog();

    virtual void Initializing(int totalFiles);
    virtual bool Progress(const std::wstring& current);
    virtual void Done();

    std::shared_ptr<base::CancellationFlag> GetCancellationFlag();
    void ResetCancellationFlag();

protected:
    virtual void DoDataExchange(CDataExchange* dataExch); // DDX/DDV support
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual LRESULT OnInitializing(WPARAM w, LPARAM l);
    virtual LRESULT OnDone(WPARAM w, LPARAM l);

    DECLARE_MESSAGE_MAP()

private:
    DECLARE_DYNAMIC(ProgressDialog)

    CStatic currentFile_;
    CProgressCtrl progress_;
    int total_;
    int finished_;
    std::shared_ptr<base::CancellationFlag> cancelFlag_;
};

#endif  // _PROGRESS_DIALOG_H_
