﻿
#define _SCL_SECURE_NO_WARNINGS

#include "sorted_set.h"

#include <chrono>
#include <iostream>
#include <random>
#include <map>


auto assert = [](bool no_error)
{
    if(!no_error)
    {
        *static_cast<int *>(0) = 0;
    }
};

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
class sorted_set_test : public sorted_set<key_t, value_t, comparator_t, allocator_t>
{
protected:
    typedef sorted_set<key_t, value_t, comparator_t, allocator_t> b_t;

    void print_tree(typename b_t::node_t *node, size_t level, std::string head, std::string with, int type)
    {
        if(!b_t::is_nil_(node))
        {
            if(b_t::get_size_(node) != b_t::get_size_(b_t::get_left_(node)) + b_t::get_size_(b_t::get_right_(node)) + 1)
            {
                assert(false);
            }
            std::string fork =
                !b_t::is_nil_(b_t::get_left_(node)) && !b_t::is_nil_(b_t::get_right_(node)) ? "┫" :
                b_t::is_nil_(b_t::get_left_(node)) && b_t::is_nil_(b_t::get_right_(node)) ? "* " :
                !b_t::is_nil_(b_t::get_right_(node)) ? "┛" : "┓";
            std::string next_left = type == 0 ? "" : type == 1 ? "┃" : "  ";
            std::string next_right = type == 0 ? "" : type == 1 ? "  " : "┃";
            print_tree(b_t::get_right_(node), level + 1, head + next_right, "┏", 1);
            printf("%s%d\n", (head + with + fork).c_str(), b_t::rank(typename b_t::iterator(node)));
            print_tree(b_t::get_left_(node), level + 1, head + next_left, "┗", 2);
        }
    }
public:
    void print_tree()
    {
        printf("\n\n\n\n\n");
        print_tree(b_t::get_root_(), 0, "  ", "", 0);
    }
};

int main()
{
    std::multimap<int, int> rb;
    sorted_set_test<int, int> sb;
    
    [&]()
    {
        sorted_set_test<int, int> sb1;
        assert(sb.size() == sb1.size());
        for(int i = 0; i < 100; ++i)
        {
            sb.insert(std::make_pair(rand(), i));
            sb.insert(std::make_pair(rand(), i));
            sb1.insert(std::make_pair(rand(), i));
        }
        sb1 = sb;
        sorted_set_test<int, int> sb2 = sb;
        assert(sb.size() == sb1.size());
        assert(sb.size() == sb2.size());
        assert(sb1.rbegin()->second == (--sb1.end())->second);
        typedef decltype(sb1.rbegin()) riter_t;
        riter_t rit(sb1.begin());
        assert(rit.base() == sb1.begin());
        assert(rit == sb1.rend());
        assert(sb2.rbegin() + 10 == sb2.rend() - 190);
        sb.clear();
        sb.insert(sb1.rbegin(), sb1.rend());
        assert(sb.get_allocator() == sb2.get_allocator());
        sb1.clear();
        sb.swap(sb1);
    }();

    int length = 2000;

    for(int i = 0; i < length / 2; ++i)
    {
        auto n = std::make_pair(i, i);
        rb.insert(n);
        sb.insert(n);
        n = std::make_pair(i, i);
        rb.emplace(i, i);
        sb.emplace(i, i);
    }
    assert(rb.find(0) == rb.begin());
    assert(sb.find(0) == sb.begin());
    assert(rb.find(length / 2 - 1) == ----rb.end());
    assert(sb.find(length / 2 - 1) == sb.end() - 2);
    assert(rb.count(1) == 2);
    assert(sb.count(1) == 2);
    assert(sb.count(1, 2) == 4);
    assert(sb.count(1, 3) == 6);
    assert(sb.range(1, 3) == std::make_pair(sb.find(1), sb.find(4)));
    assert(sb.range(0, 2) == std::make_pair(sb.begin(), sb.begin() + 6));
    assert(sb.range(2, 3) == sb.slice(4, 8));
    assert(sb.range(0, length) == sb.slice());
    assert(sb.front().second == (*sb.begin()).second);
    assert(sb.back().second == (*--sb.end()).second);
    assert(sb.rank(0) == 2);
    assert(sb.rank(1) == 4);
    assert(sb.rank(length) == sb.size());
    assert(sb.rank(length / 2) == sb.size());
    assert(sb.rank(length / 2 - 1) == sb.size());
    assert(sb.rank(length / 2 - 2) == sb.size() - 2);
    assert(rb.equal_range(2).first == rb.lower_bound(2));
    assert(sb.equal_range(2).second == sb.upper_bound(2));
    assert(sb.erase(3) == 2);
    assert(rb.erase(3) == 2);
    for(int i = 0; i < length / 2; ++i)
    {
        auto it_rb = rb.begin();
        auto it_sb = sb.begin();
        std::advance(it_rb, rand() % rb.size());
        std::advance(it_sb, rand() % sb.size());
        rb.erase(it_rb);
        sb.erase(it_sb);
    }
    for(int i = 0; i < length * 2 + 2; ++i)
    {
        auto n = std::make_pair(rand(), rand());
        rb.insert(n);
        sb.insert(n);
    }
    for(int i = 0; i < length; ++i)
    {
        decltype(sb) const &csb = sb;
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
        int part = csb.size() / 4;
        int a = part + rand() % (part * 2);
        int b = rand() % part;
        assert(csb.at(a) + b == csb.at(a + b));
        assert(csb.begin() + a == csb.at(a + b) - b);
        assert(csb.at(a) - csb.at(b) == a - b);
    }

    for(int i = 0; i < length * 2 + length / 2; ++i)
    {
        auto it_rb = rb.rbegin();
        auto it_sb = sb.rbegin();
        std::advance(it_rb, rand() % rb.size());
        std::advance(it_sb, rand() % sb.size());
        rb.erase(--(it_rb.base()));
        sb.erase(--(it_sb.base()));
    }
    

    //for(int i = 0; ; ++i)
    //{
    //    sb.insert(std::make_pair(rand(), i));
    //    sb.print_tree();
    //    system("pause");
    //}


    auto t = std::chrono::high_resolution_clock::now;
    std::mt19937 mt;
    auto mtr = std::uniform_int_distribution<int>(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

    auto testsb = [&mtr, &mt, &c = sb]()
    {
        for(int i = 0; i < 20000000; ++i)
        {
            c.insert(std::make_pair(mtr(mt), i));
        }
        c.clear();
    };
    auto testrb = [&mtr, &mt, &c = rb]()
    {
        for(int i = 0; i < 20000000; ++i)
        {
            c.insert(std::make_pair(mtr(mt), i));
        }
        c.clear();
    };

    auto ss1 = t();
    testsb();
    auto se1 = t();
    auto rs1 = t();
    testrb();
    auto re1 = t();
    auto ss2 = t();
    testsb();
    auto se2 = t();
    auto rs2 = t();
    testrb();
    auto re2 = t();
    auto ss3 = t();
    testsb();
    auto se3 = t();
    auto rs3 = t();
    testrb();
    auto re3 = t();


    rb.clear();

    std::cout
        << "sb time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(se1 - ss1).count() << std::endl
        << "rb time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re1 - rs1).count() << std::endl
        << "sb time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(se2 - ss2).count() << std::endl
        << "rb time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re2 - rs2).count() << std::endl
        << "sb time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(se3 - ss3).count() << std::endl
        << "rb time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re3 - rs3).count() << std::endl
        ;

    system("pause");

    //for(int i = 0; i < 20000000; ++i)
    //{
    //    sb.insert(std::make_pair(mtr(mt), i));
    //}
    //sb.print_tree();
}