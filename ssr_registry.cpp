#include "./include/ssr_registry.hpp"

/* pico-sdk */
#include <pico/sync.h>

/* C++ standard library */
#include <atomic>

namespace
{
    std::atomic<bool> in_panic{false};
}

bool __not_in_flash_func(ssr::isPanic)(void) noexcept
{
    return in_panic.load(std::memory_order_acquire);
}

void __not_in_flash_func(ssr::setPanic)(void) noexcept
{
    in_panic.store(true, std::memory_order_release);
}
