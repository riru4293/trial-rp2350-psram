#pragma once

/**
 * @file pal_heap.hpp
 * @brief Free-list heap for a fixed PSRAM region.
 *
 * @details
 * Declares @ref Heap, which manages dynamic allocations within one contiguous,
 * writable PSRAM region supplied by its caller. The heap does not own that
 * region; it must remain valid for the heap's lifetime. Block metadata is
 * stored within the managed region.
 *
 * Allocations may split a free block, and deallocation coalesces adjacent free
 * blocks. All public member functions acquire a recursive critical section, so
 * concurrent calls are serialized.
 *
 * An invalid allocation request, invalid deallocation pointer, or allocation
 * that cannot be satisfied requests a software reset. A region that cannot
 * hold a metadata header and one payload byte creates a zero-capacity heap.
 * Heap instances cannot be copied or moved.
 */

#include <pal_psram_arena.hpp>

/* C++ standard library */
#include <cstddef>
#include <memory>

namespace pal
{
    /**
     * @brief Simple free-list heap on a fixed PSRAM region.
     *
     * @details
     * @code
     * root_ -> [Sentinel] -> [Meta] -> [Meta] -> nullptr
     *                              |         |
     *                              v         v
     *                         [Payload] [Payload]
     *
     * One block:
     * +----------------+----------------------+
     * | Meta (header)  | Payload              |
     * +----------------+----------------------+
     *                         ^
     *                         +-- allocate() return pointer
     * @endcode
     *
     * A split can produce leading, allocated, and trailing blocks.
     */
    class Heap
    {
    public:
        Heap(Heap const &) = delete;
        Heap &operator=(Heap const &) = delete;
        Heap(Heap &&) = delete;
        Heap &operator=(Heap &&) = delete;

        /**
         * @brief Construct a heap on the specified PSRAM region.
         *
         * @note A region too small to hold a metadata header and one payload
         *       byte, or one that overflows during alignment adjustment,
         *       creates a zero-capacity heap.
         *
         * @param region [in] Writable PSRAM region to manage. It must outlive
         *                    this heap.
         */
        explicit Heap(pal::PsramRegion const region) noexcept;

        /**
         * @brief Allocate aligned memory from this heap.
         *
         * @param size [in] Requested payload size in bytes. It must be nonzero
         *                  and fit an allocation after alignment.
         * @param align [in] Required payload alignment in bytes. It must be a
         *                   power of two no greater than
         *                   `alignof(std::max_align_t)`.
         *
         * @return Pointer to the allocated payload.
         */
        void *allocate(std::size_t size, std::size_t align) noexcept;

        /**
         * @brief Release a previously allocated block.
         *
         * @param p [in] Pointer returned by allocate().
         *               A null pointer has no effect. An invalid or already
         *               freed pointer requests a software reset.
         */
        void deallocate(void *p) noexcept;

        /**
         * @brief Get current free payload bytes.
         *
         * @return Current free bytes.
         */
        std::size_t getFreeBytes(void) const noexcept;

        /**
         * @brief Get the minimum free payload bytes ever observed.
         *
         * @return Historical low-watermark of free bytes.
         */
        std::size_t getLowestEverFreeBytes(void) const noexcept;

        /**
         * @brief Write the current memory map as a Markdown table.
         *
         * @details
         * Writes the table to standard output for debugging.
         */
        void dumpMemoryMap(void) const noexcept;

    private:
        std::unique_ptr<void, void (*)(void *)> root_;
        std::size_t free_bytes_ = 0u;
        std::size_t lowest_ever_free_bytes_ = 0u;
    }; // class Heap
} // namespace pal
