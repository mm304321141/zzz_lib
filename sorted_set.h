﻿#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
class sorted_set
{
public:
    typedef key_t key_type;
    typedef value_t mapped_type;
    typedef std::pair<key_t const, value_t> value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef comparator_t key_compare;
    typedef allocator_t allocator_type;
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
    typedef typename allocator_t::template rebind<value_node_t>::other value_allocator_t;
    struct root_node_t : public node_t, public value_allocator_t, public comparator_t
    {
    };

public:
    class iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename sorted_set::value_type value_type;
        typedef typename sorted_set::difference_type difference_type;
        typedef typename sorted_set::pointer pointer;
        typedef typename sorted_set::reference reference;
    public:
        explicit iterator(node_t *in_node) : node(in_node)
        {
        }
        iterator(iterator const &other) : node(other.node)
        {
        }
        iterator &operator += (difference_type diff)
        {
            node = sorted_set::sbt_advance_(node, diff);
            return *this;
        }
        iterator &operator -= (difference_type diff)
        {
            node = sorted_set::sbt_advance_(node, -diff);
            return *this;
        }
        iterator operator + (difference_type diff) const
        {
            return iterator(sorted_set::sbt_advance_(node, diff));
        }
        iterator operator - (difference_type diff) const
        {
            return iterator(sorted_set::sbt_advance_(node, -diff));
        }
        difference_type operator - (iterator const &other) const
        {
            return static_cast<difference_type>(sorted_set::sbt_rank_(node)) - static_cast<difference_type>(sorted_set::sbt_rank_(other.node));
        }
        iterator &operator++()
        {
            node = sorted_set::bst_move_<true>(node);
            return *this;
        }
        iterator &operator--()
        {
            node = sorted_set::bst_move_<false>(node);
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
        value_type &operator *() const
        {
            return static_cast<value_node_t *>(node)->value;
        }
        value_type *operator->() const
        {
            return &static_cast<value_node_t *>(node)->value;
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
        friend class sorted_set;
        node_t *node;
    };
    class const_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename sorted_set::value_type value_type;
        typedef typename sorted_set::difference_type difference_type;
        typedef typename sorted_set::pointer pointer;
        typedef typename sorted_set::reference reference;
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
            node = sorted_set::sbt_advance_(node, diff);
            return *this;
        }
        const_iterator &operator -= (difference_type diff)
        {
            node = sorted_set::sbt_advance_(node, -diff);
            return *this;
        }
        const_iterator operator + (difference_type diff) const
        {
            return const_iterator(sorted_set::sbt_advance_(node, diff));
        }
        const_iterator operator - (difference_type diff) const
        {
            return const_iterator(sorted_set::sbt_advance_(node, -diff));
        }
        difference_type operator - (const_iterator const &other) const
        {
            return static_cast<difference_type>(sorted_set::sbt_rank_(node)) - static_cast<difference_type>(sorted_set::sbt_rank_(other.node));
        }
        const_iterator &operator++()
        {
            node = sorted_set::bst_move_<true>(node);
            return *this;
        }
        const_iterator &operator--()
        {
            node = sorted_set::bst_move_<false>(node);
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
        value_type const &operator *() const
        {
            return static_cast<value_node_t const *>(node)->value;
        }
        value_type const *operator->() const
        {
            return &static_cast<value_node_t const *>(node)->value;
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
        friend class sorted_set;
        node_t const *node;
    };
    class reverse_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename sorted_set::value_type value_type;
        typedef typename sorted_set::difference_type difference_type;
        typedef typename sorted_set::pointer pointer;
        typedef typename sorted_set::reference reference;
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
            node = sorted_set::sbt_advance_(node, -diff);
            return *this;
        }
        reverse_iterator &operator -= (difference_type diff)
        {
            node = sorted_set::sbt_advance_(node, diff);
            return *this;
        }
        reverse_iterator operator + (difference_type diff) const
        {
            return reverse_iterator(sorted_set::sbt_advance_(node, -diff));
        }
        reverse_iterator operator - (difference_type diff) const
        {
            return reverse_iterator(sorted_set::sbt_advance_(node, diff));
        }
        difference_type operator - (reverse_iterator const &other) const
        {
            return static_cast<difference_type>(sorted_set::sbt_rank_(other.node)) - static_cast<difference_type>(sorted_set::sbt_rank_(node));
        }
        reverse_iterator &operator++()
        {
            node = sorted_set::bst_move_<false>(node);
            return *this;
        }
        reverse_iterator &operator--()
        {
            node = sorted_set::bst_move_<true>(node);
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
        value_type &operator *() const
        {
            return static_cast<value_node_t *>(node)->value;
        }
        value_type *operator->() const
        {
            return &static_cast<value_node_t *>(node)->value;
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
        friend class sorted_set;
        node_t *node;
    };
    class const_reverse_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename sorted_set::value_type value_type;
        typedef typename sorted_set::difference_type difference_type;
        typedef typename sorted_set::pointer pointer;
        typedef typename sorted_set::reference reference;
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
            node = sorted_set::sbt_advance_(node, -diff);
            return *this;
        }
        const_reverse_iterator &operator -= (difference_type diff)
        {
            node = sorted_set::sbt_advance_(node, diff);
            return *this;
        }
        const_reverse_iterator operator + (difference_type diff) const
        {
            return const_reverse_iterator(sorted_set::sbt_advance_(node, -diff));
        }
        const_reverse_iterator operator - (difference_type diff) const
        {
            return const_reverse_iterator(sorted_set::sbt_advance_(node, diff));
        }
        difference_type operator - (const_reverse_iterator const &other) const
        {
            return static_cast<difference_type>(sorted_set::sbt_rank_(other.node)) - static_cast<difference_type>(sorted_set::sbt_rank_(node));
        }
        const_reverse_iterator &operator++()
        {
            node = sorted_set::bst_move_<false>(node);
            return *this;
        }
        const_reverse_iterator &operator--()
        {
            node = sorted_set::bst_move_<true>(node);
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
        value_type const &operator *() const
        {
            return static_cast<value_node_t const *>(node)->value;
        }
        value_type const *operator->() const
        {
            return &static_cast<value_node_t const *>(node)->value;
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
        friend class sorted_set;
        node_t const *node;
    };

public:
    sorted_set()
    {
        set_size_(nil_(), 0);
        set_root_(nil_());
        set_most_left_(nil_());
        set_most_right_(nil_());
    }
    sorted_set(sorted_set &&other) : sorted_set()
    {
        *this = std::move(other);
    }
    sorted_set(sorted_set const &other) : sorted_set()
    {
        insert(other.begin(), other.end());
    }
    ~sorted_set()
    {
        clear();
    }
    sorted_set &operator = (sorted_set &&other)
    {
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
        return *this;
    }
    sorted_set &operator = (sorted_set const &other)
    {
        sorted_set temp_set = std::move(*this);
        const_iterator it = other.begin();
        while(!temp_set.empty())
        {
            if(it == other.end())
            {
                temp_set.clear();
                return *this;
            }
            node_t *node = temp_set.get_root_();
            temp_set.sbt_erase_(node);
            static_cast<value_node_t *>(node)->~value_node_t();
            ::new(node) value_node_t(*it++);
            sbt_insert_(node);
        }
        insert(it, other.end());
        return *this;
    }

    allocator_type get_allocator() const
    {
        return allocator_type(head_);
    }

    void swap(sorted_set &other)
    {
        std::swap(*this, other);
    }

    typedef std::pair<iterator, iterator> pair_ii_t;
    typedef std::pair<const_iterator, const_iterator> pair_cici_t;

    //允许重复key
    iterator insert(value_type const &node)
    {
        value_node_t *new_node = get_value_allocator().allocate(1);
        ::new(new_node) value_node_t(node);
        sbt_insert_(new_node);
        return iterator(new_node);
    }
    iterator insert(const_iterator hint, value_type const &node)
    {
        value_node_t *new_node = get_value_allocator().allocate(1);
        ::new(new_node) value_node_t(node);
        sbt_insert_(new_node);
        return iterator(new_node);
    }
    iterator insert(value_type &&node)
    {
        value_node_t *new_node = get_value_allocator().allocate(1);
        ::new(new_node) value_node_t(node);
        sbt_insert_(new_node);
        return iterator(new_node);
    }
    template<class iterator_t> void insert(iterator_t begin, iterator_t end)
    {
        for(; begin != end; ++begin)
        {
            insert(*begin);
        }
    }
    void insert(std::initializer_list<value_type> ilist)
    {
        insert(ilist.begin(), ilist.end());
    }
    template<class ...args_t> iterator emplace(args_t &&...args)
    {
        value_node_t *new_node = get_value_allocator().allocate(1);
        ::new(new_node) value_node_t(args...);
        sbt_insert_(new_node);
        return iterator(new_node);
    }

    iterator find(key_t const &key)
    {
        node_t *where = bst_lower_bound_(key);
        return (is_nil_(where) || get_comparator()(key, get_key_(where))) ? iterator(nil_()) : iterator(where);
    }
    const_iterator find(key_t const &key) const
    {
        node_t *where = bst_lower_bound_(key);
        return (is_nil_(where) || get_comparator()(key, get_key_(where))) ? iterator(nil_()) : iterator(where);
    }

    void erase(iterator where)
    {
        sbt_erase_(where.node);
        static_cast<value_node_t *>(where.node)->~value_node_t();
        get_value_allocator().deallocate(static_cast<value_node_t *>(where.node), 1);
    }
    size_type erase(key_t const &key)
    {
        size_type erase_count = 0;
        pair_ii_t range = equal_range(key);
        while(range.first != range.second)
        {
            erase(range.first++);
            ++erase_count;
        }
        return erase_count;
    }

    size_type count(key_t const &key) const
    {
        pair_cici_t range = equal_range(key);
        return std::distance(range.first, range.second);
    }
    //计数[min, max)
    size_type count(key_t const &min, key_t const &max) const
    {
        return sbt_rank_(bst_upper_bound_(max)) - sbt_rank_(bst_lower_bound_(min));
    }

    //获取[min, max)
    pair_ii_t range(key_t const &min, key_t const &max)
    {
        return pair_ii_t(iterator(bst_lower_bound_(min)), iterator(bst_upper_bound_(max)));
    }
    pair_cici_t range(key_t const &min, key_t const &max) const
    {
        return pair_cici_t(const_iterator(bst_lower_bound_(min)), const_iterator(bst_upper_bound_(max)));
    }

    //获取下标begin到end之间(参数小于0反向下标)
    pair_ii_t slice(int begin = 0, int end = std::numeric_limits<int>::max())
    {
        difference_type s_size = size();
        if(begin < 0)
        {
            begin = std::max(s_size + begin, 0);
        }
        if(end < 0)
        {
            end = s_size + end;
        }
        if(begin > end || begin >= s_size)
        {
            return pair_ii_t(sorted_set::end(), sorted_set::end());
        }
        if(end > s_size)
        {
            end = s_size;
        }
        return pair_ii_t(sorted_set::begin() + begin, sorted_set::end() - (s_size - end));
    }
    pair_cici_t slice(int begin = 0, int end = std::numeric_limits<int>::max()) const
    {
        int s_size = size();
        if(begin < 0)
        {
            begin = std::max(s_size + begin, 0);
        }
        if(end < 0)
        {
            end = s_size + end;
        }
        if(begin > end || begin >= s_size)
        {
            return pair_cici_t(sorted_set::cend(), sorted_set::cend());
        }
        if(end > s_size)
        {
            end = s_size;
        }
        return pair_cici_t(sorted_set::cbegin() + begin, sorted_set::cend() - (s_size - end));
    }

    iterator lower_bound(key_t const &key)
    {
        return iterator(bst_lower_bound_(key));
    }
    const_iterator lower_bound(key_t const &key) const
    {
        return const_iterator(bst_lower_bound_(key));
    }
    iterator upper_bound(key_t const &key)
    {
        return iterator(bst_upper_bound_(key));
    }
    const_iterator upper_bound(key_t const &key) const
    {
        return const_iterator(bst_upper_bound_(key));
    }

    pair_ii_t equal_range(key_t const &key)
    {
        node_t *lower, *upper;
        bst_equal_range_(key, lower, upper);
        return pair_ii_t(iterator(lower), iterator(upper));
    }
    pair_cici_t equal_range(key_t const &key) const
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

    value_type &front()
    {
        return static_cast<value_node_t *>(get_most_left_())->value;
    }
    value_type &back()
    {
        return static_cast<value_node_t *>(get_most_right_())->value;
    }

    value_type const &front() const
    {
        return static_cast<value_node_t const *>(get_most_left_())->value;
    }
    value_type const &back() const
    {
        return static_cast<value_node_t const *>(get_most_right_())->value;
    }

    bool empty() const
    {
        return is_nil_(get_root_());
    }
    void clear()
    {
        while(!is_nil_(get_root_()))
        {
            erase(iterator(get_root_()));
        }
    }
    size_type size() const
    {
        return get_size_(get_root_());
    }
    size_type max_size() const
    {
        return std::numeric_limits<size_type>::max();
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

    //计算key的rank(相同取最末,从1开始)
    size_type rank(key_t const &key) const
    {
        return sbt_rank_(bst_upper_bound_(key));
    }
    //计算迭代器rank[1, size]
    static size_type rank(const_iterator where)
    {
        return sbt_rank_(where.node);
    }

protected:
    root_node_t head_;

protected:

    comparator_t const &get_comparator() const
    {
        return head_;
    }

    value_allocator_t &get_value_allocator()
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

    static key_t const &get_key_(node_t const *node)
    {
        return static_cast<value_node_t const *>(node)->value.first;
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

    node_t *bst_init_node_(node_t *parent, node_t *node)
    {
        set_parent_(node, parent);
        set_left_(node, nil_());
        set_right_(node, nil_());
        return node;
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

    node_t *bst_lower_bound_(key_t const &key)
    {
        node_t *node = get_root_(), *where = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator()(get_key_(node), key))
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

    node_t const *bst_lower_bound_(key_t const &key) const
    {
        node_t const *node = get_root_(), *where = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator()(get_key_(node), key))
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

    node_t *bst_upper_bound_(key_t const &key)
    {
        node_t *node = get_root_(), *where = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator()(key, get_key_(node)))
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

    node_t const *bst_upper_bound_(key_t const &key) const
    {
        node_t const *node = get_root_(), *where = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator()(key, get_key_(node)))
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

    void bst_equal_range_(key_t const &key, node_t *&lower_node, node_t *&upper_node)
    {
        node_t *node = get_root_();
        node_t *lower = nil_();
        node_t *upper = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator()(get_key_(node), key))
            {
                node = get_right_(node);
            }
            else
            {
                if(is_nil_(upper) && get_comparator()(key, get_key_(node)))
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
            if(get_comparator()(key, get_key_(node)))
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

    void bst_equal_range_(key_t const &key, node_t const *&lower_node, node_t const *&upper_node) const
    {
        node_t const *node = get_root_();
        node_t const *lower = nil_();
        node_t const *upper = nil_();
        while(!is_nil_(node))
        {
            if(get_comparator()(get_key_(node), key))
            {
                node = get_right_(node);
            }
            else
            {
                if(is_nil_(upper) && get_comparator()(key, get_key_(node)))
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
            if(get_comparator()(key, get_key_(node)))
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

    void sbt_insert_(node_t *key)
    {
        set_size_(key, 1);
        if(is_nil_(get_root_()))
        {
            set_root_(bst_init_node_(nil_(), key));
            set_most_left_(get_root_());
            set_most_right_(get_root_());
            return;
        }
        node_t *node = get_root_(), *where = nil_();
        bool is_left = true;
        while(!is_nil_(node))
        {
            set_size_(node, get_size_(node) + 1);
            where = node;
            if(is_left = get_comparator()(get_key_(key), get_key_(node)))
            {
                node = get_left_(node);
            }
            else
            {
                node = get_right_(node);
            }
        }
        if(is_left)
        {
            set_left_(where, node = bst_init_node_(where, key));
            if(where == get_most_left_())
            {
                set_most_left_(node);
            }
        }
        else
        {
            set_right_(where, node = bst_init_node_(where, key));
            if(where == get_most_right_())
            {
                set_most_right_(node);
            }
        }
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

    void sbt_erase_(node_t *node)
    {
        node_t *erase_node = node;
        node_t *fix_node = node;
        node_t *fix_node_parent;
        while(!is_nil_((fix_node = get_parent_(fix_node))))
        {
            set_size_(fix_node, get_size_(fix_node) - 1);
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
                sbt_erase_on_<true>(node);
            }
            else
            {
                sbt_erase_on_<false>(node);
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
        }
        else
        {
            set_right_(fix_node_parent, fix_node);
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

    template<bool is_left> void sbt_erase_on_(node_t *node)
    {
        node_t *erase_node = node;
        node_t *fix_node;
        node_t *fix_node_parent;
        node = bst_move_<!is_left>(node);
        fix_node = get_child_<is_left>(node);
        fix_node_parent = node;
        while((fix_node_parent = get_parent_(fix_node_parent)) != erase_node)
        {
            set_size_(fix_node_parent, get_size_(fix_node_parent) - 1);
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
        sbt_refresh_size_(node);
    }
};
