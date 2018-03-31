#ifndef AF_DETAIL_SCHEDULER_COUNTING_H
#define AF_DETAIL_SCHEDULER_COUNTING_H


#include "AF/defines.h"

#include "AF/detail/threading/atomic.h"

#include "AF/detail/utils/utils.h"


#if AF_ENABLE_COUNTERS
#define AF_COUNTER_ARG(arg) arg
#else
#define AF_COUNTER_ARG(arg)
#endif


namespace AF
{
namespace Detail
{

/*
 * Enumerated type that lists event counters.
 * 
 * All counters are local to each Framework instance, and count events in
 * the queried Framework only.
 * 
 * The counters are only incremented if the value of the AF_ENABLE_COUNTERS
 * define is non-zero. If AF_ENABLE_COUNTERS is zero then the values of the counters
 * will always be zero.
 */
enum Counter
{
    COUNTER_MESSAGES_PROCESSED = 0,     // Number of messages processed by the framework.
    COUNTER_YIELDS,                     // Number of times a worker thread yielded to other threads.
    COUNTER_LOCAL_PUSHES,               // Number of times a mailbox was pushed to a thread's local queue.
    COUNTER_SHARED_PUSHES,              // Number of times a mailbox was pushed to the shared queue.
    COUNTER_MAILBOX_QUEUE_MAX,          // Maximum number of messages ever seen in the actor mailboxes.
    COUNTER_QUEUE_LATENCY_LOCAL_MIN,    // Minimum recorded local queue latency in microseconds.
    COUNTER_QUEUE_LATENCY_LOCAL_MAX,    // Maximum recorded local queue latency in microseconds.
    COUNTER_QUEUE_LATENCY_SHARED_MIN,   // Minimum recorded shared queue latency in microseconds.
    COUNTER_QUEUE_LATENCY_SHARED_MAX,   // Maximum recorded shared queue latency in microseconds.
    MAX_COUNTERS                        // Number of counters available for querying.
};


/*
 * Static helper that increments event counters.
 */
class Counting {
public:
    inline static uint32_t Get(const Atomic::UInt32 &counter);
    inline static void Set(Atomic::UInt32 &counter, const uint32_t n);

    inline static void Reset(Atomic::UInt32 &counter, const uint32_t id);
    inline static void Increment(Atomic::UInt32 &counter);
    inline static void Raise(Atomic::UInt32 &counter, const uint32_t n);
    inline static void Lower(Atomic::UInt32 &counter, const uint32_t n);
    inline static void Accumulate(const Atomic::UInt32 &counter, const uint32_t id, uint32_t &n);
};


AF_FORCEINLINE uint32_t Counting::Get(const Atomic::UInt32 & AF_COUNTER_ARG(counter)) {
#if AF_ENABLE_COUNTERS

    return counter.Load();

#else

    return 0;

#endif
}

AF_FORCEINLINE void Counting::Set(Atomic::UInt32 & AF_COUNTER_ARG(counter), const uint32_t AF_COUNTER_ARG(n)) {
#if AF_ENABLE_COUNTERS

    counter.Store(n);

#endif
}

AF_FORCEINLINE void Counting::Reset(Atomic::UInt32 & AF_COUNTER_ARG(counter), const uint32_t AF_COUNTER_ARG(id)) {
#if AF_ENABLE_COUNTERS

    switch (id) {
        case COUNTER_QUEUE_LATENCY_LOCAL_MIN:
        case COUNTER_QUEUE_LATENCY_SHARED_MIN: {
            counter.Store(0xFFFFFFFF);
            break;
        }
        default: {
            counter.Store(0);
            break;
        }
    }

#endif
}

AF_FORCEINLINE void Counting::Increment(Atomic::UInt32 & AF_COUNTER_ARG(counter)) {
#if AF_ENABLE_COUNTERS

    counter.Increment();

#endif
}

AF_FORCEINLINE void Counting::Raise(Atomic::UInt32 & AF_COUNTER_ARG(counter), const uint32_t AF_COUNTER_ARG(n)) {
#if AF_ENABLE_COUNTERS

    uint32_t current_value(counter.Load());
    uint32_t backoff(0);

    while (n > current_value) {
        if (counter.CompareExchangeAcquire(current_value, n)) {
            break;
        }

        Utils::Backoff(backoff);
    }

#endif
}

AF_FORCEINLINE void Counting::Lower(Atomic::UInt32 & AF_COUNTER_ARG(counter), const uint32_t AF_COUNTER_ARG(n))
{
#if AF_ENABLE_COUNTERS

    uint32_t current_value(counter.Load());
    uint32_t backoff(0);

    while (n < current_value) {
        if (counter.CompareExchangeAcquire(current_value, n)) {
            break;
        }

        Utils::Backoff(backoff);
    }

#endif
}

AF_FORCEINLINE void Counting::Accumulate(
    const Atomic::UInt32 & AF_COUNTER_ARG(counter),
    const uint32_t AF_COUNTER_ARG(id),
    uint32_t & AF_COUNTER_ARG(n)) {
#if AF_ENABLE_COUNTERS

    const uint32_t val(counter.Load());

    switch (id) {
        case COUNTER_MAILBOX_QUEUE_MAX:
        case COUNTER_QUEUE_LATENCY_LOCAL_MAX:
        case COUNTER_QUEUE_LATENCY_SHARED_MAX: {
            if (val > n) {
                n = val;
            }

            break;
        }

        case COUNTER_QUEUE_LATENCY_LOCAL_MIN:
        case COUNTER_QUEUE_LATENCY_SHARED_MIN: {
            if (val < n) {
                n = val;
            }

            break;
        }

        default: {
            n += val;
            break;
        }
    }

#endif
}


} // namespace Detail
} // namespace AF


#undef AF_COUNTER_ARG


#endif // AF_DETAIL_SCHEDULER_COUNTING_H
