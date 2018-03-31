#ifndef AF_REGISTER_H
#define AF_REGISTER_H


#include "AF/detail/messages/message_traits.h"


/*
 * Optional type registration for message types.
 */
#ifndef AF_DECLARE_REGISTERED_MESSAGE

#define AF_DECLARE_REGISTERED_MESSAGE(MessageType)                      \
namespace AF                                                            \
{                                                                       \
namespace Detail                                                        \
{                                                                       \
template <>                                                             \
struct MessageTraits<MessageType> {                                     \
    static const bool HAS_TYPE_NAME = true;                             \
    static const char *const TYPE_NAME;                                 \
};                                                                      \
}                                                                       \
}

#endif // AF_DECLARE_REGISTERED_MESSAGE


#ifndef AF_DEFINE_REGISTERED_MESSAGE

#define AF_DEFINE_REGISTERED_MESSAGE(MessageType)                       \
namespace AF                                                            \
{                                                                       \
namespace Detail                                                        \
{                                                                       \
const char *const MessageTraits<MessageType>::TYPE_NAME = #MessageType; \
}                                                                       \
}

#endif // AF_DEFINE_REGISTERED_MESSAGE


#ifndef AF_REGISTER_MESSAGE

#define AF_REGISTER_MESSAGE(MessageType)                                \
namespace AF                                                            \
{                                                                       \
namespace Detail                                                        \
{                                                                       \
template <>                                                             \
struct MessageTraits<MessageType> {                                     \
    static const bool HAS_TYPE_NAME = true;                             \
    static const char *const TYPE_NAME;                                 \
};                                                                      \
                                                                        \
const char *const MessageTraits<MessageType>::TYPE_NAME = #MessageType; \
}                                                                       \
}

#endif // AF_REGISTER_MESSAGE


#endif // AF_REGISTER_H
