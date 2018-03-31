#ifndef AF_DEFINES_H
#define AF_DEFINES_H


#if !defined(AF_DEBUG)
#if defined(NDEBUG)
#define AF_DEBUG 0
#else
#define AF_DEBUG 1
#endif
#else
#define AF_DEBUG 0
#endif // AF_DEBUG


#if !defined(AF_ENABLE_ASSERTS)
#if AF_DEBUG
#define AF_ENABLE_ASSERTS 1
#else
#define AF_ENABLE_ASSERTS 0
#endif
#endif // AF_ENABLE_ASSERTS


/*
 * AF_FORCEINLINE
 *
 * Controls force-inlining of core functions within AF.
 */
#if !defined(AF_FORCEINLINE)
#if AF_DEBUG
#define AF_FORCEINLINE inline
#else // AF_DEBUG
#define AF_FORCEINLINE inline __attribute__((always_inline))
#endif // AF_DEBUG
#endif // AF_FORCEINLINE


/*
 * AF_NOINLINE
 */
#if !defined(AF_NOINLINE)
#define AF_NOINLINE __attribute__((noinline))
#endif // AF_NOINLINE


/*
 * AF_ENABLE_DEFAULTALLOCATOR_CHECKS
 *
 * Enables debug checking of allocations in the AF::DefaultAllocator
 */
#if !defined(AF_ENABLE_DEFAULTALLOCATOR_CHECKS)
#if AF_DEBUG
#define AF_ENABLE_DEFAULTALLOCATOR_CHECKS 1
#else
#define AF_ENABLE_DEFAULTALLOCATOR_CHECKS 0
#endif // AF_DEBUG
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS


/*
 * AF_ENABLE_MESSAGE_REGISTRATION_CHECKS
 *
 * Controls run-time reporting of unregistered message types.
 */
#if !defined(AF_ENABLE_MESSAGE_REGISTRATION_CHECKS)
#define AF_ENABLE_MESSAGE_REGISTRATION_CHECKS 0
#endif // AF_ENABLE_MESSAGE_REGISTRATION_CHECKS


/*
 * AF_ENABLE_UNHANDLED_MESSAGE_CHECKS
 *
 * Defaults to 1 (enabled). Set this to 0 to disable the reporting.
 */
#if !defined(AF_ENABLE_UNHANDLED_MESSAGE_CHECKS)
#define AF_ENABLE_UNHANDLED_MESSAGE_CHECKS 1
#endif


/*
 * AF_ENABLE_BUILD_CHECKS
 *
 * Enables automatic checking of build consistency between the AF library and client code.
 */
#if !defined(AF_ENABLE_BUILD_CHECKS)
#define AF_ENABLE_BUILD_CHECKS 1
#endif


/*
 * AF_ENABLE_COUNTERS
 *
 * Controls availability of per-framework counters that record the occurrence of scheduling events.
 *
 * AF::Framework::GetNumCounters
 * AF::Framework::GetCounterValue
 */
#if !defined(AF_ENABLE_COUNTERS)
#define AF_ENABLE_COUNTERS 0
#endif


/*
 * AF_CACHELINE_ALIGNMENT
 *
 * Describes the size of a cache line on the target platform.
 */
#if !defined(AF_CACHELINE_ALIGNMENT)
#define AF_CACHELINE_ALIGNMENT 64
#endif


#endif // AF_DEFINES_H

