
#define _SCL_SECURE_NO_WARNINGS

#include "segment_array.h"


int main()
{
    segment_array<int> arr;
    arr.push_back(1);
    arr.emplace_front(2);
    arr.insert(arr.end(), 100, 10);


    system("pause");
}