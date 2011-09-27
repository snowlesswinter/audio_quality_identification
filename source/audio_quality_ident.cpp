#include "audio_quality_ident.h"

#include <fstream>
#include <vector>

#include <boost/filesystem.hpp>
#include <windows.h>

#include "third_party/multimedia_core/player_interface.h"
#include "third_party/multimedia_core/audio_information_extracter_interface.h"
#include "third_party/multimedia_core/audio_spectrum_extracter_interface.h"
#include "third_party/multimedia_core/common/unknown_impl.h"

using std::unique_ptr;
using std::wstring;
using std::ifstream;
using std::vector;
using std::max;
using std::shared_ptr;
using boost::filesystem3::path;
using base::CancellationFlag;

namespace {
typedef unique_ptr<void, void (__stdcall*)(void*)> FuncHostType;
typedef HRESULT (__stdcall* MMCoreFactoryProc)(ICorePlayer** , void*);
typedef HRESULT (__stdcall* AudioInfoExtrFactoryProc)(
    IAudioInformationExtracter**);

bool LoadMultiMediaCoreFunctions(
    FuncHostType* funcHost, scoped_refptr<ICorePlayer>* mediaInfo,
    scoped_refptr<IAudioInformationExtracter>* spectrumSource)
{
    // We need multimedia core to help accomplish the work.
    unique_ptr<wchar_t[]> buf(new wchar_t[MAX_PATH]);
    GetModuleFileName(NULL, buf.get(), MAX_PATH);
    path p(buf.get());
    wstring location = p.remove_filename().wstring() + L"/kgplayer.dll";
    FuncHostType dll(LoadLibrary(location.c_str()),
                     reinterpret_cast<void (__stdcall*)(void*)>(FreeLibrary));
    if (!dll)
        return false;

    // Export all necessary functions.
    MMCoreFactoryProc mmCoreFactoryProc =
        reinterpret_cast<MMCoreFactoryProc>(
            GetProcAddress(reinterpret_cast<HMODULE>(dll.get()),
                           reinterpret_cast<char*>(3)));
    if (!mmCoreFactoryProc)
        return false;

    AudioInfoExtrFactoryProc audioInfoExtrFactoryProc =
        reinterpret_cast<AudioInfoExtrFactoryProc>(
            GetProcAddress(reinterpret_cast<HMODULE>(dll.get()),
                           reinterpret_cast<char*>(7)));
    if (!audioInfoExtrFactoryProc)
        return false;

    scoped_refptr<ICorePlayer> interface1;
    HRESULT r = mmCoreFactoryProc(reinterpret_cast<ICorePlayer**>(&interface1),
                                  NULL);
    if (FAILED(r))
        return false;

    scoped_refptr<IAudioInformationExtracter> interface2;
    r = audioInfoExtrFactoryProc(
        reinterpret_cast<IAudioInformationExtracter**>(&interface2));
    if (FAILED(r))
        return false;

    funcHost->swap(dll);
    *mediaInfo = interface1;
    *spectrumSource = interface2;
    return true;
}

class PassString : public kugou::CUnknown, public IPassString
{
public:
    PassString(wstring* str) : kugou::CUnknown(NULL, NULL), str_(str) {}

    DELEGATE_IUNKNOWN;

    virtual const wchar_t* __stdcall GetContent() { return str_->c_str(); };
    virtual void __stdcall SetContent(const wchar_t* content)
    {
        *str_ = content;
    }

private:
    wstring* str_;
};

class MySpectrumReceiver : public kugou::CUnknown, public IAudioSpectrumReceiver
{
public:
    MySpectrumReceiver(int sampleRate,
                       const shared_ptr<base::CancellationFlag>& cancelFlag);
    virtual ~MySpectrumReceiver() {}

    DELEGATE_IUNKNOWN;
    virtual bool __stdcall Receive(IAudioSpectrum* samples);

    int GetAverageFreq()
    {
        vector<int> freqs = freqs_;

        // Exclude some data at the end(about 10 sec).
        int toRemove = 10;
        while (freqs.size() && (toRemove-- >= 0))
            freqs.pop_back();

        double sum = 0.0;
        for (auto i = freqs.begin(), e = freqs.end(); i != e; ++i)
            sum += *i;

        return freqs.empty() ? 0 : static_cast<int>(sum / freqs.size());
    }

private:
    int receiveCount_;
    unique_ptr<double[]> power_;
    vector<int> freqs_;
    int sampleRate_;
    std::shared_ptr<base::CancellationFlag> cancelFlag_;
};

MySpectrumReceiver::MySpectrumReceiver(
    int sampleRate, const shared_ptr<base::CancellationFlag>& cancelFlag)
    : kugou::CUnknown(NULL, NULL)
    , receiveCount_(1)
    , power_()
    , freqs_()
    , sampleRate_(sampleRate)
    , cancelFlag_(cancelFlag)
{
}

bool MySpectrumReceiver::Receive(IAudioSpectrum* spectrum)
{
    if (cancelFlag_ && cancelFlag_->IsSet())
        return false;

    int* ptr = spectrum->GetFrequencies();
    const int amount = spectrum->GetFrequenciesCount();
    if (amount <= 1)
        return false;

    if (!power_.get()) {
        power_.reset(new double[amount]);
        for (int i = 0; i < amount; ++i)
            power_[i] = 0.0;
    }

    for (int i = 0; i < amount; ++i) {
        const double d = 10 * log(static_cast<double>(ptr[i]));
//         if (d > 0.0)
//             power_[i] += d;
        power_[i] = max(power_[i], d);
    }

    const int checkPointInterval = 20;
    const bool checkPoint = !!(receiveCount_++ % checkPointInterval);
    if (!checkPoint) {
//         for (int i = amount - 1; i >= 0; --i)
//             power_[i] /= 20;

        double prev = power_[amount - 1];
        int cutOffFreqIndex = 0;
        for (int i = amount - 2; i >= 0; --i) {
            double diff = abs(prev - power_[i]);
            if (diff > 3.0) {
                cutOffFreqIndex = i;
                break;
            }

            prev = power_[i];
        }

        int freq = (cutOffFreqIndex + 1) * (sampleRate_ / 2) / (amount - 1);
        freqs_.push_back(freq);

        for (int i = 0; i < amount; ++i)
            power_[i] = 0.0;
    }

    return true;
}
}

AudioQualityIdent::AudioQualityIdent(
    const shared_ptr<CancellationFlag>& cancelFlag)
    : funcHost_(NULL, reinterpret_cast<void (__stdcall*)(void*)>(FreeLibrary))
    , mediaInfo_()
    , spectrumSource_()
    , cancelFlag_(cancelFlag)
{
}

AudioQualityIdent::~AudioQualityIdent()
{
}

bool AudioQualityIdent::Init()
{
    return LoadMultiMediaCoreFunctions(&funcHost_, &mediaInfo_,
                                       &spectrumSource_);
}

bool AudioQualityIdent::Identify(const wstring& fullPathName, int* sampleRate,
                                 int* bitrate, int* channels, int* cutoff,
                                 int64* duration, wstring* format)
{
    assert(mediaInfo_);
    assert(spectrumSource_);
    assert(sampleRate);
    assert(bitrate);
    assert(channels);
    assert(cutoff);
    assert(duration);
    assert(format);
    if (!mediaInfo_ || !spectrumSource_ || !sampleRate || !bitrate ||
        !channels || !cutoff || !duration || !format)
        return false;

    const wchar_t* unknownFormat = L"[Unknown]";
    *sampleRate = 0;
    *bitrate = 0;
    *channels = 0;
    *cutoff = 0;
    *duration = 0;
    *format = unknownFormat;

    ifstream audioFile(fullPathName.c_str(), std::ios::binary);
    audioFile.seekg(0, std::ios::end);
    int fileSize = static_cast<int>(audioFile.tellg());
    if (fileSize > 0) {
        unique_ptr<int8[]> buf(new int8[fileSize]);
        audioFile.seekg(0);
        audioFile.read(reinterpret_cast<char*>(buf.get()), fileSize);

        // Retrieve all necessary media information.
        PassString ps(format);
        if (!mediaInfo_->GetInstantMediaInfo(buf.get(), fileSize, duration,
                                             bitrate, &ps, NULL, sampleRate,
                                             channels, NULL)) {
            // Return true and mark this file as an unrecognized format.
            return true;
        }

        if (format->empty())
            *format = unknownFormat;

        // Retrieve the average cutoff frequency.
        scoped_refptr<MySpectrumReceiver> receiver(
            new MySpectrumReceiver(*sampleRate, cancelFlag_));
        if (!spectrumSource_->Open(fullPathName.c_str())) {
            // Return true and mark this file as an unrecognized format.
            return true;
        }

        // Exclude the first 10 sec data.
        if (!spectrumSource_->Seek(10.0)) {
            // Return true so that we know it is a short-duration music.
            return true;
        }

        int channel = 0;
        IAudioSpectrumReceiver* r = receiver.get();
        spectrumSource_->ExtractSpectrum(&channel, 1, 1024, &r);
        if (cancelFlag_ && cancelFlag_->IsSet())
            return false;

        *cutoff = receiver->GetAverageFreq();
        return true;
    }

    return false;
}