#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>

double GetProcessCPUUsage(HANDLE hProcess, FILETIME ft_sysPrev, FILETIME ft_userPrev) {
    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    if (!GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser))
        return 0.0;

    ULARGE_INTEGER sysPrev, userPrev, sysNow, userNow;
    sysPrev.LowPart = ft_sysPrev.dwLowDateTime;
    sysPrev.HighPart = ft_sysPrev.dwHighDateTime;

    userPrev.LowPart = ft_userPrev.dwLowDateTime;
    userPrev.HighPart = ft_userPrev.dwHighDateTime;

    sysNow.LowPart = ftKernel.dwLowDateTime;
    sysNow.HighPart = ftKernel.dwHighDateTime;

    userNow.LowPart = ftUser.dwLowDateTime;
    userNow.HighPart = ftUser.dwHighDateTime;

    ULONGLONG sysDiff = sysNow.QuadPart - sysPrev.QuadPart;
    ULONGLONG userDiff = userNow.QuadPart - userPrev.QuadPart;
    ULONGLONG total = sysDiff + userDiff;

    return (double)total / 10000.0;
}



int main() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    while (true) {
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            std::cerr << "Ошибка: не удалось создать снимок процессов." << std::endl;
            return 1;
        }

        PROCESSENTRY32 pe32 = { 0 };
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hProcessSnap, &pe32)) {
            std::cerr << "Ошибка: не удалось получить информацию о процессе." << std::endl;
            CloseHandle(hProcessSnap);
            return 1;
        }

        std::cout << std::left
            << std::setw(8) << "PID"
            << std::setw(40) << "Name"
            << std::setw(24) << "RAM (MB)"
            << "CPU (%)" << std::endl;

        std::cout << std::string(90, '-') << std::endl;

        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (!hProcess)
                continue;

            FILETIME ftCreation, ftExit, ftKernel1, ftUser1;
            GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel1, &ftUser1);



            FILETIME ftKernel2, ftUser2;
            GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel2, &ftUser2);

            double cpu = GetProcessCPUUsage(hProcess, ftKernel1, ftUser1) / 100.0;

            PROCESS_MEMORY_COUNTERS pmc = { 0 };
            double ramMB = 0;

            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                ramMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
            }

            std::wcout
                << std::left
                << std::setw(8) << pe32.th32ProcessID
                << std::setw(40) << pe32.szExeFile
                << std::setw(24)
                << std::fixed
                << std::setprecision(1) << ramMB
                << std::fixed << std::setprecision(1) << cpu
                << std::endl;

            CloseHandle(hProcess);

        } while (Process32Next(hProcessSnap, &pe32));

        CloseHandle(hProcessSnap);
        Sleep(2000);
        system("cls");
    }
    return 0;
}
