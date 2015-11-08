#pragma once

#include "sbtree.h"


template<class key_t, class comparator_t, class allocator_t>
struct sbtree_multiset_config_t
{
    typedef key_t key_type;
    typedef key_t const mapped_type;
    typedef key_t const value_type;
    typedef comparator_t key_compare;
    typedef allocator_t allocator_type;
    static key_type const &get_key(key_type &value)
    {
        return value;
    }
};

template<class value_t, class comparator_t = std::less<value_t>, class allocator_t = std::allocator<value_t>>
using sbtree_multiset = size_balanced_tree<sbtree_multiset_config_t<value_t, comparator_t, allocator_t>>;
