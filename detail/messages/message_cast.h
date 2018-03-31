#ifndef AF_DETAIL_HANDLERS_MESSAGECAST_H
#define AF_DETAIL_HANDLERS_MESSAGECAST_H


#include "AF/assert.h"

#include "AF/defines.h"

#include "AF/detail/messages/message.h"
#include "AF/detail/messages/message_interface.h"
#include "AF/detail/messages/message_traits.h"


namespace AF
{
namespace Detail
{

/*
 * Dynamic cast utility for message pointers.
 *
 * A cast utility that can be used to dynamically cast a message of unknown type
 * to a message of a known type at runtime, using stored runtime type information.
 * If the unknown message is of the target type then the cast succeeds and a pointer
 * to the typecast message is returned, otherwise a null pointer is returned.
 */
template <bool HAS_TYPE_ID>
class MessageCast {
public:
    template <class ValueType>
    AF_FORCEINLINE static const Message<ValueType> *CastMessage(const MessageInterface *const message) {
        AF_ASSERT(message);

        // If explicit type names are used then they must be defined for all message types.
        AF_ASSERT_MSG(message->TypeName(), "Message type has null type name");

        // Check the type of the message using the type name it carries, which was set on creation.
        if (message->TypeName() == MessageTraits<ValueType>::TYPE_NAME) {
            // Hard-convert the given message to the indicated type.
            return reinterpret_cast<const Message<ValueType> *>(message);
        }

        return 0;
    }
};


// Specialization of MessageCast for the case where the message has no type name.
// This specialization uses C++ built-in RTTI instead of the explicitly stored type names.
template <>
class MessageCast<false>
{
public:
    template <class ValueType>
    AF_FORCEINLINE static const Message<ValueType> *CastMessage(const MessageInterface *const message) {
        AF_ASSERT(message);

#if AF_ENABLE_MESSAGE_REGISTRATION_CHECKS
        // We're running the specialization of this class that's used for messages without
        // registered type ids/names, so this message hasn't been registered.
        AF_FAIL_MSG("Message type is not registered");
#endif // AF_ENABLE_MESSAGE_REGISTRATION_CHECKS

        // Explicit type IDs must be defined for all message types or none at all.
        AF_ASSERT_MSG(message->TypeName() == 0, "Only some message types are registered!");

        return dynamic_cast<const Message<ValueType> *>(message);
    }
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_MESSAGECAST_H
