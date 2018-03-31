#ifndef AF_DETAIL_MESSAGES_MESSAGETRAITS_H
#define AF_DETAIL_MESSAGES_MESSAGETRAITS_H


namespace AF
{
namespace Detail
{

/* 
 * Traits template that stores meta-information about message types.
 * 
 * The MessageTraits template can be specialized for individual message types
 * in order to label the types with their string names.
 *
 * Types with null names are matched by means of dynamic_cast, which
 * relies on RTTI (Runtime Type Information) 
 */
template <class ValueType>
struct MessageTraits {
    // Indicates whether the message type has an explicit name.
    // Message types for which have valid type names are identified
    // using their names rather than with built-in C++ Runtime Type Information
    // (RTTI) via dynamic_cast.
    static const bool HAS_TYPE_NAME = false;
    
    // The unique name of the type.
    static const char *const TYPE_NAME;
};


template <class ValueType>
const char *const MessageTraits<ValueType>::TYPE_NAME = 0;


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_MESSAGES_MESSAGETRAITS_H
