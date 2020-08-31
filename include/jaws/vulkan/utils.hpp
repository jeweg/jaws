#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/assume.hpp"

#include "fmt/format.h"
#include <string>
#include <algorithm>

namespace jaws::vulkan {

template <typename T, typename FUNC>
T choose_best(const std::vector<T> &candidates, FUNC f)
{
    JAWS_ASSUME(!candidates.empty());
    T best_one = candidates[0];
    int max_weight = std::numeric_limits<int>::min(); // or zero?
    for (auto s : candidates) {
        int weight = f(s);
        if (weight > max_weight) {
            max_weight = weight;
            best_one = s;
        }
    }
    return best_one;
}

}
