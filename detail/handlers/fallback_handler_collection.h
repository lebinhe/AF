#ifndef AF_DETAIL_HANDLERS_FALLBACKHANDLERCOLLECTION_H
#define AF_DETAIL_HANDLERS_FALLBACKHANDLERCOLLECTION_H


#include <new>

#include "AF/address.h"
#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/handlers/blind_fallback_handler.h"
#include "AF/detail/handlers/fallback_handler.h"
#include "AF/detail/handlers/fallback_handler_interface.h"

#include "AF/detail/messages/message_interface.h"


namespace AF
{
namespace Detail
{

class FallbackHandlerCollection {
public:
    FallbackHandlerCollection();

    ~FallbackHandlerCollection();

    template <class ObjectType>
    inline bool Set(
        ObjectType *const handler_object,
        void (ObjectType::*handler)(const Address from));

    template <class ObjectType>
    inline bool Set(
        ObjectType *const handler_object,
        void (ObjectType::*handler)(const void *const data, const uint32_t size, const Address from));

    bool Clear();

    bool Handle(const MessageInterface *const message);

private:
    FallbackHandlerCollection(const FallbackHandlerCollection &other);
    FallbackHandlerCollection &operator=(const FallbackHandlerCollection &other);

    void UpdateHandlers();

    FallbackHandlerInterface *handler_;         // Currently registered handler (only one is supported).
    FallbackHandlerInterface *new_handler_;      // New registered handler (will replace the registered one on update).
    bool handlers_dirty_;                // Flag indicating that the handlers are out of date.
};


template <class ObjectType>
inline bool FallbackHandlerCollection::Set(
    ObjectType *const handler_object,
    void (ObjectType::*handler)(const Address from)) {
    typedef FallbackHandler<ObjectType> MessageHandlerType;

    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    handlers_dirty_ = true;

    // Destroy any previously set new handler.
    // Note that all handler objects are the same size.
    if (new_handler_) {
        new_handler_->~FallbackHandlerInterface();
        allocator->FreeWithSize(new_handler_, sizeof(MessageHandlerType));
        new_handler_ = 0;
    }

    if (handler_object && handler) {
        // Allocate memory for a message handler object.
        void *const memory = allocator->Allocate(sizeof(MessageHandlerType));
        if (memory == 0) {
            return false;
        }

        // Construct a handler object to remember the function pointer and message value type.
        new_handler_ = new (memory) MessageHandlerType(handler_object, handler);
    }

    return true;
}


template <class ObjectType>
inline bool FallbackHandlerCollection::Set(
    ObjectType *const handler_object,
    void (ObjectType::*handler)(const void *const data, const uint32_t size, const Address from)) {
    typedef BlindFallbackHandler<ObjectType> MessageHandlerType;

    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    handlers_dirty_ = true;

    // Destroy any previously set new handler.
    // Note that all handler objects are the same size.
    if (new_handler_) {
        new_handler_->~FallbackHandlerInterface();
        allocator->FreeWithSize(new_handler_, sizeof(MessageHandlerType));
        new_handler_ = 0;
    }

    if (handler_object && handler) {
        // Allocate memory for a message handler object.
        void *const memory = allocator->Allocate(sizeof(MessageHandlerType));
        if (memory == 0) {
            return false;
        }

        // Construct a handler object to remember the function pointer and message value type.
        new_handler_ = new (memory) MessageHandlerType(handler_object, handler);
    }

    return true;
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_HANDLERS_FALLBACKHANDLERCOLLECTION_H
