#ifndef AF_CONCURRENCY_SPINLOCK_H
#define AF_CONCURRENCY_SPINLOCK_H

#include "AF/align.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/threading/atomic.h"

#include "AF/detail/utils/utils.h"


namespace AF
{
namespace Detail
{

class AF_PREALIGN(AF_CACHELINE_ALIGNMENT) SpinLock {
public:
    AF_FORCEINLINE SpinLock() {
        value_.Store(0);
    }

    AF_FORCEINLINE ~SpinLock() {
    }

    AF_FORCEINLINE void Lock() {
        uint32_t backoff(0);
        while (true) {
            uint32_t current_value(UNLOCKED);
            if (value_.CompareExchangeAcquire(current_value, LOCKED)) {
                return;
            }

            Detail::Utils::Backoff(backoff);
        }
    }

    AF_FORCEINLINE void Unlock() {
        AF_ASSERT(value_.Load() == LOCKED);
        value_.Store(UNLOCKED);
    }

private:
    SpinLock(const SpinLock &other);
    SpinLock &operator=(const SpinLock &other);

    static const uint32_t UNLOCKED = 0;
    static const uint32_t LOCKED = 1;

    Atomic::UInt32 value_;

} AF_POSTALIGN(AF_CACHELINE_ALIGNMENT);


} // namespace Detail
} // namespace AF


#endif // AF_CONCURRENCY_SPINLOCK_H
