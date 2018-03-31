#ifndef AF_DETAIL_CONDITION_H
#define AF_DETAIL_CONDITION_H


#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/threading/lock.h"
#include "AF/detail/threading/mutex.h"

#include <thread>
#include <condition_variable>

namespace AF
{
namespace Detail
{

/*
 * Portable condition variable synchronization primitive, sometimes also called a monitor.
 */
class Condition {
public:
    inline Condition() {
    }

    inline ~Condition() {
    }

    AF_FORCEINLINE Mutex &GetMutex() {
        return mutex_;
    }

    AF_FORCEINLINE void Wait(Lock &lock) {
        // The lock must be locked before calling Wait().
        AF_ASSERT(lock.lock_.owns_lock());
        condition_.wait(lock.lock_);
    }

    // Wakes a single thread that is suspended after having called Wait.
    AF_FORCEINLINE void Pulse() {
        condition_.notify_one();
    }

    AF_FORCEINLINE void PulseAll() {
        condition_.notify_all();
    }

private:
    Condition(const Condition &other);
    Condition &operator=(const Condition &other);

    Mutex mutex_;

    std::condition_variable condition_;
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_CONDITION_H
