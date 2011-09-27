#ifndef _PERSISTENT_MAP_H_
#define _PERSISTENT_MAP_H_

#include <memory>

#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>

#include "third_party/chromium/base/memory/singleton.h"
#include "third_party/multimedia_core/common/multi_thread_pointer_transfer.h"

class PersistentMapGlobal;
class PersistentMap
{
public:
    static std::shared_ptr<PersistentMap> CreateInstance(
        const std::wstring& dir);

    struct MediaInfo
    {
        MediaInfo()
            : SampleRate(0)
            , Bitrate(0)
            , Channels(0)
            , CutoffFreq(0)
            , Duration(0)
            , Format(L"[Unknown]")
        {
        }

        MediaInfo(int sampleRate, int bitrate, int channels, int cutoffFreq,
                  int64 duration, const std::wstring& format)
            : SampleRate(sampleRate)
            , Bitrate(bitrate)
            , Channels(channels)
            , CutoffFreq(cutoffFreq)
            , Duration(duration)
            , Format(format)
        {
        }

        int SampleRate;
        int Bitrate;
        int Channels;
        int CutoffFreq;
        int64 Duration;
        std::wstring Format;
    };

    typedef std::map<std::wstring, MediaInfo> ContainerType;

    ~PersistentMap();

    ContainerType& GetMap() { return map_; }

private:
    friend class PersistentMapGlobal;

    DISALLOW_COPY_AND_ASSIGN(PersistentMap);

    explicit PersistentMap(const std::wstring& dir);

    void SerializeNow(const wchar_t* fileName);

    std::wstring dir_;
    ContainerType map_;
};

//------------------------------------------------------------------------------
class PersistentMapGlobal
{
public:
    static PersistentMapGlobal* GetInstance();

    void EmergencySerialize();

private:
    friend struct DefaultSingletonTraits<PersistentMapGlobal>;
    friend class PersistentMap;

    DISALLOW_COPY_AND_ASSIGN(PersistentMapGlobal);

    PersistentMapGlobal();

    void SetCurrentInstance(const std::shared_ptr<PersistentMap>& inst);

    CSpinLockPointerTransfer<std::weak_ptr<PersistentMap>> globalRef_;
};

#endif  // _PERSISTENT_MAP_H_