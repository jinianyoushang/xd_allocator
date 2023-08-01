//
// Created by 17632 on 2023/6/23.
//
#include <windows.h>
#include <psapi.h>
#include <iostream>

int main() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;
    double res=(double )virtualMemUsedByMe/1024.0/1024.0;
    std::cout << "Virtual memory used by this process: " << res << " MB\n";
    return 0;
}