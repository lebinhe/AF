#ifndef AF_DETAIL_HANDLERS_BLINDDEFAULTHANDLER_H
#define AF_DETAIL_HANDLERS_BLINDDEFAULTHANDLER_H


#include "AF/address.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/handlers/default_handler_interface.h"

#include "AF/detail/messages/message.h"
#include "AF/detail/messages/message_interface.h"


namespace AF
{

class Actor;

namespace Detail
{

/*
 * Instantiable class template that remembers a 'blind' default handler function.
 * A blind handler is one that takes the message as blind data: a void pointer and a size.
 */
template <class ActorType>
class BlindDefaultHandler : public DefaultHandlerInterface {
public:
     // Pointer to a member function of the actor type, which is the handler.
    typedef void (ActorType::*HandlerFunction)(const void *const data, const uint32_t size, const Address from);

    AF_FORCEINLINE explicit BlindDefaultHandler(HandlerFunction function) 
      : handler_function_(function) {
    }

    inline virtual ~BlindDefaultHandler() {
    }

     // Handles the given message.
     // The message is not consumed by the handler; just acted on or ignored.
     // The message will be automatically destroyed when all handlers have seen it.
    inline virtual void Handle(Actor *const actor, const MessageInterface *const message) const;

private:
    BlindDefaultHandler(const BlindDefaultHandler &other);
    BlindDefaultHandler &operator=(const BlindDefaultHandler &other);

    const HandlerFunction handler_function_;     // Pointer to a handler member function on the actor.
};


template <class ActorType>
inline void BlindDefaultHandler<ActorType>::Handle(Actor *const actor, const MessageInterface *const message) const {
    AF_ASSERT(actor);
    AF_ASSERT(message);
    AF_ASSERT(handler_function_);

    // Call the handler, passing it the from address and also the message as blind data.
    ActorType *const typed_actor = static_cast<ActorType *>(actor);

    const void *const message_data(message->GetMessageData());
    const uint32_t message_size(message->GetMessageSize());
    const AF::Address from(message->From());

    AF_ASSERT(message_data && message_size);

    (typed_actor->*handler_function_)(message_data, message_size, from);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_BLINDDEFAULTHANDLER_H
