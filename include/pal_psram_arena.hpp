#pragma once

/* C++ standard library */
#include <cstddef>
#include <cstdint>

namespace pal
{
    class PsramRegion
    {
    public:
        explicit PsramRegion(std::uintptr_t base, std::size_t size) noexcept
            : base(base), size(size) {}

        std::uintptr_t const base;
        std::size_t const size;
    };

    PsramRegion allocatePsramRegion(std::size_t size) noexcept;
}
