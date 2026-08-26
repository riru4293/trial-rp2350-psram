#include "./include/pal_psram_arena.hpp"

#include <pal_critical_section.hpp>
#include <pal_reset.hpp>

/* pico-sdk */
#include <hardware/psram.h>

/* C++ standard library */
#include <cstddef>
#include <cstdint>

namespace
{
    std::uintptr_t constexpr kCachedBase   = 0x11000000u;
    std::uintptr_t constexpr kUncachedBase = 0x15000000u;
}

pal::PsramRegion pal::allocatePsramRegion(
        std::size_t size, bool cached) noexcept
{
    pal::enterCriticalSection();

    static std::size_t const total = []() {return psram_get_size();}();
    static std::size_t remain = total;

    /* Panic if illegal argument */
    //std::size_t const remain = kPsramEndAddr - next_alloc_addr;
    if ((size == 0u) || (size > remain))
    {
        pal::panic("pal::allocatePsramRegion: Illegal argument");
        __builtin_unreachable();
    }

    std::size_t const offset = total - remain;
    std::uintptr_t const addr =
        (cached ? kCachedBase : kUncachedBase) + offset;
    remain -= size;

    pal::leaveCriticalSection();

    return pal::PsramRegion(addr, size);
}
