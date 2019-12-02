#pragma once
#include "config.hpp"

namespace commun {


template<typename T>
struct member_pointer_info;

template<typename C, typename V>
struct member_pointer_info<V C::*> {
    using value_type = V;
    using class_type = C;
};

int64_t safe_prop(int64_t arg, int64_t numer, int64_t denom) {
    return !arg || !numer ? 0 : static_cast<int64_t>((static_cast<int128_t>(arg) * numer) / denom);
}

int64_t safe_pct(int64_t arg, int64_t total) {
    return safe_prop(arg, total,  commun::config::_100percent);
}

int64_t mul_cut(int64_t a, int64_t b) {
    static constexpr int128_t max_ret128 = std::numeric_limits<int64_t>::max();
    auto ret128 = static_cast<int128_t>(a) * b;
    return static_cast<int64_t>(std::min(max_ret128, ret128));
}

static constexpr int64_t MAXINT64 = std::numeric_limits<int64_t>::max();

// TODO: can be added to CDT
name get_prefix(name acc) {
    auto tmp = acc.value;
    auto remain_bits = 64;
    for (; remain_bits >= -1; remain_bits-=5) {
        if ((tmp >> 59) == 0) { // 1st symbol is dot
            break;
        }
        tmp <<= 5;
    }
    if (remain_bits == -1) return acc;
    if (remain_bits == 64) return name();
    return name{acc.value >> remain_bits << remain_bits};
}

} // commun
