#pragma once

#include "./mw_heap_repo.hpp"

/* C++ standard library */
#include <cstddef>

namespace mw
{
    template <typename T>
    class StlAllocator
    {
    public:
        using value_type = T;

        explicit StlAllocator(HeapId id) noexcept : heap_id_(id) {}

        template<class U>
        explicit StlAllocator(StlAllocator<U> const &other) noexcept
            : heap_id_(other.heap_id_) {}

        [[nodiscard]] T *allocate(std::size_t n) noexcept
        {
            return static_cast<T *>(mw::HeapRepository::get(heap_id_)
                    .allocate(n * sizeof(T), alignof(T)));
        }

        void deallocate(T *p, size_t) noexcept
        {
            mw::HeapRepository::get(heap_id_).deallocate(p);
        }

    private:
        HeapId const heap_id_;
    }; // class StlAllocator
}; // namespace mw
