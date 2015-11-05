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
    typedef key_t storage_type;
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
    typedef typename config_t::storage_type storage_type;
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
            max = ((config_t::memory_block_size - sizeof(node_t) - sizeof(nullptr)) / (sizeof(key_type) + sizeof(nullptr))),
            min = max / 2,
        };
        node_t *children[max + 1];
        key_type item[max];

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
        typedef storage_type item_type;
        enum
        {
            max = (config_t::memory_block_size - sizeof(node_t) - sizeof(nullptr) * 2) / sizeof(value_type),
            min = max / 2,
        };
        value_node_t *prev;
        value_node_t *next;
        storage_type item[max];

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
            parent = nullptr;
            size = 0;
            level = 0;
            used = 0;
            left = right = nullptr;
        }
        value_node_t *left;
        value_node_t *right;
    };
    typedef std::pair<value_node_t *, size_type> pair_pos_t;
public:
    class iterator
    {
    public:
        iterator(value_node_t *, size_type)
        {
        }
    };
    typedef typename std::conditional<config_t::unique_t::value, std::pair<iterator, bool>, iterator>::type insert_result_t;
    typedef std::pair<iterator, bool> pair_ib_t;
    typedef std::pair<pair_pos_t, bool> pair_posi_t;
protected:
    template<class unique_t> typename std::enable_if<unique_t::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return std::make_pair(iterator(posi.first.first, posi.first.second), posi.second);
    }
    template<class unique_t> typename std::enable_if<!unique_t::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return iterator(posi.first.first, posi.first.second);
    }
public:;

    b_plus_size_tree() : root_(key_compare(), allocator_type())
    {
        static_assert(tree_node_t::max >= 4, "low memory_block_size");
        static_assert(value_node_t::max >= 4, "low memory_block_size");
    }

    insert_result_t insert(value_type const &value)
    {
        return result_<config_t::unique_t>(insert_nohint_(value));
    }

    bool debug_check()
    {
        if(root_.parent != nullptr && root_.parent->level > 0)
        {
            return debug_check_(root_.parent);
        }
        return true;
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

    bool debug_check_(node_t *node)
    {
        for(size_type i = 0; i < node->used; ++i)
        {
            tree_node_t *tree_node = static_cast<tree_node_t *>(node);
            node_t *child = tree_node->children[i];
            if(child->level > 0)
            {
                if(!debug_check_(child))
                {
                    return false;
                }
                do
                {
                    child = static_cast<tree_node_t *>(child)->children[child->used];
                }
                while(child->level != 0);
            }
            if(get_key_(static_cast<value_node_t *>(child)->item[child->used - 1]) != tree_node->item[i])
            {
                return false;
            }
        }
        return true;
    }

    tree_node_t *alloc_tree_node_()
    {
        node_allocator_t &allocator(root_);
        tree_node_t *node = reinterpret_cast<tree_node_t *>(allocator.allocate(1));
        node->parent = nullptr;
        node->size = 0;
        node->level = 1;
        node->used = 0;
        return node;
    }

    void dealloc_tree_node_(tree_node_t *node)
    {
        for(size_t i = 0; i < node->used + 1; ++i)
        {
            node->key[i].~key_type();
        }
        node_allocator_t &allocator(root_);
        allocator.deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }

    value_node_t *alloc_value_node_(node_t *parent)
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

    void dealloc_value_node_(value_node_t *node)
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
            dealloc_value_node_(static_cast<value_node_t *>(node));
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
            dealloc_tree_node_(leaf_node);
        }
    }

    template<class in_item_t> typename std::enable_if<std::is_same<in_item_t, key_type>::value, key_type const &>::type get_key_(in_item_t const &item) const
    {
        return item;
    }
    template<class in_item_t> typename std::enable_if<std::is_same<in_item_t, storage_type>::value && !std::is_same<key_type, storage_type>::value, key_type const &>::type get_key_(in_item_t const &item) const
    {
        return config_t::get_key(item);
    }

    template<typename node_type> size_type lower_bound_(node_type const *node, key_type const &key) const
    {
        node_type::item_type const *begin = node->item, *const end = node->item + node->used;
        while(begin != end && get_comparator_()(get_key_(*begin), key))
        {
            ++begin;
        }
        return begin - node->item;
    }

    template<typename node_type> size_type upper_bound_(node_type const *node, key_type const &key) const
    {
        node_type::item_type const *begin = node->item, *const end = node->item + node->used;
        while(begin != end && !get_comparator_()(key, get_key_(*begin)))
        {
            ++begin;
        }
        return begin - node->item;
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

    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        typedef typename std::iterator_traits<iterator_from_t>::value_type iterator_value_t;
        if(std::is_scalar<iterator_value_t>::value)
        {
            std::memcpy(std::addressof(*to_begin), std::addressof(*move_begin), (move_end - move_begin) * sizeof(iterator_value_t));
        }
        else
        {
            for(; move_begin != move_end; ++move_begin)
            {
                construct_one_(to_begin++, std::move(*move_begin));
                destroy_one_(move_begin);
            }
        }
    }

    template<class iterator_t, class in_value_t> void construct_one_(iterator_t where, in_value_t &&value)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        ::new(std::addressof(*where)) iterator_value_t(std::forward<in_value_t>(value));
    }

    template<class iterator_t> void destroy_one_(iterator_t where)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        where->~iterator_value_t();
    }

    template<class iterator_t, class in_value_t> void move_next_and_insert_one_(iterator_t move_begin, iterator_t move_end, in_value_t &&value)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        if(std::is_scalar<iterator_value_t>::value)
        {
            std::memmove(std::addressof(*(move_begin + 1)), std::addressof(*move_begin), (move_end - move_begin) * sizeof(iterator_value_t));
            *move_begin = std::forward<in_value_t>(value);
        }
        else
        {
            if(move_begin == move_end)
            {
                construct_one_(move_begin, std::forward<in_value_t>(value));
            }
            else
            {
                iterator_t to_end = std::next(move_end);
                construct_one_(--to_end, std::move(*--move_end));
                while(move_begin != move_end)
                {
                    *--to_end = std::move(*--move_end);
                }
                *move_begin = std::forward<in_value_t>(value);
            }
        }
    }

    template<class in_value_t> pair_posi_t insert_nohint_(in_value_t &&value)
    {
        node_t *new_child = nullptr;
        key_type const *key_ptr;
        if(root_.parent == nullptr)
        {
            root_.parent = root_.left = root_.right = alloc_value_node_(&root_);
        }
        pair_posi_t result = insert_descend(root_.parent, std::forward<in_value_t>(value), key_ptr, new_child);
        if(new_child != nullptr)
        {
            tree_node_t *new_root = alloc_tree_node_();
            new_root->level = root_.parent->level + 1;
            construct_one_(new_root->item, *key_ptr);
            new_root->children[0] = root_.parent;
            new_root->children[1] = new_child;
            new_root->used = 1;
            root_.parent = new_root;
        }
        return result;
    }

    void split_tree_node_(tree_node_t *tree_node, key_type const *&key_ptr, node_t *&new_node, size_type where)
    {
        size_type mid = (tree_node->used >> 1);
        if(where <= mid && mid > tree_node->used - (mid + 1))
        {
            --mid;
        }
        tree_node_t *new_tree_node = alloc_tree_node_();
        new_tree_node->level = tree_node->level;
        new_tree_node->used = half_size_t(tree_node->used - (mid + 1));
        move_construct_and_destroy_(tree_node->item + mid + 1, tree_node->item + tree_node->used, new_tree_node->item);
        destroy_one_(tree_node->item + mid);
        std::copy(tree_node->children + mid + 1, tree_node->children + tree_node->used + 1, new_tree_node->children);
        tree_node->used = half_size_t(mid);
        node_t *child = tree_node->children[mid];
        while(child->level != 0)
        {
            child = static_cast<tree_node_t *>(child)->children[child->used];
        }
        key_ptr = &get_key_(static_cast<value_node_t *>(child)->item[child->used - 1]);
        new_node = new_tree_node;
    }
    void split_value_node_(value_node_t *value_node, key_type const *&key_ptr, node_t *&new_node)
    {
        size_type mid = (value_node->used >> 1);
        value_node_t *new_value_node = alloc_value_node_(nullptr);
        new_value_node->used = half_size_t(value_node->used - mid);
        new_value_node->next = value_node->next;
        if(new_value_node->next == nullptr)
        {
            root_.right = new_value_node;
        }
        else
        {
            new_value_node->next->prev = new_value_node;
        }
        move_construct_and_destroy_(value_node->item + mid, value_node->item + value_node->used, new_value_node->item);
        value_node->used = half_size_t(mid);
        value_node->next = new_value_node;
        new_value_node->prev = value_node;
        key_ptr = &get_key_(value_node->item[value_node->used - 1]);
        new_node = new_value_node;
    }
    template<class in_value_t> pair_posi_t insert_descend(node_t *node, in_value_t &&value, key_type const *&split_key, node_t *&split_node)
    {
        if(node->level > 0)
        {
            tree_node_t *tree_node = static_cast<tree_node_t *>(node);
            node_t *new_child = nullptr;
            key_type const *key_ptr;
            size_type where = upper_bound_(tree_node, get_key_(value));
            pair_posi_t result = insert_descend(tree_node->children[where], std::forward<in_value_t>(value), key_ptr, new_child);
            if(new_child != nullptr)
            {
                if(tree_node->is_full())
                {
                    split_tree_node_(tree_node, split_key, split_node, where);
                    if(where == tree_node->used + 1 && tree_node->used < split_node->used)
                    {
                        tree_node_t *split_tree_node = static_cast<tree_node_t *>(split_node);
                        construct_one_(tree_node->item + tree_node->used, *split_key);
                        tree_node->children[tree_node->used + 1] = split_tree_node->children[0];
                        tree_node->used++;
                        split_tree_node->children[0] = new_child;
                        split_key = key_ptr;
                        return result;
                    }
                    else if(where >= size_type(tree_node->used + 1))
                    {
                        where -= tree_node->used + 1;
                        tree_node = static_cast<tree_node_t *>(split_node);
                    }
                }
                move_next_and_insert_one_(tree_node->item + where, tree_node->item + tree_node->used, *key_ptr);
                std::move_backward(tree_node->children + where, tree_node->children + tree_node->used + 1, tree_node->children + tree_node->used + 2);

                tree_node->children[where + 1] = new_child;
                tree_node->used++;
            }
            return result;
        }
        else
        {
            value_node_t *value_node = static_cast<value_node_t *>(node);
            size_type where = upper_bound_(value_node, get_key_(value));
            if(config_t::unique_t::value && (where > 0 || value_node->prev != nullptr))
            {
                if(where == 0)
                {
                    value_node_t *prev_value_node = value_node->prev;
                    if(!get_comparator_()(get_key_(prev_value_node->item[prev_value_node->used - 1]), get_key_(value)))
                    {
                        return std::make_pair(std::make_pair(prev_value_node, prev_value_node->used - 1), false);
                    }
                }
                else
                {
                    if(!get_comparator_()(get_key_(value_node->item[where - 1]), get_key_(value)))
                    {
                        return std::make_pair(std::make_pair(value_node, where - 1), false);
                    }
                }
            }
            if(value_node->is_full())
            {
                split_value_node_(value_node, split_key, split_node);
                if(where >= value_node->used)
                {
                    where -= value_node->used;
                    value_node = static_cast<value_node_t *>(split_node);
                }
                else
                {
                    ++split_key;
                }
            }
            move_next_and_insert_one_(value_node->item + where, value_node->item + value_node->used, std::forward<in_value_t>(value));
            value_node->used++;

            if(split_node != nullptr && value_node != split_node && where == value_node->used - 1)
            {
                split_key = &get_key_(value_node->item[where]);
            }
            return std::make_pair(std::make_pair(value_node, where), true);
        }
    }
};
