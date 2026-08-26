#include "./include/mw_heap_repo.hpp"

#include <pal_reset.hpp>

void mw::HeapRepository::put(mw::HeapId id, pal::Heap *heap)
{
    std::uint8_t idx = static_cast<std::uint8_t>(id);
    if ((idx < kHeapNum) && !heaps_[idx])
    {
        heaps_[idx] = heap;
    }
    else
    {
        pal::panic("HeapRepository::put: invalid heap id");
        __builtin_unreachable();
    }
}

pal::Heap &mw::HeapRepository::get(mw::HeapId id)
{
    std::uint8_t idx = static_cast<std::uint8_t>(id);
    if ((idx < kHeapNum) && heaps_[idx])
    {
        return *heaps_[idx];
    }
    else
    {
        pal::panic("HeapRepository::get: invalid heap id");
        __builtin_unreachable();
    }
}
