
#define _SCL_SECURE_NO_WARNINGS

#include "sorted_set.h"

#include <chrono>
#include <iostream>
#include <vector>
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
            printf("%s%d\n", (head + with + fork).c_str(), b_t::rank(typename b_t::iterator(node, this)));
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

    sb.size();

    int length = 2000;

    for(int i = 0; i < length / 2; ++i)
    {
        auto n = std::make_pair(i, i);
        rb.insert(n);
        sb.insert(n);
        n = std::make_pair(i, i);
        rb.insert(n);
        sb.insert(n);
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
        typedef decltype(sb.begin()) iter_t;
        int off = rand() % sb.size();
        iter_t it = sb.at(off);
        assert(it - sb.begin() == off);
        assert(it - off == sb.begin());
        assert(sb.begin() + off == it);
        assert(sb.begin() + off == sb.end() - (sb.size() - off));
        iter_t begin = sb.begin(), end = sb.end();
        for(int i = 0; i < off; ++i)
        {
            --it;
            ++begin;
            --end;
        }
        assert(sb.end() - end == off);
        assert(sb.begin() + off == begin);
        assert(sb.begin() == it);
        int part = sb.size() / 4;
        int a = part + rand() % (part * 2);
        int b = rand() % part;
        assert(sb.at(a) + b == sb.at(a + b));
        assert(sb.begin() + a == sb.at(a + b) - b);
        assert(sb.at(a) - sb.at(b) == a - b);
    }

    for(int i = 0; i < length * 2 + length / 2; ++i)
    {
        auto it_rb = rb.begin();
        auto it_sb = sb.begin();
        std::advance(it_rb, rand() % rb.size());
        std::advance(it_sb, rand() % sb.size());
        rb.erase(it_rb);
        sb.erase(it_sb);
    }

    //for(int i = 0; ; ++i)
    //{
    //    sb.insert(std::make_pair(rand(), i));
    //    sb.print_tree();
    //    system("pause");
    //}

    system("pause");

    sb.clear();

    for(int i = 0; i < 20000000; ++i)
    {
        sb.insert(std::make_pair(rand(), i));
    }

    auto t = std::chrono::high_resolution_clock::now;

    auto a1 = t();

    auto c1 = sb.count(1000, 2000);
    auto c2 = sb.at(rand())->second;
    auto c3 = sb.slice(1000, 2000).second->second;

    auto a2 = t();

    std::cout
        << "sb.count(1000, 2000); " << c1 << std::endl
        << "sb.at(rand());        " << c2 << std::endl
        << "sb.slice(1000, 2000); " << c3 << std::endl
        << "time(s) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(a2 - a1).count() << std::endl;

    system("pause");
}