#ifndef AF_DETAIL_HANDLERS_DEFAULTFALLBACKHANDLER_H
#define AF_DETAIL_HANDLERS_DEFAULTFALLBACKHANDLER_H


#if AF_ENABLE_UNHANDLED_MESSAGE_CHECKS
#include <stdio.h>
#endif // AF_ENABLE_UNHANDLED_MESSAGE_CHECKS

#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#if AF_ENABLE_UNHANDLED_MESSAGE_CHECKS
#define AF_FALLBACK_HANDLER_ARG(x) x
#else
#define AF_FALLBACK_HANDLER_ARG(x)
#endif //AF_ENABLE_UNHANDLED_MESSAGE_CHECKS


namespace AF
{
namespace Detail
{

class DefaultFallbackHandler {
public:
    inline void Handle(const void *const data, const uint32_t size, const Address from);
};


inline void DefaultFallbackHandler::Handle(
    const void *const AF_FALLBACK_HANDLER_ARG(data),
    const uint32_t AF_FALLBACK_HANDLER_ARG(size),
    const Address AF_FALLBACK_HANDLER_ARG(from))
{
#if AF_ENABLE_UNHANDLED_MESSAGE_CHECKS

    fprintf(stderr, "Unhandled message of %d bytes sent from address %d:\n", size, from.AsInteger());

    // Dump the message data as hex words.
    if (data) {
        const char *const format("[%d] 0x%08x\n");

        const unsigned int *const begin(reinterpret_cast<const unsigned int *>(data));
        const unsigned int *const end(begin + size / sizeof(unsigned int));

        for (const unsigned int *word(begin); word != end; ++word) {
            fprintf(stderr, format, static_cast<int>(word - begin), static_cast<int>(*word));
        }
    }

    AF_FAIL();

#endif // AF_ENABLE_UNHANDLED_MESSAGE_CHECKS
}


} // namespace Detail
} // namespace AF


#undef AF_FALLBACK_HANDLER_ARG


#endif // AF_DETAIL_HANDLERS_DEFAULTFALLBACKHANDLER_H
