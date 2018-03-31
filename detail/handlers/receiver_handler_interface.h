#ifndef AF_DETAIL_HANDLERS_RECEIVERHANDLER_INTERFACE_H
#define AF_DETAIL_HANDLERS_RECEIVERHANDLER_INTERFACE_H


#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"

#include "AF/detail/messages/message_interface.h"


namespace AF
{
namespace Detail
{

/*
 * Baseclass that allows message handlers of various types to be stored in lists.
 */
class ReceiverHandlerInterface : public List<ReceiverHandlerInterface>::Node {
public:
    AF_FORCEINLINE ReceiverHandlerInterface() {
    }

    inline virtual ~ReceiverHandlerInterface() {
    }

    /*
     * Returns the unique name of the message type handled by this handler.
     */
    virtual const char *GetMessageTypeName() const = 0;

    virtual bool Handle(const MessageInterface *const message) const = 0;

private:
    ReceiverHandlerInterface(const ReceiverHandlerInterface &other);
    ReceiverHandlerInterface &operator=(const ReceiverHandlerInterface &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_RECEIVERHANDLER_INTERFACE_H
