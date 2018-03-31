#ifndef AF_DETAIL_HANDLERS_BLINDFALLBACKHANDLER_H
#define AF_DETAIL_HANDLERS_BLINDFALLBACKHANDLER_H


#include "AF/address.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/handlers/fallback_handler_interface.h"

#include "AF/detail/messages/message_interface.h"


namespace AF
{
namespace Detail
{

/*
 * Instantiable class template that remembers a 'blind' fallback message handler function
 * registered with a framework and called for messages that are undelivered or unhandled.
 * A blind handler is one that takes the message as blind data: a void pointer and a size.
 */
template <class ObjectType>
class BlindFallbackHandler : public FallbackHandlerInterface {
public:
    // Pointer to a member function of a handler object.
    typedef void (ObjectType::*HandlerFunction)(const void *const data, const uint32_t size, const Address from);

    AF_FORCEINLINE BlindFallbackHandler(ObjectType *const object, HandlerFunction function) 
      : object_(object),
        handler_function_(function) {
    }

    inline virtual ~BlindFallbackHandler() {
    }

    inline virtual void Handle(const MessageInterface *const message) const {
        AF_ASSERT(object_);
        AF_ASSERT(handler_function_);
        AF_ASSERT(message);

        // Call the handler, passing it the from address and also the message as blind data.
        const void *const message_data(message->GetMessageData());
        const uint32_t message_size(message->GetMessageSize());
        const AF::Address from(message->From());

        AF_ASSERT(message_data && message_size);

        (object_->*handler_function_)(message_data, message_size, from);
    }

private:
    BlindFallbackHandler(const BlindFallbackHandler &other);
    BlindFallbackHandler &operator=(const BlindFallbackHandler &other);

    ObjectType *object_;                        // Pointer to the object owning the handler function.
    const HandlerFunction handler_function_;    // Pointer to the handler member function on the owning object.
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_BLINDFALLBACKHANDLER_H
