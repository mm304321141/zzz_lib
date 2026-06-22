# zzz_lib

zzz 的 C++ 工具库，包含容器、迭代器工具等组件，均支持 C++11/14/17/20。

## 文件列表

| 文件 | 说明 |
|---|---|
| `bpptree.h` / `bpptree_map.h` / `bpptree_set.h` | 基于 B+ 树的有序容器 |
| `chash.h` / `chash_map.h` / `chash_set.h` | 基于开放寻址哈希表的无序容器 |
| `sbtree.h` / `sbtree_map.h` / `sbtree_set.h` | 基于 size-balanced BST 的有序容器 |
| `segment_array.h` | 基于 B+ 树节点管理策略的序列容器 |
| `sparse_array.h` | 基于 RB 树的稀疏数组 |
| `split_iterator.h` | 字符串分割迭代器 |

## 容器

### bpptree

基于 B+ 树实现，内存管理使用固定大小内存块。支持随机访问迭代器。迭代器在插入/删除后失效。`sizeof(key)` 较小时，插入/删除/查找速度优于 `std::map`；遍历速度在任何条件下都显著快于 `std::map`。提供 `map`/`set`/`multimap`/`multiset` 变体。

### chash

基于哈希表实现，内存集中分配以利用缓存。插入时可能触发扩容和数据搬运。遍历速度快。允许重复 key 时，`equal_range` 返回 `local_iterator`，仅支持 `erase` 操作。提供 `map`/`set`/`multimap`/`multiset` 变体。

### sbtree

基于二叉搜索树实现，使用节点 size 维持平衡（size-balanced tree）。支持随机访问迭代器，提供 `map`/`set`/`multimap`/`multiset` 变体。

### segment_array

基于 B+ 树节点管理策略实现，内存管理使用固定大小内存块。任意位置插入/删除成本均较低。

### sparse_array

稀疏数组，仅存储非零区间，未设置的位置隐式为零值。内部使用自定义 `handle` 抽象的内存管理，以 RB 树管理非零区间。适合存储下标范围大、稀疏度高的数据集合。支持自定义内存分配器（`config_t`）。

## 工具

### split_iterator

以迭代器方式进行字符串分割，不需要额外内存存储分割后的数据。注意：传入字符串在 split 过程中不会被拷贝，调用方需保证字符串的有效期。

提供 `size()`（惰性计算，不推荐）和 `operator[]`（从头扫描，不推荐）。

C++17 起 `string_ref` 使用 `std::string_view`，C++14 下为自有轻量实现。提供 `to_value<T>()` 自由函数替换 `atoi`/`atof` 系列接口。C++17 起 `string_to_integer`/`string_to_real` 使用 `std::from_chars`，无 locale 依赖。

## 调试视图

| 组件 | Natvis | GDB pretty-printer |
|---|---|---|
| `bpptree` | `bpptree.natvis` | `bpptree_printer.py` |
| `chash` | `chash.natvis` | `chash_printer.py` |
| `sbtree` | `sbtree.natvis` | `sbtree_printer.py` |
| `segment_array` | `segment_array.natvis` | `segment_array_printer.py` |
| `sparse_array` | `sparse_array.natvis` | `sparse_array_printer.py` |

Natvis 文件加入工程即可在 MSVC / CLion 中生效；GDB pretty-printer 在 GDB 中 `source <file>.py` 加载。

## C++ Standard Support

| Component | C++11 | C++14 | C++17 | C++20 |
|---|---|---|---|---|
| `bpptree.h` / `bpptree_map.h` / `bpptree_set.h` | ✅ | ✅ | ✅ | ✅ |
| `chash.h` / `chash_map.h` / `chash_set.h` | ✅ | ✅ | ✅ | ✅ |
| `sbtree.h` / `sbtree_map.h` / `sbtree_set.h` | ✅ | ✅ | ✅ | ✅ |
| `segment_array.h` | ✅ | ✅ | ✅ | ✅ |
| `sparse_array.h` | ✅ | ✅ | ✅ | ✅ |
| `split_iterator.h` | ✅ | ✅ | ✅ | ✅ |

## 特性比较

![features.png](/profile/features.png)

## 性能测试

各种容器的测试。

OSX 10.11.3 (15D21)、Xcode 7.1.1 (7B1005)、2.5 GHz Intel Core i7、16 GB 1600 MHz DDR3。

- 测试采用预先随机好的随机数 5 组，测试结果取平均值
- 横轴为容器元素数量
- 纵轴为平均每个元素耗费时间（纳秒）
- 后面的数字表示 key 大小（字节）

操作说明：

| 缩写 | 含义 |
|---|---|
| `insert_o` | 顺序插入 |
| `insert_r` | 随机插入 |
| `foreach` | 遍历 |
| `find` | 查找 |
| `erase` | 删除 |

容器缩写对照：

| 实现 | 缩写 |
|---|---|
| `std::set` | `std_set` |
| `std::unordered_set` | `std_hash` |
| `chash_set` | `chash_set` |
| `bpptree_set` | `bpptree_set` |
| `std::multiset` | `std_mset` |
| `std::unordered_multiset` | `std_mhash` |
| `chash_multiset` | `chash_mset` |
| `sbtree_multiset` | `sbtree_mset` |
| `bpptree_multiset` | `bpptree_mset` |

更详细的表格：

- [Mac](/profile/profile1.xlsx?raw=true)
- [iPhone](/profile/profile2.xlsx?raw=true)

![profile.png](/profile/profile.png)
