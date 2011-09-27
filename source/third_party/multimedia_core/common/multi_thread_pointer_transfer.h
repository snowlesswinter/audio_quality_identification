#ifndef _MULTI_THREAD_POINTER_TRANSFER_H_
#define _MULTI_THREAD_POINTER_TRANSFER_H_

#include "third_party/chromium/base/atomicops.h"

class CSeqLock
{
public:
    typedef base::subtle::Atomic32 CounterType;

    CSeqLock() : m_lockCount(0) {}
    ~CSeqLock() {}

    void AcquireWrite()
    {
        for (;;)
        {
            CounterType old =
                base::subtle::NoBarrier_Load(&m_lockCount);
            if (old)
                continue; // If some one has taken the token, spin.

            if (old == base::subtle::Acquire_CompareAndSwap(
                &m_lockCount, old, beingWritten))
                return;
        }
    }

    void ReleaseWrite()
    {
        base::subtle::Release_Store(&m_lockCount, 0);
    }

    void AcquireRead()
    {
        for (;;)
        {
            CounterType old =
                base::subtle::NoBarrier_Load(&m_lockCount);
            if (beingWritten == old)
                continue; // If some one has taken the token, spin.

            if (old == base::subtle::NoBarrier_CompareAndSwap(
                &m_lockCount, old, old + 1))
                return;
        }
    }

    void ReleaseRead()
    {
        base::subtle::Barrier_AtomicIncrement(&m_lockCount, -1);
    }

private:
    static const CounterType beingWritten = -1;

    CounterType m_lockCount;
};

class CAutoReadSeqLock
{
public:
    explicit CAutoReadSeqLock(CSeqLock* seqLock) : m_lock(seqLock)
    {
        m_lock->AcquireRead();
    }

    ~CAutoReadSeqLock() { m_lock->ReleaseRead(); }

private:
    CSeqLock* m_lock;
};

class CAutoWriteSeqLock
{
public:
    explicit CAutoWriteSeqLock(CSeqLock* seqLock) : m_lock(seqLock)
    {
        m_lock->AcquireWrite();
    }

    ~CAutoWriteSeqLock() { m_lock->ReleaseWrite(); }

private:
    CSeqLock* m_lock;
};

////////////////////////////////////////////////////////////////////////////////
// CSpinLockPointerTransfer
//
// Fit-in situation:
//      Rare write
//      Frequent read
//
// Write operations are *NOT* guaranteed to be FIFO!
// 
template <typename PointerType>
class CSpinLockPointerTransfer
{
public:
    CSpinLockPointerTransfer() : m_lock() {}
    ~CSpinLockPointerTransfer() {}

    void Set(const PointerType& p)
    {
        PointerType t;
        {
            CAutoWriteSeqLock lock(&m_lock);

            // Make sure that any kind of expensive destructor wouldn't be
            // called within the scope of a spin lock.
            t = m_pointer;
            m_pointer = p;
        }
    }

    PointerType Get()
    {
        CAutoReadSeqLock lock(&m_lock);
        return m_pointer;
    }

private:
    CSeqLock m_lock;
    PointerType m_pointer;
};

#endif  // _MULTI_THREAD_POINTER_TRANSFER_H_