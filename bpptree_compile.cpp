
#define _SCL_SECURE_NO_WARNINGS

#include "bpptree_map.h"
#include "bpptree_set.h"

#include <string>

template<class T> void foo_test(T &bp)
{
    typedef typename std::remove_const<T>::type O;
    typename O::key_compare c;
    typename O::allocator_type a;
    typename O::storage_type v;
    typename O::key_type k{};
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
    T o10({}, c);
    T o11({}, a);
    T o12({}, c, a);
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
    bpptree_map<int, int> bp_0;
    bpptree_map<std::string, std::string> bp_1;
    bpptree_map<int, int> const bp_2;
    bpptree_map<std::string, std::string> const bp_3;
    bpptree_multimap<int, int> bp_4;
    bpptree_multimap<std::string, std::string> bp_5;
    bpptree_multimap<int, int> const bp_6;
    bpptree_multimap<std::string, std::string> const bp_7;
    bpptree_set<int> bp_8;
    bpptree_set<std::string> bp_9;
    bpptree_set<int> const bp_a;
    bpptree_set<std::string> const bp_b;
    bpptree_multiset<int> bp_c;
    bpptree_multiset<std::string> bp_d;
    bpptree_multiset<int> const bp_e;
    bpptree_multiset<std::string> const bp_f;

    foo_test(bp_0);
    foo_test(bp_1);
    foo_test(bp_2);
    foo_test(bp_3);
    foo_test(bp_4);
    foo_test(bp_5);
    foo_test(bp_6);
    foo_test(bp_7);
    foo_test(bp_8);
    foo_test(bp_9);
    foo_test(bp_a);
    foo_test(bp_b);
    foo_test(bp_c);
    foo_test(bp_d);
    foo_test(bp_e);
    foo_test(bp_f);
}