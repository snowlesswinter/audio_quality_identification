#include "audio_quality_ident.h"

#include <fstream>
#include <vector>

#include <boost/filesystem.hpp>
#include <windows.h>

#include "third_party/multimedia_core/player_interface.h"
#include "third_party/multimedia_core/audio_information_extracter_interface.h"
#include "third_party/multimedia_core/audio_spectrum_extracter_interface.h"
#include "third_party/multimedia_core/common/unknown_impl.h"
#include "persistent_map.h"

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

class MySpectrumReceiver : public kugou::CUnknown, public IAudioSpectrumReceiver
{
public:
    explicit MySpectrumReceiver(
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

        return static_cast<int>(sum / freqs.size());
    }

private:
    int receiveCount_;
    unique_ptr<double[]> power_;
    vector<int> freqs_;
    std::shared_ptr<base::CancellationFlag> cancelFlag_;
};

MySpectrumReceiver::MySpectrumReceiver(
    const shared_ptr<base::CancellationFlag>& cancelFlag)
    : kugou::CUnknown(NULL, NULL)
    , receiveCount_(1)
    , power_()
    , cancelFlag_(cancelFlag)
{
}

bool MySpectrumReceiver::Receive(IAudioSpectrum* spectrum)
{
    if (cancelFlag_ && cancelFlag_->IsSet())
        return false;

    int* ptr = spectrum->GetFrequencies();
    const int amount = spectrum->GetFrequenciesCount();

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

        int freq = (cutOffFreqIndex + 1) * 44100 / 1024;
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

bool AudioQualityIdent::Identify(const wstring& fullPathName, int* bitrate,
                                 int* cutoff)
{
    assert(mediaInfo_);
    assert(spectrumSource_);
    assert(bitrate);
    assert(cutoff);
    if (!mediaInfo_ || !spectrumSource_ || !bitrate || !cutoff)
        return false;

    *bitrate = 0;
    *cutoff = 0;

    ifstream audioFile(fullPathName.c_str(), std::ios::binary);
    audioFile.seekg(0, std::ios::end);
    int fileSize = static_cast<int>(audioFile.tellg());
    if (fileSize > 0) {
        unique_ptr<int8[]> buf(new int8[fileSize]);
        audioFile.seekg(0);
        audioFile.read(reinterpret_cast<char*>(buf.get()), fileSize);

        // Retrieve the average bitrate.
        if (!mediaInfo_->GetInstantMediaInfo(buf.get(), fileSize, NULL,
                                             bitrate, NULL, NULL, NULL, NULL,
                                             NULL))
            return false;

        // Retrieve the average cutoff frequency.
        scoped_refptr<MySpectrumReceiver> receiver(
            new MySpectrumReceiver(cancelFlag_));
        if (!spectrumSource_->Open(fullPathName.c_str()))
            return false;

        // Exclude the first 10 sec data.
        if (!spectrumSource_->Seek(10.0))
            return false;

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