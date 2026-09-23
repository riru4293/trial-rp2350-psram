#pragma once

/**
 * @file pal_psram_arena.hpp
 * @brief PSRAM region allocator.
 *
 * @details
 * Owns the available PSRAM address space and lends non-overlapping,
 * exactly-sized contiguous regions. Callers can select the cached or
 * uncached XIP alias for each returned region.
 *
 * Allocations advance a shared cursor and are never released or reused.
 * The available capacity is determined on the first allocation. A zero-sized
 * request or a request exceeding the remaining capacity is unrecoverable and
 * triggers a software reset.
 */

/* C++ standard library */
#include <cstddef>
#include <cstdint>

namespace pal
{
    /**
     * @brief  A contiguous PSRAM XIP address range.
     */
    struct PsramRegion
    {
        /**
         * @brief Construct a PSRAM region descriptor.
         *
         * @param base [in] Start address of the region.
         * @param size [in] Size of the region in bytes.
         */
        explicit PsramRegion(std::uintptr_t base, std::size_t size) noexcept
                : base(base), size(size) {}

        std::uintptr_t const base; //!< Start address of the region.
        std::size_t const size;    //!< Size of the region in bytes.
    };

    /**
     * @brief  Allocate a contiguous PSRAM address range.
     *
     * @param size [in] Number of bytes to allocate.
     * @param cached [in] `true` to return the cached XIP alias; `false` for
     *                     the uncached alias.
     *
     * @return Descriptor for the allocated region.
     */
    PsramRegion allocatePsramRegion(
            std::size_t size, bool cached = false) noexcept;
}
