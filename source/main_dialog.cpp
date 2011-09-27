#include "main_dialog.h"

#include <string>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <afxdialogex.h>

#include "my_app.h"
#include "preference.h"
#include "progress_dialog.h"
#include "audio_quality_ident.h"
#include "persistent_map.h"
#include "third_party/chromium/base/synchronization/cancellation_flag.h"

using std::wstring;
using std::unique_ptr;
using std::shared_ptr;
using boost::algorithm::trim_right;
using boost::filesystem3::path;
using base::CancellationFlag;

namespace {
int __stdcall BrowseCallbackProc(HWND winHandle, UINT message, LPARAM param,
                                 LPARAM data)
{
    switch (message) {
        case BFFM_INITIALIZED:
            SendMessage(winHandle, BFFM_SETSELECTION, TRUE, data);
            SendMessage(winHandle, BFFM_SETEXPANDED, TRUE, data);
            break;
        case BFFM_VALIDATEFAILED:
            return 1;
        default:
            break;
    }

    return 0;
}

class Intermedia : public DirTraversing::Callback
{
public:
    Intermedia(DirTraversing::Callback* callback, const wstring& resultDir,
               const shared_ptr<CancellationFlag>& cancelFlag)
        : callback_(callback)
        , ident_(cancelFlag)
        , initialized_(false)
        , persResult_()
        , resultDir_(resultDir)
    {
    }
    ~Intermedia() {}

    virtual void Initializing(int totalFiles)
    {
        if (initialized_)
            return;

        callback_->Initializing(totalFiles);
        persResult_ = PersistentMap::CreateInstance(resultDir_);
        initialized_ = ident_.Init();
    }

    virtual bool Progress(const std::wstring& current)
    {
        if (!initialized_)
            return false;

        bool rv = callback_->Progress(current);
        if (!rv)
            return rv;

        PersistentMap::ContainerType& persistentMap = persResult_->GetMap();
        wstring fileName(path(current).filename().wstring());
        auto iter = persistentMap.find(fileName);
        if (iter != persistentMap.end())
            return rv;

        int sampleRate;
        int bitrate;
        int channels;
        int cutoff;
        int64 duration;
        wstring format;
        try {
            if (ident_.Identify(current, &sampleRate, &bitrate, &channels,
                                &cutoff, &duration, &format))
                persistentMap.insert(
                    PersistentMap::ContainerType::value_type(
                        fileName,
                        PersistentMap::MediaInfo(sampleRate, bitrate, channels,
                                                 cutoff, duration, format)));
        } catch (const std::exception&) {
            persistentMap.insert(
                PersistentMap::ContainerType::value_type(
                    fileName,
                    PersistentMap::MediaInfo(0, 0, 0, 0, 0,
                                             wstring(L"[Crash]"))));
            return rv;
        }

        return rv;
    }

    virtual void Done()
    {
        callback_->Done();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Intermedia);

    DirTraversing::Callback* callback_;
    AudioQualityIdent ident_;
    bool initialized_;
    shared_ptr<PersistentMap> persResult_;
    wstring resultDir_;
};
}

MainDialog::MainDialog(CWnd* parent)
    : CDialogEx(MainDialog::IDD, parent)
    , icon_(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
    , audioDir_()
    , browseAudio_()
    , resultDir_()
    , browseResult_()
    , dirTraversing_()
{
}

MainDialog::~MainDialog()
{
}

BEGIN_MESSAGE_MAP(MainDialog, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_CBN_KILLFOCUS(IDC_COMBO_AUDIO_DIR,
                     &MainDialog::OnCbnKillfocusComboAudioDir)
    ON_CBN_KILLFOCUS(IDC_COMBO_RESULT_DIR,
                     &MainDialog::OnCbnKillfocusComboResultDir)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_AUDIO,
                  &MainDialog::OnBnClickedButtonBrowseAudioDir)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_RESULT,
                  &MainDialog::OnBnClickedButtonBrowseResultDir)
    ON_BN_CLICKED(IDC_BUTTON_START,
                  &MainDialog::OnBnClickedButtonStart)
END_MESSAGE_MAP()

void MainDialog::DoDataExchange(CDataExchange* dataExch)
{
    CDialogEx::DoDataExchange(dataExch);
    DDX_Control(dataExch, IDC_COMBO_AUDIO_DIR, audioDir_);
    DDX_Control(dataExch, IDC_BUTTON_BROWSE_AUDIO, browseAudio_);
    DDX_Control(dataExch, IDC_COMBO_RESULT_DIR, resultDir_);
    DDX_Control(dataExch, IDC_BUTTON_BROWSE_RESULT, browseResult_);
}

BOOL MainDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(icon_, TRUE);            // Set big icon
    SetIcon(icon_, FALSE);        // Set small icon

    const wchar_t* defaultDir = L"c:/";
    wstring d = Preference::GetInstance()->GetAudioDir();
    audioDir_.SetWindowText(d.empty() ? defaultDir : d.c_str());
    if (d.empty())
        Preference::GetInstance()->SetAudioDir(wstring(defaultDir));

    d = Preference::GetInstance()->GetResultDir();
    resultDir_.SetWindowText(d.empty() ? defaultDir : d.c_str());
    if (d.empty())
        Preference::GetInstance()->SetResultDir(wstring(defaultDir));

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

void MainDialog::OnCbnKillfocusComboAudioDir()
{
    CString curSetting;
    audioDir_.GetWindowText(curSetting);
    Preference::GetInstance()->SetAudioDir(wstring(curSetting.GetBuffer()));
}

void MainDialog::OnCbnKillfocusComboResultDir()
{
    CString curSetting;
    resultDir_.GetWindowText(curSetting);
    Preference::GetInstance()->SetResultDir(wstring(curSetting.GetBuffer()));
}

void MainDialog::OnBnClickedButtonBrowseAudioDir()
{
    CString curSetting;
    audioDir_.GetWindowText(curSetting);

    wstring title;
    title.resize(MAX_PATH + 1);

    BROWSEINFO b = {0};
    b.hwndOwner = GetSafeHwnd();
    b.pszDisplayName = &title[0];
    b.lpszTitle = L"Select audio location";
    b.ulFlags = BIF_USENEWUI | BIF_BROWSEINCLUDEFILES;
    b.lpfn = BrowseCallbackProc;
    b.lParam = reinterpret_cast<LPARAM>(curSetting.GetBuffer());

    PIDLIST_ABSOLUTE p = SHBrowseForFolder(&b);
    unique_ptr<void, void (__stdcall*)(void*)> autoRelease(p, CoTaskMemFree);
    if (!p)
        return;

    wstring result;
    result.resize(MAX_PATH + 1);
    if (SHGetPathFromIDList(p, &result[0])) {
        trim_right(result);
        if (audioDir_.FindString(-1, result.c_str()) < 0)
            audioDir_.AddString(result.c_str());

        audioDir_.SetWindowText(result.c_str());
        Preference::GetInstance()->SetAudioDir(result);
        audioDir_.SetFocus();
    }
}

void MainDialog::OnBnClickedButtonBrowseResultDir()
{
    CString curSetting;
    resultDir_.GetWindowText(curSetting);

    wstring title;
    title.resize(MAX_PATH + 1);

    BROWSEINFO b = {0};
    b.hwndOwner = GetSafeHwnd();
    b.pszDisplayName = &title[0];
    b.lpszTitle = L"Select result location";
    b.ulFlags = BIF_USENEWUI | BIF_BROWSEINCLUDEFILES;
    b.lpfn = BrowseCallbackProc;
    b.lParam = reinterpret_cast<LPARAM>(curSetting.GetBuffer());

    PIDLIST_ABSOLUTE p = SHBrowseForFolder(&b);
    unique_ptr<void, void (__stdcall*)(void*)> autoRelease(p, CoTaskMemFree);
    if (!p)
        return;

    wstring result;
    result.resize(MAX_PATH + 1);
    if (SHGetPathFromIDList(p, &result[0])) {
        trim_right(result);
        if (resultDir_.FindString(-1, result.c_str()) < 0)
            resultDir_.AddString(result.c_str());

        resultDir_.SetWindowText(result.c_str());
        Preference::GetInstance()->SetResultDir(result);
        resultDir_.SetFocus();
    }
}

void MainDialog::OnBnClickedButtonStart()
{
    CString text;
    audioDir_.GetWindowText(text);
    wstring audioDir(text.GetBuffer());
    if (audioDir_.FindString(-1, text) < 0)
        audioDir_.AddString(text);

    resultDir_.GetWindowText(text);
    wstring resultDir(text.GetBuffer());
    if (resultDir_.FindString(-1, text) < 0)
        resultDir_.AddString(text);

    ProgressDialog d(this);
    Intermedia inte(&d, resultDir, d.GetCancellationFlag());
    dirTraversing_.Traverse(&inte, audioDir.c_str());
    d.DoModal();
}