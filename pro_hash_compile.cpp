
#define _SCL_SECURE_NO_WARNINGS

#include "pro_hash_map.h"
#include "pro_hash_set.h"

#include <string>

template<class T> void foo_test(T &hs)
{
    typedef typename std::remove_const<T>::type O;
    typename O::hasher h;
    typename O::key_equal ke;
    typename O::allocator_type a;
    typename O::key_type k;
    typename O::value_type v;
    O o, oo;
    auto b = hs.cbegin();
    auto e = hs.cend();
    T o00(0, h, ke, a);
    T o01(a);
    T o02(0, a);
    T o03(0, h, a);
    T o04(b, e, 0, h, ke, a);
    T o05(b, e, 0, a);
    T o06(b, e, 0, h, a);
    T o07(o);
    T o08(o, a);
    T o09(std::move(o));
    T o10(std::move(o), a);
    T o11({}, 0, h, ke, a);
    T o12({}, 0, a);
    T o13({}, 0, h, a);
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
    hs.find(k);
    o.erase(o.cbegin());
    o.erase(k);
    o.erase(b, e);
    hs.count(k);
    hs.equal_range(k);
    hs.begin();
    hs.cbegin();
    hs.end();
    hs.cend();
    hs.empty();
    o.empty();
    o.size();
    o.max_size();
    auto lb = hs.cbegin(0);
    auto le = hs.cend(0);
    while(lb != le)
    {
        ++lb;
    }
    hs.bucket_count();
    hs.max_bucket_count();
    hs.bucket_size(0);
    hs.bucket(k);
    o.reserve(0);
    o.rehash(0);
    o.max_load_factor(0);
    o.max_load_factor();
    o.load_factor();
}

void foo()
{
    pro_hash_map<int, int> bp_0;
    pro_hash_map<std::string, std::string> bp_1;
    pro_hash_map<int, int> const bp_2;
    pro_hash_map<std::string, std::string> const bp_3;
    pro_hash_set<int> bp_8;
    pro_hash_set<std::string> bp_9;
    pro_hash_set<int> const bp_a;
    pro_hash_set<std::string> const bp_b;

    foo_test(bp_0);
    foo_test(bp_1);
    foo_test(bp_2);
    foo_test(bp_3);
    foo_test(bp_8);
    foo_test(bp_9);
    foo_test(bp_a);
    foo_test(bp_b);
}