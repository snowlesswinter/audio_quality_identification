#ifndef _DIR_TRAVERSING_H_
#define _DIR_TRAVERSING_H_

#include <string>

#include "third_party/chromium/base/memory/ref_counted.h"
#include "third_party/chromium/base/threading/thread.h"
#include "third_party/chromium/base/synchronization/lock.h"

//------------------------------------------------------------------------------
class DirTraversing : public base::RefCountedThreadSafe<DirTraversing>
{
public:
    class Callback
    {
    public:
        virtual void Initializing(int totalFiles) = 0;
        virtual bool Progress(const std::wstring& current) = 0;

        // Guaranteed to be called no matter success or failure. After "Done" is
        // called, The TraversingCallback object will no longer be accessed by
        // DirTraversing. So it's convenient to delete the object in "Done".
        virtual void Done() = 0;
    };

    DirTraversing(Callback* callback, const wchar_t* initialDir);

    void Traverse();

private:
    friend class base::RefCountedThreadSafe<DirTraversing>;
    ~DirTraversing();

    DISALLOW_COPY_AND_ASSIGN(DirTraversing);

    std::wstring initialDir_;
    Callback* callback_;
};

//------------------------------------------------------------------------------
class MessageLoop;
class DirTraversingProxy
{
public:
    DirTraversingProxy();
    ~DirTraversingProxy();

    void Traverse(DirTraversing::Callback* callback, const wchar_t* initialDir);

private:
    DISALLOW_COPY_AND_ASSIGN(DirTraversingProxy);

    base::Lock threadStatusLock_;
    base::Thread thread_;
};

#endif  // _DIR_TRAVERSING_H_