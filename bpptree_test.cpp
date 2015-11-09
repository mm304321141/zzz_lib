﻿
#define _SCL_SECURE_NO_WARNINGS

#include "bpptree_map.h"
#include "bpptree_set.h"

#include <chrono>
#include <iostream>
#include <random>
#include <map>
#include <set>
#include <cstring>
#include <string>

#define assert(exp) assert_proc(exp, #exp, __FILE__, __LINE__)

auto assert_proc = [](bool no_error, char const *query, char const *file, size_t line)
{
    if(!no_error)
    {
        printf("%s(%zd):%s\n", file, line, query);
        *static_cast<int *>(0) = 0;
    }
};

struct test_comp
{
    bool is_less = 0;
    bool operator()(int l, int r) const
    {
        if(is_less)
        {
            return l < r;
        }
        else
        {
            return l > r;
        }
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
    [&]()
    {
        std::mt19937 mt(0);
        auto mtr = std::uniform_real_distribution<double>(-99999999, 99999999);

        bpptree_multimap<double, uint64_t> bp;
        for(int i = 0; i < 100000000; ++i)
        {
            bp.emplace(mtr(mt), i);
        }
        system("pause");
    }();

    std::multimap<int, int> rb;
    bpptree_multimap<int, int> bp;

    [&]()
    {
        bpptree_set<std::string> foo = {"1", "2"};
        assert(!foo.insert(foo.begin(), "2").second);
        size_t count = foo.size();
        for(int i = 0; i < 1000000 && count < 32678; ++i)
        {
            if(foo.emplace(std::to_string(std::rand())).second)
            {
                ++count;
            }
        }
        assert(foo.size() == count);
        assert(foo.erase("2") == 1);
        while(!foo.empty())
        {
            foo.erase(foo.at(std::rand() % foo.size()));
        }
        assert(foo.size() == 0);
    }();

    [&]()
    {
        test_allocator<int> a;
        test_comp c;
        c.is_less = true;
        bpptree_multiset<int, test_comp, test_allocator<int>> aaa({1, 2, 3}, c, a);
        c.is_less = false;
        bpptree_multiset<int, test_comp, test_allocator<int>> aaa2({4, 5, 6}, c);
        bpptree_multiset<int, test_comp, test_allocator<int>> aaa3(std::move(aaa), a);
        bpptree_multiset<int, test_comp, test_allocator<int>> aaa4(aaa2);
        aaa.swap(aaa2);
        aaa3 = aaa;
        aaa3.emplace(7);
        aaa = aaa3;
        bpptree_multimap<int, int> bp({{1, 2},{1, 2}});
        bpptree_multimap<int, int> const sb2(bp, bpptree_multimap<int, int>::allocator_type());
        bp.insert({{3, 4},{5, 6}});
        bp.insert(bp.begin(), {7, 8});
        bp.erase(bp.begin() + 1, bp.end());
        sb2.find(0);
        sb2.slice();
        sb2.front();
        sb2.back();
        sb2.equal_range(2);
        sb2.lower_bound(2);
        sb2.upper_bound(2);
        sb2.range(2, 2);
        sb2.rank(2);
        sb2.count(2);
        sb2.count(2, 2);
        for(int i = 0; i < 100000; ++i)
        {
            aaa.emplace(i);
        }
        aaa.clear();
        bpptree_multimap<int, std::string, test_comp, test_allocator<std::pair<int const, std::string>>> ttt({{1, "2"},{1, "2"},{1, "2"}}, c);
        bpptree_multimap<int, std::string, test_comp, test_allocator<std::pair<int const, std::string>>> tttt({{1, "2"}}, a);
        alloc_limit = 2;
        try
        {
            tttt = ttt;
        }
        catch(...)
        {
        }
        alloc_limit = 999999999;
        bpptree_multimap<std::string, std::string> sss =
        {
            {"0", ""},
            {"1", "11"},
            {"2", "222"},
        };
        sss.emplace("0", "0");
        sss =
        {
            {"0", "0"},
            {"1", "11"},
        };
        sss.erase("1");
        sss.emplace(std::make_pair("1", "1"));
        sss.insert(std::make_pair("2", "2"));
        sss.emplace("0", "00");
        sss.emplace("2", "22");
        assert(sss.erase("0") == 2);
        assert((sss.find("2") + 1)->second == "22");
        sss.clear();
    }();

    [&]()
    {
        bpptree_multimap<int, int> sb1;
        assert(bp.size() == sb1.size());
        for(int i = 0; i < 100; ++i)
        {
            bp.insert(std::make_pair(rand(), i));
            bp.insert(std::make_pair(rand(), i));
            sb1.insert(std::make_pair(rand(), i));
        }
        bp.rank(bp.begin() + 2);
        sb1 = bp;
        bpptree_multimap<int, int> sb2 = bp;
        assert(bp.size() == sb1.size());
        assert(bp.size() == sb2.size());
        assert(sb1.rbegin()->second == (--sb1.end())->second);
        typedef decltype(sb1.rbegin()) riter_t;
        riter_t rit(sb1.begin());
        assert(rit.base() == sb1.begin());
        assert(rit == sb1.rend());
        assert(sb2.rbegin() + 10 == sb2.rend() - 190);
        assert(bp.at(100)->second == bp.at(50)[50].second);
        assert(bp.at(74) < bp.at(75));
        assert(bp.at(75) >= bp.at(75));
        bp.clear();
        bp.emplace(0, 1);
        bp.emplace(0, 0);
        bp.emplace(0, 3);
        bp.emplace(0, 4);
        bp.emplace(0, 2);
        assert(bp.at(0)->second == 1);
        assert(bp.at(1)->second == 0);
        assert(bp.at(2)->second == 3);
        assert(bp.at(3)->second == 4);
        assert(bp.at(4)->second == 2);
        assert(bp.erase(0) == 5);
        bp.insert(sb1.rbegin(), sb1.rend());
        assert(bp.get_allocator() == sb2.get_allocator());
        sb1.clear();
        bp.swap(sb1);
    }();

    [&]()
    {
        int length = 2000;

        for(int i = 0; i < length / 2; ++i)
        {
            auto n = std::make_pair(i, i);
            rb.insert(n);
            bp.insert(n);
            n = std::make_pair(i, i);
            rb.emplace(i, i);
            bp.emplace(i, i);
        }
        assert(rb.find(0) == rb.begin());
        assert(bp.find(0) == bp.begin());
        assert(rb.find(length / 2 - 1) == ----rb.end());
        assert(bp.find(length / 2 - 1) == bp.end() - 2);
        assert(rb.count(1) == 2);
        assert(bp.count(1) == 2);
        assert(bp.count(1, 2) == 4);
        assert(bp.count(1, 3) == 6);
        assert(bp.range(1, 3) == std::make_pair(bp.find(1), bp.find(4)));
        assert(bp.range(0, 2) == std::make_pair(bp.begin(), bp.begin() + 6));
        assert(bp.range(2, 3) == bp.slice(4, 8));
        assert(bp.range(0, length) == bp.slice());
        assert(bp.front().second == bp.begin()->second);
        assert(bp.front().second == (--bp.rend())->second);
        assert(bp.back().second == (--bp.end())->second);
        assert(bp.back().second == bp.rbegin()->second);
        assert(bp.rank(0) == 2);
        assert(bp.rank(1) == 4);
        assert(bp.rank(length) == bp.size());
        assert(bp.rank(length / 2) == bp.size());
        assert(bp.rank(length / 2 - 1) == bp.size());
        assert(bp.rank(length / 2 - 2) == bp.size() - 2);
        assert(rb.equal_range(2).first == rb.lower_bound(2));
        assert(bp.equal_range(2).second == bp.upper_bound(2));
        assert(bp.erase(3) == 2);
        assert(rb.erase(3) == 2);
        for(int i = 0; i < length / 2; ++i)
        {
            auto it_rb = rb.begin();
            auto it_sb = bp.begin();
            std::advance(it_rb, rand() % rb.size());
            std::advance(it_sb, rand() % bp.size());
            rb.erase(it_rb);
            bp.erase(it_sb);
        }
        for(int i = 0; i < length * 2 + 2; ++i)
        {
            auto n = std::make_pair(rand(), rand());
            rb.insert(n);
            bp.insert(n);
        }
        for(int i = 0; i < length; ++i)
        {
            decltype(bp) const &csb = bp;
            typedef decltype(csb.begin()) iter_t;
            int off = rand() % csb.size();
            iter_t it = csb.at(off);
            assert(it - csb.begin() == off);
            assert(it - off == csb.begin());
            assert(csb.begin() + off == it);
            assert(csb.begin() + off == csb.end() - (csb.size() - off));
            iter_t begin = csb.begin(), end = csb.end();
            for(int i = 0; i < off; ++i)
            {
                --it;
                ++begin;
                --end;
            }
            assert(csb.end() - end == off);
            assert(csb.begin() + off == begin);
            assert(csb.begin() == it);
            size_t part = csb.size() / 4;
            size_t a = part + rand() % (part * 2);
            size_t b = rand() % part;
            assert(csb.at(a) + b == csb.at(a + b));
            assert(csb.begin() + a == csb.at(a + b) - b);
            assert(csb.at(a) - csb.at(b) == a - b);
        }
        for(int i = 0; i < length * 2 + length / 2; ++i)
        {
            auto it_rb = rb.rbegin();
            auto it_sb = bp.rbegin();
            std::advance(it_rb, rand() % rb.size());
            std::advance(it_sb, rand() % bp.size());
            rb.erase(--(it_rb.base()));
            bp.erase(--(it_sb.base()));
        }
    }();

    [&]()
    {
        bpptree_multimap<int, int> bp;
        for(int i = 0; i < 10000; ++i)
        {
            int key = rand();
            int val = rand();
            int where = rand() % std::max<size_t>(1, bp.size() + 1);
            bp.emplace_hint(std::next(bp.begin(), where), key, val);
            rb.emplace_hint(std::next(rb.begin(), where), key, val);
            auto sit = bp.begin();
            auto rit = rb.begin();
            for(int j = 0; j < int(bp.size()); ++j, ++sit, ++rit)
            {
                assert(sit->second == rit->second);
            }
        }
        bp.clear();
        rb.clear();
    }();

    auto t = std::chrono::high_resolution_clock::now;
    std::mt19937 mt(0);
    auto mtr = std::uniform_int_distribution<int>(-10000000, 0);

    std::vector<int> v;
    v.resize(20000000);
    auto reset = [&mtr, &mt, &v]()
    {
        for(auto &value : v)
        {
            value = mtr(mt);
        }
    };

    auto testbp = [&mtr, &mt, &c = bp, &v]()
    {
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.insert(std::make_pair(v[i], i));
        }
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.insert(std::make_pair(i, i));
        }
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.erase(v[i]);
        }
        c.clear();
    };
    auto testrb = [&mtr, &mt, &c = rb, &v]()
    {
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.insert(std::make_pair(v[i], i));
        }
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.insert(std::make_pair(i, i));
        }
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.erase(v[i]);
        }
        c.clear();
    };
    reset();
    auto bs1 = t();
    testbp();
    auto be1 = t();
    auto rs1 = t();
    testrb();
    auto re1 = t();
    reset();
    auto bs2 = t();
    testbp();
    auto be2 = t();
    auto rs2 = t();
    testrb();
    auto re2 = t();
    reset();
    auto bs3 = t();
    testbp();
    auto be3 = t();
    auto rs3 = t();
    testrb();
    auto re3 = t();

    v.clear();

    std::cout
        << "bp time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(be1 - bs1).count() << std::endl
        << "rb time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re1 - rs1).count() << std::endl
        << "bp time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(be2 - bs2).count() << std::endl
        << "rb time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re2 - rs2).count() << std::endl
        << "bp time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(be3 - bs3).count() << std::endl
        << "rb time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re3 - rs3).count() << std::endl
        ;

    system("pause");

    //for(int i = 0; i < 20000000; ++i)
    //{
    //    bp.insert(std::make_pair(mtr(mt), i));
    //}
    //bp.print_tree();
}