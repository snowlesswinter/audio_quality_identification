#include "my_app.h"

#include "main_dialog.h"
#include "third_party/chromium/base/at_exit.h"

BEGIN_MESSAGE_MAP(AudioQualityIdentificationApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

namespace
{
base::AtExitManager* atExit = NULL;
}

AudioQualityIdentificationApp theApp;

AudioQualityIdentificationApp::AudioQualityIdentificationApp()
{
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

BOOL AudioQualityIdentificationApp::InitInstance()
{
    assert(!atExit);
    atExit = new base::AtExitManager;

    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX initControls;
    initControls.dwSize = sizeof(initControls);

    // Set this to include all the common control classes you want to use
    // in your application.
    initControls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&initControls);

    CWinApp::InitInstance();

    MainDialog dialog;
    m_pMainWnd = &dialog;
    INT_PTR response = dialog.DoModal();
    if (response == IDOK) {
        // TODO: Place code here to handle when the dialog is
        // dismissed with OK
    } else if (response == IDCANCEL) {
        // TODO: Place code here to handle when the dialog is
        // dismissed with Cancel
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}

int AudioQualityIdentificationApp::ExitInstance()
{
    int result = CWinApp::ExitInstance();

    assert(atExit);
    if (atExit) {
        delete atExit;
        atExit = NULL;
    }

    return result;
}