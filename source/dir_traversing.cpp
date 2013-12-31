#include "dir_traversing.h"

#include <cassert>
#include <memory>
#include <list>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "third_party/chromium/base/message_loop.h"

using std::wstring;
using std::unique_ptr;
using std::list;
using boost::filesystem::path;
using boost::filesystem::exists;
using boost::filesystem::is_regular_file;
using boost::filesystem::directory_iterator;
using boost::filesystem::is_directory;
using boost::filesystem::filesystem_error;
using boost::algorithm::iequals;

namespace {
void ReportDone(DirTraversing::Callback* c)
{
    if (c)
        c->Done();
}

int CalculateNumFiles(const path& initialPath)
{
    if (!exists(initialPath))
        return 0;

    // Is the specified path a regular file or a directory?
    if (is_regular_file(initialPath))
        return 1;

    int fileCount = 0;
    try {
        for (directory_iterator i(initialPath), e = directory_iterator();
            i != e; ++i) {
            if (is_directory(i->path())) {
                fileCount += CalculateNumFiles(i->path());
                continue;
            }

            fileCount++;
        }
    } catch (const filesystem_error&) {
    }

    return fileCount;
}

// Return false when Callback::Progress returns false, that is considered as a
// halt command.
bool Traverse(const path& cur, list<path>* pending, DirTraversing::Callback* c)
{
    try {
        for (directory_iterator i(cur), e = directory_iterator(); i != e; ++i) {
            if (is_directory(i->path())) {
                pending->push_back(i->path());
                continue;
            }

            if (!c->Progress(i->path().wstring()))
                return false;
        }
    } catch (const filesystem_error&) {
    }

    return true;
}
}


DirTraversing::DirTraversing(Callback* callback,
                             const wchar_t* initialDir)
    : initialDir_(initialDir)
    , callback_(callback)
{
    assert(callback_);
}

void DirTraversing::Traverse()
{
    assert(callback_);
    unique_ptr<Callback, void (*)(Callback*)> autoReportDone(callback_,
                                                             ReportDone);
    callback_->Initializing(CalculateNumFiles(initialDir_));

    if (!exists(initialDir_))
        return;

    if (is_regular_file(initialDir_)) {
        callback_->Progress(initialDir_);
        return;
    }

    list<path> pending;
    pending.push_back(initialDir_);
    do {
        const path& t = pending.front();
        if (!::Traverse(t, &pending, callback_))
            return;

        pending.pop_front();
    } while (pending.size());
}

DirTraversing::~DirTraversing()
{
}

//------------------------------------------------------------------------------
DirTraversingProxy::DirTraversingProxy()
    : threadStatusLock_()
    , thread_("Directory Traversing Proxy")
{
}

DirTraversingProxy::~DirTraversingProxy()
{
}

void DirTraversingProxy::Traverse(DirTraversing::Callback* callback,
                                  const wchar_t* initialDir)
{
//     MessageLoop* loop = MessageLoop::current();
//     if (loop)
    scoped_refptr<DirTraversing> t(new DirTraversing(callback, initialDir));
    {
        base::AutoLock lock(threadStatusLock_);
        if (!thread_.IsRunning())
            thread_.Start();

        thread_.message_loop()->PostTask(
            FROM_HERE,
            NewRunnableMethod(t.get(), &DirTraversing::Traverse));
    }
}