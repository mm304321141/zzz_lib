#pragma once

#ifndef ZZZ_LIB_NODISCARD
#if __cplusplus >= 201703L
#define ZZZ_LIB_NODISCARD [[nodiscard]]
#else
#define ZZZ_LIB_NODISCARD
#endif
#endif

// segment_array_implement is a segmented array (B+-tree-like) deque.
//
// Iterator / reference invalidation contract:
//   - insert() / emplace(): may split leaf or inner nodes and rebalance
//     ancestors. Iterators and references to elements in the affected leaves
//     (and any leaf reached through a rebalanced path) are invalidated.
//     Conservatively treat any insert that grows the container as
//     invalidating ALL outstanding iterators and references.
//   - erase() / pop_front() / pop_back(): may merge or rebalance leaf and
//     inner nodes. Iterators and references to the erased element are always
//     invalidated. Iterators and references to elements in the affected
//     leaves (and any leaf reached through a rebalanced path) are
//     invalidated. Conservatively treat any erase as invalidating ALL
//     outstanding iterators and references.
//   - clear() / operator= / move / swap: invalidate ALL outstanding iterators
//     and references (swap re-targets them at the other container).
//   - Read-only access (operator[] / at / front / back / iterator traversal)
//     never invalidates.

#include <cstdint>
#include <algorithm>
#include <memory>
#include <cstring>
#include <type_traits>
#include <tuple>
#include <iterator>
#include <vector>
#include <utility>
#include <cassert>

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
    class move_base_tag
    {
    };
    class move_assign_tag : public move_base_tag
    {
    };
    class move_noexcept_tag : public move_base_tag
    {
    };
    template<class iterator_t> struct get_tag
    {
        typedef typename std::iterator_traits<iterator_t>::value_type value_type;
        typedef typename std::conditional<
            std::is_trivial<value_type>::value,
            move_trivial_tag,
            typename std::conditional<
                std::is_nothrow_move_constructible<value_type>::value,
                move_noexcept_tag,
                move_assign_tag>::type>::type type;
    };

    template<class iterator_t, class in_value_t, class tag_t> void construct_one(iterator_t where, in_value_t &&value, tag_t)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        ::new(std::addressof(*where)) iterator_value_t(std::forward<in_value_t>(value));
    }

    template<class iterator_t> void destroy_one(iterator_t, move_trivial_tag)
    {
    }
    template<class iterator_t> void destroy_one(iterator_t where, move_base_tag)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        where->~iterator_value_t();
    }

    template<class iterator_t> void destroy_range(iterator_t, iterator_t, move_trivial_tag)
    {
    }
    template<class iterator_t> void destroy_range(iterator_t destroy_begin, iterator_t destroy_end, move_base_tag)
    {
        for(; destroy_begin != destroy_end; ++destroy_begin)
        {
            destroy_one(destroy_begin, move_base_tag());
        }
    }

    template<class iterator_from_t, class iterator_to_t> void move_forward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(static_cast<void *>(&*to_begin), &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_forward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        std::copy(move_begin, move_end, to_begin);
    }
    template<class iterator_from_t, class iterator_to_t> void move_forward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_noexcept_tag)
    {
        std::move(move_begin, move_end, to_begin);
    }

    template<class iterator_from_t, class iterator_to_t> void move_backward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(static_cast<void *>(&*to_begin), &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_backward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        std::copy_backward(move_begin, move_end, to_begin + (move_end - move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_backward(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_noexcept_tag)
    {
        std::move_backward(move_begin, move_end, to_begin + (move_end - move_begin));
    }

    template<class iterator_from_t, class iterator_to_t> void move_construct(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(static_cast<void *>(&*to_begin), &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        std::uninitialized_copy(move_begin, move_end, to_begin);
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_noexcept_tag)
    {
        for(; move_begin != move_end; ++move_begin, (void)++to_begin)
        {
            construct_one(to_begin, std::move(*move_begin), move_noexcept_tag());
        }
    }

    template<class iterator_t> void move_next_to_and_construct(iterator_t move_begin, iterator_t move_end, iterator_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(static_cast<void *>(&*to_begin), &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_t, class tag_t>
    typename std::enable_if<std::is_base_of<move_base_tag, tag_t>::value>::type
    move_next_to_and_construct(iterator_t move_begin, iterator_t move_end, iterator_t to_begin, tag_t)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        if(to_begin < move_end)
        {
            iterator_t split = move_end - (to_begin - move_begin);
            move_construct(split, move_end, move_end, tag_t());
            move_backward(move_begin, split, to_begin, tag_t());
        }
        else
        {
            move_construct(move_begin, move_end, to_begin, tag_t());
            std::uninitialized_fill(move_end, to_begin, iterator_value_t());
        }
    }

    template<class iterator_from_t, class iterator_to_t> void move_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(static_cast<void *>(&*to_begin), &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        for(; move_begin != move_end; ++move_begin)
        {
            *to_begin++ = *move_begin;
            destroy_one(move_begin, move_assign_tag());
        }
    }
    template<class iterator_from_t, class iterator_to_t> void move_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_noexcept_tag)
    {
        for(; move_begin != move_end; ++move_begin)
        {
            *to_begin++ = std::move(*move_begin);
            destroy_one(move_begin, move_noexcept_tag());
        }
    }

    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(static_cast<void *>(&*to_begin), &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        iterator_to_t to_cur = to_begin;
        iterator_from_t from = move_begin;
        try
        {
            for(; from != move_end; ++from)
            {
                construct_one(to_cur, *from, move_assign_tag());
                ++to_cur;
            }
        }
        catch(...)
        {
            destroy_range(to_begin, to_cur, move_assign_tag());
            throw;
        }
        destroy_range(move_begin, move_end, move_assign_tag());
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_noexcept_tag)
    {
        iterator_from_t move_begin_orig = move_begin;
        for(; move_begin != move_end; ++move_begin)
        {
            construct_one(to_begin++, std::move(*move_begin), move_noexcept_tag());
        }
        destroy_range(move_begin_orig, move_end, move_noexcept_tag());
    }

    template<class iterator_t, class in_value_t> void move_next_and_insert_one(iterator_t move_begin, iterator_t move_end, in_value_t &&value, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(static_cast<void *>(&*(move_begin + 1)), &*move_begin, count * sizeof(*move_begin));
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
            construct_one(move_end, *from_end, move_assign_tag());
            // The slot at move_end now holds a constructed element that the
            // caller has not yet accounted for. If move_backward or the value
            // assignment throws, destroy it so it is not leaked; disarm once
            // the insertion completes and the slot becomes a tracked element.
            struct tail_guard_t
            {
                iterator_t pos;
                bool armed;
                tail_guard_t(iterator_t in_pos) : pos(in_pos), armed(true)
                {
                }
                ~tail_guard_t()
                {
                    if(armed)
                    {
                        destroy_one(pos, move_assign_tag());
                    }
                }
            } tail_guard(move_end);
            move_backward(move_begin, from_end, move_begin + 1, move_assign_tag());
            *move_begin = std::forward<in_value_t>(value);
            tail_guard.armed = false;
        }
    }
    template<class iterator_t, class in_value_t> void move_next_and_insert_one(iterator_t move_begin, iterator_t move_end, in_value_t &&value, move_noexcept_tag)
    {
        if(move_begin == move_end)
        {
            construct_one(move_begin, std::forward<in_value_t>(value), move_noexcept_tag());
        }
        else
        {
            iterator_t from_end = std::prev(move_end);
            construct_one(move_end, std::move(*from_end), move_noexcept_tag());
            move_backward(move_begin, from_end, move_begin + 1, move_noexcept_tag());
            *move_begin = std::forward<in_value_t>(value);
        }
    }

    template<class iterator_t> void move_prev_and_destroy_one(iterator_t move_begin, iterator_t move_end, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(static_cast<void *>(&*(move_begin - 1)), &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_t, class tag_t>
    typename std::enable_if<std::is_base_of<move_base_tag, tag_t>::value>::type
    move_prev_and_destroy_one(iterator_t move_begin, iterator_t move_end, tag_t)
    {
        move_forward(move_begin, move_end, move_begin - 1, tag_t());
        destroy_one(move_end - 1, move_base_tag());
    }
}

// Iterator/reference invalidation rules (deque-like semantics):
//   insert at pos: pos and all iterators after it are invalidated; iterators
//     before pos remain valid.
//   erase at pos: pos and all iterators after it are invalidated; iterators
//     before pos remain valid.
//   push_back: only end() is invalidated.
//   push_front: existing iterators may be invalidated (front addresses can
//     shift) and any iterator preceding the previous begin() does not exist.
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
        // sentinel invariant: children[0..entry_count] hold valid pointers. Scan
        // backward over empty slots until reaching the last valid child slot.
        size_t entry_count() const
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
        // is_minimal(): entry_count <= min. Equivalent to children[min + 1] being unused,
        // because children[k] == nullptr iff entry_count < k.
        bool is_minimal() const
        {
            return children[min + 1].ptr == nullptr;
        }
        // is_underflow(): entry_count < min, i.e. children[min] is unused.
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
        std::vector<size_type, typename std::allocator_traits<allocator_type>::template rebind_alloc<size_type>> level_count;
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
                while(!status.level_count.empty() && status.level_count.back() == 0)
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
                while(!status.level_count.empty() && status.level_count.back() == 0)
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
    template<bool IsConst, bool IsReverse> class iterator_impl
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename segment_array_implement::value_type value_type;
        typedef typename segment_array_implement::difference_type difference_type;
        typedef typename segment_array_implement::reference reference;
        typedef typename segment_array_implement::const_reference const_reference;
        typedef typename segment_array_implement::pointer pointer;
        typedef typename segment_array_implement::const_pointer const_pointer;
        typedef typename std::conditional<IsConst, const_reference, reference>::type deref_reference;
        typedef typename std::conditional<IsConst, const_pointer, pointer>::type deref_pointer;

    public:
        iterator_impl(node_t *in_node, size_type in_where, segment_array_implement const *in_tree) : node(in_node), where(in_where), tree_(in_tree), leaf_bound_(0), inner_index_(0)
        {
            if(!in_tree->is_root_sentinel_(in_node))
                in_tree->find_leaf_in_parent_(static_cast<leaf_node_t const *>(in_node), leaf_bound_, inner_index_);
        }
        // context-carrying overload: the leaf's size and its index in the parent were
        // already resolved during the descent (e.g. by at()), so accept them
        // directly instead of re-deriving them via find_leaf_in_parent_.
        iterator_impl(node_t *in_node, size_type in_where, size_type in_leaf_bound, size_type in_inner_index, segment_array_implement const *in_tree) : node(in_node), where(in_where), tree_(in_tree), leaf_bound_(in_leaf_bound), inner_index_(in_inner_index)
        {
        }
        // non-const overload: builds a writable iterator over a mutable tree
        template<bool C = IsConst, typename std::enable_if<!C, int>::type = 0> iterator_impl(pair_pos_t pos, segment_array_implement *self) : node(pos.first == nullptr ? static_cast<node_t *>(&self->root_) : static_cast<node_t *>(pos.first)), where(pos.second), tree_(self), leaf_bound_(0), inner_index_(0)
        {
            if(pos.first != nullptr)
                self->find_leaf_in_parent_(pos.first, leaf_bound_, inner_index_);
        }
        // const overload: derives the non-const sentinel pointer through parent->parent
        template<bool C = IsConst, typename std::enable_if<C, int>::type = 0> iterator_impl(pair_pos_t pos, segment_array_implement const *self) : node(pos.first == nullptr ? self->root_.parent->parent : pos.first), where(pos.second), tree_(self), leaf_bound_(0), inner_index_(0)
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
        friend class segment_array_implement;
        template<bool, bool> friend class iterator_impl;
        node_t *node;
        size_type where;
        segment_array_implement const *tree_;
        size_type leaf_bound_;
        size_type inner_index_;
    };
    using iterator = iterator_impl<false, false>;
    using const_iterator = iterator_impl<true, false>;
    using reverse_iterator = iterator_impl<false, true>;
    using const_reverse_iterator = iterator_impl<true, true>;
    typedef std::pair<iterator, bool> pair_ib_t;

protected:
    typedef std::pair<pair_pos_t, bool> pair_posi_t;
    iterator result_(pair_posi_t posi)
    {
        return iterator(posi.first, this);
    }

public:
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
    segment_array_implement(segment_array_implement &&other) noexcept : root_(node_allocator_t())
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
    segment_array_implement &operator=(segment_array_implement &&other) noexcept
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

    void swap(segment_array_implement &other) noexcept
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
    iterator insert(const_iterator where, std::initializer_list<value_type> il)
    {
        return insert(where, il.begin(), il.end());
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
        if(is_root_sentinel_(root_.parent) || it.node == nullptr || is_root_sentinel_(it.node))
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

    ZZZ_LIB_NODISCARD iterator begin()
    {
        return iterator(root_.left, 0, this);
    }
    ZZZ_LIB_NODISCARD iterator end()
    {
        return iterator(&root_, 0, this);
    }
    ZZZ_LIB_NODISCARD const_iterator begin() const
    {
        return const_iterator(root_.left, 0, this);
    }
    ZZZ_LIB_NODISCARD const_iterator end() const
    {
        return const_iterator(root_.parent->parent, 0, this);
    }
    ZZZ_LIB_NODISCARD const_iterator cbegin() const
    {
        return const_iterator(root_.left, 0, this);
    }
    ZZZ_LIB_NODISCARD const_iterator cend() const
    {
        return const_iterator(root_.parent->parent, 0, this);
    }
    ZZZ_LIB_NODISCARD reverse_iterator rbegin()
    {
        return reverse_iterator(root_.right, root_.size == 0 ? 0 : get_leaf_size_(static_cast<leaf_node_t *>(root_.right)) - 1, this);
    }
    ZZZ_LIB_NODISCARD reverse_iterator rend()
    {
        return reverse_iterator(&root_, 0, this);
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(root_.right, root_.size == 0 ? 0 : get_leaf_size_(static_cast<leaf_node_t *>(root_.right)) - 1, this);
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator rend() const
    {
        return const_reverse_iterator(root_.parent->parent, 0, this);
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(root_.right, root_.size == 0 ? 0 : get_leaf_size_(static_cast<leaf_node_t *>(root_.right)) - 1, this);
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator crend() const
    {
        return const_reverse_iterator(root_.parent->parent, 0, this);
    }

    ZZZ_LIB_NODISCARD reference front()
    {
        assert(!empty());
        return reinterpret_cast<reference>(static_cast<leaf_node_t *>(root_.left)->item[0]);
    }
    ZZZ_LIB_NODISCARD reference back()
    {
        assert(!empty());
        leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
        return reinterpret_cast<reference>(tail->item[get_leaf_size_(tail) - 1]);
    }

    ZZZ_LIB_NODISCARD const_reference front() const
    {
        assert(!empty());
        return reinterpret_cast<const_reference>(static_cast<leaf_node_t *>(root_.left)->item[0]);
    }
    ZZZ_LIB_NODISCARD const_reference back() const
    {
        assert(!empty());
        leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
        return reinterpret_cast<const_reference>(tail->item[get_leaf_size_(tail) - 1]);
    }

    ZZZ_LIB_NODISCARD bool empty() const
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
            append_n_back_(count - size(), value_type());
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
            append_n_back_(count - size(), value);
            return;
        }
        while(count < size())
        {
            pop_back();
        }
    }
    ZZZ_LIB_NODISCARD size_type size() const
    {
        return root_.size;
    }
    ZZZ_LIB_NODISCARD size_type max_size() const
    {
        return std::allocator_traits<node_allocator_t>::max_size(node_allocator_t(get_node_allocator_()));
    }

    reference operator[](size_type index)
    {
        // single descent: access_index_full_ resolves the leaf, the slot, the leaf
        // size and the leaf's index in its parent, so the iterator is built directly
        // without a second find_leaf_in_parent_ scan.
        leaf_node_t *leaf_node;
        size_type where;
        size_type leaf_size;
        size_type inner_index;
        std::tie(leaf_node, where, leaf_size, inner_index) = access_index_full_(root_.parent, index);
        return *iterator(static_cast<node_t *>(leaf_node), where, leaf_size, inner_index, this);
    }
    const_reference operator[](size_type index) const
    {
        leaf_node_t *leaf_node;
        size_type where;
        size_type leaf_size;
        size_type inner_index;
        std::tie(leaf_node, where, leaf_size, inner_index) = access_index_full_(root_.parent, index);
        return *const_iterator(static_cast<node_t *>(leaf_node), where, leaf_size, inner_index, this);
    }
    ZZZ_LIB_NODISCARD reference at(size_type index)
    {
        if(index >= size())
        {
            throw std::out_of_range("segment_array out of range");
        }
        leaf_node_t *leaf_node;
        size_type where;
        size_type leaf_size;
        size_type inner_index;
        std::tie(leaf_node, where, leaf_size, inner_index) = access_index_full_(root_.parent, index);
        return *iterator(static_cast<node_t *>(leaf_node), where, leaf_size, inner_index, this);
    }
    ZZZ_LIB_NODISCARD const_reference at(size_type index) const
    {
        if(index >= size())
        {
            throw std::out_of_range("segment_array out of range");
        }
        leaf_node_t *leaf_node;
        size_type where;
        size_type leaf_size;
        size_type inner_index;
        std::tie(leaf_node, where, leaf_size, inner_index) = access_index_full_(root_.parent, index);
        return *const_iterator(static_cast<node_t *>(leaf_node), where, leaf_size, inner_index, this);
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
    // Maintain the nullptr sentinel invariant so that node->entry_count() == new_used:
    // children[0..new_used] stay valid, children[new_used + 1 .. max] are cleared.
    // A full node (new_used == max) keeps every slot and has no sentinel.
    void set_inner_entry_count_(inner_node_t *node, size_type new_used)
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
        size_type child_count = inner->entry_count();
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
    bool leaf_is_minimal_size_(size_type sz) const
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
        size_type child_count = inner_node->entry_count();
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

    // insert_pos_descend_ propagates a hardcoded +1 to the ancestor size chain
    // (its single-element-insert contract). When a freshly built leaf carrying
    // more than one element is hooked in, recompute every ancestor slot bottom-up
    // along that leaf's path so each level reflects the true subtree size. Slots
    // already exact (siblings off the path, levels below the split point) are
    // rewritten with the identical value, so the pass is idempotent and safe.
    void recompute_size_chain_(node_t *node)
    {
        for(;;)
        {
            node_t *parent = node->parent;
            if(is_root_sentinel_(parent))
            {
                root_.size = node_size_(node);
                break;
            }
            inner_node_t *inner_parent = static_cast<inner_node_t *>(parent);
            inner_parent->children[child_index_(inner_parent, node)].size = node_size_(node);
            node = parent;
        }
    }

    void fix_root_()
    {
        // After std::swap on root_node_t, root_.parent/left/right may carry the
        // peer tree's self-sentinel pointer when the peer was empty. Rather than
        // probing those (now stale) pointers via is_root_sentinel_(), use the
        // size invariant size_==0 <=> empty tree to decide whether to reset to
        // self-sentinels or to relink real nodes back to this root_.
        if(root_.size == 0)
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
    // Inner nodes carry no separation keys (this is an index-based container, not
    // a keyed B+ tree), so there is nothing to destroy; just release the storage.
    void dealloc_node_(inner_node_t *node)
    {
        get_node_allocator_().deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }
    void dealloc_node_(leaf_node_t *node)
    {
        size_type sz = (node->parent != nullptr) ? get_leaf_size_(node) : 0;
        destroy_range_(node->item, node->item + sz);
        get_node_allocator_().deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }

    // Release the raw storage of a freshly allocated node without destroying any
    // item. Used by the commit guard on the exception path of split/insert, where
    // a node has been allocated but the operation has not been committed; the
    // status counters allocated by alloc_*_node_ are rolled back here.
    void raw_free_node_(node_t *node, bool is_leaf)
    {
        if(is_leaf)
        {
            status_control_t::change_leaf(root_, -1);
        }
        else
        {
            status_control_t::change_inner(root_, -1, node->level);
        }
        get_node_allocator_().deallocate(reinterpret_cast<memory_node_t *>(node), 1);
    }

    // RAII guard holding a freshly allocated node. If the owning operation throws
    // before reaching its noexcept commit point, the node's storage is released so
    // that no node leaks. release() is called once the operation has committed.
    struct node_commit_guard_t
    {
        segment_array_implement *tree;
        node_t *node;
        bool is_leaf;
        node_commit_guard_t(segment_array_implement *t, node_t *n, bool leaf) : tree(t), node(n), is_leaf(leaf)
        {
        }
        ~node_commit_guard_t()
        {
            if(node != nullptr)
            {
                tree->raw_free_node_(node, is_leaf);
            }
        }
        void release()
        {
            node = nullptr;
        }
        node_commit_guard_t(node_commit_guard_t const &) = delete;
        node_commit_guard_t &operator=(node_commit_guard_t const &) = delete;
    };

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
                size_type child_count = inner_node->entry_count();
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

    // Like access_index_, but also surfaces the leaf's size and its index inside its
    // parent (inner_index), both recorded along the descent. Together this is the full
    // context an iterator needs, so at() / operator[] can build the iterator without a
    // second find_leaf_in_parent_ scan.
    std::tuple<leaf_node_t *, size_type, size_type, size_type> access_index_full_(node_t *node, size_type index) const
    {
        if(index >= node_size_(node))
        {
            return std::make_tuple(static_cast<leaf_node_t *>(nullptr), size_type(0), size_type(0), size_type(0));
        }
        size_type leaf_size = root_.size;
        size_type inner_index = 0;
        while(node->level > 0)
        {
            inner_node_t *inner_node = static_cast<inner_node_t *>(node);
            size_type w = 0;
            for(child_slot_t *child = inner_node->children;; ++child, ++w)
            {
                if(index >= child->size)
                {
                    index -= child->size;
                }
                else
                {
                    leaf_size = child->size;
                    inner_index = w;
                    node = child->ptr;
                    break;
                }
            }
        }
        return std::make_tuple(static_cast<leaf_node_t *>(node), index, leaf_size, inner_index);
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
        node_commit_guard_t guard(this, node, true);
        construct_one_(node->item, std::forward<in_value_t>(value));
        guard.release();
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
            size_type orig_size = leaf_size;
            split_leaf_node_(leaf_node, orig_size, split_node, new_leaf_size);
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
            set_inner_entry_count_(new_root, 1);
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
        // Preserve the original inner_node identity: when a split occurs and the
        // insertion target is in the upper half, `inner_node` is reassigned to
        // `split_tree_node`, but parent_where still indexes the slot of the
        // original (lower-half) node in `parent`. Mixing them up corrupts
        // parent->children[parent_where].size and produces wrong root_.size.
        inner_node_t *orig_inner_node = inner_node;
        do
        {
            if(inner_node->is_full())
            {
                parent_where = get_parent_(inner_node, parent);
                split_inner_node_(inner_node, split_node, where);
                inner_node_t *split_tree_node = static_cast<inner_node_t *>(split_node);
                size_type inner_used = inner_node->entry_count();
                size_type split_used = split_tree_node->entry_count();
                if(where == inner_used + 1 && inner_used < split_used)
                {
                    inner_node->children[inner_used + 1] = split_tree_node->children[0];
                    set_inner_entry_count_(inner_node, inner_used + 1);
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
            size_type cur_used = inner_node->entry_count();
            std::move_backward(inner_node->children + where, inner_node->children + cur_used + 1, inner_node->children + cur_used + 2);
            inner_node->children[where + 1].ptr = new_child;
            inner_node->children[where + 1].size = new_child_size;
            set_inner_entry_count_(inner_node, cur_used + 1);
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
                parent->children[parent_where].size = node_size_(orig_inner_node);
            }
            insert_pos_descend_(parent, parent_where, split_node, node_size_(split_node));
        }
    }

    // Bulk tail append used by resize(). Instead of inserting one element at a
    // time (each an O(log N) descend + split bookkeeping), it first tops up the
    // current rightmost leaf, then appends whole leaves filled to ~75% (close to
    // the B+ tree random-operation steady state, so a later random erase does not
    // immediately trip the underflow/merge threshold). Each new leaf is hooked in
    // through the existing split commit path (insert_pos_descend_), so inner-node
    // splits and root growth are handled exactly as for a normal insert.
    void append_n_back_(size_type n, const_reference value)
    {
        if(n == 0)
        {
            return;
        }
        // Empty container: materialize the first leaf so root_.right is a real
        // node and the steady-state append loop below has a tail to extend.
        if(root_.size == 0)
        {
            insert_first_(value);
            if(--n == 0)
            {
                return;
            }
        }
        // Steady-state fill policy: every leaf touched by this resize should end
        // up close to `target` (~75% full, the random-operation steady state) and
        // never below `min_fill` (50%), so a subsequent random erase does not
        // immediately trip the underflow/merge threshold.
        size_type leaf_max = size_type(leaf_node_t::max);
        size_type target = leaf_max * 3 / 4;
        if(target == 0)
        {
            target = 1;
        }
        size_type min_fill = size_type(leaf_node_t::min);
        leaf_node_t *tail = static_cast<leaf_node_t *>(root_.right);
        size_type c = get_leaf_size_(tail);

        // Top up the current rightmost leaf in place with `amt` copies of value
        // (commits the growth up the ancestor chain). Basic exception safety: a
        // throw rolls back the partially constructed slots and leaves the
        // container otherwise untouched.
        auto fill_tail_in_place = [&](size_type amt)
        {
            size_type base = get_leaf_size_(tail);
            std::uninitialized_fill_n(tail->item + base, amt, value);
            update_size_chain_(tail, difference_type(amt));
        };

        // Allocate a fresh leaf carrying `vfill` copies of value and hook it onto
        // the right spine through the standard split-commit path (insert_pos_descend_
        // for structure, recompute_size_chain_ to fix the bulk size the +1
        // propagation under-counts).
        auto append_value_leaf = [&](size_type vfill)
        {
            leaf_node_t *new_leaf = alloc_leaf_node_();
            node_commit_guard_t guard(this, new_leaf, true);
            size_type constructed = 0;
            try
            {
                for(; constructed < vfill; ++constructed)
                {
                    construct_one_(new_leaf->item + constructed, value);
                }
            }
            catch(...)
            {
                destroy_range_(new_leaf->item, new_leaf->item + constructed);
                throw;
            }
            // Commit: link into the leaf-list tail (never throws), then release the
            // guard before handing the node to the parent chain, mirroring the
            // split_leaf_node_ / insert_pos_descend_ commit protocol.
            leaf_node_t *cur_tail = static_cast<leaf_node_t *>(root_.right);
            new_leaf->prev = cur_tail;
            new_leaf->next = &root_;
            cur_tail->next = new_leaf;
            root_.right = new_leaf;
            guard.release();
            // Re-read (parent, parent_where) every round: a previous descend may
            // have split ancestors and invalidated any cached handle.
            inner_node_t *parent = nullptr;
            size_type parent_where = get_parent_(cur_tail, parent);
            insert_pos_descend_(parent, parent_where, new_leaf, vfill);
            recompute_size_chain_(new_leaf);
        };

        // Case 0: the whole request fits in the rightmost leaf, just top it up.
        if(c + n <= leaf_max)
        {
            fill_tail_in_place(n);
            return;
        }
        // Step 1: decide how much tops up the rightmost leaf toward `target`,
        // leaving `rem` for fresh leaves. A leaf already at/over target is left
        // untouched. (Because c + n > leaf_max here, when c < target we always
        // reach target, so the clamp below is defensive only.)
        size_type fill_right = (c < target) ? (target - c) : 0;
        if(fill_right > n)
        {
            fill_right = n;
        }
        size_type rem = n - fill_right;

        // Step 3 (priority, checked before the Step 2 distribution): the remainder
        // is smaller than a half-full leaf, so it cannot stand alone. Since
        // c + n > leaf_max at this point, the rightmost leaf and exactly one fresh
        // leaf must share (c + n) elements; a balanced split gives two leaves that
        // are both >= min_fill (each half > leaf_max / 2 == min_fill). The spec's
        // "absorb rem into the tail" alternative needs c + n <= leaf_max, which is
        // Case 0 above, hence provably unreachable here.
        if(rem < min_fill)
        {
            size_type total = c + n;    // > leaf_max
            size_type left = total / 2; // rightmost leaf's final size
            // Pure-append fast path keeps the tail's existing elements in place and
            // only adds value copies; when the balanced `left` is below the current
            // size we must relocate the overflow into the new leaf instead.
            size_type tail_add = (left > c) ? (left - c) : 0;
            size_type move_count = (c > left) ? (c - left) : 0;
            size_type value_in_new = n - tail_add; // == total - left - move_count
            if(tail_add > 0)
            {
                fill_tail_in_place(tail_add);
            }
            leaf_node_t *new_leaf = alloc_leaf_node_();
            node_commit_guard_t guard(this, new_leaf, true);
            // Throwing work into the still-unlinked new leaf: first the brand-new
            // value copies (placed after the slots reserved for relocated items),
            // then relocate the tail overflow. move_construct_and_destroy_ keeps the
            // source intact if a relocation throws.
            size_type constructed = 0;
            try
            {
                for(; constructed < value_in_new; ++constructed)
                {
                    construct_one_(new_leaf->item + move_count + constructed, value);
                }
            }
            catch(...)
            {
                destroy_range_(new_leaf->item + move_count, new_leaf->item + move_count + constructed);
                throw;
            }
            if(move_count > 0)
            {
                try
                {
                    move_construct_and_destroy_(tail->item + left, tail->item + c, new_leaf->item);
                }
                catch(...)
                {
                    destroy_range_(new_leaf->item + move_count, new_leaf->item + move_count + value_in_new);
                    throw;
                }
            }
            // Commit (never throws): hook the new leaf onto the right spine, fix the
            // shrunken tail's recorded size, then repair both ancestor chains.
            leaf_node_t *cur_tail = static_cast<leaf_node_t *>(root_.right); // == tail
            new_leaf->prev = cur_tail;
            new_leaf->next = &root_;
            cur_tail->next = new_leaf;
            root_.right = new_leaf;
            guard.release();
            if(move_count > 0)
            {
                set_node_size_(tail, left);
            }
            inner_node_t *parent = nullptr;
            size_type parent_where = get_parent_(cur_tail, parent);
            size_type new_leaf_size = move_count + value_in_new; // == total - left
            insert_pos_descend_(parent, parent_where, new_leaf, new_leaf_size);
            recompute_size_chain_(new_leaf);
            if(move_count > 0)
            {
                recompute_size_chain_(tail);
            }
            return;
        }

        // Step 2: top up the tail toward target, then lay down fresh leaves, each
        // close to target and never below min_fill.
        if(fill_right > 0)
        {
            fill_tail_in_place(fill_right);
        }
        size_type full = rem / target;
        size_type last = rem % target;
        size_type plain = full; // number of leaves filled exactly to `target`
        size_type trail1 = 0;
        size_type trail2 = 0; // up to two special trailing leaves
        if(last == 0)
        {
            // rem is a multiple of target: only full leaves.
        }
        else if(last >= min_fill)
        {
            // The remainder is large enough to stand alone (>= min_fill).
            trail1 = last;
        }
        else
        {
            // 0 < last < min_fill: the leftover cannot form its own leaf, so fold
            // it into the final full leaf (here full >= 1 because rem >= min_fill).
            // If the combined leaf would overflow, split it into two balanced
            // leaves, both >= min_fill since the sum exceeds leaf_max. (The spec
            // unconditionally splits target + last, which underflows whenever
            // target + last <= leaf_max; absorbing into a single leaf in that range
            // keeps every leaf >= min_fill and nearer to target.)
            plain = full - 1;
            size_type combined = target + last;
            if(combined <= leaf_max)
            {
                trail1 = combined;
            }
            else
            {
                trail1 = combined / 2;
                trail2 = combined - trail1;
            }
        }
        size_type pending = plain;
        for(;;)
        {
            size_type fill;
            if(pending > 0)
            {
                fill = target;
                --pending;
            }
            else if(trail1 > 0)
            {
                fill = trail1;
                trail1 = 0;
            }
            else if(trail2 > 0)
            {
                fill = trail2;
                trail2 = 0;
            }
            else
            {
                break;
            }
            append_value_leaf(fill);
        }
    }

    void split_inner_node_(inner_node_t *inner_node, node_t *&new_node, size_type where)
    {
        size_type used = inner_node->entry_count();
        size_type mid = (used >> 1);
        if(where <= mid && mid > used - (mid + 1))
        {
            --mid;
        }
        inner_node_t *new_inner_node = alloc_inner_node_(inner_node->parent, inner_node->level);
        node_commit_guard_t guard(this, new_inner_node, false);
        size_type new_used = used - (mid + 1);
        std::copy(inner_node->children + mid + 1, inner_node->children + used + 1, new_inner_node->children);
        set_inner_entry_count_(new_inner_node, new_used);
        set_inner_entry_count_(inner_node, mid);
        update_parent_(new_inner_node->children, new_inner_node->children + new_used + 1, new_inner_node);
        new_node = new_inner_node;
        guard.release();
    }

    void split_leaf_node_(leaf_node_t *leaf_node, size_type leaf_size, node_t *&new_node, size_type &new_leaf_size_out)
    {
        size_type mid = (leaf_size >> 1);
        leaf_node_t *new_leaf_node = alloc_leaf_node_();
        node_commit_guard_t guard(this, new_leaf_node, true);
        new_leaf_size_out = leaf_size - mid;
        // Throwing work first, into the still-unlinked new node. move_construct_and_destroy_
        // keeps the source intact if a move throws, and the guard frees the new leaf so a
        // throwing relocation never leaks it nor corrupts the leaf linked list.
        move_construct_and_destroy_(leaf_node->item + mid, leaf_node->item + leaf_size, new_leaf_node->item);
        // Commit: linked-list pointers and size updates never throw.
        new_leaf_node->next = leaf_node->next;
        if(is_root_sentinel_(new_leaf_node->next))
        {
            root_.right = new_leaf_node;
        }
        else
        {
            static_cast<leaf_node_t *>(new_leaf_node->next)->prev = new_leaf_node;
        }
        set_node_size_(leaf_node, mid);
        leaf_node->next = new_leaf_node;
        new_leaf_node->prev = leaf_node;
        new_node = new_leaf_node;
        guard.release();
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
        size_type lused = left->entry_count();
        size_type rused = right->entry_count();
        std::copy(right->children, right->children + rused + 1, left->children + lused + 1);
        update_parent_(left->children + lused + 1, left->children + lused + 1 + rused + 1, left);
        set_inner_entry_count_(left, lused + 1 + rused);
        right->children[0].ptr = nullptr;
        set_node_size_(left, node_size_(left));
        right->parent = nullptr;
        return result_t(btree_fixmerge);
    }

    void shift_left_inner_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        (void)parent;
        (void)parent_where;
        size_type lused = left->entry_count();
        size_type rused = right->entry_count();
        size_type shiftnum = (rused - lused) >> 1;
        std::copy(right->children, right->children + shiftnum, left->children + lused + 1);
        update_parent_(left->children + lused + 1, left->children + lused + 1 + shiftnum, left);
        set_inner_entry_count_(left, lused + shiftnum);
        std::copy(right->children + shiftnum, right->children + rused + 1, right->children);
        set_inner_entry_count_(right, rused - shiftnum);
        set_node_size_(left, node_size_(left));
        set_node_size_(right, node_size_(right));
    }

    void shift_right_inner_(inner_node_t *left, inner_node_t *right, inner_node_t *parent, size_type parent_where)
    {
        (void)parent;
        (void)parent_where;
        size_type lused = left->entry_count();
        size_type rused = right->entry_count();
        size_type shiftnum = (lused - rused) >> 1;
        std::copy_backward(right->children, right->children + rused + 1, right->children + rused + 1 + shiftnum);
        set_inner_entry_count_(right, rused + shiftnum);
        std::copy(left->children + lused - shiftnum + 1, left->children + lused + 1, right->children);
        update_parent_(right->children, right->children + shiftnum, right);
        set_inner_entry_count_(left, lused - shiftnum);
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
            return left_parent == nullptr ? nullptr : static_cast<in_node_t *>(left_parent->children[left_parent->entry_count() - 1].ptr);
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
            left = left_parent == nullptr ? nullptr : static_cast<in_node_t *>(left_parent->children[left_parent->entry_count() - 1].ptr);
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
                leaf_left_size = (left_parent == parent) ? left_parent->children[parent_where - 1].size : left_parent->children[left_parent->entry_count() - 1].size;
            }
            if(leaf_right != nullptr)
            {
                leaf_right_size = (right_parent == parent) ? right_parent->children[parent_where + 1].size : right_parent->children[0].size;
            }
            bool left_few = (leaf_left == nullptr) || leaf_is_minimal_size_(leaf_left_size);
            bool right_few = (leaf_right == nullptr) || leaf_is_minimal_size_(leaf_right_size);
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
            else if((leaf_left != nullptr && leaf_is_minimal_size_(leaf_left_size)) && (leaf_right != nullptr && !leaf_is_minimal_size_(leaf_right_size)))
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
            else if((leaf_left != nullptr && !leaf_is_minimal_size_(leaf_left_size)) && (leaf_right != nullptr && leaf_is_minimal_size_(leaf_right_size)))
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
            size_type used = inner_node->entry_count();
            std::copy(inner_node->children + where + 1, inner_node->children + used + 1, inner_node->children + where);
            set_inner_entry_count_(inner_node, used - 1);
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
                set_inner_entry_count_(inner_node, 0);
                free_node_<false>(inner_node);
                return;
            }
            else if((inner_left == nullptr || inner_left->is_minimal()) && (inner_right == nullptr || inner_right->is_minimal()))
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
            else if((inner_left != nullptr && inner_left->is_minimal()) && (inner_right != nullptr && !inner_right->is_minimal()))
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
            else if((inner_left != nullptr && !inner_left->is_minimal()) && (inner_right != nullptr && inner_right->is_minimal()))
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
                if(inner_left->entry_count() <= inner_right->entry_count())
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

template<class config_t> bool operator==(segment_array_implement<config_t> const &left, segment_array_implement<config_t> const &right)
{
    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin());
}
#if __cplusplus >= 202002L
#include <compare>
namespace segment_array_detail
{
// Apple Clang's libc++ (< LLVM17) does not provide std::lexicographical_compare_three_way.
// Provide a local fallback when targeting that toolchain; otherwise forward to the standard.
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 170000
    template<class It1, class It2>
    constexpr auto lex_three_way(It1 f1, It1 l1, It2 f2, It2 l2)
        -> decltype(std::compare_three_way{}(*f1, *f2))
    {
        using cat = decltype(std::compare_three_way{}(*f1, *f2));
        while(f1 != l1 && f2 != l2)
        {
            if(auto c = std::compare_three_way{}(*f1, *f2); c != 0)
            {
                return c;
            }
            ++f1;
            ++f2;
        }
        if(f1 != l1)
        {
            return cat::greater;
        }
        if(f2 != l2)
        {
            return cat::less;
        }
        return std::compare_three_way{}(0, 0); // strong_ordering::equal converts to cat
    }
#else
    template<class It1, class It2> constexpr auto lex_three_way(It1 f1, It1 l1, It2 f2, It2 l2)
    {
        return std::lexicographical_compare_three_way(f1, l1, f2, l2);
    }
#endif
} // namespace segment_array_detail
template<class config_t> auto operator<=>(segment_array_implement<config_t> const &left, segment_array_implement<config_t> const &right)
{
    return segment_array_detail::lex_three_way(left.begin(), left.end(), right.begin(), right.end());
}
#else
template<class config_t> bool operator!=(segment_array_implement<config_t> const &left, segment_array_implement<config_t> const &right)
{
    return !(left == right);
}
template<class config_t> bool operator<(segment_array_implement<config_t> const &left, segment_array_implement<config_t> const &right)
{
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}
template<class config_t> bool operator>(segment_array_implement<config_t> const &left, segment_array_implement<config_t> const &right)
{
    return right < left;
}
template<class config_t> bool operator<=(segment_array_implement<config_t> const &left, segment_array_implement<config_t> const &right)
{
    return !(right < left);
}
template<class config_t> bool operator>=(segment_array_implement<config_t> const &left, segment_array_implement<config_t> const &right)
{
    return !(left < right);
}
#endif