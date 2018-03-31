#include <stdio.h>

#include "AF/AF.h"

#define CACHE_ALIGNMENT 64

struct AF_PREALIGN(CACHE_ALIGNMENT) AlignedMessage {
    AlignedMessage(const int value) : value_(value) { }
    int value_;

} AF_POSTALIGN(CACHE_ALIGNMENT);


AF_ALIGN_MESSAGE(AlignedMessage, CACHE_ALIGNMENT);


class AF_PREALIGN(CACHE_ALIGNMENT) AlignedActor : public AF::Actor {
public:
    inline AlignedActor(AF::Framework &framework) : AF::Actor(framework) {
        RegisterHandler(this, &AlignedActor::Handler);
    }

private:
    inline void Handler(const AlignedMessage &message, const AF::Address from) {
        printf("Received message aligned to %d bytes at address 0x%p\n",
                CACHE_ALIGNMENT,
                &message);

        if (!AF_ALIGNED(&message, CACHE_ALIGNMENT)) {
            printf("ERROR: Received message isn't correctly aligned\n");
        }

        Send(message, from);
    }

} AF_POSTALIGN(CACHE_ALIGNMENT);


int main() {
    AF::Framework framework;
    AF::Receiver receiver;

    AlignedActor aligned_actor(framework);

    printf("Constructed actor aligned to %d bytes at address 0x%p\n",
            CACHE_ALIGNMENT,
            &aligned_actor);

    if (!AF_ALIGNED(&aligned_actor, CACHE_ALIGNMENT)) {
        printf("ERROR: Constructed actor isn't correctly aligned\n");
    }

    AlignedMessage aligned_message(503);

    printf("Constructed message aligned to %d bytes at address 0x%p\n",
            CACHE_ALIGNMENT,
            &aligned_message);

    if (!AF_ALIGNED(&aligned_message, CACHE_ALIGNMENT)) {
        printf("ERROR: Constructed message isn't correctly aligned\n");
    }

    if (!framework.Send(
        aligned_message,
        receiver.GetAddress(),
        aligned_actor.GetAddress())) {
        printf("ERROR: Failed to send message to address printer\n");
    }

    // Wait for the reply to be sure the message was handled.
    receiver.Wait();
}

