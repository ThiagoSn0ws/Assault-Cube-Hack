#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <cstdint>
#include <wchar.h>

using Address = std::uintptr_t;
const wchar_t* targetProcess = L"ac_client.exe";

Address GetModuleBaseAddress(const wchar_t* processName, const wchar_t* moduleName) {
    DWORD procId = 0;
    Address modBaseAddr = 0;
    HANDLE hSnap = INVALID_HANDLE_VALUE;
    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32FirstW(hSnap, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, processName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &procEntry));
        }
        CloseHandle(hSnap);
    }

    if (procId == 0) {
        return 0;
    }

    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, moduleName)) {
                    modBaseAddr = (Address)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &modEntry));
        }
        CloseHandle(hSnap);
    }

    if (procId == 0) {
        return 0;
    }

    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, moduleName)) {
                    modBaseAddr = (Address)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &modEntry));
        }
        CloseHandle(hSnap);
    }

    return modBaseAddr;
}


HANDLE GetProcessHandle(const wchar_t* processName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32FirstW(hSnap, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, processName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &procEntry));
        }
        CloseHandle(hSnap);
    }

    if (procId == 0) {
        return nullptr;
    }

    return OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, procId);

}

template<typename T>
T ReadMemory(HANDLE hProcess, Address address) {
    T value = T();
    ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), NULL);
    return value;
}

template<typename T>
void WriteMemory(HANDLE hProcess, Address address, const T& value) {
    WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), NULL);
}

const Address LOCALPLAYER_POINTER_OFFSET = 0x8D3470;
const Address HEALTH_OFFSET = 0x008D355C;
const Address AMMO_OFFSET = 0x8D35B0;

int main() {

   const int modBaseAddr = GetModuleBaseAddress(targetProcess, L"ac_client.exe");

    HANDLE hProcess = GetProcessHandle(targetProcess);

    if (hProcess == nullptr) {
        std::wcerr << L"Error" << std::endl;
        std::cin.get();
        return 1;
    }

    Address PlayerPointerAddress = modBaseAddr + LOCALPLAYER_POINTER_OFFSET;
    Address LocalPlayerBase = ReadMemory<Address>(hProcess, PlayerPointerAddress);

    if (LocalPlayerBase == 0) {
        std::wcerr << L"ERROR: Dynamic address not found" << std::endl;
        CloseHandle(hProcess);
        std::cin.get();
        return 1;
    }

    Address healthAddress = LocalPlayerBase + HEALTH_OFFSET;

    const int hp = 1000;

    while (true) {
        Address LocalPlayerBase = ReadMemory<Address>(hProcess, PlayerPointerAddress);

        if (LocalPlayerBase != 0) {
            Address healthAddress = LocalPlayerBase + HEALTH_OFFSET;

            int currentHealth = ReadMemory<int>(hProcess, healthAddress);
            WriteMemory<int>(hProcess, healthAddress, hp);
        }

        else {
            std::cout << "Awaiting player address" << std::endl;
        }
    }
    Sleep(100);

        std::cout << "HP OFFSET: " << HEALTH_OFFSET << std::endl;
        std::cout << "AMMO OFFSET: " << AMMO_OFFSET << std::endl;

}
