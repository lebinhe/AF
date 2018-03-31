#include "AF/detail/handlers/handler_collection.h"


namespace AF
{
namespace Detail
{


HandlerCollection::HandlerCollection() 
  : handlers_(),
    new_handlers_(),
    handlers_dirty_(false) {
}

HandlerCollection::~HandlerCollection() {
    Clear();
}

void HandlerCollection::UpdateHandlers() {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    handlers_dirty_ = false;

    // Add any new handlers. We do this first in case any are already marked for deletion.
    // The handler class contains the next pointer, so handlers can only be in one list at a time.
    while (MessageHandlerInterface *const handler = new_handlers_.Front()) {
        new_handlers_.Remove(handler);
        handlers_.Insert(handler);
    }

    // Transfer all handlers to the new handler list, omitting any which are marked for deletion.    
    while (MessageHandlerInterface *const handler = handlers_.Front()) {
        handlers_.Remove(handler);
        if (handler->IsMarked()) {
            handler->~MessageHandlerInterface();
            allocator->Free(handler);
        } else {
            new_handlers_.Insert(handler);
        }
    }

    // Finally transfer the filtered handlers back in the actual list.
    while (MessageHandlerInterface *const handler = new_handlers_.Front()) {
        new_handlers_.Remove(handler);
        handlers_.Insert(handler);
    }
}


} // namespace Detail
} // namespace AF


