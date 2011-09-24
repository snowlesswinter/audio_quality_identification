#ifndef _KUGOO_KGPLAYER_AUDIO_SPECTRUM_EXTRACTER_INTERFACE_H_
#define _KUGOO_KGPLAYER_AUDIO_SPECTRUM_EXTRACTER_INTERFACE_H_

#include "unknwn.h"
#include "third_party/chromium/base/basictypes.h"

// {3C9C9066-A3E6-4b22-BEB4-32ABB70147A4}
DEFINE_GUID(IID_IAudioSpectrum, 
0x3c9c9066, 0xa3e6, 0x4b22, 0xbe, 0xb4, 0x32, 0xab, 0xb7, 0x1, 0x47, 0xa4);

interface IAudioSpectrum : public IUnknown
{
public:
    virtual void __stdcall Init(int32 frequencies) = 0;
    virtual int __stdcall GetFrequenciesCount() const = 0;
    virtual int* __stdcall GetFrequencies() const = 0;
};

///////////////////////////////////////////////////////////////////////////////

// {CC875F19-2083-44d7-A287-48C668483383}
DEFINE_GUID(IID_IAudioSpectrumReceiver,
0xcc875f19, 0x2083, 0x44d7, 0xa2, 0x87, 0x48, 0xc6, 0x68, 0x48, 0x33, 0x83);

interface IAudioSpectrumReceiver : public IUnknown
{
public:
    virtual bool __stdcall Receive(IAudioSpectrum* spectrum) = 0;
};

#endif  // _KUGOO_KGPLAYER_AUDIO_SAMPLE_EXTRACTER_INTERFACE_H_