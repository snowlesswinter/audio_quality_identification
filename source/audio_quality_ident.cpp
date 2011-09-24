#include "audio_quality_ident.h"

#include <fstream>

#include <boost/filesystem.hpp>
#include <windows.h>

#include "third_party/multimedia_core/player_interface.h"
#include "third_party/multimedia_core/audio_information_extracter_interface.h"
#include "third_party/multimedia_core/audio_spectrum_extracter_interface.h"

using std::unique_ptr;
using std::wstring;
using std::ifstream;
using std::wofstream;
using boost::filesystem3::path;

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

wofstream* resu = NULL;
}

AudioQualityIdent::AudioQualityIdent(const std::wstring& resultDir)
    : funcHost_(NULL, reinterpret_cast<void (__stdcall*)(void*)>(FreeLibrary))
    , mediaInfo_()
    , spectrumSource_()
{
    resu = new wofstream((resultDir + L"result.txt").c_str(), std::ios::out);
    resu->imbue(std::locale("chs"));
}

AudioQualityIdent::~AudioQualityIdent()
{
    delete resu;
}

bool AudioQualityIdent::Init()
{
    return LoadMultiMediaCoreFunctions(&funcHost_, &mediaInfo_,
                                       &spectrumSource_);
}

void AudioQualityIdent::Identify(const wstring& fileName)
{
    assert(mediaInfo_);
    if (!mediaInfo_)
        return;

    ifstream audioFile(fileName.c_str(), std::ios::binary);
    audioFile.seekg(0, std::ios::end);
    int fileSize = static_cast<int>(audioFile.tellg());
    if (fileSize > 0) {
        unique_ptr<int8[]> buf(new int8[fileSize]);
        audioFile.seekg(0);
        audioFile.read(reinterpret_cast<char*>(buf.get()), fileSize);
        int bitrate;
        if (!mediaInfo_->GetInstantMediaInfo(buf.get(), fileSize, NULL,
                                             &bitrate, NULL, NULL, NULL, NULL,
                                             NULL))
            return;

        *resu << fileName << bitrate << std::endl;
    }
}