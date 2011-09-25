#include "preference.h"

#include <memory>

#include <boost/filesystem.hpp>

using std::wstring;
using std::unique_ptr;
using boost::filesystem3::path;

namespace {
wstring GetStorageFilePathName()
{
    unique_ptr<wchar_t[]> buf(new wchar_t[MAX_PATH]);
    GetModuleFileName(NULL, buf.get(), MAX_PATH);
    return path(buf.get()).remove_filename().wstring() + L"/preference.ini";
}

const wchar_t* audioLoc = L"audio_location";
const wchar_t* resultLoc = L"result_location";
}

Preference* Preference::GetInstance()
{
    return Singleton<Preference>::get();
}

Preference::~Preference()
{
    const wchar_t* appName = L"CONFIG";
    WritePrivateProfileString(appName, audioLoc, audioDir_.c_str(),
                              storageFile_.c_str());
    WritePrivateProfileString(appName, resultLoc, resultDir_.c_str(),
                              storageFile_.c_str());
}

Preference::Preference()
    : storageFile_(GetStorageFilePathName())
    , audioDir_()
    , resultDir_()
{
    const wchar_t* appName = L"CONFIG";
    audioDir_ = GetProfileString(appName, audioLoc, L"");
    resultDir_ = GetProfileString(appName, resultLoc, L"");
}

wstring Preference::GetProfileString(const wchar_t* appName,
                                     const wchar_t* keyName,
                                     const wchar_t* defaultValue)
{
    int bufSize = 128;
    unique_ptr<wchar_t[]> buf;
    int charCopied;
    const int terminatorLength = (!appName || !keyName) ? 2 : 1;
    do {
        bufSize *= 2;
        buf.reset(new wchar_t[bufSize]);
        charCopied = GetPrivateProfileString(appName, keyName, defaultValue, 
                                             buf.get(), bufSize,
                                             storageFile_.c_str());
    } while (charCopied >= (bufSize - terminatorLength));

    wstring result;
    if (!charCopied)
        return result;

    result.resize(charCopied);
    memcpy(&result[0], buf.get(), charCopied * sizeof(buf[0]));
    return result;
}