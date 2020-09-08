#pragma once

#include "jaws/assume.hpp"
#include <vector>
#include <utility>

namespace jaws::util {


template <typename ElementType, typename NumericIdType = uint64_t, size_t generation_bits = 16, size_t index_bits = 32>
class Pool
{
    static_assert(std::is_integral_v<NumericIdType>, "!");
    static_assert(64 >= 16 + 32, "!!!");
    static_assert(sizeof(NumericIdType) * 8 >= generation_bits + index_bits, "!");
    static_assert(index_bits > 0, "!");

public:
    // using Id = PoolId<NumericIdType, generation_bits, index_bits>;
    struct Id
    {
        Id() noexcept = default;
        Id(NumericIdType id) noexcept : _id(id) {}
        Id(NumericIdType generation, NumericIdType index) noexcept : _id((generation << index_bits) | (index + 1)) {}

        bool is_valid() const { return _id != 0; }

        static constexpr NumericIdType GENERATION_MASK = (static_cast<NumericIdType>(1) << generation_bits) - 1;
        static constexpr NumericIdType INDEX_MASK = (static_cast<NumericIdType>(1) << index_bits) - 1;

        NumericIdType get_generation() const noexcept { return (_id >> index_bits) & GENERATION_MASK; }
        NumericIdType get_index() const noexcept { return (_id & INDEX_MASK) - 1; }

        friend bool operator==(Id a, Id b) noexcept { return a._id == b._id; }
        friend bool operator!=(Id a, Id b) noexcept { return !(a == b); }

        NumericIdType _id = 0;
    };

    static constexpr size_t MAX_ELEM_COUNT = static_cast<size_t>(
        (static_cast<NumericIdType>(1) << index_bits) - 1); // Subtract 1 b/c index 0 is used for invalid id.

    Pool(NumericIdType initial_capacity = 0, float growth_factor = 1.5);
    Pool(const Pool &) = delete;
    Pool &operator=(const Pool &) = delete;
    ~Pool();

    template <typename... Args>
    Id emplace(Args &&... args);

    // I've tried turning the two insert overloads into one function
    // using forward references, but it seemed to me to get unnecessarily
    // convoluted and hard to maintain.
    Id insert(ElementType &&elem);
    Id insert(const ElementType &elem);

    void clear();
    bool remove(Id);

    ElementType *lookup(Id);
    const ElementType *lookup(Id) const;

    bool empty() const;
    size_t get_element_count() const;
    size_t size() const { return get_element_count(); }

    size_t get_capacity() const;

private:
    struct PoolSlot
    {
        NumericIdType generation = 0;
        bool holds_element = false;

        // union: bad because it doesn't handle destructors.
        // std::variant: how can we handle the case of ElementType being uint32_t,
        // the same as its next-free-index variant type? std::variant only allows
        // to query for the used element if the types are distinct.
        // So I'm settling for this.. it's very low-level, but can be encapsulated
        // nicely here.
        std::array<uint8_t, (sizeof(ElementType) > sizeof(size_t) ? sizeof(ElementType) : sizeof(size_t))> data;

        size_t &as_next_free_index()
        {
            JAWS_ASSUME(!holds_element);
            return *reinterpret_cast<size_t *>(data.data());
        }

        ElementType *as_element()
        {
            JAWS_ASSUME(holds_element);
            return reinterpret_cast<ElementType *>(data.data());
        }

        const ElementType *as_element() const
        {
            JAWS_ASSUME(holds_element);
            return reinterpret_cast<const ElementType *>(data.data());
        }

        PoolSlot() = default;
        ~PoolSlot() { cleanup(); }
        PoolSlot(const PoolSlot &) = delete;
        PoolSlot &operator=(const PoolSlot &) = delete;
        PoolSlot(PoolSlot &&) = default;
        PoolSlot &operator=(PoolSlot &&) = default;

        void cleanup()
        {
            if (holds_element) { as_element()->~ElementType(); }
            holds_element = 0;
        }
    };

    // Returns the index or 0 if not possible.
    size_t alloc_pool_slot();

    size_t _element_count = 0;
    float _growth_factor = 1.5f;
    size_t _first_free_elem = -1;
    std::vector<PoolSlot> _pool_slots;
};

}

#include "pool.inl"
