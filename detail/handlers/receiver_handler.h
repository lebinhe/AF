#ifndef AF_DETAIL_HANDLERS_RECEIVERHANDLER_H
#define AF_DETAIL_HANDLERS_RECEIVERHANDLER_H

#include "AF/address.h"
#include "AF/assert.h"

#include "AF/defines.h"

#include "AF/detail/messages/message_interface.h"
#include "AF/detail/messages/message.h"
#include "AF/detail/messages/message_cast.h"
#include "AF/detail/messages/message_traits.h"

#include "AF/detail/handlers/receiver_handler_interface.h"


namespace AF
{
namespace Detail
{

template <class ObjectType, class ValueType>
class ReceiverHandler : public ReceiverHandlerInterface {
public:
    typedef void (ObjectType::*HandlerFunction)(const ValueType &message, const Address from);

    inline ReceiverHandler(ObjectType *const object, HandlerFunction function) 
      : object_(object),
        handler_function_(function) {
    }

    inline virtual ~ReceiverHandler() {
    }

    AF_FORCEINLINE HandlerFunction GetHandlerFunction() const {
        return handler_function_;
    }

    inline virtual const char *GetMessageTypeName() const {
        return MessageTraits<ValueType>::TYPE_NAME;
    }

    inline virtual bool Handle(const MessageInterface *const message) const {
        typedef MessageCast<MessageTraits<ValueType>::HAS_TYPE_NAME> MessageCaster;
    
        AF_ASSERT(object_);
        AF_ASSERT(handler_function_);
        AF_ASSERT(message);

        // Try to convert the message, of unknown type, to message of the assumed type.
        const Message<ValueType> *const typed_message = MessageCaster:: template CastMessage<ValueType>(message);
        if (typed_message) {
            // Call the handler, passing it the message value and from address.
            (object_->*handler_function_)(typed_message->Value(), typed_message->From());
            return true;
        }

        return false;
    }

private:
    ReceiverHandler(const ReceiverHandler &other);
    ReceiverHandler &operator=(const ReceiverHandler &other);

    ObjectType *object_;                        // Pointer to the object owning the handler function.
    const HandlerFunction handler_function_;    // Pointer to the handler member function on the owning object.
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_RECEIVERHANDLER_H
