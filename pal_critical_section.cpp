#include "./include/pal_critical_section.hpp"

/* pico-sdk */
#include <pico/mutex.h>

namespace
{
    recursive_mutex_t make_recursive_mutex()
    {
        recursive_mutex_t m = {0};
        recursive_mutex_init(&m);
        return m;
    }

    recursive_mutex_t nutex = make_recursive_mutex();
}

void pal::enterCriticalSection(void)
{
    recursive_mutex_enter_blocking(&nutex);
}

void pal::leaveCriticalSection(void)
{
    recursive_mutex_exit(&nutex);
}
