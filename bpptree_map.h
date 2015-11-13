#pragma once

#include "bpptree.h"


template<class key_t, class value_t, class unique_t, class comparator_t, class allocator_t>
struct bpptree_map_config_t
{
    typedef key_t key_type;
    typedef value_t mapped_type;
    typedef std::pair<key_t const, value_t> value_type;
    typedef std::pair<key_t, value_t> storage_type;
    typedef comparator_t key_compare;
    typedef allocator_t allocator_type;
    typedef unique_t unique_type;
    typedef std::false_type status_type;
    template<class in_type> static key_type const &get_key(in_type &&value)
    {
        return value.first;
    }
    template<size_t A, size_t B> struct max_t
    {
        enum
        {
            value = A > B ? A : B
        };
    };
    enum
    {
        min_inner_size = (sizeof(key_type) + sizeof(nullptr)) * 8 + sizeof(size_t) * 3 + sizeof(nullptr) * 2,
        min_leaf_size = sizeof(storage_type) * 8 + sizeof(size_t) * 2 + sizeof(nullptr) * 3,
        memory_block_size = max_t<256, max_t<min_inner_size, min_leaf_size>::value>::value,
    };
};
template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
using bpptree_map = b_plus_plus_tree<bpptree_map_config_t<key_t, value_t, std::true_type, comparator_t, allocator_t>>;
template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
using bpptree_multimap = b_plus_plus_tree<bpptree_map_config_t<key_t, value_t, std::false_type, comparator_t, allocator_t>>;