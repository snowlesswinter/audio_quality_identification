#ifndef _KUGOO_KGPLAYER_AUDIO_SAMPLE_EXTRACTER_INTERFACE_H_
#define _KUGOO_KGPLAYER_AUDIO_SAMPLE_EXTRACTER_INTERFACE_H_

#include "chromium/base/basictypes.h"

#include "unknwn.h"

// {A8F4A5E1-108D-4f7e-9456-7B3FC812316C}
DEFINE_GUID(IID_IAudioSample, 
0xa8f4a5e1, 0x108d, 0x4f7e, 0x94, 0x56, 0x7b, 0x3f, 0xc8, 0x12, 0x31, 0x6c);

interface IAudioSample : public IUnknown
{
public:
    virtual void __stdcall Init(int32 bitsPerSample, int32 channels, 
                                int32 samples, int32 sampleRate) = 0;
    virtual void __stdcall SetAvailableSamples(int32 samples) = 0;
    virtual int __stdcall GetBitsPerSample() const = 0;
    virtual int __stdcall GetChannels() const = 0;
    virtual int __stdcall GetSampleRate() const = 0;
    virtual int __stdcall GetSampleCount() const = 0;
    virtual void* __stdcall GetSamples() const = 0;
};

///////////////////////////////////////////////////////////////////////////////

// {E068C79E-F94F-4011-8CF1-4B87ADA5C383}
DEFINE_GUID(IID_IAudioSampleReceiver, 
0xe068c79e, 0xf94f, 0x4011, 0x8c, 0xf1, 0x4b, 0x87, 0xad, 0xa5, 0xc3, 0x83);

interface IAudioSampleReceiver : public IUnknown
{
public:
    virtual int __stdcall GetBlockAlign(int sampleRate) const = 0;
    virtual bool __stdcall Receive(IAudioSample* samples) = 0;
};

#endif  // _KUGOO_KGPLAYER_AUDIO_SAMPLE_EXTRACTER_INTERFACE_H_