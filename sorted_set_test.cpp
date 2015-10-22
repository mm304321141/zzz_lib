
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
            printf("%s%zd\n", (head + with + fork).c_str(), b_t::rank(typename b_t::iterator(node)));
            print_tree(b_t::get_left_(node), level + 1, head + next_left, "┗", 2);
        }
    }

    uint64_t calc_depth(node_t *node, size_t level)
    {
        return level
            + (b_t::is_nil_(b_t::get_left_(node)) ? 0 : calc_depth(b_t::get_left_(node), level + 1))
            + (b_t::is_nil_(b_t::get_right_(node)) ? 0 : calc_depth(b_t::get_right_(node), level + 1))
            ;
    }
    double calc_diff(double avg, node_t *node, size_t level)
    {
        return std::abs(double(level) - avg)
            + (b_t::is_nil_(b_t::get_left_(node)) ? 0 : calc_diff(avg, b_t::get_left_(node), level + 1))
            + (b_t::is_nil_(b_t::get_right_(node)) ? 0 : calc_diff(avg, b_t::get_right_(node), level + 1))
            ;
    }
public:
    void print_tree(bool body = true)
    {
        if(body)
        {
            print_tree(b_t::get_root_(), 0, "  ", "", 0);
        }
        if(!b_t::empty())
        {
            double avg = double(calc_depth(get_root_(), 0)) / double(b_t::size());
            double diff = calc_diff(avg, get_root_(), 0) / double(b_t::size());
            printf("avg = %f\t diff = %f\n", avg, diff);
        }
    }
};

int main()
{
    std::multimap<int, int> rb;
    sorted_set_test<int, int> sb;
    
    while(true)
    {
        sb.print_tree();
        system("pause");
        if(sb.size() < 48)
        {
            sb.emplace(rand(), 0);
        }
        else if(sb.size() >= 64)
        {
            sb.erase(sb.at(rand() % sb.size()));
        }
        else
        {
            int r = rand() % 100;
            if(rand() % 100 < 50)
            {
                sb.emplace(rand(), 0);
            }
            else if(r < 55)
            {
                sb.erase(sb.at(rand() % sb.size()));
            }
            else
            {
                sb.erase(sb.at(rand() % (sb.size() / 4 + 1)));
            }
        }
    }

    [&]()
    {
        sorted_set_test<std::string, std::string> sss;
        sss.emplace("0", "0");
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
        sorted_set_test<int, int> sb1;
        assert(sb.size() == sb1.size());
        for(int i = 0; i < 100; ++i)
        {
            sb.insert(std::make_pair(rand(), i));
            sb.insert(std::make_pair(rand(), i));
            sb1.insert(std::make_pair(rand(), i));
        }
        sb.rank(sb.begin() + 2);
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
        sb.emplace(0, 1);
        sb.emplace(0, 0);
        sb.emplace(0, 3);
        sb.emplace(0, 4);
        sb.emplace(0, 2);
        assert(sb.at(0)->second == 1);
        assert(sb.at(1)->second == 0);
        assert(sb.at(2)->second == 3);
        assert(sb.at(3)->second == 4);
        assert(sb.at(4)->second == 2);
        assert(sb.erase(0) == 5);
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
    assert(sb.front().second == sb.begin()->second);
    assert(sb.front().second == (--sb.rend())->second);
    assert(sb.back().second == (--sb.end())->second);
    assert(sb.back().second == sb.rbegin()->second);
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
        auto it_sb = sb.rbegin();
        std::advance(it_rb, rand() % rb.size());
        std::advance(it_sb, rand() % sb.size());
        rb.erase(--(it_rb.base()));
        sb.erase(--(it_sb.base()));
    }

    auto t = std::chrono::high_resolution_clock::now;
    std::mt19937 mt;
    auto mtr = std::uniform_int_distribution<int>(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

    std::vector<int> v;
    v.resize(20000000);
    auto reset = [&mtr, &mt, &v]()
    {
        for(auto &value : v)
        {
            value = mtr(mt);
        }
    };

    auto testsb = [&mtr, &mt, &c = sb, &v]()
    {
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.insert(std::make_pair(i, i));
        }
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.insert(std::make_pair(v[i], i));
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
            c.insert(std::make_pair(i, i));
        }
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.insert(std::make_pair(v[i], i));
        }
        for(int i = 0; i < int(v.size()); ++i)
        {
            c.erase(v[i]);
        }
        c.clear();
    };
    reset();
    auto ss1 = t();
    testsb();
    auto se1 = t();
    auto rs1 = t();
    testrb();
    auto re1 = t();
    reset();
    auto ss2 = t();
    testsb();
    auto se2 = t();
    auto rs2 = t();
    testrb();
    auto re2 = t();
    reset();
    auto ss3 = t();
    testsb();
    auto se3 = t();
    auto rs3 = t();
    testrb();
    auto re3 = t();

    v.clear();

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