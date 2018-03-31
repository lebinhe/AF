#include <stdio.h>
#include <stdlib.h>

#include "AF/AF.h"
#include "timer.h"


class PingPong : public AF::Actor {
public:

    struct StartMessage {
        inline StartMessage(const AF::Address &caller, const AF::Address &partner) 
          : caller_(caller), partner_(partner) {
        }

        AF::Address caller_;
        AF::Address partner_;
    };

    inline PingPong(AF::Framework &framework) 
      : AF::Actor(framework) {
        RegisterHandler(this, &PingPong::Start);
    }

private:

    inline void Start(const StartMessage &message, const AF::Address /*from*/) {
        caller_ = message.caller_;
        partner_ = message.partner_;

        DeregisterHandler(this, &PingPong::Start);
        RegisterHandler(this, &PingPong::Receive);
    }

    inline void Receive(const int &message, const AF::Address /*from*/) {
        if (message > 0) {
            Send(message - 1, partner_);
        } else {
            Send(message, caller_);
        }
    }

    AF::Address caller_;
    AF::Address partner_;
};


AF_DECLARE_REGISTERED_MESSAGE(int);
AF_DECLARE_REGISTERED_MESSAGE(PingPong::StartMessage);

AF_DEFINE_REGISTERED_MESSAGE(int);
AF_DEFINE_REGISTERED_MESSAGE(PingPong::StartMessage);


int main(int argc, char *argv[]) {
    const int num_messages = (argc > 1 && atoi(argv[1]) > 0) ? atoi(argv[1]) : 5000000;
    const int num_threads = (argc > 2 && atoi(argv[2]) > 0) ? atoi(argv[2]) : 2;

    printf("Using num_messages = %d (use first command line argument to change)\n", num_messages);
    printf("Using num_threads = %d (use second command line argument to change)\n", num_threads);
    printf("Starting %d message sends between ping and pong...\n", num_messages);

    AF::Framework framework(num_threads);
    AF::Receiver receiver;

    PingPong ping(framework);
    PingPong pong(framework);

    // Start Ping and Pong, sending each the address of the other and the address of the receiver.
    const PingPong::StartMessage ping_start(receiver.GetAddress(), pong.GetAddress());
    framework.Send(ping_start, receiver.GetAddress(), ping.GetAddress());
    const PingPong::StartMessage pong_start(receiver.GetAddress(), ping.GetAddress());
    framework.Send(pong_start, receiver.GetAddress(), pong.GetAddress());

    Timer timer;
    timer.Start();

    // Send the initial integer count to Ping.
    framework.Send(num_messages, receiver.GetAddress(), ping.GetAddress());

    // Wait to hear back from either Ping or Pong when the count reaches zero.
    receiver.Wait();
    timer.Stop();

    // The number of full cycles is half the number of messages.
    printf("Completed %d message response cycles in %.1f seconds\n", num_messages / 2, timer.Seconds());
    printf("Average response time is %.10f seconds\n", timer.Seconds() / (num_messages / 2));

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    AF::AllocatorInterface *const allocator(AF::AllocatorManager::GetAllocator());
    const int allocation_count(static_cast<AF::DefaultAllocator *>(allocator)->GetAllocationCount());
    const int peakBytes_allocated(static_cast<AF::DefaultAllocator *>(allocator)->GetPeakBytesAllocated());
    printf("Total number of allocations: %d calls\n", allocation_count);
    printf("Peak memory usage in bytes: %d bytes\n", peakBytes_allocated);
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

}

