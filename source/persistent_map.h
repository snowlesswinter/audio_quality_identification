#ifndef _PERSISTENT_MAP_H_
#define _PERSISTENT_MAP_H_

#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>

#include "third_party/chromium/base/basictypes.h"

class PersistentMap
{
public:
    typedef std::pair<int, int> ElementType;
    typedef std::map<std::wstring, ElementType> ContainerType;

    PersistentMap();
    ~PersistentMap();

    ContainerType& GetMap() { return map_; }

private:
    DISALLOW_COPY_AND_ASSIGN(PersistentMap);

    ContainerType map_;
};

#endif  // _PERSISTENT_MAP_H_