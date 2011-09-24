#ifndef _AUDIO_QUALITY_IDENT_H_
#define _AUDIO_QUALITY_IDENT_H_

#include <memory>
#include <string>

#include "third_party/chromium/base/basictypes.h"
#include "third_party/chromium/base/memory/ref_counted.h"

//------------------------------------------------------------------------------
struct ICorePlayer;
struct IAudioInformationExtracter;
class PersistentMap;
class AudioQualityIdent
{
public:
    AudioQualityIdent(const std::wstring& resultDir, PersistentMap* persResult);
    ~AudioQualityIdent();

    bool Init();
    void Identify(const std::wstring& fileName);

private:
    DISALLOW_COPY_AND_ASSIGN(AudioQualityIdent);

    std::unique_ptr<void, void (__stdcall*)(void*)> funcHost_;
    scoped_refptr<ICorePlayer> mediaInfo_;
    scoped_refptr<IAudioInformationExtracter> spectrumSource_;
    PersistentMap* persResult_;
};

#endif  // _AUDIO_QUALITY_IDENT_H_