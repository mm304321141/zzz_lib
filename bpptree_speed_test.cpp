
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

int main()
{
    auto t = std::chrono::high_resolution_clock::now;
    std::mt19937 mt(0);
    auto mtr = std::uniform_int_distribution<int>(-10000000, 0);

    std::multimap<int, int> rb;
    bpptree_multimap<int, int> bp;

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
    std::cout << "bp time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(be1 - bs1).count() << std::endl;
    auto rs1 = t();
    testrb();
    auto re1 = t();
    std::cout << "rb time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re1 - rs1).count() << std::endl;
    reset();
    auto bs2 = t();
    testbp();
    auto be2 = t();
    std::cout << "bp time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(be2 - bs2).count() << std::endl;
    auto rs2 = t();
    testrb();
    auto re2 = t();
    std::cout << "rb time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re2 - rs2).count() << std::endl;
    reset();
    auto bs3 = t();
    testbp();
    auto be3 = t();
    std::cout << "bp time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(be3 - bs3).count() << std::endl;
    auto rs3 = t();
    testrb();
    auto re3 = t();
    std::cout << "rb time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(re3 - rs3).count() << std::endl;

    v.clear();
}