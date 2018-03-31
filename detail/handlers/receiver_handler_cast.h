#ifndef AF_DETAIL_HANDLERS_RECEIVERHANDLERCAST_H
#define AF_DETAIL_HANDLERS_RECEIVERHANDLERCAST_H

#include "AF/assert.h"

#include "AF/defines.h"

#include "AF/detail/handlers/receiver_handler.h"
#include "AF/detail/handlers/receiver_handler_interface.h"

#include "AF/detail/messages/message_traits.h"


namespace AF
{
namespace Detail
{

template <class ObjectType, bool HAS_TYPE_NAME>
class ReceiverHandlerCast {
public:
    template <class ValueType>
    AF_FORCEINLINE static const ReceiverHandler<ObjectType, ValueType> *CastHandler(const ReceiverHandlerInterface *const handler) {
        AF_ASSERT(handler);

        // If explicit type names are used then they must be defined for all message types.
        AF_ASSERT_MSG(handler->GetMessageTypeName(), "Missing type name for message type");

        // Compare the handlers using type names.
        if (handler->GetMessageTypeName() != MessageTraits<ValueType>::TYPE_NAME) {
            return 0;
        }

        // Convert the given message handler to a handler for the known type.
        typedef ReceiverHandler<ObjectType, ValueType> HandlerType;
        return reinterpret_cast<const HandlerType *>(handler);
    }
};


template <class ObjectType>
class ReceiverHandlerCast<ObjectType, false> {
public:
    template <class ValueType>
    AF_FORCEINLINE static const ReceiverHandler<ObjectType, ValueType> *CastHandler(const ReceiverHandlerInterface *const handler) {
        AF_ASSERT(handler);

        // Explicit type names must be defined for all message types or none at all.
        AF_ASSERT_MSG(handler->GetMessageTypeName() == 0, "Type names specified for only some message types!");

        // Try to convert the given message handler to this type.
        typedef ReceiverHandler<ObjectType, ValueType> HandlerType;
        return dynamic_cast<const HandlerType *>(handler);
    }
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_RECEIVERHANDLERCAST_H
