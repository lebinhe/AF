#ifndef AF_DETAIL_SCHEDULER_SCHEDULERHINTS_H
#define AF_DETAIL_SCHEDULER_SCHEDULERHINTS_H


#include "AF/basic_types.h"
#include "AF/defines.h"


namespace AF
{
namespace Detail
{

/*
 * Wraps up various bits of information available to scheduler queuing policies.
 */
class SchedulerHints {
public:
    AF_FORCEINLINE SchedulerHints() {
    }

    bool send_;                         // Indicates whether the mailbox is being scheduled due to being sent a message.
    uint32_t predicted_send_count_;     // Predicts the number of messages that will be sent by the current handler.
    uint32_t send_index_;               // Index of this message send within the current handler.
    uint32_t message_count_;            // Number of messages queued in the mailbox that is currently being processed.

private:
    SchedulerHints(const SchedulerHints &other);
    SchedulerHints &operator=(const SchedulerHints &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_SCHEDULER_SCHEDULERHINTS_H
