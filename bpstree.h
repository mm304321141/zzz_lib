#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>
#include <type_traits>


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
    struct inner_node_t : public node_t
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
    struct leaf_node_t : public node_t
    {
        typedef storage_type item_type;
        enum
        {
            max = (config_t::memory_block_size - sizeof(node_t) - sizeof(nullptr) * 2) / sizeof(value_type),
            min = max / 2,
        };
        leaf_node_t *prev;
        leaf_node_t *next;
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
        leaf_node_t *left;
        leaf_node_t *right;
    };
    struct key_stack_t
    {
        uint8_t buffer[sizeof(key_type)];
        key_stack_t()
        {
        }
        key_stack_t(key_stack_t &&key)
        {
            ::new(buffer) key_type(std::move(key.key()));
        }
        key_stack_t(key_stack_t const &key)
        {
            ::new(buffer) key_type(key.key());
        }
        key_stack_t(key_type &&key)
        {
            ::new(buffer) key_type(std::move(key));
        }
        key_stack_t(key_type const &key)
        {
            ::new(buffer) key_type(key);
        }
        operator key_type &()
        {
            return *reinterpret_cast<key_type *>(buffer);
        }
        operator key_type const &() const
        {
            return *reinterpret_cast<key_type const *>(buffer);
        }
        operator key_type &&()
        {
            return std::move(*reinterpret_cast<key_type *>(buffer));
        }
        key_type &key()
        {
            return *reinterpret_cast<key_type *>(buffer);
        }
        key_type const &key() const
        {
            return *reinterpret_cast<key_type const *>(buffer);
        }
        key_type *operator &()
        {
            return reinterpret_cast<key_type *>(buffer);
        }
        key_stack_t &operator = (key_stack_t &&other)
        {
            key() = std::move(other.key());
            return *this;
        }
        key_stack_t &operator = (key_stack_t const &key)
        {
            key() = other.key();
            return *this;
        }
        key_stack_t &operator = (key_type &&other)
        {
            key() = std::move(other);
            return *this;
        }
        key_stack_t &operator = (key_type const &other)
        {
            key() = other;
            return *this;
        }
    };
    enum result_flags_t
    {
        btree_ok = 0,
        btree_not_found = 1,
        btree_update_lastkey = 2,
        btree_fixmerge = 4
    };
    struct result_t
    {
        result_flags_t flags;
        key_stack_t last_key;

        explicit result_t(result_flags_t f = btree_ok) : flags(result_flags_t(f & ~btree_update_lastkey)), last_key()
        {
        }
        result_t(result_t &&other) : flags(other.flags), last_key()
        {
            if(other.has(btree_update_lastkey))
            {
                ::new(&last_key) key_type(std::move(other.last_key.key()));
            }
        }
        template<class in_key_t> result_t(result_flags_t f, in_key_t &&k) : flags(result_flags_t(f | btree_update_lastkey)), last_key(std::forward<in_key_t>(k))
        {
        }
        ~result_t()
        {
            if(has(btree_update_lastkey))
            {
                (&last_key)->~key_type();
            }
        }
        bool has(result_flags_t f) const
        {
            return (flags & f) != 0;
        }
        result_t &operator |= (result_t const &other)
        {
            if(has(btree_update_lastkey))
            {
                last_key.key() = std::move(other.last_key.key());
            }
            else
            {
                ::new(&last_key) key_type(std::move(other.last_key.key()));
            }
            flags = result_flags_t(flags | other.flags);
            return *this;
        }
        result_t &operator |= (result_t &&other)
        {
            if(other.has(btree_update_lastkey))
            {
                if(has(btree_update_lastkey))
                {
                    last_key.key() = std::move(other.last_key.key());
                }
                else
                {
                    ::new(&last_key) key_type(std::move(other.last_key.key()));
                }
            }
            flags = result_flags_t(flags | other.flags);
            return *this;
        }
    };

    typedef std::pair<leaf_node_t *, size_type> pair_pos_t;
public:
    class iterator
    {
    public:
        iterator(leaf_node_t *, size_type)
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
        static_assert(inner_node_t::max >= 4, "low memory_block_size");
        static_assert(leaf_node_t::max >= 4, "low memory_block_size");
    }

    insert_result_t insert(value_type const &value)
    {
        return result_<config_t::unique_t>(insert_nohint_(value));
    }

    size_type erase(key_type const &key)
    {
        size_type count = 0;
        while(erase_one(key))
        {
            ++count;
            if(config_t::unique_t::value)
            {
                break;
            }
        }
        return count;
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
            inner_node_t *inner_node = static_cast<inner_node_t *>(node);
            node_t *child = inner_node->children[i];
            if(child->level > 0)
            {
                if(!debug_check_(child))
                {
                    return false;
                }
                do
                {
                    child = static_cast<inner_node_t *>(child)->children[child->used];
                }
                while(child->level != 0);
            }
            key_type const &key1 = get_key_(static_cast<leaf_node_t *>(child)->item[child->used - 1]);
            key_type const &key2 = inner_node->item[i];
            if(get_comparator_()(key1, key2) || get_comparator_()(key2, key2))
            {
                return false;
            }
        }
        return true;
    }

    inner_node_t *alloc_inner_node_()
    {
        node_allocator_t &allocator(root_);
        inner_node_t *node = reinterpret_cast<inner_node_t *>(allocator.allocate(1));
        node->parent = nullptr;
        node->size = 0;
        node->level = 1;
        node->used = 0;
        return node;
    }

    void dealloc_inner_node_(inner_node_t *node)
    {
        for(half_size_t i = 0; i < node->used; ++i)
        {
            node->item[i].~key_type();
        }
        node_allocator_t &allocator(root_);
        allocator.deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }

    leaf_node_t *alloc_leaf_node_(node_t *parent)
    {
        node_allocator_t &allocator(root_);
        leaf_node_t *node = reinterpret_cast<leaf_node_t *>(allocator.allocate(1));
        node->parent = parent;
        node->size = 0;
        node->level = 0;
        node->used = 0;
        node->prev = nullptr;
        node->next = nullptr;
        return node;
    }

    void dealloc_leaf_node_(leaf_node_t *node)
    {
        for(size_t i = 0; i < node->used; ++i)
        {
            node->item[i].~value_type();
        }
        node_allocator_t &allocator(root_);
        allocator.deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }

    template<bool is_recursive>void free_node_(node_t *node)
    {
        if(node->level == 0)
        {
            dealloc_leaf_node_(static_cast<leaf_node_t *>(node));
        }
        else
        {
            inner_node_t *leaf_node = static_cast<inner_node_t *>(node);
            if(is_recursive)
            {
                for(size_t i = 0; i < node->used; ++i)
                {
                    free_node_<is_recursive>(leaf_node->children[i]);
                }
            }
            dealloc_inner_node_(leaf_node);
        }
    }

    template<class in_item_t> typename std::enable_if<std::is_convertible<in_item_t, key_type>::value, key_type const &>::type get_key_(in_item_t &item) const
    {
        return item;
    }
    template<class in_item_t> typename std::enable_if<!std::is_same<key_type, storage_type>::value && std::is_convertible<in_item_t, storage_type>::value, key_type const &>::type get_key_(in_item_t &item) const
    {
        return config_t::get_key(item);
    }

    template<typename node_type> size_type lower_bound_(node_type const *node, key_type const &key) const
    {
        if(std::is_scalar<key_type>::value)
        {
            node_type::item_type const *begin = node->item, *const end = node->item + node->used;
            while(begin != end && get_comparator_()(get_key_(*begin), key))
            {
                ++begin;
            }
            return begin - node->item;
        }
        else
        {
            return std::lower_bound(node->item, node->item + node->used, key, [&](node_type::item_type const &left, key_type const &right)->bool
            {
                return get_comparator_()(get_key_(left), right);
            }) - node->item;
        }
    }

    template<typename node_type> size_type upper_bound_(node_type const *node, key_type const &key) const
    {
        if(std::is_scalar<key_type>::value)
        {
            node_type::item_type const *begin = node->item, *const end = node->item + node->used;
            while(begin != end && !get_comparator_()(key, get_key_(*begin)))
            {
                ++begin;
            }
            return begin - node->item;
        }
        else
        {
            return std::upper_bound(node->item, node->item + node->used, key, [&](key_type const &left, node_type::item_type const &right)->bool
            {
                return get_comparator_()(left, get_key_(right));
            }) - node->item;
        }
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
            inner_node_t const *leaf_node = static_cast<inner_node_t const *>(node);
            size_type where = lower_bound_(node, key);
            node = leaf_node->children[where];
        }
        leaf_node_t *leaf_node = static_cast<leaf_node_t const *>(node);

        size_type where = lower_bound_(leaf_node, key);
        return std::make_pair(leaf_node, where);
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
            inner_node_t const *leaf_node = static_cast<inner_node_t const *>(node);
            size_type where = upper_bound_(node, key);
            node = leaf_node->children[where];
        }
        leaf_node_t *leaf_node = static_cast<leaf_node_t const *>(node);

        size_type where = upper_bound_(leaf_node, key);
        return std::make_pair(leaf_node, where);
    }

    template<class iterator_t, class in_value_t> static void construct_one_(iterator_t where, in_value_t &&value)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        ::new(std::addressof(*where)) iterator_value_t(std::forward<in_value_t>(value));
    }

    template<class iterator_t> static void destroy_one_(iterator_t where)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        where->~iterator_value_t();
    }

    template<class iterator_t> static void destroy_(iterator_t destroy_begin, iterator_t destroy_end)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        if(!std::is_scalar<iterator_value_t>::value)
        {
            for(; destroy_begin != destroy_end; ++destroy_begin)
            {
                destroy_begin->~iterator_value_t();
            }
        }
    }

    template<class iterator_from_t, class iterator_to_t> static void move_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        std::move(move_begin, move_end, to_begin);
    }

    template<class iterator_from_t, class iterator_to_t> static void move_construct_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
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
            }
        }
    }

    template<class iterator_t> static void move_next_to_and_construct_(iterator_t move_begin, iterator_t move_end, iterator_t to_begin)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        if(std::is_scalar<iterator_value_t>::value)
        {
            std::memmove(std::addressof(*to_begin), std::addressof(*move_begin), (move_end - move_begin) * sizeof(iterator_value_t));
        }
        else
        {
            if(to_begin < move_end)
            {
                iterator_t split = move_end - (to_begin - move_begin);
                move_construct_(split, move_end, move_end);
                move_(move_begin, split, to_begin);
            }
            else
            {
                move_construct_(move_begin, move_end, to_begin);
                while(move_end != to_begin)
                {
                    construct_one_(move_end++, iterator_value_t());
                }
            }
        }
    }

    template<class iterator_from_t, class iterator_to_t> static void move_and_destroy_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
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
                *to_begin++ = std::move(*move_begin);
                destroy_one_(move_begin);
            }
        }
    }

    template<class iterator_from_t, class iterator_to_t> static void move_construct_and_destroy_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
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

    template<class iterator_t, class in_value_t> static void move_next_and_insert_one_(iterator_t move_begin, iterator_t move_end, in_value_t &&value)
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

    template<class iterator_t> static void move_prev_and_destroy_one_(iterator_t move_begin, iterator_t move_end)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        if(std::is_scalar<iterator_value_t>::value)
        {
            (move_begin - 1)->~iterator_value_t();
            std::memmove(std::addressof(*(move_begin - 1)), std::addressof(*move_begin), (move_end - move_begin) * sizeof(iterator_value_t));
        }
        else
        {
            if(move_begin == move_end)
            {
                destroy_one_(move_begin - 1);
            }
            else
            {
                iterator_t to_begin = std::prev(move_begin);
                while(move_begin != move_end)
                {
                    *to_begin++ = std::move(*move_begin++);
                }
                destroy_one_(to_begin);
            }
        }
    }

    template<class in_value_t> pair_posi_t insert_nohint_(in_value_t &&value)
    {
        node_t *new_child = nullptr;
        key_stack_t key_out;
        if(root_.parent == nullptr)
        {
            root_.parent = root_.left = root_.right = alloc_leaf_node_(&root_);
        }
        pair_posi_t result = insert_descend_(root_.parent, std::forward<in_value_t>(value), &key_out, new_child);
        if(new_child != nullptr)
        {
            inner_node_t *new_root = alloc_inner_node_();
            new_root->level = root_.parent->level + 1;
            construct_one_(new_root->item, std::move(key_out.key()));
            destroy_one_(&key_out);
            new_root->children[0] = root_.parent;
            new_root->children[1] = new_child;
            new_root->used = 1;
            root_.parent = new_root;
        }
        if(result.second)
        {
            ++root_.size;
        }
        return result;
    }

    void split_inner_node_(inner_node_t *inner_node, key_type *key_ptr, node_t *&new_node, size_type where)
    {
        size_type mid = (inner_node->used >> 1);
        if(where <= mid && mid > inner_node->used - (mid + 1))
        {
            --mid;
        }
        inner_node_t *new_inner_node = alloc_inner_node_();
        new_inner_node->level = inner_node->level;
        new_inner_node->used = half_size_t(inner_node->used - (mid + 1));
        move_construct_and_destroy_(inner_node->item + mid + 1, inner_node->item + inner_node->used, new_inner_node->item);
        std::copy(inner_node->children + mid + 1, inner_node->children + inner_node->used + 1, new_inner_node->children);
        inner_node->used = half_size_t(mid);
        construct_one_(key_ptr, inner_node->item[mid]);
        destroy_one_(inner_node->item + mid);
        new_node = new_inner_node;
    }

    void split_leaf_node_(leaf_node_t *leaf_node, key_type *key_ptr, node_t *&new_node)
    {
        size_type mid = (leaf_node->used >> 1);
        leaf_node_t *new_leaf_node = alloc_leaf_node_(nullptr);
        new_leaf_node->used = half_size_t(leaf_node->used - mid);
        new_leaf_node->next = leaf_node->next;
        if(new_leaf_node->next == nullptr)
        {
            root_.right = new_leaf_node;
        }
        else
        {
            new_leaf_node->next->prev = new_leaf_node;
        }
        move_construct_and_destroy_(leaf_node->item + mid, leaf_node->item + leaf_node->used, new_leaf_node->item);
        leaf_node->used = half_size_t(mid);
        leaf_node->next = new_leaf_node;
        new_leaf_node->prev = leaf_node;
        construct_one_(key_ptr, get_key_(leaf_node->item[leaf_node->used - 1]));
        new_node = new_leaf_node;
    }

    template<class in_value_t> pair_posi_t insert_descend_(node_t *node, in_value_t &&value, key_type *split_key, node_t *&split_node)
    {
        if(node->level > 0)
        {
            inner_node_t *inner_node = static_cast<inner_node_t *>(node);
            node_t *new_child = nullptr;
            key_stack_t key_out;
            size_type where = upper_bound_(inner_node, get_key_(value));
            pair_posi_t result = insert_descend_(inner_node->children[where], std::forward<in_value_t>(value), &key_out, new_child);
            if(new_child != nullptr)
            {
                if(inner_node->is_full())
                {
                    split_inner_node_(inner_node, split_key, split_node, where);
                    if(where == inner_node->used + 1 && inner_node->used < split_node->used)
                    {
                        inner_node_t *split_tree_node = static_cast<inner_node_t *>(split_node);
                        construct_one_(inner_node->item + inner_node->used, *split_key);
                        inner_node->children[inner_node->used + 1] = split_tree_node->children[0];
                        inner_node->used++;
                        split_tree_node->children[0] = new_child;
                        *split_key = std::move(key_out.key());
                        destroy_one_(&key_out);
                        return result;
                    }
                    else if(where >= size_type(inner_node->used + 1))
                    {
                        where -= inner_node->used + 1;
                        inner_node = static_cast<inner_node_t *>(split_node);
                    }
                }
                move_next_and_insert_one_(inner_node->item + where, inner_node->item + inner_node->used, std::move(key_out.key()));
                destroy_one_(&key_out);
                std::move_backward(inner_node->children + where, inner_node->children + inner_node->used + 1, inner_node->children + inner_node->used + 2);

                inner_node->children[where + 1] = new_child;
                inner_node->used++;
            }
            return result;
        }
        else
        {
            leaf_node_t *leaf_node = static_cast<leaf_node_t *>(node);
            size_type where = upper_bound_(leaf_node, get_key_(value));
            if(config_t::unique_t::value && (where > 0 || leaf_node->prev != nullptr))
            {
                if(where == 0)
                {
                    leaf_node_t *prev_leaf_node = leaf_node->prev;
                    if(!get_comparator_()(get_key_(prev_leaf_node->item[prev_leaf_node->used - 1]), get_key_(value)))
                    {
                        return std::make_pair(std::make_pair(prev_leaf_node, prev_leaf_node->used - 1), false);
                    }
                }
                else
                {
                    if(!get_comparator_()(get_key_(leaf_node->item[where - 1]), get_key_(value)))
                    {
                        return std::make_pair(std::make_pair(leaf_node, where - 1), false);
                    }
                }
            }
            if(leaf_node->is_full())
            {
                split_leaf_node_(leaf_node, split_key, split_node);
                if(where >= leaf_node->used)
                {
                    where -= leaf_node->used;
                    leaf_node = static_cast<leaf_node_t *>(split_node);
                }
            }
            move_next_and_insert_one_(leaf_node->item + where, leaf_node->item + leaf_node->used, std::forward<in_value_t>(value));
            leaf_node->used++;

            if(split_node != nullptr && leaf_node != split_node && where == leaf_node->used - 1)
            {
                *split_key = get_key_(leaf_node->item[where]);
            }
            return std::make_pair(std::make_pair(leaf_node, where), true);
        }
    }

    bool erase_one(key_type const &key)
    {
        if(root_.parent == nullptr)
        {
            return false;
        }
        result_t result = erase_one_descend(key, root_.parent, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
        if(result.has(btree_not_found))
        {
            return false;
        }
        --root_.size;
        return true;
    }

    result_t merge_leaves(leaf_node_t *left, leaf_node_t *right, inner_node_t *parent)
    {
        (void)parent;
        move_construct_and_destroy_(right->item, right->item + right->used, left->item + left->used);
        left->used += right->used;
        left->next = right->next;
        if(left->next != nullptr)
        {
            left->next->prev = left;
        }
        else
        {
            root_.right = left;
        }
        right->used = 0;
        return result_t(btree_fixmerge);
    }

    static result_t shift_left_leaf(leaf_node_t *left, leaf_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (right->used - left->used) >> 1;
        move_construct_(right->item, right->item + shiftnum, left->item + left->used);
        left->used += half_size_t(shiftnum);
        move_(right->item + shiftnum, right->item + right->used, right->item);
        destroy_(right->item + right->used - shiftnum, right->item + right->used);
        right->used -= half_size_t(shiftnum);
        if(parent_where < parent->used)
        {
            parent->item[parent_where] = left->item[left->used - 1];
            return result_t(btree_ok);
        }
        else
        {
            return result_t(btree_update_lastkey, left->item[left->used - 1]);
        }
    }

    static void shift_right_leaf(leaf_node_t *left, leaf_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (left->used - right->used) >> 1;
        move_next_to_and_construct_(right->item, right->item + right->used, right->item + shiftnum);
        right->used += half_size_t(shiftnum);
        move_and_destroy_(left->item + left->used - shiftnum, left->item + left->used, right->item);
        left->used -= half_size_t(shiftnum);
        parent->item[parent_where] = left->item[left->used - 1];
    }

    static result_t merge_inner(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        construct_one_(left->item + left->used, parent->item[parent_where]);
        ++left->used;
        move_construct_and_destroy_(right->item, right->item + right->used, left->item + left->used);
        std::copy(right->children, right->children + right->used + 1, left->children + left->used);
        left->used += right->used;
        right->used = 0;
        return result_t(btree_fixmerge);
    }

    static void shift_left_inner(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (right->used - left->used) >> 1;
        construct_one_(left->item + left->used, parent->item[parent_where]);
        ++left->used;
        move_construct_(right->item, right->item + shiftnum - 1, left->item + left->used);
        std::copy(right->children, right->children + shiftnum, left->children + left->used);
        left->used += half_size_t(shiftnum - 1);
        parent->item[parent_where] = right->item[shiftnum - 1];
        move_(right->item + shiftnum, right->item + right->used, right->item);
        destroy_(right->item + right->used - shiftnum, right->item + right->used);
        std::copy(right->children + shiftnum, right->children + right->used + 1, right->children);
        right->used -= half_size_t(shiftnum);
    }

    static void shift_right_inner(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (left->used - right->used) >> 1;
        move_next_to_and_construct_(right->item, right->item + right->used, right->item + shiftnum);
        std::copy_backward(right->children, right->children + right->used + 1, right->children + right->used + 1 + shiftnum);
        right->used += half_size_t(shiftnum);
        right->item[shiftnum - 1] = parent->item[parent_where];
        move_and_destroy_(left->item + left->used - shiftnum + 1, left->item + left->used, right->item);
        std::copy(left->children + left->used - shiftnum + 1, left->children + left->used + 1, right->children);
        parent->item[parent_where] = left->item[left->used - shiftnum];
        left->used -= half_size_t(shiftnum);
    }

    result_t erase_one_descend(key_type const &key, node_t *node, node_t *left, node_t *right, inner_node_t *left_parent, inner_node_t *right_parent, inner_node_t *parent, size_type parent_where)
    {
        if(node->level == 0)
        {
            leaf_node_t* leaf_node = static_cast<leaf_node_t*>(node);
            leaf_node_t* leaf_left = static_cast<leaf_node_t*>(left);
            leaf_node_t* leaf_right = static_cast<leaf_node_t*>(right);
            size_type where = lower_bound_(leaf_node, key);
            if(where >= leaf_node->used || get_comparator_()(get_key_(leaf_node->item[where]), key))
            {
                return result_t(btree_not_found);
            }
            move_prev_and_destroy_one_(leaf_node->item + where + 1, leaf_node->item + leaf_node->used);
            leaf_node->used--;
            result_t result(btree_ok);
            if(where == leaf_node->used)
            {
                if(parent != nullptr && parent_where < parent->used)
                {
                    parent->item[parent_where] = get_key_(leaf_node->item[leaf_node->used - 1]);
                }
                else if(leaf_node->used >= 1)
                {
                    result |= result_t(btree_update_lastkey, get_key_(leaf_node->item[leaf_node->used - 1]));
                }
            }
            if(leaf_node->is_underflow() && !(leaf_node == root_.parent && leaf_node->used >= 1))
            {
                if(leaf_left == nullptr && leaf_right == nullptr)
                {
                    free_node_<false>(root_.parent);
                    root_.parent = nullptr;
                    root_.left = root_.right = nullptr;
                    return result_t(btree_ok);
                }
                else if((leaf_left == nullptr || leaf_left->is_few()) && (leaf_right == nullptr || leaf_right->is_few()))
                {
                    if(left_parent == parent)
                    {
                        result |= merge_leaves(leaf_left, leaf_node, left_parent);
                    }
                    else
                    {
                        result |= merge_leaves(leaf_node, leaf_right, right_parent);
                    }
                }
                else if((leaf_left != nullptr && leaf_left->is_few()) && (leaf_right != nullptr && !leaf_right->is_few()))
                {
                    if(right_parent == parent)
                    {
                        result |= shift_left_leaf(leaf_node, leaf_right, right_parent, parent_where);
                    }
                    else
                    {
                        result |= merge_leaves(leaf_left, leaf_node, left_parent);
                    }
                }
                else if((leaf_left != nullptr && !leaf_left->is_few()) && (leaf_right != nullptr && leaf_right->is_few()))
                {
                    if(left_parent == parent)
                    {
                        shift_right_leaf(leaf_left, leaf_node, left_parent, parent_where - 1);
                    }
                    else
                    {
                        result |= merge_leaves(leaf_node, leaf_right, right_parent);
                    }
                }
                else if(left_parent == right_parent)
                {
                    if(leaf_left->used <= leaf_right->used)
                    {
                        result |= shift_left_leaf(leaf_node, leaf_right, right_parent, parent_where);
                    }
                    else
                    {
                        shift_right_leaf(leaf_left, leaf_node, left_parent, parent_where - 1);
                    }
                }
                else
                {
                    if(left_parent == parent)
                    {
                        shift_right_leaf(leaf_left, leaf_node, left_parent, parent_where - 1);
                    }
                    else
                    {
                        result |= shift_left_leaf(leaf_node, leaf_right, right_parent, parent_where);
                    }
                }
            }
            return result;
        }
        else
        {
            inner_node_t *inner_node = static_cast<inner_node_t *>(node);
            inner_node_t *inner_left = static_cast<inner_node_t *>(left);
            inner_node_t *inner_right = static_cast<inner_node_t *>(right);
            node_t *self_left, *self_right;
            inner_node_t *self_left_parent, *self_right_parent;
            size_type where = lower_bound_(inner_node, key);
            if(where == 0)
            {
                self_left = (left == nullptr) ? nullptr : (static_cast<inner_node_t *>(left))->children[left->used - 1];
                self_left_parent = left_parent;
            }
            else
            {
                self_left = inner_node->children[where - 1];
                self_left_parent = inner_node;
            }
            if(where == inner_node->used)
            {
                self_right = (right == nullptr) ? nullptr : (static_cast<inner_node_t*>(right))->children[0];
                self_right_parent = right_parent;
            }
            else
            {
                self_right = inner_node->children[where + 1];
                self_right_parent = inner_node;
            }
            result_t result = erase_one_descend(key, inner_node->children[where], self_left, self_right, self_left_parent, self_right_parent, inner_node, where);
            if(result.has(btree_not_found))
            {
                return result;
            }
            result_t self_result(btree_ok);
            if(result.has(btree_update_lastkey))
            {
                if(parent != nullptr && parent_where < parent->used)
                {
                    parent->item[parent_where] = std::move<key_type>(result.last_key);
                }
                else
                {
                    self_result |= result_t(btree_update_lastkey, std::move(result.last_key));
                }
            }
            if(result.has(btree_fixmerge))
            {
                if(inner_node->children[where]->used != 0)
                {
                    ++where;
                }
                free_node_<false>(inner_node->children[where]);
                move_prev_and_destroy_one_(inner_node->item + where, inner_node->item + inner_node->used);
                std::copy(inner_node->children + where + 1, inner_node->children + inner_node->used + 1, inner_node->children + where);
                --inner_node->used;
                if(inner_node->level == 1)
                {
                    --where;
                    leaf_node_t *child = static_cast<leaf_node_t *>(inner_node->children[where]);
                    inner_node->item[where] = child->item[child->used - 1];
                }
            }

            if(inner_node->is_underflow() && !(inner_node == root_.parent && inner_node->used >= 1))
            {
                if(inner_left == nullptr && inner_right == nullptr)
                {
                    root_.parent = inner_node->children[0];
                    inner_node->used = 0;
                    free_node_<false>(inner_node);
                    return result_t(btree_ok);
                }
                else if((inner_left == nullptr || inner_left->is_few()) && (inner_right == nullptr || inner_right->is_few()))
                {
                    if(left_parent == parent)
                    {
                        self_result |= merge_inner(inner_left, inner_node, left_parent, parent_where - 1);
                    }
                    else
                    {
                        self_result |= merge_inner(inner_node, inner_right, right_parent, parent_where);
                    }
                }
                else if((inner_left != nullptr && inner_left->is_few()) && (inner_right != nullptr && !inner_right->is_few()))
                {
                    if(right_parent == parent)
                    {
                        shift_left_inner(inner_node, inner_right, right_parent, parent_where);
                    }
                    else
                    {
                        self_result |= merge_inner(inner_left, inner_node, left_parent, parent_where - 1);
                    }
                }
                else if((inner_left != nullptr && !inner_left->is_few()) && (inner_right != nullptr && inner_right->is_few()))
                {
                    if(left_parent == parent)
                    {
                        shift_right_inner(inner_left, inner_node, left_parent, parent_where - 1);
                    }
                    else
                    {
                        self_result |= merge_inner(inner_node, inner_right, right_parent, parent_where);
                    }
                }
                else if(left_parent == right_parent)
                {
                    if(inner_left->used <= inner_right->used)
                    {
                        shift_left_inner(inner_node, inner_right, right_parent, parent_where);
                    }
                    else
                    {
                        shift_right_inner(inner_left, inner_node, left_parent, parent_where - 1);
                    }
                }
                else
                {
                    if(left_parent == parent)
                    {
                        shift_right_inner(inner_left, inner_node, left_parent, parent_where - 1);
                    }
                    else
                    {
                        shift_left_inner(inner_node, inner_right, right_parent, parent_where);
                    }
                }
            }
            return self_result;
        }
    }
};
