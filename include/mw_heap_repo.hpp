#pragma once

#include <pal_heap.hpp>

/* C++ standard library */
#include <array>
#include <cstdint>

namespace mw
{
    /**
     * @brief Heap manager identifier.
     *
     * @details
     * Used to select a heap manager in the repository.
     */
    enum class HeapId : std::uint8_t
    {
        Core0Psram,         //!< Core#0 no cached PSRAM
        Core1Psram,         //!< Core#1 no cached PSRAM
        Core0PsramCached,   //!< Core#0 cached PSRAM
        Core1PsramCached,   //!< Core#1 cached PSRAM
        Count               //!< Number of the heap id
    };

    /**
     * @brief Heap repository singleton.
     *
     * @details
     * Populated during system initialization (one registration per
     * HeapId) and treated as immutable thereafter. No synchronization is
     * provided; callers must perform registration before concurrent use.
     */
    class HeapRepository
    {
    public:
        /**
         * @brief Register a heap manager.
         *
         * @pre `id != HeapId::Count`
         * @pre `heap != nullptr`
         * @pre The `id` is not already registered
         *
         * Panics on precondition violation.
         *
         * @param id   [in] Heap identifier
         * @param heap [in] Pointer to the heap manager
         */
        static void put(HeapId id, pal::Heap *heap);

        /**
         * @brief Return the registered heap manager.
         *
         * @pre The `id` is already registered
         * Panics if the id is not registered.
         *
         * @param id [in] heap manager id
         * @return The `pal::Heap`
         */
        static pal::Heap &get(HeapId id);

    private:
        static constexpr std::uint8_t kHeapNum =
            static_cast<std::uint8_t>(HeapId::Count);

        static inline std::array<pal::Heap *, kHeapNum> heaps_{};
    };
};
