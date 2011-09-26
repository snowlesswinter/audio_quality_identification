#ifndef _PERSISTENT_MAP_H_
#define _PERSISTENT_MAP_H_

#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>

#include "third_party/chromium/base/basictypes.h"

class PersistentMap
{
public:
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

    explicit PersistentMap(const std::wstring& dir);
    ~PersistentMap();

    ContainerType& GetMap() { return map_; }

private:
    DISALLOW_COPY_AND_ASSIGN(PersistentMap);

    std::wstring dir_;
    ContainerType map_;
};

#endif  // _PERSISTENT_MAP_H_