
#define _SCL_SECURE_NO_WARNINGS

#include "split_iterator.h"

#include <string>

void foo()
{
    typedef string_ref<char> sref;

    std::string s = "1,2,3,4";
    sref r0;
    (void)r0;
    sref r1(s);
    sref r2("hello");
    sref r3("hello", 5);
    sref r4(r1);
    r4 = r1;

    (void)(r2 == r3);
    (void)(r2 != r3);
    (void)(r2 < r3);
    (void)(r2 > r3);
    (void)(r2 <= r3);
    (void)(r2 >= r3);
    (void)(r2 == "hello");
    (void)("hello" == r2);
    (void)(r2 == s);
    (void)(s == r2);

    sref r5 = r2.substr(1, 3);
    (void)r5;
    (void)r2.find('e');
    (void)r2.find("ll");
    (void)r2.find(sref("ll"));

    sref ri("12345");
    (void)to_value<int>(ri);
    sref rd("3.14");
    (void)to_value<double>(rd);
    (void)to_string(ri);

    for(auto v : make_split(s, ','))
    {
        (void)v;
    }
    std::string s2 = "a, b, c";
    for(auto v : make_split(s2, ", "))
    {
        (void)v;
    }
    std::string s3 = "a,b;c,d";
    for(auto v : make_split_any_of(s3, ",;"))
    {
        (void)v;
    }

    auto c = make_split(s, ',');
    (void)c.size();
    auto p = c.split2();
    (void)p;
    int a = 0, b = 0, d = 0;
    (void)c.fill(a, b, d);
    (void)c[0];
    (void)c[2];
}
