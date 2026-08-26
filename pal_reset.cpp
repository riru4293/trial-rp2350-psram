#include "./include/pal_reset.hpp"

/* pico-sdk */
#include <hardware/structs/psm.h>
#include <hardware/sync.h>
#include <hardware/uart.h>
#include <hardware/watchdog.h>

/* C++ standard library */
#include <atomic>
#include <cstdint>
#include <cstring>

namespace
{
    uint32_t constexpr kMagicNum = 3319512153u;

    std::atomic<bool>latch{false};

    uint32_t __uninitialized_ram(dying_code);
    char __uninitialized_ram(dying_msg)[100];

    __force_inline bool __not_in_flash_func(uartTxIsFull)(void)
    {
        return uart0_hw->fr & UART_UARTFR_TXFF_BITS;
    }
}

std::optional<std::string_view> pal::getDyingMessage(void)
{
    if (dying_code == kMagicNum)
    {
        dying_code = 0u;
        return std::string_view(dying_msg,
                strnlen(dying_msg, sizeof(dying_msg)));
    }

    return std::nullopt;
}

[[noreturn]] void __not_in_flash_func(pal::panic)(std::string const &msg)
{
    /* Losers are stuck in an infinity loop */
    if (latch.exchange(true, std::memory_order_acq_rel))
        while (true) tight_loop_contents();

    /* Won't let anyone get in my way */
    (void)save_and_disable_interrupts();

    /* Note:
     *  No intention of stopping the others.
     *  Because there's no guarantee.
     */

    /* Reset count down has begun */
    {
        uint32_t constexpr kDelayUs = 2000u * 1000u;
        uint32_t constexpr kDbgBits = WATCHDOG_CTRL_PAUSE_DBG0_BITS
                                    | WATCHDOG_CTRL_PAUSE_DBG1_BITS
                                    | WATCHDOG_CTRL_PAUSE_JTAG_BITS;
        watchdog_hw->scratch[4] = 0x6ab73121u; /* WATCHDOG_NON_REBOOT_MAGIC */
        hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
        hw_set_bits(&psm_hw->wdsel,
            PSM_WDSEL_BITS & ~(PSM_WDSEL_ROSC_BITS | PSM_WDSEL_XOSC_BITS));
        hw_set_bits(&watchdog_hw->ctrl, kDbgBits);
        watchdog_hw->load = kDelayUs;
        hw_set_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
    }

    /* Leave a dying message */
    {
        char const * const src = msg.data();
        size_t const n = msg.size() < sizeof(dying_msg) - 1u
                       ? msg.size() : sizeof(dying_msg) - 1u;
        for (size_t i = 0u; i < n; ++i) dying_msg[i] = src[i];
        dying_msg[n] = '\0';
    }
    dying_code = kMagicNum;

    uint8_t sent = 0u;
    while (dying_msg[sent] && sent < sizeof(dying_msg))
    {
        if (!uartTxIsFull())
        {
            /* Write if there's space in the TX */
            uart_get_hw(uart0)->dr =
                static_cast<uint8_t>(dying_msg[sent++]);
        }
        else
        {
            /* Will wait just a little bit */
            volatile uint16_t wait = 0u;
            uint16_t constexpr kMaxWait = 1000u;
            while (uartTxIsFull() && wait < kMaxWait) wait++;

            if (wait >= kMaxWait) break; /* Give up */
        }
    }

    /* Quietly waiting for the end */
    while (true) __asm volatile ("wfi");

    __builtin_unreachable();
}
