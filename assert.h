#ifndef AF_DETAIL_DEBUG_ASSERT_H
#define AF_DETAIL_DEBUG_ASSERT_H

/*
 * Defines assert and fail macro's for debugging and error reporting.
 */

#include "AF/defines.h"


#if AF_ENABLE_ASSERTS

#include <stdio.h>
#include <assert.h>

#ifndef AF_ASSERT
#define AF_ASSERT(condition)                if (!(condition)) AF::Detail::Fail(__FILE__, __LINE__); else { }
#endif // AF_ASSERT

#ifndef AF_ASSERT_MSG
#define AF_ASSERT_MSG(condition, msg)       if (!(condition)) AF::Detail::Fail(__FILE__, __LINE__, msg); else { }
#endif // AF_ASSERT_MSG

#ifndef AF_FAIL
#define AF_FAIL()                           AF::Detail::Fail(__FILE__, __LINE__)
#endif // AF_FAIL

#ifndef AF_FAIL_MSG
#define AF_FAIL_MSG(msg)                    AF::Detail::Fail(__FILE__, __LINE__, msg)
#endif // AF_ASSERT

#else

#ifndef AF_ASSERT
#define AF_ASSERT(condition)
#endif // AF_ASSERT

#ifndef AF_ASSERT_MSG
#define AF_ASSERT_MSG(condition, msg)
#endif // AF_ASSERT_MSG

#ifndef AF_FAIL
#define AF_FAIL()
#endif // AF_FAIL

#ifndef AF_FAIL_MSG
#define AF_FAIL_MSG(msg)
#endif // AF_FAIL_MSG

#endif // AF_ENABLE_ASSERTS



namespace AF
{
namespace Detail
{

#if AF_ENABLE_ASSERTS
inline void Fail(const char *const file, const unsigned int line, const char *const message = 0) {
    fprintf(stderr, "FAIL in %s (%d)", file, line);
    if (message) {
        fprintf(stderr, ": %s", message);
    }
    fprintf(stderr, "\n");
    assert(false);
}
#endif // AF_ENABLE_ASSERTS


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_DEBUG_ASSERT_H

