#include "AF/detail/handlers/fallback_handler_collection.h"


namespace AF
{
namespace Detail
{


FallbackHandlerCollection::FallbackHandlerCollection() 
  : handler_(0),
    new_handler_(0),
    handlers_dirty_(false) {
}

FallbackHandlerCollection::~FallbackHandlerCollection() {
    Clear();
}

bool FallbackHandlerCollection::Clear() {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    // Destroy any currently set handlers.
    if (handler_) {
        handler_->~FallbackHandlerInterface();
        allocator->Free(handler_);
        handler_ = 0;
    }

    if (new_handler_) {
        new_handler_->~FallbackHandlerInterface();
        allocator->Free(new_handler_);
        new_handler_ = 0;
    }

    handlers_dirty_ = false;
    return true;
}

bool FallbackHandlerCollection::Handle(const MessageInterface *const message) {
    bool handled(false);

    AF_ASSERT(message);

    // Update the handler list if there have been changes.
    if (handlers_dirty_) {
        UpdateHandlers();
    }

    if (handler_) {
        handler_->Handle(message);
        handled = true;
    }

    return handled;
}

void FallbackHandlerCollection::UpdateHandlers() {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    handlers_dirty_ = false;

    // Destroy any currently set handler.
    if (handler_) {
        handler_->~FallbackHandlerInterface();
        allocator->Free(handler_);
        handler_ = 0;
    }

    // Make the new handler (if any) the current handler.
    handler_ = new_handler_;
    new_handler_ = 0;
}


} // namespace Detail
} // namespace AF


