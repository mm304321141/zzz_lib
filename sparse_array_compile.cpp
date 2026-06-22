
#define _SCL_SECURE_NO_WARNINGS

#include "sparse_array.h"

#include <string>
#include <utility>

template<class T> void foo_test()
{
    typedef sparse_array<T> array_t;
    typedef typename array_t::snapshot_data snapshot_t;

    array_t a;
    T v = T();
    T buf[4] = {};

    //single element set / get
    a[1] = v;
    a[2] = v;
    a[3] = a[2];
    v = a[2];
    v = a[3];

    // bulk set / get
    a.set_multi(0, buf, 4);
    a.get_multi(0, buf, 4);

    // span
    uint32_t n = a.span();

    // snapshot / restore
    snapshot_t d = a.snapshot();
    array_t b;
    b.restore(d);

    // allocator (mutable + const)
    array_t const &ca = a;
    a.allocator();
    ca.allocator();

    // const access
    v = ca[1];
    v = ca[1];
    n += ca.span();
    (void)ca.snapshot();

    // partial / full reset
    a.reset(0, 2);
    a.reset();

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
