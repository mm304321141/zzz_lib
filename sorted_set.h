#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
class sorted_set
{
public:
    typedef std::pair<key_t const, value_t> pair_t;
protected:
    struct node_t
    {
        node_t *parent, *left, *right;
        size_t nil : 1;
        size_t size : sizeof(size_t) * 8 - 1;
    };
    struct value_node_t : public node_t
    {
        value_node_t(pair_t const &v) : value(v)
        {
        }
        value_node_t(pair_t &&v) : value(v)
        {
        }
        pair_t value;
    };
    struct root_node_t : public node_t, public allocator_t::template rebind<value_node_t>::other, public comparator_t
    {
    };

public:
    class iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef node_t value_type;
        typedef int difference_type;
        typedef unsigned int distance_type;
        typedef node_t *pointer;
        typedef node_t reference;
    public:
        iterator(node_t *node, sorted_set *set) : ptr_(node), set_(set)
        {
        }
        iterator(iterator const &other) : ptr_(other.ptr_), set_(other.set_)
        {
        }
        iterator &operator += (difference_type diff)
        {
            ptr_ = set_->sbt_advance_(ptr_, diff);
            return *this;
        }
        iterator &operator -= (difference_type diff)
        {
            ptr_ = set_->sbt_advance_(ptr_, -diff);
            return *this;
        }
        iterator operator + (difference_type diff) const
        {
            return iterator(set_->sbt_advance_(ptr_, diff), set_);
        }
        iterator operator - (difference_type diff) const
        {
            return iterator(set_->sbt_advance_(ptr_, -diff), set_);
        }
        difference_type operator - (iterator const &other) const
        {
            return static_cast<difference_type>(set_->sbt_rank_(ptr_)) - static_cast<difference_type>(sorted_set::sbt_rank_(other.ptr_));
        }
        iterator &operator++()
        {
            ptr_ = sorted_set::bst_move_<true>(ptr_);
            return *this;
        }
        iterator &operator--()
        {
            ptr_ = sorted_set::bst_move_<false>(ptr_);
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
        pair_t &operator *() const
        {
            return static_cast<value_node_t *>(ptr_)->value;
        }
        pair_t *operator->() const
        {
            return &static_cast<value_node_t *>(ptr_)->value;
        }
        bool operator == (iterator const &other) const
        {
            return ptr_ == other.ptr_;
        }
        bool operator != (iterator const &other) const
        {
            return ptr_ != other.ptr_;
        }
    private:
        friend class sorted_set;
        node_t *ptr_;
        sorted_set *set_;
    };
    class const_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef node_t value_type;
        typedef int difference_type;
        typedef unsigned int distance_type;
        typedef node_t *pointer;
        typedef node_t reference;
    public:
        const_iterator(node_t const *node, sorted_set const *set) : ptr_(node), set_(set)
        {
        }
        const_iterator(iterator const &other) : ptr_(other.ptr_), set_(other.set_)
        {
        }
        const_iterator(const_iterator const &other) : ptr_(other.ptr_), set_(other.set_)
        {
        }
        const_iterator &operator += (difference_type diff)
        {
            ptr_ = set_->sbt_advance_(ptr_, diff);
            return *this;
        }
        const_iterator &operator -= (difference_type diff)
        {
            ptr_ = set_->sbt_advance_(ptr_, -diff);
            return *this;
        }
        const_iterator operator + (difference_type diff) const
        {
            return const_iterator(set_->sbt_advance_(ptr_, diff), set_);
        }
        const_iterator operator - (difference_type diff) const
        {
            return const_iterator(set_->sbt_advance_(ptr_, -diff), set_);
        }
        difference_type operator - (const_iterator const &other) const
        {
            return static_cast<difference_type>(set_->sbt_rank_(ptr_)) - static_cast<difference_type>(sorted_set::sbt_rank_(other.ptr_));
        }
        const_iterator &operator++()
        {
            ptr_ = sorted_set::bst_move_<true>(ptr_);
            return *this;
        }
        const_iterator &operator--()
        {
            ptr_ = sorted_set::bst_move_<false>(ptr_);
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
        pair_t const &operator *() const
        {
            return static_cast<value_node_t const *>(ptr_)->value;
        }
        pair_t const *operator->() const
        {
            return &static_cast<value_node_t const *>(ptr_)->value;
        }
        bool operator == (const_iterator const &other) const
        {
            return ptr_ == other.ptr_;
        }
        bool operator != (const_iterator const &other) const
        {
            return ptr_ != other.ptr_;
        }
    private:
        friend class sorted_set;
        node_t const *ptr_;
        sorted_set const *set_;
    };

public:
    sorted_set()
    {
        set_nil_(nil_(), true);
        set_size_(nil_(), 0);
        set_root_(nil_());
        set_most_left_(nil_());
        set_most_right_(nil_());
    }
    sorted_set(sorted_set &&other)
    {
        set_nil_(nil_(), true);
        set_size_(nil_(), 0);
        set_root_(other.get_root_());
        set_most_left_(other.get_most_left_());
        set_most_right_(other.get_most_right_());
        other.set_root_(other.nil_());
        other.set_most_left_(other.nil_());
        other.set_most_right_(other.nil_());
    }
    sorted_set(sorted_set const &other) = delete;
    sorted_set &operator = (sorted_set &&other)
    {
        set_root_(other.get_root_());
        set_most_left_(other.get_most_left_());
        set_most_right_(other.get_most_right_());
        other.set_root_(other.nil_());
        other.set_most_left_(other.nil_());
        other.set_most_right_(other.nil_());
    }
    sorted_set &operator = (sorted_set const &other) = delete;

    typedef std::pair<iterator, bool> pair_ib_t;
    typedef std::pair<iterator, iterator> pair_ii_t;
    typedef std::pair<const_iterator, const_iterator> pair_cici_t;

    //允许重复key
    pair_ib_t insert(pair_t const &node)
    {
        value_node_t *new_node = get_allocator().allocate(1);
        ::new(new_node) value_node_t(node);
        sbt_insert_(new_node);
        return pair_ib_t(iterator(new_node, this), true);
    }
    //允许重复key
    pair_ib_t insert(pair_t &&node)
    {
        value_node_t *new_node = get_allocator().allocate(1);
        ::new(new_node) value_node_t(node);
        sbt_insert_(new_node);
        return pair_ib_t(iterator(new_node, this), true);
    }
    //返回插入了多少个
    template<class iterator_t>
    size_t insert(iterator_t begin, iterator_t end)
    {
        size_t insert_count = 0;
        for(; begin != end; ++begin)
        {
            if(insert(*begin).second)
            {
                ++insert_count;
            }
        }
        return insert_count;
    }
    iterator find(key_t const &key)
    {
        node_t *where = bst_lower_bound_(key);
        return (is_nil_(where) || get_comparator()(key, get_key_(where))) ? iterator(nil_(), this) : iterator(where, this);
    }
    const_iterator find(key_t const &key) const
    {
        node_t *where = bst_lower_bound_(key);
        return (is_nil_(where) || get_comparator()(key, get_key_(where))) ? iterator(nil_(), this) : iterator(where, this);
    }
    //不解释
    void erase(iterator where)
    {
        sbt_erase_(where.ptr_);
        static_cast<value_node_t *>(where.ptr_)->~value_node_t();
        get_allocator().deallocate(static_cast<value_node_t *>(where.ptr_), 1);
    }
    //返回移除了多少个
    size_t erase(key_t const &key)
    {
        size_t erase_count = 0;
        pair_ii_t range = equal_range(key);
        while(range.first != range.second)
        {
            erase(range.first++);
            ++erase_count;
        }
        return erase_count;
    }
    //计数该key存在个数
    size_t count(key_t const &key) const
    {
        pair_cici_t range = equal_range(key);
        return std::distance(range.first, range.second);
    }
    //计数[min, max)
    size_t count(key_t const &min, key_t const &max) const
    {
        return sbt_rank_(bst_upper_bound_(max)) - sbt_rank_(bst_lower_bound_(min));
    }
    //获取[min, max)
    pair_ii_t range(key_t const &min, key_t const &max)
    {
        return pair_ii_t(iterator(bst_lower_bound_(min), this), iterator(bst_upper_bound_(max), this));
    }
    pair_cici_t range(key_t const &min, key_t const &max) const
    {
        return pair_cici_t(const_iterator(bst_lower_bound_(min), this), const_iterator(bst_upper_bound_(max), this));
    }
    //获取下标begin到end之间(参数小于0反向下标)
    pair_ii_t slice(int begin = 0, int end = std::numeric_limits<int>::max())
    {
        int size_s = size();
        if(begin < 0)
        {
            begin = std::max(size_s + begin, 0);
        }
        if(end < 0)
        {
            end = size_s + end;
        }
        if(begin > end || begin >= size_s)
        {
            return pair_ii_t(sorted_set::end(), sorted_set::end());
        }
        if(end > size_s)
        {
            end = size_s;
        }
        return pair_ii_t(sorted_set::begin() + begin, sorted_set::end() - (size_s - end));
    }
    pair_cici_t slice(int begin = 0, int end = std::numeric_limits<int>::max()) const
    {
        int size_s = size();
        if(begin < 0)
        {
            begin = std::max(size_s + begin, 0);
        }
        if(end < 0)
        {
            end = size_s + end;
        }
        if(begin > end || begin >= size_s)
        {
            return pair_ii_t(sorted_set::end(), sorted_set::end());
        }
        if(end > size_s)
        {
            end = size_s;
        }
        return pair_cici_t(sorted_set::begin() + begin, sorted_set::end() - (size_s - end));
    }

    iterator lower_bound(key_t const &key)
    {
        return iterator(bst_lower_bound_(key), this);
    }
    const_iterator lower_bound(key_t const &key) const
    {
        return const_iterator(bst_lower_bound_(key), this);
    }
    iterator upper_bound(key_t const &key)
    {
        return iterator(bst_upper_bound_(key), this);
    }
    const_iterator upper_bound(key_t const &key) const
    {
        return const_iterator(bst_upper_bound_(key), this);
    }

    pair_ii_t equal_range(key_t const &key)
    {
        node_t *lower, *upper;
        bst_equal_range_(key, lower, upper);
        return pair_ii_t(iterator(lower, this), iterator(upper, this));
    }
    pair_cici_t equal_range(key_t const &key) const
    {
        node_t const *lower, *upper;
        bst_equal_range_(key, lower, upper);
        return pair_cici_t(const_iterator(lower, this), const_iterator(upper, this));
    }

    iterator begin()
    {
        return iterator(get_most_left_(), this);
    }
    iterator end()
    {
        return iterator(nil_(), this);
    }
    const_iterator begin() const
    {
        return const_iterator(get_most_left_(), this);
    }
    const_iterator end() const
    {
        return const_iterator(nil_(), this);
    }

    pair_t &front()
    {
        return static_cast<value_node_t *>(get_most_left_())->value;
    }
    pair_t &back()
    {
        return static_cast<value_node_t *>(get_most_right_())->value;
    }

    pair_t const &front() const
    {
        return static_cast<value_node_t *>(get_most_left_())->value;
    }
    pair_t const &back() const
    {
        return static_cast<value_node_t *>(get_most_right_())->value;
    }

    //不解释
    bool empty() const
    {
        return is_nil_(get_root_());
    }
    //不解释
    void clear()
    {
        while(get_root_() != nil_())
        {
            erase(iterator(get_root_(), this));
        }
    }
    //不解释
    size_t size() const
    {
        return get_root_() == nil_() ? 0 : get_size_(get_root_());
    }
    //下标访问[0, size)
    iterator at(size_t index)
    {
        return iterator(static_cast<value_node_t *>(sbt_at_(get_root_(), index)), this);
    }
    //下标访问[0, size)
    const_iterator at(size_t index) const
    {
        return iterator(static_cast<value_node_t *>(sbt_at_(get_root_(), index)), this);
    }
    //计算key的rank(相同取最末,从1开始)
    size_t rank(key_t const &key) const
    {
        return sbt_rank_(bst_upper_bound_(key));
    }
    //计算迭代器rank[1, size]
    static size_t rank(const_iterator where)
    {
        return sbt_rank_(where.ptr_);
    }

protected:
    root_node_t head_;

protected:

    comparator_t const &get_comparator() const
    {
        return head_;
    }

    typename allocator_t::template rebind<value_node_t>::other &get_allocator()
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

    void set_most_right_(node_t *right)
    {
        set_right_(&head_, right);
    }

    static key_t const &get_key_(node_t const *node)
    {
        return static_cast<value_node_t const *>(node)->value.first;
    }

    static void set_nil_(node_t *node, bool nil)
    {
        node->nil = nil;
    }

    static bool is_nil_(node_t const *node)
    {
        return node->nil;
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

    static size_t get_size_(node_t const *node)
    {
        return node->size;
    }

    static void set_size_(node_t *node, size_t size)
    {
        node->size = size;
    }

    void sbt_refresh_size_(node_t *node)
    {
        set_size_(node, get_size_(get_left_(node)) + get_size_(get_right_(node)) + 1);
    }

    template<bool is_left>
    static void set_child_(node_t *node, node_t *child)
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

    template<bool is_left>
    static node_t *get_child_(node_t const *node)
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
        set_nil_(node, false);
        set_parent_(node, parent);
        set_left_(node, nil_());
        set_right_(node, nil_());
        return node;
    }

    template<bool is_next, typename in_node_t>
    static in_node_t *bst_move_(in_node_t *node)
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

    template<bool is_min>
    static node_t *bst_most_(node_t *node)
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

    static node_t *sbt_at_(node_t *node, size_t index)
    {
        if(index >= get_size_(node))
        {
            return nullptr;
        }
        size_t rank = get_size_(get_left_(node));
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

    static node_t *sbt_advance_(node_t *node, int step)
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
        size_t u_step;
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

    static size_t sbt_rank_(node_t const *node)
    {
        if(is_nil_(node))
        {
            return get_size_(get_parent_(node));
        }
        size_t rank = get_size_(get_left_(node));
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

    template<bool is_left>
    node_t *sbt_rotate_(node_t *node)
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

    template<bool is_left>
    node_t *sbt_maintain_(node_t *node)
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

    template<bool is_left>
    void sbt_erase_on_(node_t *node)
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
