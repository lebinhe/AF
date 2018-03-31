#ifndef AF_DETAIL_HANDLERS_FALLBACKHANDLER_INTERFACE_H
#define AF_DETAIL_HANDLERS_FALLBACKHANDLER_INTERFACE_H


#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"

#include "AF/detail/messages/message_interface.h"


namespace AF
{
namespace Detail
{

class FallbackHandlerInterface : public List<FallbackHandlerInterface>::Node {
public:
    AF_FORCEINLINE FallbackHandlerInterface() {
    }

    inline virtual ~FallbackHandlerInterface() {
    }

    virtual void Handle(const MessageInterface *const message) const = 0;

private:
    FallbackHandlerInterface(const FallbackHandlerInterface &other);
    FallbackHandlerInterface &operator=(const FallbackHandlerInterface &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_FALLBACKHANDLER_INTERFACE_H

