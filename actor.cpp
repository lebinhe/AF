#include "AF/actor.h"
#include "AF/framework.h"


namespace AF
{

Actor::Actor(Framework &framework, const char *const name) 
  : address_(),
    framework_(&framework),
    message_handlers_(),
    default_handlers_(),
    mailbox_context_(0),
    memory_(0) {
    // Claim an available directory index and mailbox for this actor.
    framework_->RegisterActor(this, name);
}

Actor::~Actor() {
    framework_->DeregisterActor(this);
}

void Actor::Fallback(
    Detail::FallbackHandlerCollection *const fallback_handlers,
    const Detail::MessageInterface *const message) {
    // If default handlers are registered with this actor, execute those.
    if (default_handlers_.Handle(this, message)) {
        return;
    }

    // Let the framework's fallback handlers handle the message.
    fallback_handlers->Handle(message);
}


} // namespace AF
