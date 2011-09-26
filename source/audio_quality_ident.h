#ifndef _AUDIO_QUALITY_IDENT_H_
#define _AUDIO_QUALITY_IDENT_H_

#include <memory>
#include <string>

#include "third_party/chromium/base/basictypes.h"
#include "third_party/chromium/base/memory/ref_counted.h"
#include "third_party/chromium/base/synchronization/cancellation_flag.h"

//------------------------------------------------------------------------------
struct ICorePlayer;
struct IAudioInformationExtracter;
class AudioQualityIdent
{
public:
    explicit AudioQualityIdent(
        const std::shared_ptr<base::CancellationFlag>& cancelFlag);
    ~AudioQualityIdent();

    bool Init();
    bool Identify(const std::wstring& fullPathName, int* sampleRate,
                  int* bitrate, int* channels, int* cutoff, int64* duration,
                  std::wstring* format);

private:
    DISALLOW_COPY_AND_ASSIGN(AudioQualityIdent);

    std::unique_ptr<void, void (__stdcall*)(void*)> funcHost_;
    scoped_refptr<ICorePlayer> mediaInfo_;
    scoped_refptr<IAudioInformationExtracter> spectrumSource_;
    std::shared_ptr<base::CancellationFlag> cancelFlag_;
};

#endif  // _AUDIO_QUALITY_IDENT_H_