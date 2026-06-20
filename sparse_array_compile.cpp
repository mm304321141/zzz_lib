
#define _SCL_SECURE_NO_WARNINGS

#include "sparse_array.h"

#include <string>
#include <utility>

template<class T> void foo_test()
{
    typedef sparse_array<T> array_t;
    typedef typename array_t::dump_data dump_t;

    array_t a;
    T v = T();
    T buf[4] = {};

    //single element set / get
    a.set(1, v);
    a[2] = v;
    a[3] = a[2];
    v = a.get(2);
    v = a[3];

    // bulk set / get
    a.set_multi(0, buf, 4);
    a.get_multi(0, buf, 4);

    // size
    uint32_t n = a.size();

    // dump / load_dump
    dump_t d = a.dump();
    array_t b;
    b.load_dump(d);

    // allocator (mutable + const)
    array_t const &ca = a;
    a.allocator();
    ca.allocator();

    // const access
    v = ca.get(1);
    v = ca[1];
    n += ca.size();
    ca.dump();

    // partial / full clear
    a.clear(0, 2);
    a.clear();

    //move ctor / move assign
    array_t c(std::move(a));
    b = std::move(c);

    (void)v;
    (void)n;
    (void)buf;
}

void foo()
{
    foo_test<int>();
    foo_test<std::string>();
}
