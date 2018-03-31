#ifndef AF_RECEIVER_H
#define AF_RECEIVER_H

#include "AF/address.h"
#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"

#include "AF/detail/directory/entry.h"

#include "AF/detail/handlers/receiver_handler.h"
#include "AF/detail/handlers/receiver_handler_interface.h"
#include "AF/detail/handlers/receiver_handler_cast.h"

#include "AF/detail/messages/message_interface.h"
#include "AF/detail/messages/message_creator.h"
#include "AF/detail/messages/message_traits.h"

#include "AF/detail/strings/string.h"
#include "AF/detail/strings/string_pool.h"

#include "AF/detail/threading/atomic.h"
#include "AF/detail/threading/condition.h"
#include "AF/detail/threading/lock.h"
#include "AF/detail/threading/mutex.h"

#include <new>


namespace AF
{


class Framework;


/*
 * A standalone entity that can accept messages sent by Actor "actors".
 */
class Receiver : public Detail::Entry::Entity {
public:

    friend class Framework;

    Receiver();

    ~Receiver();

    inline Address GetAddress() const;

    template <class ClassType, class ValueType>
    inline bool RegisterHandler(
        ClassType *const owner,
        void (ClassType::*handler)(const ValueType &message, const Address from));

    template <class ClassType, class ValueType>
    inline bool DeregisterHandler(
        ClassType *const owner,
        void (ClassType::*handler)(const ValueType &message, const Address from));

    inline void Reset();

    inline uint32_t Count() const;

    inline uint32_t Wait(const uint32_t max = 1);

    inline uint32_t Consume(const uint32_t max);

private:
    typedef Detail::List<Detail::ReceiverHandlerInterface> MessageHandlerList;

    Receiver(const Receiver &other);
    Receiver &operator=(const Receiver &other);

    void Initialize();

    void Release();

    inline void Push(Detail::MessageInterface *const message);

    Detail::StringPool::Ref string_pool_ref_;           // Ensures that the StringPool is created.
    Detail::String name_;                               // Name of the receiver.
    Address address_;                                   // Unique address of this receiver.
    MessageHandlerList message_handlers_;               // List of registered message handlers.
    mutable Detail::Condition condition_;               // Signals waiting threads when messages arrive.
    mutable Detail::Atomic::UInt32 messages_received_;  // Counts arrived messages not yet waited on.
};


// TODO: Force-inline
inline Address Receiver::GetAddress() const {
    return address_;
}

template <class ClassType, class ValueType>
inline bool Receiver::RegisterHandler(
    ClassType *const owner,
    void (ClassType::*handler)(const ValueType &message, const Address from)) {

    // If the message value type has a valid (non-zero) type name defined for it,
    // then we use explicit type names to match messages to handlers.
    // The default value of zero will indicates that no type name has been defined,
    // in which case we rely on automatically stored RTTI to identify message types.
    typedef Detail::ReceiverHandler<ClassType, ValueType> MessageHandlerType;

    // Allocate memory for a message handler object.
    void *const memory = AllocatorManager::GetCache()->Allocate(sizeof(MessageHandlerType));
    if (memory == 0) {
        return false;
    }

    // Construct a handler object to remember the function pointer and message value type.
    MessageHandlerType *const message_handler = new (memory) MessageHandlerType(owner, handler);
    if (message_handler == 0) {
        return false;
    }

    condition_.GetMutex().Lock();
    message_handlers_.Insert(message_handler);
    condition_.GetMutex().Unlock();
    
    return true;
}

template <class ClassType, class ValueType>
inline bool Receiver::DeregisterHandler(
    ClassType *const /*owner*/,
    void (ClassType::*handler)(const ValueType &message, const Address from)) {
    // If the message value type has a valid (non-zero) type name defined for it,
    // then we use explicit type IDs to match messages to handlers.
    // The default value of zero will indicate that no type name has been defined,
    // in which case we rely on automatically stored RTTI to identify message types.
    typedef Detail::ReceiverHandler<ClassType, ValueType> MessageHandlerType;
    typedef Detail::ReceiverHandlerCast<ClassType, Detail::MessageTraits<ValueType>::HAS_TYPE_NAME> HandlerCaster;

    condition_.GetMutex().Lock();

    // Find the handler in the registered handler list.
    typename MessageHandlerList::Iterator handlers(message_handlers_.GetIterator());
    while (handlers.Next()) {
        Detail::ReceiverHandlerInterface *const message_handler(handlers.Get());

        // Try to convert this handler, of unknown type, to the target type.
        const MessageHandlerType *const typed_handler = HandlerCaster:: template CastHandler<ValueType>(message_handler);
        if (typed_handler) {
            if (typed_handler->GetHandlerFunction() == handler) {
                // Remove the handler from the list.
                message_handlers_.Remove(message_handler);

                // Free the handler object, which was allocated on registration.
                AllocatorManager::GetCache()->FreeWithSize(message_handler, sizeof(MessageHandlerType));

                break;
            }
        }
    }

    condition_.GetMutex().Unlock();

    return false;
}

AF_FORCEINLINE void Receiver::Reset() {
    Detail::Lock lock(condition_.GetMutex());
    messages_received_.Store(0);
}

AF_FORCEINLINE uint32_t Receiver::Count() const {
    Detail::Lock lock(condition_.GetMutex());
    return static_cast<uint32_t>(messages_received_.Load());
}

AF_FORCEINLINE uint32_t Receiver::Wait(const uint32_t max) {
    AF_ASSERT(max > 0);

    Detail::Lock lock(condition_.GetMutex());

    // Wait for at least one message to arrive.
    // If messages were received since the last wait (or creation),
    // then we regard those messages as qualifying and early-exit.
    while (messages_received_.Load() == 0) {
        // Wait to be woken by an arriving message.
        // This blocks until a message arrives!
        condition_.Wait(lock);
    }

    uint32_t num_consumed(0);
    while (messages_received_.Load() > 0 && num_consumed < max) {
        messages_received_.Decrement();
        ++num_consumed;
    }

    return num_consumed;
}

AF_FORCEINLINE uint32_t Receiver::Consume(const uint32_t max) {
    // Consume up to the maximum number of arrived messages.
    AF_ASSERT(max > 0);

    Detail::Lock lock(condition_.GetMutex());

    uint32_t num_consumed(0);
    while (messages_received_.Load() > 0 && num_consumed < max) {
        messages_received_.Decrement();
        ++num_consumed;
    }

    return num_consumed;
}

AF_FORCEINLINE void Receiver::Push(Detail::MessageInterface *const message) {
    AF_ASSERT(message);

    condition_.GetMutex().Lock();

    // TODO: Use ReceiverHandlerCollection for thread-safety.
    MessageHandlerList::Iterator handlers(message_handlers_.GetIterator());
    while (handlers.Next()) {
        // Execute the handler.
        // It does nothing if it can't handle the message type.
        Detail::ReceiverHandlerInterface *const handler(handlers.Get());
        handler->Handle(message);
    }

    messages_received_.Increment();

    condition_.GetMutex().Unlock();
    condition_.PulseAll();

    // Destroy the message.
    // We use the global allocator to allocate messages sent to receivers.
    AllocatorInterface *const message_allocator(AllocatorManager::GetCache());
    Detail::MessageCreator::Destroy(message_allocator, message);
}


} // namespace AF


#endif // AF_RECEIVER_H
