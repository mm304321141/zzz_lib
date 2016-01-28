
#define _SCL_SECURE_NO_WARNINGS

#include "bpptree_set.h"
#include "chash_set.h"
#include "sbtree_set.h"
#include <unordered_set>
#include <set>

#include <chrono>
#include <iostream>
#include <random>
#include <cstring>
#include <string>
#include <functional>

template<size_t key_size>
struct key
{
    union
    {
        int32_t value;
        char data[key_size];
    };
    key() = default;
    key(key const &) = default;
    key(int32_t v) : value(v)
    {
    }
    operator int32_t() const
    {
        return value;
    }
    key &operator = (key const &) = default;
    bool operator < (key const &f) const
    {
        return value < f.value;
    }
};

template<size_t N> struct std::hash<key<N>>
{
    size_t operator()(key<N> const &key) const
    {
        return std::hash<int32_t>()(key.value);
    }
};

std::vector<size_t> make_test(size_t size)
{
    std::vector<size_t> ret;
    for(size_t i = 5; i < 22; ++i)
    {
        ret.push_back(size_t(std::pow(size, 1.0 / 22 * (i + 1))));
    }
    return ret;
}

template<class T, class C, size_t N>
struct test_core
{
    test_core(std::string const &_info, T const *_data[], size_t _length, std::function<void(C &, T const *, size_t)> _prepare, std::function<size_t(C &, T const *, size_t)> _run)
        : info(_info), data(_data), length(_length), prepare(_prepare), run(_run)
    {
    }
    std::string info;
    T const **data;
    size_t length;
    std::function<void(C &, T const *, size_t)> prepare;
    std::function<size_t(C &, T const *, size_t)> run;
    struct time_point_t
    {
        std::chrono::high_resolution_clock::time_point begin;
        std::chrono::high_resolution_clock::time_point end;
    };
    std::vector<time_point_t> time[N];

    void exec()
    {
        auto count = make_test(length);
        for(size_t i = 0; i < N; ++i)
        {
            for(size_t max : count)
            {
                time_point_t t;
                size_t end = std::min(max, length);
                C c;
                prepare(c, data[i], end);
                t.begin = std::chrono::high_resolution_clock::now();
                run(c, data[i], end);
                t.end = std::chrono::high_resolution_clock::now();
                time[i].push_back(t);
            }
        }
    }
};

template<class T, class C, size_t N>
class test_all
{
public:
    test_all(std::string const &_info, T const *_data[N], size_t _length)
        : info(_info), data(_data), length(_length)
    {
        test.emplace_back("insert_o", data, length,
                          [](C &c, T const *, size_t e)
        {
        },
                          [](C &c, T const *d, size_t e)
        {
            for(size_t i = 0; i < e; ++i)
            {
                c.emplace(int32_t(i));
            }
            return 0;
        });
        test.emplace_back("insert_r", data, length,
                          [](C &c, T const *, size_t e)
        {
        },
                          [](C &c, T const *d, size_t e)
        {
            for(size_t i = 0; i < e; ++i)
            {
                c.emplace(d[i]);
            }
            return 0;
        });
        test.emplace_back("foreach", data, length,
                          [](C &c, T const *d, size_t e)
        {
            for(size_t i = 0; i < e; ++i)
            {
                c.emplace(d[i]);
            }
        },
                          [](C &c, T const *d, size_t e)
        {
            size_t sum = 0;
            for(auto &item : c)
            {
                sum += item;
            }
            return sum;
        });
        test.emplace_back("find", data, length,
                          [](C &c, T const *d, size_t e)
        {
            for(size_t i = 0; i < e; ++i)
            {
                c.emplace(d[i]);
            }
        },
                          [](C &c, T const *d, size_t e)
        {
            size_t sum = 0;
            for(size_t i = 0; i < e; ++i)
            {
                sum += *c.find(d[i]);
            }
            return sum;
        });
        test.emplace_back("erase", data, length,
                          [](C &c, T const *d, size_t e)
        {
            for(size_t i = 0; i < e; ++i)
            {
                c.emplace(d[i]);
            }
        },
                          [](C &c, T const *d, size_t e)
        {
            for(size_t i = 0; i < e; ++i)
            {
                c.erase(d[i]);
            }
            return 0;
        });
        empty_call = [](C &c, T const *d, size_t e)
        {
            return 0;
        };
    }

    void exec()
    {
        auto test_empty_call = [&]
        {
            C c;
            auto begin = std::chrono::high_resolution_clock::now();
            empty_call(c, data[0], 0);
            auto end = std::chrono::high_resolution_clock::now();
            return end - begin;
        };
        std::chrono::high_resolution_clock::duration waste = std::chrono::high_resolution_clock::duration();
        for(size_t i = 1; i < 100; ++i)
        {
            waste += test_empty_call();
        }
        waste /= 100;

        for(auto &item : test)
        {
            std::cout << info << ", ";
            std::cout << item.info << ", ";
            item.exec();
            std::vector<std::chrono::high_resolution_clock::duration> time_avg;
            auto count = make_test(length);
            time_avg.resize(count.size());
            for(size_t i = 1; i < N; ++i)
            {
                for(size_t j = 0; j < item.time[i].size(); ++j)
                {
                    time_avg[j] += std::max(std::chrono::high_resolution_clock::duration(), (item.time[i][j].end - item.time[i][j].begin) - waste);
                }
            }
            for(size_t i = 0; i < time_avg.size(); ++i)
            {
                std::cout << std::chrono::duration_cast<std::chrono::duration<float, std::nano>>(time_avg[i]).count() / count[i] / N << ", ";
            }
            std::cout << std::endl;
        }
    }

    std::vector<test_core<T, C, N>> test;
    std::function<size_t(C &, T const *, size_t)> empty_call;

private:
    std::string info;
    T const **data;
    size_t length;
};

int main()
{
    typedef std::set<key<4  >>           std_set_4      ;
    typedef std::set<key<32 >>           std_set_32     ;
    typedef std::set<key<64 >>           std_set_64     ;
    typedef std::set<key<128>>           std_set_128    ;
    typedef std::set<key<200>>           std_set_200    ;
    typedef std::unordered_set<key<4  >> std_hash_4     ;
    typedef std::unordered_set<key<32 >> std_hash_32    ;
    typedef std::unordered_set<key<64 >> std_hash_64    ;
    typedef std::unordered_set<key<128>> std_hash_128   ;
    typedef std::unordered_set<key<200>> std_hash_200   ;
    typedef chash_set<key<4  >>          chash_set_4    ;
    typedef chash_set<key<32 >>          chash_set_32   ;
    typedef chash_set<key<64 >>          chash_set_64   ;
    typedef chash_set<key<128>>          chash_set_128  ;
    typedef chash_set<key<200>>          chash_set_200  ;
    typedef bpptree_set<key<4  >>        bpptree_set_4  ;
    typedef bpptree_set<key<32 >>        bpptree_set_32 ;
    typedef bpptree_set<key<64 >>        bpptree_set_64 ;
    typedef bpptree_set<key<128>>        bpptree_set_128;
    typedef bpptree_set<key<200>>        bpptree_set_200;

    
    typedef std::multiset<key<4  >>           std_mset_4      ;
    typedef std::multiset<key<32 >>           std_mset_32     ;
    typedef std::multiset<key<64 >>           std_mset_64     ;
    typedef std::multiset<key<128>>           std_mset_128    ;
    typedef std::multiset<key<200>>           std_mset_200    ;
    typedef std::unordered_multiset<key<4  >> std_mhash_4     ;
    typedef std::unordered_multiset<key<32 >> std_mhash_32    ;
    typedef std::unordered_multiset<key<64 >> std_mhash_64    ;
    typedef std::unordered_multiset<key<128>> std_mhash_128   ;
    typedef std::unordered_multiset<key<200>> std_mhash_200   ;
    typedef chash_multiset<key<4  >>          chash_mset_4    ;
    typedef chash_multiset<key<32 >>          chash_mset_32   ;
    typedef chash_multiset<key<64 >>          chash_mset_64   ;
    typedef chash_multiset<key<128>>          chash_mset_128  ;
    typedef chash_multiset<key<200>>          chash_mset_200  ;
    typedef sbtree_multiset<key<4  >>         sbtree_mset_4   ;
    typedef sbtree_multiset<key<32 >>         sbtree_mset_32  ;
    typedef sbtree_multiset<key<64 >>         sbtree_mset_64  ;
    typedef sbtree_multiset<key<128>>         sbtree_mset_128 ;
    typedef sbtree_multiset<key<200>>         sbtree_mset_200 ;
    typedef bpptree_multiset<key<4  >>        bpptree_mset_4  ;
    typedef bpptree_multiset<key<32 >>        bpptree_mset_32 ;
    typedef bpptree_multiset<key<64 >>        bpptree_mset_64 ;
    typedef bpptree_multiset<key<128>>        bpptree_mset_128;
    typedef bpptree_multiset<key<200>>        bpptree_mset_200;


    std::mt19937 mt(0);
    auto mtr = std::uniform_int_distribution<int>(-200000000, 200000000);
    size_t count = 409600;
    static constexpr size_t times = 5;
    std::vector<int> v_arr[times];

    for(auto &v : v_arr)
    {
        v.resize(count);
        for(auto &value : v)
        {
            value = mtr(mt);
        }
    }
    int const *v[times];
    for(size_t i = 0; i < times; ++i)
    {
        v[i] = v_arr[i].data();
    }

    std::cout << "container      , method, ";
    for(auto item : make_test(count))
    {
        std::cout << item << ", ";
    }
    std::cout << std::endl;

    test_all<int, std_set_4      , times>("std_set_4      ", v, count).exec();
    test_all<int, std_hash_4     , times>("std_hash_4     ", v, count).exec();
    test_all<int, chash_set_4    , times>("chash_set_4    ", v, count).exec();
    test_all<int, bpptree_set_4  , times>("bpptree_set_4  ", v, count).exec();
    test_all<int, std_set_32     , times>("std_set_32     ", v, count).exec();
    test_all<int, std_hash_32    , times>("std_hash_32    ", v, count).exec();
    test_all<int, chash_set_32   , times>("chash_set_32   ", v, count).exec();
    test_all<int, bpptree_set_32 , times>("bpptree_set_32 ", v, count).exec();
    test_all<int, std_set_64     , times>("std_set_64     ", v, count).exec();
    test_all<int, std_hash_64    , times>("std_hash_64    ", v, count).exec();
    test_all<int, chash_set_64   , times>("chash_set_64   ", v, count).exec();
    test_all<int, bpptree_set_64 , times>("bpptree_set_64 ", v, count).exec();
    test_all<int, std_set_128    , times>("std_set_128    ", v, count).exec();
    test_all<int, std_hash_128   , times>("std_hash_128   ", v, count).exec();
    test_all<int, chash_set_128  , times>("chash_set_128  ", v, count).exec();
    test_all<int, bpptree_set_128, times>("bpptree_set_128", v, count).exec();
    test_all<int, std_set_200    , times>("std_set_200    ", v, count).exec();
    test_all<int, std_hash_200   , times>("std_hash_200   ", v, count).exec();
    test_all<int, chash_set_200  , times>("chash_set_200  ", v, count).exec();
    test_all<int, bpptree_set_200, times>("bpptree_set_200", v, count).exec();

    test_all<int, std_mset_4      , times>("std_mset_4      ", v, count).exec();
    test_all<int, std_mhash_4     , times>("std_mhash_4     ", v, count).exec();
    test_all<int, chash_mset_4    , times>("chash_mset_4    ", v, count).exec();
    test_all<int, sbtree_mset_4   , times>("sbtree_mset_4   ", v, count).exec();
    test_all<int, bpptree_mset_4  , times>("bpptree_mset_4  ", v, count).exec();
    test_all<int, std_mset_32     , times>("std_mset_32     ", v, count).exec();
    test_all<int, std_mhash_32    , times>("std_mhash_32    ", v, count).exec();
    test_all<int, chash_mset_32   , times>("chash_mset_32   ", v, count).exec();
    test_all<int, sbtree_mset_32  , times>("sbtree_mset_32  ", v, count).exec();
    test_all<int, bpptree_mset_32 , times>("bpptree_mset_32 ", v, count).exec();
    test_all<int, std_mset_64     , times>("std_mset_64     ", v, count).exec();
    test_all<int, std_mhash_64    , times>("std_mhash_64    ", v, count).exec();
    test_all<int, chash_mset_64   , times>("chash_mset_64   ", v, count).exec();
    test_all<int, sbtree_mset_64  , times>("sbtree_mset_64  ", v, count).exec();
    test_all<int, bpptree_mset_64 , times>("bpptree_mset_64 ", v, count).exec();
    test_all<int, std_mset_128    , times>("std_mset_128    ", v, count).exec();
    test_all<int, std_mhash_128   , times>("std_mhash_128   ", v, count).exec();
    test_all<int, chash_mset_128  , times>("chash_mset_128  ", v, count).exec();
    test_all<int, sbtree_mset_128 , times>("sbtree_mset_128 ", v, count).exec();
    test_all<int, bpptree_mset_128, times>("bpptree_mset_128", v, count).exec();
    test_all<int, std_mset_200    , times>("std_mset_200    ", v, count).exec();
    test_all<int, std_mhash_200   , times>("std_mhash_200   ", v, count).exec();
    test_all<int, chash_mset_200  , times>("chash_mset_200  ", v, count).exec();
    test_all<int, sbtree_mset_200 , times>("sbtree_mset_200 ", v, count).exec();
    test_all<int, bpptree_mset_200, times>("bpptree_mset_200", v, count).exec();
}