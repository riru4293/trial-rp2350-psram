#include "./include/pal_reset.hpp"
#include <ssr_registry.hpp>

/* pico-sdk */
#include <hardware/structs/psm.h>
#include <hardware/sync.h>
#include <hardware/uart.h>
#include <hardware/watchdog.h>

/* C++ standard library */
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

using namespace pal;

namespace
{
    uint32_t constexpr kMyNumber = 3319512153u;

    std::atomic<bool>reset_latch{false};

    uint32_t __uninitialized_ram(blood_stain);
    ResetKind __uninitialized_ram(cause_of_death);
    char __uninitialized_ram(dying_msg)[100];

    __force_inline bool __not_in_flash_func(uartTxIsFull)(void)
    {
        return uart0_hw->fr & UART_UARTFR_TXFF_BITS;
    }
}

std::optional<ResetContext> pal::getResetContext(void) noexcept
{
    static std::atomic<bool>latch{false};

    if (latch.exchange(true, std::memory_order_acq_rel))
        return std::nullopt;

    if (blood_stain == kMyNumber)
    {
        blood_stain = 0u;
        std::size_t msg_size = strnlen(dying_msg, sizeof(dying_msg));

        return ResetContext{
            cause_of_death, std::string_view(dying_msg, msg_size)};
    }

    return std::nullopt;
}

[[noreturn]] void __not_in_flash_func(
        pal::panic)(std::string const &msg) noexcept
{
    /* Losers are stuck in an infinity loop */
    if (reset_latch.exchange(true, std::memory_order_acq_rel))
        while (true) tight_loop_contents();

    /* Set now panic to the system state registry */
    ssr::setPanic();

    /* Won't let anyone get in my way */
    static_cast<void>(save_and_disable_interrupts());

    /* Note:
     *  No intention of stopping the others.
     *  Because there's no guarantee.
     */

    /* Reset count down has begun */
    {
        uint32_t constexpr kDelayUs = 2000u * 1000u; // 2 seconds
        hw_clear_bits(&watchdog_hw->ctrl,
            WATCHDOG_CTRL_ENABLE_BITS |
            WATCHDOG_CTRL_PAUSE_DBG0_BITS |
            WATCHDOG_CTRL_PAUSE_DBG1_BITS |
            WATCHDOG_CTRL_PAUSE_JTAG_BITS);
        hw_set_bits(&psm_hw->wdsel,
            PSM_WDSEL_BITS & ~(PSM_WDSEL_ROSC_BITS | PSM_WDSEL_XOSC_BITS));
        watchdog_hw->load = kDelayUs;
        hw_set_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
    }

    /* Leave a dying message */
    {
        char const *const src = msg.data();
        size_t const n = msg.size() < sizeof(dying_msg) - 1u
                       ? msg.size() : sizeof(dying_msg) - 1u;
        for (size_t i = 0u; i < n; i++) dying_msg[i] = src[i];
        dying_msg[n] = '\0';
    }
    cause_of_death = ResetKind::Panic;
    blood_stain = kMyNumber;

    uint8_t sent = 0u;
    while (sent < sizeof(dying_msg) && dying_msg[sent])
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

[[noreturn]] void pal::reset(std::string const &msg) noexcept
{
    /* Losers are stuck in an infinity loop */
    if (reset_latch.exchange(true, std::memory_order_acq_rel))
        while (true) tight_loop_contents();

    /* Leave a dying message */
    {
        std::snprintf(dying_msg, sizeof(dying_msg), "%s", msg.c_str());
        cause_of_death = ResetKind::SoftReset;
        blood_stain = kMyNumber;
    }

    /* Request a reboot */
    watchdog_reboot(0/*Standard boot will be performed*/,
                    0/*No use a stack pointer*/,
                    0/*Delay milli seconds*/);

    /* Waiting for the end */
    while (true) tight_loop_contents();

    __builtin_unreachable();
}
