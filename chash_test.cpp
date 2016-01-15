
#define _SCL_SECURE_NO_WARNINGS

#include "chash_map.h"
#include "chash_set.h"

#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <string>


#define assert(exp) assert_proc(exp, #exp, __FILE__, __LINE__)

auto assert_proc = [](bool no_error, char const *query, char const *file, size_t line)
{
    if(!no_error)
    {
        struct hasher
        {
            size_t operator()(std::tuple<char const *, char const *, size_t> const &ref) const
            {
                return
                    std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(std::get<0>(ref))) ^
                    std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(std::get<1>(ref))) ^
                    std::hash<size_t>()(std::get<2>(ref));
            }
        };
        static chash_set<std::tuple<char const *, char const *, size_t>, hasher> check;
        if(check.emplace(query, file, line).second)
        {
            printf("%s(%zd):%s\n", file, line, query);
        }
    }
};

int main()
{
    std::unordered_map<int, int> xh;
    chash_map<int, int> ch;

    auto t = std::chrono::high_resolution_clock::now;
    std::mt19937 mt(0);
    auto mtr = std::uniform_int_distribution<int>(-10000000, 0);

    std::vector<int> v;
    v.resize(10000000);
    auto reset = [&mtr, &mt, &v]()
    {
        for(auto &value : v)
        {
            value = mtr(mt);
        }
    };
    assert(false);

    auto testch = [&mtr, &mt, &c = ch, &v]()
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
    auto testxh = [&mtr, &mt, &c = xh, &v]()
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
    xh.max_load_factor(1);
    ch.max_load_factor(1);
    auto cs1 = t();
    testch();
    auto ce1 = t();
    std::cout << "ch time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(ce1 - cs1).count() << std::endl;
    auto xs1 = t();
    testxh();
    auto xr1 = t();
    std::cout << "xh time 1(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(xr1 - xs1).count() << std::endl;
    reset();
    xh.max_load_factor(0.8f);
    ch.max_load_factor(0.8f);
    auto cs2 = t();
    testch();
    auto ce2 = t();
    std::cout << "ch time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(ce2 - cs2).count() << std::endl;
    auto xs2 = t();
    testxh();
    auto xr2 = t();
    std::cout << "xh time 2(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(xr2 - xs2).count() << std::endl;
    reset();
    xh.max_load_factor(0.5f);
    ch.max_load_factor(0.5f);
    auto cs3 = t();
    testch();
    auto ce3 = t();
    std::cout << "ch time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(ce3 - cs3).count() << std::endl;
    auto xs3 = t();
    testxh();
    auto xr3 = t();
    std::cout << "xh time 3(ms) = " << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(xr3 - xs3).count() << std::endl;

    v.clear();

    system("pause");

    //for(int i = 0; i < 20000000; ++i)
    //{
    //    ch.insert(std::make_pair(mtr(mt), i));
    //}
    //ch.print_tree();
}