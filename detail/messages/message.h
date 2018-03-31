#ifndef AF_DETAIL_MESSAGES_MESSAGE_H
#define AF_DETAIL_MESSAGES_MESSAGE_H


#include "AF/allocator_manager.h"
#include "AF/align.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/messages/message_interface.h"
#include "AF/detail/messages/message_size.h"
#include "AF/detail/messages/message_traits.h"

#include <new>


namespace AF
{
namespace Detail
{

/*
 * Message class, used for sending data between actors.
 */
template <class ValueType>
class Message : public MessageInterface {
public:
    typedef Message<ValueType> ThisType;

    AF_FORCEINLINE virtual ~Message() {
    }

    // Returns the memory block size required to initialize a message of this type.
    AF_FORCEINLINE static uint32_t GetSize() {
        return MessageSize<ValueType>::GetSize() + sizeof(ThisType);
    }

    // Returns the memory block alignment required to initialize a message of this type.
    AF_FORCEINLINE static uint32_t GetAlignment() {
        return MessageAlignment<ValueType>::ALIGNMENT;
    }

    // Initializes a message of this type in the provided memory block.
    // The block is allocated and freed by the caller.
    AF_FORCEINLINE static ThisType *Initialize(void *const block, const ValueType &value, const Address &from) {
        AF_ASSERT(block);

        // Instantiate a new instance of the value type in aligned position at the start of the buffer.
        // We assume that the message value type can be copy-constructed.
        // Messages are explicitly copied to avoid shared memory.
        ValueType *const pvalue = new (block) ValueType(value);

        // Allocate the message object immediately after the value, passing it the value's address.
        char *const pobject(reinterpret_cast<char *>(pvalue) + MessageSize<ValueType>::GetSize());
        return new (pobject) ThisType(pvalue, from);
    }

    // Returns the name of the message type.
    virtual const char *TypeName() const {
        return MessageTraits<ValueType>::TYPE_NAME;
    }

    // Allows the message instance to destruct its constructed value object before being freed.
    virtual void Release() {
        // The referenced block owned by this message is blind data, but we know it holds
        // an instance of the value type, that needs to be explicitly destructed.
        // We have to call the destructor manually because we constructed the object in-place.
        Value().~ValueType();
    }

    // Returns the size in bytes of the message value.
    virtual uint32_t GetMessageSize() const {
        // Calculate the size of the message value itself. There's no padding between the
        // message value block and the Message object that follows it.
        return GetBlockSize() - static_cast<uint32_t>(sizeof(ThisType));
    }

    // Gets the value carried by the message.
    AF_FORCEINLINE const ValueType &Value() const {
        // The value is stored at the start of the memory block.
        return *reinterpret_cast<const ValueType *>(GetBlock());
    }

private:
    AF_FORCEINLINE Message(void *const block, const Address &from) 
      : MessageInterface(from, block, ThisType::GetSize()) {
        AF_ASSERT(block);
    }

    Message(const Message &other);
    Message &operator=(const Message &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_MESSAGES_MESSAGE_H

