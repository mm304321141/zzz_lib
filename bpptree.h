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
    class move_trivial_tag
    {
    };
    class move_assign_tag
    {
    };
    template<class T> struct is_trivial_expand : public std::is_trivial<T>
    {
    };
    template<class K, class V> struct is_trivial_expand<std::pair<K, V>> : public std::conditional<std::is_trivial<K>::value && std::is_trivial<V>::value, std::true_type, std::false_type>::type
    {
    };
    template<class iterator_t> struct get_tag
    {
        typedef typename std::conditional<is_trivial_expand<typename std::iterator_traits<iterator_t>::value_type>::value, move_trivial_tag, move_assign_tag>::type type;
    };

    template<class iterator_t, class in_value_t, class tag_t> void construct_one(iterator_t where, in_value_t &&value, tag_t)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        ::new(std::addressof(*where)) iterator_value_t(std::forward<in_value_t>(value));
    }

    template<class iterator_t> void destroy_one(iterator_t, move_trivial_tag)
    {
    }
    template<class iterator_t> void destroy_one(iterator_t where, move_assign_tag)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        where->~iterator_value_t();
    }

    template<class iterator_t> void destroy_range(iterator_t, iterator_t, move_trivial_tag)
    {
    }
    template<class iterator_t> void destroy_range(iterator_t destroy_begin, iterator_t destroy_end, move_assign_tag)
    {
        for(; destroy_begin != destroy_end; ++destroy_begin)
        {
            destroy_one(destroy_begin, move_assign_tag());
        }
    }

    template<class iterator_from_t, class iterator_to_t> void move_forward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*to_begin, &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_forward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        std::move(move_begin, move_end, to_begin);
    }

    template<class iterator_from_t, class iterator_to_t> void move_backward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*to_begin, &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_backward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        std::move_backward(move_begin, move_end, to_begin + (move_end - move_begin));
    }

    template<class iterator_from_t, class iterator_to_t> void move_construct(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*to_begin, &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        std::uninitialized_copy(std::make_move_iterator(move_begin), std::make_move_iterator(move_end), to_begin);
    }

    template<class iterator_t> void move_next_to_and_construct(iterator_t move_begin, iterator_t move_end, iterator_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*to_begin, &*move_begin, count * sizeof(*move_begin));
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
            std::uninitialized_fill(move_end, to_begin, iterator_value_t());
        }
    }

    template<class iterator_from_t, class iterator_to_t> void move_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*to_begin, &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        for(; move_begin != move_end; ++move_begin)
        {
            *to_begin++ = std::move(*move_begin);
            destroy_one(move_begin, move_assign_tag());
        }
    }

    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*to_begin, &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        for(; move_begin != move_end; ++move_begin)
        {
            construct_one(to_begin++, std::move(*move_begin), move_assign_tag());
            destroy_one(move_begin, move_assign_tag());
        }
    }

    template<class iterator_t, class in_value_t> void move_next_and_insert_one(iterator_t move_begin, iterator_t move_end, in_value_t &&value, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*(move_begin + 1), &*move_begin, count * sizeof(*move_begin));
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
            iterator_t from_end = std::prev(move_end);
            construct_one(move_end, std::move(*from_end), move_assign_tag());
            move_backward(move_begin, from_end, move_begin + 1, move_assign_tag());
            *move_begin = std::forward<in_value_t>(value);
        }
    }

    template<class iterator_t> void move_prev_and_destroy_one(iterator_t move_begin, iterator_t move_end, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*(move_begin - 1), &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_t> void move_prev_and_destroy_one(iterator_t move_begin, iterator_t move_end, move_assign_tag)
    {
        move_forward(move_begin, move_end, move_begin - 1, move_assign_tag());
        destroy_one(move_end - 1, move_assign_tag());
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
        size_t level;
    };
    struct child_slot_t
    {
        size_t size;
        node_t *ptr;
    };
    struct inner_node_t : public node_t
    {
        typedef key_type item_type;
        enum
        {
            max = ((config_t::memory_block_size - sizeof(node_t) - sizeof(child_slot_t)) / (sizeof(key_type) + sizeof(child_slot_t))),
            min = max / 2,
        };
        child_slot_t children[max + 1];
        key_type item[max];

        // The "used" count is no longer stored. It is encoded by the nullptr
        // sentinel invariant: children[0..key_count] hold valid pointers. Scan
        // backward over empty slots until reaching the last valid child slot.
        size_t key_count() const
        {
            size_t i = max;
            while(i > 0 && children[i].ptr == nullptr)
                --i;
            return i;
        }
        bool is_full() const
        {
            return children[max].ptr != nullptr;
        }
        // is_few(): key_count <= min. Equivalent to children[min + 1] being unused,
        // because children[k] == nullptr iff key_count < k.
        bool is_few() const
        {
            return children[min + 1].ptr == nullptr;
        }
        // is_underflow(): key_count < min, i.e. children[min] is unused.
        bool is_underflow() const
        {
            return children[min].ptr == nullptr;
        }
    };
    struct leaf_node_t : public node_t
    {
        typedef storage_type item_type;
        enum
        {
            max = (config_t::memory_block_size - sizeof(node_t) - sizeof(node_t *) * 2) / sizeof(storage_type),
            min = max / 2,
        };
        node_t *prev;
        node_t *next;
        storage_type item[max];
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
        static void change_inner(status_t &, difference_type, size_type)
        {
        }
    };
    typedef status_control_select_t<typename config_t::status_type::type, void> status_control_t;
    typedef typename std::aligned_union<config_t::memory_block_size, inner_node_t, leaf_node_t>::type memory_node_t;
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<memory_node_t> node_allocator_t;
    struct root_node_t : public node_t, public key_compare, public node_allocator_t, public status_t
    {
        template<class any_key_compare, class any_allocator_t> root_node_t(any_key_compare &&comp, any_allocator_t &&alloc) : key_compare(std::forward<any_key_compare>(comp)), node_allocator_t(std::forward<any_allocator_t>(alloc)), status_t()
        {
            static_assert(inner_node_t::max >= 4, "low memory_block_size");
            static_assert(leaf_node_t::max >= 4, "low memory_block_size");
            static_assert(sizeof(inner_node_t) <= config_t::memory_block_size, "bad memory size");
            static_assert(sizeof(leaf_node_t) <= config_t::memory_block_size, "bad memory size");
            node_t::parent = left = right = this;
            size = 0;
            node_t::level = 0;
        }
        size_t size;
        node_t *left;
        node_t *right;
    };
    struct key_stack_t
    {
        typename std::aligned_storage<sizeof(key_type), std::alignment_of<key_type>::value>::type key_pod;
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
        key_type *operator&()
        {
            return reinterpret_cast<key_type *>(&key_pod);
        }
        key_stack_t &operator=(key_stack_t &&other)
        {
            key() = std::move(other.key());
            return *this;
        }
        key_stack_t &operator=(key_stack_t const &other)
        {
            key() = other.key();
            return *this;
        }
        key_stack_t &operator=(key_type &&other)
        {
            key() = std::move(other);
            return *this;
        }
        key_stack_t &operator=(key_type const &other)
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
        result_t &operator|=(result_t &&other)
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
        result_t &operator=(result_t &&other)
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
        binary_search_limit = 16 * 1024
    };

public:
    template<bool IsConst, bool IsReverse> class iterator_impl
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename b_plus_plus_tree::value_type value_type;
        typedef typename b_plus_plus_tree::difference_type difference_type;
        typedef typename b_plus_plus_tree::reference reference;
        typedef typename b_plus_plus_tree::const_reference const_reference;
        typedef typename b_plus_plus_tree::pointer pointer;
        typedef typename b_plus_plus_tree::const_pointer const_pointer;
        typedef typename std::conditional<IsConst, const_reference, reference>::type deref_reference;
        typedef typename std::conditional<IsConst, const_pointer, pointer>::type deref_pointer;

    public:
        iterator_impl(node_t *in_node, size_type in_where, b_plus_plus_tree const *in_tree) : node(in_node), where(in_where), tree_(in_tree), leaf_bound_(0), inner_index_(0)
        {
            if(!in_tree->is_root_sentinel_(in_node))
                in_tree->find_leaf_in_parent_(static_cast<leaf_node_t const *>(in_node), leaf_bound_, inner_index_);
        }
        // non-const overload: builds a writable iterator over a mutable tree
        template<bool C = IsConst, typename std::enable_if<!C, int>::type = 0> iterator_impl(pair_pos_t pos, b_plus_plus_tree *self) : node(pos.first == nullptr ? static_cast<node_t *>(&self->root_) : static_cast<node_t *>(pos.first)), where(pos.second), tree_(self), leaf_bound_(0), inner_index_(0)
        {
            if(pos.first != nullptr)
                self->find_leaf_in_parent_(pos.first, leaf_bound_, inner_index_);
        }
        // const overload: derives the non-const sentinel pointer through parent->parent
        template<bool C = IsConst, typename std::enable_if<C, int>::type = 0> iterator_impl(pair_pos_t pos, b_plus_plus_tree const *self) : node(pos.first == nullptr ? self->root_.parent->parent : pos.first), where(pos.second), tree_(self), leaf_bound_(0), inner_index_(0)
        {
            if(pos.first != nullptr)
                self->find_leaf_in_parent_(pos.first, leaf_bound_, inner_index_);
        }
        iterator_impl(iterator_impl const &) = default;
        // implicit conversion: non-const -> const, same direction (mirrors original iterator -> const_iterator)
        template<bool WasConst, typename std::enable_if<!WasConst && IsConst, int>::type = 0> iterator_impl(iterator_impl<WasConst, IsReverse> const &other) : node(other.node), where(other.where), tree_(other.tree_), leaf_bound_(other.leaf_bound_), inner_index_(other.inner_index_)
        {
        }
        // explicit adaptor: forward -> reverse of the same const-ness (mirrors original explicit reverse_iterator(iterator))
        template<bool R = IsReverse, typename std::enable_if<R, int>::type = 0> explicit iterator_impl(iterator_impl<IsConst, false> const &other) : node(other.node), where(other.where), tree_(other.tree_), leaf_bound_(other.leaf_bound_), inner_index_(other.inner_index_)
        {
            ++*this;
        }
        iterator_impl &operator+=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, IsReverse ? -diff : diff);
            return *this;
        }
        iterator_impl &operator-=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, IsReverse ? diff : -diff);
            return *this;
        }
        iterator_impl operator+(difference_type diff) const
        {
            iterator_impl ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, IsReverse ? -diff : diff);
            return ret;
        }
        iterator_impl operator-(difference_type diff) const
        {
            iterator_impl ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, IsReverse ? diff : -diff);
            return ret;
        }
        difference_type operator-(iterator_impl const &other) const
        {
            if(IsReverse)
            {
                return static_cast<difference_type>(tree_->calculate_rank_(other.node, other.where)) - static_cast<difference_type>(tree_->calculate_rank_(node, where));
            }
            return static_cast<difference_type>(tree_->calculate_rank_(node, where)) - static_cast<difference_type>(tree_->calculate_rank_(other.node, other.where));
        }
        iterator_impl &operator++()
        {
            if(IsReverse)
            {
                tree_->advance_prev_(node, where, leaf_bound_, inner_index_);
            }
            else
            {
                tree_->advance_next_(node, where, leaf_bound_, inner_index_);
            }
            return *this;
        }
        iterator_impl &operator--()
        {
            if(IsReverse)
            {
                tree_->advance_next_(node, where, leaf_bound_, inner_index_);
            }
            else
            {
                tree_->advance_prev_(node, where, leaf_bound_, inner_index_);
            }
            return *this;
        }
        iterator_impl operator++(int)
        {
            iterator_impl save(*this);
            ++*this;
            return save;
        }
        iterator_impl operator--(int)
        {
            iterator_impl save(*this);
            --*this;
            return save;
        }
        deref_reference operator*() const
        {
            return reinterpret_cast<deref_reference>(static_cast<leaf_node_t *>(node)->item[where]);
        }
        deref_pointer operator->() const
        {
            return reinterpret_cast<deref_pointer>(static_cast<leaf_node_t *>(node)->item + where);
        }
        deref_reference operator[](difference_type index) const
        {
            return *(*this + index);
        }
        bool operator>(iterator_impl const &other) const
        {
            return *this - other > 0;
        }
        bool operator<(iterator_impl const &other) const
        {
            return *this - other < 0;
        }
        bool operator>=(iterator_impl const &other) const
        {
            return *this - other >= 0;
        }
        bool operator<=(iterator_impl const &other) const
        {
            return *this - other <= 0;
        }
        bool operator==(iterator_impl const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator!=(iterator_impl const &other) const
        {
            return node != other.node || where != other.where;
        }
        iterator_impl<IsConst, false> base() const
        {
            return ++iterator_impl<false, false>(node, where, tree_);
        }

    private:
        friend class b_plus_plus_tree;
        template<bool, bool> friend class iterator_impl;
        node_t *node;
        size_type where;
        b_plus_plus_tree const *tree_;
        size_type leaf_bound_;
        size_type inner_index_;
    };
    using iterator = iterator_impl<false, false>;
    using const_iterator = iterator_impl<true, false>;
    using reverse_iterator = iterator_impl<false, true>;
    using const_reverse_iterator = iterator_impl<true, true>;
    typedef typename std::conditional<config_t::unique_type::value, std::pair<iterator, bool>, iterator>::type insert_result_t;
    typedef std::pair<iterator, bool> pair_ib_t;

protected:
    typedef std::pair<pair_pos_t, bool> pair_posi_t;
    template<class unique_type> typename std::enable_if<unique_type::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return std::make_pair(iterator(posi.first, this), posi.second);
    }
    template<class unique_type> typename std::enable_if<!unique_type::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return iterator(posi.first, this);
    }

public:
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
    template<class iterator_t> b_plus_plus_tree(iterator_t begin, iterator_t end, key_compare const &comp = key_compare(), allocator_type const &alloc = allocator_type()) : root_(comp, alloc)
    {
        insert(begin, end);
    }
    //range
    template<class iterator_t> b_plus_plus_tree(iterator_t begin, iterator_t end, allocator_type const &alloc) : root_(key_compare(), alloc)
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
        insert(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
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
    b_plus_plus_tree &operator=(b_plus_plus_tree const &other)
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
    b_plus_plus_tree &operator=(b_plus_plus_tree &&other)
    {
        if(this == &other)
        {
            return *this;
        }
        swap(other);
        return *this;
    }
    //initializer list
    b_plus_plus_tree &operator=(std::initializer_list<value_type> il)
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
        return result_<std::false_type>(insert_hint_(is_root_sentinel_(hint.node) ? nullptr : static_cast<leaf_node_t *>(hint.node), hint.where, value));
    }
    //with hint
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(const_iterator hint, in_value_t &&value)
    {
        return result_<typename config_t::unique_type>(insert_hint_(is_root_sentinel_(hint.node) ? nullptr : static_cast<leaf_node_t *>(hint.node), hint.where, std::forward<in_value_t>(value)));
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
    template<class... args_t> insert_result_t emplace(args_t &&...args)
    {
        return result_<typename config_t::unique_type>(insert_nohint_<false>(std::move(storage_type(std::forward<args_t>(args)...))));
    }
    //with hint
    template<class... args_t> insert_result_t emplace_hint(const_iterator hint, args_t &&...args)
    {
        return result_<typename config_t::unique_type>(insert_hint_(is_root_sentinel_(hint.node) ? nullptr : static_cast<leaf_node_t *>(hint.node), hint.where, std::move(storage_type(std::forward<args_t>(args)...))));
    }

    template<class in_key_type> iterator find(in_key_type const &key)
    {
        pair_pos_t pos = lower_bound_(key);
        return (pos.first == nullptr || get_comparator_()(key, get_key_t()(pos.first->item[pos.second]))) ? iterator(&root_, 0, this) : iterator(pos.first, pos.second, this);
    }
    template<class in_key_type> const_iterator find(in_key_type const &key) const
    {
        pair_pos_t pos = lower_bound_(key);
        return (pos.first == nullptr || get_comparator_()(key, get_key_t()(pos.first->item[pos.second]))) ? iterator(root_.parent->parent, 0, this) : iterator(pos.first, pos.second, this);
    }

    template<class in_key_t, class = typename std::enable_if<std::is_convertible<in_key_t, key_type>::value && config_t::unique_type::value && !std::is_same<key_type, storage_type>::value, void>::type> mapped_type &operator[](in_key_t &&key)
    {
        pair_pos_t pos = lower_bound_(key);
        if(pos.first == nullptr || get_comparator_()(key, get_key_t()(pos.first->item[pos.second])))
        {
            pos = insert_hint_(pos.first, pos.second, std::make_pair(key, mapped_type())).first;
        }
        return pos.first->item[pos.second].second;
    }

    iterator erase(const_iterator it)
    {
        if(root_.size == 0)
        {
            return end();
        }
        size_type pos_at = rank(it);
        erase_pos_(static_cast<leaf_node_t *>(it.node), it.where, it.leaf_bound_);
        return at(pos_at);
    }
    size_type erase(key_type const &key)
    {
        if(root_.size == 0)
        {
            return 0;
        }
        size_type count = 0;
        leaf_node_t *leaf_node;
        size_type where;
        size_type leaf_size;
        while(true)
        {
            std::tie(leaf_node, where, leaf_size) = lower_bound_impl_(key);
            if(leaf_node == nullptr || where >= leaf_size || get_comparator_()(key, get_key_t()(leaf_node->item[where])))
            {
                break;
            }
            erase_pos_(leaf_node, where, leaf_size);
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
            if(erase_begin == erase_end)
            {
                return iterator(erase_begin.node, erase_begin.where, this);
            }
            size_type pos_begin = rank(erase_begin), pos_end = rank(erase_end);
            while(pos_begin != pos_end)
            {
                pair_pos_t pos = access_index_(root_.parent, --pos_end);
                erase_pos_(pos.first, pos.second, get_leaf_size_(pos.first));
            }
            return iterator(access_index_(root_.parent, pos_begin), this);
        }
    }

    template<class in_key_type> size_type count(in_key_type const &key) const
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

    template<class in_key_type> iterator lower_bound(in_key_type const &key)
    {
        return iterator(lower_bound_(key), this);
    }
    template<class in_key_type> const_iterator lower_bound(in_key_type const &key) const
    {
        return const_iterator(lower_bound_(key), this);
    }
    template<class in_key_type> iterator upper_bound(in_key_type const &key)
    {
        return iterator(upper_bound_(key), this);
    }
    template<class in_key_type> const_iterator upper_bound(in_key_type const &key) const
    {
        return const_iterator(upper_bound_(key), this);
    }

    template<class in_key_type> pair_ii_t equal_range(in_key_type const &key)
    {
        return pair_ii_t(iterator(lower_bound_(key), this), iterator(upper_bound_(key), this));
    }
    template<class in_key_type> pair_cici_t equal_range(in_key_type const &key) const
    {
        return pair_cici_t(const_iterator(lower_bound_(key), this), const_iterator(upper_bound_(key), this));
    }

    iterator begin()
    {
        return iterator(root_.left, 0, this);
    }
    iterator end()
    {
        return iterator(&root_, 0, this);
    }
    const_iterator begin() const
    {
        return const_iterator(root_.left, 0, this);
    }
    const_iterator end() const
    {
        return const_iterator(root_.parent->parent, 0, this);
    }
    const_iterator cbegin() const
    {
        return const_iterator(root_.left, 0, this);
    }
    const_iterator cend() const
    {
        return const_iterator(root_.parent->parent, 0, this);
    }
    reverse_iterator rbegin()
    {
        return reverse_iterator(root_.right, root_.size == 0 ? 0 : get_leaf_size_(static_cast<leaf_node_t *>(root_.right)) - 1, this);
    }
    reverse_iterator rend()
    {
        return reverse_iterator(&root_, 0, this);
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(root_.right, root_.size == 0 ? 0 : get_leaf_size_(static_cast<leaf_node_t *>(root_.right)) - 1, this);
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(root_.parent->parent, 0, this);
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(root_.right, root_.size == 0 ? 0 : get_leaf_size_(static_cast<leaf_node_t *>(root_.right)) - 1, this);
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(root_.parent->parent, 0, this);
    }

    reference front()
    {
        return reinterpret_cast<reference>(static_cast<leaf_node_t *>(root_.left)->item[0]);
    }
    reference back()
    {
        leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
        return reinterpret_cast<reference>(tail->item[get_leaf_size_(tail) - 1]);
    }

    const_reference front() const
    {
        return reinterpret_cast<const_reference>(static_cast<leaf_node_t *>(root_.left)->item[0]);
    }
    const_reference back() const
    {
        leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
        return reinterpret_cast<const_reference>(tail->item[get_leaf_size_(tail) - 1]);
    }

    bool empty() const
    {
        return root_.size == 0;
    }
    void clear()
    {
        if(root_.parent != &root_)
        {
            free_node_<true>(root_.parent);
            root_.parent = root_.left = root_.right = &root_;
            root_.size = 0;
        }
    }
    size_type size() const
    {
        return root_.size;
    }
    size_type max_size() const
    {
        return std::allocator_traits<node_allocator_t>::max_size(node_allocator_t(get_node_allocator_()));
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
        return where.tree_->calculate_rank_(where.node, where.where);
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
        static_assert(config_t::status_type::value, "status disabled");
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

    bool is_root_sentinel_(node_t const *node) const
    {
        return node == static_cast<node_t const *>(&root_);
    }
    // Maintain the nullptr sentinel invariant so that node->key_count() == new_used:
    // children[0..new_used] stay valid, children[new_used + 1 .. max] are cleared.
    // A full node (new_used == max) keeps every slot and has no sentinel.
    void set_inner_key_count_(inner_node_t *node, size_type new_used)
    {
        for(size_type i = new_used + 1; i <= size_type(inner_node_t::max); ++i)
        {
            node->children[i].ptr = nullptr;
        }
    }
    void find_leaf_in_parent_(leaf_node_t const *leaf, size_type &size_out, size_type &index_out) const
    {
        node_t *p = leaf->parent;
        if(is_root_sentinel_(p))
        {
            size_out = root_.size;
            index_out = 0;
            return;
        }
        inner_node_t const *inner = static_cast<inner_node_t const *>(p);
        size_type child_count = inner->key_count();
        for(size_type i = 0; i <= child_count; ++i)
        {
            if(inner->children[i].ptr == static_cast<node_t const *>(leaf))
            {
                size_out = inner->children[i].size;
                index_out = i;
                return;
            }
        }
        size_out = 0;
        index_out = 0;
    }
    size_type get_leaf_size_(leaf_node_t const *leaf) const
    {
        size_type size_out = 0, index_out = 0;
        find_leaf_in_parent_(leaf, size_out, index_out);
        return size_out;
    }
    bool leaf_is_full_size_(size_type sz) const
    {
        return sz == size_type(leaf_node_t::max);
    }
    bool leaf_is_few_size_(size_type sz) const
    {
        return sz <= size_type(leaf_node_t::min);
    }
    bool leaf_is_underflow_size_(size_type sz) const
    {
        return sz < size_type(leaf_node_t::min);
    }
    size_type child_index_(inner_node_t *parent, node_t *node) const
    {
        size_type i = 0;
        while(parent->children[i].ptr != node)
        {
            ++i;
        }
        return i;
    }
    size_type node_size_(node_t *node) const
    {
        if(is_root_sentinel_(node))
        {
            return 0;
        }
        if(node->level == 0)
        {
            return get_leaf_size_(static_cast<leaf_node_t const *>(node));
        }
        inner_node_t const *inner_node = static_cast<inner_node_t const *>(node);
        size_type total = 0;
        size_type child_count = inner_node->key_count();
        for(size_type i = 0; i <= child_count; ++i)
        {
            total += inner_node->children[i].size;
        }
        return total;
    }
    void update_size_chain_(node_t *node, difference_type diff)
    {
        for(;;)
        {
            node_t *parent = node->parent;
            if(is_root_sentinel_(parent))
            {
                root_.size += diff;
                break;
            }
            inner_node_t *inner_parent = static_cast<inner_node_t *>(parent);
            inner_parent->children[child_index_(inner_parent, node)].size += diff;
            node = parent;
        }
    }
    void set_node_size_(node_t *node, size_type size)
    {
        node_t *parent = node->parent;
        if(is_root_sentinel_(parent))
        {
            root_.size = size;
            return;
        }
        inner_node_t *inner_parent = static_cast<inner_node_t *>(parent);
        inner_parent->children[child_index_(inner_parent, node)].size = size;
    }

    void fix_root_()
    {
        if(is_root_sentinel_(root_.parent))
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
        node->level = level;
        node->children[0].ptr = nullptr;
        status_control_t::change_inner(root_, 1, level);
        return node;
    }
    leaf_node_t *alloc_leaf_node_()
    {
        leaf_node_t *node = reinterpret_cast<leaf_node_t *>(get_node_allocator_().allocate(1));
        node->parent = nullptr;
        node->level = 0;
        node->prev = nullptr;
        node->next = nullptr;
        status_control_t::change_leaf(root_, 1);
        return node;
    }
    template<class in_node_t> void dealloc_node_(in_node_t *node)
    {
        destroy_range_(node->item, node->item + node->key_count());
        get_node_allocator_().deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }
    void dealloc_node_(leaf_node_t *node)
    {
        size_type sz = (node->parent != nullptr) ? get_leaf_size_(node) : 0;
        destroy_range_(node->item, node->item + sz);
        get_node_allocator_().deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }

    template<bool is_recursive> void free_node_(node_t *node)
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
                size_type child_count = inner_node->key_count();
                for(size_type i = 0; i <= child_count; ++i)
                {
                    free_node_<is_recursive>(inner_node->children[i].ptr);
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
            if(root_.size == 0)
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
            if(pos.second + 1 >= get_leaf_size_(pos.first))
            {
                return std::make_pair(is_root_sentinel_(pos.first->next) ? nullptr : static_cast<leaf_node_t *>(pos.first->next), 0);
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
            leaf_node_t *leaf_node = root_.size == 0 ? nullptr : static_cast<leaf_node_t *>(pos.first->prev);
            return std::make_pair(leaf_node, leaf_node == nullptr ? 0 : get_leaf_size_(leaf_node) - 1);
        }
        else
        {
            return std::make_pair(pos.first, pos.second - 1);
        }
    }

    void advance_next_(node_t *&node, size_type &where, size_type &leaf_bound, size_type &inner_index) const
    {
        if(is_root_sentinel_(node))
        {
            node = static_cast<root_node_t *>(node)->left;
            if(is_root_sentinel_(node))
            {
                leaf_bound = 0;
                inner_index = 0;
            }
            else
            {
                find_leaf_in_parent_(static_cast<leaf_node_t const *>(node), leaf_bound, inner_index);
            }
        }
        else
        {
            if(++where >= leaf_bound)
            {
                node_t *old_leaf = node;
                node = static_cast<leaf_node_t *>(node)->next;
                where = 0;
                if(is_root_sentinel_(node))
                {
                    leaf_bound = 0;
                    inner_index = 0;
                }
                else
                {
                    inner_node_t *parent = static_cast<inner_node_t *>(old_leaf->parent);
                    if(!is_root_sentinel_(parent) && parent->children[inner_index + 1].ptr == node)
                    {
                        leaf_bound = parent->children[inner_index + 1].size;
                        ++inner_index;
                    }
                    else
                    {
                        find_leaf_in_parent_(static_cast<leaf_node_t const *>(node), leaf_bound, inner_index);
                    }
                }
            }
        }
    }
    void advance_prev_(node_t *&node, size_type &where, size_type &leaf_bound, size_type &inner_index) const
    {
        if(where == 0)
        {
            node_t *old_leaf = node;
            bool old_is_sentinel = is_root_sentinel_(old_leaf);
            node = old_is_sentinel ? static_cast<root_node_t *>(node)->right : static_cast<leaf_node_t *>(node)->prev;
            if(is_root_sentinel_(node))
            {
                leaf_bound = 0;
                inner_index = 0;
                where = 0;
            }
            else if(!old_is_sentinel)
            {
                inner_node_t *parent = static_cast<inner_node_t *>(old_leaf->parent);
                if(!is_root_sentinel_(parent) && inner_index > 0 && parent->children[inner_index - 1].ptr == node)
                {
                    leaf_bound = parent->children[inner_index - 1].size;
                    --inner_index;
                    where = leaf_bound - 1;
                }
                else
                {
                    find_leaf_in_parent_(static_cast<leaf_node_t const *>(node), leaf_bound, inner_index);
                    where = leaf_bound - 1;
                }
            }
            else
            {
                find_leaf_in_parent_(static_cast<leaf_node_t const *>(node), leaf_bound, inner_index);
                where = leaf_bound - 1;
            }
        }
        else
        {
            --where;
        }
    }

    void advance_step_(node_t *&node, size_type &where, size_type &leaf_bound, size_type &inner_index, difference_type step) const
    {
        if(is_root_sentinel_(node))
        {
            if(step == 0)
            {
                return;
            }
            else if(step > 0)
            {
                --step;
                advance_next_(node, where, leaf_bound, inner_index);
            }
            else
            {
                ++step;
                advance_prev_(node, where, leaf_bound, inner_index);
            }
            if(is_root_sentinel_(node))
            {
                return;
            }
        }
        step += where;
        if(step == 0)
        {
            where = 0;
            return;
        }
        if(step > 0)
        {
            while(size_type(step) >= node_size_(node))
            {
                if(is_root_sentinel_(node->parent))
                {
                    node = node->parent;
                    where = 0;
                    leaf_bound = 0;
                    inner_index = 0;
                    return;
                }
                inner_node_t *parent = static_cast<inner_node_t *>(node->parent);
                for(child_slot_t *child = parent->children;; ++child)
                {
                    if(child->ptr == node)
                    {
                        node = parent;
                        break;
                    }
                    else
                    {
                        step += child->size;
                    }
                }
            }
        }
        else
        {
            do
            {
                if(is_root_sentinel_(node->parent))
                {
                    node = node->parent;
                    where = 0;
                    leaf_bound = 0;
                    inner_index = 0;
                    return;
                }
                inner_node_t *parent = static_cast<inner_node_t *>(node->parent);
                for(child_slot_t *child = parent->children;; ++child)
                {
                    if(child->ptr == node)
                    {
                        node = parent;
                        break;
                    }
                    else
                    {
                        step += child->size;
                    }
                }
            } while(step < 0);
        }
        while(node->level > 0)
        {
            inner_node_t *inner_node = static_cast<inner_node_t *>(node);
            size_type idx = 0;
            for(child_slot_t *child = inner_node->children;; ++child, ++idx)
            {
                if(size_type(step) >= child->size)
                {
                    step -= child->size;
                }
                else
                {
                    node = child->ptr;
                    if(inner_node->level == 1)
                    {
                        // captured leaf's slot index directly during descent
                        leaf_bound = child->size;
                        inner_index = idx;
                    }
                    break;
                }
            }
        }
        where = step;
        if(is_root_sentinel_(node))
        {
            leaf_bound = 0;
            inner_index = 0;
        }
        else if(is_root_sentinel_(node->parent))
        {
            // single-leaf tree: no inner descent happened
            leaf_bound = root_.size;
            inner_index = 0;
        }
    }

    template<bool is_leftish> size_type calculate_key_rank_(key_type const &key) const
    {
        node_t *node = root_.parent;
        if(is_root_sentinel_(node))
        {
            return root_.size;
        }
        size_type rank = 0;
        size_type leaf_size = root_.size;
        while(node->level > 0)
        {
            inner_node_t const *inner_node = static_cast<inner_node_t const *>(node);
            size_type where;
            if(std::is_scalar<key_type>::value)
            {
                size_type child_keys = inner_node->key_count();
                for(where = 0; where < child_keys; ++where)
                {
                    if(is_leftish ? !get_comparator_()(get_key_t()(inner_node->item[where]), key) : get_comparator_()(key, get_key_t()(inner_node->item[where])))
                    {
                        break;
                    }
                    else
                    {
                        rank += inner_node->children[where].size;
                    }
                }
            }
            else
            {
                where = is_leftish ? lower_bound_(inner_node, key) : upper_bound_(inner_node, key);
                for(size_type i = 0; i < where; ++i)
                {
                    rank += inner_node->children[i].size;
                }
            }
            leaf_size = inner_node->children[where].size;
            node = inner_node->children[where].ptr;
        }
        return rank + (is_leftish ? lower_bound_(static_cast<leaf_node_t *>(node), leaf_size, key) : upper_bound_(static_cast<leaf_node_t *>(node), leaf_size, key));
    }

    size_type calculate_rank_(node_t *node, size_type where) const
    {
        if(is_root_sentinel_(node))
        {
            return root_.size;
        }
        else
        {
            size_type rank;
            std::tie(std::ignore, rank) = advance_root_(node, where);
            return rank;
        }
    }

    std::pair<node_t *, size_type> advance_root_(node_t *node, size_type where) const
    {
        while(!is_root_sentinel_(node->parent))
        {
            inner_node_t *parent = static_cast<inner_node_t *>(node->parent);
            for(size_type i = 0;; ++i)
            {
                if(parent->children[i].ptr == node)
                {
                    node = parent;
                    break;
                }
                else
                {
                    where += parent->children[i].size;
                }
            }
        }
        return std::make_pair(node, where);
    }

    pair_pos_t access_index_(node_t *node, size_type index) const
    {
        if(index >= node_size_(node))
        {
            return std::make_pair(nullptr, 0);
        }
        while(node->level > 0)
        {
            inner_node_t *inner_node = static_cast<inner_node_t *>(node);
            for(child_slot_t *child = inner_node->children;; ++child)
            {
                if(index >= child->size)
                {
                    index -= child->size;
                }
                else
                {
                    node = child->ptr;
                    break;
                }
            }
        }
        return std::make_pair(static_cast<leaf_node_t *>(node), index);
    }

    template<class node_type, class in_key_key> size_type lower_bound_(node_type *node, in_key_key const &key) const
    {
        if(std::is_scalar<key_type>::value && size_type(node_type::max * sizeof(typename node_type::item_type)) <= size_type(binary_search_limit))
        {
            return std::find_if(node->item, node->item + node->key_count(), [&](typename node_type::item_type const &item) -> bool
                                { return !get_comparator_()(get_key_t()(item), key); }) -
                   node->item;
        }
        else
        {
            // Hand-written binary search over inner_node_t: avoids key_count() (O(fanout) scan).
            // item[i] is a valid key iff children[i+1].ptr != nullptr; children size is max+1.
            size_type lo = 0, hi = node_type::max;
            while(lo < hi)
            {
                size_type mid = lo + ((hi - lo) >> 1);
                if(node->children[mid + 1].ptr == nullptr)
                {
                    hi = mid;
                }
                else if(get_comparator_()(get_key_t()(node->item[mid]), key))
                {
                    lo = mid + 1;
                }
                else
                {
                    hi = mid;
                }
            }
            return lo;
        }
    }
    template<class in_key_key> size_type lower_bound_(leaf_node_t *node, size_type sz, in_key_key const &key) const
    {
        if(std::is_scalar<key_type>::value && size_type(leaf_node_t::max * sizeof(typename leaf_node_t::item_type)) <= size_type(binary_search_limit))
        {
            return std::find_if(node->item, node->item + sz, [&](typename leaf_node_t::item_type const &item) -> bool
                                { return !get_comparator_()(get_key_t()(item), key); }) -
                   node->item;
        }
        else
        {
            return std::lower_bound(node->item, node->item + sz, key, [&](typename leaf_node_t::item_type const &left, in_key_key const &right) -> bool
                                    { return get_comparator_()(get_key_t()(left), right); }) -
                   node->item;
        }
    }
    template<class node_type, class in_key_key> size_type upper_bound_(node_type *node, in_key_key const &key) const
    {
        if(std::is_scalar<key_type>::value && size_type(node_type::max * sizeof(typename node_type::item_type)) <= size_type(binary_search_limit))
        {
            return std::find_if(node->item, node->item + node->key_count(), [&](typename node_type::item_type const &item) -> bool
                                { return get_comparator_()(key, get_key_t()(item)); }) -
                   node->item;
        }
        else
        {
            // Hand-written binary search over inner_node_t: avoids key_count() (O(fanout) scan).
            // item[i] is a valid key iff children[i+1].ptr != nullptr; children size is max+1.
            size_type lo = 0, hi = node_type::max;
            while(lo < hi)
            {
                size_type mid = lo + ((hi - lo) >> 1);
                if(node->children[mid + 1].ptr == nullptr)
                {
                    hi = mid;
                }
                else if(get_comparator_()(key, get_key_t()(node->item[mid])))
                {
                    hi = mid;
                }
                else
                {
                    lo = mid + 1;
                }
            }
            return lo;
        }
    }
    template<class in_key_key> size_type upper_bound_(leaf_node_t *node, size_type sz, in_key_key const &key) const
    {
        if(std::is_scalar<key_type>::value && size_type(leaf_node_t::max * sizeof(typename leaf_node_t::item_type)) <= size_type(binary_search_limit))
        {
            return std::find_if(node->item, node->item + sz, [&](typename leaf_node_t::item_type const &item) -> bool
                                { return get_comparator_()(key, get_key_t()(item)); }) -
                   node->item;
        }
        else
        {
            return std::upper_bound(node->item, node->item + sz, key, [&](in_key_key const &left, typename leaf_node_t::item_type const &right) -> bool
                                    { return get_comparator_()(left, get_key_t()(right)); }) -
                   node->item;
        }
    }

    // Internal descent that additionally surfaces the leaf's size (maintained along the descent).
    // Returned tuple: (leaf, where, leaf_size). leaf is nullptr only when the tree is empty.
    template<class in_key_key> std::tuple<leaf_node_t *, size_type, size_type> lower_bound_impl_(in_key_key const &key) const
    {
        node_t *node = root_.parent;
        if(is_root_sentinel_(node))
        {
            return std::make_tuple(static_cast<leaf_node_t *>(nullptr), size_type(0), size_type(0));
        }
        size_type leaf_size = root_.size;
        while(node->level > 0)
        {
            inner_node_t const *inner_node = static_cast<inner_node_t const *>(node);
            size_type w = lower_bound_(inner_node, key);
            leaf_size = inner_node->children[w].size;
            node = inner_node->children[w].ptr;
        }
        leaf_node_t *leaf_node = static_cast<leaf_node_t *>(node);
        size_type where = lower_bound_(leaf_node, leaf_size, key);
        return std::make_tuple(leaf_node, where, leaf_size);
    }
    template<class in_key_key> std::tuple<leaf_node_t *, size_type, size_type> upper_bound_impl_(in_key_key const &key) const
    {
        node_t *node = root_.parent;
        if(is_root_sentinel_(node))
        {
            return std::make_tuple(static_cast<leaf_node_t *>(nullptr), size_type(0), size_type(0));
        }
        size_type leaf_size = root_.size;
        while(node->level > 0)
        {
            inner_node_t const *inner_node = static_cast<inner_node_t const *>(node);
            size_type w = upper_bound_(inner_node, key);
            leaf_size = inner_node->children[w].size;
            node = inner_node->children[w].ptr;
        }
        leaf_node_t *leaf_node = static_cast<leaf_node_t *>(node);
        size_type where = upper_bound_(leaf_node, leaf_size, key);
        return std::make_tuple(leaf_node, where, leaf_size);
    }
    template<class in_key_key> pair_pos_t lower_bound_(in_key_key const &key) const
    {
        leaf_node_t *leaf_node;
        size_type where;
        size_type leaf_size;
        std::tie(leaf_node, where, leaf_size) = lower_bound_impl_(key);
        if(leaf_node == nullptr || where >= leaf_size)
        {
            return std::make_pair(nullptr, 0);
        }
        return std::make_pair(leaf_node, where);
    }
    template<class in_key_key> pair_pos_t upper_bound_(in_key_key const &key) const
    {
        leaf_node_t *leaf_node;
        size_type where;
        size_type leaf_size;
        std::tie(leaf_node, where, leaf_size) = upper_bound_impl_(key);
        if(leaf_node == nullptr || where >= leaf_size)
        {
            return std::make_pair(nullptr, 0);
        }
        return std::make_pair(leaf_node, where);
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

    static size_type update_parent_(child_slot_t *update_begin, child_slot_t *update_end, node_t *parent)
    {
        size_type count = 0;
        while(update_begin != update_end)
        {
            child_slot_t &slot = *update_begin++;
            count += slot.size;
            slot.ptr->parent = parent;
        }
        return count;
    }

    template<class in_value_t> pair_posi_t insert_first_(in_value_t &&value)
    {
        leaf_node_t *node = alloc_leaf_node_();
        construct_one_(node->item, std::forward<in_value_t>(value));
        root_.parent = root_.left = root_.right = node;
        node->parent = node->next = node->prev = &root_;
        root_.size = 1;
        return std::make_pair(std::make_pair(node, 0), true);
    }

    template<class in_value_t> pair_posi_t insert_hint_(leaf_node_t *leaf_node, size_type where, in_value_t &&value)
    {
        bool is_leftish = false;
        pair_pos_t other;
        if(root_.size == 0)
        {
            return insert_first_(std::forward<in_value_t>(value));
        }
        // Resolve hint leaf's size once (avoid repeated parent scans inside this function).
        size_type hint_size = (leaf_node != nullptr) ? get_leaf_size_(leaf_node) : 0;
        if(config_t::unique_type::value)
        {
            if(leaf_node == root_.left && where == 0)
            {
                if(get_comparator_()(get_key_t()(value), get_key_t()(leaf_node->item[where])))
                {
                    return insert_pos_(leaf_node, 0, std::forward<in_value_t>(value), hint_size);
                }
            }
            else if(leaf_node == nullptr)
            {
                leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
                size_type tail_size = get_leaf_size_(tail);
                if(get_comparator_()(get_key_t()(tail->item[tail_size - 1]), get_key_t()(value)))
                {
                    return insert_pos_(tail, tail_size, std::forward<in_value_t>(value), tail_size);
                }
            }
            else if(get_comparator_()(get_key_t()(value), get_key_t()(leaf_node->item[where])) && get_comparator_()(get_key_t()(other = advance_prev_(std::make_pair(leaf_node, where))), get_key_t()(value)))
            {
                return insert_pos_(leaf_node, where, std::forward<in_value_t>(value), hint_size);
            }
            else if(get_comparator_()(get_key_t()(leaf_node->item[where]), get_key_t()(value)) && ((other = advance_next_(std::make_pair(leaf_node, where))).first == nullptr || get_comparator_()(get_key_t()(value), get_key_t()(other))))
            {
                if(other.first == nullptr)
                {
                    leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
                    size_type tail_size = get_leaf_size_(tail);
                    return insert_pos_(tail, tail_size, std::forward<in_value_t>(value), tail_size);
                }
                else
                {
                    size_type other_size = (other.first == leaf_node) ? hint_size : get_leaf_size_(other.first);
                    return insert_pos_(other.first, other.second, std::forward<in_value_t>(value), other_size);
                }
            }
        }
        else
        {
            if(leaf_node == root_.left && where == 0)
            {
                if(!get_comparator_()(get_key_t()(leaf_node->item[where]), get_key_t()(value)))
                {
                    return insert_pos_(leaf_node, 0, std::forward<in_value_t>(value), hint_size);
                }
                is_leftish = true;
            }
            else if(leaf_node == nullptr)
            {
                leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
                size_type tail_size = get_leaf_size_(tail);
                if(!get_comparator_()(get_key_t()(value), get_key_t()(tail->item[tail_size - 1])))
                {
                    return insert_pos_(tail, tail_size, std::forward<in_value_t>(value), tail_size);
                }
            }
            else if(!get_comparator_()(get_key_t()(leaf_node->item[where]), get_key_t()(value)) && !get_comparator_()(get_key_t()(value), get_key_t()(other = advance_prev_(std::make_pair(leaf_node, where)))))
            {
                return insert_pos_(leaf_node, where, std::forward<in_value_t>(value), hint_size);
            }
            else if(!get_comparator_()(get_key_t()(value), get_key_t()(leaf_node->item[where])) && ((other = advance_next_(std::make_pair(leaf_node, where))).first == nullptr || !get_comparator_()(get_key_t()(other), get_key_t()(value))))
            {
                if(other.first == nullptr)
                {
                    leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
                    size_type tail_size = get_leaf_size_(tail);
                    return insert_pos_(tail, tail_size, std::forward<in_value_t>(value), tail_size);
                }
                else
                {
                    size_type other_size = (other.first == leaf_node) ? hint_size : get_leaf_size_(other.first);
                    return insert_pos_(other.first, other.second, std::forward<in_value_t>(value), other_size);
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

    template<class in_value_t> pair_posi_t insert_pos_(leaf_node_t *leaf_node, size_type where, in_value_t &&value, size_type leaf_size)
    {
        key_stack_t key_out;
        node_t *split_node = nullptr;
        inner_node_t *parent = nullptr;
        size_type parent_where = 0;
        size_type new_leaf_size = 0;
        if(leaf_is_full_size_(leaf_size))
        {
            parent_where = get_parent_(leaf_node, parent);
            size_type orig_size = leaf_size;
            split_leaf_node_(leaf_node, orig_size, &key_out, split_node, new_leaf_size);
            size_type mid = orig_size >> 1;
            if(where >= mid)
            {
                where -= mid;
                leaf_node = static_cast<leaf_node_t *>(split_node);
                leaf_size = new_leaf_size;
            }
            else
            {
                leaf_size = mid;
            }
        }
        move_next_and_insert_one_(leaf_node->item + where, leaf_node->item + leaf_size, std::forward<in_value_t>(value));
        ++leaf_size;
        if(split_node != nullptr && leaf_node != split_node && where == leaf_size - 1)
        {
            key_out = get_key_t()(leaf_node->item[where]);
        }
        if(split_node == nullptr)
        {
            update_size_chain_(leaf_node, 1);
        }
        else
        {
            // After split: original leaf is attached (slot was set to mid by split_leaf_node_);
            // split leaf is unattached — its size is communicated via insert_pos_descend_.
            leaf_node_t *split_leaf = static_cast<leaf_node_t *>(split_node);
            size_type mid = size_type(leaf_node_t::max) >> 1;
            size_type orig_after = (leaf_node == split_leaf) ? mid : leaf_size;
            size_type split_after = (leaf_node == split_leaf) ? leaf_size : new_leaf_size;
            // Refresh original leaf's slot to post-insert value when insertion went into original.
            if(leaf_node != split_leaf)
            {
                set_node_size_(leaf_node, orig_after);
            }
            insert_pos_descend_(parent, parent_where, std::move(key_out), split_node, split_after);
        }
        return std::make_pair(std::make_pair(leaf_node, where), true);
    }

    void insert_pos_descend_(inner_node_t *inner_node, size_type where, key_stack_t &&key_out, node_t *new_child, size_type new_child_size)
    {
        if(inner_node == nullptr)
        {
            inner_node_t *new_root = alloc_inner_node_(&root_, root_.parent->level + 1);
            construct_one_(new_root->item, std::move(key_out.key()));
            destroy_one_(&key_out);
            new_root->children[0].ptr = root_.parent;
            new_root->children[1].ptr = new_child;
            set_inner_key_count_(new_root, 1);
            // root_.parent's current slot size is in root_.size (since its parent is &root_).
            new_root->children[0].size = (root_.parent->level == 0) ? root_.size : node_size_(root_.parent);
            new_root->children[1].size = new_child_size;
            root_.parent->parent = new_root;
            new_child->parent = new_root;
            root_.parent = new_root;
            root_.size = new_root->children[0].size + new_root->children[1].size;
            return;
        }
        key_stack_t split_key_out;
        node_t *split_node = nullptr;
        inner_node_t *parent = nullptr;
        size_type parent_where = 0;
        do
        {
            if(inner_node->is_full())
            {
                parent_where = get_parent_(inner_node, parent);
                split_inner_node_(inner_node, &split_key_out, split_node, where);
                inner_node_t *split_tree_node = static_cast<inner_node_t *>(split_node);
                size_type inner_used = inner_node->key_count();
                size_type split_used = split_tree_node->key_count();
                if(where == inner_used + 1 && inner_used < split_used)
                {
                    construct_one_(inner_node->item + inner_used, std::move(split_key_out.key()));
                    inner_node->children[inner_used + 1] = split_tree_node->children[0];
                    set_inner_key_count_(inner_node, inner_used + 1);
                    inner_node->children[inner_used + 1].ptr->parent = inner_node;
                    new_child->parent = split_tree_node;
                    split_tree_node->children[0].ptr = new_child;
                    split_tree_node->children[0].size = new_child_size;
                    split_key_out = std::move(key_out.key());
                    destroy_one_(&key_out);
                    break;
                }
                else if(where >= size_type(inner_used + 1))
                {
                    where -= inner_used + 1;
                    inner_node = split_tree_node;
                }
            }
            size_type cur_used = inner_node->key_count();
            move_next_and_insert_one_(inner_node->item + where, inner_node->item + cur_used, std::move(key_out.key()));
            destroy_one_(&key_out);
            std::move_backward(inner_node->children + where, inner_node->children + cur_used + 1, inner_node->children + cur_used + 2);
            inner_node->children[where + 1].ptr = new_child;
            inner_node->children[where + 1].size = new_child_size;
            set_inner_key_count_(inner_node, cur_used + 1);
            new_child->parent = inner_node;
        } while(false);
        if(split_node == nullptr)
        {
            update_size_chain_(inner_node, 1);
        }
        else
        {
            if(parent != nullptr)
            {
                parent->children[parent_where].size = node_size_(inner_node);
            }
            // For inner-level recursion, new_child (split_node) is an inner; size via node_size_ on inner works (uses slot sums, no leaf scan).
            insert_pos_descend_(parent, parent_where, std::move(split_key_out), split_node, node_size_(split_node));
        }
    }

    template<bool is_leftish, class in_value_t> pair_posi_t insert_nohint_(in_value_t &&value)
    {
        if(root_.size == 0)
        {
            return insert_first_(std::forward<in_value_t>(value));
        }
        leaf_node_t *leaf_node;
        size_type where;
        size_type leaf_size;
        std::tie(leaf_node, where, leaf_size) = is_leftish ? lower_bound_impl_(get_key_t()(value)) : upper_bound_impl_(get_key_t()(value));
        // When the key is past-end, the descent already lands on the rightmost leaf
        // (== root_.right) with where == leaf_size, so no extra fix-up is needed.
        if(config_t::unique_type::value && (where > 0 || leaf_node->prev != &root_))
        {
            if(where == 0)
            {
                leaf_node_t *prev_leaf_node = static_cast<leaf_node_t *>(leaf_node->prev);
                size_type prev_size = get_leaf_size_(prev_leaf_node);
                if(!get_comparator_()(get_key_t()(prev_leaf_node->item[prev_size - 1]), get_key_t()(value)))
                {
                    return std::make_pair(std::make_pair(prev_leaf_node, prev_size - 1), false);
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
        return insert_pos_(leaf_node, where, std::forward<in_value_t>(value), leaf_size);
    }

    void split_inner_node_(inner_node_t *inner_node, key_type *key_ptr, node_t *&new_node, size_type where)
    {
        size_type used = inner_node->key_count();
        size_type mid = (used >> 1);
        if(where <= mid && mid > used - (mid + 1))
        {
            --mid;
        }
        inner_node_t *new_inner_node = alloc_inner_node_(inner_node->parent, inner_node->level);
        size_type new_used = used - (mid + 1);
        move_construct_and_destroy_(inner_node->item + mid + 1, inner_node->item + used, new_inner_node->item);
        std::copy(inner_node->children + mid + 1, inner_node->children + used + 1, new_inner_node->children);
        set_inner_key_count_(new_inner_node, new_used);
        set_inner_key_count_(inner_node, mid);
        construct_one_(key_ptr, inner_node->item[mid]);
        destroy_one_(inner_node->item + mid);
        update_parent_(new_inner_node->children, new_inner_node->children + new_used + 1, new_inner_node);
        new_node = new_inner_node;
    }

    void split_leaf_node_(leaf_node_t *leaf_node, size_type leaf_size, key_type *key_ptr, node_t *&new_node, size_type &new_leaf_size_out)
    {
        size_type mid = (leaf_size >> 1);
        leaf_node_t *new_leaf_node = alloc_leaf_node_();
        new_leaf_size_out = leaf_size - mid;
        new_leaf_node->next = leaf_node->next;
        if(is_root_sentinel_(new_leaf_node->next))
        {
            root_.right = new_leaf_node;
        }
        else
        {
            static_cast<leaf_node_t *>(new_leaf_node->next)->prev = new_leaf_node;
        }
        move_construct_and_destroy_(leaf_node->item + mid, leaf_node->item + leaf_size, new_leaf_node->item);
        set_node_size_(leaf_node, mid);
        leaf_node->next = new_leaf_node;
        new_leaf_node->prev = leaf_node;
        construct_one_(key_ptr, get_key_t()(leaf_node->item[mid - 1]));
        new_node = new_leaf_node;
    }

    result_t merge_leaves_(leaf_node_t *left, size_type left_size, leaf_node_t *right, size_type right_size, inner_node_t *parent)
    {
        (void)parent;
        move_construct_and_destroy_(right->item, right->item + right_size, left->item + left_size);
        left->next = right->next;
        if(!is_root_sentinel_(left->next))
        {
            static_cast<leaf_node_t *>(left->next)->prev = left;
        }
        else
        {
            root_.right = left;
        }
        set_node_size_(left, left_size + right_size);
        right->parent = nullptr;
        return result_t(btree_fixmerge);
    }

    result_t shift_left_leaf_(leaf_node_t *left, size_type left_size, leaf_node_t *right, size_type right_size, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (right_size - left_size) >> 1;
        move_construct_(right->item, right->item + shiftnum, left->item + left_size);
        size_type new_left_size = left_size + shiftnum;
        move_forward_(right->item + shiftnum, right->item + right_size, right->item);
        destroy_range_(right->item + right_size - shiftnum, right->item + right_size);
        size_type new_right_size = right_size - shiftnum;
        set_node_size_(left, new_left_size);
        set_node_size_(right, new_right_size);
        if(parent_where < parent->key_count())
        {
            parent->item[parent_where] = get_key_t()(left->item[new_left_size - 1]);
            return result_t(btree_ok);
        }
        else
        {
            return result_t(btree_update_lastkey, get_key_t()(left->item[new_left_size - 1]));
        }
    }

    void shift_right_leaf_(leaf_node_t *left, size_type left_size, leaf_node_t *right, size_type right_size, inner_node_t *parent, size_type parent_where)
    {
        size_type shiftnum = (left_size - right_size) >> 1;
        move_next_to_and_construct_(right->item, right->item + right_size, right->item + shiftnum);
        size_type new_right_size = right_size + shiftnum;
        move_and_destroy_(left->item + left_size - shiftnum, left->item + left_size, right->item);
        size_type new_left_size = left_size - shiftnum;
        set_node_size_(left, new_left_size);
        set_node_size_(right, new_right_size);
        parent->item[parent_where] = get_key_t()(left->item[new_left_size - 1]);
    }

    result_t merge_inners_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type lused = left->key_count();
        size_type rused = right->key_count();
        construct_one_(left->item + lused, parent->item[parent_where]);
        move_construct_and_destroy_(right->item, right->item + rused, left->item + lused + 1);
        std::copy(right->children, right->children + rused + 1, left->children + lused + 1);
        update_parent_(left->children + lused + 1, left->children + lused + 1 + rused + 1, left);
        set_inner_key_count_(left, lused + 1 + rused);
        right->children[0].ptr = nullptr;
        set_node_size_(left, node_size_(left));
        right->parent = nullptr;
        return result_t(btree_fixmerge);
    }

    void shift_left_inner_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type lused = left->key_count();
        size_type rused = right->key_count();
        size_type shiftnum = (rused - lused) >> 1;
        construct_one_(left->item + lused, parent->item[parent_where]);
        move_construct_(right->item, right->item + shiftnum - 1, left->item + lused + 1);
        std::copy(right->children, right->children + shiftnum, left->children + lused + 1);
        update_parent_(left->children + lused + 1, left->children + lused + 1 + shiftnum, left);
        set_inner_key_count_(left, lused + shiftnum);
        parent->item[parent_where] = right->item[shiftnum - 1];
        move_forward_(right->item + shiftnum, right->item + rused, right->item);
        destroy_range_(right->item + rused - shiftnum, right->item + rused);
        std::copy(right->children + shiftnum, right->children + rused + 1, right->children);
        set_inner_key_count_(right, rused - shiftnum);
        set_node_size_(left, node_size_(left));
        set_node_size_(right, node_size_(right));
    }

    void shift_right_inner_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        size_type lused = left->key_count();
        size_type rused = right->key_count();
        size_type shiftnum = (lused - rused) >> 1;
        move_next_to_and_construct_(right->item, right->item + rused, right->item + shiftnum);
        std::copy_backward(right->children, right->children + rused + 1, right->children + rused + 1 + shiftnum);
        set_inner_key_count_(right, rused + shiftnum);
        right->item[shiftnum - 1] = parent->item[parent_where];
        move_and_destroy_(left->item + lused - shiftnum + 1, left->item + lused, right->item);
        std::copy(left->children + lused - shiftnum + 1, left->children + lused + 1, right->children);
        update_parent_(right->children, right->children + shiftnum, right);
        parent->item[parent_where] = left->item[lused - shiftnum];
        set_inner_key_count_(left, lused - shiftnum);
        set_node_size_(left, node_size_(left));
        set_node_size_(right, node_size_(right));
    }

    size_type get_parent_(node_t *node, inner_node_t *&parent)
    {
        if(is_root_sentinel_(node->parent))
        {
            parent = nullptr;
            return 0;
        }
        parent = static_cast<inner_node_t *>(node->parent);
        for(child_slot_t *child = parent->children;; ++child)
        {
            if(child->ptr == node)
            {
                return child - parent->children;
            }
        }
        return 0;
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
            return left_parent == nullptr ? nullptr : static_cast<in_node_t *>(left_parent->children[left_parent->key_count() - 1].ptr);
        }
        else
        {
            return static_cast<in_node_t *>(parent->children[where - 1].ptr);
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
        if(parent->children[where + 1].ptr == nullptr)
        {
            in_node_t *right_parent = get_right_(parent);
            return right_parent == nullptr ? nullptr : static_cast<in_node_t *>(right_parent->children[0].ptr);
        }
        else
        {
            return static_cast<in_node_t *>(parent->children[where + 1].ptr);
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
            left = left_parent == nullptr ? nullptr : static_cast<in_node_t *>(left_parent->children[left_parent->key_count() - 1].ptr);
        }
        else
        {
            left_parent = parent;
            left = static_cast<in_node_t *>(parent->children[where - 1].ptr);
        }
        if(parent->children[where + 1].ptr == nullptr)
        {
            right_parent = get_right_(parent);
            right = right_parent == nullptr ? nullptr : static_cast<in_node_t *>(right_parent->children[0].ptr);
        }
        else
        {
            right_parent = parent;
            right = static_cast<in_node_t *>(parent->children[where + 1].ptr);
        }
    }

    void erase_pos_(leaf_node_t *leaf_node, size_type where, size_type leaf_size)
    {
        move_prev_and_destroy_one_(leaf_node->item + where + 1, leaf_node->item + leaf_size);
        --leaf_size;
        update_size_chain_(leaf_node, -1);
        result_t result(btree_ok);
        inner_node_t *parent = nullptr;
        size_type parent_where = 0;
        if(where == leaf_size)
        {
            parent_where = get_parent_(leaf_node, parent);
            if(parent != nullptr && parent->children[parent_where + 1].ptr != nullptr)
            {
                parent->item[parent_where] = get_key_t()(leaf_node->item[leaf_size - 1]);
            }
            else if(leaf_size >= 1)
            {
                result |= result_t(btree_update_lastkey, get_key_t()(leaf_node->item[leaf_size - 1]));
            }
        }
        if(leaf_is_underflow_size_(leaf_size) && !(leaf_node == root_.parent && leaf_size >= 1))
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
            size_type leaf_left_size = 0, leaf_right_size = 0;
            if(leaf_left != nullptr)
            {
                leaf_left_size = (left_parent == parent) ? left_parent->children[parent_where - 1].size : left_parent->children[left_parent->key_count() - 1].size;
            }
            if(leaf_right != nullptr)
            {
                leaf_right_size = (right_parent == parent) ? right_parent->children[parent_where + 1].size : right_parent->children[0].size;
            }
            bool left_few = (leaf_left == nullptr) || leaf_is_few_size_(leaf_left_size);
            bool right_few = (leaf_right == nullptr) || leaf_is_few_size_(leaf_right_size);
            if(left_few && right_few)
            {
                if(left_parent == parent)
                {
                    result |= merge_leaves_(leaf_left, leaf_left_size, leaf_node, leaf_size, left_parent);
                }
                else
                {
                    result |= merge_leaves_(leaf_node, leaf_size, leaf_right, leaf_right_size, right_parent);
                }
            }
            else if((leaf_left != nullptr && leaf_is_few_size_(leaf_left_size)) && (leaf_right != nullptr && !leaf_is_few_size_(leaf_right_size)))
            {
                if(right_parent == parent)
                {
                    result |= shift_left_leaf_(leaf_node, leaf_size, leaf_right, leaf_right_size, right_parent, parent_where);
                }
                else
                {
                    result |= merge_leaves_(leaf_left, leaf_left_size, leaf_node, leaf_size, left_parent);
                }
            }
            else if((leaf_left != nullptr && !leaf_is_few_size_(leaf_left_size)) && (leaf_right != nullptr && leaf_is_few_size_(leaf_right_size)))
            {
                if(left_parent == parent)
                {
                    shift_right_leaf_(leaf_left, leaf_left_size, leaf_node, leaf_size, left_parent, parent_where - 1);
                }
                else
                {
                    result |= merge_leaves_(leaf_node, leaf_size, leaf_right, leaf_right_size, right_parent);
                }
            }
            else if(left_parent == right_parent)
            {
                if(leaf_left_size <= leaf_right_size)
                {
                    result |= shift_left_leaf_(leaf_node, leaf_size, leaf_right, leaf_right_size, right_parent, parent_where);
                }
                else
                {
                    shift_right_leaf_(leaf_left, leaf_left_size, leaf_node, leaf_size, left_parent, parent_where - 1);
                }
            }
            else
            {
                if(left_parent == parent)
                {
                    shift_right_leaf_(leaf_left, leaf_left_size, leaf_node, leaf_size, left_parent, parent_where - 1);
                }
                else
                {
                    result |= shift_left_leaf_(leaf_node, leaf_size, leaf_right, leaf_right_size, right_parent, parent_where);
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
    }

    void erase_pos_descend_(inner_node_t *inner_node, size_type where, result_t &&result)
    {
        result_t self_result(btree_ok);
        inner_node_t *parent = nullptr;
        size_type parent_where;
        if(result.has(btree_update_lastkey))
        {
            parent_where = get_parent_(inner_node, parent);
            if(parent != nullptr && parent->children[parent_where + 1].ptr != nullptr)
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
            if(inner_node->children[where].ptr->parent != nullptr)
            {
                ++where;
            }
            free_node_<false>(inner_node->children[where].ptr);
            size_type used = inner_node->key_count();
            move_prev_and_destroy_one_(inner_node->item + where, inner_node->item + used);
            std::copy(inner_node->children + where + 1, inner_node->children + used + 1, inner_node->children + where);
            set_inner_key_count_(inner_node, used - 1);
            if(inner_node->level == 1)
            {
                --where;
                leaf_node_t *child = static_cast<leaf_node_t *>(inner_node->children[where].ptr);
                inner_node->item[where] = get_key_t()(child->item[inner_node->children[where].size - 1]);
            }
        }
        if(inner_node->is_underflow() && !(inner_node == root_.parent && inner_node->children[1].ptr != nullptr))
        {
            inner_node_t *inner_left, *inner_right;
            inner_node_t *left_parent, *right_parent;
            get_left_right_parent_(parent, parent_where, inner_left, left_parent, inner_right, right_parent);
            if(inner_left == nullptr && inner_right == nullptr)
            {
                root_.parent = inner_node->children[0].ptr;
                root_.parent->parent = &root_;
                set_inner_key_count_(inner_node, 0);
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
                if(inner_left->key_count() <= inner_right->key_count())
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
    }
};
