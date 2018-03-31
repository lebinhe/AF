#ifndef AF_DETAIL_UTILS_UTILS_H
#define AF_DETAIL_UTILS_UTILS_H


#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include <stdlib.h>
#include <string.h>
#include <thread>

namespace AF
{
namespace Detail
{

class Utils {
public:

    // Waste some cycles to avoid busy-waiting on a shared memory resource.
    // This function is intentionally not force-inlined.
    inline static void Backoff(uint32_t &backoff);

    // Put the calling thread to sleep for a given number of milliseconds.
    inline static void SleepThread(const uint32_t milliseconds);

private:
    Utils(const Utils &other);
    Utils &operator=(const Utils &other);
};


inline void Utils::Backoff(uint32_t &backoff) {
    std::this_thread::yield();
}

AF_FORCEINLINE void Utils::SleepThread(const uint32_t milliseconds) {
    AF_ASSERT(milliseconds < 1000);
    std::this_thread::sleep_for(std::chrono::microseconds(milliseconds * 1000));
}

/*
 * Union that combines a framework index and a mailbox index.
 */
union Index {
    AF_FORCEINLINE Index() : uint32_(0) {
    }

    AF_FORCEINLINE Index(const uint32_t framework, const uint32_t index) : uint32_(0) {
        componets_.framework_ = framework;
        componets_.index_ = index;
    }

    AF_FORCEINLINE Index(const Index &other) : uint32_(other.uint32_) {
    }

    AF_FORCEINLINE Index &operator=(const Index &other) {
        uint32_ = other.uint32_;
        return *this;
    }

    AF_FORCEINLINE bool operator==(const Index &other) const {
        return (uint32_ == other.uint32_);
    }

    AF_FORCEINLINE bool operator!=(const Index &other) const {
        return (uint32_ != other.uint32_);
    }

    AF_FORCEINLINE bool operator<(const Index &other) const {
        return (uint32_ < other.uint32_);
    }

    uint32_t uint32_;               // Unsigned 32-bit value.

    struct {
        uint32_t framework_ : 12;  // Integer index identifying the framework within the local process (zero indicates a receiver).
        uint32_t index_ : 20;      // Integer index of the actor within the framework (or receiver within the process).

    } componets_;
};


/*
 * Generates string names from numbers.
 */
class NameGenerator {
public:
    inline static void Generate(
        char *const buffer,
        const uint32_t id) {
        AF_ASSERT(buffer);
        sprintf(buffer, "%x", id);
    }

    inline static void Combine(
        char *const buffer,
        const uint32_t buffer_size,
        const char *const raw_name,
        const char *const framework_name) {
        AF_ASSERT(buffer);
        AF_ASSERT(raw_name);

        if (strlen(raw_name) + 1 < buffer_size) {
            strcpy(buffer, raw_name);
        }

        if (framework_name && strlen(buffer) + strlen(framework_name) + 2 < buffer_size) {
            strcat(buffer, ".");
            strcat(buffer, framework_name);
        }
    }
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_UTILS_UTILS_H
