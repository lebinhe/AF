#ifndef AF_DETAIL_HANDLERS_FALLBACKHANDLER_H
#define AF_DETAIL_HANDLERS_FALLBACKHANDLER_H


#include "AF/address.h"
#include "AF/assert.h"
#include "AF/defines.h"

#include "AF/detail/handlers/fallback_handler_interface.h"
#include "AF/detail/messages/message_interface.h"


namespace AF
{
namespace Detail
{

template <class ObjectType>
class FallbackHandler : public FallbackHandlerInterface {
public:
    typedef void (ObjectType::*HandlerFunction)(const Address from);

    AF_FORCEINLINE FallbackHandler(ObjectType *const object, HandlerFunction function) 
      : object_(object),
        handler_function_(function) {
    }

    inline virtual ~FallbackHandler() {
    }

    inline virtual void Handle(const MessageInterface *const message) const {
        AF_ASSERT(object_);
        AF_ASSERT(handler_function_);
        AF_ASSERT(message);

        // Call the handler, passing it the from address.
        (object_->*handler_function_)(message->From());
    }

private:
    FallbackHandler(const FallbackHandler &other);
    FallbackHandler &operator=(const FallbackHandler &other);

    ObjectType *object_;                         // Pointer to the object owning the handler function.
    const HandlerFunction handler_function_;     // Pointer to the handler member function on the owning object.
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_FALLBACKHANDLER_H
