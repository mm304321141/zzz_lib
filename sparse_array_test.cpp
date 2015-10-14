
#define _SCL_SECURE_NO_WARNINGS

#include "sparse_array.h"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
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
    ~sparse_array_debug_config()
    {
        for(auto &item : mem_map)
        {
            ::free(item.second);
        }
    }
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
            assert(0);
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
            assert(0);
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
    sa2_t array1;
    sa2_t array2;
    int *array3 = new int[32768];
    int *array4 = new int[32768];
    memset(array3, 0, sizeof(int) * 32768);
    std::ifstream ifs("./test_dump.bin", std::ios::in | std::ios::binary);
    if(false && ifs.good())
    {
        auto &allocator = array1.allocator();
        sa2_t::dump_data dump;
        auto read = [&ifs](void *p, size_t l)
        {
            ifs.read(reinterpret_cast<char *>(p), l);
        };
        read(&dump, sizeof dump);
        array1.load_dump(dump);
        size_t map_size;
        read(&map_size, sizeof map_size);
        while(map_size-- > 0)
        {
            int k;
            read(&k, sizeof k);
            void *ptr = ::malloc(sparse_array_debug_config::memory_size);
            read(ptr, sparse_array_debug_config::memory_size);
            allocator.mem_map.insert(std::make_pair(k, ptr));
            if(k > allocator.mem_key_seed)
            {
                allocator.mem_key_seed = k;
            }
        }
        for(int i = 1; i < allocator.mem_key_seed; ++i)
        {
            if(allocator.mem_map.find(i) == allocator.mem_map.end())
            {
                allocator.mem_key.push_back(i);
            }
        }
        ifs.close();

        array1.get_multi(0, array3, 32768);
        array1[198] = 198;
        array1.get_multi(0, array4, 32768);
        array3[198] = 198;
        if(std::memcmp(array3, array4, sizeof(int) * 32768) != 0)
        {
            assert(0);
        }
    }


    for(uint32_t i = 0; i < count; ++i)
    {
        int c1, c2, r, b, e, s, l;
        if((c1 = (std::rand() & 1)))
        {
            r = std::rand();
            array1[r] = r;
            array3[r] = r;
        }
        else
        {
            b = std::rand();
            e = std::rand();
            s = std::min(b, e);
            l = std::max(b, e) - s;
            if((c2 = (std::rand() & 1)))
            {
                std::fill(array3 + s, array3 + s + l, 1);
                array1.set_multi(s, array3 + s, l);
            }
            else
            {
                std::fill(array3 + s, array3 + s + l, 0);
                array1.clear(s, l);
            }
        }
        array1.get_multi(0, array4, 32768);
        if(std::memcmp(array3, array4, sizeof(int) * 32768) != 0)
        {
            auto &map = array1.allocator().mem_map;
            auto dump = array1.dump();
            std::ofstream ofs("./test_dump.bin", std::ios::out | std::ios::binary);
            auto write = [&ofs](void const *p, size_t l)
            {
                ofs.write(reinterpret_cast<char const *>(p), l);
            };
            write(&dump, sizeof dump);
            size_t map_size = map.size();
            write(&map_size, sizeof map_size);
            for(auto &item : map)
            {
                write(&item.first, sizeof item.first);
                write(item.second, sparse_array_debug_config::memory_size);
            }
            ofs.flush();
            ofs.close();
            if(c1)
            {
                printf("%d,%d error !\n", i, r);
            }
            else
            {
                printf("%d,%d,%d,%d error !\n", i, c2, s, l);
            }
            break;
        }
        else
        {
            if(i % 10000 == 0)
            {
                printf("%d\n", i);
            }
        }
    }

    delete[] array3;
    delete[] array4;
    system("pause");
}