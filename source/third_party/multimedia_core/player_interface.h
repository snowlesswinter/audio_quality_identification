#ifndef _IPLAYER_H_
#define _IPLAYER_H_

#include <unknwn.h>

#include "third_party/chromium/base/basictypes.h"

struct TAudioInfo;
interface IDecoderEx : public IUnknown
{
    virtual bool __stdcall Open(const wchar_t* fileName) = 0;
    virtual void __stdcall Seek(int64 seconds) = 0;
    virtual int64 __stdcall GetPosition() = 0;
    virtual int __stdcall GetPCMData(char* data, int len) = 0;
    virtual void __stdcall GetAudioInfo(TAudioInfo* audioInfo) = 0;
};

//------------------------------------------------------------------------------
interface IPassString : public IUnknown
{
    virtual const wchar_t* __stdcall GetContent() = 0;
    virtual void __stdcall SetContent(const wchar_t* content) = 0;
};

//------------------------------------------------------------------------------
interface IExtraMediaInfo : public IUnknown
{
    virtual int64 __stdcall GetStartTime() = 0; 
    virtual int64 __stdcall GetStopTime() = 0;  
    // GetAudioTrackMappingCode返回0是正常，返回1表示原唱伴唱翻转
    virtual int32 __stdcall GetAudioTrackMappingCode() = 0;
};

//------------------------------------------------------------------------------
enum KProxyType;
struct ILegacyP2PStream;
struct TPluginInfo;
enum OutputType;
enum KAudioTrackType;
class CGetPCMEvent;
struct TDeviceInfo;
interface IAsyncCallback;
enum KSingEvent;
interface ICorePlayer : public IUnknown
{
    virtual void __stdcall OpenStream(ILegacyP2PStream* stream, 
                                      IExtraMediaInfo* extraInfo) = 0;
    virtual void __stdcall OpenFile(const wchar_t* fileName,
                                    IExtraMediaInfo* extraInfo) = 0;
    virtual void __stdcall Play() = 0;
    virtual void __stdcall Pause() = 0;
    virtual void __stdcall Stop() = 0;
    virtual void __stdcall SetPosition(int64 time) = 0;
    virtual int64 __stdcall GetPosition() = 0;
    virtual void __stdcall SetVolume(int volume) = 0;
    virtual int __stdcall GetVolume() = 0;
    virtual void __stdcall SetMute(bool mute) = 0;
    virtual bool __stdcall GetMute() = 0;
    virtual void __stdcall SetBalance(int balance) = 0;
    virtual int __stdcall GetBalance() = 0;
    virtual void __stdcall GetAudioInfo(TAudioInfo* audioInfo) = 0;
    virtual HRESULT __stdcall StartRecording(const wchar_t* fileName,
                                             int64 duration,
                                             IAsyncCallback* callback,
                                             int type, IUnknown* inst) = 0;
    virtual HRESULT __stdcall StopRecording(IUnknown* inst) = 0;
    virtual void __stdcall SelectAudioTrack(KAudioTrackType trackType) = 0;
    virtual bool __stdcall GetInstantMediaInfo(const void* buf, int64 length, 
                                               int64* duration, int* bitrate, 
                                               IPassString* format, 
                                               int* trackCount, int* sampleRate,
                                               int* channel, int* bitDepth) = 0;
};

//------------------------------------------------------------------------------
interface IExternalDSPSet : public IUnknown
{
    virtual bool __stdcall Flush() = 0;
    virtual bool __stdcall Process(void* inData, int dataSize, void* outData,
                                   int* outSize, int sampleRate, int channels,
                                   int bits, int outFormat) = 0;//0为整形1为浮点
};

interface IDSPConfig : public IUnknown
{
    virtual void __stdcall SetPreamp(float preamp) = 0;
    virtual float __stdcall GetPreamp() = 0;
    virtual void __stdcall SetSound3D(int sound3d) = 0;
    virtual int __stdcall GetSound3D() = 0;
    virtual void __stdcall SetReplayGain(float replayGain) = 0;
    virtual void __stdcall SetWinampDSPPluginsInfo(const wchar_t* path,
                                                   HWND mainWindow) = 0;
    virtual void __stdcall GetPluginInfos(TPluginInfo* pluginInfos, 
                                          int* pluginCount) = 0;
    virtual int __stdcall GetWinampPluginPath(wchar_t* path, int buffLen) = 0;
    virtual void __stdcall EnableWinampDSPPlugin(int index) = 0;
    virtual void __stdcall ShowWinampDSPPluginConfigPanel(int index) = 0;
    virtual void __stdcall DisableWinampDSPPlugin(int index) = 0;
    virtual void __stdcall SetEqEnabled(bool enabled) = 0;
    virtual void __stdcall SetEqGain(int bandIndex, float bandGain) = 0;
    virtual void __stdcall SetSpectrumEnabled(bool enable) = 0;
    virtual void __stdcall SetWaveFormEnabled(bool enable) = 0;
    virtual void __stdcall SetDsp(IExternalDSPSet* dsp) = 0;
};

//------------------------------------------------------------------------------
interface IPlayerConfig : public IUnknown
{
    // 当IDs为NULL时返回OutputType的总数目 TUDO:这个已经改成了获取AudioRenderer
    virtual int __stdcall GetOutputTypeIDList(int* IDs, int count) = 0;
    // name最大长度为_MAX_PATH(260)
    virtual int __stdcall GetOutputTypeName(int ID, wchar_t* name,
                                            int count) = 0;

    // 当info为NULL时返回Devices的总数目,如果outputTypeID有误则返回-1
    // TUDO:已经废弃
    virtual int __stdcall GetOutputDevices(int outputTypeID, TDeviceInfo* info,
                                           int count) = 0;
    virtual void __stdcall GetCurrentOutput(int* typeID, int* deviceIndex) = 0;
    virtual bool __stdcall SetOutputDevice(int outputTypeID, 
                                           int deviceIndex) = 0;
    virtual void __stdcall SetOutputBits(int bits) = 0;
    virtual void __stdcall SetOutputFloat(bool isFloat) = 0;
    virtual void __stdcall SetOutputFadeEnabled(bool fadeEnabled) = 0;
    virtual void __stdcall SetFadingDuration(int fadeIn, int fadeOut) = 0;
    virtual void __stdcall SetProxy(const wchar_t* url, 
                                    KProxyType type,
                                    bool isuser) = 0;
    virtual void __stdcall SetDefaultVedioRenderer(CLSID& rendererId,
                                                   int mode) = 0;
};

// 用于提高设备名称传递的效率，避免繁复的查询和不当的内存分配问题。
interface IPassDevices : public IUnknown
{
    virtual void __stdcall AddDevice(const wchar_t* deviceName) = 0;
};

// ISingingCallback接口是K歌功能的事件回调接口。
interface ISingingCallback : public IUnknown
{
    virtual void __stdcall OnDeviceDetectionDone(bool succeeded) = 0;
    virtual void __stdcall OnOpeningDeviceFailure(int errorCode) = 0;
    virtual void __stdcall OnRealTimeVolumeUpdate(int currentVolume) = 0;
    virtual void __stdcall OnRecordingFailure(int errorCode) = 0;
    virtual void __stdcall OnEventHappened(KSingEvent event) = 0;
};

interface ISingingControl : public IUnknown
{
    virtual void __stdcall EnableSingingMode(ISingingCallback* callback) = 0;
    virtual void __stdcall DisableSingingMode() = 0;
    virtual void __stdcall SetInputDevice(const wchar_t* deviceName) = 0;
    virtual void __stdcall GetInputDevices(IPassDevices* deviceDescs) = 0;
    virtual void __stdcall SetOutputDevice(const wchar_t* deviceName) = 0;
    virtual void __stdcall GetOutputDevices(IPassDevices* deviceDescs) = 0;
    virtual void __stdcall GetCurrentInput(IPassString* des) = 0;
    virtual void __stdcall GetCurrentOutput(IPassString* des) = 0;
    virtual void __stdcall EnableRealTimeVolumeUpdate(bool enable) = 0;
    virtual void __stdcall StartRecording(const wchar_t* filePath) = 0;
    virtual void __stdcall StopRecording() = 0;
    // 0 = 打开麦克风侦听来回放， 1 = 通过本进程回放， -1 = 不回放
    virtual void __stdcall SetMicrophonePlaybackMode(int mode) = 0;
    virtual int __stdcall GetMicrophonePlaybackMode() = 0;
    virtual void __stdcall SetMicrophoneVolume(int volume) = 0; // 0 - 100
    virtual int __stdcall GetMicrophoneVolume() = 0;
    virtual void __stdcall EnableNoiseSuppression(bool enable) = 0;
    virtual bool __stdcall IsNoiseSuppressionEnabled() = 0;
    virtual void __stdcall EnableAcousticEchoCancellation(bool enable) = 0;
    virtual bool __stdcall IsAcousticEchoCancellationEnabled() = 0;
};
#endif
