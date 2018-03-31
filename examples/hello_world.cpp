#include <stdio.h>
#include <string>

#include "AF/AF.h"

class Printer : public AF::Actor {
public:
    Printer(AF::Framework &framework) : AF::Actor(framework) {
        RegisterHandler(this, &Printer::Print);
    }

private:
    void Print(const std::string &message, const AF::Address from) {
        printf("%s\n", message.c_str());

        Send(0, from);
    }
};

int main()
{
    AF::Framework framework;
    Printer printer(framework);

    AF::Receiver receiver;

    if (!framework.Send(
        std::string("Hello world!"),
        receiver.GetAddress(),
        printer.GetAddress())) {
        printf("ERROR: Failed to send message\n");
    }

    receiver.Wait();
}

