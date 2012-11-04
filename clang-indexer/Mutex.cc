#include "Mutex.h"

#include <cassert>

namespace indexer {

Mutex::Mutex()
{
#ifdef INDEXER_MUTEX_USE_PTHREADS
    int ret = pthread_mutex_init(&mutex, NULL);
    assert(ret == 0 && "pthread_mutex_init failed");
#endif
}

Mutex::~Mutex()
{
#ifdef INDEXER_MUTEX_USE_PTHREADS
    int ret = pthread_mutex_destroy(&mutex);
    assert(ret == 0 && "pthread_mutex_destroy failed");
#endif
}

void Mutex::lock()
{
#ifdef INDEXER_MUTEX_USE_PTHREADS
    int ret = pthread_mutex_lock(&mutex);
    assert(ret == 0 && "pthread_mutex_lock failed");
#else
    mutex.lock();
#endif
}

void Mutex::unlock()
{
#ifdef INDEXER_MUTEX_USE_PTHREADS
    int ret = pthread_mutex_unlock(&mutex);
    assert(ret == 0 && "pthread_mutex_lock failed");
#else
    mutex.unlock();
#endif
}

} // namespace indexer
