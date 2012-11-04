#ifndef INDEXER_MUTEX_H
#define INDEXER_MUTEX_H

#ifdef __unix__
#define INDEXER_MUTEX_USE_PTHREADS 1
#endif

// Use pthreads on Unix instead of C++11's chrono header.  There is a bug in
// the chrono header in libstdc++ 4.6 and 4.7 that prevents Clang 3.1 from
// parsing it.  Newer versions of Clang might work around the bug.
//
// http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=666539
// http://llvm.org/bugs/show_bug.cgi?id=12893
// Clang SVN commit 166455
//
#if INDEXER_MUTEX_USE_PTHREADS
#include <pthread.h>
#else
#include <chrono>
#endif

namespace indexer {

class Mutex {
public:
    Mutex();
    ~Mutex();
    void lock();
    void unlock();

private:
#ifdef INDEXER_MUTEX_USE_PTHREADS
    pthread_mutex_t mutex;
#else
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
