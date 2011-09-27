#include "my_app.h"

#include <sstream>
#include <string>
#include <memory>

#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <dbghelp.h>

#include "main_dialog.h"
#include "persistent_map.h"
#include "third_party/chromium/base/at_exit.h"

using std::wstringstream;
using std::wstring;
using std::unique_ptr;
using boost::posix_time::ptime;
using boost::posix_time::second_clock;
using boost::filesystem3::path;

BEGIN_MESSAGE_MAP(AudioQualityIdentificationApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

namespace
{
base::AtExitManager* atExit = NULL;

LONG __stdcall MyUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    ptime now = second_clock::local_time();

    PersistentMapGlobal::GetInstance()->EmergencySerialize();

    unique_ptr<wchar_t[]> buf(new wchar_t[MAX_PATH]);
    GetModuleFileName(NULL, buf.get(), MAX_PATH);
    path p(buf.get());
    wstring dumpFileName = p.remove_filename().wstring() + L"/AQI";

    wstringstream dumpFileNameStream;
    dumpFileNameStream << dumpFileName << '-' << to_iso_wstring(now) << L".dmp";
    HANDLE dumpFileHandle = CreateFile(dumpFileNameStream.str().c_str(),
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0,
                                       0);
    if (INVALID_HANDLE_VALUE == dumpFileHandle)
        return EXCEPTION_EXECUTE_HANDLER;

    unique_ptr<void, void (__stdcall*)(void*)> autoClose(
        dumpFileHandle,
        reinterpret_cast<void (__stdcall*)(void*)>(CloseHandle));

    MINIDUMP_EXCEPTION_INFORMATION miniDumpInfo = {0};
    miniDumpInfo.ThreadId = GetCurrentThreadId();
    miniDumpInfo.ExceptionPointers = exceptionInfo;
    miniDumpInfo.ClientPointers = TRUE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                      dumpFileHandle, MiniDumpNormal, &miniDumpInfo, NULL,
                      NULL);

    return EXCEPTION_EXECUTE_HANDLER;
}
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

    SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

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