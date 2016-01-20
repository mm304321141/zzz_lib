
#define _SCL_SECURE_NO_WARNINGS

#include "split_iterator.h"
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
    []
    {
        std::string str = "1234,5678.9012 3456";
        for(auto item : make_split_any_of(str, ",. "))
        {
            if(item != "1234")
            {
                std::cout << item.to_value<float>() << std::endl;
            }
        }
        for(auto item : make_split(str, "34"))
        {
            std::cout << item.to_value<int>() << std::endl;
        }
        std::cout << string_ref<char>("-999.888888").to_value<double>() << std::endl;
    }();
    []
    {
        std::string str = "/1//23/456///";
        auto it = make_split(str, '/');
        std::vector<std::string> token;
        std::copy(it, it.end(), std::back_inserter(token));
        for(auto item : token)
        {
            std::cout << item << std::endl;
        }
    }();

    system("pause");
}