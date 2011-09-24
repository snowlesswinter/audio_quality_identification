#ifndef _KUGOO_KGPLAYER_AUDIO_INFORMATION_EXTRACTER_INTERFACE_H_
#define _KUGOO_KGPLAYER_AUDIO_INFORMATION_EXTRACTER_INTERFACE_H_

#include "unknwn.h"
#include "third_party/chromium/base/basictypes.h"

// {9096BE74-638E-46f7-89AD-B18181F0F546}
DEFINE_GUID(IID_IAudioInformationExtracter, 
0x9096be74, 0x638e, 0x46f7, 0x89, 0xad, 0xb1, 0x81, 0x81, 0xf0, 0xf5, 0x46);

interface IAudioSampleReceiver;
interface IAudioSpectrumReceiver;
interface IAudioInformationExtracter : public IUnknown
{
public:
    virtual bool __stdcall Open(const wchar_t* filePath) = 0;
    virtual bool __stdcall Seek(double seconds) = 0;
    virtual void __stdcall ExtractFormat(int32* bitsPerSample, 
                                         int32* channels,
                                         int32* sampleRate, 
                                         int64* sampleCount,
                                         int32* bitRate) = 0;
    virtual bool __stdcall ExtractResampled(
        int32 bitsPerSample, 
        int32 channels, 
        int32 sampleRate, 
        bool signedSample,
        IAudioSampleReceiver* receiver) = 0;
    virtual bool __stdcall ExtractSpectrum(
        const int32* channelIndexes,
        int32 channelCount,
        int32 windowSize, 
        IAudioSpectrumReceiver** receivers) = 0;
};

#endif  // _KUGOO_KGPLAYER_AUDIO_INFORMATION_EXTRACTER_INTERFACE_H_