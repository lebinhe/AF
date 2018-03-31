#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/threading/clock.h"


namespace AF
{
namespace Detail
{


Clock::Static Clock::static_;


Clock::Static::Static() : ticks_per_second_(NANOSECONDS_PER_SECOND) {
}


} // namespace Detail
} // namespace AF


