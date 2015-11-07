
#define _SCL_SECURE_NO_WARNINGS

#include "bpstree_map.h"
#include "bpstree_set.h"

#include <chrono>
#include <iostream>
#include <random>
#include <map>
#include <set>
#include <cstring>
#include <string>


auto assert = [](bool no_error)
{
    if(!no_error)
    {
        *static_cast<int *>(0) = 0;
    }
};

size_t identity_seed = 0;
size_t alloc_limit = 999999999;
template<typename T>
class test_allocator
{
public:
    typedef T value_type;
    typedef T *pointer;
    typedef T const *const_pointer;
    typedef T &reference;
    typedef T const &const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    test_allocator() : set(new std::set<T *>()), max(999999999)
    {
        root = fork = ++identity_seed;
    }
    test_allocator(test_allocator const &other) : set(other.set), max(other.max)
    {
        root = other.root;
        fork = other.fork;
    }
    template<class U> test_allocator(test_allocator<U> const &other) : set(new std::set<T *>()), max(other.max)
    {
        root = other.root;
        fork = ++identity_seed;
    }

    template<class U>
    friend class test_allocator;

    template<class U>
    struct rebind
    {
        typedef test_allocator<U> other;
    };
    pointer allocate(size_type n)
    {
        if(alloc_limit == 0 || set->size() >= max_size() - 1)
        {
            throw std::bad_alloc();
        }
        --alloc_limit;
        return *set->insert(reinterpret_cast<pointer>(new uint8_t[sizeof(T) * n])).first;
    }
    void deallocate(pointer ptr, size_type n)
    {
        assert(set->erase(ptr) == 1);
        delete[] reinterpret_cast<uint8_t *>(ptr);
    }
    template<class U, class ...args_t> void construct(U *ptr, args_t &&...args)
    {
        ::new(ptr) U(std::forward<args_t>(args)...);
    }
    void destroy(pointer ptr)
    {
        ptr->~T();
    }
    pointer address(reference x)
    {
        return &x;
    }
    size_type max_size() const
    {
        return max;
    }
    size_type& max_size()
    {
        return max;
    }
    bool operator == (test_allocator const &other)
    {
        return set == other.set;
    }
private:
    std::shared_ptr<std::set<T *>> set;
    size_t max;
    size_t root;
    size_t fork;
};

class checker
{
    int i;
    void c(bool b) const
    {
        if(!b)
        {
            *static_cast<int *>(0) = 0;
        }
    }
public:
    checker() : i(-1)
    {
    }
    ~checker()
    {
        i = -1;
    }
    checker(int v) : i(v)
    {
        c(i >= 0);
    }
    checker(checker const &o) : i(o.i)
    {
        c(i >= 0);
    }
    checker(checker &&o) : i(o.i)
    {
        o.i = -1;
        c(i >= 0);
    }
    checker &operator = (checker const &o)
    {
        i = o.i;
        c(i >= 0);
        return *this;
    }
    checker &operator = (checker &&o)
    {
        i = o.i;
        o.i = -1;
        c(i >= 0);
        return *this;
    }
    bool operator < (checker const &o) const
    {
        c(i >= 0);
        c(o.i >= 0);
        return i < o.i;
    }
};

int main()
{
    auto t = std::chrono::high_resolution_clock::now;
    std::mt19937 mt(0);
    auto mtr = std::uniform_int_distribution<int>(-99999, 99999);

    [&]
    {
        bpstree_multimap<std::string, std::string> tree;

        mt.seed(0);
        for(int i = 0; i <= 999999; ++i)
        {
            tree.insert(std::make_pair(std::to_string(mtr(mt)), "2"));
            tree.emplace(std::to_string(i), "1");
        }
        mt.seed(0);
        for(int i = 0; i <= 999999; ++i)
        {
            tree.erase(tree.find(std::to_string(999999 - i)));
        }
        for(int i = 0; i <= 999999; ++i)
        {
            tree.erase(std::to_string(mtr(mt)));
        }
    }();
    system("pause");
    [&]
    {
        bpstree_set<int> tree;

        mt.seed(0);
        for(int i = 0; i <= 99999999; ++i)
        {
            tree.insert(mtr(mt));
            tree.emplace(i);
        }
        mt.seed(0);
        for(int i = 0; i <= 99999999; ++i)
        {
            tree.erase(tree.find(99999999 - i));
        }
        for(int i = 0; i <= 99999999; ++i)
        {
            tree.erase(mtr(mt));
        }
    }();
    system("pause");
























    //std::vector<int> v;
    //v.resize(20000000);
    //auto reset = [&mtr, &mt, &v]()
    //{
    //    for(auto &value : v)
    //    {
    //        value = mtr(mt);
    //    }
    //};

    //auto testsb = [&mtr, &mt, &c = sb, &v]()
    //{
    //    for(int i = 0; i < int(v.size()); ++i)
    //    {
    //        c.insert(std::make_pair(i, i));
    //    }
    //    for(int i = 0; i < int(v.size()); ++i)
    //    {
    //        c.insert(std::make_pair(v[i], i));
    //    }
    //    for(int i = 0; i < int(v.size()); ++i)
    //    {
    //        c.erase(v[i]);
    //    }
    //    c.clear();
    //};
    //auto testrb = [&mtr, &mt, &c = rb, &v]()
    //{
    //    for(int i = 0; i < int(v.size()); ++i)
    //    {
    //        c.insert(std::make_pair(i, i));
    //    }
    //    for(int i = 0; i < int(v.size()); ++i)
    //    {
    //        c.insert(std::make_pair(v[i], i));
    //    }
    //    for(int i = 0; i < int(v.size()); ++i)
    //    {
    //        c.erase(v[i]);
    //    }
    //    c.clear();
    //};
    //reset();
    //auto ss1 = t();
    //testsb();
    //auto se1 = t();
    //auto rs1 = t();
    //testrb();
    //auto re1 = t();
    //reset();
    //auto ss2 = t();
    //testsb();
    //auto se2 = t();
    //auto rs2 = t();
    //testrb();
    //auto re2 = t();
    //reset();
    //auto ss3 = t();
    //testsb();
    //auto se3 = t();
    //auto rs3 = t();
    //testrb();
    //auto re3 = t();

    //v.clear();

    //std::cout
    //    << "sb time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(se1 - ss1).count() << std::endl
    //    << "rb time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re1 - rs1).count() << std::endl
    //    << "sb time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(se2 - ss2).count() << std::endl
    //    << "rb time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re2 - rs2).count() << std::endl
    //    << "sb time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(se3 - ss3).count() << std::endl
    //    << "rb time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re3 - rs3).count() << std::endl
    //    ;

    //system("pause");

    //for(int i = 0; i < 20000000; ++i)
    //{
    //    sb.insert(std::make_pair(mtr(mt), i));
    //}
    //sb.print_tree();
}