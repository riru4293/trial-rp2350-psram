#include <pal_psram_arena.hpp>

#include <pal_critical_section.hpp>
#include <pal_panic.hpp>

/* C++ standard library */
#include <cstddef>
#include <cstdint>

namespace
{
    /* Constants */
    /* Note: About the begin address (0x15000000) is
     *  0x14000000 .. XIP_NOCACHE_NOALLOC_BASE
     *  +
     *  0x01000000 .. flash_devinfo_size_to_bytes(FLASH_DEVINFO_SIZE_MAX)
     *                  (flash memory area)
     */
    std::size_t constexpr kPsramBytes = 0x800000u; /**< 8 MiB */
    std::uintptr_t constexpr kPsramBeginAddr = 0x15000000u;
    std::uintptr_t constexpr kPsramEndAddr = kPsramBeginAddr + kPsramBytes;

    /* Variables */
    std::uintptr_t next_alloc_addr = kPsramBeginAddr;
}

pal::PsramRegion pal::allocatePsramRegion(std::size_t size) noexcept
{
    pal::enterCriticalSection();

    /* Panic if illegal argument */
    std::size_t const remain = kPsramEndAddr - next_alloc_addr;
    if ((size == 0u) || (size > remain))
    {
        pal::panic("palPsramArenaAllocate: Illegal argument");
        return pal::PsramRegion(0u, 0u); /* No reached */
    }

    std::uintptr_t const addr = next_alloc_addr;
    next_alloc_addr += size;

    pal::leaveCriticalSection();

    return pal::PsramRegion(addr, size);
}
