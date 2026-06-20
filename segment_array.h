#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>
#include <cstring>
#include <type_traits>
#include <tuple>
#include <iterator>
#include <vector>

template<class value_t, class allocator_t>
struct segment_array_config
{
    typedef value_t value_type;
    typedef allocator_t allocator_type;
    typedef std::false_type status_type;
    template<size_t A, size_t B> struct max_t
    {
        enum
        {
            value = A > B ? A : B
        };
    };
    enum
    {
        // inner child slot now carries both subtree size and pointer (child_slot_t),
        // node_t no longer stores size. 8 children + node overhead + sentinel slot.
        min_inner_size = (sizeof(size_t) + sizeof(nullptr)) * 8 + sizeof(size_t) + sizeof(nullptr) + (sizeof(size_t) + sizeof(nullptr)),
        // leaf: node_t{parent, level} + prev + next + value[8]; node_t::size removed.
        min_leaf_size = sizeof(value_t) * 8 + sizeof(size_t) + sizeof(nullptr) * 3,
        memory_block_size = max_t < 256,
        max_t < min_inner_size,
        min_leaf_size > ::value > ::value,
    };
};

namespace segment_array_detail
{
    template<typename T, typename = void> struct is_iterator : std::false_type
    {
    };
    template<typename T> struct is_iterator<T, typename std::enable_if<!std::is_same<typename std::iterator_traits<T>::iterator_category, void>::value>::type> : std::true_type
    {
    };

    class move_trivial_tag
    {
    };
    class move_assign_tag
    {
    };
    template<class iterator_t> struct get_tag
    {
        typedef typename std::conditional<std::is_trivial<typename std::iterator_traits<iterator_t>::value_type>::value, move_trivial_tag, move_assign_tag>::type type;
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
class segment_array_implement
{
public:
    typedef typename config_t::value_type value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
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
        enum
        {
            max = ((config_t::memory_block_size - sizeof(node_t) - sizeof(child_slot_t)) / sizeof(child_slot_t)),
            min = max / 2,
        };
        child_slot_t children[max + 1];

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
        bool is_few() const
        {
            return children[min + 1].ptr == nullptr;
        }
        bool is_underflow() const
        {
            return children[min].ptr == nullptr;
        }
    };
    struct leaf_node_t : public node_t
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
    struct root_node_t : public node_t, public node_allocator_t, public status_t
    {
        template<class any_allocator_t> root_node_t(any_allocator_t &&alloc) : node_allocator_t(std::forward<any_allocator_t>(alloc)), status_t()
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
    enum result_flags_t
    {
        btree_ok = 0,
        btree_not_found = 1,
        btree_fixmerge = 2,
    };
    struct result_t
    {
        result_flags_t flags;

        explicit result_t(result_flags_t f = btree_ok) : flags(result_flags_t(f))
        {
        }
        result_t(result_t const &other) : flags(other.flags)
        {
        }
        bool has(result_flags_t f) const
        {
            return (flags & f) != 0;
        }
        result_t &operator|=(result_t const &other)
        {
            flags = result_flags_t(flags | other.flags);
            return *this;
        }
        result_t &operator=(result_t const &other)
        {
            flags = other.flags;
            return *this;
        }
    };
    typedef std::pair<leaf_node_t *, size_type> pair_pos_t;
    enum
    {
        binary_search_limit = 1024
    };

public:
    class iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename segment_array_implement::value_type value_type;
        typedef typename segment_array_implement::difference_type difference_type;
        typedef typename segment_array_implement::reference reference;
        typedef typename segment_array_implement::pointer pointer;

    public:
        iterator(node_t *in_node, size_type in_where, segment_array_implement const *in_tree) : node(in_node), where(in_where), tree_(in_tree), leaf_bound_(0), inner_index_(0)
        {
            if(!tree_->is_root_sentinel_(in_node))
                tree_->find_leaf_in_parent_(static_cast<leaf_node_t const *>(in_node), leaf_bound_, inner_index_);
        }
        iterator(pair_pos_t pos, segment_array_implement *self) : node(pos.first == nullptr ? static_cast<node_t *>(&self->root_) : static_cast<node_t *>(pos.first)), where(pos.second), tree_(self), leaf_bound_(0), inner_index_(0)
        {
            if(pos.first != nullptr)
                self->find_leaf_in_parent_(pos.first, leaf_bound_, inner_index_);
        }
        iterator(iterator const &) = default;
        iterator &operator+=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, diff);
            return *this;
        }
        iterator &operator-=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, -diff);
            return *this;
        }
        iterator operator+(difference_type diff) const
        {
            iterator ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, diff);
            return ret;
        }
        iterator operator-(difference_type diff) const
        {
            iterator ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, -diff);
            return ret;
        }
        difference_type operator-(iterator const &other) const
        {
            return static_cast<difference_type>(tree_->calculate_rank_(node, where)) - static_cast<difference_type>(tree_->calculate_rank_(other.node, other.where));
        }
        iterator &operator++()
        {
            tree_->advance_next_(node, where, leaf_bound_, inner_index_);
            return *this;
        }
        iterator &operator--()
        {
            tree_->advance_prev_(node, where, leaf_bound_, inner_index_);
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
        reference operator*() const
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
        bool operator>(iterator const &other) const
        {
            return *this - other > 0;
        }
        bool operator<(iterator const &other) const
        {
            return *this - other < 0;
        }
        bool operator>=(iterator const &other) const
        {
            return *this - other >= 0;
        }
        bool operator<=(iterator const &other) const
        {
            return *this - other <= 0;
        }
        bool operator==(iterator const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator!=(iterator const &other) const
        {
            return node != other.node || where != other.where;
        }

    private:
        friend class segment_array_implement;
        node_t *node;
        size_type where;
        segment_array_implement const *tree_;
        size_type leaf_bound_;
        size_type inner_index_;
    };
    class const_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename segment_array_implement::value_type value_type;
        typedef typename segment_array_implement::difference_type difference_type;
        typedef typename segment_array_implement::reference reference;
        typedef typename segment_array_implement::const_reference const_reference;
        typedef typename segment_array_implement::pointer pointer;
        typedef typename segment_array_implement::const_pointer const_pointer;

    public:
        const_iterator(node_t *in_node, size_type in_where, segment_array_implement const *in_tree) : node(in_node), where(in_where), tree_(in_tree), leaf_bound_(0), inner_index_(0)
        {
            if(!tree_->is_root_sentinel_(in_node))
                tree_->find_leaf_in_parent_(static_cast<leaf_node_t const *>(in_node), leaf_bound_, inner_index_);
        }
        const_iterator(pair_pos_t pos, segment_array_implement const *self) : node(pos.first == nullptr ? self->root_.parent->parent : pos.first), where(pos.second), tree_(self), leaf_bound_(0), inner_index_(0)
        {
            if(pos.first != nullptr)
                self->find_leaf_in_parent_(pos.first, leaf_bound_, inner_index_);
        }
        const_iterator(iterator const &it) : node(it.node), where(it.where), tree_(it.tree_), leaf_bound_(it.leaf_bound_), inner_index_(it.inner_index_)
        {
        }
        const_iterator(const_iterator const &) = default;
        const_iterator &operator+=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, diff);
            return *this;
        }
        const_iterator &operator-=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, -diff);
            return *this;
        }
        const_iterator operator+(difference_type diff) const
        {
            const_iterator ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, diff);
            return ret;
        }
        const_iterator operator-(difference_type diff) const
        {
            const_iterator ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, -diff);
            return ret;
        }
        difference_type operator-(const_iterator const &other) const
        {
            return static_cast<difference_type>(tree_->calculate_rank_(node, where)) - static_cast<difference_type>(tree_->calculate_rank_(other.node, other.where));
        }
        const_iterator &operator++()
        {
            tree_->advance_next_(node, where, leaf_bound_, inner_index_);
            return *this;
        }
        const_iterator &operator--()
        {
            tree_->advance_prev_(node, where, leaf_bound_, inner_index_);
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
        const_reference operator*() const
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
        bool operator>(const_iterator const &other) const
        {
            return *this - other > 0;
        }
        bool operator<(const_iterator const &other) const
        {
            return *this - other < 0;
        }
        bool operator>=(const_iterator const &other) const
        {
            return *this - other >= 0;
        }
        bool operator<=(const_iterator const &other) const
        {
            return *this - other <= 0;
        }
        bool operator==(const_iterator const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator!=(const_iterator const &other) const
        {
            return node != other.node || where != other.where;
        }

    private:
        friend class segment_array_implement;
        node_t *node;
        size_type where;
        segment_array_implement const *tree_;
        size_type leaf_bound_;
        size_type inner_index_;
    };
    class reverse_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename segment_array_implement::value_type value_type;
        typedef typename segment_array_implement::difference_type difference_type;
        typedef typename segment_array_implement::reference reference;
        typedef typename segment_array_implement::pointer pointer;

    public:
        reverse_iterator(node_t *in_node, size_type in_where, segment_array_implement const *in_tree) : node(in_node), where(in_where), tree_(in_tree), leaf_bound_(0), inner_index_(0)
        {
            if(!tree_->is_root_sentinel_(in_node))
                tree_->find_leaf_in_parent_(static_cast<leaf_node_t const *>(in_node), leaf_bound_, inner_index_);
        }
        reverse_iterator(pair_pos_t pos, segment_array_implement *self) : node(pos.first == nullptr ? static_cast<node_t *>(&self->root_) : static_cast<node_t *>(pos.first)), where(pos.second), tree_(self), leaf_bound_(0), inner_index_(0)
        {
            if(pos.first != nullptr)
                self->find_leaf_in_parent_(pos.first, leaf_bound_, inner_index_);
        }
        explicit reverse_iterator(iterator const &other) : node(other.node), where(other.where), tree_(other.tree_), leaf_bound_(other.leaf_bound_), inner_index_(other.inner_index_)
        {
            ++*this;
        }
        reverse_iterator(reverse_iterator const &) = default;
        reverse_iterator &operator+=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, -diff);
            return *this;
        }
        reverse_iterator &operator-=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, diff);
            return *this;
        }
        reverse_iterator operator+(difference_type diff) const
        {
            reverse_iterator ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, -diff);
            return ret;
        }
        reverse_iterator operator-(difference_type diff) const
        {
            reverse_iterator ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, diff);
            return ret;
        }
        difference_type operator-(reverse_iterator const &other) const
        {
            return static_cast<difference_type>(tree_->calculate_rank_(other.node, other.where)) - static_cast<difference_type>(tree_->calculate_rank_(node, where));
        }
        reverse_iterator &operator++()
        {
            tree_->advance_prev_(node, where, leaf_bound_, inner_index_);
            return *this;
        }
        reverse_iterator &operator--()
        {
            tree_->advance_next_(node, where, leaf_bound_, inner_index_);
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
        reference operator*() const
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
        bool operator>(reverse_iterator const &other) const
        {
            return *this - other > 0;
        }
        bool operator<(reverse_iterator const &other) const
        {
            return *this - other < 0;
        }
        bool operator>=(reverse_iterator const &other) const
        {
            return *this - other >= 0;
        }
        bool operator<=(reverse_iterator const &other) const
        {
            return *this - other <= 0;
        }
        bool operator==(reverse_iterator const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator!=(reverse_iterator const &other) const
        {
            return node != other.node || where != other.where;
        }
        iterator base() const
        {
            return ++iterator(node, where, tree_);
        }

    private:
        friend class segment_array_implement;
        node_t *node;
        size_type where;
        segment_array_implement const *tree_;
        size_type leaf_bound_;
        size_type inner_index_;
    };
    class const_reverse_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename segment_array_implement::value_type value_type;
        typedef typename segment_array_implement::difference_type difference_type;
        typedef typename segment_array_implement::reference reference;
        typedef typename segment_array_implement::const_reference const_reference;
        typedef typename segment_array_implement::pointer pointer;
        typedef typename segment_array_implement::const_pointer const_pointer;

    public:
        const_reverse_iterator(node_t *in_node, size_type in_where, segment_array_implement const *in_tree) : node(in_node), where(in_where), tree_(in_tree), leaf_bound_(0), inner_index_(0)
        {
            if(!tree_->is_root_sentinel_(in_node))
                tree_->find_leaf_in_parent_(static_cast<leaf_node_t const *>(in_node), leaf_bound_, inner_index_);
        }
        const_reverse_iterator(pair_pos_t pos, segment_array_implement const *self) : node(pos.first == nullptr ? self->root_.parent->parent : pos.first), where(pos.second), tree_(self), leaf_bound_(0), inner_index_(0)
        {
            if(pos.first != nullptr)
                self->find_leaf_in_parent_(pos.first, leaf_bound_, inner_index_);
        }
        explicit const_reverse_iterator(const_iterator const &other) : node(other.node), where(other.where), tree_(other.tree_), leaf_bound_(other.leaf_bound_), inner_index_(other.inner_index_)
        {
            ++*this;
        }
        const_reverse_iterator(reverse_iterator const &other) : node(other.node), where(other.where), tree_(other.tree_), leaf_bound_(other.leaf_bound_), inner_index_(other.inner_index_)
        {
        }
        const_reverse_iterator(const_reverse_iterator const &) = default;
        const_reverse_iterator &operator+=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, -diff);
            return *this;
        }
        const_reverse_iterator &operator-=(difference_type diff)
        {
            tree_->advance_step_(node, where, leaf_bound_, inner_index_, diff);
            return *this;
        }
        const_reverse_iterator operator+(difference_type diff) const
        {
            const_reverse_iterator ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, -diff);
            return ret;
        }
        const_reverse_iterator operator-(difference_type diff) const
        {
            const_reverse_iterator ret = *this;
            tree_->advance_step_(ret.node, ret.where, ret.leaf_bound_, ret.inner_index_, diff);
            return ret;
        }
        difference_type operator-(const_reverse_iterator const &other) const
        {
            return static_cast<difference_type>(tree_->calculate_rank_(other.node, other.where)) - static_cast<difference_type>(tree_->calculate_rank_(node, where));
        }
        const_reverse_iterator &operator++()
        {
            tree_->advance_prev_(node, where, leaf_bound_, inner_index_);
            return *this;
        }
        const_reverse_iterator &operator--()
        {
            tree_->advance_next_(node, where, leaf_bound_, inner_index_);
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
        const_reference operator*() const
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
        bool operator>(const_reverse_iterator const &other) const
        {
            return *this - other > 0;
        }
        bool operator<(const_reverse_iterator const &other) const
        {
            return *this - other < 0;
        }
        bool operator>=(const_reverse_iterator const &other) const
        {
            return *this - other >= 0;
        }
        bool operator<=(const_reverse_iterator const &other) const
        {
            return *this - other <= 0;
        }
        bool operator==(const_reverse_iterator const &other) const
        {
            return node == other.node && where == other.where;
        }
        bool operator!=(const_reverse_iterator const &other) const
        {
            return node != other.node || where != other.where;
        }
        const_iterator base() const
        {
            return ++iterator(node, where, tree_);
        }

    private:
        friend class segment_array_implement;
        node_t *node;
        size_type where;
        segment_array_implement const *tree_;
        size_type leaf_bound_;
        size_type inner_index_;
    };
    typedef std::pair<iterator, bool> pair_ib_t;

protected:
    typedef std::pair<pair_pos_t, bool> pair_posi_t;
    iterator result_(pair_posi_t posi)
    {
        return iterator(posi.first, this);
    }

public:
    ;
    // default
    segment_array_implement() : root_(allocator_type())
    {
    }
    // default
    segment_array_implement(allocator_type const &alloc) : root_(alloc)
    {
    }
    // fill
    explicit segment_array_implement(size_type count, allocator_type const &alloc = allocator_type()) : root_(alloc)
    {
        while(count-- > 0)
        {
            emplace_back(value_type());
        }
    }
    // fill
    segment_array_implement(size_type count, value_type const &value, allocator_type const &alloc = allocator_type()) : root_(alloc)
    {
        while(count-- > 0)
        {
            emplace_back(value);
        }
    }
    //range
    template<class iterator_t, class = typename std::enable_if<segment_array_detail::is_iterator<iterator_t>::value, void>::type> segment_array_implement(iterator_t begin, iterator_t end, allocator_type const &alloc = allocator_type()) : root_(alloc)
    {
        assign(begin, end);
    }
    //copy
    segment_array_implement(segment_array_implement const &other) : root_(other.get_node_allocator_())
    {
        assign(other.begin(), other.end());
    }
    //copy
    segment_array_implement(segment_array_implement const &other, allocator_type const &alloc) : root_(alloc)
    {
        assign(other.begin(), other.end());
    }
    //move
    segment_array_implement(segment_array_implement &&other) : root_(node_allocator_t())
    {
        swap(other);
    }
    //move
    segment_array_implement(segment_array_implement &&other, allocator_type const &alloc) : root_(alloc)
    {
        assign(std::move_iterator<iterator>(other.begin()), std::move_iterator<iterator>(other.end()));
    }
    //initializer list
    segment_array_implement(std::initializer_list<value_type> il, allocator_type const &alloc = allocator_type()) : segment_array_implement(il.begin(), il.end(), alloc)
    {
    }
    //destructor
    ~segment_array_implement()
    {
        clear();
    }
    //copy
    segment_array_implement &operator=(segment_array_implement const &other)
    {
        if(this == &other)
        {
            return *this;
        }
        if(get_node_allocator_() != other.get_node_allocator_())
        {
            clear();
            get_node_allocator_() = other.get_node_allocator_();
        }
        assign(other.cbegin(), other.cend());
        return *this;
    }
    //move
    segment_array_implement &operator=(segment_array_implement &&other)
    {
        if(this == &other)
        {
            return *this;
        }
        swap(other);
        return *this;
    }
    //initializer list
    segment_array_implement &operator=(std::initializer_list<value_type> il)
    {
        assign(il.begin(), il.end());
        return *this;
    }

    allocator_type get_allocator() const
    {
        return root_;
    }

    void swap(segment_array_implement &other)
    {
        std::swap(root_, other.root_);
        fix_root_();
        other.fix_root_();
    }

    typedef std::pair<iterator, iterator> pair_ii_t;
    typedef std::pair<const_iterator, const_iterator> pair_cici_t;

    //range
    template<class iterator_t, class = typename std::enable_if<segment_array_detail::is_iterator<iterator_t>::value, void>::type> void assign(iterator_t assign_begin, iterator_t assign_end)
    {
        size_type count = size();
        iterator it = begin();
        while(true)
        {
            if(assign_begin == assign_end)
            {
                while(count-- > 0)
                {
                    pop_back();
                }
                return;
            }
            if(count-- == 0)
            {
                std::copy(assign_begin, assign_end, std::back_inserter(*this));
                return;
            }
            *it++ = *assign_begin;
            ++assign_begin;
        }
    }
    // fill
    void assign(size_type count, value_type const &value)
    {
        size_type self_count = size();
        iterator it = begin();
        while(true)
        {
            if(count-- == 0)
            {
                while(self_count-- > 0)
                {
                    pop_back();
                }
                return;
            }
            if(self_count-- == 0)
            {
                insert(cend(), count, value);
                return;
            }
            *it++ = value;
        }
    }
    //initializer list
    void assign(std::initializer_list<value_type> il)
    {
        assign(il.begin(), il.end());
    }

    //single element
    iterator insert(const_iterator where, value_type const &value)
    {
        return result_(insert_pos_(is_root_sentinel_(where.node) ? nullptr : static_cast<leaf_node_t *>(where.node), where.where, value));
    }
    //move
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, iterator>::type insert(const_iterator where, in_value_t &&value)
    {
        return result_(insert_pos_(is_root_sentinel_(where.node) ? nullptr : static_cast<leaf_node_t *>(where.node), where.where, std::forward<in_value_t>(value)));
    }
    // fill
    iterator insert(const_iterator where, size_type count, value_type const &value)
    {
        if(count == 0)
        {
            return iterator(where.node, where.where, this);
        }
        size_type index = rank(where);
        insert(where, value);
        while(true)
        {
            pair_pos_t pos = access_index_(root_.parent, index);
            if(--count == 0)
            {
                return iterator(pos, this);
            }
            insert_pos_(pos.first, pos.second, value);
        }
    }
    //range
    template<class iterator_t, class = typename std::enable_if<segment_array_detail::is_iterator<iterator_t>::value, void>::type> iterator insert(const_iterator where, iterator_t insert_begin, iterator_t insert_end)
    {
        if(insert_begin == insert_end)
        {
            return iterator(where.node, where.where, this);
        }
        size_type index = rank(where);
        insert(where, *insert_begin++);
        while(true)
        {
            pair_pos_t pos = access_index_(root_.parent, index);
            if(insert_begin == insert_end)
            {
                return iterator(pos, this);
            }
            insert_pos_(pos.first, pos.second, *insert_begin++);
        }
    }
    //initializer list
    void insert(const_iterator where, std::initializer_list<value_type> il)
    {
        insert(where, il.begin(), il.end());
    }

    void push_back(value_type const &value)
    {
        insert(cend(), value);
    }
    void push_back(value_type &&value)
    {
        insert(cend(), std::move(value));
    }
    void push_front(value_type const &value)
    {
        insert(cbegin(), value);
    }
    void push_front(value_type &&value)
    {
        insert(cbegin(), std::move(value));
    }
    void pop_back()
    {
        leaf_node_t *leaf_node = static_cast<leaf_node_t *>(root_.right);
        size_type leaf_size = get_leaf_size_(leaf_node);
        erase_pos_(leaf_node, leaf_size - 1, leaf_size);
    }
    void pop_front()
    {
        leaf_node_t *leaf_node = static_cast<leaf_node_t *>(root_.left);
        erase_pos_(leaf_node, 0, get_leaf_size_(leaf_node));
    }

    template<class... args_t> iterator emplace(const_iterator where, args_t &&...args)
    {
        return result_(insert_pos_(is_root_sentinel_(where.node) ? nullptr : static_cast<leaf_node_t *>(where.node), where.where, std::move(value_type(std::forward<args_t>(args)...))));
    }
    template<class... args_t> iterator emplace_back(args_t &&...args)
    {
        return emplace(cend(), std::forward<args_t>(args)...);
    }
    template<class... args_t> iterator emplace_front(args_t &&...args)
    {
        return emplace(cbegin(), std::forward<args_t>(args)...);
    }

    iterator erase(const_iterator it)
    {
        if(is_root_sentinel_(root_.parent))
        {
            return end();
        }
        size_type pos_at = rank(it);
        erase_pos_(static_cast<leaf_node_t *>(it.node), it.where, it.leaf_bound_);
        return iterator(access_index_(root_.parent, pos_at), this);
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
    void resize(size_type count)
    {
        if(count > size())
        {
            insert(cend(), count - size(), value_type());
            return;
        }
        while(count < size())
        {
            pop_back();
        }
    }
    void resize(size_type count, value_type const &value)
    {
        if(count > size())
        {
            insert(cend(), count - size(), value);
            return;
        }
        while(count < size())
        {
            pop_back();
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

    reference operator[](size_type index)
    {
        return *iterator(access_index_(root_.parent, index), this);
    }
    const_reference operator[](size_type index) const
    {
        return *const_iterator(access_index_(root_.parent, index), this);
    }
    reference at(size_type index)
    {
        if(index >= size())
        {
            throw std::out_of_range("segment_array out of range");
        }
        return *iterator(access_index_(root_.parent, index), this);
    }
    const_reference at(size_type index) const
    {
        if(index >= size())
        {
            throw std::out_of_range("segment_array out of range");
        }
        return *const_iterator(access_index_(root_.parent, index), this);
    }

    //rank(begin) == 0, rank of iterator
    static size_type rank(const_iterator where)
    {
        return where.tree_->calculate_rank_(where.node, where.where);
    }

    status_t const &status() const
    {
        static_assert(config_t::status_type::value, "status_type false");
        return root_;
    }

protected:
    root_node_t root_;

protected:
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
    void dealloc_inner_node_(inner_node_t *node)
    {
        get_node_allocator_().deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }
    void dealloc_leaf_node_(leaf_node_t *node)
    {
        size_type sz = (node->parent != nullptr) ? get_leaf_size_(node) : 0;
        destroy_range_(node->item, node->item + sz);
        get_node_allocator_().deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }

    template<bool is_recursive> void free_node_(node_t *node)
    {
        if(node->level == 0)
        {
            dealloc_leaf_node_(static_cast<leaf_node_t *>(node));
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
            dealloc_inner_node_(inner_node);
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
                    if(!is_root_sentinel_(parent) && inner_index < size_type(inner_node_t::max) && parent->children[inner_index + 1].ptr == node)
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

    template<class iterator_t, class in_value_t> static void construct_one_(iterator_t where, in_value_t &&value)
    {
        segment_array_detail::construct_one(where, std::forward<in_value_t>(value), typename segment_array_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_t> static void destroy_one_(iterator_t where)
    {
        segment_array_detail::destroy_one(where, typename segment_array_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_t> static void destroy_range_(iterator_t destroy_begin, iterator_t destroy_end)
    {
        segment_array_detail::destroy_range(destroy_begin, destroy_end, typename segment_array_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_from_t, class iterator_to_t> static void move_forward_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        segment_array_detail::move_forward(move_begin, move_end, to_begin, typename segment_array_detail::get_tag<iterator_from_t>::type());
    }

    template<class iterator_from_t, class iterator_to_t> static void move_construct_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        segment_array_detail::move_construct(move_begin, move_end, to_begin, typename segment_array_detail::get_tag<iterator_from_t>::type());
    }

    template<class iterator_t> static void move_next_to_and_construct_(iterator_t move_begin, iterator_t move_end, iterator_t to_begin)
    {
        segment_array_detail::move_next_to_and_construct(move_begin, move_end, to_begin, typename segment_array_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_from_t, class iterator_to_t> static void move_and_destroy_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        segment_array_detail::move_and_destroy(move_begin, move_end, to_begin, typename segment_array_detail::get_tag<iterator_from_t>::type());
    }

    template<class iterator_from_t, class iterator_to_t> static void move_construct_and_destroy_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        segment_array_detail::move_construct_and_destroy(move_begin, move_end, to_begin, typename segment_array_detail::get_tag<iterator_from_t>::type());
    }

    template<class iterator_t, class in_value_t> static void move_next_and_insert_one_(iterator_t move_begin, iterator_t move_end, in_value_t &&value)
    {
        segment_array_detail::move_next_and_insert_one(move_begin, move_end, std::forward<in_value_t>(value), typename segment_array_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_t> static void move_prev_and_destroy_one_(iterator_t move_begin, iterator_t move_end)
    {
        segment_array_detail::move_prev_and_destroy_one(move_begin, move_end, typename segment_array_detail::get_tag<iterator_t>::type());
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

    template<class in_value_t> pair_posi_t insert_pos_(leaf_node_t *leaf_node, size_type where, in_value_t &&value)
    {
        if(root_.size == 0)
        {
            return insert_first_(std::forward<in_value_t>(value));
        }
        if(leaf_node == nullptr)
        {
            leaf_node = static_cast<leaf_node_t *>(root_.right);
            where = get_leaf_size_(leaf_node);
        }
        size_type leaf_size = get_leaf_size_(leaf_node);
        node_t *split_node = nullptr;
        inner_node_t *parent = nullptr;
        size_type parent_where = 0;
        size_type new_leaf_size = 0;
        if(leaf_is_full_size_(leaf_size))
        {
            parent_where = get_parent_(leaf_node, parent);
            split_leaf_node_(leaf_node, leaf_size, split_node, new_leaf_size);
            size_type mid = leaf_size >> 1;
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
        if(split_node == nullptr)
        {
            update_size_chain_(leaf_node, 1);
        }
        else
        {
            leaf_node_t *split_leaf = static_cast<leaf_node_t *>(split_node);
            size_type split_after = (leaf_node == split_leaf) ? leaf_size : new_leaf_size;
            if(leaf_node != split_leaf)
            {
                set_node_size_(leaf_node, leaf_size);
            }
            insert_pos_descend_(parent, parent_where, split_node, split_after);
        }
        return std::make_pair(std::make_pair(leaf_node, where), true);
    }

    void insert_pos_descend_(inner_node_t *inner_node, size_type where, node_t *new_child, size_type new_child_size)
    {
        if(inner_node == nullptr)
        {
            inner_node_t *new_root = alloc_inner_node_(&root_, root_.parent->level + 1);
            new_root->children[0].ptr = root_.parent;
            new_root->children[1].ptr = new_child;
            set_inner_key_count_(new_root, 1);
            new_root->children[0].size = (root_.parent->level == 0) ? root_.size : node_size_(root_.parent);
            new_root->children[1].size = new_child_size;
            root_.parent->parent = new_root;
            new_child->parent = new_root;
            root_.parent = new_root;
            root_.size = new_root->children[0].size + new_root->children[1].size;
            return;
        }
        node_t *split_node = nullptr;
        inner_node_t *parent = nullptr;
        size_type parent_where = 0;
        inner_node_t *left_after_split = nullptr;
        do
        {
            if(inner_node->is_full())
            {
                parent_where = get_parent_(inner_node, parent);
                split_inner_node_(inner_node, split_node, where);
                inner_node_t *split_tree_node = static_cast<inner_node_t *>(split_node);
                left_after_split = inner_node;
                size_type inner_used = inner_node->key_count();
                size_type split_used = split_tree_node->key_count();
                if(where == inner_used + 1 && inner_used < split_used)
                {
                    inner_node->children[inner_used + 1] = split_tree_node->children[0];
                    set_inner_key_count_(inner_node, inner_used + 1);
                    inner_node->children[inner_used + 1].ptr->parent = inner_node;
                    new_child->parent = split_tree_node;
                    split_tree_node->children[0].ptr = new_child;
                    split_tree_node->children[0].size = new_child_size;
                    break;
                }
                else if(where >= size_type(inner_used + 1))
                {
                    where -= inner_used + 1;
                    inner_node = split_tree_node;
                }
            }
            size_type cur_used = inner_node->key_count();
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
                parent->children[parent_where].size = node_size_(left_after_split);
            }
            insert_pos_descend_(parent, parent_where, split_node, node_size_(split_node));
        }
    }

    void split_inner_node_(inner_node_t *inner_node, node_t *&new_node, size_type where)
    {
        size_type used = inner_node->key_count();
        size_type mid = (used >> 1);
        if(where <= mid && mid > used - (mid + 1))
        {
            --mid;
        }
        inner_node_t *new_inner_node = alloc_inner_node_(inner_node->parent, inner_node->level);
        size_type new_used = used - (mid + 1);
        std::copy(inner_node->children + mid + 1, inner_node->children + used + 1, new_inner_node->children);
        set_inner_key_count_(new_inner_node, new_used);
        set_inner_key_count_(inner_node, mid);
        update_parent_(new_inner_node->children, new_inner_node->children + new_used + 1, new_inner_node);
        new_node = new_inner_node;
    }

    void split_leaf_node_(leaf_node_t *leaf_node, size_type leaf_size, node_t *&new_node, size_type &new_leaf_size_out)
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

    void shift_left_leaf_(leaf_node_t *left, size_type left_size, leaf_node_t *right, size_type right_size, inner_node_t *parent, size_type parent_where)
    {
        (void)parent;
        (void)parent_where;
        size_type shiftnum = (right_size - left_size) >> 1;
        move_construct_(right->item, right->item + shiftnum, left->item + left_size);
        size_type new_left_size = left_size + shiftnum;
        move_forward_(right->item + shiftnum, right->item + right_size, right->item);
        destroy_range_(right->item + right_size - shiftnum, right->item + right_size);
        size_type new_right_size = right_size - shiftnum;
        set_node_size_(left, new_left_size);
        set_node_size_(right, new_right_size);
    }

    void shift_right_leaf_(leaf_node_t *left, size_type left_size, leaf_node_t *right, size_type right_size, inner_node_t *parent, size_type parent_where)
    {
        (void)parent;
        (void)parent_where;
        size_type shiftnum = (left_size - right_size) >> 1;
        move_next_to_and_construct_(right->item, right->item + right_size, right->item + shiftnum);
        size_type new_right_size = right_size + shiftnum;
        move_and_destroy_(left->item + left_size - shiftnum, left->item + left_size, right->item);
        size_type new_left_size = left_size - shiftnum;
        set_node_size_(left, new_left_size);
        set_node_size_(right, new_right_size);
    }

    result_t merge_inners_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        (void)parent;
        (void)parent_where;
        size_type lused = left->key_count();
        size_type rused = right->key_count();
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
        (void)parent;
        (void)parent_where;
        size_type lused = left->key_count();
        size_type rused = right->key_count();
        size_type shiftnum = (rused - lused) >> 1;
        std::copy(right->children, right->children + shiftnum, left->children + lused + 1);
        update_parent_(left->children + lused + 1, left->children + lused + 1 + shiftnum, left);
        set_inner_key_count_(left, lused + shiftnum);
        std::copy(right->children + shiftnum, right->children + rused + 1, right->children);
        set_inner_key_count_(right, rused - shiftnum);
        set_node_size_(left, node_size_(left));
        set_node_size_(right, node_size_(right));
    }

    void shift_right_inner_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        (void)parent;
        (void)parent_where;
        size_type lused = left->key_count();
        size_type rused = right->key_count();
        size_type shiftnum = (lused - rused) >> 1;
        std::copy_backward(right->children, right->children + rused + 1, right->children + rused + 1 + shiftnum);
        set_inner_key_count_(right, rused + shiftnum);
        std::copy(left->children + lused - shiftnum + 1, left->children + lused + 1, right->children);
        update_parent_(right->children, right->children + shiftnum, right);
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
        if(where >= size_type(inner_node_t::max) || parent->children[where + 1].ptr == nullptr)
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
        if(where >= size_type(inner_node_t::max) || parent->children[where + 1].ptr == nullptr)
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
        if(leaf_is_underflow_size_(leaf_size) && !(leaf_node == root_.parent && leaf_size >= 1))
        {
            parent_where = get_parent_(leaf_node, parent);
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
                    shift_left_leaf_(leaf_node, leaf_size, leaf_right, leaf_right_size, right_parent, parent_where);
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
                    shift_left_leaf_(leaf_node, leaf_size, leaf_right, leaf_right_size, right_parent, parent_where);
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
                    shift_left_leaf_(leaf_node, leaf_size, leaf_right, leaf_right_size, right_parent, parent_where);
                }
            }
        }
        if(result.has(result_flags_t(btree_fixmerge)))
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
        size_type parent_where = 0;
        if(result.has(btree_fixmerge))
        {
            parent_where = get_parent_(inner_node, parent);
            if(inner_node->children[where].ptr->parent != nullptr)
            {
                ++where;
            }
            free_node_<false>(inner_node->children[where].ptr);
            size_type used = inner_node->key_count();
            std::copy(inner_node->children + where + 1, inner_node->children + used + 1, inner_node->children + where);
            set_inner_key_count_(inner_node, used - 1);
            if(inner_node->level == 1)
            {
                --where;
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
        if(self_result.has(result_flags_t(btree_fixmerge)))
        {
            if(parent != nullptr)
            {
                erase_pos_descend_(parent, parent_where, std::move(self_result));
            }
        }
    }
};

template<class value_t, class allocator_t = std::allocator<value_t>>
using segment_array = segment_array_implement<segment_array_config<value_t, allocator_t>>;