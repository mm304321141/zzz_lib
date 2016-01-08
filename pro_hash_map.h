#pragma once

#include "pro_hash.h"


template<class key_t, class value_t, class hasher_t, class key_equal_t, class allocator_t>
struct pro_hash_map_config_t
{
    typedef key_t key_type;
    typedef value_t mapped_type;
    typedef std::pair<key_t const, value_t> value_type;
    typedef std::pair<key_t, value_t> storage_type;
    typedef hasher_t hasher;
    typedef key_equal_t key_equal;
    typedef allocator_t allocator_type;
    typedef std::uintptr_t offset_type;
    typedef decltype(hasher_t()(key_t())) hash_value_type;
    template<class in_type> static key_type const &get_key(in_type &&value)
    {
        return value.first;
    }
};
template<class key_t, class value_t, class hasher_t = std::hash<key_t>, class key_equal_t = std::equal_to<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
using pro_hash_map = pro_hash<pro_hash_map_config_t<key_t, value_t, hasher_t, key_equal_t, allocator_t>>;