#pragma once

#include "sbtree.h"

template<class key_t, class unique_t, class comparator_t, class allocator_t>
struct sbtree_set_config_t
{
    typedef key_t key_type;
    typedef key_t const mapped_type;
    typedef key_t const value_type;
    typedef comparator_t key_compare;
    typedef allocator_t allocator_type;
    typedef unique_t unique_type;
    // Default index/size type used by the underlying size_balanced_tree.
    // Users can derive from this struct (or specialize the alias below) and
    // override size_type with a narrower type (e.g. uint32_t) to halve the
    // per-node bookkeeping cost when the expected element count is bounded.
    // sbtree.h picks up this typedef via a SFINAE trait and falls back to
    // size_t when absent, so omitting/keeping the field is fully backward
    // compatible.
    typedef size_t size_type;
    template<class in_type> static key_type const &get_key(in_type &&value)
    {
        return value;
    }
};

template<class value_t, class comparator_t = std::less<value_t>, class allocator_t = std::allocator<value_t>>
using sbtree_set = size_balanced_tree<sbtree_set_config_t<value_t, std::true_type, comparator_t, allocator_t>>;

template<class value_t, class comparator_t = std::less<value_t>, class allocator_t = std::allocator<value_t>>
using sbtree_multiset = size_balanced_tree<sbtree_set_config_t<value_t, std::false_type, comparator_t, allocator_t>>;
