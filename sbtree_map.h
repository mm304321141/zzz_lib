#pragma once

#include "sbtree.h"

template<class key_t, class value_t, class comparator_t, class allocator_t>
struct sbtree_multimap_config_t
{
    static key_t const &get_key(std::pair<key_t const, value_t> const &value)
    {
        return value.first;
    }
    typedef key_t key_type;
    typedef value_t mapped_type;
    typedef std::pair<key_t const, value_t> value_type;
    typedef comparator_t key_compare;
    typedef allocator_t allocator_type;
};

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
using sbtree_multimap = size_balanced_tree<sbtree_multimap_config_t<key_t, value_t, comparator_t, allocator_t>>;
