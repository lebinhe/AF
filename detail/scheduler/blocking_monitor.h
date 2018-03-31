#ifndef AF_DETAIL_SCHEDULER_BLOCKINGMONITOR_H
#define AF_DETAIL_SCHEDULER_BLOCKINGMONITOR_H

#include "AF/align.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/threading/condition.h"
#include "AF/detail/threading/lock.h"
#include "AF/detail/threading/mutex.h"


namespace AF
{
namespace Detail
{

/* 
 * Blocking monitor thread synchronization primitive based on a condition variable.
 */
class BlockingMonitor
{
public:
    struct Context {
    };

    class LockType {
    public:
        friend class BlockingMonitor;

        AF_FORCEINLINE explicit LockType(BlockingMonitor &monitor) 
          : lock_(monitor.condition_.GetMutex()) {
        }

        AF_FORCEINLINE void Unlock() {
            lock_.Unlock();
        }

        AF_FORCEINLINE void Relock() {
            lock_.Relock();
        }

    private:
        LockType(const LockType &other);
        LockType &operator=(const LockType &other);

        Lock lock_;
    };

    friend class LockType;

    inline explicit BlockingMonitor();

    // Initializes the context structure of a worker thread.
    // The calling thread must be a worker thread.
    inline void InitializeWorkerContext(Context *const context);

    // Resets the yield backoff following a successful acquire.
    // The calling thread should not hold a lock.
    inline void ResetYield(Context *const context);

    // Wakes at most one waiting thread.
    // The calling thread should hold a lock while changing the protected state 
    // but should release it before calling Pulse.
    inline void Pulse();

    // Wakes all waiting threads.
    // The calling thread should hold a lock while changing the protected state 
    // but should release it before calling PulseAll.
    inline void PulseAll();

    // Puts the calling thread to sleep until it is woken by a pulse.
    // The calling thread should hold a lock and should pass the lock as a parameter.
    inline void Wait(Context *const context, LockType &lock);

private:
    BlockingMonitor(const BlockingMonitor &other);
    BlockingMonitor &operator=(const BlockingMonitor &other);

    mutable Condition condition_;
};


inline BlockingMonitor::BlockingMonitor() {
}

inline void BlockingMonitor::InitializeWorkerContext(Context *const context) {
}

AF_FORCEINLINE void BlockingMonitor::ResetYield(Context *const context) {
}

AF_FORCEINLINE void BlockingMonitor::Pulse() {
    condition_.Pulse();
}

AF_FORCEINLINE void BlockingMonitor::PulseAll() {
    condition_.PulseAll();
}

AF_FORCEINLINE void BlockingMonitor::Wait(Context *const context, LockType &lock) {
    condition_.Wait(lock.lock_);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_SCHEDULER_BLOCKINGMONITOR_H
