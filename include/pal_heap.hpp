#pragma once

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
     * This allocator manages one contiguous region and tracks free space by
     * block metadata stored in the region itself.
     *
     * Region and block image:
     *
     *   root_ -> [Meta] -> [Meta] -> [Meta] -> nullptr
     *              |        |        |
     *              v        v        v
     *          +--------+--------+--------+
     *          | Block0 | Block1 | Block2 |
     *          +--------+--------+--------+
     *
     *   One block layout:
     *
     *          +----------------+----------------------+
     *          | Meta (header)  | Payload              |
     *          +----------------+----------------------+
     *                             ^
     *                             +-- allocate() return pointer
     *
     * allocate() may split one free block into lead/alloc/trail blocks.
     * deallocate() merges adjacent free blocks into one larger block.
     */
    class Heap
    {
    public:
        /**
         * @brief Construct a heap on the specified PSRAM region.
         *
         * @note Invalid region arguments create a zero-capacity heap.
         *
         * @param region [in] Target region to manage.
         */
        explicit Heap(pal::PsramRegion const region) noexcept;

        /**
         * @brief Allocate aligned memory from this heap.
         *
         * @note This function is non-reentrant and
         *       executes heap mutation under a critical section.
         *
         * @param size  [in] Requested payload size in bytes.
         * @param align [in] Required payload alignment in bytes.
         *
         * @return Pointer to allocated payload, or nullptr for invalid request.
         *         Panics on out-of-memory.
         */
        void *allocate(std::size_t size, std::size_t align) noexcept;

        /**
         * @brief Release a previously allocated block.
         *
         * @note This function is non-reentrant and
         *       executes heap mutation under a critical section.
         *
         * @param p [in] Pointer returned by allocate().
         *               Panics if the pointer is invalid or already freed.
         */
        void deallocate(void const *p) noexcept;

        /**
         * @brief Get current free payload bytes.
         *
         * @note This function is non-reentrant and
         *       executes heap mutation under a critical section.
         *
         * @return Current free bytes.
         */
        std::size_t getFreeBytes(void) const noexcept;

        /**
         * @brief Get the minimum free payload bytes ever observed.
         *
         * @note This function is non-reentrant and
         *       executes heap mutation under a critical section.
         *
         * @return Historical low-watermark of free bytes.
         */
        std::size_t getLowestEverFreeBytes(void) const noexcept;

        /**
         * @brief Dump the current memory map for debugging purposes.
         *
         * @note This function is non-reentrant and
         *       executes heap mutation under a critical section.
         */
        void dumpMemoryMap(void) const noexcept;

    private:
        std::unique_ptr<void, void (*)(void *)> root_;
        std::size_t free_bytes_ = 0u;
        std::size_t lowest_ever_free_bytes_ = 0u;
    }; // class Heap
} // namespace pal
