#ifndef AF_DETAIL_MESSAGES_MESSAGECREATOR_H
#define AF_DETAIL_MESSAGES_MESSAGECREATOR_H


#include "AF/allocator_interface.h"
#include "AF/address.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/messages/message.h"
#include "AF/detail/messages/message_interface.h"


namespace AF
{
namespace Detail
{

/*
 * Helper class that constructs and destroys AF's internal message objects.
 */
class MessageCreator {
public:
    template <class ValueType>
    inline static Message<ValueType> *Create(
        AllocatorInterface *const message_allocator,
        const ValueType &value,
        const Address &from);

    inline static void Destroy(
        AllocatorInterface *const message_allocator,
        MessageInterface *const message);
};


template <class ValueType>
AF_FORCEINLINE Message<ValueType> *MessageCreator::Create(
    AllocatorInterface *const message_allocator,
    const ValueType &value,
    const Address &from) {

    typedef Message<ValueType> MessageType;
    const uint32_t block_size(MessageType::GetSize());
    const uint32_t block_alignment(MessageType::GetAlignment());

    // Allocate a message. It'll be deleted by the actor after it's been handled.
    // We allocate a block from the global free list for caching of common allocations.
    // The free list is thread-safe so we don't need to lock it ourselves.
    void *const block = message_allocator->AllocateAligned(block_size, block_alignment);
    if (block) {
        return MessageType::Initialize(block, value, from);
    }

    return 0;
}

AF_FORCEINLINE void MessageCreator::Destroy(
    AllocatorInterface *const message_allocator,
    MessageInterface *const message) {
    // Call release on the message to give it chance to destruct its value type.
    message->Release();

    // Destruct the message object itself.
    // This calls the derived Message class destructor by virtual function magic.
    message->~MessageInterface();

    // Return the block to the global free list.
    message_allocator->FreeWithSize(message->GetBlock(), message->GetBlockSize());
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_MESSAGES_MESSAGECREATOR_H
