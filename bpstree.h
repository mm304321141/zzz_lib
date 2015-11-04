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
    struct tree_node_t : public node_t
    {
        typedef key_type item_type;
        enum
        {
            max = ((config_t::memory_block_size - sizeof(node_t) - sizeof(key_type)) / (sizeof(key_type) + sizeof(nullptr))),
            min = max / 2,
        };
        node_t *children[max];
        key_type item[max + 1];

        bool is_full() const
        {
            return node_t::used == max;
        }
        bool is_few() const
        {
            return node_t::used <= min;
        }
        inline bool is_underflow() const
        {
            return node_t::used < min;
        }
    };
    struct value_node_t : public node_t
    {
        typedef value_type item_type;
        enum
        {
            max = (config_t::memory_block_size - sizeof(node_t) - sizeof(nullptr) * 2) / sizeof(value_type),
            min = max / 2,
        };
        node_t *prev;
        node_t *next;
        value_type item[max];

        bool is_full() const
        {
            return node_t::used == max;
        }
        bool is_few() const
        {
            return node_t::used <= min;
        }
        inline bool is_underflow() const
        {
            return node_t::used < min;
        }
    };
    struct memory_node_t
    {
        uint8_t buffer[config_t::memory_block_size];
    };
    typedef typename allocator_type::template rebind<memory_node_t>::other node_allocator_t;
    struct root_node_t : public node_t, public node_allocator_t, public key_compare
    {
        template<class any_key_compare, class any_allocator_t> root_node_t(any_key_compare &&comp, any_allocator_t &&alloc) : key_compare(std::forward<any_key_compare>(comp)), node_allocator_t(std::forward<any_allocator_t>(alloc))
        {
            root_.parent = nullptr;
            root_.size = 0;
            root_.level = 0;
            root_.used = 0;
            left = right = nullptr;
        }
        node_t *left;
        node_t *right;
    };
    typedef std::pair<value_node_t *, size_type> pair_pos_t;
public:
    class iterator
    {
    };
    typedef std::conditional<config_t::unique_t::value, std::pair<iterator, bool>, iterator>::type insert_result_t;
    typedef std::pair<iterator *, size_type> pair_pos_t;
    typedef std::pair<pair_pos_t, bool> pair_posi_t;

    insert_result_t insert(value_type const &value)
    {
        return insert_nohint_(value);
    }

protected:
    root_node_t root_;

protected:
    key_compare &get_comparator_()
    {
        return root_;
    }
    key_compare const &get_comparator_() const
    {
        return root_;
    }
    tree_node_t *alloc_left_()
    {
        node_allocator_t &allocator(root_);
        tree_node_t *node = reinterpret_cast<tree_node_t *>(allocator.allocate(1));
        node->parent = nullptr;
        node->size = 0;
        node->level = 1;
        node->used = 0;
        return node;
    }
    void dealloc_left_(tree_node_t *node)
    {
        for(size_t i = 0; i < node->used + 1; ++i)
        {
            node->key[i].~key_type();
        }
        node_allocator_t &allocator(root_);
        allocator.deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }
    value_node_t *alloc_value_(tree_node_t *parent)
    {
        node_allocator_t &allocator(root_);
        value_node_t *node = reinterpret_cast<value_node_t *>(allocator.allocate(1));
        node->parent = parent;
        node->size = 0;
        node->level = 0;
        node->used = 0;
        node->prev = nullptr;
        node->next = nullptr;
        return node;
    }
    void dealloc_value_(value_node_t *node)
    {
        for(size_t i = 0; i < node->used; ++i)
        {
            node->value[i].~value_type();
        }
        node_allocator_t &allocator(root_);
        allocator.deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }
    template<bool is_recursive>void free_node_(node_t *node)
    {
        if(node->level == 0)
        {
            dealloc_value_(static_cast<value_node_t *>(node));
        }
        else
        {
            tree_node_t *leaf_node = static_cast<tree_node_t *>(node);
            if(is_recursive)
            {
                for(size_t i = 0; i < node->used; ++i)
                {
                    free_node_<is_recursive>(leaf_node->children[i]);
                }
            }
            dealloc_left_(leaf_node);
        }
    }
    key_type const &get_key_(key_type const &key) const
    {
        return key;
    }
    key_type const &get_key_(value_type const &value) const
    {
        return config_t::get_key(value);
    }
    template<typename node_type> size_type lower_bound_(node_type const *node, const key_type& key) const
    {
        return std::lower_bound(node->item, node->item + node->used, [&](node_type::item_type const &item)->bool
        {
            return get_comparator_()(get_key_(item), key);
        }) - node->item;
    }
    template<typename node_type> size_type upper_bound_(node_type const *node, const key_type& key) const
    {
        return std::upper_bound(node->item, node->item + node->used, [&](node_type::item_type const &item)->bool
        {
            return get_comparator_()(get_key_(item), key);
        }) - node->item;
    }
    pair_pos_t lower_bound_(key_type const &key) const
    {
        node_t *node = root_.parent;
        if(node == nullptr)
        {
            return std::make_pair(&root_, 0);
        }
        while(node->level > 0)
        {
            tree_node_t const *leaf_node = static_cast<tree_node_t const *>(node);
            size_type where = lower_bound_(node, key);
            node = leaf_node->children[where];
        }
        value_node_t *value_node = static_cast<value_node_t const *>(node);

        size_type where = lower_bound_(value_node, key);
        return std::make_pair(value_node, where);
    }
    pair_pos_t upper_bound_(key_type const &key) const
    {
        node_t *node = root_.parent;
        if(node == nullptr)
        {
            return std::make_pair(&root_, 0);
        }
        while(node->level > 0)
        {
            tree_node_t const *leaf_node = static_cast<tree_node_t const *>(node);
            size_type where = upper_bound_(node, key);
            node = leaf_node->children[where];
        }
        value_node_t *value_node = static_cast<value_node_t const *>(node);

        size_type where = upper_bound_(value_node, key);
        return std::make_pair(value_node, where);
    }
    template<class in_value_t> pair_posi_t insert_nohint_(in_value_t &&value)
    {

    }
};
