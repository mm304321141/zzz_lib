# zzz_lib
zzz's c++ lib  

<br/>

* sbtree_map.h
* sbtree_set.h
* bpptree_map.h
* bpptree_set.h

可以随机访问的有序容器<br/>
标准库风格<br/>
如果你不在意迭代器在容器修改后是否仍然有效<br/>
那么强烈推荐使用bpptree_map/set系列<br/>
相比std::map/set内存更小,时间更短<br/>
a random accessible key value container ...<br/>
standard library style<br/>

<br/>

* segment_array.h

不连续数组<br/>
标准库风格<br/>
内部实际上是B+树节点管理策略<br/>
任意位置插入/删除成本都很低<br/>
a segment array implement ...<br/>
insert/erase anywhere are low cost ...<br/>
standard library style<br/>

<br/>

* sparse_array.h

稀松数组<br/>
内存管理使用相同大小内存块<br/>
a sparse array implement ...<br/>
use fixed size memory ...<br/>

<br/>

* sbtree.natvis
* bpptree.natvis
* segment_array.natvis

加入到工程,调试时候有更友好的视图<br/>
custom views of native<br/>