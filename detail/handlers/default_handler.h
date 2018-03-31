#ifndef AF_DETAIL_HANDLERS_DEFAULTHANDLER_H
#define AF_DETAIL_HANDLERS_DEFAULTHANDLER_H

#include "AF/address.h"
#include "AF/assert.h"
#include "AF/defines.h"

#include "AF/detail/handlers/default_handler_interface.h"

#include "AF/detail/messages/message_interface.h"


namespace AF
{

class Actor;

namespace Detail
{

template <class ActorType>
class DefaultHandler : public DefaultHandlerInterface {
public:
    typedef void (ActorType::*HandlerFunction)(const Address from);

    AF_FORCEINLINE explicit DefaultHandler(HandlerFunction function) 
      : handler_function_(function) {
    }

    inline virtual ~DefaultHandler() {
    }

    inline virtual void Handle(Actor *const actor, const MessageInterface *const message) const;

private:
    DefaultHandler(const DefaultHandler &other);
    DefaultHandler &operator=(const DefaultHandler &other);

    const HandlerFunction handler_function_;     // Pointer to a handler member function on the actor.
};


template <class ActorType>
inline void DefaultHandler<ActorType>::Handle(Actor *const actor, const MessageInterface *const message) const {
    AF_ASSERT(actor);
    AF_ASSERT(message);
    AF_ASSERT(handler_function_);

    // Call the handler, passing it the from address.
    // We can't pass the value because we don't even know the type.
    ActorType *const typed_actor = static_cast<ActorType *>(actor);
    (typed_actor->*handler_function_)(message->From());
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_HANDLERS_DEFAULTHANDLER_H
