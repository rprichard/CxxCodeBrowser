#include "Mutex.h"

#include <cassert>

namespace indexer {

Mutex::Mutex()
{
#if INDEXER_MUTEX_USE_PTHREADS
    int ret = pthread_mutex_init(&mutex, NULL);
    assert(ret == 0 && "pthread_mutex_init failed");
#elif INDEXER_MUTEX_USE_WIN32
    InitializeCriticalSection(&mutex);
#endif
}

Mutex::~Mutex()
{
#if INDEXER_MUTEX_USE_PTHREADS
    int ret = pthread_mutex_destroy(&mutex);
    assert(ret == 0 && "pthread_mutex_destroy failed");
#elif INDEXER_MUTEX_USE_WIN32
    DeleteCriticalSection(&mutex);
#endif
}

void Mutex::lock()
{
#if INDEXER_MUTEX_USE_PTHREADS
    int ret = pthread_mutex_lock(&mutex);
    assert(ret == 0 && "pthread_mutex_lock failed");
#elif INDEXER_MUTEX_USE_WIN32
    EnterCriticalSection(&mutex);
#elif INDEXER_MUTEX_USE_CXX11
    mutex.lock();
#else
#error "Not implemented"
#endif
}

void Mutex::unlock()
{
#if INDEXER_MUTEX_USE_PTHREADS
    int ret = pthread_mutex_unlock(&mutex);
    assert(ret == 0 && "pthread_mutex_lock failed");
#elif INDEXER_MUTEX_USE_WIN32
    LeaveCriticalSection(&mutex);
#elif INDEXER_MUTEX_USE_CXX11
    mutex.unlock();
#else
#error "Not implemented"
#endif
}

} // namespace indexer
