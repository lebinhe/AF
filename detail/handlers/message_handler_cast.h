#ifndef AF_DETAIL_HANDLERS_MESSAGEHANDLERCAST_H
#define AF_DETAIL_HANDLERS_MESSAGEHANDLERCAST_H

#include "AF/assert.h"

#include "AF/defines.h"

#include "AF/detail/handlers/message_handler.h"
#include "AF/detail/handlers/message_handler_interface.h"

#include "AF/detail/messages/message_traits.h"


namespace AF
{
namespace Detail
{

/*
 * Dynamic cast utility for message handler pointers.
 * A cast utility that can be used to dynamically cast a message handler of unknown type
 * to a message handler of a known type at runtime, using stored runtime type information.
 * If the unknown message handler is of the target type then the cast succeeds and a pointer
 * to the typecast message handler is returned, otherwise a null pointer is returned.
 */
template <class ActorType, bool HAS_TYPE_NAME>
class MessageHandlerCast {
public:
    // ValueType: The value type of the target message handler.
    // handler: A pointer to the message handler of unknown type.
    // return A pointer to the converted message handler, or null if the types don't match.
    template <class ValueType>
    AF_FORCEINLINE static const MessageHandler<ActorType, ValueType> *CastHandler(const MessageHandlerInterface *const handler) {
        AF_ASSERT(handler);

        // If explicit type names are used then they must be defined for all message types.
        AF_ASSERT_MSG(handler->GetMessageTypeName(), "Missing type name for message type");

        // Compare the handlers using type names.
        if (handler->GetMessageTypeName() != MessageTraits<ValueType>::TYPE_NAME) {
            return 0;
        }

        // Convert the given message handler to a handler for the known type.
        typedef MessageHandler<ActorType, ValueType> HandlerType;
        return reinterpret_cast<const HandlerType *>(handler);
    }
};


// Specialization of the MessageHandlerCast for the case where the message type has no type name.
// This specialization uses C++ built-in RTTI instead of explicitly stored type names.
template <class ActorType>
class MessageHandlerCast<ActorType, false> {
public:
    template <class ValueType>
    AF_FORCEINLINE static const MessageHandler<ActorType, ValueType> *CastHandler(const MessageHandlerInterface *const handler) {
        AF_ASSERT(handler);

        // Explicit type names must be defined for all message types or none at all.
        AF_ASSERT_MSG(handler->GetMessageTypeName() == 0, "Type names specified for only some message types!");

        // Try to convert the given message handler to this type.
        typedef MessageHandler<ActorType, ValueType> HandlerType;
        return dynamic_cast<const HandlerType *>(handler);
    }
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_MESSAGEHANDLERCAST_H
