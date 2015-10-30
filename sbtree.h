#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <stdexcept>

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
    struct node_t
    {
        node_t *parent;
        node_t *left;
        node_t *right;
        size_t size;
    };
    struct value_node_t : public node_t
    {
        value_node_t(value_type const &v) : value(v)
        {
        }
        template<class ...args_t> value_node_t(args_t &&...args) : value(args...)
        {
        }
        value_type value;
    };
    struct root_node_t : public node_t, public key_compare, public allocator_type
    {
        root_node_t(key_compare const &comp, allocator_type const &alloc) : key_compare(comp), allocator_type(alloc)
        {
        }
    };
    typedef typename allocator_type::template rebind<value_node_t>::other value_allocator_t;

public:
    class iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename size_balanced_tree::value_type value_type;
        typedef typename size_balanced_tree::difference_type difference_type;
        typedef typename size_balanced_tree::reference reference;
        typedef typename size_balanced_tree::pointer pointer;
    public:
        explicit iterator(node_t *in_node) : node(in_node)
        {
        }
        iterator(iterator const &other) : node(other.node)
        {
        }
        iterator &operator += (difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, diff);
            return *this;
        }
        iterator &operator -= (difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, -diff);
            return *this;
        }
        iterator operator + (difference_type diff) const
        {
            return iterator(size_balanced_tree::sbt_advance_(node, diff));
        }
        iterator operator - (difference_type diff) const
        {
            return iterator(size_balanced_tree::sbt_advance_(node, -diff));
        }
        difference_type operator - (iterator const &other) const
        {
            return static_cast<difference_type>(size_balanced_tree::sbt_rank_(node)) - static_cast<difference_type>(size_balanced_tree::sbt_rank_(other.node));
        }
        iterator &operator++()
        {
            node = size_balanced_tree::bst_move_<true>(node);
            return *this;
        }
        iterator &operator--()
        {
            node = size_balanced_tree::bst_move_<false>(node);
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
            return static_cast<value_node_t *>(node)->value;
        }
        pointer operator->() const
        {
            return &static_cast<value_node_t *>(node)->value;
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
            return node == other.node;
        }
        bool operator != (iterator const &other) const
        {
            return node != other.node;
        }
    private:
        friend class size_balanced_tree;
        node_t *node;
    };
    class const_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename size_balanced_tree::value_type value_type;
        typedef typename size_balanced_tree::difference_type difference_type;
        typedef typename size_balanced_tree::reference reference;
        typedef typename size_balanced_tree::const_reference const_reference;
        typedef typename size_balanced_tree::pointer pointer;
        typedef typename size_balanced_tree::const_pointer const_pointer;
    public:
        explicit const_iterator(node_t const *in_node) : node(in_node)
        {
        }
        const_iterator(iterator const &other) : node(other.node)
        {
        }
        const_iterator(const_iterator const &other) : node(other.node)
        {
        }
        const_iterator &operator += (difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, diff);
            return *this;
        }
        const_iterator &operator -= (difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, -diff);
            return *this;
        }
        const_iterator operator + (difference_type diff) const
        {
            return const_iterator(size_balanced_tree::sbt_advance_(node, diff));
        }
        const_iterator operator - (difference_type diff) const
        {
            return const_iterator(size_balanced_tree::sbt_advance_(node, -diff));
        }
        difference_type operator - (const_iterator const &other) const
        {
            return static_cast<difference_type>(size_balanced_tree::sbt_rank_(node)) - static_cast<difference_type>(size_balanced_tree::sbt_rank_(other.node));
        }
        const_iterator &operator++()
        {
            node = size_balanced_tree::bst_move_<true>(node);
            return *this;
        }
        const_iterator &operator--()
        {
            node = size_balanced_tree::bst_move_<false>(node);
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
            return static_cast<value_node_t const *>(node)->value;
        }
        const_pointer operator->() const
        {
            return &static_cast<value_node_t const *>(node)->value;
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
            return node == other.node;
        }
        bool operator != (const_iterator const &other) const
        {
            return node != other.node;
        }
    private:
        friend class size_balanced_tree;
        node_t const *node;
    };
    class reverse_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename size_balanced_tree::value_type value_type;
        typedef typename size_balanced_tree::difference_type difference_type;
        typedef typename size_balanced_tree::reference reference;
        typedef typename size_balanced_tree::pointer pointer;
    public:
        explicit reverse_iterator(node_t *in_node) : node(in_node)
        {
        }
        explicit reverse_iterator(iterator const &other) : node(other.node)
        {
            ++*this;
        }
        reverse_iterator(reverse_iterator const &other) : node(other.node)
        {
        }
        reverse_iterator &operator += (difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, -diff);
            return *this;
        }
        reverse_iterator &operator -= (difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, diff);
            return *this;
        }
        reverse_iterator operator + (difference_type diff) const
        {
            return reverse_iterator(size_balanced_tree::sbt_advance_(node, -diff));
        }
        reverse_iterator operator - (difference_type diff) const
        {
            return reverse_iterator(size_balanced_tree::sbt_advance_(node, diff));
        }
        difference_type operator - (reverse_iterator const &other) const
        {
            return static_cast<difference_type>(size_balanced_tree::sbt_rank_(other.node)) - static_cast<difference_type>(size_balanced_tree::sbt_rank_(node));
        }
        reverse_iterator &operator++()
        {
            node = size_balanced_tree::bst_move_<false>(node);
            return *this;
        }
        reverse_iterator &operator--()
        {
            node = size_balanced_tree::bst_move_<true>(node);
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
            return static_cast<value_node_t *>(node)->value;
        }
        pointer operator->() const
        {
            return &static_cast<value_node_t *>(node)->value;
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
            return node == other.node;
        }
        bool operator != (reverse_iterator const &other) const
        {
            return node != other.node;
        }
        iterator base() const
        {
            return ++iterator(node);
        }
    private:
        friend class size_balanced_tree;
        node_t *node;
    };
    class const_reverse_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename size_balanced_tree::value_type value_type;
        typedef typename size_balanced_tree::difference_type difference_type;
        typedef typename size_balanced_tree::reference reference;
        typedef typename size_balanced_tree::const_reference const_reference;
        typedef typename size_balanced_tree::pointer pointer;
        typedef typename size_balanced_tree::const_pointer const_pointer;
    public:
        explicit const_reverse_iterator(node_t const *in_node) : node(in_node)
        {
        }
        explicit const_reverse_iterator(const_iterator const &other) : node(other.node)
        {
            ++*this;
        }
        const_reverse_iterator(reverse_iterator const &other) : node(other.node)
        {
        }
        const_reverse_iterator(const_reverse_iterator const &other) : node(other.node)
        {
        }
        const_reverse_iterator &operator += (difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, -diff);
            return *this;
        }
        const_reverse_iterator &operator -= (difference_type diff)
        {
            node = size_balanced_tree::sbt_advance_(node, diff);
            return *this;
        }
        const_reverse_iterator operator + (difference_type diff) const
        {
            return const_reverse_iterator(size_balanced_tree::sbt_advance_(node, -diff));
        }
        const_reverse_iterator operator - (difference_type diff) const
        {
            return const_reverse_iterator(size_balanced_tree::sbt_advance_(node, diff));
        }
        difference_type operator - (const_reverse_iterator const &other) const
        {
            return static_cast<difference_type>(size_balanced_tree::sbt_rank_(other.node)) - static_cast<difference_type>(size_balanced_tree::sbt_rank_(node));
        }
        const_reverse_iterator &operator++()
        {
            node = size_balanced_tree::bst_move_<false>(node);
            return *this;
        }
        const_reverse_iterator &operator--()
        {
            node = size_balanced_tree::bst_move_<true>(node);
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
            return static_cast<value_node_t const *>(node)->value;
        }
        const_pointer operator->() const
        {
            return &static_cast<value_node_t const *>(node)->value;
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
            return node == other.node;
        }
        bool operator != (const_reverse_iterator const &other) const
        {
            return node != other.node;
        }
        const_iterator base() const
        {
            return ++iterator(node);
        }
    private:
        friend class size_balanced_tree;
        node_t const *node;
    };

public:
    //empty
    size_balanced_tree() : size_balanced_tree(key_compare(), allocator_type())
    {
    }
    //empty
    explicit size_balanced_tree(key_compare const &comp, allocator_type const &alloc = allocator_type()) : head_(comp, alloc)
    {
        set_size_(nil_(), 0);
        set_root_(nil_());
        set_most_left_(nil_());
        set_most_right_(nil_());
    }
    //empty
    explicit size_balanced_tree(allocator_type const &alloc) : size_balanced_tree(key_compare(), alloc)
    {
    }
    //range
    template <class InputIterator> size_balanced_tree(InputIterator begin, InputIterator end, key_compare const &comp = key_compare(), allocator_type const &alloc = allocator_type()) : size_balanced_tree(key_compare(), allocator_type())
    {
        insert(begin, end);
    }
    //range
    template <class InputIterator> size_balanced_tree(InputIterator begin, InputIterator end, allocator_type const &alloc = allocator_type()) : size_balanced_tree(begin, end, key_compare(), alloc)
    {
    }
    //copy
    size_balanced_tree(size_balanced_tree const &other) : size_balanced_tree(other.begin(), other.end(), other.get_comparator_(), other.get_allocator())
    {
    }
    //copy
    size_balanced_tree(size_balanced_tree const &other, allocator_type const &alloc) : size_balanced_tree(other.begin(), other.end(), other.get_comparator_(), alloc)
    {
    }
    //move
    size_balanced_tree(size_balanced_tree &&other) : size_balanced_tree(other.get_comparator_(), other.get_allocator())
    {
        *this = std::move(other);
    }
    //move
    size_balanced_tree(size_balanced_tree &&other, allocator_type const &alloc) : size_balanced_tree(other.get_comparator_(), alloc)
    {
        *this = std::move(other);
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
    }
    //copy
    size_balanced_tree &operator = (size_balanced_tree const &other)
    {
        if(this == &other)
        {
            return *this;
        }
        if(get_allocator_() == other.get_allocator_())
        {
            size_balanced_tree tree_memory = std::move(*this);
            clear();
            get_comparator_() = other.get_comparator_();
            get_allocator_() = other.get_allocator_();
            const_iterator it = other.begin();
            while(!tree_memory.empty())
            {
                if(it == other.end())
                {
                    tree_memory.clear();
                    return *this;
                }
                node_t *node = tree_memory.get_root_();
                tree_memory.sbt_erase_<true>(node);
                static_cast<value_node_t *>(node)->~value_node_t();
                ::new(node) value_node_t(*it++);
                sbt_insert_hint_(nil_(), node);
            }
            insert(it, other.end());
        }
        else
        {
            clear();
            get_comparator_() = other.get_comparator_();
            get_allocator_() = other.get_allocator_();
            insert(other.begin(), other.end());
        }
        return *this;
    }
    //move
    size_balanced_tree &operator = (size_balanced_tree &&other)
    {
        if(this == &other)
        {
            return *this;
        }
        if(other.empty())
        {
            clear();
        }
        else
        {
            set_root_(other.get_root_());
            set_most_left_(other.get_most_left_());
            set_most_right_(other.get_most_right_());
            other.set_root_(other.nil_());
            other.set_most_left_(other.nil_());
            other.set_most_right_(other.nil_());
        }
        get_comparator_() = std::move(other.get_comparator_());
        get_allocator_() = std::move(other.get_allocator_());
        return *this;
    }
    //initializer list
    size_balanced_tree &operator = (std::initializer_list<value_type> il)
    {
        size_balanced_tree tree_memory = std::move(*this);
        clear();
        auto it = il.begin();
        while(!tree_memory.empty())
        {
            if(it == il.end())
            {
                tree_memory.clear();
                return *this;
            }
            node_t *node = tree_memory.get_root_();
            tree_memory.sbt_erase_<true>(node);
            static_cast<value_node_t *>(node)->~value_node_t();
            ::new(node) value_node_t(*it++);
            sbt_insert_hint_(nil_(), node);
        }
        insert(it, il.end());
        return *this;
    }

    allocator_type get_allocator() const
    {
        return head_;
    }

    void swap(size_balanced_tree &other)
    {
        std::swap(*this, other);
    }

    typedef std::pair<iterator, iterator> pair_ii_t;
    typedef std::pair<const_iterator, const_iterator> pair_cici_t;

    //single element
    iterator insert(value_type const &value)
    {
        check_max_size_();
        return iterator(sbt_insert_<false>(sbt_create_node_(value)));
    }
    //single element
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, iterator>::type insert(in_value_t &&value)
    {
        check_max_size_();
        return iterator(sbt_insert_<false>(sbt_create_node_(value)));
    }
    //with hint
    iterator insert(const_iterator hint, value_type const &value)
    {
        check_max_size_();
        return iterator(sbt_insert_hint_(const_cast<node_t *>(hint.node), sbt_create_node_(value)));
    }
    //with hint
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, iterator>::type insert(const_iterator hint, in_value_t &&value)
    {
        check_max_size_();
        return iterator(sbt_insert_hint_<false>(const_cast<node_t *>(hint.node), sbt_create_node_(value)));
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

    template<class ...args_t> iterator emplace(args_t &&...args)
    {
        check_max_size_();
        return iterator(sbt_insert_<false>(sbt_create_node_(args...)));
    }
    template<class ...args_t> iterator emplace_hint(const_iterator hint, args_t &&...args)
    {
        check_max_size_();
        return iterator(sbt_insert_hint_(const_cast<node_t *>(hint.node), sbt_create_node_(args...)));
    }

    iterator find(key_type const &key)
    {
        node_t *where = bst_lower_bound_(key);
        return (is_nil_(where) || get_comparator_()(key, get_key_(where))) ? iterator(nil_()) : iterator(where);
    }
    const_iterator find(key_type const &key) const
    {
        node_t const *where = bst_lower_bound_(key);
        return (is_nil_(where) || get_comparator_()(key, get_key_(where))) ? const_iterator(nil_()) : const_iterator(where);
    }

    void erase(const_iterator where)
    {
        node_t *node = const_cast<node_t *>(where.node);
        sbt_erase_<false>(node);
        sbt_destroy_node_(node);
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
    iterator erase(const_iterator begin, const_iterator end)
    {
        if(begin == cbegin() && end == cend())
        {
            clear();
            return size_balanced_tree::begin();
        }
        else
        {
            while(begin != end)
            {
                erase(begin++);
            }
            return iterator(const_cast<node_t *>(begin.node));
        }
    }

    size_type count(key_type const &key) const
    {
        pair_cici_t range = equal_range(key);
        return std::distance(range.first, range.second);
    }
    //计数[min, max)
    size_type count(key_type const &min, key_type const &max) const
    {
        if(get_comparator_()(max, min))
        {
            return 0;
        }
        return sbt_rank_(bst_upper_bound_(max)) - sbt_rank_(bst_lower_bound_(min));
    }

    //获取[min, max)
    pair_ii_t range(key_type const &min, key_type const &max)
    {
        if(get_comparator_()(max, min))
        {
            return pair_ii_t(end(), end());
        }
        return pair_ii_t(iterator(bst_lower_bound_(min)), iterator(bst_upper_bound_(max)));
    }
    pair_cici_t range(key_type const &min, key_type const &max) const
    {
        if(get_comparator_()(max, min))
        {
            return pair_cici_t(cend(), cend());
        }
        return pair_cici_t(const_iterator(bst_lower_bound_(min)), const_iterator(bst_upper_bound_(max)));
    }

    //获取下标begin到end之间(参数小于0反向下标)
    pair_ii_t slice(difference_type begin = 0, difference_type end = std::numeric_limits<difference_type>::max())
    {
        difference_type s_size = size();
        if(begin < 0)
        {
            begin = std::max<difference_type>(s_size + begin, 0);
        }
        if(end < 0)
        {
            end = s_size + end;
        }
        if(begin > end || begin >= s_size)
        {
            return pair_ii_t(size_balanced_tree::end(), size_balanced_tree::end());
        }
        if(end > s_size)
        {
            end = s_size;
        }
        return pair_ii_t(at(begin), at(end));
    }
    pair_cici_t slice(difference_type begin = 0, difference_type end = std::numeric_limits<difference_type>::max()) const
    {
        difference_type s_size = size();
        if(begin < 0)
        {
            begin = std::max<difference_type>(s_size + begin, 0);
        }
        if(end < 0)
        {
            end = s_size + end;
        }
        if(begin > end || begin >= s_size)
        {
            return pair_cici_t(cend(), cend());
        }
        if(end > s_size)
        {
            end = s_size;
        }
        return pair_cici_t(at(begin), at(end));
    }

    iterator lower_bound(key_type const &key)
    {
        return iterator(bst_lower_bound_(key));
    }
    const_iterator lower_bound(key_type const &key) const
    {
        return const_iterator(bst_lower_bound_(key));
    }
    iterator upper_bound(key_type const &key)
    {
        return iterator(bst_upper_bound_(key));
    }
    const_iterator upper_bound(key_type const &key) const
    {
        return const_iterator(bst_upper_bound_(key));
    }

    pair_ii_t equal_range(key_type const &key)
    {
        node_t *lower, *upper;
        bst_equal_range_(key, lower, upper);
        return pair_ii_t(iterator(lower), iterator(upper));
    }
    pair_cici_t equal_range(key_type const &key) const
    {
        node_t const *lower, *upper;
        bst_equal_range_(key, lower, upper);
        return pair_cici_t(const_iterator(lower), const_iterator(upper));
    }

    iterator begin()
    {
        return iterator(get_most_left_());
    }
    iterator end()
    {
        return iterator(nil_());
    }
    const_iterator begin() const
    {
        return const_iterator(get_most_left_());
    }
    const_iterator end() const
    {
        return const_iterator(nil_());
    }
    const_iterator cbegin() const
    {
        return const_iterator(get_most_left_());
    }
    const_iterator cend() const
    {
        return const_iterator(nil_());
    }
    reverse_iterator rbegin()
    {
        return reverse_iterator(get_most_right_());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(nil_());
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(get_most_right_());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(nil_());
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(get_most_right_());
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(nil_());
    }

    reference front()
    {
        return static_cast<value_node_t *>(get_most_left_())->value;
    }
    reference back()
    {
        return static_cast<value_node_t *>(get_most_right_())->value;
    }

    const_reference front() const
    {
        return static_cast<value_node_t const *>(get_most_left_())->value;
    }
    const_reference back() const
    {
        return static_cast<value_node_t const *>(get_most_right_())->value;
    }

    bool empty() const
    {
        return is_nil_(get_root_());
    }
    void clear()
    {
        for(node_t *root = get_root_(); !is_nil_(root); root = get_root_())
        {
            sbt_erase_<true>(root);
            sbt_destroy_node_(root);
        }
    }
    size_type size() const
    {
        return get_size_(get_root_());
    }
    size_type max_size() const
    {
        return value_allocator_t(get_allocator_()).max_size();
    }

    //下标访问[0, size)
    iterator at(size_type index)
    {
        return iterator(sbt_at_(index));
    }
    //下标访问[0, size)
    const_iterator at(size_type index) const
    {
        return const_iterator(sbt_at_(index));
    }

    //计算key的rank(相同取最末,从0开始)
    size_type rank(key_type const &key) const
    {
        return sbt_rank_(bst_upper_bound_(key));
    }
    //计算迭代器rank[0, size),end的rank为size
    static size_type rank(const_iterator where)
    {
        return sbt_rank_(where.node);
    }

protected:
    root_node_t head_;

protected:

    key_compare &get_comparator_()
    {
        return head_;
    }
    key_compare const &get_comparator_() const
    {
        return head_;
    }

    allocator_type &get_allocator_()
    {
        return head_;
    }
    allocator_type const &get_allocator_() const
    {
        return head_;
    }

    node_t *nil_()
    {
        return &head_;
    }
    node_t const *nil_() const
    {
        return &head_;
    }

    node_t *get_root_()
    {
        return get_parent_(&head_);
    }
    node_t const *get_root_() const
    {
        return get_parent_(&head_);
    }

    void set_root_(node_t *root)
    {
        set_parent_(&head_, root);
    }

    node_t *get_most_left_()
    {
        return get_left_(&head_);
    }
    node_t const *get_most_left_() const
    {
        return get_left_(&head_);
    }

    void set_most_left_(node_t *left)
    {
        set_left_(&head_, left);
    }

    node_t *get_most_right_()
    {
        return get_right_(&head_);
    }
    node_t const *get_most_right_() const
    {
        return get_right_(&head_);
    }

    void set_most_right_(node_t *right)
    {
        set_right_(&head_, right);
    }

    static key_type const &get_key_(node_t const *node)
    {
        return config_t::get_key(static_cast<value_node_t const *>(node)->value);
    }

    static bool is_nil_(node_t const *node)
    {
        return node->size == 0;
    }

    static node_t *get_parent_(node_t const *node)
    {
        return node->parent;
    }

    static void set_parent_(node_t *node, node_t *parent)
    {
        node->parent = parent;
    }

    static node_t *get_left_(node_t const *node)
    {
        return node->left;
    }

    static void set_left_(node_t *node, node_t *left)
    {
        node->left = left;
    }

    static node_t *get_right_(node_t const *node)
    {
        return node->right;
    }

    static void set_right_(node_t *node, node_t *right)
    {
        node->right = right;
    }

    static size_type get_size_(node_t const *node)
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

    template<bool is_left> static node_t *get_child_(node_t const *node)
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

    template<bool is_next, typename in_node_t> static in_node_t *bst_move_(in_node_t *node)
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
                in_node_t *parent;
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

    node_t *bst_lower_bound_(key_type const &key)
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

    node_t const *bst_lower_bound_(key_type const &key) const
    {
        node_t const *node = get_root_(), *where = nil_();
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

    node_t *bst_upper_bound_(key_type const &key)
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

    node_t const *bst_upper_bound_(key_type const &key) const
    {
        node_t const *node = get_root_(), *where = nil_();
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

    void bst_equal_range_(key_type const &key, node_t *&lower_node, node_t *&upper_node)
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
        lower_node = lower;
        upper_node = upper;
    }

    void bst_equal_range_(key_type const &key, node_t const *&lower_node, node_t const *&upper_node) const
    {
        node_t const *node = get_root_();
        node_t const *lower = nil_();
        node_t const *upper = nil_();
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
        lower_node = lower;
        upper_node = upper;
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

    node_t const *sbt_at_(size_type index) const
    {
        node_t const *node = get_root_();
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

    template<typename in_node_t> static in_node_t *sbt_advance_(in_node_t *node, difference_type step)
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
                u_step = -step;
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

    static size_type sbt_rank_(node_t const *node)
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

    template<class ...args_t> node_t *sbt_create_node_(args_t &&...args)
    {
        return ::new(value_allocator_t(get_allocator_()).allocate(1)) value_node_t(args...);
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
            set_size_(node, get_size_(node) + 1);
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
        sbt_insert_at_<false>(is_left, where, key);
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
            }
            while(!is_nil_(parent = get_parent_(parent)));
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
        static_cast<value_node_t *>(node)->~value_node_t();
        value_allocator_t(get_allocator_()).deallocate(static_cast<value_node_t *>(node), 1);
    }

    template<bool is_clear> void sbt_erase_(node_t *node)
    {
        node_t *erase_node = node;
        node_t *fix_node;
        node_t *fix_node_parent;
        bool is_left;
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
            is_left = true;
        }
        else if(is_nil_(get_right_(node)))
        {
            fix_node = get_left_(node);
            is_left = false;
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
};
