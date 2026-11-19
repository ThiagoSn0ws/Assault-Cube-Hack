#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <cstdint>
#include <wchar.h>


using Address = DWORD;
const wchar_t* targetProcess = L"ac_client.exe";

Address GetModuleBaseAddress(const wchar_t* processName, const wchar_t* moduleName) {
    DWORD procId = 0;
    Address modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W procEntry;
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

    if (procId == 0) return 0;

    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, moduleName)) {
                    modBaseAddr = reinterpret_cast<Address>(modEntry.modBaseAddr);
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
        PROCESSENTRY32W procEntry;
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

    if (procId == 0) return nullptr;

    return OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, procId);
}

template<typename T>
T ReadMemory(HANDLE hProcess, Address address) {
    T value = T();
    ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), NULL);
    return value;
}

template<typename T>
void WriteMemory(HANDLE hProcess, Address address, const T& value) {
    WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(address), &value, sizeof(T), NULL);
}

const Address GLOBAL_AMMO_OFFSET = 0xC73EF;
const Address COOLDOWN_OFFSET = 0x00C73EA;
const Address COOLDOWN_POINTER = 0x00183828;
const Address COOLDOWN_OFFSET_1 = 0x8;
const Address COOLDOWN_OFFSET_2 = 0x3DC;
const Address COOLDOWN_FINAL = 0x594;
const Address AMMO_POINTER_PRIMARY = 0x00183828;
const Address AMMO_OFFSET_PRIMARY_1 = 0x8;
const Address AMMO_OFFSET_PRIMARY_2 = 0x73C;
const Address AMMO_OFFSET_PRIMARY_FINAL = 0x60C;
const Address AMMO_OFFSET_SECONDARY = 0x12C;
const Address AMMO_POINTER_SECONDARY = 0x0017E0A8;
const Address Grenade_OFFSET = 0x144;
const Address PlayerPointer = 0x0018B0B8;
const Address HEALTH_OFFSET = 0x408;

void RapidFire(HANDLE hProcess, Address modBaseAddr) {
    Address RapidFireAddr = modBaseAddr + COOLDOWN_OFFSET;

    BYTE nops[] = { 0x90, 0x90 };
    size_t instructionSize = sizeof(nops);

    DWORD OriginalPermission;

    if (VirtualProtectEx(hProcess, (LPVOID)RapidFireAddr, instructionSize, PAGE_EXECUTE_READWRITE, &OriginalPermission)) {
        size_t bytesWriten;
        WriteProcessMemory(hProcess, (LPVOID)RapidFireAddr, nops, instructionSize, &bytesWriten);

        VirtualProtectEx(hProcess, (LPVOID)RapidFireAddr, instructionSize, OriginalPermission, &OriginalPermission);

    }
}

void InfiniteAmmo(HANDLE hProcess, Address modBaseAddr) {
    Address AmmoAddress = modBaseAddr + GLOBAL_AMMO_OFFSET;

    BYTE nops[] = { 0x90, 0x90 };
    size_t instructionSize = sizeof(nops);

    DWORD OriginalPermission;

    if (VirtualProtectEx(hProcess, (LPVOID)AmmoAddress, instructionSize, PAGE_EXECUTE_READWRITE, &OriginalPermission)) {
        size_t bytesWriten;
        WriteProcessMemory(hProcess, (LPVOID)AmmoAddress, nops, instructionSize, &bytesWriten);

        VirtualProtectEx(hProcess, (LPVOID)AmmoAddress, instructionSize, OriginalPermission, &OriginalPermission);
    }
}



int main() {
    const DWORD modBaseAddr = GetModuleBaseAddress(targetProcess, L"ac_client.exe");


    if (modBaseAddr == 0) {
        std::wcerr << L"Erro: Process not found" << std::endl;
        std::cin.get();
        return 1;
    }


    HANDLE hProcess = GetProcessHandle(targetProcess);

    if (hProcess == nullptr) {
        std::wcerr << L"Error: Cannot open proess" << std::endl;
        std::cin.get();
        return 1;
    }

    std::wcout << L"Base module: " << std::hex << modBaseAddr << std::endl;
    Address PlayerPointerAddress = modBaseAddr + PlayerPointer;

    RapidFire(hProcess, modBaseAddr);
    InfiniteAmmo(hProcess, modBaseAddr);

    /*while (true)
    {

        Address PrimeiroPonteiro = ReadMemory<Address>(hProcess, PlayerPointerAddress);


        Address LocalPlayerBase = ReadMemory<Address>(hProcess, PrimeiroPonteiro);

        if (LocalPlayerBase != 0)
        {
            Address healthAddress = LocalPlayerBase + HEALTH_OFFSET;

            int currentHealth = ReadMemory<int>(hProcess, healthAddress);

       
            WriteMemory<int>(hProcess, healthAddress, 1000);

            std::cout << "HP: " << std::dec << currentHealth << std::endl;
        }
        else
        {
            std::cout << "Awaiting player" << std::endl;
        }

        Address ptrAmmo1 = ReadMemory<Address>(hProcess, modBaseAddr + 0x00183828);
        Address ptrAmmo2 = ReadMemory<Address>(hProcess, ptrAmmo1 + 0x8);
        Address PrimaryWeaponBase = ReadMemory<Address>(hProcess, ptrAmmo2 + 0x73C);

        if (PrimaryWeaponBase != 0) {
            Address PrimaryAmmoAddress = PrimaryWeaponBase + 0x60C;
            WriteMemory<int>(hProcess, PrimaryAmmoAddress, 1000);
            int CurrentPrimaryAmmo = ReadMemory<int>(hProcess, PrimaryAmmoAddress);
            std::cout << "Primary Ammo: " << CurrentPrimaryAmmo << std::endl;
        }

        Address SecondaryWeaponBase = ReadMemory<Address>(hProcess, modBaseAddr + 0x0017E0A8);

        if (SecondaryWeaponBase != 0) {
            Address SecondaryAmmoAddress = SecondaryWeaponBase + 0x12C;
            WriteMemory<int>(hProcess, SecondaryAmmoAddress, 1000);
            int CurrentSecondaryAmmo = ReadMemory<int>(hProcess, SecondaryAmmoAddress);
            std::cout << "Secondary Ammo: " << CurrentSecondaryAmmo << std::endl;
        }

        Address GrenadeBase = ReadMemory<Address>(hProcess, modBaseAddr + 0x0017E0A8);

        if (GrenadeBase != 0) {
            Address GrenadeAddress = GrenadeBase + 0x144;
            WriteMemory<int>(hProcess, GrenadeAddress, 1000);
            int Grenade = ReadMemory<int>(hProcess, GrenadeAddress);
            std::cout << "Grenade: " << Grenade << std::endl;
        }

        Address RapidFirePtr1 = ReadMemory<Address>(hProcess, modBaseAddr + COOLDOWN_POINTER);
        if (RapidFirePtr1 != 0) {
            
            Address RapidFirePtr2 = ReadMemory<Address>(hProcess, RapidFirePtr1 + COOLDOWN_OFFSET_1);
            if (RapidFirePtr2 != 0) {
                
                Address RapidFirePtr3 = ReadMemory<Address>(hProcess, RapidFirePtr2 + COOLDOWN_OFFSET_2);
                if (RapidFirePtr3 != 0) {
                    Address FinalCoolDownAddress = RapidFirePtr3 + COOLDOWN_FINAL;

                    WriteMemory<int>(hProcess, FinalCoolDownAddress, 0);
                }
            }
        }
        Sleep(1000);

      /*  while (true) {
            Address RapidFirePtr1 = ReadMemory<Address>(hProcess, modBaseAddr + COOLDOWN_POINTER);
            if (RapidFirePtr1 != 0) {

                Address RapidFirePtr2 = ReadMemory<Address>(hProcess, RapidFirePtr1 + COOLDOWN_OFFSET_1);
                if (RapidFirePtr2 != 0) {

                    Address RapidFirePtr3 = ReadMemory<Address>(hProcess, RapidFirePtr2 + COOLDOWN_OFFSET_2);
                    if (RapidFirePtr3 != 0) {
                        Address FinalCoolDownAddress = RapidFirePtr3 + COOLDOWN_FINAL;

                        WriteMemory<int>(hProcess, FinalCoolDownAddress, 0);
                    }
                }
            }
        }*/
    

    CloseHandle(hProcess);
    return 0;
}
