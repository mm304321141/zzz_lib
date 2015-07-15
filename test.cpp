
#define _SCL_SECURE_NO_WARNINGS

#include "sparse_array.h"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

struct sparse_array_debug_config
{
    typedef int handle_t;
    enum
    {
        memory_size = 512,
        atomic_length = 4,
        invalid_handle = 0,
    };
    handle_t alloc()
    {
        int key;
        if(!mem_key.empty())
        {
            key = mem_key.back();
            mem_key.pop_back();
        }
        else
        {
            key = ++mem_key_seed;
        }
        mem_map.insert(std::make_pair(key, ::malloc(memory_size)));
        return key;
    }
    void dealloc(handle_t handle)
    {
        auto find = mem_map.find(handle);
        if(find == mem_map.end())
        {
            _asm int 3;
        }
        ::free(find->second);
        mem_map.erase(find);
        mem_key.push_back(handle);
    }
    void *get(handle_t handle)
    {
        auto find = mem_map.find(handle);
        if(find == mem_map.end())
        {
            _asm int 3;
        }
        return find->second;
    }
    std::unordered_map<int, void *> mem_map;
    int mem_key_seed = 0;
    std::vector<int> mem_key;
};

int main()
{
    const uint32_t count = 10000000;
    typedef sparse_array<int> sa1_t;
    typedef sparse_array<int, sparse_array_debug_config> sa2_t;
    sa1_t array1;
    sa2_t array2;
    int *array4 = new int[32768];
    int *array3 = new int[32768];
    memset(array4, 0, sizeof(int) * 32768);
    memset(array3, 0, sizeof(int) * 32768);
    for(uint32_t i = 0; i < count; ++i)
    {
        int random = std::rand();
        array1[random] = random;
        array4[random] = random;
        array2[i] = array1[i];
    }
    array1.get_multi(0, array3, 32768);
    array2.set_multi(0, array3, 32768);
    array2.clear(32768, count - 32768);
    std::cout << std::memcmp(array4, array3, sizeof(int) * 32768) << std::endl;
    array2.get_multi(0, array4, 32768);
    std::cout << std::memcmp(array4, array3, sizeof(int) * 32768) << std::endl;
    array1.clear(999, 1002);
    memset(array4 + 999, 0, sizeof(int) * 1002);
    array1.get_multi(0, array3, 32768);
    std::cout << std::memcmp(array4, array3, sizeof(int) * 32768) << std::endl;
    array1.clear();
    std::cout << array1.size() << std::endl;
    array2.clear();
    std::cout << array2.size() << std::endl;

    delete[] array4;
    delete[] array3;
    system("pause");
}