#ifndef AF_DEFAULTALLOCATOR_H
#define AF_DEFAULTALLOCATOR_H

#include "AF/align.h"
#include "AF/allocator_interface.h"
#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/threading/spin_lock.h"


namespace AF
{

/*
 * A simple general purpose memory allocator used by default.
 *
 * It extends the native C++ new and delete with support for aligned allocations,
 * allocation counting, and guardband checking.
 */
class DefaultAllocator : public AllocatorInterface {
public:
    static const uint32_t GUARD_VALUE = 0xdddddddd;

    inline DefaultAllocator();

    inline virtual ~DefaultAllocator();

    /*
     * Allocates a block of contiguous memory.
     *
     * size: The size of the memory block to allocate, in bytes, which must be a non-zero multiple of four bytes.
     * return A pointer to the allocated memory.
     */
    inline virtual void *Allocate(const SizeType size);

    /*
     * Allocates a block of contiguous memory aligned to a given byte-multiple boundary.
     *
     * The internal layout of each allocated block is illustrated below:
     * 
     * |-----------|--------|--------|--------|---------------------------------|--------|---------|
     * |  padding  | offset |  size  | guard  |          caller block           | guard  |  waste  |
     * |-----------|--------|--------|--------|---------------------------------|--------|---------|
     *
     * When AF_ENABLE_DEFAULTALLOCATOR_CHECKS is zero, some checking fields are omitted,
     * and allocated blocks have the following structure:
     *
     * |-----------|--------|----------------------------------|---------|
     * |  padding  | offset |           caller block           |  waste  |
     * |-----------|--------|----------------------------------|---------|
     * 
     * Where:
     * - The "caller block" starts at an aligned address.
     * - "padding" is a padding field of variable size used to ensure that "caller block" is aligned.
     * - "offset" is a uint32_t recording the offset in bytes of "caller block" within the block.
     * - "size" is a uint32_t recording the size of "caller block".
     * - "guard" is a uint32_t marker word with known value of 0xdddddddd.
     * - "waste" is an unused field of variable size left over within the block after offsetting of the "caller block".
     * 
     * size: The size of the memory block to allocate, in bytes, which must be a non-zero multiple of four bytes.
     * alignment: The alignment of the memory to allocate, in bytes, which must be a power of two.
     * return A pointer to the allocated memory.
     */
    inline virtual void *AllocateAligned(const SizeType size, const SizeType alignment);

    /*
     * Frees a previously allocated block of contiguous memory.
     *
     * memory: Pointer to the memory to be deallocated.
     */
    inline virtual void Free(void *const memory) override;

    /* 
     * Gets the number of bytes currently allocated by the allocator.
     *
     * This method is only useful when allocation checking is enabled using AF_ENABLE_DEFAULTALLOCATOR_CHECKS
     * If allocation checking is disabled then GetBytesAllocated returns zero.
     */
    inline uint32_t GetBytesAllocated() const;

    /*
     * Gets the peak number of bytes ever allocated by the allocator at one time.
     * 
     * This method is only useful when allocation checking is enabled using AF_ENABLE_DEFAULTALLOCATOR_CHECKS
     * If allocation checking is disabled then GetPeakBytesAllocated returns zero.
     */
    inline uint32_t GetPeakBytesAllocated() const;

    /*
     * Gets the number of distinct allocations performed by the allocator.
     * 
     * This method is only useful when allocation checking is enabled using AF_ENABLE_DEFAULTALLOCATOR_CHECKS
     * If allocation checking is disabled then GetAllocationCount returns zero.
     */
    inline uint32_t GetAllocationCount() const;

private:
    DefaultAllocator(const DefaultAllocator &other);
    DefaultAllocator &operator=(const DefaultAllocator &other);

    // Internal method which is force-inlined to avoid a function call.
    inline void *AllocateInline(const SizeType size, const SizeType alignment);

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    Detail::SpinLock spin_lock_;         // Synchronization object used to protect access to the allocation counts.
    uint32_t bytes_allocated_;           // Tracks the number of bytes currently allocated not yet freed.
    uint32_t peak_allocated_;            // Tracks the peak number of bytes allocated but not yet freed.
    uint32_t allocations_;               // Tracks the number of distinct allocations.
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

};


inline DefaultAllocator::DefaultAllocator() {

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    spin_lock_.Lock();

    bytes_allocated_ = 0;
    peak_allocated_ = 0;
    allocations_ = 0;

    spin_lock_.Unlock();
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

}

inline DefaultAllocator::~DefaultAllocator() {

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    // Memory leak detection.
    // Failures likely indicate AF bugs, unless the allocator is used by user code.
    if (bytes_allocated_ > 0) {
        AF_FAIL_MSG("DefaultAllocator detected memory leaks");
    }
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

}

inline void *DefaultAllocator::Allocate(const SizeType size) {
    // Default to 4-byte alignment in this lowest-level allocator.
    // This call is force-inlined.
    return AllocateInline(size, sizeof(int));
}

inline void *DefaultAllocator::AllocateAligned(const SizeType size, const SizeType alignment) {
    // This call is force-inlined.
    return AllocateInline(size, alignment);
}

inline void DefaultAllocator::Free(void *const memory) {
    // We don't expect to have allocated any blocks that aren't at least aligned to the machine word size.
    AF_ASSERT_MSG(memory, "Free of null pointer");
    AF_ASSERT_MSG(AF_ALIGNED(memory, sizeof(int)), "Free of unaligned pointer");

    uint32_t *const caller_block(reinterpret_cast<uint32_t *>(memory));

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    // Check the pre-and post-guard fields, bookending the caller block.
    const uint32_t *const offset_field(reinterpret_cast<uint32_t *>(caller_block) - 3);
    const uint32_t *const size_field(caller_block - 2);
    const uint32_t *const pre_guard_field(caller_block - 1);

    const uint32_t caller_block_size(*size_field);
    const uint32_t *const post_guard_field(reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(caller_block) + caller_block_size));

    if (*pre_guard_field != GUARD_VALUE || *post_guard_field != GUARD_VALUE) {
        AF_FAIL_MSG("Corrupted guardband indicates memory corruption");
    }

    spin_lock_.Lock();

    AF_ASSERT_MSG(bytes_allocated_ >= caller_block_size, "Unallocated free, suggests duplicate free");
    bytes_allocated_ -= caller_block_size;

    spin_lock_.Unlock();

#else
    uint32_t *const offset_field(reinterpret_cast<uint32_t *>(caller_block) - 1);
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

    // Address of the internally allocated block.
    const uint32_t caller_block_offset(*offset_field);
    uint8_t *const block(reinterpret_cast<uint8_t *>(caller_block) - caller_block_offset);

    // Free the memory block using global delete.
    AF_ASSERT(block);
    delete [] reinterpret_cast<uint8_t *>(block);
}

AF_FORCEINLINE uint32_t DefaultAllocator::GetBytesAllocated() const {

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    return bytes_allocated_;
#else
    return 0;
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

}

AF_FORCEINLINE uint32_t DefaultAllocator::GetPeakBytesAllocated() const {

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    return peak_allocated_;
#else
    return 0;
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

}

AF_FORCEINLINE uint32_t DefaultAllocator::GetAllocationCount() const {

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    return allocations_;
#else
    return 0;
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

}

AF_FORCEINLINE void *DefaultAllocator::AllocateInline(const SizeType size, const SizeType alignment) {
    // Minimum allocation size is 4 bytes.
    const SizeType block_size(size < 4 ? 4 : size);

    // Alignment values are expected to be powers of two greater than or equal to four bytes.
    // This ensures that the size, offset, and guard fields are 4-byte aligned.
    AF_ASSERT_MSG(alignment >= 4, "Actor and message memory alignments must be at least 4 bytes");
    AF_ASSERT_MSG((alignment & (alignment - 1)) == 0, "Actor and message alignments must be powers of two");

    // Allocation sizes are expected to be non-zero multiples of four bytes.
    // This ensures that the trailing guard field is 4-byte aligned.
    AF_ASSERT_MSG((block_size & 0x3) == 0, "Allocation of memory block not a multiple of four bytes in size");

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
    const uint32_t num_pre_fields(3);
    const uint32_t num_post_fields(1);
#else
    const uint32_t num_pre_fields(1);
    const uint32_t num_post_fields(0);
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

    // Calculate the size of the internally allocated block.
    // We assume underlying allocations are always 4-byte aligned, so padding is at most (alignment-4) bytes.
    const uint32_t max_padding_size(alignment - 4);
    const uint32_t preamble_size(max_padding_size + num_pre_fields * sizeof(uint32_t));
    const uint32_t postamble_size(num_post_fields * sizeof(uint32_t));
    const uint32_t internal_size(preamble_size + block_size + postamble_size);

    // Allocate the memory block using global new.
    uint32_t *const block = reinterpret_cast<uint32_t *>(new uint8_t[internal_size]);

    AF_ASSERT_MSG(internal_size >= 4, "Unexpected request to allocate less than 4 bytes");
    AF_ASSERT_MSG(AF_ALIGNED(block, 4), "Global new is assumed to always align to 4-byte boundaries");

    if (block) {
        // Calculate the pre-padding required to offset the caller block to an aligned address.
        // We do this by accounting for the hidden pre-fields and then aligning the pointer.
        uint8_t *caller_block(reinterpret_cast<uint8_t *>(block + num_pre_fields));
        AF_ALIGN(caller_block, alignment);

#if AF_ENABLE_DEFAULTALLOCATOR_CHECKS
        uint32_t *const offset_field(reinterpret_cast<uint32_t *>(caller_block) - 3);
        uint32_t *const size_field(reinterpret_cast<uint32_t *>(caller_block) - 2);
        uint32_t *const pre_guard_field(reinterpret_cast<uint32_t *>(caller_block) - 1);
        uint32_t *const post_guard_field(reinterpret_cast<uint32_t *>(caller_block + block_size));

        *size_field = block_size;
        *pre_guard_field = GUARD_VALUE;
        *post_guard_field = GUARD_VALUE;

        spin_lock_.Lock();

        bytes_allocated_ += block_size;
        if (bytes_allocated_ > peak_allocated_) {
            peak_allocated_ = bytes_allocated_;
        }

        ++allocations_;

        spin_lock_.Unlock();
#else
        uint32_t *const offset_field(reinterpret_cast<uint32_t *>(caller_block) - 1);
#endif // AF_ENABLE_DEFAULTALLOCATOR_CHECKS

        // Offset of the caller block within the internally allocated block, in bytes.
        const uint32_t caller_block_offset(static_cast<uint32_t>(caller_block - reinterpret_cast<uint8_t *>(block)));
        *offset_field = caller_block_offset;

        // Caller gets back the address of the caller block, which is expected to be aligned.
        AF_ASSERT(AF_ALIGNED(caller_block, alignment));
        return caller_block;
    }

    AF_FAIL_MSG("Out of memory in DefaultAllocator!");
    return 0;
}


} // namespace AF


#endif // AF_DEFAULTALLOCATOR_H
