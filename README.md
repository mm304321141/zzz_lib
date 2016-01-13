# zzz_lib
zzz's c++ lib  

<br/>

* sbtree_map.h
* sbtree_set.h
* bpptree_map.h
* bpptree_set.h
* pro_hash_map.h
* pro_hash_set.h
* segment_array.h

标准库风格容器<br/>
standard library style<br/>

* sbtree系列

基于二叉搜索树实现,使用size平衡<br/>
可以随机访问,随机访问迭代器<br/>
key可以重复<br/>

* bpptree系列

基于B+树实现<br/>
可以随机访问,随机访问迭代器<br/>
内存管理使用相同大小内存块<br/>
相比标准库map,迭代器在插入/删除元素之后会失效<br/>
sizeof(key)非巨大的情况下,插入/删除/查找速度都超过标准库map<br/>
sizeof(key)巨大的情况下去,内存占用会偏大,并且性能下降<br/>
遍历速度任何条件下都很快!比标准库map快得多!<br/>

* pro_hash系列

基于哈希表实现<br/>
相比标准库unordered_map,迭代器在插入元素之后会失效<br/>
内部使用内存连续,在rehash动作发生时候,会发生搬运数据操作<br/>
在允许重复key时候,equal_range返回local_iterator,仅支持erase操作<br/>

* segment_array系列

基于B+树的节点管理策略实现<br/>
内存管理使用相同大小内存块<br/>
任意位置插入/删除成本都很低<br/>

<br/>
<br/>

* sbtree.natvis
* bpptree.natvis
* pro_hash.natvis
* segment_array.natvis

加入到工程,调试时候有更友好的视图<br/>
custom views of native<br/>

* sparse_array.h

稀松数组...不成熟的玩意...<br/>