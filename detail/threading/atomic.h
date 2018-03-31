#ifndef AF_DETAIL_ATOMIC_H
#define AF_DETAIL_ATOMIC_H


#include "AF/align.h"

#include "AF/basic_types.h"
#include "AF/defines.h"

#include <atomic>


namespace AF
{
namespace Detail
{
namespace Atomic
{

class UInt32 {
public:
    inline UInt32() : value_(0) {
    }

    inline explicit UInt32(const uint32_t initial_value) 
      : value_(static_cast<int32_t>(initial_value)) {
    }

    inline ~UInt32() {
    }

    AF_FORCEINLINE bool CompareExchangeAcquire(uint32_t &current_value, const uint32_t new_value) {
        return value_.compare_exchange_weak(
            current_value,
            new_value,
            std::memory_order_acquire);
    }

    AF_FORCEINLINE void Increment() {
        ++value_;
    }

    AF_FORCEINLINE void Decrement() {
        --value_;
    }

    AF_FORCEINLINE uint32_t Load() const {
        return value_.load();
    }

    AF_FORCEINLINE void Store(const uint32_t val) {
        value_.store(val);
    }

private:
    UInt32(const UInt32 &other);
    UInt32 &operator=(const UInt32 &other);

    volatile std::atomic_uint_least32_t value_;
};


} // namespace Atomic
} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_ATOMIC_H
