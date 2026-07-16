#include <pico/stdlib.h>
#include <hardware/psram.h>
#include <pico/cyw43_arch.h>

#include <cstdio>
#include <cstdint>
#include <cstring>

static bool constexpr LED_OFF = false;
static bool constexpr LED_ON = true;

int main( void )
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        printf("Wi-Fi init failed");
        return -1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);

    sleep_ms(2000); // Wait until the USB UART is initialized.

    printf("PSRAM available %d.\n", psram_is_available());

    size_t const sizeof_psram = psram_get_size();
    printf("PSRAM size %zu.\n", sizeof_psram);

    uint32_t constexpr NO_CACHE_MASK = ~0x4000000;
    uintptr_t constexpr CACHED_BASE_ADDR = 0x11000000;
    uintptr_t constexpr NO_CACHED_BASE_ADDR = CACHED_BASE_ADDR & NO_CACHE_MASK;

    uintptr_t constexpr BASE_ADDR = NO_CACHED_BASE_ADDR;
    uintptr_t const END_EXCLUSIVE_ADDR = BASE_ADDR + sizeof_psram;

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);
    printf("Clear PSRAM\n");
    memset((uint32_t *)BASE_ADDR, 0xA5, sizeof_psram);
    uint32_t a = 0U;
    for(a = BASE_ADDR; a < END_EXCLUSIVE_ADDR; a++)
    {
        if(*(uint32_t *)a != 0xA5A5A5A5U) break;
        tight_loop_contents();
    }
    if(a == END_EXCLUSIVE_ADDR) { printf("Clear [OK]\n"); }
    else                        { printf("Clear [NG]\n"); }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);

    sleep_ms(2000); // Wait

    // Note: Infinity loop
    for(uint8_t offset = 0U; offset <= UINT8_MAX; offset++)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);
        printf("Test %u: ", offset + 1U);
        uint32_t end_exclusive = sizeof_psram / sizeof(uint64_t);
        for(uint32_t i = 0U; i < end_exclusive; i++)
        {
            uint64_t n = (i + offset) % UINT8_MAX;
            uint64_t v = (n << 56U) + (n << 48U) + (n << 40U) + (n << 32U)
                       + (n << 24U) + (n << 16U) + (n <<  8U) + n;
            ((uint64_t *)BASE_ADDR)[i] = v;
            tight_loop_contents();
        }
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);
        bool isOk = true;
        for(uint32_t i = 0U; i < end_exclusive; i++)
        {
            uint64_t n = (i + offset) % UINT8_MAX;
            uint64_t v = (n << 56U) + (n << 48U) + (n << 40U) + (n << 32U)
                       + (n << 24U) + (n << 16U) + (n <<  8U) + n;
            isOk = isOk && (((uint64_t *)BASE_ADDR)[i] == v);
            tight_loop_contents();
        }
        if(isOk) { printf("[OK]\n"); } else { printf("[NG]\n"); break; }
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);

    return 0;
}

