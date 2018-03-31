#ifndef AF_DETAIL_SCHEDULER_SCHEDULERINTERFACE_H
#define AF_DETAIL_SCHEDULER_SCHEDULERINTERFACE_H


#include "AF/basic_types.h"
#include "AF/defines.h"
#include "AF/allocator_interface.h"

#include "AF/detail/directory/directory.h"

#include "AF/detail/handlers/message_handler_interface.h"

#include "AF/detail/mailboxes/mailbox.h"


namespace AF
{
namespace Detail
{


class FallbackHandlerCollection;
class MailboxContext;


/*
 * Mailbox scheduler interface.
 * 
 * The Scheduler class itself is templated to allow the use of different queue implementations.
 * This interface allows instantiations of the template against different queue types to be
 * referenced polymorphically.
 */
class SchedulerInterface {
public:
    SchedulerInterface() {
    }

    virtual ~SchedulerInterface() {
    }

    /*
     * Initializes a scheduler at start of day.
     */
    virtual void Initialize(const uint32_t thread_count) = 0;        

    /*
     * Tears down the scheduler prior to destruction.
     */
    virtual void Release() = 0;

    /*
     * Notifies the scheduler that a worker thread is about to start executing a message handler.
     */
    virtual void BeginHandler(MailboxContext *const mailbox_context, MessageHandlerInterface *const message_handler) = 0;

    /*
     * Notifies the scheduler that a worker thread has finished executing a message handler.
     */
    virtual void EndHandler(MailboxContext *const mailbox_context, MessageHandlerInterface *const message_handler) = 0;

    /*
     * Schedules for processing a mailbox that has received a message.
     */
    virtual void Schedule(MailboxContext *const mailbox_context, Mailbox *const mailbox) = 0;

    /*
     * Sets a maximum limit on the number of worker threads enabled in the scheduler.
     */
    virtual void SetMaxThreads(const uint32_t count) = 0;

    /*
     * Sets a minimum limit on the number of worker threads enabled in the scheduler.
     */
    virtual void SetMinThreads(const uint32_t count) = 0;

    /*
     * Gets the current maximum limit on the number of worker threads enabled in the scheduler.
     */
    virtual uint32_t GetMaxThreads() const = 0;

    /*
     * Gets the current minimum limit on the number of worker threads enabled in the scheduler.
     */
    virtual uint32_t GetMinThreads() const = 0;

    /*
     * Gets the current number of worker threads enabled in the scheduler.
     */
    virtual uint32_t GetNumThreads() const = 0;

    /*
     * Gets the highest number of worker threads that was ever enabled at one time in the scheduler.
     */
    virtual uint32_t GetPeakThreads() const = 0;

    /*
     * Resets all the scheduler's internal event counters to zero.
     */
    virtual void ResetCounters() = 0;

    /*
     * Gets the current value of a specified event counter, accumulated for all worker threads).
     */
    virtual uint32_t GetCounterValue(const uint32_t counter) const = 0;

    /*
     * Gets the current value of a specified event counter, for each worker thread individually.
     */
    virtual uint32_t GetPerThreadCounterValues(
        const uint32_t counter,
        uint32_t *const per_thread_counts,
        const uint32_t max_counts) const = 0;

private:
    SchedulerInterface(const SchedulerInterface &other);
    SchedulerInterface &operator=(const SchedulerInterface &other);
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_SCHEDULER_SCHEDULERINTERFACE_H
