#include <pal_psram_arena.hpp>
#include <pico/stdlib.h>
#include <hardware/psram.h>
#include <pico/cyw43_arch.h>
#include <tusb.h>

#include <array>
#include <vector>

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstring>



static bool constexpr LED_OFF = false;
static bool constexpr LED_ON = true;

/* 試作 */

enum class HeapId : uint8_t
{
    Default, // デフォルトヒープ
    Count    // 要素数カウント用
};

#include <pal_heap.hpp>

using namespace pal;

class HeapRepository
{
public:
    static void do_register(HeapId id, Heap *heap)
    {
        uint8_t idx = static_cast<uint8_t>(id);
        if (idx < static_cast<uint8_t>(HeapId::Count))
        {
            heaps_[idx] = heap;
        }
        else
        {
            // エラー処理: ToDo: panic
        }
    }

    static Heap &get(HeapId id = HeapId::Default)
    {
        uint8_t idx = static_cast<uint8_t>(id);
        if ((idx < static_cast<uint8_t>(HeapId::Count)) && heaps_[idx])
        {
            return *heaps_[idx];
        }
        else
        {
            // エラー処理: ToDo: panic 暫定でデフォルトヒープを返す
            return *heaps_[static_cast<uint8_t>(HeapId::Default)];
        }
    }

private:
    // 動的割当（std::map等）を使わず、固定長配列で管理するのが組み込みでは安全
    static inline std::array<Heap *, static_cast<uint8_t>(HeapId::Count)> heaps_{};
};

template <typename T>
class PsramAllocator
{
public:
    using value_type = T;

    explicit PsramAllocator(HeapId id = HeapId::Default) noexcept
        : heap_id_(id) {}

    template<class U>
    explicit PsramAllocator(PsramAllocator<U> const &other) noexcept
        : heap_id_(other.heap_id_) {}

    [[nodiscard]] T *allocate(size_t n) noexcept
    {
        return static_cast<T *>(HeapRepository::get(heap_id_)
                .allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T *p, size_t) noexcept
    {
        HeapRepository::get(heap_id_).deallocate(p);
    }

private:
    HeapId const heap_id_;
};
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
    //pal::PsramRegion const region = pal::allocatePsramRegion(4U * 1024U * 1024U);
    pal::PsramRegion const dust = pal::allocatePsramRegion(1U);
    pal::PsramRegion const region = pal::allocatePsramRegion(100U);

    // 2. ヒープを作成
    static Heap default_heap(region);

    // 3. リポジトリへ登録
    HeapRepository::do_register(HeapId::Default, &default_heap);

    // 利用例
    printf("Start proc");
    /*
    std::vector<int, PsramAllocator<int>> vec;
    vec.clear();
    //vec.reserve(100);
    for(int i = 0; i < 999; i++)
    {
        vec.push_back(i);
    }

    vec.push_back(42);
    vec.push_back(100);
    vec.push_back(200);
    vec.push_back(300);
    printf("Inter proc");
    */
    sleep_ms(2000);
    printf("region: base=0x%08X, size=%zu\n", region.base, region.size);
    /*
    for (auto const &val : vec)
    {
        // ここで val を使用する
        printf("Value: %d\n", val);
    }
    */
    default_heap.dumpMemoryMap();

    printf("\nStep 1\n");
    void *a = default_heap.allocate(39, 4);
    if (!a) printf("Failed allocation\n");
    default_heap.dumpMemoryMap();

    printf("\nStep 2\n");
    void *b = default_heap.allocate(30, 8);
    if (!b) printf("Failed allocation\n");
    default_heap.dumpMemoryMap();
/*
    printf("\nStep 3\n");
    void *c = default_heap.allocate(400, 4);
    default_heap.dumpMemoryMap();

    printf("\nStep 4\n");
    default_heap.deallocate(c);
    default_heap.dumpMemoryMap();

    printf("\nStep 5\n");
    default_heap.deallocate(a);
    default_heap.dumpMemoryMap();

    printf("\nStep 6\n");
    default_heap.deallocate(b);
    default_heap.dumpMemoryMap();
 */  
    while (1)
    {
        tight_loop_contents();
        sleep_ms(10);
    }
    __builtin_unreachable();

    return 0;
}
