#include "AF/defines.h"
#include "AF/receiver.h"

#include "AF/detail/directory/static_directory.h"

#include "AF/detail/strings/string.h"

#include "AF/detail/utils/utils.h"


namespace AF
{


Receiver::Receiver() 
  : string_pool_ref_(),
    name_(),
    address_(),
    message_handlers_(),
    condition_(),
    messages_received_(0) {
    Initialize();
}

Receiver::~Receiver() {
    Release();
}

void Receiver::Initialize() {
    // Register this receiver, claiming a unique address for this receiver.
    const uint32_t receiver_index(Detail::StaticDirectory<Receiver>::Register(this));

    if (name_.IsNull()) {
        char raw_name[16];
        Detail::NameGenerator::Generate(raw_name, receiver_index);

        char scoped_name[256];
        Detail::NameGenerator::Combine(
            scoped_name,
            256,
            raw_name,
            0);

        name_ = Detail::String(scoped_name);
    }

    // Receivers are identified as a receiver by a framework index of zero.
    // All frameworks have non-zero indices, so all actors have non-zero framework indices.
    const Detail::Index index(0, receiver_index);
    address_ = Address(name_, index);

    // Register the receiver at its claimed address.
    Detail::Entry &entry(Detail::StaticDirectory<Receiver>::GetEntry(address_.AsInteger()));

    entry.Lock();
    entry.SetEntity(this);
    entry.Unlock();
}

void Receiver::Release() {
    const Address &address(GetAddress());

    // Deregister the receiver, so that the worker threads will leave it alone.
    Detail::StaticDirectory<Receiver>::Deregister(address.AsInteger());

    condition_.GetMutex().Lock();

    // Free all currently allocated handler objects.
    while (Detail::ReceiverHandlerInterface *const handler = message_handlers_.Front()) {
        message_handlers_.Remove(handler);
        AllocatorManager::GetCache()->Free(handler);
    }

    condition_.GetMutex().Unlock();
}


} // namespace AF


