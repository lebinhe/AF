#ifndef AF_DETAIL_LOCK_H
#define AF_DETAIL_LOCK_H


#include "AF/assert.h"
#include "AF/defines.h"

#include "AF/detail/threading/mutex.h"

#include <thread>

namespace AF
{
namespace Detail
{

/*
 * Portable lock synchronization primitive.
 * This class is a helper which allows a mutex to be locked and automatically locked within a scope.
 */
class Lock {
public:
    friend class Condition;

    AF_FORCEINLINE explicit Lock(Mutex &mutex) 
      : lock_(mutex.mutex_) {
    }

    AF_FORCEINLINE ~Lock() {
    }

    // Explicitly and temporarily unlocks the locked mutex.
    // The caller should always call Relock after this call and before destruction.
    AF_FORCEINLINE void Unlock() {
        AF_ASSERT(lock_.owns_lock() == true);
        lock_.unlock();
    }

    // Re-locks the associated mutex, which must have been previously unlocked with Unlock.
    // Relock can only be called after a preceding call to Unlock.
    AF_FORCEINLINE void Relock() {
        AF_ASSERT(lock_.owns_lock() == false);
        lock_.lock();
    }

private:
    Lock(const Lock &other);
    Lock &operator=(const Lock &other);

    std::unique_lock<std::mutex> lock_;
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_LOCK_H
