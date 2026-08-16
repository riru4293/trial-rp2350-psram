#include "./include/pal_panic.hpp"

/* pico-sdk */
#include <pico.h>
#include <pico/platform.h>
#include <hardware/watchdog.h>

/* C++ standard library */
#include <cstdio>

[[noreturn]] void __not_in_flash_func(pal::panic)(char const *msg)
{
    printf("PANIC\n%s\n", msg);

    watchdog_reboot(0, 0, 8000);
    while (1)
    {
        tight_loop_contents();
    }
    __builtin_unreachable();
}
