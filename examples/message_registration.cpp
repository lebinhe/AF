#include <stdio.h>
#include <string>

#include <AF/AF.h>

AF_REGISTER_MESSAGE(std::string);

class Replier : public AF::Actor {
public:
    inline Replier(AF::Framework &framework) : AF::Actor(framework) {
        RegisterHandler(this, &Replier::StringHandler);
    }

private:
    inline void StringHandler(const std::string &message, const AF::Address from) {
        printf("Received message '%s'\n", message.c_str());
        Send(message, from);
    }
};


int main() {
    AF::Framework framework;
    AF::Receiver receiver;
    Replier replier(framework);

    if (!framework.Send(
        std::string("Hello"),
        receiver.GetAddress(),
        replier.GetAddress())) {
        printf("ERROR: Failed to send message to replier\n");
    }

    receiver.Wait();
}

