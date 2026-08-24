#include "./include/pal_heap.hpp"
#include <pal_critical_section.hpp>
#include <pal_panic.hpp>

/* C++ standard library */
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

namespace
{
    /* Aliases */
    using Addr = std::uintptr_t;
    using Size = std::size_t;

    // ====================================================
    //  Structs
    // ====================================================

    /* Represents a memory block metadata. */
    struct Meta
    {
        Meta *next = nullptr; // Pointer to the next block
        Size size = 0u;       // Size of the payload
    };
    static_assert(sizeof(Meta) == 8u);

    // ====================================================
    //  Constants
    // ====================================================

    Addr constexpr kMaxAddr /* Maximum memory address */
        = std::numeric_limits<Addr>::max();
    Size constexpr kMinSize = 0x00000001u;   /* Min block size */
    Size constexpr kMaxSize = 0x00FFFFFFu;   /* Max block size */
    Size constexpr kMetaSize = sizeof(Meta); /* Metadata size  */
    Size constexpr kBlockAlign = 8u; /* Align of a new block */
    Size constexpr kNewBlockRequired /* Required size for a new block */
        = kMetaSize + kMinSize;

    /* Bit mask for block size */
    struct BSM
    {
        static Size constexpr kIsAlloc = 0x80000000u; /* block is allocated  */
        static Size constexpr kAlign   = 0x0F000000u; /* block payload align */
        static Size constexpr kSize    = 0x00FFFFFFu; /* block payload size  */

        static constexpr Size toShift(Size mask)
        {
            if (mask == 0u) return 0u;

            Size shift = 0u;

            while ((mask & 1u) == 0u)
            {
                mask >>= 1u;
                shift++;
            }

            return shift;
        }
    };

    // ====================================================
    //  Private functions
    // ====================================================

    /* Return true if the block is free, otherwise means allocated. */
    bool constexpr isFreeBlock(Meta const &m) noexcept
    {
        return ((m.size & BSM::kIsAlloc) != BSM::kIsAlloc);
    }

    /* Calculate margin to align the payload. */
    Size constexpr calcNewBlockMargin(Addr addr, Size align) noexcept
    {
        if (align <= kBlockAlign) return 0u; // Already aligned

        /* Compute payload alignment offset. */
        Size const align_mask = align - 1u;
        Size const block_offset = addr & align_mask;
        Size const payload_offset = (block_offset + kMetaSize) & align_mask;

        if (payload_offset == 0u) return 0u; // Already aligned

        /* Smallest forward shift that aligns the payload, */
        /* ensuring room for a lead block.                 */
        Size const margin = (align * 2 - payload_offset) & align_mask;
        return (margin < kNewBlockRequired) ? margin + align : margin;
    }

    /* Calculate padding to align an address. */
    Size constexpr calcAlignPadding(Addr addr, Size align) noexcept
    {
        Addr const align_mask = align - 1u;
        return (align - (addr & align_mask)) & align_mask;
    };

    /* Verify alignment. */
    bool constexpr isIllegalAlign(Size align) noexcept
    {
        return (
            /* Too small    */(align == 0u) ||
            /* Too large    */(align > alignof(std::max_align_t)) ||
            /* Not pow of 2 */(align & (align - 1u)));
    }

    /* Return only valid metadata, otherwise panic. */
    Meta requireValid(Meta *p) noexcept
    {
        if (!p)
        {
            pal::panic("Heap::requireValid: null");
            __builtin_unreachable();
        }

        Meta &m = *p;
        Size const size = m.size & BSM::kSize;

        if (size < kMinSize)
        {
            pal::panic("Heap::requireValid: too small size");
            __builtin_unreachable();
        }

        if (size > kMaxSize)
        {
            pal::panic("Heap::requireValid: too large size");
            __builtin_unreachable();
        }

        if (size > kMaxAddr - reinterpret_cast<Addr>(p))
        {
            pal::panic("Heap::requireValid: overflow");
            __builtin_unreachable();
        }

        return m;
    }
} // namespace

// ====================================================
//  Public functions
// ====================================================

pal::Heap::Heap(pal::PsramRegion const region) noexcept
    : root_(new Meta(), [](void *p) { delete static_cast<Meta *>(p); })
{
    pal::PsramRegion const &r = region; // Alias
    Size const pad = calcAlignPadding(r.base, kBlockAlign);

    /* Early return if illegal argument */
    {
        bool overflow = /* BOA */(r.base > kMaxAddr - pad) ||
                        /* EOA */(r.size > kMaxAddr - pad - r.base);
        bool too_small = (r.size < (pad + kNewBlockRequired));

        if (overflow || too_small) return;
    }

    Size const free_bytes = r.size - pad - kMetaSize;

    free_bytes_ = free_bytes;
    lowest_ever_free_bytes_ = free_bytes;

    Meta *meta = reinterpret_cast<Meta *>(r.base + pad);
    reinterpret_cast<Meta *>(root_.get())->next = meta;
    *meta = {.next = nullptr, .size = free_bytes};
}

void *pal::Heap::allocate(std::size_t size, std::size_t align) noexcept
{
    /* Panic if illegal arguments */
    if (isIllegalAlign(align))
    {
        pal::panic("Heap::allocate: illegal alignment specified");
        __builtin_unreachable();
    }
    if (Size max = kMaxSize & ~(align - 1u); !size || (size > max))
    {
        pal::panic("Heap::allocate: illegal size specified");
        __builtin_unreachable();
    }

    /* Calculate the request size with alignment */
    Size const request_size = (size + (align - 1u)) & ~(align - 1u);

    /* <<<<< Begin critical section */
    enterCriticalSection();

    void *ret = nullptr;
    Meta *cur = reinterpret_cast<Meta *>(root_.get())->next;

    /* Discovery the free block */
    while (cur)
    {
        /* Copy to reduce access to real memory */
        Meta const src = requireValid(cur);

        Addr const addr = reinterpret_cast<Addr>(cur);
        Size const head_gap = calcNewBlockMargin(addr, align);
        Size const required = head_gap + request_size;

        /* Early continue if the requirements are not met */
        if (!isFreeBlock(src) || (src.size < required))
        {
            cur = cur->next;
            continue;
        }

        Size consumed = 0u; // To be consumed free space bytes
        Meta *lead = nullptr, *alloc = nullptr, *trail = nullptr;

        /* Add a trailing free block */
        Size const trail_offset = kMetaSize + required;
        Size const trail_gap =
            calcAlignPadding(addr + trail_offset, kBlockAlign);

        if ((trail_gap <= kMaxAddr - addr - trail_offset) &&
            (src.size >= trail_offset + trail_gap + kNewBlockRequired))
        {
            Size const trail_size = src.size - trail_offset - trail_gap;
            trail = reinterpret_cast<Meta *>(addr + trail_offset + trail_gap);
            *trail = {.next = src.next, .size = trail_size};
            consumed += kMetaSize; // Split meta consumes free space
        }
        Size const slack = trail ? trail_gap : src.size - required;

        /* Allocate */
        {
            Size const alloc_align =
                ((align - 1u) << BSM::toShift(BSM::kAlign)) & BSM::kAlign;
            Size const alloc_size = request_size + slack;
            consumed += alloc_size;
            alloc = reinterpret_cast<Meta *>(addr + head_gap);
            *alloc = {.next = trail ? trail : src.next,
                      .size = BSM::kIsAlloc | alloc_align | alloc_size};
        }

        /* Add a leading free block */
        if (head_gap != 0u)
        {
            lead = reinterpret_cast<Meta *>(addr);
            *lead = {.next = alloc, .size = head_gap - kMetaSize};
            consumed += kMetaSize; // Split meta consumes free space
        }

        /* Update the free space size */
        {
            if (consumed > free_bytes_)
            {
                pal::panic("Heap::allocate: overflow");
                __builtin_unreachable();
            }

            free_bytes_ -= consumed;

            if (free_bytes_ < lowest_ever_free_bytes_)
            {
                lowest_ever_free_bytes_ = free_bytes_;
            }
        }

        ret = reinterpret_cast<void *>(alloc + 1u);
        break;
    }

    if (!ret) /* OOM */
    {
        pal::panic("Heap::allocate: out of memory");
        __builtin_unreachable();
    }

    leaveCriticalSection();
    /* >>>>> End critical section */

    return ret;
}

void pal::Heap::deallocate(void const *p) noexcept
{
    /* Early return if null */
    if (!p) return;

    Addr const payload_addr = reinterpret_cast<Addr>(p);

    /* Fail safe */
    if (payload_addr < kMetaSize)
    {
        pal::panic("Heap::deallocate: too small address");
        __builtin_unreachable();
    }

    /* <<<<< Begin critical section */
    enterCriticalSection();

    Meta const *const target =
        reinterpret_cast<Meta *>(payload_addr - kMetaSize);
    Meta *pre = nullptr;
    Meta *cur = reinterpret_cast<Meta *>(root_.get())->next;

    /* Discovery the allocated block */
    while (cur)
    {
        /* Early continue if no match */
        if (cur != target)
        {
            pre = cur;
            cur = cur->next;
            continue;
        }

        /* Copy to reduce access to real memory */
        Meta const src = requireValid(cur);

        /* Fail safe */
        if (isFreeBlock(src))
        {
            pal::panic("Heap::deallocate: already freed");
            __builtin_unreachable();
        }

        Size released = src.size & BSM::kSize;
        Meta *merged_next = src.next;
        Size merged_size = released;
        Addr merged_addr = reinterpret_cast<Addr>(cur);

        /* Combine previous free block */
        if (pre)
        {
            if (Meta m = requireValid(pre); isFreeBlock(m))
            {
                merged_addr = reinterpret_cast<Addr>(pre);
                merged_size += kMetaSize + m.size;
                released += kMetaSize; // Merged meta becomes free space
            }
        }

        /* Combine next free block */
        if (src.next)
        {
            if (Meta m = requireValid(src.next); isFreeBlock(m))
            {
                merged_next = m.next;
                merged_size += kMetaSize + m.size;
                released += kMetaSize; // Merged meta becomes free space
            }
        }

        /* Apply new free block */
        *reinterpret_cast<Meta *>(merged_addr) =
            {.next = merged_next, .size = merged_size};

        /* Update the free space size */
        if (released > kMaxAddr - free_bytes_)
        {
            pal::panic("Heap::deallocate: overflow");
            __builtin_unreachable();
        }
        free_bytes_ += released;

        break;
    }

    if (!cur) /* No Found */
    {
        pal::panic("Heap::deallocate: invalid address");
        __builtin_unreachable();
    }

    leaveCriticalSection();
    /* >>>>> End critical section */
}

std::size_t pal::Heap::getFreeBytes(void) const noexcept
{
    /* <<<<< Begin critical section */
    pal::enterCriticalSection();
    size_t ret = free_bytes_;
    pal::leaveCriticalSection();
    /* >>>>> End critical section */
    return ret;
}

std::size_t pal::Heap::getLowestEverFreeBytes(void) const noexcept
{
    /* <<<<< Begin critical section */
    pal::enterCriticalSection();
    size_t ret = lowest_ever_free_bytes_;
    pal::leaveCriticalSection();
    /* >>>>> End critical section */
    return ret;
}

void pal::Heap::dumpMemoryMap(void) const noexcept
{
    /* <<<<< Begin critical section */
    pal::enterCriticalSection();

    Meta *cur = reinterpret_cast<Meta *>(root_.get())->next;

    printf("| Stat | Head addr  | Begin addr | End addr   "
           "| Size     | Align |\n");
    printf("| :--- | :--------- | :--------- | :--------- "
           "| -------: | ----: |\n");
    while (cur)
    {
        Meta const src = *cur;
        Addr const addr = reinterpret_cast<Addr>(cur);
        Addr const begin = addr + kMetaSize;
        Size const size = src.size & BSM::kSize;
        Addr const end = begin + size - 1u;

        if (isFreeBlock(src))
        {
            printf("|      | 0x%08X | 0x%08X | 0x%08X "
                   "| %8zu |       |\n", addr, begin, end, size);
        }
        else
        {
            Size const align =
                ((src.size & BSM::kAlign) >> BSM::toShift(BSM::kAlign)) + 1u;
            printf("| Used | 0x%08X | 0x%08X | 0x%08X "
                   "| %8zu |     %zu |\n", addr, begin, end, size, align);
        }
        cur = src.next;
    }

    pal::leaveCriticalSection();
    /* >>>>> End critical section */
}
