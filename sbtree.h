#pragma once

#ifndef ZZZ_LIB_NODISCARD
#if __cplusplus >= 201703L
#define ZZZ_LIB_NODISCARD [[nodiscard]]
#else
#define ZZZ_LIB_NODISCARD
#endif
#endif

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace size_balanced_tree_detail
{
    // void_t helper (std::void_t is C++17); single type parameter avoids the
    // C++11 alias-template SFINAE caveat (CWG1558).
    template<class T> struct make_void
    {
        typedef void type;
    };
    // node_t::size element type: config_t::size_type when the config provides one,
    // otherwise std::size_t (the historical default, so behaviour is unchanged
    // unless a config deliberately opts into a narrower type to save memory).
    template<class config_t, class = void> struct config_size_type
    {
        typedef std::size_t type;
    };
    template<class config_t> struct config_size_type<config_t, typename make_void<typename config_t::size_type>::type>
    {
        typedef typename config_t::size_type type;
    };
} // namespace size_balanced_tree_detail

template<class config_t>
class size_balanced_tree
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

protected:
    // node_t::size honours the config's (optionally narrower) size_type to save
    // memory at large node counts; defaults to std::size_t when unspecified.
    typedef typename size_balanced_tree_detail::config_size_type<config_t>::type node_size_type;
    struct node_t
    {
        node_t *parent;
        node_t *left;
        node_t *right;
        node_size_type size;
    };
    struct value_node_t : public node_t
    {
        value_node_t(value_type const &v) : value(v)
        {
        }
        template<class... args_t> value_node_t(args_t &&...args) : value(std::forward<args_t>(args)...)
        {
        }
        value_type value;
    };
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<value_node_t> node_allocator_t;
    struct root_node_t : public node_t, public key_compare, public node_allocator_t
    {
        template<class any_key_compare, class any_allocator_t> root_node_t(any_key_compare &&comp, any_allocator_t &&alloc) : key_compare(std::forward<any_key_compare>(comp)), node_allocator_t(std::forward<any_allocator_t>(alloc))
        {
        }
    };
    typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<root_node_t> root_allocator_t;
    struct head_t : public root_allocator_t
    {
        template<class any_allocator_t> head_t(any_allocator_t &&alloc) : root_allocator_t(std::forward<any_allocator_t>(alloc))
        {
        }
        root_node_t *root;
    };

public:
    // one implementation covers all four public iterator flavours; IsConst selects
    // the dereference constness and IsReverse internalises the reversed traversal
    //(no std::reverse_iterator wrapper).
    template<bool is_const, bool is_reverse> class iterator_impl
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        // NOTE: operator+= / operator-= / operator+ / operator- / operator[] are O(log N), not O(1). Declared as random_access for algorithm compatibility.
        typedef typename size_balanced_tree::value_type value_type;
        typedef typename size_balanced_tree::difference_type difference_type;
        typedef typename size_balanced_tree::reference reference;
        typedef typename size_balanced_tree::const_reference const_reference;
        typedef typename size_balanced_tree::pointer pointer;
        typedef typename size_balanced_tree::const_pointer const_pointer;

    private:
        typedef typename std::conditional<is_const, const_reference, reference>::type deref_reference;
        typedef typename std::conditional<is_const, const_pointer, pointer>::type deref_pointer;

    public:
        explicit iterator_impl(node_t *in_node) : node(in_node)
        {
        }
        iterator_impl(iterator_impl const &) = default;
        // non-const -> const, same direction (implicit)
        template<bool enable = is_const> iterator_impl(iterator_impl<false, is_reverse> const &other, typename std::enable_if<enable, int>::type = 0) : node(other.node)
        {
        }
        // forward -> reverse, same constness (explicit, mirrors the legacy ++ on construction)
        template<bool enable = is_reverse> explicit iterator_impl(iterator_impl<is_const, false> const &other, typename std::enable_if<enable, void *>::type = nullptr) : node(other.node)
        {
            ++*this;
        }
        iterator_impl &operator+=(difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, is_reverse ? -diff : diff);
            return *this;
        }
        iterator_impl &operator-=(difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, is_reverse ? diff : -diff);
            return *this;
        }
        iterator_impl operator+(difference_type diff) const
        {
            return iterator_impl(size_balanced_tree::sbt_advance_(node, is_reverse ? -diff : diff));
        }
        iterator_impl operator-(difference_type diff) const
        {
            return iterator_impl(size_balanced_tree::sbt_advance_(node, is_reverse ? diff : -diff));
        }
        difference_type operator-(iterator_impl const &other) const
        {
            return is_reverse ? static_cast<difference_type>(size_balanced_tree::sbt_rank_(other.node)) - static_cast<difference_type>(size_balanced_tree::sbt_rank_(node)) : static_cast<difference_type>(size_balanced_tree::sbt_rank_(node)) - static_cast<difference_type>(size_balanced_tree::sbt_rank_(other.node));
        }
        iterator_impl &operator++()
        {
            node = size_balanced_tree::bst_move_<!is_reverse>(node);
            return *this;
        }
        iterator_impl &operator--()
        {
            node = size_balanced_tree::bst_move_<is_reverse>(node);
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
            return static_cast<value_node_t *>(node)->value;
        }
        deref_pointer operator->() const
        {
            return &static_cast<value_node_t *>(node)->value;
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
            return node == other.node;
        }
        bool operator!=(iterator_impl const &other) const
        {
            return node != other.node;
        }
        // only reverse iterators expose base(), returning the matching forward iterator
        template<bool other_reverse = is_reverse, typename std::enable_if<other_reverse, int>::type = 0> iterator_impl<is_const, false> base() const
        {
            return ++iterator_impl<is_const, false>(node);
        }

    private:
        template<bool, bool> friend class iterator_impl;
        friend class size_balanced_tree;
        node_t *node;
    };
    typedef iterator_impl<false, false> iterator;
    typedef iterator_impl<true, false> const_iterator;
    typedef iterator_impl<false, true> reverse_iterator;
    typedef iterator_impl<true, true> const_reverse_iterator;

protected:
    //full
    template<class in_root_allocator_t, class in_node_allocator_t> size_balanced_tree(key_compare const &comp, in_root_allocator_t &&root_alloc, in_node_allocator_t &&node_alloc) : head_(std::forward<in_root_allocator_t>(root_alloc))
    {
        head_.root = get_root_allocator_().allocate(1);
        std::allocator_traits<root_allocator_t>::construct(get_root_allocator_(), head_.root, comp, std::forward<in_node_allocator_t>(node_alloc));
        set_size_(nil_(), 0);
        set_root_(nil_());
        set_most_left_(nil_());
        set_most_right_(nil_());
    }

public:
    //empty
    size_balanced_tree() : size_balanced_tree(key_compare(), allocator_type())
    {
    }
    //empty
    explicit size_balanced_tree(key_compare const &comp, allocator_type const &alloc = allocator_type()) : size_balanced_tree(comp, alloc, alloc)
    {
    }
    //empty
    explicit size_balanced_tree(allocator_type const &alloc) : size_balanced_tree(key_compare(), alloc, alloc)
    {
    }
    //range
    template<class iterator_t> size_balanced_tree(iterator_t begin, iterator_t end, key_compare const &comp = key_compare(), allocator_type const &alloc = allocator_type()) : size_balanced_tree(comp, alloc, alloc)
    {
        insert(begin, end);
    }
    //range
    template<class iterator_t> size_balanced_tree(iterator_t begin, iterator_t end, allocator_type const &alloc = allocator_type()) : size_balanced_tree(key_compare(), alloc, alloc)
    {
        insert(begin, end);
    }
    //copy
    size_balanced_tree(size_balanced_tree const &other) : size_balanced_tree(other.get_comparator_(), other.get_root_allocator_(), other.get_node_allocator_())
    {
        sbt_copy_<std::false_type>(nullptr, other.get_root_());
    }
    //copy
    size_balanced_tree(size_balanced_tree const &other, allocator_type const &alloc) : size_balanced_tree(other.get_comparator_(), alloc, alloc)
    {
        sbt_copy_<std::false_type>(nullptr, other.get_root_());
    }
    //move
    size_balanced_tree(size_balanced_tree &&other) noexcept : size_balanced_tree(key_compare(), other.get_root_allocator_(), node_allocator_t())
    {
        std::swap(get_root_allocator_(), other.get_root_allocator_());
        std::swap(head_.root, other.head_.root);
    }
    //move
    size_balanced_tree(size_balanced_tree &&other, allocator_type const &alloc) : size_balanced_tree(key_compare(), alloc, alloc)
    {
        sbt_copy_<std::true_type>(nullptr, other.get_root_());
    }
    //initializer list
    size_balanced_tree(std::initializer_list<value_type> il, key_compare const &comp = key_compare(), allocator_type const &alloc = allocator_type()) : size_balanced_tree(il.begin(), il.end(), comp, alloc)
    {
    }
    //initializer list
    size_balanced_tree(std::initializer_list<value_type> il, allocator_type const &alloc) : size_balanced_tree(il.begin(), il.end(), key_compare(), alloc)
    {
    }
    //destructor
    ~size_balanced_tree()
    {
        clear();
        std::allocator_traits<root_allocator_t>::destroy(get_root_allocator_(), head_.root);
        get_root_allocator_().deallocate(head_.root, 1);
    }
    //copy
    size_balanced_tree &operator=(size_balanced_tree const &other)
    {
        if(this == &other)
        {
            return *this;
        }
        if(get_node_allocator_() == other.get_node_allocator_())
        {
            size_balanced_tree tree_memory(get_comparator_(), get_root_allocator_(), get_node_allocator_());
            std::swap(head_.root, tree_memory.head_.root);
            get_comparator_() = other.get_comparator_();
            get_node_allocator_() = other.get_node_allocator_();
            sbt_copy_<std::false_type>(&tree_memory, other.get_root_());
        }
        else
        {
            clear();
            get_comparator_() = other.get_comparator_();
            get_node_allocator_() = other.get_node_allocator_();
            sbt_copy_<std::false_type>(nullptr, other.get_root_());
        }
        return *this;
    }
    //move
    size_balanced_tree &operator=(size_balanced_tree &&other) noexcept
    {
        if(this == &other)
        {
            return *this;
        }
        std::swap(head_, other.head_);
        return *this;
    }
    //initializer list
    size_balanced_tree &operator=(std::initializer_list<value_type> il)
    {
        size_balanced_tree tree_memory(get_comparator_(), get_root_allocator_(), get_node_allocator_());
        std::swap(head_.root, tree_memory.head_.root);
        typename std::initializer_list<value_type>::iterator it = il.begin();
        while(!tree_memory.empty())
        {
            if(it == il.end())
            {
                return *this;
            }
            value_node_t *node = static_cast<value_node_t *>(tree_memory.get_root_());
            tree_memory.sbt_erase_<true>(node);
            std::allocator_traits<node_allocator_t>::destroy(get_node_allocator_(), node);
            std::allocator_traits<node_allocator_t>::construct(get_node_allocator_(), node, *it++);
            sbt_insert_hint_(nil_(), node);
        }
        insert(it, il.end());
        return *this;
    }

    allocator_type get_allocator() const
    {
        return *head_.root;
    }

    void swap(size_balanced_tree &other) noexcept
    {
        std::swap(head_, other.head_);
    }

    typedef std::pair<iterator, iterator> pair_ii_t;
    typedef std::pair<const_iterator, const_iterator> pair_cici_t;

    //single element
    iterator insert(value_type const &value)
    {
        check_max_size_();
        return iterator(sbt_insert_check_(sbt_create_node_(value)));
    }
    //single element
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, iterator>::type insert(in_value_t &&value)
    {
        check_max_size_();
        return iterator(sbt_insert_check_(sbt_create_node_(std::forward<in_value_t>(value))));
    }
    //with hint
    iterator insert(const_iterator hint, value_type const &value)
    {
        check_max_size_();
        return iterator(sbt_insert_hint_check_(hint.node, sbt_create_node_(value)));
    }
    //with hint
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, iterator>::type insert(const_iterator hint, in_value_t &&value)
    {
        check_max_size_();
        return iterator(sbt_insert_hint_check_(hint.node, sbt_create_node_(std::forward<in_value_t>(value))));
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
    template<class... args_t> iterator emplace(args_t &&...args)
    {
        check_max_size_();
        return iterator(sbt_insert_check_(sbt_create_node_(std::forward<args_t>(args)...)));
    }
    //with hint
    template<class... args_t> iterator emplace_hint(const_iterator hint, args_t &&...args)
    {
        check_max_size_();
        return iterator(sbt_insert_hint_check_(hint.node, sbt_create_node_(std::forward<args_t>(args)...)));
    }
    // unique map only, mirrors std::map::operator[]
    template<class k_t = key_type, typename std::enable_if<config_t::unique_type::value && !std::is_same<value_type, k_t const>::value, int>::type = 0> mapped_type &operator[](key_type const &key)
    {
        // single descent: locate the key or the slot it would occupy, then reuse
        // that slot for the insert instead of searching from the root a second time.
        node_t *parent = nil_();
        bool is_left = true;
        node_t *where = sbt_find_or_insert_pos_(key, parent, is_left);
        if(is_nil_(where))
        {
            check_max_size_();
            where = sbt_insert_at_pos_(parent, is_left, sbt_create_node_(value_type(key, mapped_type())));
        }
        return static_cast<value_node_t *>(where)->value.second;
    }

#if __cplusplus >= 201703L
    template<class... args_t, class self_t = config_t, class = typename std::enable_if<self_t::unique_type::value && !std::is_same<typename self_t::value_type, typename self_t::key_type const>::value, void>::type> std::pair<iterator, bool> try_emplace(key_type const &key, args_t &&...args)
    {
        iterator it = find(key);
        if(it != end())
        {
            return std::pair<iterator, bool>(it, false);
        }
        return std::pair<iterator, bool>(emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(std::forward<args_t>(args)...)), true);
    }
    template<class... args_t, class self_t = config_t, class = typename std::enable_if<self_t::unique_type::value && !std::is_same<typename self_t::value_type, typename self_t::key_type const>::value, void>::type> std::pair<iterator, bool> try_emplace(key_type &&key, args_t &&...args)
    {
        iterator it = find(key);
        if(it != end())
        {
            return std::pair<iterator, bool>(it, false);
        }
        return std::pair<iterator, bool>(emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple(std::forward<args_t>(args)...)), true);
    }
    template<class mapped_arg_t, class self_t = config_t, class = typename std::enable_if<self_t::unique_type::value && !std::is_same<typename self_t::value_type, typename self_t::key_type const>::value, void>::type> std::pair<iterator, bool> insert_or_assign(key_type const &key, mapped_arg_t &&obj)
    {
        iterator it = find(key);
        if(it != end())
        {
            it->second = std::forward<mapped_arg_t>(obj);
            return std::pair<iterator, bool>(it, false);
        }
        return std::pair<iterator, bool>(emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(std::forward<mapped_arg_t>(obj))), true);
    }
    template<class mapped_arg_t, class self_t = config_t, class = typename std::enable_if<self_t::unique_type::value && !std::is_same<typename self_t::value_type, typename self_t::key_type const>::value, void>::type> std::pair<iterator, bool> insert_or_assign(key_type &&key, mapped_arg_t &&obj)
    {
        iterator it = find(key);
        if(it != end())
        {
            it->second = std::forward<mapped_arg_t>(obj);
            return std::pair<iterator, bool>(it, false);
        }
        return std::pair<iterator, bool>(emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple(std::forward<mapped_arg_t>(obj))), true);
    }
#endif

#if __cplusplus >= 201703L
    // Owning handle for an extracted node (mirrors std::map::node_type). The
    // node stays alive and unlinked from any tree until the handle is consumed
    // by insert() or destroyed.
    class node_type
    {
        friend class size_balanced_tree;
        node_t *node_;
        node_allocator_t alloc_;
        bool has_alloc_;
        node_type(node_t *node, node_allocator_t const &alloc) : node_(node), alloc_(alloc), has_alloc_(true)
        {
        }
        void release_() noexcept
        {
            node_ = nullptr;
            has_alloc_ = false;
        }
        void destroy_() noexcept
        {
            if(node_ != nullptr)
            {
                value_node_t *value_node = static_cast<value_node_t *>(node_);
                std::allocator_traits<node_allocator_t>::destroy(alloc_, value_node);
                alloc_.deallocate(value_node, 1);
                node_ = nullptr;
            }
        }

    public:
        node_type() noexcept : node_(nullptr), has_alloc_(false)
        {
        }
        node_type(node_type &&other) noexcept : node_(other.node_), alloc_(other.alloc_), has_alloc_(other.has_alloc_)
        {
            other.release_();
        }
        node_type &operator=(node_type &&other) noexcept
        {
            if(this != &other)
            {
                destroy_();
                node_ = other.node_;
                alloc_ = other.alloc_;
                has_alloc_ = other.has_alloc_;
                other.release_();
            }
            return *this;
        }
        node_type(node_type const &) = delete;
        node_type &operator=(node_type const &) = delete;
        ~node_type()
        {
            destroy_();
        }
        bool empty() const noexcept
        {
            return node_ == nullptr;
        }
        explicit operator bool() const noexcept
        {
            return node_ != nullptr;
        }
        value_type &value() const
        {
            return static_cast<value_node_t *>(node_)->value;
        }
        auto &key() const
        {
            return static_cast<value_node_t *>(node_)->value.first;
        }
        mapped_type &mapped() const
        {
            return static_cast<value_node_t *>(node_)->value.second;
        }
    };

    node_type extract(const_iterator where)
    {
        if(where == cend() || is_nil_(where.node))
        {
            return node_type();
        }
        node_t *node = where.node;
        sbt_erase_<false>(node);
        return node_type(node, get_node_allocator_());
    }
    node_type extract(key_type const &key)
    {
        iterator it = find(key);
        if(it == end())
        {
            return node_type();
        }
        return extract(const_iterator(it.node));
    }
    iterator insert(node_type &&handle)
    {
        if(handle.empty())
        {
            return end();
        }
        node_t *node = handle.node_;
        if(config_t::unique_type::value)
        {
            node_t *where = sbt_insert_unique_(node);
            if(where == node)
            {
                handle.release_();
                return iterator(node);
            }
            return iterator(where);
        }
        node_t *where = sbt_insert_<false>(node);
        handle.release_();
        return iterator(where);
    }
    void merge(size_balanced_tree &other)
    {
        if(this == &other)
        {
            return;
        }
        iterator it = other.begin();
        while(it != other.end())
        {
            node_t *node = it.node;
            iterator next = it;
            ++next;
            bool skip = false;
            if(config_t::unique_type::value)
            {
                node_t *exist = bst_lower_bound_(get_key_(node));
                if(!is_nil_(exist) && !get_comparator_()(get_key_(node), get_key_(exist)))
                {
                    skip = true;
                }
            }
            if(!skip)
            {
                other.sbt_erase_<false>(node);
                if(config_t::unique_type::value)
                {
                    sbt_insert_unique_(node);
                }
                else
                {
                    sbt_insert_<false>(node);
                }
            }
            it = next;
        }
    }
#endif

    ZZZ_LIB_NODISCARD iterator find(key_type const &key)
    {
        node_t *where = bst_lower_bound_(key);
        return (is_nil_(where) || get_comparator_()(key, get_key_(where))) ? iterator(nil_()) : iterator(where);
    }
    ZZZ_LIB_NODISCARD const_iterator find(key_type const &key) const
    {
        node_t *where = bst_lower_bound_(key);
        return (is_nil_(where) || get_comparator_()(key, get_key_(where))) ? const_iterator(nil_()) : const_iterator(where);
    }

#if __cplusplus >= 202002L
    ZZZ_LIB_NODISCARD bool contains(key_type const &key) const
    {
        return find(key) != end();
    }
#endif

    iterator erase(const_iterator where)
    {
        const_iterator pos = std::next(where);
        sbt_erase_<false>(where.node);
        sbt_destroy_node_(where.node);
        return iterator(pos.node);
    }
    size_type erase(key_type const &key)
    {
        size_type erase_count = 0;
        node_t *where = bst_lower_bound_(key);
        while(!is_nil_(where) && !get_comparator_()(key, get_key_(where)))
        {
            node_t *next = bst_move_<true>(where);
            erase(iterator(where));
            where = next;
            ++erase_count;
        }
        return erase_count;
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
            while(erase_begin != erase_end)
            {
                erase(erase_begin++);
            }
            return iterator(erase_begin.node);
        }
    }

    ZZZ_LIB_NODISCARD size_type count(key_type const &key) const
    {
        pair_cici_t range = equal_range(key);
        return std::distance(range.first, range.second);
    }
    ZZZ_LIB_NODISCARD size_type count(key_type const &min, key_type const &max) const
    {
        if(get_comparator_()(max, min))
        {
            return 0;
        }
        return sbt_rank_(bst_upper_bound_(max)) - sbt_rank_(bst_lower_bound_(min));
    }

    ZZZ_LIB_NODISCARD pair_ii_t between(key_type const &min, key_type const &max)
    {
        if(get_comparator_()(max, min))
        {
            return pair_ii_t(end(), end());
        }
        return pair_ii_t(iterator(bst_lower_bound_(min)), iterator(bst_upper_bound_(max)));
    }
    ZZZ_LIB_NODISCARD pair_cici_t between(key_type const &min, key_type const &max) const
    {
        if(get_comparator_()(max, min))
        {
            return pair_cici_t(cend(), cend());
        }
        return pair_cici_t(const_iterator(bst_lower_bound_(min)), const_iterator(bst_upper_bound_(max)));
    }

    //reverse index when index < 0
    ZZZ_LIB_NODISCARD pair_ii_t slice(difference_type slice_begin = 0, difference_type slice_end = 0)
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
    ZZZ_LIB_NODISCARD pair_cici_t slice(difference_type slice_begin = 0, difference_type slice_end = 0) const
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

    ZZZ_LIB_NODISCARD iterator lower_bound(key_type const &key)
    {
        return iterator(bst_lower_bound_(key));
    }
    ZZZ_LIB_NODISCARD const_iterator lower_bound(key_type const &key) const
    {
        return const_iterator(bst_lower_bound_(key));
    }
    ZZZ_LIB_NODISCARD iterator upper_bound(key_type const &key)
    {
        return iterator(bst_upper_bound_(key));
    }
    ZZZ_LIB_NODISCARD const_iterator upper_bound(key_type const &key) const
    {
        return const_iterator(bst_upper_bound_(key));
    }

    ZZZ_LIB_NODISCARD pair_ii_t equal_range(key_type const &key)
    {
        node_t *lower, *upper;
        std::tie(lower, upper) = bst_equal_range_(key);
        return pair_ii_t(iterator(lower), iterator(upper));
    }
    ZZZ_LIB_NODISCARD pair_cici_t equal_range(key_type const &key) const
    {
        node_t *lower, *upper;
        std::tie(lower, upper) = bst_equal_range_(key);
        return pair_cici_t(const_iterator(lower), const_iterator(upper));
    }

    ZZZ_LIB_NODISCARD iterator begin()
    {
        return iterator(get_most_left_());
    }
    ZZZ_LIB_NODISCARD iterator end()
    {
        return iterator(nil_());
    }
    ZZZ_LIB_NODISCARD const_iterator begin() const
    {
        return const_iterator(get_most_left_());
    }
    ZZZ_LIB_NODISCARD const_iterator end() const
    {
        return const_iterator(nil_());
    }
    ZZZ_LIB_NODISCARD const_iterator cbegin() const
    {
        return const_iterator(get_most_left_());
    }
    ZZZ_LIB_NODISCARD const_iterator cend() const
    {
        return const_iterator(nil_());
    }
    ZZZ_LIB_NODISCARD reverse_iterator rbegin()
    {
        return reverse_iterator(get_most_right_());
    }
    ZZZ_LIB_NODISCARD reverse_iterator rend()
    {
        return reverse_iterator(nil_());
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(get_most_right_());
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator rend() const
    {
        return const_reverse_iterator(nil_());
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(get_most_right_());
    }
    ZZZ_LIB_NODISCARD const_reverse_iterator crend() const
    {
        return const_reverse_iterator(nil_());
    }

    ZZZ_LIB_NODISCARD reference front()
    {
        return static_cast<value_node_t *>(get_most_left_())->value;
    }
    ZZZ_LIB_NODISCARD reference back()
    {
        return static_cast<value_node_t *>(get_most_right_())->value;
    }

    ZZZ_LIB_NODISCARD const_reference front() const
    {
        return static_cast<value_node_t *>(get_most_left_())->value;
    }
    ZZZ_LIB_NODISCARD const_reference back() const
    {
        return static_cast<value_node_t *>(get_most_right_())->value;
    }

    ZZZ_LIB_NODISCARD bool empty() const
    {
        return is_nil_(get_root_());
    }
    void clear()
    {
        sbt_clear_(get_root_());
        set_root_(nil_());
        set_most_left_(nil_());
        set_most_right_(nil_());
    }
    ZZZ_LIB_NODISCARD size_type size() const
    {
        return get_size_(get_root_());
    }
    ZZZ_LIB_NODISCARD size_type max_size() const noexcept
    {
        return std::allocator_traits<node_allocator_t>::max_size(get_node_allocator_());
    }

    //if(index >= size) return end
    ZZZ_LIB_NODISCARD iterator at(size_type index)
    {
        return iterator(sbt_at_(index));
    }
    //if(index >= size) return end
    ZZZ_LIB_NODISCARD const_iterator at(size_type index) const
    {
        return const_iterator(sbt_at_(index));
    }

    //rank(begin) == 0, key rank
    ZZZ_LIB_NODISCARD size_type rank(key_type const &key) const
    {
        return bst_lower_rank_(key);
    }
    //rank(begin) == 0, rank of iterator
    ZZZ_LIB_NODISCARD static size_type rank(const_iterator where)
    {
        return sbt_rank_(where.node);
    }

    //rank(begin) == 0, key rank current best
    ZZZ_LIB_NODISCARD size_type lower_rank(key_type const &key) const
    {
        return bst_lower_rank_(key);
    }
    //rank(begin) == 0, key rank when insert
    ZZZ_LIB_NODISCARD size_type upper_rank(key_type const &key) const
    {
        return bst_upper_rank_(key);
    }

protected:
    head_t head_;

protected:
    key_compare &get_comparator_()
    {
        return *head_.root;
    }
    key_compare const &get_comparator_() const
    {
        return *head_.root;
    }

    root_allocator_t &get_root_allocator_()
    {
        return head_;
    }
    root_allocator_t const &get_root_allocator_() const
    {
        return head_;
    }

    node_allocator_t &get_node_allocator_()
    {
        return *head_.root;
    }
    node_allocator_t const &get_node_allocator_() const
    {
        return *head_.root;
    }

    node_t *nil_() const
    {
        return head_.root;
    }

    node_t *get_root_() const
    {
        return get_parent_(nil_());
    }

    void set_root_(node_t *root)
    {
        set_parent_(nil_(), root);
    }

    node_t *get_most_left_() const
    {
        return get_left_(nil_());
    }

    void set_most_left_(node_t *left)
    {
        set_left_(nil_(), left);
    }

    node_t *get_most_right_() const
    {
        return get_right_(nil_());
    }

    void set_most_right_(node_t *right)
    {
        set_right_(nil_(), right);
    }

    static key_type const &get_key_(node_t *node)
    {
        return config_t::get_key(static_cast<value_node_t const *>(node)->value);
    }

    static bool is_nil_(node_t *node)
    {
        return node->size == 0;
    }

    static node_t *get_parent_(node_t *node)
    {
        return node->parent;
    }

    static void set_parent_(node_t *node, node_t *parent)
    {
        node->parent = parent;
    }

    static node_t *get_left_(node_t *node)
    {
        return node->left;
    }

    static void set_left_(node_t *node, node_t *left)
    {
        node->left = left;
    }

    static node_t *get_right_(node_t *node)
    {
        return node->right;
    }

    static void set_right_(node_t *node, node_t *right)
    {
        node->right = right;
    }

    static size_type get_size_(node_t *node)
    {
        return node->size;
    }

    static void set_size_(node_t *node, size_type size)
    {
        node->size = size;
    }

    void sbt_refresh_size_(node_t *node)
    {
        set_size_(node, get_size_(get_left_(node)) + get_size_(get_right_(node)) + 1);
    }

    template<bool is_left> static void set_child_(node_t *node, node_t *child)
    {
        if(is_left)
        {
            set_left_(node, child);
        }
        else
        {
            set_right_(node, child);
        }
    }

    template<bool is_left> static node_t *get_child_(node_t *node)
    {
        if(is_left)
        {
            return get_left_(node);
        }
        else
        {
            return get_right_(node);
        }
    }

    void bst_init_node_(node_t *parent, node_t *node)
    {
        set_parent_(node, parent);
        set_left_(node, nil_());
        set_right_(node, nil_());
        set_size_(node, 1);
    }

    template<bool is_next> static node_t *bst_move_(node_t *node)
    {
        if(!is_nil_(node))
        {
            if(!is_nil_(get_child_<!is_next>(node)))
            {
                node = get_child_<!is_next>(node);
                while(!is_nil_(get_child_<is_next>(node)))
                {
                    node = get_child_<is_next>(node);
                }
            }
            else
            {
                node_t *parent;
                while(!is_nil_(parent = get_parent_(node)) && node == get_child_<!is_next>(parent))
                {
                    node = parent;
                }
                node = parent;
            }
        }
        else
        {
            return get_child_<is_next>(node);
        }
        return node;
    }

    template<bool is_min> static node_t *bst_most_(node_t *node)
    {
        while(!is_nil_(get_child_<is_min>(node)))
        {
            node = get_child_<is_min>(node);
        }
        return node;
    }

    node_t *bst_lower_bound_(key_type const &key) const
    {
        node_t *node = get_root_(), *where = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator_()(get_key_(node), key))
            {
                node = get_right_(node);
            }
            else
            {
                where = node;
                node = get_left_(node);
            }
        }
        return where;
    }

    node_t *bst_upper_bound_(key_type const &key) const
    {
        node_t *node = get_root_(), *where = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator_()(key, get_key_(node)))
            {
                where = node;
                node = get_left_(node);
            }
            else
            {
                node = get_right_(node);
            }
        }
        return where;
    }

    std::pair<node_t *, node_t *> bst_equal_range_(key_type const &key) const
    {
        node_t *node = get_root_();
        node_t *lower = nil_();
        node_t *upper = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator_()(get_key_(node), key))
            {
                node = get_right_(node);
            }
            else
            {
                if(is_nil_(upper) && get_comparator_()(key, get_key_(node)))
                {
                    upper = node;
                }
                lower = node;
                node = get_left_(node);
            }
        }
        node = is_nil_(upper) ? get_root_() : get_left_(upper);
        while(!is_nil_(node))
        {
            if(get_comparator_()(key, get_key_(node)))
            {
                upper = node;
                node = get_left_(node);
            }
            else
            {
                node = get_right_(node);
            }
        }
        return std::make_pair(lower, upper);
    }

    node_t *sbt_at_(size_type index)
    {
        node_t *node = get_root_();
        if(index >= get_size_(node))
        {
            return nil_();
        }
        size_type rank = get_size_(get_left_(node));
        while(index != rank)
        {
            if(index < rank)
            {
                node = get_left_(node);
            }
            else
            {
                index -= rank + 1;
                node = get_right_(node);
            }
            rank = get_size_(get_left_(node));
        }
        return node;
    }

    node_t *sbt_at_(size_type index) const
    {
        node_t *node = get_root_();
        if(index >= get_size_(node))
        {
            return nil_();
        }
        size_type rank = get_size_(get_left_(node));
        while(index != rank)
        {
            if(index < rank)
            {
                node = get_left_(node);
            }
            else
            {
                index -= rank + 1;
                node = get_right_(node);
            }
            rank = get_size_(get_left_(node));
        }
        return node;
    }

    static node_t *sbt_advance_(node_t *node, difference_type step)
    {
        if(is_nil_(node))
        {
            if(step == 0)
            {
                return node;
            }
            else if(step > 0)
            {
                --step;
                node = get_left_(node);
            }
            else
            {
                ++step;
                node = get_right_(node);
            }
            if(is_nil_(node))
            {
                return node;
            }
        }
        size_type u_step;
        while(step != 0)
        {
            if(step > 0)
            {
                u_step = step;
                if(get_size_(get_right_(node)) >= u_step)
                {
                    step -= get_size_(get_left_(get_right_(node))) + 1;
                    node = get_right_(node);
                    continue;
                }
            }
            else
            {
                u_step = size_type(0) - static_cast<size_type>(step);
                if(get_size_(get_left_(node)) >= u_step)
                {
                    step += get_size_(get_right_(get_left_(node))) + 1;
                    node = get_left_(node);
                    continue;
                }
            }
            if(is_nil_(get_parent_(node)))
            {
                return get_parent_(node);
            }
            else
            {
                if(get_right_(get_parent_(node)) == node)
                {
                    step += get_size_(get_left_(node)) + 1;
                    node = get_parent_(node);
                }
                else
                {
                    step -= get_size_(get_right_(node)) + 1;
                    node = get_parent_(node);
                }
            }
        }
        return node;
    }

    static size_type sbt_rank_(node_t *node)
    {
        if(is_nil_(node))
        {
            return get_size_(get_parent_(node));
        }
        size_type rank = get_size_(get_left_(node));
        node_t *parent = get_parent_(node);
        while(!is_nil_(parent))
        {
            if(node == get_right_(parent))
            {
                rank += get_size_(get_left_(parent)) + 1;
            }
            node = parent;
            parent = get_parent_(node);
        }
        return rank;
    }

    size_type bst_lower_rank_(key_type const &key) const
    {
        node_t *node = get_root_();
        size_type rank = 0;
        while(!is_nil_(node))
        {
            if(get_comparator_()(get_key_(node), key))
            {
                rank += get_size_(get_left_(node)) + 1;
                node = get_right_(node);
            }
            else
            {
                node = get_left_(node);
            }
        }
        return rank;
    }

    size_type bst_upper_rank_(key_type const &key) const
    {
        node_t *node = get_root_();
        size_type rank = 0;
        while(!is_nil_(node))
        {
            if(get_comparator_()(key, get_key_(node)))
            {
                node = get_left_(node);
            }
            else
            {
                rank += get_size_(get_left_(node)) + 1;
                node = get_right_(node);
            }
        }
        return rank;
    }

    template<bool is_left> node_t *sbt_rotate_(node_t *node)
    {
        node_t *child = get_child_<!is_left>(node), *parent = get_parent_(node);
        set_child_<!is_left>(node, get_child_<is_left>(child));
        if(!is_nil_(get_child_<is_left>(child)))
        {
            set_parent_(get_child_<is_left>(child), node);
        }
        set_parent_(child, parent);
        if(node == get_root_())
        {
            set_root_(child);
        }
        else if(node == get_child_<is_left>(parent))
        {
            set_child_<is_left>(parent, child);
        }
        else
        {
            set_child_<!is_left>(parent, child);
        }
        set_child_<is_left>(child, node);
        set_parent_(node, child);
        set_size_(child, get_size_(node));
        sbt_refresh_size_(node);
        return child;
    }

    template<bool is_left> node_t *sbt_maintain_(node_t *node)
    {
        if(is_nil_(get_child_<is_left>(node)))
        {
            return node;
        }
        if(get_size_(get_child_<is_left>(get_child_<is_left>(node))) > get_size_(get_child_<!is_left>(node)))
        {
            node = sbt_rotate_<!is_left>(node);
        }
        else
        {
            if(get_size_(get_child_<!is_left>(get_child_<is_left>(node))) > get_size_(get_child_<!is_left>(node)))
            {
                sbt_rotate_<is_left>(get_child_<is_left>(node));
                node = sbt_rotate_<!is_left>(node);
            }
            else
            {
                return node;
            };
        };
        if(!is_nil_(get_child_<true>(node)))
        {
            sbt_maintain_<true>(get_child_<true>(node));
        }
        if(!is_nil_(get_child_<false>(node)))
        {
            sbt_maintain_<false>(get_child_<false>(node));
        }
        node = sbt_maintain_<true>(node);
        node = sbt_maintain_<false>(node);
        return node;
    }

    void check_max_size_()
    {
        if(size() >= max_size() - 1)
        {
            throw std::length_error("sbtree too long");
        }
    }

    template<class... args_t> node_t *sbt_create_node_(args_t &&...args)
    {
        value_node_t *node = get_node_allocator_().allocate(1);
        std::allocator_traits<node_allocator_t>::construct(get_node_allocator_(), node, std::forward<args_t>(args)...);
        return node;
    }

    // single-pass search primitive: descends once from the root looking for key.
    // On a hit, returns the existing node (parent_out/is_left_out are untouched).
    // On a miss, returns nil_() and reports the insertion slot through parent_out
    // (the future parent, nil_() when the tree is empty) and is_left_out (the side
    // under that parent). This lets callers share one descent between the lookup
    // and the subsequent insert / iterator construction instead of searching twice.
    node_t *sbt_find_or_insert_pos_(key_type const &key, node_t *&parent_out, bool &is_left_out) const
    {
        node_t *node = get_root_(), *where = nil_();
        bool is_left = true;
        while(!is_nil_(node))
        {
            where = node;
            if(get_comparator_()(key, get_key_(node)))
            {
                is_left = true;
                node = get_left_(node);
            }
            else if(get_comparator_()(get_key_(node), key))
            {
                is_left = false;
                node = get_right_(node);
            }
            else
            {
                return node;
            }
        }
        parent_out = where;
        is_left_out = is_left;
        return nil_();
    }

    // links node at the slot returned by sbt_find_or_insert_pos_ (parent == nil_()
    // means the tree was empty); commits size bookkeeping / rebalancing exactly as
    // the legacy single-pass insert path did.
    node_t *sbt_insert_at_pos_(node_t *parent, bool is_left, node_t *node)
    {
        if(is_nil_(parent))
        {
            bst_init_node_(nil_(), node);
            set_root_(node);
            set_most_left_(node);
            set_most_right_(node);
            return node;
        }
        sbt_insert_at_<true>(is_left, parent, node);
        return node;
    }

    // unique insert: if an equal key already exists the new node is not linked in
    // and the existing node is returned instead (the caller discards the new node)
    node_t *sbt_insert_unique_(node_t *key)
    {
        node_t *parent = nil_();
        bool is_left = true;
        node_t *where = sbt_find_or_insert_pos_(get_key_(key), parent, is_left);
        if(!is_nil_(where))
        {
            return where;
        }
        return sbt_insert_at_pos_(parent, is_left, key);
    }

    // routes single inserts through the unique check when the config asks for it;
    // the duplicate node created up front is destroyed when insertion is rejected
    node_t *sbt_insert_check_(node_t *node)
    {
        if(config_t::unique_type::value)
        {
            node_t *where = sbt_insert_unique_(node);
            if(where != node)
            {
                sbt_destroy_node_(node);
            }
            return where;
        }
        return sbt_insert_<false>(node);
    }

    node_t *sbt_insert_hint_check_(node_t *hint, node_t *node)
    {
        if(config_t::unique_type::value)
        {
            node_t *where = sbt_insert_hint_unique_(hint, node);
            if(where != node)
            {
                sbt_destroy_node_(node);
            }
            return where;
        }
        return sbt_insert_hint_(hint, node);
    }

    // unique-mode hint insert. Mirrors the multi-mode sbt_insert_hint_ fast paths
    // but with strict (non-equal) comparisons so an O(1) link only happens when the
    // hint provably brackets a brand-new key. Any inconclusive case (bad hint, or a
    // possible duplicate) falls back to the full single-pass unique search, which
    // both de-duplicates and inserts. Returns the existing node on duplicate (the
    // passed-in node stays unlinked for the caller to destroy), else returns node.
    node_t *sbt_insert_hint_unique_(node_t *where, node_t *key)
    {
        node_t *other;
        if(is_nil_(get_root_()))
        {
            bst_init_node_(nil_(), key);
            set_root_(key);
            set_most_left_(key);
            set_most_right_(key);
            return key;
        }
        else if(where == get_most_left_())
        {
            // hint == begin(): valid only when key sorts strictly before the front.
            if(get_comparator_()(get_key_(key), get_key_(where)))
            {
                sbt_insert_at_<true>(true, where, key);
                return key;
            }
        }
        else if(where == nil_())
        {
            // hint == end(): valid only when key sorts strictly after the back.
            if(get_comparator_()(get_key_(get_most_right_()), get_key_(key)))
            {
                sbt_insert_at_<true>(false, get_most_right_(), key);
                return key;
            }
        }
        else if(get_comparator_()(get_key_(other = bst_move_<false>(where)), get_key_(key)) && get_comparator_()(get_key_(key), get_key_(where)))
        {
            // predecessor(where) < key < *where: key is new and slots between them.
            if(is_nil_(get_right_(other)))
            {
                sbt_insert_at_<true>(false, other, key);
            }
            else
            {
                sbt_insert_at_<true>(true, where, key);
            }
            return key;
        }
        return sbt_insert_unique_(key);
    }

    template<bool is_leftish> node_t *sbt_insert_(node_t *key)
    {
        if(is_nil_(get_root_()))
        {
            bst_init_node_(nil_(), key);
            set_root_(key);
            set_most_left_(key);
            set_most_right_(key);
            return key;
        }
        node_t *node = get_root_(), *where = nil_();
        bool is_left = true;
        while(!is_nil_(node))
        {
            where = node;
            if(is_leftish)
            {
                is_left = !get_comparator_()(get_key_(node), get_key_(key));
            }
            else
            {
                is_left = get_comparator_()(get_key_(key), get_key_(node));
            }
            if(is_left)
            {
                node = get_left_(node);
            }
            else
            {
                node = get_right_(node);
            }
        }
        // descend without touching sizes first: the comparator may throw, and
        // nothing is committed until the position is settled; sbt_insert_at_<true>
        // then commits the size bookkeeping up the path once insertion happens.
        sbt_insert_at_<true>(is_left, where, key);
        return key;
    }

    node_t *sbt_insert_hint_(node_t *where, node_t *key)
    {
        bool is_leftish = false;
        node_t *other;
        if(is_nil_(get_root_()))
        {
            bst_init_node_(nil_(), key);
            set_root_(key);
            set_most_left_(key);
            set_most_right_(key);
            return key;
        }
        else if(where == get_most_left_())
        {
            if(!get_comparator_()(get_key_(where), get_key_(key)))
            {
                sbt_insert_at_<true>(true, where, key);
                return key;
            }
            is_leftish = true;
        }
        else if(where == nil_())
        {
            if(!get_comparator_()(get_key_(key), get_key_(get_most_right_())))
            {
                sbt_insert_at_<true>(false, get_most_right_(), key);
                return key;
            }
        }
        else if(!get_comparator_()(get_key_(where), get_key_(key)) && !get_comparator_()(get_key_(key), get_key_(other = bst_move_<false>(where))))
        {
            if(is_nil_(get_right_(other)))
            {
                sbt_insert_at_<true>(false, other, key);
            }
            else
            {
                sbt_insert_at_<true>(true, where, key);
            }
            return key;
        }
        else if(!get_comparator_()(get_key_(key), get_key_(where)) && ((other = bst_move_<true>(where)) == nil_() || !get_comparator_()(get_key_(other), get_key_(key))))
        {
            if(is_nil_(get_right_(where)))
            {
                sbt_insert_at_<true>(false, where, key);
            }
            else
            {
                sbt_insert_at_<true>(true, other, key);
            }
            return key;
        }
        else
        {
            is_leftish = true;
        }
        if(is_leftish)
        {
            sbt_insert_<true>(key);
        }
        else
        {
            sbt_insert_<false>(key);
        }
        return key;
    }

    template<bool is_hint> void sbt_insert_at_(bool is_left, node_t *where, node_t *node)
    {
        if(is_hint)
        {
            node_t *parent = where;
            do
            {
                set_size_(parent, get_size_(parent) + 1);
            } while(!is_nil_(parent = get_parent_(parent)));
        }
        bst_init_node_(where, node);
        if(is_left)
        {
            set_left_(where, node);
            if(where == get_most_left_())
            {
                set_most_left_(node);
            }
        }
        else
        {
            set_right_(where, node);
            if(where == get_most_right_())
            {
                set_most_right_(node);
            }
        }
        sbt_insert_maintain_(where, node);
    }

    void sbt_insert_maintain_(node_t *where, node_t *node)
    {
        while(!is_nil_(where))
        {
            if(node == get_left_(where))
            {
                where = sbt_maintain_<true>(where);
            }
            else
            {
                where = sbt_maintain_<false>(where);
            }
            node = where;
            where = get_parent_(where);
        }
    }

    void sbt_destroy_node_(node_t *node)
    {
        value_node_t *value_node = static_cast<value_node_t *>(node);
        std::allocator_traits<node_allocator_t>::destroy(get_node_allocator_(), value_node);
        get_node_allocator_().deallocate(value_node, 1);
    }

    template<bool is_clear> void sbt_erase_(node_t *node)
    {
        node_t *erase_node = node;
        node_t *fix_node;
        node_t *fix_node_parent;
        if(!is_clear)
        {
            fix_node = node;
            while(!is_nil_((fix_node = get_parent_(fix_node))))
            {
                set_size_(fix_node, get_size_(fix_node) - 1);
            }
        }

        if(is_nil_(get_left_(node)))
        {
            fix_node = get_right_(node);
        }
        else if(is_nil_(get_right_(node)))
        {
            fix_node = get_left_(node);
        }
        else
        {
            if(get_size_(get_left_(node)) > get_size_(get_right_(node)))
            {
                node = sbt_erase_at_<is_clear, true>(node);
                if(!is_clear)
                {
                    sbt_erase_maintain_(node, true);
                }
            }
            else
            {
                node = sbt_erase_at_<is_clear, false>(node);
                if(!is_clear)
                {
                    sbt_erase_maintain_(node, false);
                }
            }
            return;
        }
        fix_node_parent = get_parent_(erase_node);
        if(!is_nil_(fix_node))
        {
            set_parent_(fix_node, fix_node_parent);
        }
        if(get_root_() == erase_node)
        {
            set_root_(fix_node);
        }
        else if(get_left_(fix_node_parent) == erase_node)
        {
            set_left_(fix_node_parent, fix_node);
            if(!is_clear)
            {
                sbt_erase_maintain_(fix_node_parent, true);
            }
        }
        else
        {
            set_right_(fix_node_parent, fix_node);
            if(!is_clear)
            {
                sbt_erase_maintain_(fix_node_parent, false);
            }
        }
        if(get_most_left_() == erase_node)
        {
            set_most_left_(is_nil_(fix_node) ? fix_node_parent : bst_most_<true>(fix_node));
        }
        if(get_most_right_() == erase_node)
        {
            set_most_right_(is_nil_(fix_node) ? fix_node_parent : bst_most_<false>(fix_node));
        }
    }

    template<bool is_clear, bool is_left> node_t *sbt_erase_at_(node_t *node)
    {
        node_t *erase_node = node;
        node_t *fix_node;
        node_t *fix_node_parent;
        node = bst_move_<!is_left>(node);
        fix_node = get_child_<is_left>(node);
        if(!is_clear)
        {
            fix_node_parent = node;
            while((fix_node_parent = get_parent_(fix_node_parent)) != erase_node)
            {
                set_size_(fix_node_parent, get_size_(fix_node_parent) - 1);
            }
        }
        set_parent_(get_child_<!is_left>(erase_node), node);
        set_child_<!is_left>(node, get_child_<!is_left>(erase_node));
        if(node == get_child_<is_left>(erase_node))
        {
            fix_node_parent = node;
        }
        else
        {
            fix_node_parent = get_parent_(node);
            if(!is_nil_(fix_node))
            {
                set_parent_(fix_node, fix_node_parent);
            }
            set_child_<!is_left>(fix_node_parent, fix_node);
            set_child_<is_left>(node, get_child_<is_left>(erase_node));
            set_parent_(get_child_<is_left>(erase_node), node);
        }
        if(get_root_() == erase_node)
        {
            set_root_(node);
        }
        else if(get_child_<!is_left>(get_parent_(erase_node)) == erase_node)
        {
            set_child_<!is_left>(get_parent_(erase_node), node);
        }
        else
        {
            set_child_<is_left>(get_parent_(erase_node), node);
        }
        set_parent_(node, get_parent_(erase_node));
        if(!is_clear)
        {
            sbt_refresh_size_(node);
        }
        return fix_node_parent;
    }

    void sbt_erase_maintain_(node_t *where, bool is_left)
    {
        if(is_left)
        {
            where = sbt_maintain_<false>(where);
        }
        else
        {
            where = sbt_maintain_<true>(where);
        }
        node_t *node = where;
        where = get_parent_(where);
        while(!is_nil_(where))
        {
            if(node == get_left_(where))
            {
                where = sbt_maintain_<false>(where);
            }
            else
            {
                where = sbt_maintain_<true>(where);
            }
            node = where;
            where = get_parent_(where);
        }
    }

    void sbt_clear_(node_t *node)
    {
        if(!is_nil_(node))
        {
            sbt_clear_uncheck_(node);
            sbt_destroy_node_(node);
        }
    }

    void sbt_clear_uncheck_(node_t *node)
    {
        if(!is_nil_(get_left_(node)))
        {
            sbt_clear_uncheck_(get_left_(node));
            sbt_destroy_node_(get_left_(node));
        }
        if(!is_nil_(get_right_(node)))
        {
            sbt_clear_uncheck_(get_right_(node));
            sbt_destroy_node_(get_right_(node));
        }
    }

    node_t *sbt_copy_node_(size_balanced_tree *memory, node_t *other, std::true_type)
    {
        if(memory != nullptr && !memory->empty())
        {
            value_node_t *node = static_cast<value_node_t *>(memory->get_root_());
            memory->sbt_erase_<true>(node);
            std::allocator_traits<node_allocator_t>::destroy(get_node_allocator_(), node);
            std::allocator_traits<node_allocator_t>::construct(get_node_allocator_(), node, std::move_if_noexcept(static_cast<value_node_t *>(other)->value));
            return node;
        }
        else
        {
            return sbt_create_node_(std::move_if_noexcept(static_cast<value_node_t *>(other)->value));
        }
    }

    node_t *sbt_copy_node_(size_balanced_tree *memory, node_t *other, std::false_type)
    {
        if(memory != nullptr && !memory->empty())
        {
            value_node_t *node = static_cast<value_node_t *>(memory->get_root_());
            memory->sbt_erase_<true>(node);
            std::allocator_traits<node_allocator_t>::destroy(get_node_allocator_(), node);
            std::allocator_traits<node_allocator_t>::construct(get_node_allocator_(), node, static_cast<value_node_t *>(other)->value);
            return node;
        }
        else
        {
            return sbt_create_node_(static_cast<value_node_t *>(other)->value);
        }
    }

    template<class is_move> void sbt_copy_(size_balanced_tree *memory, node_t *other)
    {
        if(!is_nil_(other))
        {
            set_root_(sbt_copy_uncheck_<is_move>(memory, nil_(), other));
            set_most_left_(bst_most_<true>(get_root_()));
            set_most_right_(bst_most_<false>(get_root_()));
        }
    }

    template<class is_move> node_t *sbt_copy_uncheck_(size_balanced_tree *memory, node_t *node, node_t *other)
    {
        node_t *new_node = sbt_copy_node_(memory, other, is_move());
        set_parent_(new_node, node);
        set_left_(new_node, nil_());
        set_right_(new_node, nil_());
        set_size_(new_node, get_size_(other));
        try
        {
            if(!is_nil_(get_left_(other)))
            {
                set_left_(new_node, sbt_copy_uncheck_<is_move>(memory, new_node, get_left_(other)));
            }
            if(!is_nil_(get_right_(other)))
            {
                set_right_(new_node, sbt_copy_uncheck_<is_move>(memory, new_node, get_right_(other)));
            }
        }
        catch(...)
        {
            sbt_clear_(new_node);
            throw;
        }
        return new_node;
    }
};

template<class config_t> bool operator==(size_balanced_tree<config_t> const &left, size_balanced_tree<config_t> const &right)
{
    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin());
}
#if __cplusplus >= 202002L
#include <compare>
namespace size_balanced_tree_detail
{
// Apple Clang's libc++ (< LLVM17) does not provide std::lexicographical_compare_three_way.
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
        return std::compare_three_way{}(0, 0);
    }
#else
    template<class It1, class It2> constexpr auto lex_three_way(It1 f1, It1 l1, It2 f2, It2 l2)
    {
        return std::lexicographical_compare_three_way(f1, l1, f2, l2);
    }
#endif
} // namespace size_balanced_tree_detail
template<class config_t> auto operator<=>(size_balanced_tree<config_t> const &left, size_balanced_tree<config_t> const &right)
{
    return size_balanced_tree_detail::lex_three_way(left.begin(), left.end(), right.begin(), right.end());
}
#else
template<class config_t> bool operator!=(size_balanced_tree<config_t> const &left, size_balanced_tree<config_t> const &right)
{
    return !(left == right);
}
template<class config_t> bool operator<(size_balanced_tree<config_t> const &left, size_balanced_tree<config_t> const &right)
{
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}
template<class config_t> bool operator>(size_balanced_tree<config_t> const &left, size_balanced_tree<config_t> const &right)
{
    return right < left;
}
template<class config_t> bool operator<=(size_balanced_tree<config_t> const &left, size_balanced_tree<config_t> const &right)
{
    return !(right < left);
}
template<class config_t> bool operator>=(size_balanced_tree<config_t> const &left, size_balanced_tree<config_t> const &right)
{
    return !(left < right);
}
#endif
