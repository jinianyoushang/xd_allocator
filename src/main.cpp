#include <iostream>
#include <list>
#include "new_allocator.h"
#include "pool_allocator.h"
#include "Loki_allocator.h"

using namespace std;

//使用allocator例子
void test01() {
    int *p;
    allocator<int> alloc1;
    p = alloc1.allocate(512);


    *p = 10;
    cout << *p << endl;

    alloc1.deallocate(p, 512);
    list<int> list1;
    list1.push_back(1);
    list1.push_back(1);
    cout << *list1.begin() << endl;
}

//使用xd new_allocator例子
void test02() {
    int *p;
    xd::new_allocator<int> alloc1;
    p = alloc1.allocate(512);//这里分配的是元素的个数

    *p = 10;
    cout << *p << endl;
    alloc1.deallocate(p, 512);

    list<int, xd::new_allocator<int>> list1;
    list1.push_back(1);
    list1.push_back(1);
    cout << *list1.begin() << endl;
}

//使用xd pool_allocator例子
void test03() {
    int *p;
    xd::pool_allocator<int> alloc1;
    p = alloc1.allocate(10);//这里分配的是元素的个数
    *p = 10;
    cout << *p << endl;
    alloc1.deallocate(p, 10);

    p = alloc1.allocate(512);
    alloc1.deallocate(p, 512);

    list<int, xd::pool_allocator<int>> list1;
    list1.push_back(1);
    list1.push_back(1);
    cout << *list1.begin() << endl;
}

//测试loki allocator的FixedAllocator
void test04() {
    xd::FixedAllocator fixedAllocator(20);
    auto p = (char *) fixedAllocator.allocate();
    *p = '0';
    *(p+1)='\0';
    cout<<*p<<endl;
    fixedAllocator.deallocate(p);
}

//测试loki allocator 最上层分配器
void test05(){
    int *p;
    xd::Loki_allocator<int> alloc1;
    p = alloc1.allocate(10);//这里分配的是元素的个数
    *p = 10;
    cout << *p << endl;
    alloc1.deallocate(p, 10);

    p = alloc1.allocate(512);
    alloc1.deallocate(p, 512);

    list<int, xd::Loki_allocator<int>> list1;
    list1.push_back(1);
    list1.push_back(1);
    cout << *list1.begin() << endl;
};

void test06(){
    xd::FixedAllocator fixedAllocator(8);
    for (int i = 0; i < 64; ++i) {
        auto p = (char *) fixedAllocator.allocate();
    }
}

int main() {
//    test01();//使用allocator例子
//    test02();//使用xd new_allocator例子 也就是默认的allocator
//    test03();//使用xd pool_allocator例子
//    test04(); //测试loki allocator的FixedAllocator
    test05();//使用xd loki allocator例子
//    test06();//测试loki allocator的FixedAllocator2
    return 0;
}
