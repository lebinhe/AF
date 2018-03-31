#ifndef AF_ALLOCATOR_H
#define AF_ALLOCATOR_H

#include "AF/basic_types.h"


namespace AF
{

/*
 * Interface describing a general-purpose memory allocator.
 */
class AllocatorInterface {
public:
    /*
     * Defines an integer type used for specifying sizes of memory allocations.
     */
    typedef uint32_t SizeType;

    inline AllocatorInterface() {
    }
    
    inline virtual ~AllocatorInterface() {
    }

    /*
     * Allocates a piece of contiguous memory.
     *
     * size: The size of the memory to allocate, in bytes.
     * return A pointer to the allocated memory.
     */
    virtual void *Allocate(const SizeType size) = 0;

    /*
     * Allocates a piece of contiguous memory aligned to a given byte-multiple boundary.
     *
     * size: The size of the memory to allocate, in bytes.
     * alignment: The alignment of the memory to allocate, in bytes.
     * return A pointer to the allocated memory.
     */
    inline virtual void *AllocateAligned(const SizeType size, const SizeType alignment) {
        return Allocate(size);
    }

    /*
     * Frees a previously allocated piece of contiguous memory.
     *
     * memory:  A pointer to the memory to be deallocated.
     */
    virtual void Free(void *const memory) = 0;

    /*
     * Frees a previously allocated block of contiguous memory of a known size.
     * Knowing the size of the freed block allows some implementations to cache and reuse freed blocks.
     *
     * memory: A pointer to the block of memory to be deallocated.
     * size: The size of the freed block.
     */
    inline virtual void FreeWithSize(void *const memory, const SizeType /* size */) {
        Free(memory);
    }

private:
    AllocatorInterface(const AllocatorInterface &other);
    AllocatorInterface &operator=(const AllocatorInterface &other);
};


} // namespace AF


#endif // AF_ALLOCATOR_H
