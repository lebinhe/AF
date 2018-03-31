#include <stdio.h>
#include <string>

#include "AF/AF.h"

#include "AF/detail/threading/spin_lock.h"

// A simple linear allocator implementing AF::AllocatorInterface.
// It allocates from a memory buffer, never freeing, until it runs out.
class LinearAllocator : public AF::AllocatorInterface {
public:
    LinearAllocator(void *const buffer, const SizeType size) 
      : spin_lock_(),
        buffer_(static_cast<unsigned char *>(buffer)),
        offset_(buffer_),
        end_(buffer_ + size) {
    }

    virtual ~LinearAllocator() {
    }

    virtual void *Allocate(const SizeType size) {
        // Default 4-byte alignment.
        return AllocateAligned(size, 4);
    }

    virtual void *AllocateAligned(const SizeType size, const SizeType alignment) {
        unsigned char *allocation(0);

        spin_lock_.Lock();

        allocation = offset_;
        AF_ALIGN(allocation, alignment);

        // Buffer used up yet?
        if (allocation + size <= end_) {
            offset_ = allocation + size;
        } else {
            allocation = 0;
        }

        spin_lock_.Unlock();

        return static_cast<void *>(allocation);
    }

    virtual void Free(void *const /*memory*/) {
    }

    virtual void Free(void *const /*memory*/, const SizeType /*size*/) {
    }

    SizeType GetBytesAllocated() {
        return static_cast<SizeType>(offset_ - buffer_);
    }    

private:
    LinearAllocator(const LinearAllocator &other);
    LinearAllocator &operator=(const LinearAllocator &other);

    AF::Detail::SpinLock spin_lock_;        // Used to ensure thread-safe access.
    unsigned char *buffer_;                 // Base address of referenced memory buffer.
    unsigned char *offset_;                 // Current place within referenced memory buffer.
    unsigned char *end_;                    // End of referenced memory buffer (exclusive).
};

// A simple actor that sends back string messages it receives.
class Replier : public AF::Actor {
public:
    Replier(AF::Framework &framework) : AF::Actor(framework) {
        RegisterHandler(this, &Replier::StringHandler);
    }

private:
    void StringHandler(const std::string &message, const AF::Address from) {
        Send(message, from);
    }
};


int main() {
    const unsigned int BUFFER_SIZE(1024 * 1024);
    unsigned char *const buffer = new unsigned char[BUFFER_SIZE];
    LinearAllocator allocator(buffer, BUFFER_SIZE);

    AF::AllocatorManager::SetAllocator(&allocator);

    {
        AF::Framework framework;
        AF::Receiver receiver;
        Replier replier(framework);

        if (!framework.Send(std::string("base"), receiver.GetAddress(), replier.GetAddress())) {
            printf("ERROR: Failed to send message to replier\n");
        }

        receiver.Wait();
    }

    printf("Allocated %d bytes\n", static_cast<int>(allocator.GetBytesAllocated()));

    delete [] buffer;
}

