#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>
#include <cstring>
#include <type_traits>
#include <tuple>
#include <vector>


namespace b_plus_plus_tree_detail
{
    class move_scalar_tag
    {
    };
    class move_assign_tag
    {
    };
    template<class iterator_t> struct get_tag
    {
        typedef typename std::conditional<std::is_scalar<typename std::iterator_traits<iterator_t>::value_type>::value, move_scalar_tag, move_assign_tag>::type type;
    };

    template<class iterator_t, class in_value_t, class tag_t> void construct_one(iterator_t where, in_value_t &&value, tag_t)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        ::new(std::addressof(*where)) iterator_value_t(std::forward<in_value_t>(value));
    }

    template<class iterator_t> void destroy_one(iterator_t where, move_scalar_tag)
    {
    }
    template<class iterator_t> void destroy_one(iterator_t where, move_assign_tag)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        where->~iterator_value_t();
    }

    template<class iterator_t> void destroy_range(iterator_t destroy_begin, iterator_t destroy_end, move_scalar_tag)
    {
    }
    template<class iterator_t> void destroy_range(iterator_t destroy_begin, iterator_t destroy_end, move_assign_tag)
    {
        for(; destroy_begin != destroy_end; ++destroy_begin)
        {
            destroy_one(destroy_begin, move_assign_tag());
        }
    }

    template<class iterator_from_t, class iterator_to_t, class tag_t> void move_forward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, tag_t)
    {
        std::move(move_begin, move_end, to_begin);
    }

    template<class iterator_from_t, class iterator_to_t, class tag_t> void move_backward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, tag_t)
    {
        std::move_backward(move_begin, move_end, to_begin + (move_end - move_begin));
    }

    template<class iterator_from_t, class iterator_to_t> void move_construct(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_scalar_tag)
    {
        std::move(move_begin, move_end, to_begin);
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        for(; move_begin != move_end; ++move_begin)
        {
            construct_one(to_begin++, std::move(*move_begin), move_assign_tag());
        }
    }

    template<class iterator_t> void move_next_to_and_construct(iterator_t move_begin, iterator_t move_end, iterator_t to_begin, move_scalar_tag)
    {
        std::move(move_begin, move_end, to_begin);
    }
    template<class iterator_t> void move_next_to_and_construct(iterator_t move_begin, iterator_t move_end, iterator_t to_begin, move_assign_tag)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        if(to_begin < move_end)
        {
            iterator_t split = move_end - (to_begin - move_begin);
            move_construct(split, move_end, move_end, move_assign_tag());
            move_backward(move_begin, split, to_begin, move_assign_tag());
        }
        else
        {
            move_construct(move_begin, move_end, to_begin, move_assign_tag());
            while(move_end != to_begin)
            {
                construct_one(move_end++, iterator_value_t(), move_assign_tag());
            }
        }
    }

    template<class iterator_from_t, class iterator_to_t> void move_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_scalar_tag)
    {
        std::move(move_begin, move_end, to_begin);
    }
    template<class iterator_from_t, class iterator_to_t> void move_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        for(; move_begin != move_end; ++move_begin)
        {
            *to_begin++ = std::move(*move_begin);
            destroy_one(move_begin, move_assign_tag());
        }
    }

    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_scalar_tag)
    {
        std::move(move_begin, move_end, to_begin);
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        for(; move_begin != move_end; ++move_begin)
        {
            construct_one(to_begin++, std::move(*move_begin), move_assign_tag());
            destroy_one(move_begin, move_assign_tag());
        }
    }

    template<class iterator_t, class in_value_t> void move_next_and_insert_one(iterator_t move_begin, iterator_t move_end, in_value_t &&value, move_scalar_tag)
    {
        std::move(move_begin, move_end, move_begin + 1);
        *move_begin = std::forward<in_value_t>(value);
    }
    template<class iterator_t, class in_value_t> void move_next_and_insert_one(iterator_t move_begin, iterator_t move_end, in_value_t &&value, move_assign_tag)
    {
        if(move_begin == move_end)
        {
            construct_one(move_begin, std::forward<in_value_t>(value), move_assign_tag());
        }
        else
        {
            iterator_t to_end = std::next(move_end);
            construct_one(--to_end, std::move(*--move_end), move_assign_tag());
            while(move_begin != move_end)
            {
                *--to_end = std::move(*--move_end);
            }
            *move_begin = std::forward<in_value_t>(value);
        }
    }

    template<class iterator_t> void move_prev_and_destroy_one(iterator_t move_begin, iterator_t move_end, move_scalar_tag)
    {
        std::move_backward(move_begin, move_end, move_end - 1);
    }
    template<class iterator_t> void move_prev_and_destroy_one(iterator_t move_begin, iterator_t move_end, move_assign_tag)
    {
        if(move_begin == move_end)
        {
            destroy_one(move_begin - 1, move_assign_tag());
        }
        else
        {
            iterator_t to_begin = std::prev(move_begin);
            while(move_begin != move_end)
            {
                *to_begin++ = std::move(*move_begin++);
            }
            destroy_one(to_begin, move_assign_tag());
        }
    }
}

template<class config_t>
class b_plus_plus_tree
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

protected:
    struct node_t
    {
        node_t *parent;
        size_t size;
        size_t level;
    };
    struct inner_node_t : public node_t
    {
        typedef key_type item_type;
        enum
        {
            max = ((config_t::memory_block_size - sizeof(node_t) - sizeof(size_t) - sizeof(nullptr)) / (sizeof(key_type) + sizeof(nullptr))),
            min = max / 2,
        };
        size_t used;
        node_t *children[max + 1];
        key_type item[max];

        size_t &bound()
        {
            return used;
        }
        size_t bound() const
        {
            return used;
        }
        bool is_full() const
        {
            return used == max;
        }
        bool is_few() const
        {
            return used <= min;
        }
        bool is_underflow() const
        {
            return used < min;
        }
    };
    struct leaf_node_t : public node_t
    {
        typedef storage_type item_type;
        enum
        {
            max = (config_t::memory_block_size - sizeof(node_t) - sizeof(nullptr) * 2) / sizeof(storage_type),
            min = max / 2,
        };
        node_t *prev;
        node_t *next;
        storage_type item[max];

        size_t &bound()
        {
            return node_t::size;
        }
        size_t bound() const
        {
            return node_t::size;
        }
        bool is_full() const
        {
            return node_t::size == max;
        }
        bool is_few() const
        {
            return node_t::size <= min;
        }
        bool is_underflow() const
        {
            return node_t::size < min;
        }
    };
    template<class, class> struct status_select_t
    {
        status_select_t() : inner_count(), leaf_count()
        {
        }
        size_type inner_count;
        size_type leaf_count;
        std::vector<size_type, typename allocator_type::template rebind<size_type>::other> level_count;
        static const size_type inner_bound = inner_node_t::max;
        static const size_type leaf_bound = leaf_node_t::max;
    };
    template<class unused_t> struct status_select_t<std::false_type, unused_t>
    {
        status_select_t()
        {
        }
    };
    typedef status_select_t<typename config_t::status_type::type, void> status_t;
    template<class, class> struct status_control_select_t
    {
        static void change_leaf(status_t &status, difference_type value)
        {
            status.leaf_count += value;
            if(status.level_count.empty())
            {
                status.level_count.resize(1, 0);
            }
            status.level_count[0] += value;
            if(value < 0)
            {
                while(status.level_count.back() == 0)
                {
                    status.level_count.pop_back();
                }
            }
        }
        static void change_inner(status_t &status, difference_type value, size_type level)
        {
            status.inner_count += value;
            if(status.level_count.size() <= level)
            {
                status.level_count.resize(level + 1, 0);
            }
            status.level_count[level] += value;
            if(value < 0)
            {
                while(status.level_count.back() == 0)
                {
                    status.level_count.pop_back();
                }
            }
        }
    };
    template<class unused_t> struct status_control_select_t<std::false_type, unused_t>
    {
        static void change_leaf(status_t &, difference_type)
        {
        }
        static void change_inner(status_t &status, difference_type value, size_type level)
        {
        }
    };
    typedef status_control_select_t<typename config_t::status_type::type, void> status_control_t;
    typedef typename std::aligned_union<config_t::memory_block_size, inner_node_t, leaf_node_t>::type memory_node_t;
    typedef typename allocator_type::template rebind<memory_node_t>::other node_allocator_t;
    struct root_node_t : public node_t, public key_compare, public node_allocator_t, public status_t
    {
        template<class any_key_compare, class any_allocator_t> root_node_t(any_key_compare &&comp, any_allocator_t &&alloc) : key_compare(std::forward<any_key_compare>(comp)), node_allocator_t(std::forward<any_allocator_t>(alloc)), status_t()
        {
            static_assert(inner_node_t::max >= 4, "low memory_block_size");
            static_assert(leaf_node_t::max >= 4, "low memory_block_size");
            node_t::parent = left = right = this;
            node_t::size = 0;
            node_t::level = 0;
        }
        node_t *left;
        node_t *right;
    };
    struct key_stack_t
    {
        typename std::aligned_storage<sizeof(key_type), alignof(key_type)>::type key_pod;
        key_stack_t()
        {
        }
        key_stack_t(key_stack_t &&key)
        {
            ::new(&key_pod) key_type(std::move(key.key()));
        }
        key_stack_t(key_stack_t const &key)
        {
            ::new(&key_pod) key_type(key.key());
        }
        key_stack_t(key_type &&key)
        {
            ::new(&key_pod) key_type(std::move(key));
        }
        key_stack_t(key_type const &key)
        {
            ::new(&key_pod) key_type(key);
        }
        operator key_type &()
        {
            return *reinterpret_cast<key_type *>(&key_pod);
        }
        operator key_type const &() const
        {
            return *reinterpret_cast<key_type const *>(&key_pod);
        }
        operator key_type &&()
        {
            return std::move(*reinterpret_cast<key_type *>(&key_pod));
        }
        key_type &key()
        {
            return *reinterpret_cast<key_type *>(&key_pod);
        }
        key_type const &key() const
        {
            return *reinterpret_cast<key_type const *>(&key_pod);
        }
        key_type *operator &()
        {
            return reinterpret_cast<key_type *>(&key_pod);
        }
        key_stack_t &operator = (key_stack_t &&other)
        {
            key() = std::move(other.key());
            return *this;
        }
        key_stack_t &operator = (key_stack_t const &other)
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
                if(has(btree_update_lastkey))
                {
                    last_key.key() = other.last_key.key();
                }
                else
                {
                    ::new(&last_key) key_type(other.last_key.key());
                }
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
        result_t &operator = (result_t const &other)
        {
            if(other.has(btree_update_lastkey))
            {
                if(has(btree_update_lastkey))
                {
                    last_key.key() = other.last_key.key();
                }
                else
                {
                    ::new(&last_key) key_type(other.last_key.key());
                }
            }
            else
            {
                if(has(btree_update_lastkey))
                {
                    last_key.key().~key_type();
                }
            }
            flags = other.flags;
            return *this;
        }
        result_t &operator = (result_t &&other)
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
            else
            {
                if(has(btree_update_lastkey))
                {
                    last_key.key().~key_type();
                }
            }
            flags = other.flags;
            return *this;
        }
    };
    typedef std::pair<leaf_node_t *, size_type> pair_pos_t;
    template<class k_t, class v_t> struct get_key_select_t
    {
        key_type const &operator()(key_type const &value)
        {
            return value;
        }
        key_type const &operator()(storage_type const &value)
        {
            return config_t::get_key(value);
        }
        key_type const &operator()(pair_pos_t pos)
        {
            return (*this)(pos.first->item[pos.second]);
        }
    };
    template<class k_t> struct get_key_select_t<k_t, k_t>
    {
        key_type const &operator()(key_type const &value)
        {
            return config_t::get_key(value);
        }
        key_type const &operator()(pair_pos_t pos)
        {
            return (*this)(pos.first->item[pos.second]);
        }
    };
    typedef get_key_select_t<key_type, storage_type> get_key_t;
    enum
    {
        binary_search_limit = 1024
    };
public:
    class iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename b_plus_plus_tree::value_type value_type;
        typedef typename b_plus_plus_tree::difference_type difference_type;
        typedef typename b_plus_plus_tree::reference reference;
        typedef typename b_plus_plus_tree::pointer pointer;
    public:
        iterator(node_t *in_node, size_type in_where) : node(in_node), where(in_where)
        {
        }
        iterator(pair_pos_t pos, b_plus_plus_tree *self) : node(pos.first == nullptr ? static_cast<node_t *>(&self->root_) : static_cast<node_t *>(pos.first)), where(pos.second)
        {
        }
        iterator(iterator const &other) : node(other.node), where(other.where)
        {
        }
        iterator &operator += (difference_type diff)
        {
            b_plus_plus_tree::advance_step_(node, where, diff);
            return *this;
        }
        iterator &operator -= (difference_type diff)
        {
            b_plus_plus_tree::advance_step_(node, where, -diff);
            return *this;
        }
        iterator operator + (difference_type diff) const
        {
            iterator ret = *this;
            b_plus_plus_tree::advance_step_(ret.node, ret.where, diff);
            return ret;
        }
        iterator operator - (difference_type diff) const
        {
            iterator ret = *this;
            b_plus_plus_tree::advance_step_(ret.node, ret.where, -diff);
            return ret;
        }
        difference_type operator - (iterator const &other) const
        {
            return static_cast<difference_type>(b_plus_plus_tree::calculate_rank_(node, where)) - static_cast<difference_type>(b_plus_plus_tree::calculate_rank_(other.node, other.where));
        }
        iterator &operator++()
        {
            b_plus_plus_tree::advance_next_(node, where);
            return *this;
        }
        iterator &operator--()
        {
            b_plus_plus_tree::advance_prev_(node, where);
            return *this;
        }
        iterator operator++(int)
        {
            iterator save(*this);
            ++*this;
            return save;
        }
        iterator operator--(int)
        {
            iterator save(*this);
            --*this;
            return save;
        }
        reference operator *() const
        {
            return reinterpret_cast<reference>(static_cast<leaf_node_t *>(node)->item[where]);
        }
        pointer operator->() const
        {
            return reinterpret_cast<pointer>(static_cast<leaf_node_t *>(node)->item + where);
        }
        reference operator[](difference_type index) const
        {
            return *(*this + index);
        }
        bool operator > (iterator const &other) const
        {
            return *this - other > 0;
        }
        bool operator < (iterator const &other) const
        {
            return *this - other < 0;
        }
        bool operator >= (iterator const &other) const
        {
            return *this - other >= 0;
        }
        bool operator <= (iterator const &other) const
        {
            return *this - other <= 0;
        }
        bool operator == (iterator const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator != (iterator const &other) const
        {
            return node != other.node || where != other.where;
        }
    private:
        friend class b_plus_plus_tree;
        node_t *node;
        size_type where;
    };
    class const_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename b_plus_plus_tree::value_type value_type;
        typedef typename b_plus_plus_tree::difference_type difference_type;
        typedef typename b_plus_plus_tree::reference reference;
        typedef typename b_plus_plus_tree::const_reference const_reference;
        typedef typename b_plus_plus_tree::pointer pointer;
        typedef typename b_plus_plus_tree::const_pointer const_pointer;
    public:
        const_iterator(node_t *in_node, size_type in_where) : node(in_node), where(in_where)
        {
        }
        const_iterator(pair_pos_t pos, b_plus_plus_tree const *self) : node(pos.first == nullptr ? self->root_.parent->parent : pos.first), where(pos.second)
        {
        }
        const_iterator(iterator const &it) : node(it.node), where(it.where)
        {
        }
        const_iterator(const_iterator const &other) : node(other.node), where(other.where)
        {
        }
        const_iterator &operator += (difference_type diff)
        {
            b_plus_plus_tree::advance_step_(node, where, diff);
            return *this;
        }
        const_iterator &operator -= (difference_type diff)
        {
            b_plus_plus_tree::advance_step_(node, where, -diff);
            return *this;
        }
        const_iterator operator + (difference_type diff) const
        {
            const_iterator ret = *this;
            b_plus_plus_tree::advance_step_(ret.node, ret.where, diff);
            return ret;
        }
        const_iterator operator - (difference_type diff) const
        {
            const_iterator ret = *this;
            b_plus_plus_tree::advance_step_(ret.node, ret.where, -diff);
            return ret;
        }
        difference_type operator - (const_iterator const &other) const
        {
            return static_cast<difference_type>(b_plus_plus_tree::calculate_rank_(node, where)) - static_cast<difference_type>(b_plus_plus_tree::calculate_rank_(other.node, other.where));
        }
        const_iterator &operator++()
        {
            b_plus_plus_tree::advance_next_(node, where);
            return *this;
        }
        const_iterator &operator--()
        {
            b_plus_plus_tree::advance_prev_(node, where);
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator save(*this);
            ++*this;
            return save;
        }
        const_iterator operator--(int)
        {
            const_iterator save(*this);
            --*this;
            return save;
        }
        const_reference operator *() const
        {
            return reinterpret_cast<const_reference>(static_cast<leaf_node_t *>(node)->item[where]);
        }
        const_pointer operator->() const
        {
            return reinterpret_cast<const_pointer>(static_cast<leaf_node_t *>(node)->item + where);
        }
        const_reference operator[](difference_type index) const
        {
            return *(*this + index);
        }
        bool operator > (const_iterator const &other) const
        {
            return *this - other > 0;
        }
        bool operator < (const_iterator const &other) const
        {
            return *this - other < 0;
        }
        bool operator >= (const_iterator const &other) const
        {
            return *this - other >= 0;
        }
        bool operator <= (const_iterator const &other) const
        {
            return *this - other <= 0;
        }
        bool operator == (const_iterator const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator != (const_iterator const &other) const
        {
            return node != other.node || where != other.where;
        }
    private:
        friend class b_plus_plus_tree;
        node_t *node;
        size_type where;
    };
    class reverse_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename b_plus_plus_tree::value_type value_type;
        typedef typename b_plus_plus_tree::difference_type difference_type;
        typedef typename b_plus_plus_tree::reference reference;
        typedef typename b_plus_plus_tree::pointer pointer;
    public:
        reverse_iterator(node_t *in_node, size_type in_where) : node(in_node), where(in_where)
        {
        }
        reverse_iterator(pair_pos_t pos, b_plus_plus_tree *self) : node(pos.first == nullptr ? static_cast<node_t *>(&self->root_) : static_cast<node_t *>(pos.first)), where(pos.second)
        {
        }
        explicit reverse_iterator(iterator const &other) : node(other.node), where(other.where)
        {
            ++*this;
        }
        reverse_iterator(reverse_iterator const &other) : node(other.node), where(other.where)
        {
        }
        reverse_iterator &operator += (difference_type diff)
        {
            b_plus_plus_tree::advance_step_(node, where, -diff);
            return *this;
        }
        reverse_iterator &operator -= (difference_type diff)
        {
            b_plus_plus_tree::advance_step_(node, where, diff);
            return *this;
        }
        reverse_iterator operator + (difference_type diff) const
        {
            reverse_iterator ret = *this;
            b_plus_plus_tree::advance_step_(ret.node, ret.where, -diff);
            return ret;
        }
        reverse_iterator operator - (difference_type diff) const
        {
            reverse_iterator ret = *this;
            b_plus_plus_tree::advance_step_(ret.node, ret.where, diff);
            return ret;
        }
        difference_type operator - (reverse_iterator const &other) const
        {
            return static_cast<difference_type>(b_plus_plus_tree::calculate_rank_(other.node, other.where)) - static_cast<difference_type>(b_plus_plus_tree::calculate_rank_(node, where));
        }
        reverse_iterator &operator++()
        {
            b_plus_plus_tree::advance_prev_(node, where);
            return *this;
        }
        reverse_iterator &operator--()
        {
            b_plus_plus_tree::advance_next_(node, where);
            return *this;
        }
        reverse_iterator operator++(int)
        {
            reverse_iterator save(*this);
            ++*this;
            return save;
        }
        reverse_iterator operator--(int)
        {
            reverse_iterator save(*this);
            --*this;
            return save;
        }
        reference operator *() const
        {
            return reinterpret_cast<reference>(static_cast<leaf_node_t *>(node)->item[where]);
        }
        pointer operator->() const
        {
            return reinterpret_cast<pointer>(static_cast<leaf_node_t *>(node)->item + where);
        }
        reference operator[](difference_type index) const
        {
            return *(*this + index);
        }
        bool operator > (reverse_iterator const &other) const
        {
            return *this - other > 0;
        }
        bool operator < (reverse_iterator const &other) const
        {
            return *this - other < 0;
        }
        bool operator >= (reverse_iterator const &other) const
        {
            return *this - other >= 0;
        }
        bool operator <= (reverse_iterator const &other) const
        {
            return *this - other <= 0;
        }
        bool operator == (reverse_iterator const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator != (reverse_iterator const &other) const
        {
            return node != other.node || where != other.where;
        }
        iterator base() const
        {
            return ++iterator(node, where);
        }
    private:
        friend class b_plus_plus_tree;
        node_t *node;
        size_type where;
    };
    class const_reverse_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename b_plus_plus_tree::value_type value_type;
        typedef typename b_plus_plus_tree::difference_type difference_type;
        typedef typename b_plus_plus_tree::reference reference;
        typedef typename b_plus_plus_tree::const_reference const_reference;
        typedef typename b_plus_plus_tree::pointer pointer;
        typedef typename b_plus_plus_tree::const_pointer const_pointer;
    public:
        const_reverse_iterator(node_t *in_node, size_type in_where) : node(in_node), where(in_where)
        {
        }
        const_reverse_iterator(pair_pos_t pos, b_plus_plus_tree const *self) : node(pos.first == nullptr ? self->root_.parent->parent : pos.first), where(pos.second)
        {
        }
        explicit const_reverse_iterator(const_iterator const &other) : node(other.node), where(other.where)
        {
            ++*this;
        }
        const_reverse_iterator(reverse_iterator const &other) : node(other.node), where(other.where)
        {
        }
        const_reverse_iterator(reverse_iterator it) : node(it.node), where(it.where)
        {
        }
        const_reverse_iterator(const_reverse_iterator const &other) : node(other.node), where(other.where)
        {
        }
        const_reverse_iterator &operator += (difference_type diff)
        {
            b_plus_plus_tree::advance_step_(node, where, -diff);
            return *this;
        }
        const_reverse_iterator &operator -= (difference_type diff)
        {
            b_plus_plus_tree::advance_step_(node, where, diff);
            return *this;
        }
        const_reverse_iterator operator + (difference_type diff) const
        {
            const_reverse_iterator ret = *this;
            b_plus_plus_tree::advance_step_(ret.node, ret.where, -diff);
            return ret;
        }
        const_reverse_iterator operator - (difference_type diff) const
        {
            const_reverse_iterator ret = *this;
            b_plus_plus_tree::advance_step_(ret.node, ret.where, diff);
            return ret;
        }
        difference_type operator - (const_reverse_iterator const &other) const
        {
            return static_cast<difference_type>(b_plus_plus_tree::calculate_rank_(other.node, other.where)) - static_cast<difference_type>(b_plus_plus_tree::calculate_rank_(node, where));
        }
        const_reverse_iterator &operator++()
        {
            b_plus_plus_tree::advance_prev_(node, where);
            return *this;
        }
        const_reverse_iterator &operator--()
        {
            b_plus_plus_tree::advance_next_(node, where);
            return *this;
        }
        const_reverse_iterator operator++(int)
        {
            const_reverse_iterator save(*this);
            ++*this;
            return save;
        }
        const_reverse_iterator operator--(int)
        {
            const_reverse_iterator save(*this);
            --*this;
            return save;
        }
        const_reference operator *() const
        {
            return reinterpret_cast<const_reference>(static_cast<leaf_node_t *>(node)->item[where]);
        }
        const_pointer operator->() const
        {
            return reinterpret_cast<const_pointer>(static_cast<leaf_node_t *>(node)->item + where);
        }
        const_reference operator[](difference_type index) const
        {
            return *(*this + index);
        }
        bool operator > (const_reverse_iterator const &other) const
        {
            return *this - other > 0;
        }
        bool operator < (const_reverse_iterator const &other) const
        {
            return *this - other < 0;
        }
        bool operator >= (const_reverse_iterator const &other) const
        {
            return *this - other >= 0;
        }
        bool operator <= (const_reverse_iterator const &other) const
        {
            return *this - other <= 0;
        }
        bool operator == (const_reverse_iterator const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator != (const_reverse_iterator const &other) const
        {
            return node != other.node || where != other.where;
        }
        const_iterator base() const
        {
            return ++iterator(node, where);
        }
    private:
        friend class b_plus_plus_tree;
        node_t *node;
        size_type where;
    };
    typedef typename std::conditional<config_t::unique_type::value, std::pair<iterator, bool>, iterator>::type insert_result_t;
    typedef std::pair<iterator, bool> pair_ib_t;
protected:
    typedef std::pair<pair_pos_t, bool> pair_posi_t;
    template<class unique_type> typename std::enable_if<unique_type::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return std::make_pair(iterator(posi.first.first, posi.first.second), posi.second);
    }
    template<class unique_type> typename std::enable_if<!unique_type::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return iterator(posi.first.first, posi.first.second);
    }
public:;
       //empty
       b_plus_plus_tree() : root_(key_compare(), allocator_type())
       {
       }
       //empty
       explicit b_plus_plus_tree(key_compare const &comp, allocator_type const &alloc = allocator_type()) : root_(comp, alloc)
       {
       }
       //empty
       explicit b_plus_plus_tree(allocator_type const &alloc) : root_(key_compare(), alloc)
       {
       }
       //range
       template <class iterator_t> b_plus_plus_tree(iterator_t begin, iterator_t end, key_compare const &comp = key_compare(), allocator_type const &alloc = allocator_type()) : root_(comp, alloc)
       {
           insert(begin, end);
       }
       //range
       template <class iterator_t> b_plus_plus_tree(iterator_t begin, iterator_t end, allocator_type const &alloc) : root_(key_compare(), alloc)
       {
           insert(begin, end);
       }
       //copy
       b_plus_plus_tree(b_plus_plus_tree const &other) : root_(other.get_comparator_(), other.get_node_allocator_())
       {
           insert(other.begin(), other.end());
       }
       //copy
       b_plus_plus_tree(b_plus_plus_tree const &other, allocator_type const &alloc) : root_(other.get_comparator_(), alloc)
       {
           insert(other.begin(), other.end());
       }
       //move
       b_plus_plus_tree(b_plus_plus_tree &&other) : root_(key_compare(), node_allocator_t())
       {
           swap(other);
       }
       //move
       b_plus_plus_tree(b_plus_plus_tree &&other, allocator_type const &alloc) : root_(key_compare(), alloc)
       {
           insert(std::move_iterator<iterator>(other.begin()), std::move_iterator<iterator>(other.end()));
       }
       //initializer list
       b_plus_plus_tree(std::initializer_list<value_type> il, key_compare const &comp = key_compare(), allocator_type const &alloc = allocator_type()) : b_plus_plus_tree(il.begin(), il.end(), comp, alloc)
       {
       }
       //initializer list
       b_plus_plus_tree(std::initializer_list<value_type> il, allocator_type const &alloc) : b_plus_plus_tree(il.begin(), il.end(), key_compare(), alloc)
       {
       }
       //destructor
       ~b_plus_plus_tree()
       {
           clear();
       }
       //copy
       b_plus_plus_tree &operator = (b_plus_plus_tree const &other)
       {
           if(this == &other)
           {
               return *this;
           }
           clear();
           get_comparator_() = other.get_comparator_();
           get_node_allocator_() = other.get_node_allocator_();
           insert(other.begin(), other.end());
           return *this;
       }
       //move
       b_plus_plus_tree &operator = (b_plus_plus_tree &&other)
       {
           if(this == &other)
           {
               return *this;
           }
           swap(other);
           return *this;
       }
       //initializer list
       b_plus_plus_tree &operator = (std::initializer_list<value_type> il)
       {
           clear();
           insert(il.begin(), il.end());
           return *this;
       }

       allocator_type get_allocator() const
       {
           return root_;
       }

       void swap(b_plus_plus_tree &other)
       {
           std::swap(root_, other.root_);
           fix_root_();
           other.fix_root_();
       }

       typedef std::pair<iterator, iterator> pair_ii_t;
       typedef std::pair<const_iterator, const_iterator> pair_cici_t;

       //single element
       insert_result_t insert(value_type const &value)
       {
           return result_<typename config_t::unique_type>(insert_nohint_<false>(value));
       }
       //single element
       template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(in_value_t &&value)
       {
           return result_<typename config_t::unique_type>(insert_nohint_<false>(std::forward<in_value_t>(value)));
       }
       //with hint
       iterator insert(const_iterator hint, value_type const &value)
       {
           return result_<std::false_type>(insert_hint_(hint.node->size == 0 ? nullptr : static_cast<leaf_node_t *>(hint.node), hint.where, value));
       }
       //with hint
       template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(const_iterator hint, in_value_t &&value)
       {
           return result_<typename config_t::unique_type>(insert_hint_(hint.node->size == 0 ? nullptr : static_cast<leaf_node_t *>(hint.node), hint.where, std::forward<in_value_t>(value)));
       }
       //range
       template<class iterator_t> void insert(iterator_t begin, iterator_t end)
       {
           for(; begin != end; ++begin)
           {
               emplace_hint(cend(), *begin);
           }
       }
       //initializer list
       void insert(std::initializer_list<value_type> il)
       {
           insert(il.begin(), il.end());
       }

       //single element
       template<class ...args_t> insert_result_t emplace(args_t &&...args)
       {
           return result_<typename config_t::unique_type>(insert_nohint_<false>(std::move(storage_type(std::forward<args_t>(args)...))));
       }
       //with hint
       template<class ...args_t> insert_result_t emplace_hint(const_iterator hint, args_t &&...args)
       {
           return result_<typename config_t::unique_type>(insert_hint_(hint.node->size == 0 ? nullptr : static_cast<leaf_node_t *>(hint.node), hint.where, std::move(storage_type(std::forward<args_t>(args)...))));
       }

       iterator find(key_type const &key)
       {
           pair_pos_t pos = lower_bound_(key);
           return (pos.first == nullptr || pos.second >= pos.first->bound() || get_comparator_()(key, get_key_t()(pos.first->item[pos.second]))) ? iterator(&root_, 0) : iterator(pos.first, pos.second);
       }
       const_iterator find(key_type const &key) const
       {
           pair_pos_t pos = lower_bound_(key);
           return (pos.first == nullptr || pos.second >= pos.first->bound() || get_comparator_()(key, get_key_t()(pos.first->item[pos.second]))) ? iterator(root_.parent->parent, 0) : iterator(pos.first, pos.second);
       }

       iterator erase(const_iterator it)
       {
           if(root_.parent->size == 0)
           {
               return end();
           }
           size_type pos_at = rank(it);
           erase_pos_(static_cast<leaf_node_t *>(it.node), it.where);
           return at(pos_at);
       }
       size_type erase(key_type const &key)
       {
           if(root_.parent->size == 0)
           {
               return 0;
           }
           size_type count = 0;
           leaf_node_t *leaf_node;
           size_type where;
           while(true)
           {
               std::tie(leaf_node, where) = lower_bound_(key);
               if(leaf_node == nullptr || get_comparator_()(key, get_key_t()(leaf_node->item[where])))
               {
                   break;
               }
               erase_pos_(leaf_node, where);
               ++count;
               if(config_t::unique_type::value)
               {
                   break;
               }
           }
           return count;
       }
       iterator erase(const_iterator erase_begin, const_iterator erase_end)
       {
           if(erase_begin == cbegin() && erase_end == cend())
           {
               clear();
               return begin();
           }
           else
           {
               size_type pos_begin = rank(erase_begin), pos_end = rank(erase_end);
               while(pos_begin != pos_end)
               {
                   erase(at(--pos_end));
               }
               return at(pos_begin);
           }
       }

       size_type count(key_type const &key) const
       {
           if(config_t::unique_type::value)
           {
               return find(key) == end() ? 0 : 1;
           }
           else
           {
               pair_cici_t range = equal_range(key);
               return std::distance(range.first, range.second);
           }
       }
       size_type count(key_type const &min, key_type const &max) const
       {
           if(get_comparator_()(max, min))
           {
               return 0;
           }
           pair_cici_t range = b_plus_plus_tree::range(min, max);
           return std::distance(range.first, range.second);
       }

       pair_ii_t range(key_type const &min, key_type const &max)
       {
           if(get_comparator_()(max, min))
           {
               return pair_ii_t(end(), end());
           }
           return pair_ii_t(iterator(lower_bound_(min), this), iterator(upper_bound_(max), this));
       }
       pair_cici_t range(key_type const &min, key_type const &max) const
       {
           if(get_comparator_()(max, min))
           {
               return pair_cici_t(cend(), cend());
           }
           return pair_cici_t(const_iterator(lower_bound_(min), this), const_iterator(upper_bound_(max), this));
       }

       //reverse index when index < 0
       pair_ii_t slice(difference_type slice_begin = 0, difference_type slice_end = 0)
       {
           difference_type s_size = size();
           if(slice_begin < 0)
           {
               slice_begin = std::max<difference_type>(s_size + slice_begin, 0);
           }
           if(slice_end <= 0)
           {
               slice_end = s_size + slice_end;
           }
           if(slice_begin > slice_end || slice_begin >= s_size)
           {
               return pair_ii_t(end(), end());
           }
           return pair_ii_t(at(slice_begin), at(slice_end));
       }
       //reverse index when index < 0
       pair_cici_t slice(difference_type slice_begin = 0, difference_type slice_end = 0) const
       {
           difference_type s_size = size();
           if(slice_begin < 0)
           {
               slice_begin = std::max<difference_type>(s_size + slice_begin, 0);
           }
           if(slice_end <= 0)
           {
               slice_end = s_size + slice_end;
           }
           if(slice_begin > slice_end || slice_begin >= s_size)
           {
               return pair_cici_t(cend(), cend());
           }
           return pair_cici_t(at(slice_begin), at(slice_end));
       }

       iterator lower_bound(key_type const &key)
       {
           return iterator(lower_bound_(key), this);
       }
       const_iterator lower_bound(key_type const &key) const
       {
           return const_iterator(lower_bound_(key), this);
       }
       iterator upper_bound(key_type const &key)
       {
           return iterator(upper_bound_(key), this);
       }
       const_iterator upper_bound(key_type const &key) const
       {
           return const_iterator(upper_bound_(key), this);
       }

       pair_ii_t equal_range(key_type const &key)
       {
           return pair_ii_t(iterator(lower_bound_(key), this), iterator(upper_bound_(key), this));
       }
       pair_cici_t equal_range(key_type const &key) const
       {
           return pair_cici_t(const_iterator(lower_bound_(key), this), const_iterator(upper_bound_(key), this));
       }

       iterator begin()
       {
           return iterator(root_.left, 0);
       }
       iterator end()
       {
           return iterator(&root_, 0);
       }
       const_iterator begin() const
       {
           return iterator(root_.left, 0);
       }
       const_iterator end() const
       {
           return const_iterator(root_.parent->parent, 0);
       }
       const_iterator cbegin() const
       {
           return iterator(root_.left, 0);
       }
       const_iterator cend() const
       {
           return const_iterator(root_.parent->parent, 0);
       }
       reverse_iterator rbegin()
       {
           return reverse_iterator(root_.right, root_.right->size == 0 ? 0 : static_cast<leaf_node_t *>(root_.right)->bound() - 1);
       }
       reverse_iterator rend()
       {
           return reverse_iterator(&root_, 0);
       }
       const_reverse_iterator rbegin() const
       {
           return const_reverse_iterator(root_.right, root_.right->size == 0 ? 0 : static_cast<leaf_node_t *>(root_.right)->bound() - 1);
       }
       const_reverse_iterator rend() const
       {
           return const_reverse_iterator(root_.parent->parent, 0);
       }
       const_reverse_iterator crbegin() const
       {
           return const_reverse_iterator(root_.right, root_.right->size == 0 ? 0 : static_cast<leaf_node_t *>(root_.right)->bound() - 1);
       }
       const_reverse_iterator crend() const
       {
           return const_reverse_iterator(root_.parent->parent, 0);
       }

       reference front()
       {
           return reinterpret_cast<reference>(static_cast<leaf_node_t *>(root_.left)->item[0]);
       }
       reference back()
       {
           leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
           return reinterpret_cast<reference>(tail->item[tail->bound() - 1]);
       }

       const_reference front() const
       {
           return reinterpret_cast<const_reference>(static_cast<leaf_node_t *>(root_.left)->item[0]);
       }
       const_reference back() const
       {
           leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
           return reinterpret_cast<const_reference>(tail->item[tail->bound() - 1]);
       }

       bool empty() const
       {
           return root_.parent->size == 0;
       }
       void clear()
       {
           if(root_.parent != &root_)
           {
               free_node_<true>(root_.parent);
               root_.parent = root_.left = root_.right = &root_;
           }
       }
       size_type size() const
       {
           return root_.parent->size;
       }
       size_type max_size() const
       {
           return node_allocator_t(get_node_allocator_()).max_size();
       }

       //if(index >= size) return end
       iterator at(size_type index)
       {
           return iterator(access_index_(root_.parent, index), this);
       }
       //if(index >= size) return end
       const_iterator at(size_type index) const
       {
           return const_iterator(access_index_(root_.parent, index), this);
       }

       //rank(begin) == 0, key rank
       size_type rank(key_type const &key) const
       {
           return calculate_key_rank_<true>(key);
       }
       //rank(begin) == 0, rank of iterator
       static size_type rank(const_iterator where)
       {
           return calculate_rank_(where.node, where.where);
       }

       //rank(begin) == 0, key rank current best
       size_type lower_rank(key_type const &key) const
       {
           return calculate_key_rank_<true>(key);
       }
       //rank(begin) == 0, key rank when insert
       size_type upper_rank(key_type const &key) const
       {
           return calculate_key_rank_<false>(key);
       }

       status_t const &status() const
       {
           static_assert(config_t::status_type::value, "status_type false");
           return root_;
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

    node_allocator_t &get_node_allocator_()
    {
        return root_;
    }
    node_allocator_t const &get_node_allocator_() const
    {
        return root_;
    }

    void fix_root_()
    {
        if(root_.parent->size == 0)
        {
            root_.parent = root_.left = root_.right = &root_;
        }
        else
        {
            root_.parent->parent = &root_;
            static_cast<leaf_node_t *>(root_.left)->prev = &root_;
            static_cast<leaf_node_t *>(root_.right)->next = &root_;
        }
    }

    inner_node_t *alloc_inner_node_(node_t *parent, size_type level)
    {
        inner_node_t *node = reinterpret_cast<inner_node_t *>(get_node_allocator_().allocate(1));
        node->parent = parent;
        node->size = 0;
        node->level = level;
        node->used = 0;
        status_control_t::change_inner(root_, 1, level);
        return node;
    }
    leaf_node_t *alloc_leaf_node_()
    {
        leaf_node_t *node = reinterpret_cast<leaf_node_t *>(get_node_allocator_().allocate(1));
        node->parent = nullptr;
        node->size = 0;
        node->level = 0;
        node->prev = nullptr;
        node->next = nullptr;
        status_control_t::change_leaf(root_, 1);
        return node;
    }
    template<class in_node_t> void dealloc_node_(in_node_t *node)
    {
        destroy_range_(node->item, node->item + node->bound());
        get_node_allocator_().deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }

    template<bool is_recursive>void free_node_(node_t *node)
    {
        if(node->level == 0)
        {
            dealloc_node_(static_cast<leaf_node_t *>(node));
            status_control_t::change_leaf(root_, -1);
        }
        else
        {
            inner_node_t *inner_node = static_cast<inner_node_t *>(node);
            if(is_recursive)
            {
                for(size_type i = 0; i <= inner_node->bound(); ++i)
                {
                    free_node_<is_recursive>(inner_node->children[i]);
                }
            }
            status_control_t::change_inner(root_, -1, inner_node->level);
            dealloc_node_(inner_node);
        }
    }

    pair_pos_t advance_next_(pair_pos_t pos)
    {
        if(pos.first == nullptr)
        {
            if(root_.parent->size == 0)
            {
                return std::make_pair(nullptr, 0);
            }
            else
            {
                return std::make_pair(static_cast<leaf_node_t *>(root_.left), 0);
            }
        }
        else
        {
            if(pos.second + 1 >= pos.first->bound())
            {
                return std::make_pair(pos.first->next->size == 0 ? nullptr : static_cast<leaf_node_t *>(pos.first->next), 0);
            }
            else
            {
                return std::make_pair(pos.first, pos.second + 1);
            }
        }
    }
    pair_pos_t advance_prev_(pair_pos_t pos)
    {
        if(pos.second == 0)
        {
            leaf_node_t *leaf_node = root_.parent->size == 0 ? nullptr : static_cast<leaf_node_t *>(pos.first->prev);
            return std::make_pair(leaf_node, leaf_node == nullptr ? 0 : leaf_node->bound() - 1);
        }
        else
        {
            return std::make_pair(pos.first, pos.second - 1);
        }
    }

    static void advance_next_(node_t *&node, size_type &where)
    {
        if(node->size == 0)
        {
            node = static_cast<root_node_t *>(node)->left;
        }
        else
        {
            leaf_node_t *leaf_node = static_cast<leaf_node_t *>(node);
            if(++where >= leaf_node->bound())
            {
                node = leaf_node->next;
                where = 0;
            }
        }
    }
    static void advance_prev_(node_t *&node, size_type &where)
    {
        if(where == 0)
        {
            node = node->size == 0 ? static_cast<root_node_t *>(node)->right : static_cast<leaf_node_t *>(node)->prev;
            where = node->size == 0 ? 0 : static_cast<leaf_node_t *>(node)->bound() - 1;
        }
        else
        {
            --where;
        }
    }

    static void advance_step_(node_t *&node, size_type &where, difference_type step)
    {
        if(node->size == 0)
        {
            if(step == 0)
            {
                return;
            }
            else if(step > 0)
            {
                --step;
                advance_next_(node, where);
            }
            else
            {
                ++step;
                advance_prev_(node, where);
            }
            if(node->size == 0)
            {
                return;
            }
        }
        std::tie(node, where) = advance_root_(node, where);
        step += where;
        if(step < 0 || size_type(step) >= node->size)
        {
            node = node->parent;
            where = 0;
        }
        else
        {
            std::tie(node, where) = access_index_(node, step);
        }
    }

    template<bool is_leftish> size_type calculate_key_rank_(key_type const &key) const
    {
        node_t *node = root_.parent;
        if(node->size == 0)
        {
            return root_.parent->size;
        }
        size_type rank = 0;
        while(node->level > 0)
        {
            inner_node_t const *inner_node = static_cast<inner_node_t const *>(node);
            size_type where;
            if(std::is_scalar<key_type>::value)
            {
                for(where = 0; where < inner_node->bound(); ++where)
                {
                    if(is_leftish ? !get_comparator_()(get_key_t()(inner_node->item[where]), key) : get_comparator_()(key, get_key_t()(inner_node->item[where])))
                    {
                        break;
                    }
                    else
                    {
                        rank += inner_node->children[where]->size;
                    }
                }
            }
            else
            {
                where = is_leftish ? lower_bound_(inner_node, key) : upper_bound_(inner_node, key);
                for(size_type i = 0; i < where; ++i)
                {
                    rank += inner_node->children[i]->size;
                }
            }
            node = inner_node->children[where];
        }
        return rank + (is_leftish ? lower_bound_(static_cast<leaf_node_t *>(node), key) : upper_bound_(static_cast<leaf_node_t *>(node), key));
    }

    static size_type calculate_rank_(node_t *node, size_type where)
    {
        if(node->size == 0)
        {
            return node->parent->size;
        }
        else
        {
            size_type rank;
            std::tie(std::ignore, rank) = advance_root_(node, where);
            return rank;
        }
    }

    static std::pair<node_t *, size_type> advance_root_(node_t *node, size_type where)
    {
        while(node->parent->size != 0)
        {
            inner_node_t *parent = static_cast<inner_node_t *>(node->parent);
            for(size_type i = 0; ; ++i)
            {
                if(parent->children[i] == node)
                {
                    node = parent;
                    break;
                }
                else
                {
                    where += parent->children[i]->size;
                }
            }
        }
        return std::make_pair(node, where);
    }

    static pair_pos_t access_index_(node_t *node, size_type index)
    {
        if(index >= node->size)
        {
            return std::make_pair(nullptr, 0);
        }
        while(node->level > 0)
        {
            inner_node_t *inner_node = static_cast<inner_node_t *>(node);
            for(size_type i = 0; ; ++i)
            {
                if(index >= inner_node->children[i]->size)
                {
                    index -= inner_node->children[i]->size;
                }
                else
                {
                    node = inner_node->children[i];
                    break;
                }
            }
        }
        return std::make_pair(static_cast<leaf_node_t *>(node), index);
    }

    template<typename node_type> size_type lower_bound_(node_type *node, key_type const &key) const
    {
        if(std::is_scalar<key_type>::value && size_type(node_type::max) < size_type(binary_search_limit))
        {
            typename node_type::item_type const *begin = node->item, *const end = node->item + node->bound();
            while(begin != end && get_comparator_()(get_key_t()(*begin), key))
            {
                ++begin;
            }
            return begin - node->item;
        }
        else
        {
            return std::lower_bound(node->item, node->item + node->bound(), key, [&](typename node_type::item_type const &left, key_type const &right)->bool
            {
                return get_comparator_()(get_key_t()(left), right);
            }) - node->item;
        }
    }
    template<typename node_type> size_type upper_bound_(node_type *node, key_type const &key) const
    {
        if(std::is_scalar<key_type>::value && size_type(node_type::max) < size_type(binary_search_limit))
        {
            typename node_type::item_type const *begin = node->item, *const end = node->item + node->bound();
            while(begin != end && !get_comparator_()(key, get_key_t()(*begin)))
            {
                ++begin;
            }
            return begin - node->item;
        }
        else
        {
            return std::upper_bound(node->item, node->item + node->bound(), key, [&](key_type const &left, typename node_type::item_type const &right)->bool
            {
                return get_comparator_()(left, get_key_t()(right));
            }) - node->item;
        }
    }

    pair_pos_t lower_bound_(key_type const &key) const
    {
        node_t *node = root_.parent;
        if(node->size == 0)
        {
            return std::make_pair(nullptr, 0);
        }
        while(node->level > 0)
        {
            inner_node_t const *inner_node = static_cast<inner_node_t const *>(node);
            size_type where = lower_bound_(inner_node, key);
            node = inner_node->children[where];
        }
        leaf_node_t *leaf_node = static_cast<leaf_node_t *>(node);
        size_type where = lower_bound_(leaf_node, key);
        if(where >= leaf_node->bound())
        {
            return std::make_pair(nullptr, 0);
        }
        else
        {
            return std::make_pair(leaf_node, where);
        }
    }
    pair_pos_t upper_bound_(key_type const &key) const
    {
        node_t *node = root_.parent;
        if(node->size == 0)
        {
            return std::make_pair(nullptr, 0);
        }
        while(node->level > 0)
        {
            inner_node_t const *inner_node = static_cast<inner_node_t const *>(node);
            size_type where = upper_bound_(inner_node, key);
            node = inner_node->children[where];
        }
        leaf_node_t *leaf_node = static_cast<leaf_node_t *>(node);
        size_type where = upper_bound_(leaf_node, key);
        if(where >= leaf_node->bound())
        {
            return std::make_pair(nullptr, 0);
        }
        else
        {
            return std::make_pair(leaf_node, where);
        }
    }

    template<class iterator_t, class in_value_t> static void construct_one_(iterator_t where, in_value_t &&value)
    {
        b_plus_plus_tree_detail::construct_one(where, std::forward<in_value_t>(value), typename b_plus_plus_tree_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_t> static void destroy_one_(iterator_t where)
    {
        b_plus_plus_tree_detail::destroy_one(where, typename b_plus_plus_tree_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_t> static void destroy_range_(iterator_t destroy_begin, iterator_t destroy_end)
    {
        b_plus_plus_tree_detail::destroy_range(destroy_begin, destroy_end, typename b_plus_plus_tree_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_from_t, class iterator_to_t> static void move_forward_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        b_plus_plus_tree_detail::move_forward(move_begin, move_end, to_begin, typename b_plus_plus_tree_detail::get_tag<iterator_from_t>::type());
    }

    template<class iterator_from_t, class iterator_to_t> static void move_construct_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        b_plus_plus_tree_detail::move_construct(move_begin, move_end, to_begin, typename b_plus_plus_tree_detail::get_tag<iterator_from_t>::type());
    }

    template<class iterator_t> static void move_next_to_and_construct_(iterator_t move_begin, iterator_t move_end, iterator_t to_begin)
    {
        b_plus_plus_tree_detail::move_next_to_and_construct(move_begin, move_end, to_begin, typename b_plus_plus_tree_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_from_t, class iterator_to_t> static void move_and_destroy_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        b_plus_plus_tree_detail::move_and_destroy(move_begin, move_end, to_begin, typename b_plus_plus_tree_detail::get_tag<iterator_from_t>::type());
    }

    template<class iterator_from_t, class iterator_to_t> static void move_construct_and_destroy_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        b_plus_plus_tree_detail::move_construct_and_destroy(move_begin, move_end, to_begin, typename b_plus_plus_tree_detail::get_tag<iterator_from_t>::type());
    }

    template<class iterator_t, class in_value_t> static void move_next_and_insert_one_(iterator_t move_begin, iterator_t move_end, in_value_t &&value)
    {
        b_plus_plus_tree_detail::move_next_and_insert_one(move_begin, move_end, std::forward<in_value_t>(value), typename b_plus_plus_tree_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_t> static void move_prev_and_destroy_one_(iterator_t move_begin, iterator_t move_end)
    {
        b_plus_plus_tree_detail::move_prev_and_destroy_one(move_begin, move_end, typename b_plus_plus_tree_detail::get_tag<iterator_t>::type());
    }

    static size_type update_parent_(node_t **update_begin, node_t **update_end, node_t *parent)
    {
        size_type count = 0;
        while(update_begin != update_end)
        {
            node_t *node = *update_begin++;
            count += node->size;
            node->parent = parent;
        }
        return count;
    }

    template<class in_value_t> pair_posi_t insert_first_(in_value_t &&value)
    {
        leaf_node_t *node = alloc_leaf_node_();
        construct_one_(node->item, std::forward<in_value_t>(value));
        node->bound() = 1;
        root_.parent = root_.left = root_.right = node;
        node->parent = node->next = node->prev = &root_;
        return std::make_pair(std::make_pair(node, 0), true);
    }

    template<class in_value_t> pair_posi_t insert_hint_(leaf_node_t *leaf_node, size_type where, in_value_t &&value)
    {
        bool is_leftish = false;
        pair_pos_t other;
        if(root_.parent->size == 0)
        {
            return insert_first_(std::forward<in_value_t>(value));
        }
        else if(config_t::unique_type::value)
        {
            if(leaf_node == root_.left && where == 0)
            {
                if(get_comparator_()(get_key_t()(value), get_key_t()(leaf_node->item[where])))
                {
                    return insert_pos_(leaf_node, 0, std::forward<in_value_t>(value));
                }
            }
            else if(leaf_node == nullptr)
            {
                leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
                if(get_comparator_()(get_key_t()(tail->item[tail->bound() - 1]), get_key_t()(value)))
                {
                    return insert_pos_(tail, tail->bound(), std::forward<in_value_t>(value));
                }
            }
            else if(get_comparator_()(get_key_t()(value), get_key_t()(leaf_node->item[where])) && get_comparator_()(get_key_t()(other = advance_prev_(std::make_pair(leaf_node, where))), get_key_t()(value)))
            {
                return insert_pos_(leaf_node, where, std::forward<in_value_t>(value));
            }
            else if(get_comparator_()(get_key_t()(leaf_node->item[where]), get_key_t()(value)) && ((other = advance_next_(std::make_pair(leaf_node, where))).first == nullptr || get_comparator_()(get_key_t()(value), get_key_t()(other))))
            {
                if(other.first == nullptr)
                {
                    leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
                    return insert_pos_(tail, tail->bound(), std::forward<in_value_t>(value));
                }
                else
                {
                    return insert_pos_(other.first, other.second, std::forward<in_value_t>(value));
                }
            }
        }
        else
        {
            if(leaf_node == root_.left && where == 0)
            {
                if(!get_comparator_()(get_key_t()(leaf_node->item[where]), get_key_t()(value)))
                {
                    return insert_pos_(leaf_node, 0, std::forward<in_value_t>(value));
                }
                is_leftish = true;
            }
            else if(leaf_node == nullptr)
            {
                leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
                if(!get_comparator_()(get_key_t()(value), get_key_t()(tail->item[tail->bound() - 1])))
                {
                    return insert_pos_(tail, tail->bound(), std::forward<in_value_t>(value));
                }
            }
            else if(!get_comparator_()(get_key_t()(leaf_node->item[where]), get_key_t()(value)) && !get_comparator_()(get_key_t()(value), get_key_t()(other = advance_prev_(std::make_pair(leaf_node, where)))))
            {
                return insert_pos_(leaf_node, where, std::forward<in_value_t>(value));
            }
            else if(!get_comparator_()(get_key_t()(value), get_key_t()(leaf_node->item[where])) && ((other = advance_next_(std::make_pair(leaf_node, where))).first == nullptr || !get_comparator_()(get_key_t()(other), get_key_t()(value))))
            {
                if(other.first == nullptr)
                {
                    leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
                    return insert_pos_(tail, tail->bound(), std::forward<in_value_t>(value));
                }
                else
                {
                    return insert_pos_(other.first, other.second, std::forward<in_value_t>(value));
                }
            }
            else
            {
                is_leftish = true;
            }
        }
        if(is_leftish)
        {
            return insert_nohint_<true>(std::forward<in_value_t>(value));
        }
        else
        {
            return insert_nohint_<false>(std::forward<in_value_t>(value));
        }
    }

    template<class in_value_t> pair_posi_t insert_pos_(leaf_node_t *leaf_node, size_type where, in_value_t &&value)
    {
        key_stack_t key_out;
        node_t *split_node = nullptr;
        inner_node_t *parent = nullptr;
        size_type parent_where;
        if(leaf_node->is_full())
        {
            parent_where = get_parent_(leaf_node, parent);
            split_leaf_node_(leaf_node, &key_out, split_node);
            if(where >= leaf_node->bound())
            {
                where -= leaf_node->bound();
                leaf_node = static_cast<leaf_node_t *>(split_node);
            }
        }
        move_next_and_insert_one_(leaf_node->item + where, leaf_node->item + leaf_node->bound(), std::forward<in_value_t>(value));
        ++leaf_node->bound();
        if(split_node != nullptr && leaf_node != split_node && where == leaf_node->bound() - 1)
        {
            key_out = get_key_t()(leaf_node->item[where]);
        }
        if(split_node == nullptr)
        {
            for(node_t *parent = leaf_node->parent; parent->size != 0; parent = parent->parent)
            {
                ++parent->size;
            }
        }
        else
        {
            insert_pos_descend_(parent, parent_where, std::move(key_out), split_node);
        }
        return std::make_pair(std::make_pair(leaf_node, where), true);
    }

    void insert_pos_descend_(inner_node_t *inner_node, size_type where, key_stack_t &&key_out, node_t *new_child)
    {
        if(inner_node == nullptr)
        {
            inner_node_t *new_root = alloc_inner_node_(&root_, root_.parent->level + 1);
            construct_one_(new_root->item, std::move(key_out.key()));
            destroy_one_(&key_out);
            new_root->children[0] = root_.parent;
            new_root->children[1] = new_child;
            new_root->bound() = 1;
            new_root->size = update_parent_(new_root->children, new_root->children + 2, new_root);
            root_.parent = new_root;
            return;
        }
        key_stack_t split_key_out;
        node_t *split_node = nullptr;
        inner_node_t *parent = nullptr;
        size_type parent_where;
        ++inner_node->size;
        do
        {
            if(inner_node->is_full())
            {
                parent_where = get_parent_(inner_node, parent);
                split_inner_node_(inner_node, &split_key_out, split_node, where);
                inner_node_t *split_tree_node = static_cast<inner_node_t *>(split_node);
                if(where == inner_node->bound() + 1 && inner_node->bound() < split_tree_node->bound())
                {
                    construct_one_(inner_node->item + inner_node->bound(), std::move(split_key_out.key()));
                    inner_node->children[inner_node->bound() + 1] = split_tree_node->children[0];
                    ++inner_node->bound();
                    inner_node->size = inner_node->size + split_tree_node->children[0]->size - new_child->size;
                    split_tree_node->size = split_tree_node->size - split_tree_node->children[0]->size + new_child->size;
                    split_tree_node->children[0]->parent = inner_node;
                    new_child->parent = split_tree_node;
                    split_tree_node->children[0] = new_child;
                    split_key_out = std::move(key_out.key());
                    destroy_one_(&key_out);
                    break;
                }
                else if(where >= size_type(inner_node->bound() + 1))
                {
                    where -= inner_node->bound() + 1;
                    inner_node->size -= new_child->size;
                    split_tree_node->size += new_child->size;
                    inner_node = split_tree_node;
                }
            }
            move_next_and_insert_one_(inner_node->item + where, inner_node->item + inner_node->bound(), std::move(key_out.key()));
            destroy_one_(&key_out);
            std::move_backward(inner_node->children + where, inner_node->children + inner_node->bound() + 1, inner_node->children + inner_node->bound() + 2);
            inner_node->children[where + 1] = new_child;
            inner_node->bound()++;
            new_child->parent = inner_node;
        }
        while(false);
        if(split_node == nullptr)
        {
            for(node_t *parent = inner_node->parent; parent->size != 0; parent = parent->parent)
            {
                ++parent->size;
            }
        }
        else
        {
            insert_pos_descend_(parent, parent_where, std::move(split_key_out), split_node);
        }
    }

    template<bool is_leftish, class in_value_t> pair_posi_t insert_nohint_(in_value_t &&value)
    {
        if(root_.parent->size == 0)
        {
            return insert_first_(std::forward<in_value_t>(value));
        }
        leaf_node_t *leaf_node;
        size_type where;
        std::tie(leaf_node, where) = is_leftish ? lower_bound_(get_key_t()(value)) : upper_bound_(get_key_t()(value));
        if(leaf_node == nullptr)
        {
            leaf_node = static_cast<leaf_node_t *>(root_.right);
            where = leaf_node->bound();
        }
        if(config_t::unique_type::value && (where > 0 || leaf_node->prev != &root_))
        {
            if(where == 0)
            {
                leaf_node_t *prev_leaf_node = static_cast<leaf_node_t *>(leaf_node->prev);
                if(!get_comparator_()(get_key_t()(prev_leaf_node->item[prev_leaf_node->bound() - 1]), get_key_t()(value)))
                {
                    return std::make_pair(std::make_pair(prev_leaf_node, prev_leaf_node->bound() - 1), false);
                }
            }
            else
            {
                if(!get_comparator_()(get_key_t()(leaf_node->item[where - 1]), get_key_t()(value)))
                {
                    return std::make_pair(std::make_pair(leaf_node, where - 1), false);
                }
            }
        }
        return insert_pos_(leaf_node, where, std::forward<in_value_t>(value));
    }

    void split_inner_node_(inner_node_t *inner_node, key_type *key_ptr, node_t *&new_node, size_type where)
    {
        size_type mid = (inner_node->bound() >> 1);
        if(where <= mid && mid > inner_node->bound() - (mid + 1))
        {
            --mid;
        }
        inner_node_t *new_inner_node = alloc_inner_node_(inner_node->parent, inner_node->level);
        new_inner_node->bound() = inner_node->bound() - (mid + 1);
        move_construct_and_destroy_(inner_node->item + mid + 1, inner_node->item + inner_node->bound(), new_inner_node->item);
        std::copy(inner_node->children + mid + 1, inner_node->children + inner_node->bound() + 1, new_inner_node->children);
        inner_node->bound() = mid;
        construct_one_(key_ptr, inner_node->item[mid]);
        destroy_one_(inner_node->item + mid);
        size_type count = update_parent_(new_inner_node->children, new_inner_node->children + new_inner_node->bound() + 1, new_inner_node);
        new_inner_node->size = count;
        inner_node->size -= count;
        new_node = new_inner_node;
    }

    void split_leaf_node_(leaf_node_t *leaf_node, key_type *key_ptr, node_t *&new_node)
    {
        size_type mid = (leaf_node->bound() >> 1);
        leaf_node_t *new_leaf_node = alloc_leaf_node_();
        new_leaf_node->bound() = leaf_node->bound() - mid;
        new_leaf_node->next = leaf_node->next;
        if(new_leaf_node->next->size == 0)
        {
            root_.right = new_leaf_node;
        }
        else
        {
            static_cast<leaf_node_t *>(new_leaf_node->next)->prev = new_leaf_node;
        }
        move_construct_and_destroy_(leaf_node->item + mid, leaf_node->item + leaf_node->bound(), new_leaf_node->item);
        leaf_node->bound() = mid;
        leaf_node->next = new_leaf_node;
        new_leaf_node->prev = leaf_node;
        construct_one_(key_ptr, get_key_t()(leaf_node->item[leaf_node->bound() - 1]));
        new_node = new_leaf_node;
    }

    result_t merge_leaves_(leaf_node_t *left, leaf_node_t *right, inner_node_t *parent)
    {
        (void)parent;
        move_construct_and_destroy_(right->item, right->item + right->bound(), left->item + left->bound());
        left->bound() += right->bound();
        left->next = right->next;
        if(left->next != &root_)
        {
            static_cast<leaf_node_t *>(left->next)->prev = left;
        }
        else
        {
            root_.right = left;
        }
        right->bound() = 0;
        right->parent = nullptr;
        return result_t(btree_fixmerge);
    }

    static result_t shift_left_leaf_(leaf_node_t *left, leaf_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (right->bound() - left->bound()) >> 1;
        move_construct_(right->item, right->item + shiftnum, left->item + left->bound());
        left->bound() += shiftnum;
        move_forward_(right->item + shiftnum, right->item + right->bound(), right->item);
        destroy_range_(right->item + right->bound() - shiftnum, right->item + right->bound());
        right->bound() -= shiftnum;
        if(parent_where < parent->bound())
        {
            parent->item[parent_where] = get_key_t()(left->item[left->bound() - 1]);
            return result_t(btree_ok);
        }
        else
        {
            return result_t(btree_update_lastkey, get_key_t()(left->item[left->bound() - 1]));
        }
    }

    static void shift_right_leaf_(leaf_node_t *left, leaf_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (left->bound() - right->bound()) >> 1;
        move_next_to_and_construct_(right->item, right->item + right->bound(), right->item + shiftnum);
        right->bound() += shiftnum;
        move_and_destroy_(left->item + left->bound() - shiftnum, left->item + left->bound(), right->item);
        left->bound() -= shiftnum;
        parent->item[parent_where] = get_key_t()(left->item[left->bound() - 1]);
    }

    static result_t merge_inners_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        construct_one_(left->item + left->bound(), parent->item[parent_where]);
        ++left->bound();
        move_construct_and_destroy_(right->item, right->item + right->bound(), left->item + left->bound());
        std::copy(right->children, right->children + right->bound() + 1, left->children + left->bound());
        left->size += update_parent_(left->children + left->bound(), left->children + left->bound() + right->bound() + 1, left);
        left->bound() += right->bound();
        right->bound() = 0;
        right->size = 0;
        right->parent = nullptr;
        return result_t(btree_fixmerge);
    }

    static void shift_left_inner_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (right->bound() - left->bound()) >> 1;
        construct_one_(left->item + left->bound(), parent->item[parent_where]);
        ++left->bound();
        move_construct_(right->item, right->item + shiftnum - 1, left->item + left->bound());
        std::copy(right->children, right->children + shiftnum, left->children + left->bound());
        size_t count = update_parent_(left->children + left->bound(), left->children + left->bound() + shiftnum, left);
        left->bound() += shiftnum - 1;
        left->size += count;
        parent->item[parent_where] = right->item[shiftnum - 1];
        move_forward_(right->item + shiftnum, right->item + right->bound(), right->item);
        destroy_range_(right->item + right->bound() - shiftnum, right->item + right->bound());
        std::copy(right->children + shiftnum, right->children + right->bound() + 1, right->children);
        right->bound() -= shiftnum;
        right->size -= count;
    }

    static void shift_right_inner_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (left->bound() - right->bound()) >> 1;
        move_next_to_and_construct_(right->item, right->item + right->bound(), right->item + shiftnum);
        std::copy_backward(right->children, right->children + right->bound() + 1, right->children + right->bound() + 1 + shiftnum);
        right->bound() += shiftnum;
        right->item[shiftnum - 1] = parent->item[parent_where];
        move_and_destroy_(left->item + left->bound() - shiftnum + 1, left->item + left->bound(), right->item);
        std::copy(left->children + left->bound() - shiftnum + 1, left->children + left->bound() + 1, right->children);
        size_t count = update_parent_(right->children, right->children + shiftnum, right);
        parent->item[parent_where] = left->item[left->bound() - shiftnum];
        left->bound() -= shiftnum;
        left->size -= count;
        right->size += count;
    }

    size_type get_parent_(node_t *node, inner_node_t *&parent)
    {
        if(node->parent->size == 0)
        {
            parent = nullptr;
            return 0;
        }
        parent = static_cast<inner_node_t *>(node->parent);
        return std::find(parent->children, parent->children + parent->bound() + 1, node) - parent->children;
    }

    template<class in_node_t> in_node_t *get_left_(in_node_t *node)
    {
        inner_node_t *parent;
        size_type where = get_parent_(node, parent);
        if(parent == nullptr)
        {
            return nullptr;
        }
        if(where == 0)
        {
            in_node_t *left_parent = get_left_(parent);
            return left_parent == nullptr ? nullptr : static_cast<in_node_t *>(left_parent->children[left_parent->bound() - 1]);
        }
        else
        {
            return static_cast<in_node_t *>(parent->children[where - 1]);
        }
    }

    template<class in_node_t> in_node_t *get_right_(in_node_t *node)
    {
        inner_node_t *parent;
        size_type where = get_parent_(node, parent);
        if(parent == nullptr)
        {
            return nullptr;
        }
        if(where == parent->bound())
        {
            in_node_t *right_parent = get_right_(parent);
            return right_parent == nullptr ? nullptr : static_cast<in_node_t *>(right_parent->children[0]);
        }
        else
        {
            return static_cast<in_node_t *>(parent->children[where + 1]);
        }
    }

    template<class in_node_t> void get_left_right_parent_(inner_node_t *parent, size_type where, in_node_t *&left, inner_node_t *&left_parent, in_node_t *&right, inner_node_t *&right_parent)
    {
        if(parent == nullptr)
        {
            left = right = nullptr;
            left_parent = right_parent = nullptr;
            return;
        }
        if(where == 0)
        {
            left_parent = get_left_(parent);
            left = left_parent == nullptr ? nullptr : static_cast<in_node_t *>(left_parent->children[left_parent->bound() - 1]);
        }
        else
        {
            left_parent = parent;
            left = static_cast<in_node_t *>(parent->children[where - 1]);
        }
        if(where == parent->bound())
        {
            right_parent = get_right_(parent);
            right = right_parent == nullptr ? nullptr : static_cast<in_node_t *>(right_parent->children[0]);
        }
        else
        {
            right_parent = parent;
            right = static_cast<in_node_t *>(parent->children[where + 1]);
        }
    }

    void erase_pos_(leaf_node_t *leaf_node, size_type where)
    {
        move_prev_and_destroy_one_(leaf_node->item + where + 1, leaf_node->item + leaf_node->bound());
        leaf_node->bound()--;
        result_t result(btree_ok);
        inner_node_t *parent = nullptr;
        size_type parent_where;
        if(where == leaf_node->bound())
        {
            parent_where = get_parent_(leaf_node, parent);
            if(parent != nullptr && parent_where < parent->bound())
            {
                parent->item[parent_where] = get_key_t()(leaf_node->item[leaf_node->bound() - 1]);
            }
            else if(leaf_node->bound() >= 1)
            {
                result |= result_t(btree_update_lastkey, get_key_t()(leaf_node->item[leaf_node->bound() - 1]));
            }
        }
        if(leaf_node->is_underflow() && !(leaf_node == root_.parent && leaf_node->bound() >= 1))
        {
            if(parent == nullptr)
            {
                parent_where = get_parent_(leaf_node, parent);
            }
            leaf_node_t *leaf_left, *leaf_right;
            inner_node_t *left_parent, *right_parent;
            get_left_right_parent_(parent, parent_where, leaf_left, left_parent, leaf_right, right_parent);
            if(leaf_left == nullptr && leaf_right == nullptr)
            {
                free_node_<false>(root_.parent);
                root_.parent = root_.left = root_.right = &root_;
                return;
            }
            else if((leaf_left == nullptr || leaf_left->is_few()) && (leaf_right == nullptr || leaf_right->is_few()))
            {
                if(left_parent == parent)
                {
                    result |= merge_leaves_(leaf_left, leaf_node, left_parent);
                }
                else
                {
                    result |= merge_leaves_(leaf_node, leaf_right, right_parent);
                }
            }
            else if((leaf_left != nullptr && leaf_left->is_few()) && (leaf_right != nullptr && !leaf_right->is_few()))
            {
                if(right_parent == parent)
                {
                    result |= shift_left_leaf_(leaf_node, leaf_right, right_parent, parent_where);
                }
                else
                {
                    result |= merge_leaves_(leaf_left, leaf_node, left_parent);
                }
            }
            else if((leaf_left != nullptr && !leaf_left->is_few()) && (leaf_right != nullptr && leaf_right->is_few()))
            {
                if(left_parent == parent)
                {
                    shift_right_leaf_(leaf_left, leaf_node, left_parent, parent_where - 1);
                }
                else
                {
                    result |= merge_leaves_(leaf_node, leaf_right, right_parent);
                }
            }
            else if(left_parent == right_parent)
            {
                if(leaf_left->bound() <= leaf_right->bound())
                {
                    result |= shift_left_leaf_(leaf_node, leaf_right, right_parent, parent_where);
                }
                else
                {
                    shift_right_leaf_(leaf_left, leaf_node, left_parent, parent_where - 1);
                }
            }
            else
            {
                if(left_parent == parent)
                {
                    shift_right_leaf_(leaf_left, leaf_node, left_parent, parent_where - 1);
                }
                else
                {
                    result |= shift_left_leaf_(leaf_node, leaf_right, right_parent, parent_where);
                }
            }
        }
        if(result.has(result_flags_t(btree_update_lastkey | btree_fixmerge)))
        {
            if(parent != nullptr)
            {
                erase_pos_descend_(parent, parent_where, std::move(result));
            }
        }
        else
        {
            for(node_t *parent = leaf_node->parent; parent->size != 0; parent = parent->parent)
            {
                --parent->size;
            }
        }
    }

    void erase_pos_descend_(inner_node_t *inner_node, size_type where, result_t &&result)
    {
        --inner_node->size;
        result_t self_result(btree_ok);
        inner_node_t *parent = nullptr;
        size_type parent_where;
        if(result.has(btree_update_lastkey))
        {
            parent_where = get_parent_(inner_node, parent);
            if(parent != nullptr && parent_where < parent->bound())
            {
                parent->item[parent_where] = std::move(result.last_key.key());
            }
            else
            {
                self_result |= result_t(btree_update_lastkey, std::move(result.last_key));
            }
        }
        if(result.has(btree_fixmerge))
        {
            if(parent == nullptr)
            {
                parent_where = get_parent_(inner_node, parent);
            }
            if(inner_node->children[where]->parent != nullptr)
            {
                ++where;
            }
            free_node_<false>(inner_node->children[where]);
            move_prev_and_destroy_one_(inner_node->item + where, inner_node->item + inner_node->bound());
            std::copy(inner_node->children + where + 1, inner_node->children + inner_node->bound() + 1, inner_node->children + where);
            --inner_node->bound();
            if(inner_node->level == 1)
            {
                --where;
                leaf_node_t *child = static_cast<leaf_node_t *>(inner_node->children[where]);
                inner_node->item[where] = get_key_t()(child->item[child->bound() - 1]);
            }
        }
        if(inner_node->is_underflow() && !(inner_node == root_.parent && inner_node->bound() >= 1))
        {
            inner_node_t *inner_left, *inner_right;
            inner_node_t *left_parent, *right_parent;
            get_left_right_parent_(parent, parent_where, inner_left, left_parent, inner_right, right_parent);
            if(inner_left == nullptr && inner_right == nullptr)
            {
                root_.parent = inner_node->children[0];
                root_.parent->parent = &root_;
                inner_node->bound() = 0;
                free_node_<false>(inner_node);
                return;
            }
            else if((inner_left == nullptr || inner_left->is_few()) && (inner_right == nullptr || inner_right->is_few()))
            {
                if(left_parent == parent)
                {
                    self_result |= merge_inners_(inner_left, inner_node, left_parent, parent_where - 1);
                }
                else
                {
                    self_result |= merge_inners_(inner_node, inner_right, right_parent, parent_where);
                }
            }
            else if((inner_left != nullptr && inner_left->is_few()) && (inner_right != nullptr && !inner_right->is_few()))
            {
                if(right_parent == parent)
                {
                    shift_left_inner_(inner_node, inner_right, right_parent, parent_where);
                }
                else
                {
                    self_result |= merge_inners_(inner_left, inner_node, left_parent, parent_where - 1);
                }
            }
            else if((inner_left != nullptr && !inner_left->is_few()) && (inner_right != nullptr && inner_right->is_few()))
            {
                if(left_parent == parent)
                {
                    shift_right_inner_(inner_left, inner_node, left_parent, parent_where - 1);
                }
                else
                {
                    self_result |= merge_inners_(inner_node, inner_right, right_parent, parent_where);
                }
            }
            else if(left_parent == right_parent)
            {
                if(inner_left->bound() <= inner_right->bound())
                {
                    shift_left_inner_(inner_node, inner_right, right_parent, parent_where);
                }
                else
                {
                    shift_right_inner_(inner_left, inner_node, left_parent, parent_where - 1);
                }
            }
            else
            {
                if(left_parent == parent)
                {
                    shift_right_inner_(inner_left, inner_node, left_parent, parent_where - 1);
                }
                else
                {
                    shift_left_inner_(inner_node, inner_right, right_parent, parent_where);
                }
            }
        }
        if(self_result.has(result_flags_t(btree_update_lastkey | btree_fixmerge)))
        {
            if(parent != nullptr)
            {
                erase_pos_descend_(parent, parent_where, std::move(self_result));
            }
        }
        else
        {
            for(node_t *parent = inner_node->parent; parent->size != 0; parent = parent->parent)
            {
                --parent->size;
            }
        }
    }
};
