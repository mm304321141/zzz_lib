#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>
#include <type_traits>

template<class key_t, class comparator_t, class allocator_t>
struct bstree_multiset_config_t
{
    static key_t const &get_key(key_t const &value)
    {
        return value;
    }
    typedef key_t key_type;
    typedef key_t const mapped_type;
    typedef key_t const value_type;
    typedef comparator_t key_compare;
    typedef allocator_t allocator_type;
    typedef std::false_type unique_t;
    enum
    {
        memory_block_size = 256
    };
};


template<class config_t>
class b_plus_size_tree
{
public:
    typedef typename config_t::key_type key_type;
    typedef typename config_t::mapped_type mapped_type;
    typedef typename config_t::value_type value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef typename config_t::key_compare key_compare;
    typedef typename config_t::allocator_type allocator_type;
    typedef value_type &reference;
    typedef value_type const &const_reference;
    typedef value_type *pointer;
    typedef value_type const *const_pointer;

    typedef std::conditional<sizeof(size_t) == 4, uint16_t, uint32_t>::type half_size_t;

    struct node_t
    {
        node_t *parent;
        size_t size;
        half_size_t level;
        half_size_t used;
    };
    struct value_node_t : public node_t
    {
        enum
        {
            max = (config_t::memory_block_size - sizeof(node_t) - sizeof(nullptr) * 2) / sizeof(value_type)
        };
        node_t *prev;
        node_t *next;
        value_type[max];
    };
    struct leaf_node_t : public node_t
    {
        enum
        {
            max = ((config_t::memory_block_size - sizeof(node_t) - sizeof(key_type)) / (sizeof(key_type) + sizeof(nullptr)))
        };
        node_t *children[max];
        key_type key[max + 1];
    };
    struct root_node_t : public node_t, public allocator_type, public key_compare
    {
        node_t *left;
        node_t *right;
    };

};

typedef b_plus_size_tree<bstree_multiset_config_t<int, std::less<int>, std::allocator<int>>> tree_t;

int foo()
{
    printf("%d", tree_t::value_node_t::max);
    printf("%d", tree_t::leaf_node_t::max);
}