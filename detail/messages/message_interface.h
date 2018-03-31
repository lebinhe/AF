#ifndef AF_DETAIL_MESSAGES_IMESSAGE_H
#define AF_DETAIL_MESSAGES_IMESSAGE_H


#include "AF/address.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/queue.h"


namespace AF
{
namespace Detail
{

/*
 * Interface describing the generic API of the message class template.
 */
class MessageInterface : public Queue<MessageInterface>::Node {
public:

    /*
     * Gets the address from which the message was sent.
     * TODO: Force-inline
     */
    inline Address From() const {
        return from_;
    }

    /*
     * Returns the memory block in which this message was allocated.
     */
    AF_FORCEINLINE void *GetBlock() const {
        AF_ASSERT(block_);
        return block_;
    }

    /*
     * Returns the size in bytes of the memory block in which this message was allocated.
     */
    AF_FORCEINLINE uint32_t GetBlockSize() const {
        AF_ASSERT(block_size_);
        return block_size_;
    }

    /*
     * Returns the message value as blind data.
     */
    AF_FORCEINLINE const void *GetMessageData() const {
        AF_ASSERT(block_);
        return block_;
    }

    /*
     * Returns the size in bytes of the message data.
     */
    virtual uint32_t GetMessageSize() const = 0;

    /*
     * Returns the name of the message type.
     * This uniquely identifies the type of the message value.
     *
     * Unless explicitly specified to avoid C++ RTTI, message names are null.
     */
    virtual const char *TypeName() const = 0;

    /*
     * Allows the message instance to destruct its constructed value object before being freed.
     */
    virtual void Release() = 0;

    virtual ~MessageInterface() {
    }

protected:
    /*
     * from: The address from which the message was sent.
     * block: The memory block containing the message.
     * block_size: The size of the memory block containing the message.
     * type_name: String identifier uniquely identifying the type of the message value.
     */
    AF_FORCEINLINE MessageInterface(
        const Address &from,
        void *const block,
        const uint32_t block_size) 
      : from_(from),
        block_(block),
        block_size_(block_size) {
    }

private:
    
    MessageInterface(const MessageInterface &other);
    MessageInterface &operator=(const MessageInterface &other);

    const Address from_;            // The address from which the message was sent.
    void *const block_;             // Pointer to the memory block containing the message.
    const uint32_t block_size_;     // Total size of the message memory block in bytes.
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_MESSAGES_IMESSAGE_H
