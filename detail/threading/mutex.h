#ifndef AF_DETAIL_MUTEX_H
#define AF_DETAIL_MUTEX_H

#include "AF/defines.h"

#include <thread>
#include <mutex>

namespace AF
{
namespace Detail
{

class Mutex {
public:
    friend class Condition;
    friend class Lock;

    AF_FORCEINLINE Mutex() {
    }

    AF_FORCEINLINE ~Mutex() {
    }

    AF_FORCEINLINE void Lock() {
        mutex_.lock();
    }

    AF_FORCEINLINE void Unlock() {
        mutex_.unlock();
    }

private:
    Mutex(const Mutex &other);
    Mutex &operator=(const Mutex &other);

    std::mutex mutex_;
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_MUTEX_H
