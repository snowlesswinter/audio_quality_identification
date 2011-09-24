#ifndef _PREFERENCE_H_
#define _PREFERENCE_H_

#include <string>

#include "third_party/chromium/base/memory/singleton.h"

class Preference
{
public:
    static Preference* GetInstance();

    ~Preference();

    const std::wstring& GetAudioDir() const { return audioDir_; }
    void SetAudioDir(const std::wstring& d) { audioDir_ = d; }
    const std::wstring& GetResultDir() const { return resultDir_; }
    void SetResultDir(const std::wstring& d) { resultDir_ = d; }

private:
    friend struct DefaultSingletonTraits<Preference>;

    DISALLOW_COPY_AND_ASSIGN(Preference);
    Preference();

    std::wstring GetProfileString(const wchar_t* appName,
                                  const wchar_t* keyName,
                                  const wchar_t* defaultValue);

    std::wstring storageFile_;
    std::wstring audioDir_;
    std::wstring resultDir_;
};

#endif  // _PREFERENCE_H_