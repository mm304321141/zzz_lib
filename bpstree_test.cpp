
#define _SCL_SECURE_NO_WARNINGS

#include "bpstree.h"

#include <chrono>
#include <iostream>
#include <random>
#include <map>
#include <cstring>
#include <string>


auto assert = [](bool no_error)
{
    if(!no_error)
    {
        *static_cast<int *>(0) = 0;
    }
};

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
struct bstree_map_config_t
{
    template<class in_type> static key_t const &get_key(in_type &value)
    {
        return value.first;
    }
    typedef key_t key_type;
    typedef value_t mapped_type;
    typedef std::pair<key_t const, value_t> value_type;
    typedef std::pair<key_t, value_t> storage_type;
    typedef comparator_t key_compare;
    typedef allocator_t allocator_type;
    typedef std::true_type unique_t;
    enum
    {
        memory_block_size = 256
    };
};
template<class key_t, class comparator_t = std::less<key_t>, class allocator_t = std::allocator<key_t>>
struct bstree_set_config_t
{
    template<class in_type> static key_t const &get_key(in_type &value)
    {
        return value;
    }
    typedef key_t key_type;
    typedef key_t const mapped_type;
    typedef key_t const value_type;
    typedef key_t storage_type;
    typedef comparator_t key_compare;
    typedef allocator_t allocator_type;
    typedef std::true_type unique_t;
    enum
    {
        memory_block_size = 256
    };
};

int main()
{
    auto t = std::chrono::high_resolution_clock::now;
    std::mt19937 mt;
    auto mtr = std::uniform_int_distribution<int>(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

    b_plus_size_tree<bstree_set_config_t<std::string>> tree;

    for(int i = 0; ; ++i)
    {
        tree.insert(std::to_string(mtr(mt)));
        if(!tree.debug_check())
        {
            _asm int 3;
        }
    }


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