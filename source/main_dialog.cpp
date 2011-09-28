#include "main_dialog.h"

#include <string>
#include <memory>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
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
using std::wofstream;
using std::endl;
using boost::algorithm::trim_right;
using boost::filesystem3::path;
using boost::lexical_cast;
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

void LimitPricision(wstring* s)
{
    auto pos = s->rfind('.');
    if (pos != wstring::npos)
        *s = s->substr(0, pos + 3);
}

void CreateBitrateProportionReport(const PersistentMap::ContainerType& result,
                                   const wstring& resultDir)
{
    struct { int Baseline; int Count; } bitrateCounts[] =
    {
        { 318000, 0 }, { 222000, 0 }, { 190000, 0 }, { 126000, 0 },
        { 94000, 0 }, { 62000, 0 }, { 30000, 0 }, { 0, 0 }, { -1, 0 }
    };

    for (auto i = result.begin(), e = result.end(); i != e; ++i) {
        const PersistentMap::MediaInfo& info = i->second;
        if (info.Format.empty() || (L"[Unknown]" == info.Format) ||
            (L"[Crash]" == info.Format)) {
            bitrateCounts[arraysize(bitrateCounts) - 1].Count++;
            continue;
        }

        for (int j = 0; j < arraysize(bitrateCounts); ++j) {
            if (info.Bitrate >= bitrateCounts[j].Baseline) {
                bitrateCounts[j].Count++;
                break;
            }
        }
    }

    if (result.empty())
        return;

    wofstream output(
        path(resultDir + L"/").remove_filename().wstring() +
            L"/bitrate_proportion_report.txt");

    // Print header.
    const wstring field1 = L"Bitrate(kbps)";
    const wstring field2 = L"Proportion";
    const int column1Width = field1.length() + 6;
    const int column2Width = field2.length() + 3;
    output << field1;
    output.width(6 + field2.length());
    output << field2 << endl;
    output.fill('-');
    output.width(29);
    output << '-' << endl;
    output.fill(' ');

    for (int i = 0; i < arraysize(bitrateCounts); ++i) {
        const int upper = i ? bitrateCounts[i - 1].Baseline / 1000 : 0;
        const int lower = bitrateCounts[i].Baseline / 1000;
        wstring bitrateDesc =
            upper ?
                (lexical_cast<wstring>(lower) + L" - " +
                    lexical_cast<wstring>(upper)) :
                (L"above " + lexical_cast<wstring>(lower));

        wstring v1 =
            (arraysize(bitrateCounts) - 1 == i) ?
            L"Crash or Unknown" : bitrateDesc;
        wstring v2 =
            lexical_cast<wstring>(
                bitrateCounts[i].Count * 100.0f / result.size());

        LimitPricision(&v2);
        v2 += '%';

        output << v1;
        output.width(column1Width - v1.length() + v2.length());
        output << v2 << endl;
    }
}

void CreateBitrateToCutoffReport(const PersistentMap::ContainerType& result,
                                 const wstring& resultDir)
{
    std::map<int, std::pair<double, int>> bitrateToCutoff;
    for (auto i = result.begin(), e = result.end(); i != e; ++i) {
        const PersistentMap::MediaInfo& info = i->second;
        if (info.Format.empty() || (L"[Unknown]" == info.Format) ||
            (L"[Crash]" == info.Format))
            continue;

        const int bitrate = info.Bitrate / 1000;

        auto iter = bitrateToCutoff.find(bitrate);
        if (iter == bitrateToCutoff.end()) {
            bitrateToCutoff.insert(
                std::pair<int, std::pair<double, int>>(
                    bitrate, std::pair<double, int>(info.CutoffFreq, 1)));
        } else {
            iter->second.second++;
            iter->second.first += info.CutoffFreq;
        }
    }

    wofstream output(
        path(resultDir + L"/").remove_filename().wstring() +
            L"/bitrate_cutoff_report.txt");

    // Print header.
    const wstring field1 = L"Bitrate(kbps)";
    const wstring field2 = L"Quantity";
    const wstring field3 = L"Cutoff Frequency(kHz)";
    const int column1Width = field1.length() + 3;
    const int column2Width = field2.length() + 3;
    const int column3Width = field3.length() + 3;
    output << field1;
    output.width(3 + field2.length());
    output << field2;
    output.width(3 + field3.length());
    output << field3 << endl;
    output.fill('-');
    output.width(59);
    output << '-' << endl;
    output.fill(' ');

    for (auto i = bitrateToCutoff.begin(), e = bitrateToCutoff.end(); i != e;
        ++i) {
        assert(i->second.second);

        wstring v1 = lexical_cast<wstring>(i->first);
        wstring v2 = lexical_cast<wstring>(i->second.second);
        wstring v3 =
            lexical_cast<wstring>(i->second.first / i->second.second / 1000);

        LimitPricision(&v3);

        output << v1;
        output.width(column1Width - v1.length() + v2.length());
        output << v2;
        output.width(column2Width - v2.length() + v3.length());
        output << v3 << endl;
    }
}

void CreateFakeHighQualityReport(const PersistentMap::ContainerType& result,
                                 const wstring& resultDir, int lowerBitrate,
                                 int upperBitrate, int minCutoff)
{
    const bool upperNotSet = upperBitrate < lowerBitrate;
    wstring bitrateDesc =
        upperNotSet ?
            (L"above_" + lexical_cast<wstring>(lowerBitrate / 1000)) :
            (lexical_cast<wstring>(lowerBitrate / 1000) + L"_" +
                lexical_cast<wstring>(upperBitrate / 1000));
    wofstream output(
        path(resultDir + L"/").remove_filename().wstring() +
            L"/fake_high_quality_" + bitrateDesc + L"_" +
            lexical_cast<wstring>(minCutoff / 1000) + L"_report.txt");

    // Print header.
    const wstring field1 = L"File Name";
    const wstring field2 = L"Bitrate(kbps)";
    const wstring field3 = L"Cutoff Frequency(kHz)";
    const wstring field4 = L"Duration(sec)";
    const int column1Width = field1.length() + 30;
    const int column2Width = field2.length() + 3;
    const int column3Width = field3.length() + 8;
    const int column4Width = field4.length() + 3;
    output << field1;
    output.width(30 + field2.length());
    output << field2;
    output.width(3 + field3.length());
    output << field3;
    output.width(8 + field4.length());
    output << field4 << endl;

    const int seperatorWidth = 113;
    output.fill('-');
    output.width(seperatorWidth - 1);
    output << '-' << endl;
    output.fill(' ');

    int totalCount = 0;
    int scopeCount = 0;
    int fakeCount = 0;
    for (auto i = result.begin(), e = result.end(); i != e; ++i) {
        const PersistentMap::MediaInfo& info = i->second;
        if (info.Format.empty() || (L"[Unknown]" == info.Format) ||
            (L"[Crash]" == info.Format))
            continue;

        totalCount++;

        if ((info.Bitrate < lowerBitrate) ||
            (!upperNotSet && (info.Bitrate > upperBitrate)))
            continue;

        scopeCount++;

        if (info.CutoffFreq > minCutoff)
            continue;

        wstring v1 = i->first;
        wstring v2 = lexical_cast<wstring>(info.Bitrate / 1000);
        wstring v3 = lexical_cast<wstring>(info.CutoffFreq / 1000.0);
        wstring v4 = lexical_cast<wstring>(info.Duration / 10000000.0);

        LimitPricision(&v3);
        LimitPricision(&v4);

        output << v1;
        output.width(column1Width - v1.length() + v2.length());
        output << v2;
        output.width(column2Width - v2.length() + v3.length());
        output << v3;
        output.width(column3Width - v3.length() + v4.length());
        output << v4 << endl;

        fakeCount++;
    }

    output.fill('-');
    output.width(seperatorWidth - 1);
    output << '-' << endl;
    output.fill(' ');

    // Output footer.
    wstring fake = L"Fake: " + lexical_cast<wstring>(fakeCount);
    wstring total = L"Total: " + lexical_cast<wstring>(scopeCount);
    wstring percentage = L"Fake Percentage: " +
        (scopeCount ?
            lexical_cast<wstring>(fakeCount * 100.0f / scopeCount) : L"-");
    LimitPricision(&percentage);
    percentage += '%';

    wstring percToAll = L"Percentage To All: " +
        (totalCount ?
            lexical_cast<wstring>(fakeCount * 100.0f / totalCount) : L"-");
    LimitPricision(&percToAll);
    percToAll += '%';

    output << fake;
    output.width(column1Width - fake.length() + total.length());
    output << total;
    output.width(column2Width - total.length() + percentage.length());
    output << percentage;
    output.width(column3Width - percentage.length() + percToAll.length());
    output << percToAll << endl;
}

void CreateDurationProportionReport(const PersistentMap::ContainerType& result,
                                    const wstring& resultDir)
{
    struct { int64 Baseline; int Count; } durationCounts[] =
    {
        { 6000000000I64, 0 }, { 3600000000I64, 0 }, { 1800000000I64, 0 },
        { 600000000I64, 0 }, { 300000000I64, 0 }, { 0I64, 0 }
    };

    int total = 0;
    for (auto i = result.begin(), e = result.end(); i != e; ++i) {
        const PersistentMap::MediaInfo& info = i->second;
        if (info.Format.empty() || (L"[Unknown]" == info.Format) ||
            (L"[Crash]" == info.Format))
            continue;

        total++;
        for (int j = 0; j < arraysize(durationCounts); ++j) {
            if (info.Duration >= durationCounts[j].Baseline) {
                durationCounts[j].Count++;
                break;
            }
        }
    }

    if (!total)
        return;

    wofstream output(
        path(resultDir + L"/").remove_filename().wstring() +
            L"/duration_proportion_report.txt");

    // Print header.
    const wstring field1 = L"Duration(sec)";
    const wstring field2 = L"Proportion";
    const int column1Width = field1.length() + 6;
    const int column2Width = field2.length() + 3;
    output << field1;
    output.width(6 + field2.length());
    output << field2 << endl;
    output.fill('-');
    output.width(29);
    output << '-' << endl;
    output.fill(' ');

    for (int i = 0; i < arraysize(durationCounts); ++i) {
        wstring upper =
            lexical_cast<wstring>(
                i ? durationCounts[i - 1].Baseline / 10000000.0 : 0.0);
        wstring lower =
            lexical_cast<wstring>(durationCounts[i].Baseline / 10000000.0);
        LimitPricision(&upper);
        LimitPricision(&lower);

        wstring durationDesc = i ? lower + L" to " + upper : L"over " + lower;
        wstring v1 = durationDesc;
        wstring v2 =
            lexical_cast<wstring>(durationCounts[i].Count * 100.0f / total);

        LimitPricision(&v2);
        v2 += '%';

        output << v1;
        output.width(column1Width - v1.length() + v2.length());
        output << v2 << endl;
    }
}

void CreateChannelProportionReport(const PersistentMap::ContainerType& result,
                                   const wstring& resultDir)
{
    struct { int Baseline; int Count; } channelCounts[] =
    {
        { 3, 0 }, { 2, 0 }, { 1, 0 }
    };

    int total = 0;
    for (auto i = result.begin(), e = result.end(); i != e; ++i) {
        const PersistentMap::MediaInfo& info = i->second;
        if (info.Format.empty() || (L"[Unknown]" == info.Format) ||
            (L"[Crash]" == info.Format))
            continue;

        total++;
        for (int j = 0; j < arraysize(channelCounts); ++j) {
            if (info.Channels >= channelCounts[j].Baseline) {
                channelCounts[j].Count++;
                break;
            }
        }
    }

    if (!total)
        return;

    wofstream output(
        path(resultDir + L"/").remove_filename().wstring() +
            L"/channel_proportion_report.txt");

    // Print header.
    const wstring field1 = L"Channels";
    const wstring field2 = L"Proportion";
    const int column1Width = field1.length() + 8;
    const int column2Width = field2.length() + 3;
    output << field1;
    output.width(8 + field2.length());
    output << field2 << endl;
    output.fill('-');
    output.width(29);
    output << '-' << endl;
    output.fill(' ');

    for (int i = 0; i < arraysize(channelCounts); ++i) {
        wstring channel =
            i ?
                lexical_cast<wstring>(channelCounts[i].Baseline) :
                L"More Than 2";
        wstring proportion =
            lexical_cast<wstring>(channelCounts[i].Count * 100.0f / total);

        LimitPricision(&proportion);
        proportion += '%';

        output << channel;
        output.width(column1Width - channel.length() + proportion.length());
        output << proportion << endl;
    }
}

void CreateSampleRateProportionReport(
    const PersistentMap::ContainerType& result, const wstring& resultDir)
{
    int total = 0;
    std::map<int, int> rateToCount;
    for (auto i = result.begin(), e = result.end(); i != e; ++i) {
        const PersistentMap::MediaInfo& info = i->second;
        if (info.Format.empty() || (L"[Unknown]" == info.Format) ||
            (L"[Crash]" == info.Format))
            continue;

        total++;
        auto iter = rateToCount.find(info.SampleRate);
        if (iter == rateToCount.end())
            rateToCount.insert(std::pair<int, int>(info.SampleRate, 1));
        else
            iter->second++;
    }

    if (!total)
        return;

    wofstream output(
        path(resultDir + L"/").remove_filename().wstring() +
            L"/sample_rate_proportion_report.txt");

    // Print header.
    const wstring field1 = L"Sample Rate(Hz)";
    const wstring field2 = L"Proportion";
    const int column1Width = field1.length() + 3;
    const int column2Width = field2.length() + 3;
    output << field1;
    output.width(3 + field2.length());
    output << field2 << endl;
    output.fill('-');
    output.width(29);
    output << '-' << endl;
    output.fill(' ');

    for (auto i = rateToCount.begin(), e = rateToCount.end(); i != e; ++i) {
        wstring rate = lexical_cast<wstring>(i->first);
        wstring proportion = lexical_cast<wstring>(i->second * 100.0f / total);
        LimitPricision(&proportion);
        proportion += '%';

        output << rate;
        output.width(column1Width - rate.length() + proportion.length());
        output << proportion << endl;
    }
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
    ON_BN_CLICKED(IDC_BUTTON_ANALYZE,
                  &MainDialog::OnBnClickedButtonAnalyze)
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

void MainDialog::OnBnClickedButtonAnalyze()
{
    CString text;
    resultDir_.GetWindowText(text);
    wstring resultDir(text.GetBuffer());
    if (resultDir_.FindString(-1, text) < 0)
        resultDir_.AddString(text);

    shared_ptr<PersistentMap> pm = PersistentMap::CreateInstance(resultDir);

#if defined(EXCLUDE_SHORT_MUSIC)
    PersistentMap::ContainerType& ref = pm->GetMap();
    PersistentMap::ContainerType result;
    for (auto i = ref.begin(), e = ref.end(); i != e; ++i) {
        const PersistentMap::MediaInfo& info = i->second;
        if (info.Duration > 600000000I64) {
            result.insert(
                PersistentMap::ContainerType::value_type(i->first, info));
        }
    }
#else
    PersistentMap::ContainerType& result = pm->GetMap();
#endif

    CreateBitrateProportionReport(result, resultDir);
    CreateBitrateToCutoffReport(result, resultDir);

    struct { int Bitrate; int Cutoff; } baselines[] =
    {
        { 318000, 18000 }, { 222000, 17000 }, { 190000, 16000 },
        { 126000, 15000 }
    };

    for (int i = 0; i < arraysize(baselines); ++i)
        CreateFakeHighQualityReport(result, resultDir, baselines[i].Bitrate,
                                    i > 0 ? baselines[i - 1].Bitrate : 0,
                                    baselines[i].Cutoff);

    CreateDurationProportionReport(result, resultDir);
    CreateChannelProportionReport(result, resultDir);
    CreateSampleRateProportionReport(result, resultDir);
}