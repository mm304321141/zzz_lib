
#define _SCL_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING

#include "split_iterator.h"
#include "chash_set.h"

#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <string>
#include <climits>

#define assert(exp) assert_proc(exp, #exp, __FILE__, __LINE__)

auto assert_proc = [](bool no_error, char const *query, char const *file, size_t line)
{
    if(!no_error)
    {
        struct hasher
        {
            size_t operator()(std::tuple<char const *, char const *, size_t> const &ref) const
            {
                return std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(std::get<0>(ref))) ^
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
        double r = to_value<double>(string_ref<>(s));
        double rel = (l == 0.0) ? 0.0 : (l - r) / l;
        printf("%f\n%f\n%f\n\n", l, r, rel);
    };

    test_to_real("1234567890.1234567890");
    test_to_real("-1234567890.1234567890e3");
    test_to_real("1234567890.1234567890e-3");
    test_to_real("1234567890123456789012345678901234567890.1234567890");
    test_to_real("-1234567890123456789012345678901234567890.1234567890e3");
    test_to_real("1234567890123456789012345678901234567890.1234567890e-3");
    test_to_real("0.000000000000000000000000000000000000123456789012345678901234567890");
    test_to_real("-0.000000000000000000000000000000000000123456789012345678901234567890e3");
    test_to_real("0.000000000000000000000000000000000000123456789012345678901234567890e-3");

    []
    {
        std::string str = "1234,5678.9012 3456";
        for(auto item : make_split_any_of(str, ",. "))
        {
            if(item != "1234")
            {
                std::cout << to_value<float>(item) << std::endl;
            }
        }
        for(auto item : make_split(str, "34"))
        {
            // Tokens may contain non-numeric characters; the new free-function
            // to_value<int> reports that via std::invalid_argument, swallow
            // it here so the demo loop keeps printing.
            try
            {
                std::cout << to_value<int>(item) << std::endl;
            }
            catch(std::exception const &)
            {
                std::cout << 0 << std::endl;
            }
        }
    }();
    []
    {
        std::string str = "/1//23/456///";
        auto split = make_split(str, '/');
        std::vector<std::string> token;
        // std::copy through back_inserter requires implicit construction of
        // std::string from the iterator's reference; std::string_view's
        // string constructor is explicit (C++17), so build the strings
        // manually instead.
        for(auto item : split)
        {
            token.emplace_back(item.data(), item.size());
        }
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
            // to_value<int>() on an empty field throws invalid_argument under
            // the new free-function API; guard explicitly so the demo loop
            // still prints a placeholder for empty / out-of-range fields.
            auto field = split[i];
            if(field.empty())
            {
                std::cout << 0 << std::endl;
            }
            else
            {
                std::cout << to_value<int>(field) << std::endl;
            }
        }
    }();
    []
    {
        auto str = "aaa,111,ccc";
        std::string v1;
        int v2 = 0;
        string_ref<> v3;
        make_split(string_ref<>(str), ',').fill(v1, v2, v3);
        assert(v1 == "aaa");
        assert(v2 == 111);
        assert(v3 == "ccc");
    }();
    []
    {
        std::wstring str = L"aaa,111,ccc";
        std::wstring v1;
        float v2 = 0.0f;
        string_ref<wchar_t> v3;
        make_split(str, L',').fill(v1, v2, v3);
        assert(v1 == L"aaa");
        assert(v2 == 111.0f);
        assert(v3 == L"ccc");
    }();

    // Unsigned wrap-around (matches std::stoul semantics) — both the
    // C++11/14 fallback and the C++17 std::from_chars path must agree.
    {
        unsigned u1 = to_value<unsigned>(string_ref<>("-5"));
        assert(u1 == UINT_MAX - 4u);
        unsigned long long u2 = to_value<unsigned long long>(string_ref<>("-1"));
        assert(u2 == ~0ULL);
        unsigned u3 = to_value<unsigned>(string_ref<>("0"));
        assert(u3 == 0u);
        unsigned u4 = to_value<unsigned>(string_ref<>("42"));
        assert(u4 == 42u);
        // Plain "+" sign still works on the signed-stripping path.
        int i1 = to_value<int>(string_ref<>("+7"));
        assert(i1 == 7);
        int i2 = to_value<int>(string_ref<>("-7"));
        assert(i2 == -7);
    }

    return 0;
}
