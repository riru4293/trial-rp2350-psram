#pragma once

/**
 * @file pal_psram_arena.hpp
 * @brief PSRAM region allocator.
 *
 * @details
 * Provides non-overlapping regions from the available PSRAM capacity. Each
 * allocation advances a shared cursor; allocated regions are never released
 * or reused. The available capacity is determined when the first allocation
 * is made.
 *
 * Each allocation selects either the cached or uncached XIP address alias for
 * its returned base address. A zero-sized request or a request exceeding the
 * remaining capacity is an unrecoverable error and requests a software reset.
 */

/* C++ standard library */
#include <cstddef>
#include <cstdint>

namespace pal
{
    /**
     * @brief A contiguous allocated PSRAM region.
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
     * @brief Allocate a contiguous PSRAM region.
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
