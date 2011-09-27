#include "persistent_map.h"

#include <memory>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <windows.h>

using std::unique_ptr;
using std::basic_string;
using std::wstring;
using std::ifstream;
using std::ofstream;
using std::shared_ptr;
using boost::filesystem3::path;
using boost::archive::xml_iarchive;
using boost::archive::xml_oarchive;
using boost::archive::archive_exception;

namespace boost
{
namespace serialization
{
template <typename Archive>
void serialize(Archive& ar, PersistentMap::MediaInfo& info,
               const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(info.SampleRate);
    ar & BOOST_SERIALIZATION_NVP(info.Bitrate);
    ar & BOOST_SERIALIZATION_NVP(info.Channels);
    ar & BOOST_SERIALIZATION_NVP(info.CutoffFreq);
    ar & BOOST_SERIALIZATION_NVP(info.Duration);
    ar & BOOST_SERIALIZATION_NVP(info.Format);
}
}
}

namespace for_test
{
template <typename T>
class MyAllocator
{
public:
    template<class Other> struct rebind
    {
        typedef MyAllocator<Other> other;
    };

    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    MyAllocator()
    {
    }

    template<class Other>
    MyAllocator(const MyAllocator<Other>&)
    {
    }

    pointer allocate(size_type count)
    {
        return (pointer)::operator new(count * sizeof(value_type));
    }

    void deallocate(pointer p, size_type)
    {
        ::operator delete(p);
    }

    void construct(pointer p, const value_type& v)
    {
        void* t = p;
        ::new (t) value_type(v);
    }

    void destroy(pointer p)
    {
    }
};

typedef basic_string<wchar_t, std::char_traits<wchar_t>, MyAllocator<wchar_t>>
    MyWString;

void fun()
{
    MyWString a;
}
}

//------------------------------------------------------------------------------
namespace
{
inline wstring GetArchiveFileName(const std::wstring& dir,
                                  const wchar_t* fileName)
{
    return path(dir + L"/").remove_filename().wstring() + L"/" + fileName;
}

const wchar_t* defaultArchiveFileName = L"/persistence.xml";
}

shared_ptr<PersistentMap> PersistentMap::CreateInstance(const wstring& dir)
{
    shared_ptr<PersistentMap> rv(new PersistentMap(dir));
    PersistentMapGlobal::GetInstance()->SetCurrentInstance(rv);
    return rv;
}

PersistentMap::~PersistentMap()
{
    SerializeNow(defaultArchiveFileName);
}

PersistentMap::PersistentMap(const std::wstring& dir)
    : dir_(dir)
    , map_()
{
    // Setting locale is important for saving non-ascii characters.
    setlocale(LC_ALL, "chs");

    ifstream inFile(GetArchiveFileName(dir_, defaultArchiveFileName).c_str());
    if (inFile.good()) {
        try {
            xml_iarchive inArch(inFile);
            inArch >> BOOST_SERIALIZATION_NVP(map_);
        } catch (const archive_exception&) {
            MessageBoxA(NULL,
                        "Failed to load analyzing result.", "ERROR", MB_OK);
        }
    }
}

void PersistentMap::SerializeNow(const wchar_t* fileName)
{
    ofstream outFile(GetArchiveFileName(dir_, fileName).c_str());
    if (outFile.good()) {
        try {
            xml_oarchive outArch(outFile);
            outArch << BOOST_SERIALIZATION_NVP(map_);
        } catch (const archive_exception&) {
            MessageBoxA(NULL,
                        "Failed to save analyzing result.", "ERROR", MB_OK);
        }
    }
}

//------------------------------------------------------------------------------
PersistentMapGlobal* PersistentMapGlobal::GetInstance()
{
    return Singleton<PersistentMapGlobal>::get();
}

void PersistentMapGlobal::EmergencySerialize()
{
    shared_ptr<PersistentMap> m = globalRef_.Get().lock();
    if (m) {
        // Data in the map maybe corrupted, save to another file.
        m->SerializeNow(L"emergency_archive.xml");
    }
}

PersistentMapGlobal::PersistentMapGlobal()
    : globalRef_()
{
}

void PersistentMapGlobal::SetCurrentInstance(
    const shared_ptr<PersistentMap>& inst)
{
    globalRef_.Set(inst);
}