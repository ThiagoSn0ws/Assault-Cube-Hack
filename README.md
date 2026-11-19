# AssaultCube External Trainer

An external memory trainer for [AssaultCube](https://assault.cubers.net/), the open-source first-person shooter. I built it to practice **Windows internals, process memory manipulation and reverse engineering** in C++.

The trainer runs as a separate process. It finds the game, opens a handle to it and changes the game's memory from outside, without injecting a DLL.

> ⚠️ **For educational use only.** Use it only offline or in single-player matches. Don't use it on public multiplayer servers: it ruins the game for other players and can get you banned.

---

## Features

| Feature | Status | How it works |
|---|---|---|
| **Infinite ammo** | ✅ Working | Overwrites the instruction that decrements ammo with `NOP`s |
| **Rapid fire** | ✅ Working | Overwrites the instruction that applies the weapon cooldown with `NOP`s |
| **God mode (health lock)** | 🚧 In progress | Follows a pointer chain to the local player and writes to the health value |
| **Primary / secondary ammo lock** | 🚧 In progress | Follows pointer chains to the weapon structures and writes the ammo value |
| **Infinite grenades** | 🚧 In progress | Follows a pointer chain and writes the grenade count |

---

## How it works

### 1. Finding the game process
The trainer takes a snapshot of the running processes with `CreateToolhelp32Snapshot` and walks through them with `Process32FirstW` / `Process32NextW` until it finds `ac_client.exe`.

### 2. Getting the module base address
Because of ASLR, the game is loaded at a different address every time it starts. So the trainer takes a module snapshot of the game process and reads the base address of `ac_client.exe`. All static offsets are added to this base.

### 3. Opening the process
`OpenProcess` is called with only the access rights the trainer needs:

```cpp
PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION
```

### 4. Patching code (NOP patching)
To get infinite ammo and rapid fire, the trainer overwrites a 2-byte instruction in the game's code with two `NOP`s (`0x90 0x90`). The code section is read-only, so the patch follows this sequence:

1. `VirtualProtectEx` changes the page to `PAGE_EXECUTE_READWRITE`.
2. `WriteProcessMemory` writes the `NOP` bytes.
3. `VirtualProtectEx` restores the original protection.

### 5. Reading and writing data (pointer chains)
Values like health and ammo live in memory that moves around while the game runs. They are reached by following **multi-level pointer chains** from a static base, for example:

```
[ac_client.exe + 0x183828] → +0x8 → +0x73C → +0x60C  =  primary ammo
```

Generic `ReadMemory<T>` and `WriteMemory<T>` templates wrap `ReadProcessMemory` / `WriteProcessMemory`, so any data type can be read or written the same way.

All offsets were found by reverse engineering the game with memory scanning and disassembly.

---

## Project structure

```
├── AssaultCubeHack2.cpp      # Entry point: process discovery, memory helpers, features
├── proc.h / proc.cpp         # Helpers for process ID and module base address lookup
├── AssaultCubeHack2.vcxproj  # Visual Studio project
└── README.md
```

---

## Building

**Requirements**
- Windows
- Visual Studio 2022 with the **Desktop development with C++** workload (toolset v143)

**Steps**
1. Clone the repository:
   ```bash
   git clone https://github.com/ThiagoSn0ws/Assault-Cube-Hack.git
   ```
2. Open `AssaultCubeHack2.vcxproj` in Visual Studio.
3. Select the **Release | x86 (Win32)** configuration. AssaultCube is a 32-bit game and the addresses are 32-bit.
4. Build with **Build → Build Solution** (`Ctrl+Shift+B`).

---

## Usage

1. Start AssaultCube and join an offline or single-player match.
2. Run the compiled `.exe`. If it can't open the process, run it as administrator.
3. The console prints the module base address, and the patches are applied right away.

> The offsets only work for the specific game version they were found in. If the game updates, they have to be found again.

---

## Roadmap

- [ ] Finish god mode, the ammo lock and infinite grenades
- [ ] Hotkeys to turn each feature on and off
- [ ] Restore the original bytes when a feature is turned off or the trainer exits
- [ ] Signature (pattern) scanning so the trainer keeps working after game updates
- [ ] A simple console menu showing the status of each feature

---

## What I learned

- How Windows processes, modules and virtual memory work
- Using the Win32 API for external memory access (`ToolHelp32`, `OpenProcess`, `ReadProcessMemory`, `WriteProcessMemory`, `VirtualProtectEx`)
- Finding static addresses and pointer chains through reverse engineering
- Changing program behavior at the assembly level with byte patching
- Memory page protections and why code sections are read-only

---

## Disclaimer

This project is for learning and research purposes only. AssaultCube is open-source software and is often used as a practice target for learning reverse engineering. The author is not responsible for misuse of this code.
