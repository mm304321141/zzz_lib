#pragma once

#include <cstdint>
#include <algorithm>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <type_traits>


namespace pro_hash_detail
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

    template<class iterator_t, class tag_t, class ...args_t> void construct_one(iterator_t where, tag_t, args_t &&...args)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        ::new(std::addressof(*where)) iterator_value_t(std::forward<args_t>(args)...);
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
}

template<class config_t>
class pro_hash
{
public:
    typedef typename config_t::key_type key_type;
    typedef typename config_t::mapped_type mapped_type;
    typedef typename config_t::value_type value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef typename config_t::hasher hasher;
    typedef typename config_t::key_equal key_equal;
    typedef typename config_t::allocator_type allocator_type;
    typedef typename config_t::offset_type offset_type;
    typedef typename config_t::hash_value_type hash_value_type;
    typedef value_type &reference;
    typedef value_type const &const_reference;
    typedef value_type *pointer;
    typedef value_type const *const_pointer;

    enum
    {
        offset_empty = offset_type(-1)
    };

protected:
    struct hash_t
    {
        hash_value_type hash;
        hash_t()
        {
        }
        hash_t(hash_value_type value)
        {
            set(value);
        }
        size_type operator % (size_type value) const
        {
            return hash % value;
        }
        bool operator == (hash_t const &other) const
        {
            return hash == other.hash;
        }
        bool operator !() const
        {
            return hash == (hash_value_type(1) << (sizeof(hash_value_type) * 8 - 1));
        }
        operator bool() const
        {
            return hash != (hash_value_type(1) << (sizeof(hash_value_type) * 8 - 1));
        }
        void set(hash_value_type value)
        {
            hash = value & ~(hash_value_type(1) << (sizeof(hash_value_type) * 8 - 1));
        }
        void clear()
        {
            hash = (hash_value_type(1) << (sizeof(hash_value_type) * 8 - 1));
        }
    };
    struct index_t
    {
        hash_t hash;
        offset_type next;
        offset_type prev;
    };
    struct value_t
    {
        typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type value_pod;

        value_type *value()
        {
            return reinterpret_cast<value_type *>(&value_pod);
        }
        value_type const *value() const
        {
            return reinterpret_cast<value_type const *>(&value_pod);
        }
    };

    typedef typename allocator_type::template rebind<offset_type>::other bucket_allocator_t;
    typedef typename allocator_type::template rebind<index_t>::other index_allocator_t;
    typedef typename allocator_type::template rebind<value_t>::other value_allocator_t;
    struct root_t : public hasher, public key_equal, public bucket_allocator_t, public index_allocator_t, public value_allocator_t
    {
        template<class any_hasher, class any_key_equal, class any_allocator_type> root_t(any_hasher &&hash, any_key_equal &&equal, any_allocator_type &&alloc)
            : hasher(std::forward<any_hasher>(hash))
            , key_equal(std::forward<any_key_equal>(equal))
            , bucket_allocator_t(alloc)
            , index_allocator_t(alloc)
            , value_allocator_t(std::forward<any_allocator_type>(alloc))
        {
            static_assert(std::is_unsigned<offset_type>::value && std::is_integral<offset_type>::value, "offset_type must be unsighed integer");
            static_assert(sizeof(offset_type) <= sizeof(pro_hash::size_type), "offset_type too big");
            static_assert(std::is_integral<hash_value_type>::value, "hash_value_type must be integer");
            bucket_count = 0;
            capacity = 0;
            size = 0;
            free_list = offset_empty;
            free_count = 0;
            setting_load_factor = 1;
            bucket = nullptr;
            index = nullptr;
            value = nullptr;
        }
        offset_type bucket_count;
        offset_type capacity;
        offset_type size;
        offset_type free_list;
        offset_type free_count;
        float setting_load_factor;
        offset_type *bucket;
        index_t *index;
        value_t *value;
    };
    template<class k_t, class v_t> struct get_key_select_t
    {
        key_type const &operator()(key_type const &value)
        {
            return value;
        }
        key_type const &operator()(value_type const &value)
        {
            return config_t::get_key(value);
        }
    };
    template<class k_t> struct get_key_select_t<k_t, k_t>
    {
        key_type const &operator()(key_type const &value)
        {
            return config_t::get_key(value);
        }
    };
    typedef get_key_select_t<key_type, value_type> get_key_t;
    enum
    {
        binary_search_limit = 1024
    };
public:
    class iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename pro_hash::value_type value_type;
        typedef typename pro_hash::difference_type difference_type;
        typedef typename pro_hash::reference reference;
        typedef typename pro_hash::pointer pointer;
    public:
        iterator(size_type _offset, pro_hash const *_self) : offset(_offset), self(_self)
        {
        }
        iterator(iterator const &other) : offset(other.offset), self(other.self)
        {
        }
        iterator &operator++()
        {
            offset = self->advance_next_(offset);
            return *this;
        }
        iterator operator++(int)
        {
            iterator save(*this);
            ++*this;
            return save;
        }
        reference operator *() const
        {
            return *self->root_.value[offset].value();
        }
        pointer operator->() const
        {
            return self->root_.value[offset].value();
        }
        bool operator == (iterator const &other) const
        {
            return offset == other.offset && self == other.self;
        }
        bool operator != (iterator const &other) const
        {
            return offset != other.offset || self != other.self;
        }
    private:
        friend class pro_hash;
        size_type offset;
        pro_hash const *self;
    };
    class const_iterator
    {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef typename pro_hash::value_type value_type;
        typedef typename pro_hash::difference_type difference_type;
        typedef typename pro_hash::reference reference;
        typedef typename pro_hash::const_reference const_reference;
        typedef typename pro_hash::pointer pointer;
        typedef typename pro_hash::const_pointer const_pointer;
    public:
        const_iterator(size_type _offset, pro_hash const *_self) : offset(_offset), self(_self)
        {
        }
        const_iterator(const_iterator const &other) : offset(other.offset), self(other.self)
        {
        }
        const_iterator(iterator const &it) : offset(other.offset), self(other.self)
        {
        }
        const_iterator &operator++()
        {
            offset = self->advance_next_(offset);
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator save(*this);
            ++*this;
            return save;
        }
        const_reference operator *() const
        {
            return *self->root_.value[offset].value();
        }
        const_pointer operator->() const
        {
            return self->root_.value[offset].value();
        }
        bool operator == (const_iterator const &other) const
        {
            return offset == other.offset && self == other.self;
        }
        bool operator != (const_iterator const &other) const
        {
            return offset != other.offset || self != other.self;
        }
    private:
        friend class pro_hash;
        size_type offset;
        pro_hash const *self;
    };
    typedef std::pair<iterator, bool> pair_ib_t, insert_result_t;
    typedef std::pair<size_type, bool> pair_posi_t;
protected:
    insert_result_t result_(pair_posi_t posi)
    {
        return std::make_pair(iterator(posi.first, this), posi.second);
    }
public:;
       //empty
       pro_hash() : root_(hasher(), key_equal(), allocator_type())
       {
       }
       //empty
       explicit pro_hash(size_type bucket_count, hasher const &hash = hasher(), key_equal const &equal = key_equal(), allocator_type const &alloc = allocator_type()) : root_(hash, equal, alloc)
       {
           rehash(bucket_count);
       }
       //empty
       explicit pro_hash(allocator_type const &alloc) : root_(hasher(), key_equal(), alloc)
       {
       }
       //empty
       pro_hash(size_type bucket_count, allocator_type const &alloc) : root_(hasher(), key_equal(), alloc)
       {
           rehash(bucket_count);
       }
       //empty
       pro_hash(size_type bucket_count, hasher const &hash, allocator_type const &alloc) : root_(hash, key_equal(), alloc)
       {
           rehash(bucket_count);
       }
       //range
       template <class iterator_t> pro_hash(iterator_t begin, iterator_t end, size_type bucket_count = 8, hasher const &hash = hasher(), key_equal const &equal = key_equal(), allocator_type const &alloc = allocator_type()) : root_(hash, equal, alloc)
       {
           rehash(bucket_count);
           insert(begin, end);
       }
       //range
       template <class iterator_t> pro_hash(iterator_t begin, iterator_t end, size_type bucket_count, allocator_type const &alloc) : root_(hasher(), key_equal(), alloc)
       {
           rehash(bucket_count);
           insert(begin, end);
       }
       //range
       template <class iterator_t> pro_hash(iterator_t begin, iterator_t end, size_type bucket_count, hasher const &hash, allocator_type const &alloc) : root_(hash, key_equal(), alloc)
       {
           rehash(bucket_count);
           insert(begin, end);
       }
       //copy
       pro_hash(pro_hash const &other) : root_(other.get_hasher(), other.get_key_equal(), other.get_value_allocator_())
       {
           copy_all_<false>(&other.root_);
       }
       //copy
       pro_hash(pro_hash const &other, allocator_type const &alloc) : root_(other.get_hasher(), other.get_key_equal(), alloc)
       {
           copy_all_<false>(&other.root_);
       }
       //move
       pro_hash(pro_hash &&other) : root_(hasher(), key_equal(), value_allocator_t())
       {
           swap(other);
       }
       //move
       pro_hash(pro_hash &&other, allocator_type const &alloc) : root_(other.get_hasher(), other.get_key_equal(), alloc)
       {
           copy_all_<true>(&other.root_);
       }
       //initializer list
       pro_hash(std::initializer_list<value_type> il, size_type bucket_count = 8, hasher const &hash = hasher(), key_equal const &equal = key_equal(), allocator_type const &alloc = allocator_type()) : pro_hash(il.begin(), il.end(), hash, equal, alloc)
       {
       }
       //initializer list
       pro_hash(std::initializer_list<value_type> il, size_type bucket_count, allocator_type const &alloc) : pro_hash(il.begin(), il.end(), alloc)
       {
       }
       //initializer list
       pro_hash(std::initializer_list<value_type> il, size_type bucket_count, hasher const &hash, allocator_type const &alloc) : pro_hash(il.begin(), il.end(), hash, alloc)
       {
       }
       //destructor
       ~pro_hash()
       {
           dealloc_all_();
       }
       //copy
       pro_hash &operator = (pro_hash const &other)
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
       pro_hash &operator = (pro_hash &&other)
       {
           if(this == &other)
           {
               return *this;
           }
           swap(other);
           return *this;
       }
       //initializer list
       pro_hash &operator = (std::initializer_list<value_type> il)
       {
           clear();
           insert(il.begin(), il.end());
           return *this;
       }

       allocator_type get_allocator() const
       {
           return *static_cast<value_allocator_t const *>(&root_);
       }
       hasher hash_function() const
       {
           return *static_cast<hasher const *>(&root_);
       }
       key_equal key_eq() const
       {
           return *static_cast<key_equal const *>(&root_);
       }

       void swap(pro_hash &other)
       {
           std::swap(root_, other.root_);
       }

       typedef std::pair<iterator, iterator> pair_ii_t;
       typedef std::pair<const_iterator, const_iterator> pair_cici_t;

       //single element
       insert_result_t insert(value_type const &value)
       {
           return result_(insert_value_(value));
       }
       //single element
       template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(in_value_t &&value)
       {
           return result_(insert_value_(std::forward<in_value_t>(value)));
       }
       //with hint
       iterator insert(const_iterator hint, value_type const &value)
       {
           return result_(insert_value_(value));
       }
       //with hint
       template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(const_iterator hint, in_value_t &&value)
       {
           return result_(insert_value_(std::forward<in_value_t>(value)));
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
           return result_(insert_value_(std::forward<args_t>(args)...));
       }
       //with hint
       template<class ...args_t> insert_result_t emplace_hint(const_iterator hint, args_t &&...args)
       {
           return result_(insert_value_(std::forward<args_t>(args)...));
       }

       iterator find(key_type const &key)
       {
           if(root_.size == 0)
           {
               return end();
           }
           return iterator(find_value_(key), this);
       }
       const_iterator find(key_type const &key) const
       {
           if(root_.size == 0)
           {
               return cend();
           }
           return const_iterator(find_value_(key), this);
       }

       template<class in_key_t, class = typename std::enable_if<std::is_convertible<in_key_t, key_type>::value && config_t::unique_type::value && !std::is_same<key_type, value_type>::value, void>::type> mapped_type &operator[](in_key_t const &key)
       {
           offset_type offset = root_.size;
           if(root_.size != 0)
           {
               offset = find_value_(key);
           }
           if(offset == root_.size)
           {
               offset = insert_value_(std::make_pair(key, mapped_type())).first;
           }
           return root_.value[offset]->value()->second;
       }

       iterator erase(const_iterator it)
       {
           if(root_.size == 0)
           {
               return end();
           }
           remove_offset_(it.offset);
           return 1;
       }
       size_type erase(key_type const &key)
       {
           if(root_.size == 0)
           {
               return 0;
           }
           return remove_value_(key) ? 1 : 0;
       }
       iterator erase(const_iterator erase_begin, const_iterator erase_end)
       {
           if(erase_begin == cbegin() && erase_end == cend())
           {
               clear();
               return end();
           }
           else
           {
               while(erase_begin != erase_end)
               {
                   erase(erase_begin++);
               }
               return erase_begin;
           }
       }

       size_type count(key_type const &key) const
       {
           return find(key) == end() ? 0 : 1;
       }

       pair_ii_t equal_range(key_type const &key)
       {
           auto where = find(key);
           return std::make_pair(where, where == end() ? where : std::next(where));
       }
       pair_cici_t equal_range(key_type const &key) const
       {
           auto where = find(key);
           return std::make_pair(where, where == end() ? where : std::next(where));
       }

       iterator begin()
       {
           return iterator(find_begin_(), this);
       }
       iterator end()
       {
           return iterator(root_.size, this);
       }
       const_iterator begin() const
       {
           return const_iterator(find_begin_(), this);
       }
       const_iterator end() const
       {
           return const_iterator(root_.size, this);
       }
       const_iterator cbegin() const
       {
           return const_iterator(find_begin_(), this);
       }
       const_iterator cend() const
       {
           return const_iterator(root_.size, this);
       }

       bool empty() const
       {
           return root_.data.size == 0;
       }
       void clear()
       {
           clear_all_();
       }
       size_type size() const
       {
           return root_.size - root_.free_count;
       }
       size_type max_size() const
       {
           return offset_empty - 1;
       }

       void reserve(size_type count)
       {
           rehash(size_type(std::ceil(count / root_.setting_load_factor)));
       }
       void rehash(size_type count)
       {
           rehash_(std::max<size_type>({8, count, size_type(std::ceil(size() / root_.setting_load_factor))}));
       }

       void max_load_factor(float ml)
       {
           if(ml <= 0)
           {
               return;
           }
           root_.setting_load_factor = ml;
           if(root_.size != 0)
           {
               rehash_(size_type(std::ceil(size() / root_.setting_load_factor)));
           }
       }
       float max_load_factor() const
       {
           return setting_load_factor;
       }
       float load_factor() const
       {
           return size() / float(root_.bucket_count);
       }

protected:
    root_t root_;

protected:

    hasher &get_hasher()
    {
        return root_;
    }
    hasher const &get_hasher() const
    {
        return root_;
    }

    key_equal &get_key_equal()
    {
        return root_;
    }
    key_equal const &get_key_equal() const
    {
        return root_;
    }

    bucket_allocator_t &get_bucket_allocator_()
    {
        return root_;
    }
    index_allocator_t &get_index_allocator_()
    {
        return root_;
    }
    value_allocator_t &get_value_allocator_()
    {
        return root_;
    }
    value_allocator_t const &get_value_allocator_() const
    {
        return root_;
    }

    size_type advance_next_(size_type i) const
    {
        for(++i; i < root_.size; ++i)
        {
            if(root_.index[i].hash)
            {
                break;
            }
        }
        return i;
    }

    size_type find_begin_() const
    {
        for(size_type i = 0; i < root_.size; ++i)
        {
            if(root_.index[i].hash)
            {
                return i;
            }
        }
        return root_.size;
    }

    template<class iterator_t, class ...args_t> static void construct_one_(iterator_t where, args_t &&...args)
    {
        pro_hash_detail::construct_one(where, typename pro_hash_detail::get_tag<iterator_t>::type(), std::forward<args_t>(args)...);
    }

    template<class iterator_t> static void destroy_one_(iterator_t where)
    {
        pro_hash_detail::destroy_one(where, typename pro_hash_detail::get_tag<iterator_t>::type());
    }

    template<class iterator_t> static void destroy_range_(iterator_t destroy_begin, iterator_t destroy_end)
    {
        pro_hash_detail::destroy_range(destroy_begin, destroy_end, typename pro_hash_detail::get_tag<iterator_t>::type());
    }

    void dealloc_all_()
    {
        for(size_type i = 0; i < root_.size; i++)
        {
            if(root_.index[i].hash)
            {
                destroy_one_(root_.value[i].value());
            }
        }
        if(root_.bucket_count != 0)
        {
            get_bucket_allocator_().deallocate(root_.bucket, root_.bucket_count);
        }
        if(root_.capacity != 0)
        {
            get_index_allocator_().deallocate(root_.index, root_.capacity);
            get_value_allocator_().deallocate(root_.value, root_.capacity);
        }
    }

    void clear_all_()
    {
        dealloc_all_();
        root_.bucket_count = 0;
        root_.capacity = 0;
        root_.size = 0;
        root_.free_list = offset_empty;
        root_.free_count = 0;
        root_.bucket = nullptr;
        root_.index = nullptr;
        root_.value = nullptr;
    }
    
    void rehash_(size_type size)
    {
        if(root_.bucket_count != 0)
        {
            get_bucket_allocator_().deallocate(root_.bucket, root_.bucket_count);
        }
        root_.bucket = get_bucket_allocator_().allocate(size);
        std::memset(root_.bucket, 0xFFFFFFFF, sizeof(offset_type) * size);

        for(size_type i = 0; i < root_.size; i++)
        {
            if(root_.index[i].hash)
            {
                size_type bucket = root_.index[i].hash % size;
                if(root_.bucket[bucket] != offset_empty)
                {
                    root_.index[root_.bucket[bucket]].prev = i;
                }
                root_.index[i].prev = offset_empty;
                root_.index[i].next = root_.bucket[bucket];
                root_.bucket[bucket] = i;
            }
        }
        root_.bucket_count = size;
    }

    void realloc_(size_type size)
    {
        index_t *new_index = get_index_allocator_().allocate(size);
        value_t *new_value = get_value_allocator_().allocate(size);

        std::memset(new_index + root_.capacity, 0xFFFFFFFF, sizeof(index_t) * (size - root_.capacity));
        if(root_.capacity != 0)
        {
            std::memcpy(new_index, root_.index, sizeof(index_t) * root_.capacity);
            std::uninitialized_copy(std::move_iterator<value_type *>(root_.value->value()), std::move_iterator<value_type *>(root_.value->value() + root_.capacity), new_value->value());
            destroy_range_(root_.value->value(), root_.value->value() + root_.capacity);
            get_index_allocator_().deallocate(root_.index, root_.capacity);
            get_value_allocator_().deallocate(root_.value, root_.capacity);
        }
        root_.capacity = size;
        root_.index = new_index;
        root_.value = new_value;
    }

    template<bool move> void copy_all_(root_t const *other)
    {
        root_.bucket_count = other->bucket_count;
        root_.capacity = other->capacity;
        root_.size = other->size;
        root_.free_list = other->free_list;
        root_.free_count = other->free_count;
        root_.bucket = get_bucket_allocator_().allocate(other->bucket_count);
        root_.index = get_index_allocator_().allocate(other->capacity);
        root_.value = get_value_allocator_().allocate(other->capacity);
        std::memcpy(root_.bucket, other->bucket, sizeof(offset_type) * other->bucket_count);
        std::memcpy(root_.index, other->index, sizeof(index_t) * other->capacity);
        if(move)
        {
            std::uninitialized_copy(std::move_iterator<value_type *>(other->value->value()), std::move_iterator<value_type *>(other->value->value() + other->capacity), root_.value->value());
        }
        else
        {
            std::uninitialized_copy(other->value->value(), other->value->value() + other->capacity, root_.value->value());
        }
    }

    void check_grow_()
    {
        if(root_.size >= max_size())
        {
            throw std::length_error("pro_hash too long");
        }
        if(root_.bucket_count < std::max<size_type>(8, size_type(std::ceil(root_.size / root_.setting_load_factor))))
        {
            rehash_(std::max<size_type>(8, root_.bucket_count * 2 + 1));
        }
        if(root_.size == root_.capacity)
        {
            realloc_(std::max<size_type>({8, root_.capacity * 2 + 1, size_type(std::ceil(root_.bucket_count * root_.setting_load_factor))}));
        }
    }

    template<class ...args_t> pair_posi_t insert_value_(args_t &&...args)
    {
        check_grow_();
        return insert_value_uncheck_(std::forward<args_t>(args)...);
    }

    template<class in_t, class ...args_t> pair_posi_t insert_value_uncheck_(in_t &&in, args_t &&...args)
    {
        hash_t hash = get_hasher()(get_key_t()(in));
        size_type bucket = hash % root_.bucket_count;
        for(size_type i = root_.bucket[bucket]; i != offset_empty; i = root_.index[i].next)
        {
            if(root_.index[i].hash == hash && get_key_equal()(get_key_t()(*root_.value[i].value()), get_key_t()(in)))
            {
                return std::make_pair(i, false);
            }
        }
        size_type offset = root_.free_list == offset_empty ? root_.size : root_.free_list;
        construct_one_(root_.value[offset].value(), std::forward<in_t>(in), std::forward<args_t>(args)...);
        if(offset == root_.free_list)
        {
            root_.free_list = root_.index[offset].next;
            --root_.free_count;
        }
        else
        {
            ++root_.size;
        }
        if(root_.bucket[bucket] != offset_empty)
        {
            root_.index[root_.bucket[bucket]].prev = offset;
        }
        root_.index[offset].prev = offset_empty;
        root_.index[offset].next = root_.bucket[bucket];
        root_.index[offset].hash = hash;
        root_.bucket[bucket] = offset;
        return std::make_pair(offset, true);
    }

    offset_type find_value_(key_type const &key)
    {
        hash_t hash = get_hasher()(get_key_t()(key));
        offset_type bucket = hash % root_.bucket_count;

        for(offset_type i = root_.bucket[bucket]; i != offset_empty; i = root_.index[i].next)
        {
            if(root_.index[i].hash == hash && get_key_equal()(get_key_t()(*root_.value[i].value()), get_key_t()(key)))
            {
                return i;
            }
        }
        return root_.size;
    }

    bool remove_value_(key_type const &key)
    {
        hash_t hash = get_hasher()(get_key_t()(key));
        size_type bucket = hash % root_.bucket_count;
        size_type last = offset_empty;

        for(offset_type i = root_.bucket[bucket]; i != offset_empty; i = root_.index[i].next)
        {
            if(root_.index[i].hash == hash && get_key_equal()(get_key_t()(*root_.value[i].value()), get_key_t()(key)))
            {
                if(last == offset_empty)
                {
                    root_.bucket[bucket] = root_.index[i].next;
                }
                else
                {
                    root_.index[last].next = root_.index[i].next;
                }
                if(root_.index[i].next != offset_empty)
                {
                    root_.index[root_.index[i].next].prev = root_.index[i].prev;
                }
                root_.index[i].next = root_.free_list;
                root_.index[i].hash.clear();
                root_.free_list = i;
                ++root_.free_count;

                destroy_one_(root_.value[i].value());
                return true;
            }
            last = i;
        }
        return false;
    }

    void remove_offset_(offset_type offset)
    {
        if(root_.index[offset].prev != offset_empty)
        {
            root_.index[root_.index[offset].prev].next = root_.index[offset].next;
        }
        else
        {
            root_.bucket[root_.index[offset].hash % root_.bucket_count] = root_.index[offset].next;
        }
        if(root_.index[offset].next != offset_empty)
        {
            root_.index[root_.index[offset].next].prev = root_.index[offset].prev;
        }
        index->hash.clear();
        dealloc_offset_(offset);

        destroy_one_(root_.value[offset].value());
    }
    
};
