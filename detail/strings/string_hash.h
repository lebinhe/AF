#ifndef AF_DETAIL_STRINGS_STRINGHASH_H
#define AF_DETAIL_STRINGS_STRINGHASH_H


#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"


namespace AF
{
namespace Detail
{

/*
 * Simple hash utility for C strings.
 */
class StringHash {
public:
    enum {
        RANGE = 256
    };

    AF_FORCEINLINE static uint32_t Compute(const char *const str) {
        AF_ASSERT(str);

        // XOR the first n characters of the string together.
        const char *const end(str + 64);

        const char *ch(str);
        uint8_t hash(0);

        while (ch != end && *ch != '\0') {
            hash ^= static_cast<uint8_t>(*ch);
            ++ch;
        }

        return static_cast<uint32_t>(hash);
    }
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_STRINGS_STRINGHASH_H
