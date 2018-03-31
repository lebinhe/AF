#ifndef AF_DETAIL_STRINGS_STRING_H
#define AF_DETAIL_STRINGS_STRING_H


#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/strings/string_pool.h"


namespace AF
{
namespace Detail
{

/*
 * A copyable string type that is a lightweight wrapper around a pooled string.
 */
class String {
public:
    AF_FORCEINLINE String() : value_(0) {
    }

    AF_FORCEINLINE explicit String(const char *const str) : value_(0) {
        if (str) {
            value_ = StringPool::Get(str);
        }
    }

    AF_FORCEINLINE bool IsNull() const {
        return (value_ == 0);
    }

    AF_FORCEINLINE const char *GetValue() const {
        return value_;
    }

    AF_FORCEINLINE bool operator==(const String &other) const {
        // Pooled strings are unique so we can compare their addresses.
        // This works for null strings too, whose value pointers are zero.
        return (value_ == other.value_);
    }

    AF_FORCEINLINE bool operator!=(const String &other) const {
        return !operator==(other);
    }

    AF_FORCEINLINE bool operator<(const String &other) const {
        // Arbitrary less-than based on cheap pointer comparison.
        // This works for null strings too, whose value pointers are zero.
        return (value_ < other.value_);
    }

private:
    const char *value_;
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_STRINGS_STRING_H
