
#define _SCL_SECURE_NO_WARNINGS

#include "segment_array.h"

#include <string>

template<class T> void foo_test(T &bp)
{
    typedef typename std::remove_const<T>::type O;
    typename O::allocator_type a;
    typename O::value_type v;
    O o, oo;
    auto b = bp.cbegin();
    auto e = bp.cend();
    T o01(a);
    T o02(1, a);
    T o03(1, v, a);
    T o04(b, e, a);
    T o05(o);
    T o06(o, a);
    T o07(std::move(o));
    T o08(std::move(o), a);
    T o09({}, a);
    o = oo;
    o = std::move(oo);
    o = {};
    o.swap(oo);
    o.assign(b, e);
    o.assign(1, v);
    o.assign({});
    o.insert(o.begin(), v);
    o.insert(o.begin(), std::move(v));
    o.insert(o.begin(), 1, v);
    o.insert(o.begin(), b, e);
    o.insert(o.begin(), {});
    o.push_back(v);
    o.push_back(std::move(v));
    o.push_front(v);
    o.push_front(std::move(v));
    o.pop_back();
    o.pop_front();
    o.emplace(o.begin(), v);
    o.emplace_back(v);
    o.emplace_front(v);
    o.erase(o.cbegin());
    o.erase(b, e);
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
    o[0];
    O::rank(b);
}

void foo()
{
    segment_array<int> bp_0;
    segment_array<std::string> bp_1;
    segment_array<int> const bp_2;
    segment_array<std::string> const bp_3;

    foo_test(bp_0);
    foo_test(bp_1);
    foo_test(bp_2);
    foo_test(bp_3);
}