#pragma once

#include "pro_hash.h"


template<class key_t, class hasher_t, class key_equal_t, class allocator_t>
struct pro_hash_set_config_t
{
    typedef key_t key_type;
    typedef key_t mapped_type;
    typedef key_t value_type;
    typedef hasher_t hasher;
    typedef key_equal_t key_equal;
    typedef allocator_t allocator_type;
    typedef std::uintptr_t offset_type;
    typedef decltype(hasher()(key_type())) hash_value_type;
    template<class in_type> static key_type const &get_key(in_type &&value)
    {
        return value;
    }
};
template<class key_t, class hasher_t = std::hash<key_t>, class key_equal_t = std::equal_to<key_t>, class allocator_t = std::allocator<key_t>>
using pro_hash_set = pro_hash<pro_hash_set_config_t<key_t, hasher_t, key_equal_t, allocator_t>>;