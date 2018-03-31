#include <new>

#include "AF/actor.h"
#include "AF/allocator_interface.h"
#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/defines.h"
#include "AF/framework.h"
#include "AF/receiver.h"

#include "AF/detail/directory/static_directory.h"

#include "AF/detail/scheduler/blocking_monitor.h"
#include "AF/detail/scheduler/mailbox_queue.h"
#include "AF/detail/scheduler/scheduler.h"

#include "AF/detail/strings/string.h"

#include "AF/detail/utils/utils.h"


namespace AF
{

void Framework::Initialize() {
    scheduler_ = CreateScheduler();

    // Set up the scheduler.
    scheduler_->Initialize(params_.thread_count_);

    // Set up the default fallback handler, which catches and reports undelivered messages.
    SetFallbackHandler(&default_fallback_handler_, &Detail::DefaultFallbackHandler::Handle);
     
    // Register the framework and get a non-zero index for it, unique within the local process.
    index_ = Detail::StaticDirectory<Framework>::Register(this);
    AF_ASSERT(index_);

    // If the framework name wasn't set explicitly then generate a default name.
    if (name_.IsNull()) {
        char buffer[16];
        Detail::NameGenerator::Generate(buffer, index_);
        name_ = Detail::String(buffer);
    }
}

void Framework::Release() {
    // Deregister the framework.
    Detail::StaticDirectory<Framework>::Deregister(index_);

    scheduler_->Release();
    DestroyScheduler(scheduler_);
    scheduler_ = 0;
}

Detail::SchedulerInterface *Framework::CreateScheduler() {
    typedef Detail::MailboxQueue<Detail::BlockingMonitor> BlockingQueue;

    typedef Detail::Scheduler<BlockingQueue> BlockingScheduler;

    AllocatorInterface *const allocator(AllocatorManager::GetCache());
    void *scheduler_memory(0);

    scheduler_memory = allocator->AllocateAligned(
        sizeof(BlockingScheduler),
        AF_CACHELINE_ALIGNMENT);

    AF_ASSERT_MSG(scheduler_memory, "Failed to allocate scheduler");

    return new (scheduler_memory) BlockingScheduler(
        &mailboxes_,
        &fallback_handlers_,
        &message_allocator_,
        &shared_mailbox_context_);
}

void Framework::DestroyScheduler(Detail::SchedulerInterface *const scheduler) {
    AllocatorInterface *const allocator(AllocatorManager::GetCache());

    scheduler->~SchedulerInterface();
    allocator->Free(scheduler);
}

void Framework::RegisterActor(Actor *const actor, const char *const name) {
    // Allocate an unused mailbox.
    const uint32_t mailbox_index(mailboxes_.Allocate());
    Detail::Mailbox &mailbox(mailboxes_.GetEntry(mailbox_index));

    // Use the provided name for the actor if one was provided.
    Detail::String mailbox_name(name);
    if (name == 0) {
        char raw_name[16];
        Detail::NameGenerator::Generate(raw_name, mailbox_index);

        char scoped_name[256];
        Detail::NameGenerator::Combine(
            scoped_name,
            256,
            raw_name,
            name_.GetValue());

        mailbox_name = Detail::String(scoped_name);
    }

    // Name the mailbox and register the actor.
    mailbox.Lock();
    mailbox.SetName(mailbox_name);
    mailbox.RegisterActor(actor);
    mailbox.Unlock();

    // Create the unique address of the mailbox.
    // Its a pair comprising the framework index and the mailbox index within the framework.
    const Detail::Index index(index_, mailbox_index);
    const Address mailbox_address(mailbox_name, index);

    // Set the actor's mailbox address.
    // The address contains the index of the framework and the index of the mailbox within the framework.
    actor->address_ = mailbox_address;
}

void Framework::DeregisterActor(Actor *const actor) {
    const Address address(actor->GetAddress());

    // Deregister the actor, so that the worker threads will leave it alone.
    const uint32_t mailbox_index(address.AsInteger());
    Detail::Mailbox &mailbox(mailboxes_.GetEntry(mailbox_index));

    // If the entry is pinned then we have to wait for it to be unpinned.
    bool deregistered(false);
    uint32_t backoff(0);

    while (!deregistered) {
        mailbox.Lock();

        if (!mailbox.IsPinned()) {
            mailbox.DeregisterActor();
            deregistered = true;
        }

        mailbox.Unlock();

        Detail::Utils::Backoff(backoff);
    }
}

bool Framework::DeliverWithinLocalProcess(Detail::MessageInterface *const message, const Detail::Index &index) {
    const uint32_t target_framework_index(index.componets_.framework_);

    AF_ASSERT(index.uint32_ != 0);

    // Is the message addressed to a receiver? Receiver addresses have zero framework indices.
    if (target_framework_index == 0) {
        // Get a reference to the receiver directory entry for this address.
        Detail::Entry &entry(Detail::StaticDirectory<Receiver>::GetEntry(index.componets_.index_));

        // Pin the entry and lookup the entity registered at the address.
        entry.Lock();
        entry.Pin();
        Receiver *const receiver(static_cast<Receiver *>(entry.GetEntity()));
        entry.Unlock();

        // If a receiver is registered at the mailbox then deliver the message to it.
        if (receiver) {
            receiver->Push(message);
        }

        // Unpin the entry, allowing it to be changed by other threads.
        entry.Lock();
        entry.Unpin();
        entry.Unlock();

        return (receiver != 0);
    }

    bool delivered(false);

    // TODO: Return a pointer so we can handle missing pages gracefully.
    // Get the entry for the addressed framework.
    Detail::Entry &entry(Detail::StaticDirectory<Framework>::GetEntry(index.componets_.framework_));

    // Pin the entry and lookup the framework registered at the index.
    entry.Lock();
    entry.Pin();
    Framework *const framework(static_cast<Framework *>(entry.GetEntity()));
    entry.Unlock();

    // If a framework is registered at this index then forward the message to it.
    if (framework) {
        // The address is just an index with no name.
        const Address address(Detail::String(), index);
        delivered = framework->FrameworkReceive(message, address);
    }

    // Unpin the entry, allowing it to be changed by other threads.
    entry.Lock();
    entry.Unpin();
    entry.Unlock();

    return delivered;
}


} // namespace AF

