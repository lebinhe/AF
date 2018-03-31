#ifndef AF_DETAIL_DIRECTORY_ENTRY_H
#define AF_DETAIL_DIRECTORY_ENTRY_H

#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/threading/spin_lock.h"


namespace AF
{
namespace Detail
{

/*
 * An entry in the directory, potentially recording the registration of one entity.
 */
class Entry {
public:
    /* 
     * A non-copyable entity that can be registered in a directory.
     * In order to be registered in the directory, types must derive from this class.
     */
    class Entity {
    public:
        inline Entity() {
        }
        
    private:
        Entity(const Entity &other);
        Entity &operator=(const Entity &other);
    };

    inline Entry() 
      : spin_lock_(),
        entity_(0),
        pin_count_(0) {
    }

    inline void Lock() const;

    inline void Unlock() const;

    /* 
     * Deregisters any entity registered at this entry.
     */
    inline void Free();

    /*
     * Registers the given entity at this entry.
     */
    inline void SetEntity(Entity *const entity);

    inline Entity *GetEntity() const;

    /*
     * Pins the entry, preventing the registered entry from being changed.
     */
    inline void Pin();

    /*
     * Unpins the entry, allowed the registered entry to be changed.
     */
    inline void Unpin();

    inline bool IsPinned() const;

private:
    Entry(const Entry &other);
    Entry &operator=(const Entry &other);

    mutable SpinLock spin_lock_;                 // Thread synchronization object protecting the entry.
    Entity *entity_;                             // Pointer to the registered entity.
    uint32_t pin_count_;                         // Number of times this entity has been pinned and not unpinned.
};


AF_FORCEINLINE void Entry::Lock() const {
    spin_lock_.Lock();
}

AF_FORCEINLINE void Entry::Unlock() const {
    spin_lock_.Unlock();
}

AF_FORCEINLINE void Entry::Free() {
    AF_ASSERT(pin_count_ == 0);
    entity_ = 0;
}

AF_FORCEINLINE void Entry::SetEntity(Entity *const entity) {
    AF_ASSERT(pin_count_ == 0);
    entity_ = entity;
}

AF_FORCEINLINE Entry::Entity *Entry::GetEntity() const {
    return entity_;
}

AF_FORCEINLINE void Entry::Pin() {
    ++pin_count_;
}

AF_FORCEINLINE void Entry::Unpin() {
    AF_ASSERT(pin_count_ > 0);
    --pin_count_;
}

AF_FORCEINLINE bool Entry::IsPinned() const {
    return (pin_count_ > 0);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_DIRECTORY_ENTRY_H

