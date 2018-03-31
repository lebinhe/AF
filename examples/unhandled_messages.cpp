#include <stdio.h>
#include <string>

#include <AF/AF.h>

static void DumpMessage(
    const void *const data,
    const AF::uint32_t size,
    const AF::Address from) {
    printf("Unhandled %d byte message sent from address %d:\n",
            size,
            from.AsInteger());

    if (data) {
        const char *const format("[%d] 0x%08x\n");

        const unsigned int *const begin(reinterpret_cast<const unsigned int *>(data));
        const unsigned int *const end(begin + size / sizeof(unsigned int));

        for (const unsigned int *word(begin); word != end; ++word) {
            printf(format, static_cast<int>(word - begin), *word);
        }
    }
}

class Printer : public AF::Actor {
public:
    Printer(AF::Framework &framework) : AF::Actor(framework) {
        RegisterHandler(this, &Printer::Print);

        SetDefaultHandler(this, &Printer::DefaultHandler);
    }

private:
    void Print(const std::string &message, const AF::Address from) {
        printf("%s\n", message.c_str());

        Send(message, from);
    }

    void DefaultHandler(
        const void *const data,
        const AF::uint32_t size,
        const AF::Address from) {
        DumpMessage(data, size, from);
    }
};


class FallbackHandler {
public:

    void Handle(
        const void *const data,
        const AF::uint32_t size,
        const AF::Address from) {
        DumpMessage(data, size, from);
    }
};


int main()
{
    AF::Receiver receiver;
    AF::Address printer_address;

    AF::Framework framework;
    FallbackHandler fallback_handler;
    framework.SetFallbackHandler(&fallback_handler, &FallbackHandler::Handle);

    {
        Printer printer(framework);
        printer_address = printer.GetAddress();

        framework.Send(103, receiver.GetAddress(), printer_address);

        framework.Send(std::string("hit"), receiver.GetAddress(), printer_address);

        receiver.Wait();
    }

    framework.Send(std::string("miss"), receiver.GetAddress(), printer_address);
}

