#include "./include/pal_panic.hpp"

/* pico-sdk */
#include <hardware/irq.h>
#include <hardware/sync.h>
#include <hardware/uart.h>
#include <hardware/watchdog.h>

/* C++ standard library */
#include <atomic>

namespace
{
    uint8_t constexpr kMaxChars = 100u;
    uint16_t constexpr kMaxWait = 1000u;
    volatile std::atomic<uint32_t>lock{0};
}

[[noreturn]] void __not_in_flash_func(pal::panic)(char const *msg)
{
    /* Losers are stuck in an infinity loop */
    if (lock.exchange(1u, std::memory_order_acq_rel))
    {
        while (true)
        {
            tight_loop_contents();
        }
    }

    /* Won't let anyone get in my way */
    (void)save_and_disable_interrupts();
    irq_set_mask_enabled(0xFFFFFFFFu/*mask*/, false/*enabled*/);

    /* Note:
     *  No intention of stopping the others.
     *  Because there's no guarantee.
     */

    /* Reset count down has begun */
    watchdog_enable(2000u/*ms*/, true/*pause on debug*/);
    watchdog_update();

    /* Leave a dying message */
    uint8_t sent = 0u;
    while (msg[sent] && sent < kMaxChars)
    {
        if (!(uart0_hw->fr & UART_UARTFR_TXFF_BITS))
        {
            /* Write if there's space in the TX */
            uart_get_hw(uart0)->dr = (uint8_t)msg[sent++];
        }
        else
        {
            /* Will wait just a little bit */
            uint16_t wait = 0u;
            while ((uart0_hw->fr & UART_UARTFR_TXFF_BITS) &&
                    wait < kMaxWait)
            {
                wait++;
            }

            if (wait >= kMaxWait)
            {
                break; /* Give up */
            }
        }
    }

    /* Quietly waiting for the end */
    while (true)
    {
        __asm volatile ("wfi");
    }

    __builtin_unreachable();
}
