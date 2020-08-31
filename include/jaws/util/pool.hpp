#pragma once

#include "jaws/assume.hpp"
#include <vector>
#include <utility>

namespace jaws::util {


// TODO: check incoming ids against pool id. might also assign pool ids automatically.

template <typename ElementType>
class Pool
{
public:
    Pool(uint16_t pool_id, uint32_t initial_capacity, float growth_factor = 1.5);

    using element_type = ElementType;

    template <typename... Args>
    uint64_t emplace(Args &&... args);

    // I've tried turning the two insert overloads into one function
    // using forward references, but it seemed to me to get unnecessarily
    // convoluted and hard to maintain.
    uint64_t insert(ElementType &&elem);
    uint64_t insert(const ElementType &elem);

    bool remove(uint64_t id);

    ElementType *lookup(uint64_t id);
    const ElementType *lookup(uint64_t id) const;

    bool empty() const { return get_element_count() == 0; }
    uint32_t get_element_count() const;

    uint32_t get_capacity() const;

private:
    struct PoolSlot
    {
        uint16_t generation : 15;
        uint16_t holds_element : 1;

        // union: bad because it doesn't handle destructors.
        // std::variant: how can we handle the case of ElementType being uint32_t, the same as its next-free-index
        // variant type?
        // Hence this implementation: TODO

        std::array<uint8_t, (sizeof(ElementType) > 4 ? sizeof(ElementType) : 4)> data;

        uint32_t &next_free_index()
        {
            JAWS_ASSUME(!holds_element);
            return *reinterpret_cast<uint32_t *>(data.data());
        }

        ElementType *element()
        {
            JAWS_ASSUME(holds_element);
            return reinterpret_cast<ElementType *>(data.data());
        }

        const ElementType *element() const
        {
            JAWS_ASSUME(holds_element);
            return reinterpret_cast<const ElementType *>(data.data());
        }

        PoolSlot() : generation(0), holds_element(0) {}
        ~PoolSlot() { cleanup(); }
        PoolSlot(const PoolSlot &) = delete;
        PoolSlot &operator=(const PoolSlot &) = delete;
        PoolSlot(PoolSlot &&) = default;
        PoolSlot &operator=(PoolSlot &&) = default;

        void cleanup()
        {
            if (holds_element) { element()->~ElementType(); }
            holds_element = 0;
        }
    };

    uint32_t alloc_pool_slot(); //< Returns the index.

    static constexpr uint32_t extract_index(uint64_t id) { return id & 0xffffffff; }
    static constexpr uint16_t extract_generation(uint64_t id) { return (id >> 32) & 0x7fff; }
    static constexpr uint16_t extract_pool_id(uint64_t id) { return (id >> 48) & 0xffff; }

    static constexpr uint16_t next_generation(uint16_t gen) { return (gen + 1) % 0x8000; }
    constexpr uint64_t make_id(uint32_t index, uint16_t gen)
    {
        return (static_cast<uint64_t>(_pool_id) << 48) | (static_cast<uint64_t>(gen) << 32) | index;
    }

    uint16_t _pool_id;
    uint32_t _element_count;
    float _growth_factor;
    uint32_t _first_free_elem;
    std::vector<PoolSlot> _pool_slots;
};

}

#include "pool.inl"
