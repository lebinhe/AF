#ifndef AF_DETAIL_THREAD_H
#define AF_DETAIL_THREAD_H

#include <new>

#include "AF/allocator_manager.h"
#include "AF/assert.h"
#include "AF/defines.h"

#include <thread>


namespace AF
{
namespace Detail
{

class Thread {
public:
    typedef void (*EntryPoint)(void *const context);

    inline Thread() : thread_(0) {
    }

    inline ~Thread() {
        AF_ASSERT(thread_ == 0);
    }

    inline bool Start(EntryPoint entry_point, void *const context) {
        AF_ASSERT(thread_ == 0);

        // Allocate memory for a std::thread object. They're not copyable.
        void *const memory = AllocatorManager::GetCache()->AllocateAligned(sizeof(std::thread), AF_CACHELINE_ALIGNMENT);
        if (memory == 0) {
            return false;
        }

        // Construct a std::thread in the allocated memory
        // Pass it a callable object that in turn calls the entry point, passing it some context.
        ThreadStarter starter(entry_point, context);
        thread_ = new (memory) std::thread(starter);

        if (thread_ == 0) {
            return false;
        }

        return true;
    }

    inline void Join() {
        AF_ASSERT(thread_);

        // This waits for the thread function to return.
        thread_->join();

        AllocatorManager::GetCache()->FreeWithSize(thread_, sizeof(std::thread)); 
        thread_ = 0;
    }

    AF_FORCEINLINE bool Running() const {
        return (thread_ != 0);
    }

private:
    class ThreadStarter {
    public:
        inline ThreadStarter(EntryPoint entry_point, void *const context) 
          : entry_point_(entry_point),
            context_(context) {
        }

        inline void operator()() {
            entry_point_(context_);
        }

    private:
        EntryPoint entry_point_;
        void *context_;
    };

    Thread(const Thread &other);
    Thread &operator=(const Thread &other);

    std::thread *thread_;     // Pointer to the owned std::thread.
};


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_THREADING_THREAD_H
