#ifndef AF_DETAIL_MESSAGES_MESSAGESIZE_H
#define AF_DETAIL_MESSAGES_MESSAGESIZE_H


#include "AF/basic_types.h"
#include "AF/defines.h"


namespace AF
{
namespace Detail
{

/*
 * Helper that tells us the allocated size of a message value type.
 */
template <class ValueType>
class MessageSize {
public:
    AF_FORCEINLINE static uint32_t GetSize() {
        uint32_t value_size(sizeof(ValueType));
        const uint32_t minimum_allocation_size(4);

        // Empty structs passed as message values have a size of one byte, which we don't like.
        // To be on the safe side we round every allocation up to at least four bytes.
        // If we don't then the data that follows won't be word-aligned.
        if (value_size < minimum_allocation_size) {
            value_size = minimum_allocation_size;
        }

        return value_size;
    }

private:
	MessageSize();
    MessageSize(const MessageSize &other);
    MessageSize &operator=(const MessageSize &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_MESSAGES_MESSAGESIZE_H

