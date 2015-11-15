
#define _SCL_SECURE_NO_WARNINGS

#include "sbtree_map.h"
#include "sbtree_set.h"

#include <string>

template<class V, class T> void foo_test(T &bp)
{
    typedef typename std::remove_const<T>::type O;
    typename O::key_compare c;
    typename O::allocator_type a;
    V v = V();
    typename O::key_type k;
    O o, oo;
    auto b = bp.cbegin();
    auto e = bp.cend();
    T o00(c);
    T o01(a);
    T o02(c, a);
    T o03(b, e, c);
    T o04(b, e, a);
    T o05(b, e, c, a);
    T o06(o);
    T o07(o, a);
    T o08(std::move(o));
    T o09(std::move(o), a);
    T o10({v}, c);
    T o11({v}, a);
    T o12({v}, c, a);
    o = oo;
    o = std::move(oo);
    o = {};
    o.swap(oo);
    o.insert(v);
    o.insert(std::move(v));
    o.insert(o.begin(), v);
    o.insert(o.begin(), std::move(v));
    o.insert(b, e);
    o.insert({});
    o.emplace(v);
    o.emplace_hint(o.begin(), v);
    bp.find(k);
    o.erase(o.cbegin());
    o.erase(k);
    o.erase(b, e);
    bp.count(k);
    bp.count(k, k);
    bp.range(k, k);
    bp.lower_bound(k);
    bp.upper_bound(k);
    bp.equal_range(k);
    bp.begin();
    bp.cbegin();
    bp.rbegin();
    bp.crbegin();
    bp.end();
    bp.cend();
    bp.rend();
    bp.crend();
    bp.front();
    bp.back();
    bp.empty();
    o.empty();
    o.size();
    o.max_size();
    o.at(0);
    o.rank(k);
    o.lower_rank(k);
    o.upper_rank(k);
    O::rank(b);
}

void foo()
{
    sbtree_multimap<int, int> bp_4;
    sbtree_multimap<std::string, std::string> bp_5;
    sbtree_multimap<int, int> const bp_6;
    sbtree_multimap<std::string, std::string> const bp_7;
    sbtree_multiset<int> bp_c;
    sbtree_multiset<std::string> bp_d;
    sbtree_multiset<int> const bp_e;
    sbtree_multiset<std::string> const bp_f;

    foo_test<std::pair<int, int>>(bp_4);
    foo_test<std::pair<std::string, std::string>>(bp_5);
    foo_test<std::pair<int, int>>(bp_6);
    foo_test<std::pair<std::string, std::string>>(bp_7);
    foo_test<int>(bp_c);
    foo_test<std::string>(bp_d);
    foo_test<int>(bp_e);
    foo_test<std::string>(bp_f);
}