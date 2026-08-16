#include <pal_psram_arena.hpp>
#include <pico/stdlib.h>
#include <hardware/psram.h>
#include <pico/cyw43_arch.h>
#include <tusb.h>

#include <vector>

#include <cstdio>
#include <cstring>

#include <pal_heap.hpp>
#include <mw_heap_repo.hpp>
#include <mw_stl_allocator.hpp>

static bool constexpr LED_OFF = false;
static bool constexpr LED_ON = true;

/* 試作 */
using namespace pal;

/* 試作 */

int main( void )
{
    stdio_init_all();

    printf("Bootup");

    if (cyw43_arch_init())
    {
        printf("Wi-Fi init failed.");
        return -1;
    }

    /* Wait until the USB UART is connected. */
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);
    while (!tud_cdc_connected())
    {
        tight_loop_contents();
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);

    if (!psram_is_available())
    {
        printf("PSRAM is not available." );
        return -2;
    }

    // 1. PSRAM領域を確保
    pal::PsramRegion const region = pal::allocatePsramRegion(4U * 1024U * 1024U, false);
    pal::PsramRegion const region2 = pal::allocatePsramRegion(4U * 1024U * 1024U, true);

    // 2. ヒープを作成
    static Heap c0_psram_heap(region);
    static Heap c0_cached_psram_heap(region2);

    // 3. リポジトリへ登録
    mw::HeapRepository::put(mw::HeapId::Core0Psram, &c0_psram_heap);
    mw::HeapRepository::put(mw::HeapId::Core0PsramCached, &c0_cached_psram_heap);

    // 利用例
    printf("Start proc");

    mw::StlAllocator<int> allocator(mw::HeapId::Core0Psram);
    mw::StlAllocator<int> allocator2(mw::HeapId::Core0PsramCached);
    std::vector<int, decltype(allocator)> vec(allocator);
    std::vector<int, decltype(allocator)> vec2(allocator2);
    uint32_t st1 = time_us_32();
    vec.clear();
    for(int i = 0; i < 999; i++)
    {
        vec.push_back(i);
    }
    for(int i = 0; i < 999; i++)
    {
    for (auto &val : vec)
    {
        // ここで val を使用する
        val++;
    }
    }
    uint32_t ed1 = time_us_32();
    uint32_t st2 = time_us_32();
    vec2.clear();
    for(int i = 0; i < 999; i++)
    {
        vec2.push_back(i);
    }
    for(int i = 0; i < 999; i++)
    {
    for (auto &val : vec2)
    {
        // ここで val を使用する
        val++;
    }
    }
    uint32_t ed2 = time_us_32();
    printf("Inter proc");

    sleep_ms(2000);
    printf("region: base=0x%08X, size=%zu\n", region.base, region.size);

    c0_psram_heap.dumpMemoryMap();

    printf("\nStep 1\n");
    void *a = c0_psram_heap.allocate(39, 4);
    if (!a) printf("Failed allocation\n");
    c0_psram_heap.dumpMemoryMap();

    printf("\nStep 2\n");
    void *b = c0_psram_heap.allocate(30, 8);
    if (!b) printf("Failed allocation\n");
    c0_psram_heap.dumpMemoryMap();

    printf("\nStep 3\n");
    void *c = c0_psram_heap.allocate(400, 4);
    c0_psram_heap.dumpMemoryMap();
    c0_psram_heap.dumpMemoryMap();

    printf("\nStep 4\n");
    c0_psram_heap.deallocate(b);
    c0_psram_heap.dumpMemoryMap();

    printf("time1 = %u us\n", ed1 - st1);
    printf("time2 = %u us\n", ed2 - st2);

    while (1)
    {
        tight_loop_contents();
        sleep_ms(10);
    }
    __builtin_unreachable();

    return 0;
}
