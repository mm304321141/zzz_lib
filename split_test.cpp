
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
    auto test_to_real = [](char const *s)
    {
        double l = std::atof(s);
        double r = string_ref<>(s).to_value<double>();
        printf("%f\n%f\n%f\n\n", l, r, (l - r) / l);
    };

    test_to_real("1234567890.1234567890");
    test_to_real("1234567890.1234567890e3");
    test_to_real("-1234567890.1234567890e-3");

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
    }();
    []
    {
        std::string str = "/1//23/456///";
        auto split = make_split(str, '/');
        std::vector<std::string> token;
        std::copy(split.begin(), split.end(), std::back_inserter(token));
        for(auto item : token)
        {
            std::cout << item << std::endl;
        }
    }();
    []
    {
        std::string str = "/1//23/456///";
        auto split = make_split(str, '/');
        for(size_t i = 0; i <= split.size(); ++i)
        {
            std::cout << split[i].to_value<int>() << std::endl;
        }
    }();
    []
    {
        std::string str = "aaa,111,ccc";
        std::string v1;
        int v2;
        string_ref<> v3;
        make_split(str, ',').fill(v1, v2, v3);
    }();
    []
    {
        std::wstring str = L"aaa,111,ccc";
        std::wstring v1;
        int v2;
        string_ref<wchar_t> v3;
        make_split(str, L',').fill(v1, v2, v3);
    }();

    system("pause");
}