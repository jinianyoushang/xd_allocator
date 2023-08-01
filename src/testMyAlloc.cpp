//
// Created by 17632 on 2023/6/19.
//
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <ext/malloc_allocator.h>
#include <ext/pool_allocator.h>
#include <ext/mt_allocator.h>
#include <ext/bitmap_allocator.h>
#include <windows.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include "pool_allocator.h"
#include "Loki_allocator.h"
#include "new_allocator.h"


using namespace std;

const vector<int> benchmarkSize{8, 16, 32, 64, 128, 256};
std::random_device rd; // 用于获取真随机数种子
std::mt19937 gen(rd()); // 基于真随机数种子创建随机数生成器
std::uniform_int_distribution<> dis(0, benchmarkSize.size() - 1); // 定义范围在1到6的均匀分布
const int testnum = 500000;
//const bool test1= true;
//const bool test2= true;
//const bool test3= true;
//const bool test4= true;


//返回当前进程占用虚拟内存大小，调用windows接口
//返回值是double 单位MB
double getWindowsProcessMemoryInfo(){
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;
    double res=(double )virtualMemUsedByMe/1024.0/1024.0;
    return res;
}

template<typename T>
void test(T alloc1) {
    // 获取当前时间
    auto start = std::chrono::system_clock::now();
    double memory_begin=getWindowsProcessMemoryInfo();
    char *p;
    //测试1 各自申请8-256字节大小内存，之后立即回收，重复50w次。
    for (const auto &item: benchmarkSize) {
        for (int i = 0; i < testnum; ++i) {
            p = alloc1.allocate(item);
            alloc1.deallocate(p, item);
        }
    }

    // 获取执行结束时间
    auto time1 = std::chrono::system_clock::now();
    // 计算时间间隔
    std::chrono::duration<double> diff1 = time1 - start;
    std::cout << "test1 taken: " << diff1.count() << " seconds" << std::endl;


    // 测试2：依次申请(8-256)，回收后申请下次,50w次。
    vector<char *> v;
    v.reserve(benchmarkSize.size());
    for (int i = 0; i < testnum; ++i) {
        v.clear();
        for (const auto &item: benchmarkSize) {
            v.emplace_back(alloc1.allocate(item));
        }
        for (int j = 0; j < benchmarkSize.size(); ++j) {
            alloc1.deallocate(v[j], benchmarkSize[j]);
        }
    }
    // 获取执行结束时间
    auto time2 = std::chrono::system_clock::now();
    // 计算时间间隔
    std::chrono::duration<double> diff2 = time2 - time1;
    std::cout << "test2 taken: " << diff2.count() << " seconds" << std::endl;

    // 测试3：全部申请8-256*50w之后，回收。
    double memory1=getWindowsProcessMemoryInfo();
    vector<char *> v2;
    v2.reserve(benchmarkSize.size() * testnum);
    for (const auto &item: benchmarkSize) {
        for (int i = 0; i < testnum; ++i) {
            v2.emplace_back(alloc1.allocate(item));
        }
    }
    double memory2=getWindowsProcessMemoryInfo();
    std::cout << "test3 memory alloc: " << memory2-memory1 << " MB" << std::endl;
    for (int i = 0; i < benchmarkSize.size(); ++i) {
        for (int j = 0; j < testnum; ++j) {
            alloc1.deallocate(v2[i * testnum + j], benchmarkSize[i]);
        }
    }
    double memory3=getWindowsProcessMemoryInfo();
    std::cout << "test3 memory alloc and free: " << memory3-memory1 << " MB" << std::endl;
    // 获取执行结束时间
    auto time3 = std::chrono::system_clock::now();
//     计算时间间隔
    std::chrono::duration<double> diff3 = time3 - time2;
    std::cout << "test3 taken: " << diff3.count() << " seconds" << std::endl;

    // 测试4：随机全部申请8-256大小的内存，200w次，回收
    //一个容器记录申请数据大小，另一个记录申请的指针
    vector<char *> v3;
    vector<int> v4;
    v3.reserve(testnum * 4);
    v4.reserve(testnum * 4);
    double memory4=getWindowsProcessMemoryInfo();
    for (int i = 0; i < testnum * 4; ++i) {
        v4.emplace_back(benchmarkSize[dis(gen)]);
    }
    for (const auto &item: v4) {
        v3.emplace_back(alloc1.allocate(item));
    }
    double memory5=getWindowsProcessMemoryInfo();
    std::cout << "test4 memory alloc " << memory5-memory4 << " MB" << std::endl;
    for (int i = 0; i < testnum * 4; ++i) {
        alloc1.deallocate(v3[i], v4[i]);
    }
    double memory6=getWindowsProcessMemoryInfo();
    std::cout << "test4 memory alloc and free" << memory6-memory4 << " MB" << std::endl;
    // 获取执行结束时间
    auto time4 = std::chrono::system_clock::now();
    // 计算时间间隔
    std::chrono::duration<double> diff4 = time4 - time3;
    std::cout << "test4 taken: " << diff4.count() << " seconds" << std::endl;

    // 获取执行结束时间
    auto end = std::chrono::system_clock::now();
    // 计算时间间隔
    std::chrono::duration<double> diff = end - start;
    std::cout << "Time all taken: " << diff.count() << " seconds" << std::endl;
    double memory_end=getWindowsProcessMemoryInfo();
    std::cout << "test ext all taken " << memory_end-memory_begin << " MB" << std::endl;
    cout<<endl;
}

void test01() {
    cout << "std::allocator-----------------" << endl;
    allocator<char> a;
    test(a);
}

void test02() {
    cout << "xd::new_allocator-----------------" << endl;
    xd::new_allocator<char> a;
    test(a);
}

void test03() {
    cout << "xd::pool_allocator-----------------" << endl;
    xd::pool_allocator<char> a;
    test(a);
}

void test04() {
    cout << "xd::Loki_allocator-----------------" << endl;
    xd::Loki_allocator<char> a;
    test(a);
}

void test05() {
    cout << "__gnu_cxx::malloc_allocator-----------------" << endl;
    __gnu_cxx::malloc_allocator<char> a;
    test(a);
}

void test06() {
    cout << "__gnu_cxx::__pool_alloc-----------------" << endl;
    __gnu_cxx::__pool_alloc<char> a;
    test(a);
}

void test07() {
    cout << "__gnu_cxx::__mt_alloc-----------------" << endl;
    __gnu_cxx::__mt_alloc<char> a;
    test(a);
}


void test08() {
    cout << "__gnu_cxx::bitmap_allocator-----------------" << endl;
    __gnu_cxx::bitmap_allocator<char> a;
    test(a);
}



int main() {
    test01();//使用默认的allocator例子
    test02();//使用new allocator例子
    test03();//使用pool allocator例子
    test04();//使用Loki allocator例子
    test05();//__gnu_cxx::malloc_allocator
    test06();//__gnu_cxx::__pool_alloc<int>
    test07();//__gnu_cxx::__mt_alloc<int>
    test08();//__gnu_cxx::bitmap_allocator<int>

    return 0;
}