#ifndef AF_DETAIL_HANDLERS_MESSAGEHANDLER_H
#define AF_DETAIL_HANDLERS_MESSAGEHANDLER_H

#include "AF/address.h"
#include "AF/assert.h"
#include "AF/defines.h"

#include "AF/detail/handlers/message_handler_interface.h"

#include "AF/detail/messages/message.h"
#include "AF/detail/messages/message_cast.h"
#include "AF/detail/messages/message_interface.h"
#include "AF/detail/messages/message_traits.h"


namespace AF
{

class Actor;

namespace Detail
{

/*
 * Instantiable class template that remembers a message handler function and
 * the type of message it accepts. It is responsible for checking whether
 * incoming messages are of the type accepted by the handler, and executing the
 * handler for messages that match.
 * 
 * ActorType: The type of actor whose message handlers are considered.
 * ValueType: The type of message handled by this message handler.
 */
template <class ActorType, class ValueType>
class MessageHandler : public MessageHandlerInterface {
public:
    typedef void (ActorType::*HandlerFunction)(const ValueType &message, const Address from);

    inline explicit MessageHandler(HandlerFunction function) : handler_function_(function) {
    }

    inline virtual ~MessageHandler() {
    }

    AF_FORCEINLINE HandlerFunction GetHandlerFunction() const {
        return handler_function_;
    }

    inline virtual const char *GetMessageTypeName() const {
        return MessageTraits<ValueType>::TYPE_NAME;
    }

    /*
     * Handles the given message, if it's of the type accepted by the handler.
     * return True, if the handler handled the message.
     * The message is not consumed by the handler; just acted on or ignored.
     * The message will be automatically destroyed when all handlers have seen it.
     */
    inline virtual bool Handle(Actor *const actor, const MessageInterface *const message) {
        typedef MessageCast<MessageTraits<ValueType>::HAS_TYPE_NAME> MessageCaster;
    
        AF_ASSERT(actor);
        AF_ASSERT(handler_function_);
        AF_ASSERT(message);

        // Try to convert the message, of unknown type, to message of the assumed type.
        const Message<ValueType> *const typed_message = MessageCaster:: template CastMessage<ValueType>(message);
        if (typed_message) {
            // Call the handler, passing it the message value and from address.
            ActorType *const typed_actor = static_cast<ActorType *>(actor);
            (typed_actor->*handler_function_)(typed_message->Value(), typed_message->From());

            return true;
        }

        return false;
    }

private:
    MessageHandler(const MessageHandler &other);
    MessageHandler &operator=(const MessageHandler &other);

    const HandlerFunction handler_function_;     // Pointer to a handler member function on an actor.
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_MESSAGEHANDLER_H
