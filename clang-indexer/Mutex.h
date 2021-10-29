#ifndef INDEXER_MUTEX_H
#define INDEXER_MUTEX_H

#include "../shared_headers/host.h"

#if defined(CXXCODEBROWSER_UNIX)
#define INDEXER_MUTEX_USE_PTHREADS 1
#elif defined(_WIN32)
// MinGW32 also does not provide C++11 threading.  (MinGW-w64 might, though.)
#define INDEXER_MUTEX_USE_WIN32 1
#else
#define INDEXER_MUTEX_USE_CXX11 1
#endif

// Use pthreads on Unix instead of C++11's mutex header.  There is a bug in the
// chrono header in libstdc++ 4.6 and 4.7 that prevents Clang 3.1 from parsing
// the mutex header.  Newer versions of Clang might work around the bug.
//
// http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=666539
// http://llvm.org/bugs/show_bug.cgi?id=12893
// Clang SVN commit 166455
//
#if INDEXER_MUTEX_USE_PTHREADS
#include <pthread.h>
#endif

#if INDEXER_MUTEX_USE_WIN32
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#endif

#if INDEXER_MUTEX_USE_CXX11
#include <mutex>
#endif

namespace indexer {

class Mutex {
public:
    Mutex();
    ~Mutex();
    void lock();
    void unlock();

private:
#if INDEXER_MUTEX_USE_PTHREADS
    pthread_mutex_t mutex;
#elif INDEXER_MUTEX_USE_WIN32
    CRITICAL_SECTION mutex;
#elif INDEXER_MUTEX_USE_CXX11
    std::mutex mutex;
#endif
};

// std::mutex<T> is defined in mutex, which cannot be included because it
// includes chrono.
template <typename T>
class LockGuard {
public:
    LockGuard(T &mutex) : m_mutex(mutex) { m_mutex.lock(); }
    ~LockGuard() { m_mutex.unlock(); }
    LockGuard(const LockGuard &other) = delete;
    LockGuard &operator=(const LockGuard &other) = delete;
private:
    T &m_mutex;
};

} // namespace indexer

#endif // INDEXER_MUTEX_H
