#pragma once

#ifndef ZZZ_LIB_NODISCARD
#if __cplusplus >= 201703L
#define ZZZ_LIB_NODISCARD [[nodiscard]]
#else
#define ZZZ_LIB_NODISCARD
#endif
#endif

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <iterator>
#include <type_traits>
#include <utility>
#include <string>
#include <sstream>

struct sparse_array_default_config
{
    typedef void *handle_t;
    enum
    {
        memory_size = 512,  // memory is allocated per block; this defines each block's size (the minimum varies by type; too small fails to compile)
        atomic_length = 4,  // minimum contiguous data length (varies by type; too small fails to compile)
        invalid_handle = 0, // invalid memory handle (treat as nil)
    };
    // allocate a block of memory_size bytes
    handle_t alloc()
    {
        return ::malloc(memory_size);
    }
    // release a previously allocated block
    void dealloc(handle_t handle)
    {
        ::free(handle);
    }
    // resolve a memory handle to its address
    void *get(handle_t handle)
    {
        return handle;
    }
};

template<class value_t, class config_t = sparse_array_default_config>
class sparse_array
{
public:
    typedef sparse_array<value_t, config_t> self_t;
    typedef typename config_t::handle_t handle_t;
    enum
    {
        memory_size = config_t::memory_size,
        atomic_length = config_t::atomic_length,
        invalid_handle = config_t::invalid_handle,
    };
    // memory range
    struct sparse_range
    {
        uint32_t index;
        uint16_t length;
        uint16_t offset;
        handle_t handle;
    };
    // red-black tree node
    struct sparse_range_set_base
    {
        handle_t parent_handle;
        handle_t left_handle;
        handle_t right_handle;
        uint32_t black : 1;
        uint32_t nil : 1;
        uint32_t length : 30;
    };
    // snapshot data
    struct snapshot_data
    {
        handle_t parent_handle;
        handle_t left_handle;
        handle_t right_handle;
        handle_t list_handle;
        handle_t free_handle;
        uint32_t free_length;
        uint16_t free_offset;
        uint16_t free_cross;
    };
    // red-black tree root + memory adapter
    class sparse_range_tree
    {
    public:
        sparse_range_set_base const *root() const
        {
            return &root_;
        }
        sparse_range_set_base *root()
        {
            return &root_;
        }
        handle_t alloc()
        {
            return root_.alloc();
        }
        config_t const &allocator() const
        {
            return root_;
        }
        config_t &allocator()
        {
            return root_;
        }
        void dealloc(handle_t handle)
        {
            root_.dealloc(handle);
        }
        void *deref(handle_t handle) const
        {
            return root_.get(handle);
        }

    private:
        struct sparse_range_root : public sparse_range_set_base, public config_t
        {
        };
        mutable sparse_range_root root_;
    };
    // red-black tree data node, set of memory ranges (data is sorted)
    struct sparse_range_set : public sparse_range_set_base
    {
        uint32_t end;
        sparse_range begin[1];
    };
    // free memory list node
    struct memory_blcok_free
    {
        handle_t handle;
        uint16_t offset;
        uint16_t cross;
    };
    // data block element (data / free-list node)
    struct memory_block_data
    {
        union
        {
            value_t data[atomic_length];
            memory_blcok_free free;
        };
    };
    // data block
    struct memory_block
    {
        handle_t next_handle;
        handle_t prev_handle;
        memory_block_data data[1];
    };
    struct sparse_range_op
    {
        bool operator()(sparse_range const &left, uint32_t const &right) const
        {
            return left.index < right;
        }
        bool operator()(uint32_t const &left, sparse_range const &right) const
        {
            return left < right.index;
        }
    };
    enum
    {
        // number of ranges per range-set
        range_length = (memory_size - sizeof(sparse_range_set)) / sizeof(sparse_range) + 1,
        // number of elements per data block
        block_length = (memory_size - sizeof(memory_block)) / sizeof(memory_block_data) + 1,
    };
    // operator[] proxy
    class index_proxy
    {
    public:
        operator value_t() const
        {
            return self_.get_value_(index_);
        }
        index_proxy const &operator=(value_t const &value) const
        {
            self_.set_value_(index_, value);
            return *this;
        }
        index_proxy const &operator=(index_proxy const &other) const
        {
            self_.set_value_(index_, other.self_.get_value_(other.index_));
            return *this;
        }
        index_proxy(uint32_t index, self_t &self) : index_(index), self_(self)
        {
        }
        index_proxy(index_proxy const &) = default;

    private:
        uint32_t index_;
        self_t &self_;
    };

public:
    sparse_array()
    {
        static_assert(memory_size >= sizeof(memory_block), "low memory_size");
        static_assert(memory_size >= sizeof(sparse_range_set), "low memory_size");
        static_assert(range_length >= 3, "low memory_size");
        static_assert(sizeof(value_t) * atomic_length >= sizeof(memory_blcok_free), "low atomic_length");
        block_handle_ = handle_t(invalid_handle);
        free_.handle = handle_t(invalid_handle);
        // free_.offset = 0;
        // free_.cross = 0;
        index_tree_.root()->length = 0;
        rb_set_nil_(rb_nil_(), true);
        rb_set_root_(rb_nil_());
        rb_set_most_left_(rb_nil_());
        rb_set_most_right_(rb_nil_());
    }
    ~sparse_array()
    {
        for(handle_t rb_it = rb_get_most_left_(); rb_it != rb_nil_();)
        {
            // trivially destructible: no per-element destruction needed
            if(!std::is_trivially_destructible<value_t>::value)
            {
                sparse_range_set *range_set = deref_<sparse_range_set>(rb_it);
                for(sparse_range *range = range_set->begin, *range_end = range + range_set->end; range != range_end; ++range)
                {
                    memory_block_data *data = deref_<memory_block>(range->handle)->data + range->offset;
                    for(value_t *it = data->data, *end = it + range->length; it != end; ++it)
                    {
                        it->~value_t();
                    }
                }
            }
            handle_t next_it = rb_move_<true>(rb_it);
            rb_erase_(rb_it);
            index_tree_.dealloc(rb_it);
            rb_it = next_it;
        }
        while(block_handle_ != handle_t(invalid_handle))
        {
            handle_t next_handle = deref_<memory_block>(block_handle_)->next_handle;
            index_tree_.dealloc(block_handle_);
            block_handle_ = next_handle;
        }
    }

    // disable copy (implicit shallow copy would cause double-free)
    sparse_array(self_t const &) = delete;
    self_t &operator=(self_t const &) = delete;

    // move constructor (transfer all members; leave source in a valid empty state)
    sparse_array(self_t &&other) noexcept
        : index_tree_()
    {
        // transfer allocator (config_t portion)
        index_tree_.allocator() = std::move(other.index_tree_.allocator());
        // transfer rb-tree header state (root/most_left/most_right/length/nil etc.)
        *index_tree_.root() = *other.index_tree_.root();
        block_handle_ = other.block_handle_;
        free_ = other.free_;
        // restore source to a state identical to default construction
        other.block_handle_ = handle_t(invalid_handle);
        other.free_.handle = handle_t(invalid_handle);
        other.rb_clear_();
        other.index_tree_.root()->length = 0;
        other.index_tree_.root()->nil = 1;
    }

    // move assignment (release own resources before taking over to avoid leak / double-free)
    self_t &operator=(self_t &&other) noexcept
    {
        if(this != &other)
        {
            this->~sparse_array();
            ::new(this) self_t(std::move(other));
        }
        return *this;
    }

    // noexcept swap (swap rb-tree header / allocator / data list / free list — all members)
    void swap(self_t &other) noexcept
    {
        using std::swap;
        swap(*index_tree_.root(), *other.index_tree_.root());
        swap(index_tree_.allocator(), other.index_tree_.allocator());
        swap(block_handle_, other.block_handle_);
        swap(free_, other.free_);
    }

    // bidirectional iterator (folded form: const/reverse controlled by template params); only stops at non-default slots
    template<bool IsConst, bool IsReverse>
    class iterator_impl
    {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef value_t value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_t reference;
        typedef value_t *pointer;

    private:
        typedef typename std::conditional<IsConst, self_t const, self_t>::type owner_t;
        friend class sparse_array;
        template<bool, bool> friend class iterator_impl;

        owner_t *owner_;
        handle_t node_;
        uint32_t ri_;
        uint32_t vi_;

        value_t *slot_() const
        {
            sparse_range_set *set = owner_->template deref_<sparse_range_set>(node_);
            sparse_range *rng = set->begin + ri_;
            return owner_->template deref_<memory_block>(rng->handle)->data[rng->offset].data + vi_;
        }
        bool at_default_() const
        {
            return !(*slot_() != value_t());
        }
        void raw_step_forward_()
        {
            sparse_range_set *set = owner_->template deref_<sparse_range_set>(node_);
            ++vi_;
            if(vi_ < set->begin[ri_].length)
            {
                return;
            }
            vi_ = 0;
            ++ri_;
            if(ri_ < set->end)
            {
                return;
            }
            ri_ = 0;
            node_ = owner_->template rb_move_<true>(node_);
        }
        void raw_step_backward_()
        {
            if(owner_->rb_is_nil_(node_))
            {
                node_ = owner_->rb_get_most_right_();
                if(owner_->rb_is_nil_(node_))
                {
                    return;
                }
            }
            else if(vi_ > 0)
            {
                --vi_;
                return;
            }
            else if(ri_ > 0)
            {
                --ri_;
                vi_ = owner_->template deref_<sparse_range_set>(node_)->begin[ri_].length - 1;
                return;
            }
            else
            {
                node_ = owner_->template rb_move_<false>(node_);
                if(owner_->rb_is_nil_(node_))
                {
                    ri_ = 0;
                    vi_ = 0;
                    return;
                }
            }
            sparse_range_set *set = owner_->template deref_<sparse_range_set>(node_);
            ri_ = set->end - 1;
            vi_ = set->begin[ri_].length - 1;
        }
        void skip_forward_()
        {
            while(!owner_->rb_is_nil_(node_) && at_default_())
            {
                raw_step_forward_();
            }
        }
        void skip_backward_()
        {
            while(!owner_->rb_is_nil_(node_) && at_default_())
            {
                raw_step_backward_();
            }
        }
        void advance_()
        {
            if(IsReverse)
            {
                raw_step_backward_();
                skip_backward_();
            }
            else
            {
                raw_step_forward_();
                skip_forward_();
            }
        }
        void retreat_()
        {
            if(IsReverse)
            {
                raw_step_forward_();
                skip_forward_();
            }
            else
            {
                raw_step_backward_();
                skip_backward_();
            }
        }

    public:
        iterator_impl() : owner_(NULL), node_(handle_t(invalid_handle)), ri_(0), vi_(0)
        {
        }
        iterator_impl(owner_t *owner, handle_t node, uint32_t ri, uint32_t vi) : owner_(owner), node_(node), ri_(ri), vi_(vi)
        {
        }
        template<bool C = IsConst, typename std::enable_if<C, int>::type = 0>
        iterator_impl(iterator_impl<false, IsReverse> const &other) : owner_(other.owner_), node_(other.node_), ri_(other.ri_), vi_(other.vi_)
        {
        }
        reference operator*() const
        {
            return *slot_();
        }
        // index corresponding to the current slot
        uint32_t where() const
        {
            return owner_->template deref_<sparse_range_set>(node_)->begin[ri_].index + vi_;
        }
        iterator_impl &operator++()
        {
            advance_();
            return *this;
        }
        iterator_impl operator++(int)
        {
            iterator_impl tmp = *this;
            advance_();
            return tmp;
        }
        iterator_impl &operator--()
        {
            retreat_();
            return *this;
        }
        iterator_impl operator--(int)
        {
            iterator_impl tmp = *this;
            retreat_();
            return tmp;
        }
        bool operator==(iterator_impl const &other) const
        {
            return owner_ == other.owner_ && node_ == other.node_ && ri_ == other.ri_ && vi_ == other.vi_;
        }
        bool operator!=(iterator_impl const &other) const
        {
            return !(*this == other);
        }
    };
    typedef iterator_impl<false, false> iterator;
    typedef iterator_impl<true, false> const_iterator;
    typedef iterator_impl<false, true> reverse_iterator;
    typedef iterator_impl<true, true> const_reverse_iterator;

    ZZZ_LIB_NODISCARD iterator begin()
    {
        iterator it(this, rb_get_most_left_(), 0, 0);
        it.skip_forward_();
        return it;
    }
    ZZZ_LIB_NODISCARD iterator end()
    {
        return iterator(this, rb_nil_(), 0, 0);
    }
    ZZZ_LIB_NODISCARD const_iterator begin() const
    {
        const_iterator it(this, rb_get_most_left_(), 0, 0);
        it.skip_forward_();
        return it;
    }
    ZZZ_LIB_NODISCARD const_iterator end() const
    {
        return const_iterator(this, rb_nil_(), 0, 0);
    }
    ZZZ_LIB_NODISCARD const_iterator cbegin() const
    {
        return begin();
    }
    ZZZ_LIB_NODISCARD const_iterator cend() const
    {
        return end();
    }
    ZZZ_LIB_NODISCARD reverse_iterator rbegin()
    {
        handle_t node = rb_get_most_right_();
        if(rb_is_nil_(node))
        {
            return reverse_iterator(this, node, 0, 0);
        }
        sparse_range_set *set = deref_<sparse_range_set>(node);
        reverse_iterator it(this, node, set->end - 1, set->begin[set->end - 1].length - 1);
        it.skip_backward_();
        return it;
    }
    ZZZ_LIB_NODISCARD reverse_iterator rend()
    {
        return reverse_iterator(this, rb_nil_(), 0, 0);
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator rbegin() const
    {
        handle_t node = rb_get_most_right_();
        if(rb_is_nil_(node))
        {
            return const_reverse_iterator(this, node, 0, 0);
        }
        sparse_range_set *set = deref_<sparse_range_set>(node);
        const_reverse_iterator it(this, node, set->end - 1, set->begin[set->end - 1].length - 1);
        it.skip_backward_();
        return it;
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator rend() const
    {
        return const_reverse_iterator(this, rb_nil_(), 0, 0);
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator crbegin() const
    {
        return rbegin();
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator crend() const
    {
        return rend();
    }
    // whether empty (no non-default elements)
    ZZZ_LIB_NODISCARD bool empty() const
    {
        return begin() == end();
    }

    // memory adapter
    config_t const &allocator() const
    {
        return index_tree_.allocator();
    }
    // memory adapter
    config_t &allocator()
    {
        return index_tree_.allocator();
    }
    // snapshot (used together with allocator; save allocator state yourself; formerly dump, memory-image-level serialization)
    ZZZ_LIB_NODISCARD snapshot_data snapshot() const
    {
        snapshot_data d;
        d.parent_handle = index_tree_.root()->parent_handle;
        d.left_handle = index_tree_.root()->left_handle;
        d.right_handle = index_tree_.root()->right_handle;
        d.list_handle = block_handle_;
        d.free_handle = free_.handle;
        d.free_length = index_tree_.root()->length;
        d.free_offset = free_.offset;
        d.free_cross = free_.cross;
        return d;
    }
    // restore (used together with allocator; restore allocator state after restore; formerly load_dump, memory-image-level deserialization)
    void restore(snapshot_data const &d)
    {
        reset();
        index_tree_.root()->parent_handle = d.parent_handle;
        index_tree_.root()->left_handle = d.left_handle;
        index_tree_.root()->right_handle = d.right_handle;
        block_handle_ = d.list_handle;
        free_.handle = d.free_handle;
        index_tree_.root()->length = d.free_length;
        free_.offset = d.free_offset;
        free_.cross = d.free_cross;
    }
    // span (largest aligned valid index + 1; not the actual element count)
    ZZZ_LIB_NODISCARD uint32_t span() const
    {
        if(rb_is_nil_(rb_get_root_()))
        {
            return 0;
        }
        else
        {
            sparse_range_set *rs = deref_<sparse_range_set>(rb_get_most_right_());
            sparse_range *last = rs->begin + rs->end - 1;
            return last->index + last->length;
        }
    }
    // internal: read element (no real default-slot entity at the storage level; returned by value)
    value_t get_value_(uint32_t index) const
    {
        sparse_range *range = find_index_(index, NULL);
        if(range == NULL)
        {
            return value_t();
        }
        return deref_<memory_block>(range->handle)->data[range->offset].data[index - range->index];
    }
    // internal: write element (writing the default value is equivalent to reset)
    void set_value_(uint32_t index, value_t const &value)
    {
        handle_t find;
        sparse_range *range = find_index_(index, &find);
        if(!(value != value_t()))
        {
            if(range != NULL)
            {
                erase_index_range_(index, 1);
            }
            return;
        }
        if(range != NULL)
        {
            deref_<memory_block>(range->handle)->data[range->offset].data[index - range->index] = value;
        }
        else
        {
            create_new_range_(find, index, &value, 1);
        }
    }
    // batch get elements
    void get_multi(uint32_t index, value_t *out_arr, uint32_t length) const
    {
        assert(length == 0 || out_arr != NULL);
        if(length == 0)
        {
            return;
        }
        while(true)
        {
            uint32_t copy_length;
            sparse_range *range = find_index_(index, NULL);
            if(range != NULL)
            {
                copy_length = std::min<uint32_t>(length, range->index + range->length - index);
                value_t *ptr = deref_<memory_block>(range->handle)->data[range->offset].data + (index - range->index);
                std::copy(ptr, ptr + copy_length, out_arr);
            }
            else
            {
                copy_length = std::min<uint32_t>(length, atomic_length - index % atomic_length);
                std::fill(out_arr, out_arr + copy_length, value_t());
            }
            length -= copy_length;
            if(length == 0)
            {
                return;
            }
            index += copy_length;
            out_arr += copy_length;
        }
    }
    // batch set elements
    void set_multi(uint32_t index, value_t const *in_arr, uint32_t length)
    {
        assert(length == 0 || in_arr != NULL);
        if(length == 0)
        {
            return;
        }
        value_t default_value = value_t();
        uint32_t offset = 0, i = 1;
        struct check
        {
            static bool is_default(self_t *self, uint32_t index, value_t const *in_arr, uint32_t length, uint32_t &i, uint32_t &offset, value_t const &default_value)
            {
                while(i < length && in_arr[i] == default_value)
                {
                    ++i;
                }
                self->erase_index_range_(index + offset, i - offset);
                if(i == length)
                {
                    return true;
                }
                offset = i;
                return false;
            }
            static bool is_value(self_t *self, uint32_t index, value_t const *in_arr, uint32_t length, uint32_t &i, uint32_t &offset, value_t const &default_value)
            {
                while(i < length && in_arr[i] != default_value)
                {
                    ++i;
                }
                self->set_multi_without_check_(index + offset, in_arr + offset, i - offset);
                if(i == length)
                {
                    return true;
                }
                offset = i;
                return false;
            }
        };
        if(in_arr[0] == default_value)
        {
            while(true)
            {
                if(check::is_default(this, index, in_arr, length, i, offset, default_value))
                {
                    break;
                }
                if(check::is_value(this, index, in_arr, length, i, offset, default_value))
                {
                    break;
                }
            }
        }
        else
        {
            while(true)
            {
                if(check::is_value(this, index, in_arr, length, i, offset, default_value))
                {
                    break;
                }
                if(check::is_default(this, index, in_arr, length, i, offset, default_value))
                {
                    break;
                }
            }
        }
    }
    // whether a slot holds a non-default value
    ZZZ_LIB_NODISCARD bool test(uint32_t index) const
    {
        sparse_range *range = find_index_(index, NULL);
        if(range == NULL)
        {
            return false;
        }
        return deref_<memory_block>(range->handle)->data[range->offset].data[index - range->index] != value_t();
    }
    // reset a single slot to default
    void reset(uint32_t index)
    {
        erase_index_range_(index, 1);
    }
    // reset [first, last) to default
    void reset(uint32_t first, uint32_t last)
    {
        if(last > first)
        {
            erase_index_range_(first, last - first);
        }
    }
    // reset all
    void reset()
    {
        this->~sparse_array();
        ::new(this) self_t();
    }
    // subscript access (read-only, by value)
    ZZZ_LIB_NODISCARD value_t operator[](uint32_t index) const
    {
        return get_value_(index);
    }
    // subscript access
    index_proxy operator[](uint32_t index)
    {
        return index_proxy(index, *this);
    }
    // debug output: print all non-default index = value pairs
    template<class ostream_t>
    ostream_t &print(ostream_t &os) const
    {
        os << "sparse_array{";
        bool first = true;
        for(const_iterator it = begin(); it != end(); ++it)
        {
            if(!first)
            {
                os << ", ";
            }
            first = false;
            os << '[' << it.where() << "] = " << *it;
        }
        os << '}';
        return os;
    }
    // debug output: return string representation
    std::string to_string() const
    {
        std::ostringstream oss;
        print(oss);
        return oss.str();
    }

private:
    // find a range in the range-set tree
    sparse_range *find_index_(uint32_t index, handle_t *out_handle) const
    {
        handle_t find = rb_special_bound_(index);
        if(out_handle != NULL)
        {
            *out_handle = find;
        }
        if(find == rb_nil_())
        {
            return NULL;
        }
        sparse_range_set *range_set = deref_<sparse_range_set>(find);
        sparse_range *range = &range_set->begin[range_set->end - 1];
        if(index >= range->index + range->length)
        {
            return NULL;
        }
        sparse_range *const end = range_set->begin + range_set->end;
        range = special_bound_(range_set->begin, end, index);
        if(range == end || index >= range->index + range->length)
        {
            return NULL;
        }
        return range;
    }
    // allocate a node / data block; throws bad_alloc on failure (returned invalid_handle or malloc returning NULL)
    handle_t alloc_node_()
    {
        handle_t handle = index_tree_.alloc();
        if(handle == handle_t(invalid_handle))
        {
            throw std::bad_alloc();
        }
        return handle;
    }
    // allocate a data block
    handle_t alloc_block_(memory_block **out_block)
    {
        handle_t new_handle = alloc_node_();
        memory_block *block = deref_<memory_block>(new_handle);
        block->next_handle = block_handle_;
        block->prev_handle = handle_t(invalid_handle);
        if(block_handle_ != handle_t(invalid_handle))
        {
            deref_<memory_block>(block_handle_)->prev_handle = new_handle;
        }
        block_handle_ = new_handle;
        if(out_block != NULL)
        {
            *out_block = block;
        }
        return block_handle_;
    }
    // free a data block
    void dealloc_block_(handle_t handle)
    {
        memory_block *block = deref_<memory_block>(handle);
        if(block->next_handle != handle_t(invalid_handle))
        {
            deref_<memory_block>(block->next_handle)->prev_handle = block->prev_handle;
        }
        if(block->prev_handle != handle_t(invalid_handle))
        {
            deref_<memory_block>(block->prev_handle)->next_handle = block->next_handle;
        }
        if(handle == block_handle_)
        {
            block_handle_ = block->next_handle;
        }
        index_tree_.dealloc(handle);
    }
    // allocate a chunk of memory (prefer exact-size match; otherwise split a larger free chunk; otherwise allocate a new block)
    void alloc_(uint32_t cross, handle_t &out_handle, uint16_t &out_offset)
    {
        assert(cross >= 1);
        assert(cross <= block_length);
        memory_blcok_free *result = NULL, *parent = &free_;
        for(memory_blcok_free *it = &free_; it->handle != handle_t(invalid_handle);)
        {
            memory_block_data *next_data = deref_<memory_block>(it->handle)->data + it->offset;
            if(it->cross == cross)
            {
                result = &next_data->free;
                parent = it;
                break;
            }
            if(it->cross > cross && (result == NULL || it->cross < parent->cross))
            {
                result = &next_data->free;
                parent = it;
            }
            it = &next_data->free;
        }
        if(result == NULL)
        {
            memory_block *block;
            handle_t new_handle = alloc_block_(&block);
            out_handle = new_handle;
            out_offset = 0;
            if(cross < block_length)
            {
                block->data[cross].free = free_;
                free_.handle = new_handle;
                free_.offset = cross;
                free_.cross = block_length - cross;
                index_tree_.root()->length += free_.cross;
            }
        }
        else
        {
            out_handle = parent->handle;
            out_offset = parent->offset;
            if(parent->cross == cross)
            {
                *parent = *result;
            }
            else
            {
                parent->offset += cross;
                parent->cross -= cross;
                memory_block_data *next_data = deref_<memory_block>(parent->handle)->data + parent->offset;
                next_data->free = *result;
            }
            index_tree_.root()->length -= cross;
        }
    }
    // free a chunk of memory (prefer coalescing with adjacent free chunks)
    void dealloc_(handle_t handle, uint16_t offset, uint32_t cross)
    {
        assert(cross >= 1);
        assert(cross <= block_length);
        if(cross == block_length)
        {
            dealloc_block_(handle);
            return;
        }
        for(memory_blcok_free *it = &free_; it->handle != handle_t(invalid_handle);)
        {
            memory_block_data *next_data = deref_<memory_block>(it->handle)->data + it->offset;
            if(it->handle == handle)
            {
                if(it->offset + it->cross == offset)
                {
                    index_tree_.root()->length -= it->cross;
                    offset = it->offset;
                    cross += it->cross;
                    *it = deref_<memory_block>(handle)->data[it->offset].free;
                    dealloc_(handle, offset, cross);
                    return;
                }
                if(it->offset == offset + cross)
                {
                    index_tree_.root()->length -= it->cross;
                    cross += it->cross;
                    *it = deref_<memory_block>(handle)->data[it->offset].free;
                    dealloc_(handle, offset, cross);
                    return;
                }
            }
            it = &next_data->free;
        }
        index_tree_.root()->length += cross;
        deref_<memory_block>(handle)->data[offset].free = free_;
        free_.handle = handle;
        free_.offset = offset;
        free_.cross = cross;
    }
    // actual batch set (set_multi above filters out default-valued elements)
    void set_multi_without_check_(uint32_t index, value_t const *in_arr, uint32_t length)
    {
        handle_t find;
        while(true)
        {
            uint32_t copy_length;
            sparse_range *range = find_index_(index, &find);
            if(range != NULL)
            {
                copy_length = std::min<uint32_t>(length, range->index + range->length - index);
                value_t *ptr = deref_<memory_block>(range->handle)->data[range->offset].data + (index - range->index);
                std::copy(in_arr, in_arr + copy_length, ptr);
            }
            else
            {
                copy_length = std::min<uint32_t>(length, atomic_length - index % atomic_length);
                create_new_range_(find, index, in_arr, copy_length);
            }
            length -= copy_length;
            if(length == 0)
            {
                return;
            }
            index += copy_length;
            in_arr += copy_length;
        }
    }
    // erase a span of data (restore to default)
    void erase_index_range_(uint32_t index, uint32_t length)
    {
        handle_t find;
        while(true)
        {
            uint32_t erase_length;
            sparse_range *range = find_index_(index, &find);
            if(range != NULL)
            {
                erase_length = std::min<uint32_t>(length, range->index + range->length - index);
                if(erase_length == range->length && index == range->index)
                {
                    remove_range_(find, range);
                }
                else
                {
                    value_t *ptr = deref_<memory_block>(range->handle)->data[range->offset].data + (index - range->index);
                    std::fill(ptr, ptr + erase_length, value_t());
                }
            }
            else
            {
                erase_length = std::min<uint32_t>(length, atomic_length - index % atomic_length);
            }
            length -= erase_length;
            if(length == 0)
            {
                return;
            }
            index += erase_length;
        }
    }
    template<class iterator_t> static iterator_t special_bound_(iterator_t bound_begin, iterator_t bound_end, uint32_t index)
    {
        uint32_t count = std::distance(bound_begin, bound_end);
        iterator_t where = bound_end;
        while(0 < count)
        {
            uint32_t half_count = count / 2;
            iterator_t mid = std::next(bound_begin, half_count);

            if(!sparse_range_op()(index, *mid))
            {
                where = mid;
                bound_begin = ++mid;
                count -= half_count + 1;
            }
            else
            {
                count = half_count;
            }
        }
        return where;
    }

    // destroy `constructed` elements and free a chunk (used for failure rollback)
    void destroy_and_free_chunk_(handle_t handle, uint16_t offset, uint32_t cross, uint32_t constructed)
    {
        if(!std::is_trivially_destructible<value_t>::value && constructed > 0)
        {
            value_t *data = deref_<memory_block>(handle)->data[offset].data;
            for(uint32_t i = 0; i < constructed; ++i)
            {
                data[i].~value_t();
            }
        }
        dealloc_(handle, offset, cross);
    }
    // RAII guard for a freshly allocated chunk: destroys constructed elements and frees the chunk if not committed
    struct chunk_guard
    {
        self_t *self;
        handle_t handle;
        uint16_t offset;
        uint32_t cross;
        uint32_t constructed;
        bool active;
        chunk_guard(self_t *s, handle_t h, uint16_t o, uint32_t c)
            : self(s), handle(h), offset(o), cross(c), constructed(0), active(true)
        {
        }
        void commit()
        {
            active = false;
        }
        ~chunk_guard()
        {
            if(active)
            {
                self->destroy_and_free_chunk_(handle, offset, cross, constructed);
            }
        }
        chunk_guard(chunk_guard const &) = delete;
        chunk_guard &operator=(chunk_guard const &) = delete;
    };
    // create a new range
    void create_new_range_(handle_t where, uint32_t index, value_t const *arr, uint32_t length)
    {
        assert(index % atomic_length + length <= atomic_length);
        if(merge_range_(index, arr, length))
        {
            return;
        }
        sparse_range range_insert = {
            index - index % atomic_length, atomic_length, 0, handle_t(invalid_handle)
        };
        alloc_(1, range_insert.handle, range_insert.offset);
        // once the chunk is allocated, take ownership with a guard so any later throw frees the chunk (avoiding orphan blocks)
        chunk_guard guard(this, range_insert.handle, range_insert.offset, 1);
        memory_block_data *data = deref_<memory_block>(range_insert.handle)->data + range_insert.offset;
        init_range_data_(data->data + (index - range_insert.index), data->data, data->data + atomic_length, arr, length);
        guard.constructed = atomic_length;
        insert_range_(where, &range_insert);
        guard.commit();
    }
    // initialize range data (strong exception safety: roll back constructed elements if a throw occurs midway)
    void init_range_data_(value_t *ptr, value_t *begin, value_t *end, value_t const *arr, uint32_t length)
    {
        value_t *it = begin;
        try
        {
            while(it != end)
            {
                if(it == ptr)
                {
                    it = std::uninitialized_copy_n(arr, length, it);
                    ptr = NULL;
                }
                else
                {
                    ::new(it) value_t();
                    ++it;
                }
            }
        }
        catch(...)
        {
            for(value_t *rollback = begin; rollback != it; ++rollback)
            {
                rollback->~value_t();
            }
            throw;
        }
    }
    // relocate `count` elements from src to dst (preferring move_if_noexcept); on throw, roll back constructed dst without touching src
    void relocate_values_(value_t *src, value_t *dst, uint32_t count)
    {
        uint32_t i = 0;
        try
        {
            for(; i < count; ++i)
            {
                ::new(dst + i) value_t(std::move_if_noexcept(src[i]));
            }
        }
        catch(...)
        {
            for(uint32_t j = 0; j < i; ++j)
            {
                dst[j].~value_t();
            }
            throw;
        }
    }
    // destroy `count` elements (skipped for trivially destructible types)
    void destroy_values_(value_t *p, uint32_t count)
    {
        if(!std::is_trivially_destructible<value_t>::value)
        {
            for(uint32_t i = 0; i < count; ++i)
            {
                p[i].~value_t();
            }
        }
    }
    // resize range head (first relocate old data into a new chunk and commit, then destroy old data and free the old chunk)
    void adjust_range_head_(handle_t, sparse_range *range, int32_t change)
    {
        assert(change != 0);
        assert(range->length / atomic_length + change >= 1);
        assert(range->length / atomic_length + change <= block_length);
        sparse_range adjust = *range;
        uint32_t new_cross = range->length / atomic_length + change;
        alloc_(new_cross, adjust.handle, adjust.offset);
        value_t *old_data = deref_<memory_block>(range->handle)->data[range->offset].data;
        if(change > 0)
        {
            value_t *to = deref_<memory_block>(adjust.handle)->data[adjust.offset + change].data;
            chunk_guard guard(this, adjust.handle, adjust.offset, new_cross);
            relocate_values_(old_data, to, range->length);
            guard.commit();
        }
        else
        {
            uint32_t cut_length = uint32_t(-change) * atomic_length;
            value_t *to = deref_<memory_block>(adjust.handle)->data[adjust.offset].data;
            chunk_guard guard(this, adjust.handle, adjust.offset, new_cross);
            relocate_values_(old_data + cut_length, to, range->length - cut_length);
            guard.commit();
        }
        // commit succeeded: destroy all old elements (relocated ones are in move-from state; trimmed ones were never relocated) and free the old chunk
        destroy_values_(old_data, range->length);
        dealloc_(range->handle, range->offset, range->length / atomic_length);
        adjust.index -= change * atomic_length;
        adjust.length += change * atomic_length;
        *range = adjust;
    }
    // resize range tail
    void adjust_range_tail_(handle_t, sparse_range *range, int32_t change)
    {
        assert(change != 0);
        assert(range->length / atomic_length + change >= 1);
        assert(range->length / atomic_length + change <= block_length);
        sparse_range adjust = *range;
        uint32_t new_cross = range->length / atomic_length + change;
        alloc_(new_cross, adjust.handle, adjust.offset);
        value_t *old_data = deref_<memory_block>(range->handle)->data[range->offset].data;
        if(change > 0)
        {
            value_t *to = deref_<memory_block>(adjust.handle)->data[adjust.offset].data;
            chunk_guard guard(this, adjust.handle, adjust.offset, new_cross);
            relocate_values_(old_data, to, range->length);
            guard.commit();
        }
        else
        {
            uint32_t cut_length = uint32_t(-change) * atomic_length;
            value_t *to = deref_<memory_block>(adjust.handle)->data[adjust.offset + change].data;
            chunk_guard guard(this, adjust.handle, adjust.offset, new_cross);
            relocate_values_(old_data, to, range->length - cut_length);
            guard.commit();
        }
        // commit succeeded: destroy all old elements (relocated ones are in move-from state; trimmed ones were never relocated) and free the old chunk
        destroy_values_(old_data, range->length);
        dealloc_(range->handle, range->offset, range->length / atomic_length);
        adjust.length += change * atomic_length;
        *range = adjust;
    }
    // try to merge data into adjacent ranges
    bool merge_range_(uint32_t index, value_t const *arr, uint32_t length)
    {
        uint32_t fix_index = index - index % atomic_length;
        handle_t find_before, find_after;
        sparse_range *range_before, *range_after;
        if(fix_index > 0)
        {
            range_before = find_index_(fix_index - atomic_length, &find_before);
        }
        else
        {
            range_before = NULL;
            find_before = handle_t(invalid_handle);
        }
        range_after = find_index_(fix_index + atomic_length, &find_after);
        if(range_before != NULL && range_after != NULL && range_before->length + atomic_length + range_after->length <= int(block_length) * atomic_length)
        {
            sparse_range before = *range_before;
            adjust_range_tail_(find_before, range_before, (atomic_length + range_after->length) / atomic_length);
            value_t *to = deref_<memory_block>(range_before->handle)->data[range_before->offset + before.length / atomic_length].data;
            value_t *from = deref_<memory_block>(range_after->handle)->data[range_after->offset].data;
            init_range_data_(to + (index - fix_index), to, to + atomic_length, arr, length);
            to += atomic_length;
            for(value_t *end = from + range_after->length; from != end; ++from, ++to)
            {
                ::new(to) value_t(std::move_if_noexcept(*from));
            }
            remove_range_(find_after, range_after);
            return true;
        }
        if(range_before != NULL && range_before->length + atomic_length <= int(block_length) * atomic_length)
        {
            sparse_range before = *range_before;
            adjust_range_tail_(find_before, range_before, 1);
            value_t *to = deref_<memory_block>(range_before->handle)->data[range_before->offset + before.length / atomic_length].data;
            init_range_data_(to + (index - fix_index), to, to + atomic_length, arr, length);
            return true;
        }
        if(range_after != NULL && range_after->length + atomic_length <= int(block_length) * atomic_length)
        {
            adjust_range_head_(find_after, range_after, 1);
            value_t *to = deref_<memory_block>(range_after->handle)->data[range_after->offset].data;
            init_range_data_(to + (index - fix_index), to, to + atomic_length, arr, length);
            return true;
        }
        return false;
    }
    // split a range-set, 2/3
    void split_range_set_(handle_t where_left, handle_t where_right)
    {
        sparse_range_set *range_set_left = deref_<sparse_range_set>(where_left);
        sparse_range_set *range_set_right = deref_<sparse_range_set>(where_right);
        handle_t handle = alloc_node_();
        sparse_range_set *range_set = deref_<sparse_range_set>(handle);
        enum
        {
            move_size = range_length / 3
        };
        std::memmove(range_set->begin, range_set_left->begin + range_set_left->end - move_size, move_size * sizeof(sparse_range));
        range_set_left->end -= move_size;
        std::memmove(range_set->begin + move_size, range_set_right->begin, move_size * sizeof(sparse_range));
        range_set->end = move_size * 2;
        range_set_right->end -= move_size;
        std::memmove(range_set_right->begin, range_set_right->begin + move_size, range_set_right->end * sizeof(sparse_range));
        rb_insert_(handle);
    }
    // split a range-set, 1/2
    void split_range_set_(handle_t where)
    {
        sparse_range_set *range_set_left = deref_<sparse_range_set>(where);
        handle_t handle = alloc_node_();
        sparse_range_set *range_set = deref_<sparse_range_set>(handle);
        enum
        {
            move_size = range_length / 2
        };
        std::memmove(range_set->begin, range_set_left->begin + range_set_left->end - move_size, move_size * sizeof(sparse_range));
        range_set_left->end -= move_size;
        range_set->end = move_size;
        rb_insert_(handle);
    }
    // insert a range
    void insert_range_(handle_t where, sparse_range *range)
    {
        if(rb_is_nil_(rb_get_root_()))
        {
            handle_t handle = alloc_node_();
            sparse_range_set *range_set = deref_<sparse_range_set>(handle);
            range_set->end = 1;
            *range_set->begin = *range;
            rb_insert_(handle);
            return;
        }
        if(where == rb_nil_())
        {
            where = rb_get_most_left_();
        }
        sparse_range_set *range_set = deref_<sparse_range_set>(where);
        if(range_set->end == range_length)
        {
            sparse_range_set *range_set_left = nullptr;
            sparse_range_set *range_set_right = nullptr;
            handle_t left = rb_move_<false>(where);
            if(left != rb_nil_())
            {
                range_set_left = deref_<sparse_range_set>(left);
                if(range_set_left->end != range_length)
                {
                    if(range_set->begin->index < range->index)
                    {
                        range_set_left->begin[range_set_left->end++] = *range_set->begin;
                        set_remove_range_(range_set, range_set->begin);
                        set_insert_range_(range_set, range);
                    }
                    else
                    {
                        range_set_left->begin[range_set_left->end++] = *range;
                    }
                    return;
                }
            }
            handle_t right = rb_move_<true>(where);
            if(right != rb_nil_())
            {
                range_set_right = deref_<sparse_range_set>(right);
                if(range_set_right->end != range_length)
                {
                    sparse_range *back_range = range_set->begin + range_set->end - 1;
                    if(range->index < back_range->index)
                    {
                        set_insert_range_(range_set_right, back_range);
                        --range_set->end;
                        set_insert_range_(range_set, range);
                    }
                    else
                    {
                        set_insert_range_(range_set_right, range);
                    }
                    return;
                }
            }
            if(range_set_right != nullptr)
            {
                split_range_set_(where, right);
            }
            else if(range_set_left != nullptr)
            {
                split_range_set_(left, where);
            }
            else
            {
                split_range_set_(where);
            }
            if((where = rb_special_bound_(range->index)) == rb_nil_())
            {
                where = rb_get_most_left_();
            }
            range_set = deref_<sparse_range_set>(where);
        }
        set_insert_range_(range_set, range);
    }
    // unconditional insert of a range
    void set_insert_range_(sparse_range_set *range_set, sparse_range *range)
    {
        sparse_range *const end = range_set->begin + range_set->end;
        sparse_range *where = std::lower_bound(range_set->begin, end, range->index, sparse_range_op());
        if(where != end)
        {
            std::memmove(where + 1, where, (end - where) * sizeof(sparse_range));
        }
        *where = *range;
        ++range_set->end;
    }
    // merge range-sets
    void merge_range_set_(handle_t left, handle_t right)
    {
        sparse_range_set *range_set_left = deref_<sparse_range_set>(left);
        sparse_range_set *range_set_right = deref_<sparse_range_set>(right);
        std::memmove(range_set_left->begin + range_set_left->end, range_set_right->begin, range_set_right->end * sizeof(sparse_range));
        range_set_left->end += range_set_right->end;
        rb_erase_(right);
        index_tree_.dealloc(right);
    }
    // destroy elements within a range (skipped for trivially destructible types)
    void destroy_range_values_(sparse_range const *range)
    {
        if(!std::is_trivially_destructible<value_t>::value)
        {
            value_t *data = deref_<memory_block>(range->handle)->data[range->offset].data;
            for(value_t *it = data, *end = it + range->length; it != end; ++it)
            {
                it->~value_t();
            }
        }
    }
    // remove a range
    void remove_range_(handle_t where, sparse_range *range)
    {
        destroy_range_values_(range);
        dealloc_(range->handle, range->offset, range->length / atomic_length);
        sparse_range_set *range_set = deref_<sparse_range_set>(where);
        if(range_set->end == 1)
        {
            rb_erase_(where);
            index_tree_.dealloc(where);
            return;
        }
        set_remove_range_(range_set, range);
        enum
        {
            move_size = range_length / 3
        };
        if(range_set->end < range_length / 2)
        {
            handle_t left = rb_move_<false>(where);
            sparse_range_set *range_set_left = nullptr;
            if(left != rb_nil_())
            {
                range_set_left = deref_<sparse_range_set>(left);
                if(range_set_left->end > range_length / 2)
                {
                    sparse_range *back_range = range_set_left->begin + range_set_left->end - 1;
                    set_insert_range_(range_set, back_range);
                    --range_set_left->end;
                    return;
                }
            }
            handle_t right = rb_move_<true>(where);
            sparse_range_set *range_set_right = nullptr;
            if(right != rb_nil_())
            {
                range_set_right = deref_<sparse_range_set>(right);
                if(range_set_right->end > range_length / 2)
                {
                    range_set->begin[range_set->end++] = *range_set_right->begin;
                    set_remove_range_(range_set_right, range_set_right->begin);
                    return;
                }
            }
            uint32_t total_length;
            if(range_set_left != nullptr && range_set_right != nullptr && (total_length = range_set_left->end + range_set->end + range_set_right->end) <= range_length * 9 / 5)
            {
                uint32_t each_length = (total_length + 1) / 2;
                if(range_set_left->end < each_length)
                {
                    uint32_t copy_length = std::min(each_length - range_set_left->end, range_set->end);
                    std::memmove(range_set_left->begin + range_set_left->end, range_set->begin, copy_length * sizeof(sparse_range));
                    range_set_left->end += copy_length;
                    if(range_set->end > copy_length)
                    {
                        range_set->end -= copy_length;
                        std::memmove(range_set->begin, range_set->begin + copy_length, range_set->end * sizeof(sparse_range));
                        std::memmove(range_set->begin + range_set->end, range_set_right->begin, range_set_right->end * sizeof(sparse_range));
                        range_set->end += range_set_right->end;
                        rb_erase_(right);
                        index_tree_.dealloc(right);
                    }
                    else
                    {
                        rb_erase_(where);
                        index_tree_.dealloc(where);
                    }
                }
                else
                {
                    merge_range_set_(where, right);
                }
            }
            else if(range_set_left != nullptr && range_set_left->end + range_set->end < range_length)
            {
                merge_range_set_(left, where);
            }
            else if(range_set_right != nullptr && range_set->end + range_set_right->end < range_length)
            {
                merge_range_set_(where, right);
            }
        }
    }
    void set_remove_range_(sparse_range_set *range_set, sparse_range *range)
    {
        uint32_t offset = uint32_t(range - range_set->begin);
        if(offset != range_set->end - 1)
        {
            std::memmove(range, range + 1, (range_set->end - offset - 1) * sizeof(sparse_range));
        }
        --range_set->end;
    }
    // resolve an object from a handle
    template<typename T> T *deref_(handle_t handle) const
    {
        assert(handle != handle_t(invalid_handle));
        return static_cast<T *>(index_tree_.deref(handle));
    }
    // red-black tree node
    sparse_range_set_base const *rb_deref_(handle_t handle) const
    {
        return handle == handle_t(invalid_handle) ? (sparse_range_set_base const *)&index_tree_ : (sparse_range_set_base const *)deref_<sparse_range_set>(handle);
    }
    sparse_range_set_base *rb_deref_(handle_t handle)
    {
        return handle == handle_t(invalid_handle) ? (sparse_range_set_base *)&index_tree_ : (sparse_range_set_base *)deref_<sparse_range_set>(handle);
    }
    handle_t rb_nil_() const
    {
        return handle_t(invalid_handle);
    }
    handle_t rb_get_root_() const
    {
        return index_tree_.root()->parent_handle;
    }
    void rb_set_root_(handle_t root)
    {
        index_tree_.root()->parent_handle = root;
    }
    handle_t rb_get_most_left_() const
    {
        return index_tree_.root()->left_handle;
    }
    void rb_set_most_left_(handle_t left)
    {
        index_tree_.root()->left_handle = left;
    }
    handle_t rb_get_most_right_() const
    {
        return index_tree_.root()->right_handle;
    }
    void rb_set_most_right_(handle_t right)
    {
        index_tree_.root()->right_handle = right;
    }
    uint32_t const &rb_get_key_(handle_t node) const
    {
        return deref_<sparse_range_set>(node)->begin->index;
    }
    bool rb_is_nil_(handle_t node) const
    {
        return rb_deref_(node)->nil;
    }
    void rb_set_nil_(handle_t node, bool nil)
    {
        rb_deref_(node)->nil = !!nil;
    }
    bool rb_is_black_(handle_t node)
    {
        return rb_deref_(node)->black;
    }
    void rb_set_black_(handle_t node, bool black)
    {
        rb_deref_(node)->black = !!black;
    }
    handle_t rb_get_parent_(handle_t node) const
    {
        return rb_deref_(node)->parent_handle;
    }
    void rb_set_parent_(handle_t node, handle_t parent)
    {
        rb_deref_(node)->parent_handle = parent;
    }
    handle_t rb_get_left_(handle_t node) const
    {
        return rb_deref_(node)->left_handle;
    }
    void rb_set_left_(handle_t node, handle_t left)
    {
        rb_deref_(node)->left_handle = left;
    }
    handle_t rb_get_right_(handle_t node) const
    {
        return rb_deref_(node)->right_handle;
    }
    void rb_set_right_(handle_t node, handle_t right)
    {
        rb_deref_(node)->right_handle = right;
    }
    template<bool is_left> void rb_set_child_(handle_t node, handle_t child)
    {
        if(is_left)
        {
            rb_set_left_(node, child);
        }
        else
        {
            rb_set_right_(node, child);
        }
    }
    template<bool is_left> handle_t rb_get_child_(handle_t node) const
    {
        if(is_left)
        {
            return rb_get_left_(node);
        }
        else
        {
            return rb_get_right_(node);
        }
    }
    bool rb_predicate_(uint32_t const &left, uint32_t const &right) const
    {
        return left < right;
    }
    void rb_clear_()
    {
        rb_set_root_(rb_nil_());
        rb_set_most_left_(rb_nil_());
        rb_set_most_right_(rb_nil_());
    }
    handle_t rb_init_node_(handle_t parent, handle_t node)
    {
        rb_set_nil_(node, false);
        rb_set_parent_(node, parent);
        rb_set_left_(node, rb_nil_());
        rb_set_right_(node, rb_nil_());
        return node;
    }
    template<bool is_next> handle_t rb_move_(handle_t node) const
    {
        if(!rb_is_nil_(node))
        {
            if(!rb_is_nil_(rb_get_child_<!is_next>(node)))
            {
                node = rb_get_child_<!is_next>(node);
                while(!rb_is_nil_(rb_get_child_<is_next>(node)))
                {
                    node = rb_get_child_<is_next>(node);
                }
            }
            else
            {
                handle_t parent;
                while(!rb_is_nil_(parent = rb_get_parent_(node)) && node == rb_get_child_<!is_next>(parent))
                {
                    node = parent;
                }
                node = parent;
            }
        }
        else
        {
            return rb_get_child_<is_next>(node);
        }
        return node;
    }
    template<bool is_min> handle_t rb_most_(handle_t node) const
    {
        while(!rb_is_nil_(rb_get_child_<is_min>(node)))
        {
            node = rb_get_child_<is_min>(node);
        }
        return node;
    }
    handle_t rb_special_bound_(uint32_t const &key) const
    {
        handle_t node = rb_get_root_(), where = rb_nil_();
        while(!rb_is_nil_(node))
        {
            if(!rb_predicate_(key, rb_get_key_(node)))
            {
                where = node;
                node = rb_get_right_(node);
            }
            else
            {
                node = rb_get_left_(node);
            }
        }
        return where;
    }
    void rb_equal_range_(uint32_t const &key, handle_t &lower_node, handle_t &upper_node) const
    {
        handle_t node = rb_get_root_();
        handle_t lower = rb_nil_();
        handle_t upper = rb_nil_();
        while(!rb_is_nil_(node))
        {
            if(rb_predicate_(rb_get_key_(node), key))
            {
                node = rb_get_right_(node);
            }
            else
            {
                if(rb_is_nil_(upper) && rb_predicate_(key, rb_get_key_(node)))
                {
                    upper = node;
                }
                lower = node;
                node = rb_get_left_(node);
            }
        }
        node = rb_is_nil_(upper) ? rb_get_root_() : rb_get_left_(upper);
        while(!rb_is_nil_(node))
        {
            if(rb_predicate_(key, rb_get_key_(node)))
            {
                upper = node;
                node = rb_get_left_(node);
            }
            else
            {
                node = rb_get_right_(node);
            }
        }
        lower_node = lower;
        upper_node = upper;
    }
    template<bool is_left> handle_t rb_rotate_(handle_t node)
    {
        handle_t child = rb_get_child_<!is_left>(node), parent = rb_get_parent_(node);
        rb_set_child_<!is_left>(node, rb_get_child_<is_left>(child));
        if(!rb_is_nil_(rb_get_child_<is_left>(child)))
        {
            rb_set_parent_(rb_get_child_<is_left>(child), node);
        }
        rb_set_parent_(child, parent);
        if(node == rb_get_root_())
        {
            rb_set_root_(child);
        }
        else if(node == rb_get_child_<is_left>(parent))
        {
            rb_set_child_<is_left>(parent, child);
        }
        else
        {
            rb_set_child_<!is_left>(parent, child);
        }
        rb_set_child_<is_left>(child, node);
        rb_set_parent_(node, child);
        return child;
    }
    void rb_insert_(handle_t key)
    {
        if(rb_is_nil_(rb_get_root_()))
        {
            rb_set_root_(rb_init_node_(rb_nil_(), key));
            rb_set_black_(rb_get_root_(), true);
            rb_set_most_left_(rb_get_root_());
            rb_set_most_right_(rb_get_root_());
            return;
        }
        rb_set_black_(key, false);
        handle_t node = rb_get_root_(), where = rb_nil_();
        bool is_left = true;
        while(!rb_is_nil_(node))
        {
            where = node;
            if((is_left = rb_predicate_(rb_get_key_(key), rb_get_key_(node))))
            {
                node = rb_get_left_(node);
            }
            else
            {
                node = rb_get_right_(node);
            }
        }
        if(is_left)
        {
            rb_set_left_(where, node = rb_init_node_(where, key));
            if(where == rb_get_most_left_())
            {
                rb_set_most_left_(node);
            }
        }
        else
        {
            rb_set_right_(where, node = rb_init_node_(where, key));
            if(where == rb_get_most_right_())
            {
                rb_set_most_right_(node);
            }
        }
        while(!rb_is_black_(rb_get_parent_(node)))
        {
            if(rb_get_parent_(node) == rb_get_left_(rb_get_parent_(rb_get_parent_(node))))
            {
                where = rb_get_right_(rb_get_parent_(rb_get_parent_(node)));
                if(!rb_is_black_(where))
                {
                    rb_set_black_(rb_get_parent_(node), true);
                    rb_set_black_(where, true);
                    rb_set_black_(rb_get_parent_(rb_get_parent_(node)), false);
                    node = rb_get_parent_(rb_get_parent_(node));
                }
                else
                {
                    if(node == rb_get_right_(rb_get_parent_(node)))
                    {
                        node = rb_get_parent_(node);
                        rb_rotate_<true>(node);
                    }
                    rb_set_black_(rb_get_parent_(node), true);
                    rb_set_black_(rb_get_parent_(rb_get_parent_(node)), false);
                    rb_rotate_<false>(rb_get_parent_(rb_get_parent_(node)));
                }
            }
            else
            {
                where = rb_get_left_(rb_get_parent_(rb_get_parent_(node)));
                if(!rb_is_black_(where))
                {
                    rb_set_black_(rb_get_parent_(node), true);
                    rb_set_black_(where, true);
                    rb_set_black_(rb_get_parent_(rb_get_parent_(node)), false);
                    node = rb_get_parent_(rb_get_parent_(node));
                }
                else
                {
                    if(node == rb_get_left_(rb_get_parent_(node)))
                    {
                        node = rb_get_parent_(node);
                        rb_rotate_<false>(node);
                    }
                    rb_set_black_(rb_get_parent_(node), true);
                    rb_set_black_(rb_get_parent_(rb_get_parent_(node)), false);
                    rb_rotate_<true>(rb_get_parent_(rb_get_parent_(node)));
                }
            }
        }
        rb_set_black_(rb_get_root_(), true);
    }
    void rb_erase_(handle_t node)
    {
        handle_t erase_node = node;
        handle_t fix_node;
        handle_t fix_node_parent;
        if(rb_is_nil_(rb_get_left_(node)))
        {
            fix_node = rb_get_right_(node);
        }
        else if(rb_is_nil_(rb_get_right_(node)))
        {
            fix_node = rb_get_left_(node);
        }
        else
        {
            node = rb_move_<true>(node);
            fix_node = rb_get_right_(node);
        }
        if(node == erase_node)
        {
            fix_node_parent = rb_get_parent_(erase_node);
            if(!rb_is_nil_(fix_node))
            {
                rb_set_parent_(fix_node, fix_node_parent);
            }
            if(rb_get_root_() == erase_node)
            {
                rb_set_root_(fix_node);
            }
            else if(rb_get_left_(fix_node_parent) == erase_node)
            {
                rb_set_left_(fix_node_parent, fix_node);
            }
            else
            {
                rb_set_right_(fix_node_parent, fix_node);
            }
            if(rb_get_most_left_() == erase_node)
            {
                rb_set_most_left_(rb_is_nil_(fix_node) ? fix_node_parent : rb_most_<true>(fix_node));
            }
            if(rb_get_most_right_() == erase_node)
            {
                rb_set_most_right_(rb_is_nil_(fix_node) ? fix_node_parent : rb_most_<false>(fix_node));
            }
        }
        else
        {
            rb_set_parent_(rb_get_left_(erase_node), node);
            rb_set_left_(node, rb_get_left_(erase_node));
            if(node == rb_get_right_(erase_node))
            {
                fix_node_parent = node;
            }
            else
            {
                fix_node_parent = rb_get_parent_(node);
                if(!rb_is_nil_(fix_node))
                {
                    rb_set_parent_(fix_node, fix_node_parent);
                }
                rb_set_left_(fix_node_parent, fix_node);
                rb_set_right_(node, rb_get_right_(erase_node));
                rb_set_parent_(rb_get_right_(erase_node), node);
            }
            if(rb_get_root_() == erase_node)
            {
                rb_set_root_(node);
            }
            else if(rb_get_left_(rb_get_parent_(erase_node)) == erase_node)
            {
                rb_set_left_(rb_get_parent_(erase_node), node);
            }
            else
            {
                rb_set_right_(rb_get_parent_(erase_node), node);
            }
            rb_set_parent_(node, rb_get_parent_(erase_node));
            bool is_black = rb_is_black_(node);
            rb_set_black_(node, rb_is_black_(erase_node));
            rb_set_black_(erase_node, is_black);
        }
        if(rb_is_black_(erase_node))
        {
            for(; fix_node != rb_get_root_() && rb_is_black_(fix_node); fix_node_parent = rb_get_parent_(fix_node))
            {
                if(fix_node == rb_get_left_(fix_node_parent))
                {
                    node = rb_get_right_(fix_node_parent);
                    if(!rb_is_black_(node))
                    {
                        rb_set_black_(node, true);
                        rb_set_black_(fix_node_parent, false);
                        rb_rotate_<true>(fix_node_parent);
                        node = rb_get_right_(fix_node_parent);
                    }
                    if(rb_is_nil_(node))
                    {
                        fix_node = fix_node_parent;
                    }
                    else if(rb_is_black_(rb_get_left_(node)) && rb_is_black_(rb_get_right_(node)))
                    {
                        rb_set_black_(node, false);
                        fix_node = fix_node_parent;
                    }
                    else
                    {
                        if(rb_is_black_(rb_get_right_(node)))
                        {
                            rb_set_black_(rb_get_left_(node), true);
                            rb_set_black_(node, false);
                            rb_rotate_<false>(node);
                            node = rb_get_right_(fix_node_parent);
                        }
                        rb_set_black_(node, rb_is_black_(fix_node_parent));
                        rb_set_black_(fix_node_parent, true);
                        rb_set_black_(rb_get_right_(node), true);
                        rb_rotate_<true>(fix_node_parent);
                        break;
                    }
                }
                else
                {
                    node = rb_get_left_(fix_node_parent);
                    if(!rb_is_black_(node))
                    {
                        rb_set_black_(node, true);
                        rb_set_black_(fix_node_parent, false);
                        rb_rotate_<false>(fix_node_parent);
                        node = rb_get_left_(fix_node_parent);
                    }
                    if(rb_is_nil_(node))
                    {
                        fix_node = fix_node_parent;
                    }
                    else if(rb_is_black_(rb_get_right_(node)) && rb_is_black_(rb_get_left_(node)))
                    {
                        rb_set_black_(node, false);
                        fix_node = fix_node_parent;
                    }
                    else
                    {
                        if(rb_is_black_(rb_get_left_(node)))
                        {
                            rb_set_black_(rb_get_right_(node), true);
                            rb_set_black_(node, false);
                            rb_rotate_<true>(node);
                            node = rb_get_left_(fix_node_parent);
                        }
                        rb_set_black_(node, rb_is_black_(fix_node_parent));
                        rb_set_black_(fix_node_parent, true);
                        rb_set_black_(rb_get_left_(node), true);
                        rb_rotate_<false>(fix_node_parent);
                        break;
                    }
                }
            }
            if(!rb_is_nil_(fix_node))
            {
                rb_set_black_(fix_node, true);
            }
        }
    }

private:
    // index tree
    sparse_range_tree index_tree_;
    // data block list (kept solely for destruction)
    handle_t block_handle_;
    // free data list
    memory_blcok_free free_;
};
