#ifndef AF_ALIGN_H
#define AF_ALIGN_H

#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

namespace AF 
{
namespace Detail 
{

/*
 * Traits struct template that stores alignment information about messages.
 */
template <class MessageType>
struct MessageAlignment {
     // The default alignment is four bytes.
    static const uint32_t ALIGNMENT = 4;
};

template <class Type>
AF_FORCEINLINE void AlignPointer(Type *& pointer, const uint32_t alignment) {
    AF_ASSERT_MSG((alignment & (alignment - 1)) == 0, "Alignment values must be powers of two");
    pointer = reinterpret_cast<Type *>((reinterpret_cast<uintptr_t>(pointer) + alignment - 1) & ~uintptr_t(alignment - 1));
}

template <class Type>
AF_FORCEINLINE Type RoundUp(Type &pointer, const uint32_t alignment) {
    AF_ASSERT_MSG((alignment & (alignment - 1)) == 0, "Alignment values must be powers of two");
    return static_cast<Type>((static_cast<uintptr_t>(pointer) + alignment - 1) & ~uintptr_t(alignment - 1));
}

AF_FORCEINLINE bool IsAligned(void *const pointer, const uint32_t alignment) {
    AF_ASSERT_MSG((alignment & (alignment - 1)) == 0, "Alignment values must be powers of two");
    return ((reinterpret_cast<uintptr_t>(pointer) & uintptr_t(alignment - 1)) == 0);
}

AF_FORCEINLINE bool IsAligned(const void *const pointer, const uint32_t alignment) {
    AF_ASSERT_MSG((alignment & (alignment - 1)) == 0, "Alignment values must be powers of two");
    return ((reinterpret_cast<uintptr_t>(pointer) & uintptr_t(alignment - 1)) == 0);
}


} // namespace Detail
} // namespace AF

/*
 * AF_ALIGN_MESSAGE 
 * Informs AF of the alignment requirements of a message type.
 *
 * namespace MyNamespace
 * {
 * class MyMessage {
 * };
 * }
 * AF_ALIGN_MESSAGE(MyNamespace::MyMessage, 16);
 */
#ifndef AF_ALIGN_MESSAGE
#define AF_ALIGN_MESSAGE(MessageType, alignment)                            \
namespace AF                                                                \
{                                                                           \
namespace Detail                                                            \
{                                                                           \
template <>                                                                 \
struct MessageAlignment<MessageType> {                                      \
    static const uint32_t ALIGNMENT = (alignment);                          \
};                                                                          \
}                                                                           \
}
#endif // AF_ALIGN_MESSAGE


/*
 * AF_PREALIGN 
 *
 * Informs the compiler of the stack alignment requirements of a type.
 * 
 * // A message type that requires alignment.
 * struct AF_PREALIGN(128) AlignedMessage {
 *  int mValue;
 * } AF_POSTALIGN(128);
 *
 * int main() {
 *  AlignedMessage message;  // Aligned to 128 bytes by the compiler
 * }
 */
#ifndef AF_PREALIGN
#define AF_PREALIGN(alignment) __attribute__ ((__aligned__ (alignment)))
#endif // AF_PREALIGN


/*
 * AF_POSTALIGN
 */
#ifndef AF_POSTALIGN
#define AF_POSTALIGN(alignment)
#endif // AF_POSTALIGN


/*
 * AF_ALIGNOF
 */
#ifndef AF_ALIGNOF
#define AF_ALIGNOF(type) alignof(type)
#endif // AF_ALIGNOF


/*
 * AF_ALIGN
 *
 * Aligns the given pointer to the given alignment, in bytes, increasing its value if necessary.
 */
#ifndef AF_ALIGN
#define AF_ALIGN(p, alignment) AF::Detail::AlignPointer(p, alignment)
#endif // AF_ALIGN


/*
 * AF_ALIGNED
 *
 * Checks the alignment of a pointer.
 */
#ifndef AF_ALIGNED
#define AF_ALIGNED(pointer, alignment) AF::Detail::IsAligned(pointer, alignment)
#endif // AF_ALIGNED


/*
 * AF_ROUNDUP 
 *
 * Rounds the given integer type up to the next multiple of the given unit size in bytes.
 */
#ifndef AF_ROUNDUP
#define AF_ROUNDUP(i, alignment) AF::Detail::RoundUp(i, alignment)
#endif // AF_ROUNDUP

#endif // THERON_ALIGN_H
