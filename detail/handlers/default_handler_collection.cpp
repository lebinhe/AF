#include "AF/detail//handlers/default_handler_collection.h"


namespace AF
{
namespace Detail
{


DefaultHandlerCollection::DefaultHandlerCollection() 
  : handler_(0),
    new_handler_(0),
    handlers_dirty_(false) {
}

DefaultHandlerCollection::~DefaultHandlerCollection() {
    Clear();
}

bool DefaultHandlerCollection::Clear() {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    // Destroy any currently set handlers.
    if (handler_) {
        handler_->~DefaultHandlerInterface();
        allocator->Free(handler_);
        handler_ = 0;
    }

    if (new_handler_) {
        new_handler_->~DefaultHandlerInterface();
        allocator->Free(new_handler_);
        new_handler_ = 0;
    }

    handlers_dirty_ = false;
    return true;
}

bool DefaultHandlerCollection::Handle(Actor *const actor, const MessageInterface *const message) {
    bool handled(false);

    AF_ASSERT(actor);
    AF_ASSERT(message);

    // Update the handler list if there have been changes.
    if (handlers_dirty_) {
        UpdateHandlers();
    }

    if (handler_) {
        handler_->Handle(actor, message);
        handled = true;
    }

    return handled;
}

void DefaultHandlerCollection::UpdateHandlers() {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    handlers_dirty_ = false;

    // Destroy any currently set handler.
    if (handler_) {
        handler_->~DefaultHandlerInterface();
        allocator->Free(handler_);
        handler_ = 0;
    }

    // Make the new handler (if any) the current handler.
    handler_ = new_handler_;
    new_handler_ = 0;
}


} // namespace Detail
} // namespace AF


