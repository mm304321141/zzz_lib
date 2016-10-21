# zzz_lib
zzz's c++ lib  

<br/>

* sbtree_map.h
* sbtree_set.h
* bpptree_map.h
* bpptree_set.h
* chash_map.h
* chash_set.h
* segment_array.h

标准库风格容器<br/>
standard library style<br/>

* sbtree系列

基于二叉搜索树实现,使用size平衡<br/>
可以随机访问,随机访问迭代器<br/>
有multimap/multiset实现<br/>

* bpptree系列

基于B+树实现<br/>
可以随机访问,随机访问迭代器<br/>
内存管理使用相同大小内存块<br/>
相比标准库map,迭代器在插入/删除元素之后会失效<br/>
sizeof(key)非巨大的情况下,插入/删除/查找速度都超过标准库map<br/>
sizeof(key)巨大的情况下去,内存占用会偏大,并且性能下降<br/>
遍历速度任何条件下都很快!比标准库map快得多!<br/>
有map/set/multimap/multiset实现<br/>

* chash系列

基于哈希表实现<br/>
内存集中分配,尽可能利用缓存加速<br/>
插入元素可能导致扩容,产生搬运数据操作<br/>
遍历速度飞快!<br/>
在允许重复key时候,equal_range返回local_iterator,仅支持erase操作<br/>
有map/set/multimap/multiset实现<br/>

* segment_array系列

基于B+树的节点管理策略实现<br/>
内存管理使用相同大小内存块<br/>
任意位置插入/删除成本都很低<br/>

<br/>
<br/>

* sbtree.natvis
* bpptree.natvis
* chash.natvis
* segment_array.natvis

加入到工程,调试时候有更友好的视图<br/>
custom views of native<br/>

<br/>
<br/>
<br/>

* split_iterator

迭代器方式进行split<br/>
适配std::string<br/>
不需要额外的内存存储split后的数据<br/>
提供了size(),惰性计算,不推荐使用<br/>
提供了operator\[index\],从头扫描实现,不推荐使用<br/>
**请保证**传入的字符串的有效期,split过程中不会拷贝字符串<br/>
string_ref实现**不完整**,提供了to_value<>替换ato?接口<br/>

<br/>

* sparse_array.h

稀松数组...不成熟的玩意...<br/>

<br/>


#特性比较
![features.png](/profile/features.png)


#性能测试
各种容器的测试

<br/>

OSX 10.11.3 (15D21)<br/>
XCode 7.1.1 (7B1005)<br/>
2.5 GHz Intel Core i7<br/>
16 GB 1600 MHz DDR3<br/>

<br/>

* 测试采用与预先随机好的随机数5组,测试结果取平均值
* 横轴为容器元素数量
* 纵轴为平均每个元素耗费时间(纳秒)
* 后面的数字表示key大小(字节)

<br/>

* insert_o -> 顺序插入
* insert_r -> 随机插入
* foreach -> 遍历
* find -> 查找
* erase -> 删除

<br/>

* std::set                -> std_set     
* std::unordered_set      -> std_hash    
* chash_set               -> chash_set   
* bpptree_set             -> bpptree_set 
* std::multiset           -> std_mset    
* std::unordered_multiset -> std_mhash   
* chash_multiset          -> chash_mset  
* sbtree_multiset         -> sbtree_mset 
* bpptree_multiset        -> bpptree_mset

<br/>

更详细的表格:

* [Mac](/profile/profile1.xlsx?raw=true)
* [iPhone](/profile/profile2.xlsx?raw=true)

<br/>

![profile.png](/profile/profile.png)