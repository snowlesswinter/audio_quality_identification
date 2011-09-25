#ifndef _KUGOO_KGPLAYER_PLUGINS_COMMON_UNKNOWNIMPL_H_
#define _KUGOO_KGPLAYER_PLUGINS_COMMON_UNKNOWNIMPL_H_

#include <windows.h>

struct IUnknown;
namespace kugou
{

class __declspec(novtable) CNonDelegatingUnknown
{
public:
    virtual HRESULT __stdcall NonDelegatingQueryInterface(const IID&,
                                                          void**) = 0;
    virtual ULONG __stdcall NonDelegatingAddRef() = 0;
    virtual ULONG __stdcall NonDelegatingRelease() = 0;
};

// CUnknown的简化版。
class __declspec(novtable) CUnknown : public CNonDelegatingUnknown
{
public:
    static bool ImplementsThreadSafeReferenceCounting() { return true; }

    CUnknown(IUnknown* outer, HRESULT* r)
        : m_refCount(0)
        , m_outer(outer)
    {
        if (!m_outer)
            m_outer = reinterpret_cast<IUnknown*>(
                static_cast<CNonDelegatingUnknown*>(this));
    }
    virtual ~CUnknown() {};

    // CNonDelegatingUnknown
    virtual HRESULT __stdcall NonDelegatingQueryInterface(const IID& intID, 
                                                          void** o)
    {
        if (!o)
             return E_POINTER;

        if (IID_IUnknown == intID)
        {
            CNonDelegatingUnknown* u = this;
            u->NonDelegatingAddRef();
            *o = u;
            return S_OK;
        }

        *o = NULL;
        return E_NOINTERFACE;
    }

    virtual ULONG __stdcall NonDelegatingAddRef()
    {
        return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
    }

    virtual ULONG __stdcall NonDelegatingRelease()
    {
        long ref = InterlockedDecrement(&m_refCount);
        if (!ref)
        {
            ++m_refCount;
            delete this;
            return 0;
        }

        return static_cast<ULONG>(ref);
    }

    IUnknown* GetOwner() { return m_outer; };

protected: // So we can override NonDelegatingRelease()
    volatile long m_refCount;

private:
    IUnknown* m_outer;
};

// 将IUnknown接口的方法委托给CNonDelegatingUnknown，DirectShow里面有一个同样的
// 宏，但是并非所有使用IUnknown的单元都依赖于DirectShow，所以这里单独写一个属于
// 本项目的。
#define DELEGATE_IUNKNOWN                                                   \
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) {     \
        return kugou::CUnknown::GetOwner()->QueryInterface(riid, ppv);                       \
    }                                                                       \
    virtual ULONG __stdcall AddRef() {                                      \
        return kugou::CUnknown::GetOwner()->AddRef();                                        \
    }                                                                       \
    virtual ULONG __stdcall Release() {                                     \
        return kugou::CUnknown::GetOwner()->Release();                                       \
    }

}   // namespace kugou

#endif  // _KUGOO_KGPLAYER_PLUGINS_COMMON_UNKNOWNIMPL_H_