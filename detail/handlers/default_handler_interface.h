#ifndef AF_DETAIL_HANDLERS_DEFAULTHANDLER_INTERFACE_H
#define AF_DETAIL_HANDLERS_DEFAULTHANDLER_INTERFACE_H


#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/messages/message_interface.h"


namespace AF
{

class Actor;

namespace Detail
{

class DefaultHandlerInterface {
public:
    inline DefaultHandlerInterface() {
    }

    inline virtual ~DefaultHandlerInterface() {
    }

    // Handles the given message.
    virtual void Handle(Actor *const actor, const MessageInterface *const message) const = 0;

private:
    DefaultHandlerInterface(const DefaultHandlerInterface &other);
    DefaultHandlerInterface &operator=(const DefaultHandlerInterface &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_DEFAULTHANDLER_INTERFACE_H
