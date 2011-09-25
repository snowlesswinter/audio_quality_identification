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
using boost::filesystem3::path;
using boost::archive::xml_iarchive;
using boost::archive::xml_oarchive;
using boost::archive::archive_exception;

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
inline wstring GetArchiveFileName(const std::wstring& dir)
{
    return path(dir + L"/").remove_filename().wstring() + L"/persistence.txt";
}
}

PersistentMap::PersistentMap(const std::wstring& dir)
    : dir_(dir)
    , map_()
{
    // Setting locale is important for saving non-ascii characters.
    setlocale(LC_ALL, "chs");

    ifstream inFile(GetArchiveFileName(dir_).c_str());
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

PersistentMap::~PersistentMap()
{
    ofstream outFile(GetArchiveFileName(dir_).c_str());
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