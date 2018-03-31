#ifndef AF_DETAIL_THREADING_CLOCK_H
#define AF_DETAIL_THREADING_CLOCK_H


#include "AF/basic_types.h"
#include "AF/defines.h"

#include <time.h>
#include <sys/time.h>

namespace AF
{
namespace Detail
{

/*
 * Static helper class that queries system performance timers.
 */
class Clock {
public:
    AF_FORCEINLINE static uint64_t GetTicks() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t) ts.tv_sec * NANOSECONDS_PER_SECOND + (uint64_t) ts.tv_nsec;
    }

    AF_FORCEINLINE static uint64_t GetFrequency() {
        return static_.ticks_per_second_;
    }

private:
    struct Static {
        Static();
        uint64_t ticks_per_second_;
    };

    static const uint64_t NANOSECONDS_PER_SECOND = 1000000000ULL;
    static Static static_;
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_THREADING_CLOCK_H
