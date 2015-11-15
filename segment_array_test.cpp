
#define _SCL_SECURE_NO_WARNINGS

#include "segment_array.h"

#include <chrono>
#include <iostream>
#include <random>

struct segment_char_array_config
{
    typedef char value_type;
    typedef std::allocator<char> allocator_type;
    typedef std::true_type status_type;
    enum
    {
        memory_block_size = 256,
    };
};


int main()
{
    segment_array<int> arr;
    arr.push_back(1);
    arr.emplace_front(2);
    arr.insert(arr.end(), 10, 10);
    arr.resize(10);



    auto t = std::chrono::high_resolution_clock::now;
    std::mt19937 mt;
    auto mtr = std::uniform_int_distribution<int>(0, 25);

    segment_array_implement<segment_char_array_config> char_arr;

    for(int i = 0; i <= 999999999; ++i)
    {
        char_arr.emplace_back("ABCDEFGHIJKLMNOPQRSTUVWXYZ"[mtr(mt)]);
    }

    std::cout << "inner bound = " << char_arr.status().inner_bound << std::endl;
    std::cout << "leaf bound = " << char_arr.status().leaf_bound << std::endl;
    std::cout << "inner count = " << char_arr.status().inner_count << std::endl;
    std::cout << "leaf count = " << char_arr.status().leaf_count << std::endl;
    for(size_t i = 0; i < char_arr.status().level_count.size(); ++i)
    {
        std::cout << "level count [" << i << "] = " << char_arr.status().level_count[i] << std::endl;
    }

    system("pause");
}