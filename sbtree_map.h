#pragma once

#include "sbtree.h"

template<class key_t, class value_t, class unique_t, class comparator_t, class allocator_t>
struct sbtree_map_config_t
{
    typedef key_t key_type;
    typedef value_t mapped_type;
    typedef std::pair<key_t const, value_t> value_type;
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
        return value.first;
    }
};

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
using sbtree_map = size_balanced_tree<sbtree_map_config_t<key_t, value_t, std::true_type, comparator_t, allocator_t>>;

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
using sbtree_multimap = size_balanced_tree<sbtree_map_config_t<key_t, value_t, std::false_type, comparator_t, allocator_t>>;
