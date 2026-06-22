
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
    typename O::key_type k = typename O::key_type();
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
    T o10({ v }, c);
    T o11({ v }, a);
    T o12({ v }, c, a);
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
    (void)bp.find(k);
    o.erase(o.cbegin());
    o.erase(k);
    o.erase(b, e);
    (void)bp.count(k);
    (void)bp.count(k, k);
    (void)bp.between(k, k);
    (void)bp.lower_bound(k);
    (void)bp.upper_bound(k);
    (void)bp.equal_range(k);
    (void)bp.begin();
    (void)bp.cbegin();
    (void)bp.rbegin();
    (void)bp.crbegin();
    (void)bp.end();
    (void)bp.cend();
    (void)bp.rend();
    (void)bp.crend();
    (void)bp.front();
    (void)bp.back();
    (void)bp.empty();
    (void)o.empty();
    (void)o.size();
    (void)o.max_size();
    (void)o.at(0);
    (void)o.rank(k);
    (void)o.lower_rank(k);
    (void)o.upper_rank(k);
    (void)O::rank(b);
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