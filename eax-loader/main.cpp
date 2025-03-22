#define _CRT_SECURE_NO_WARNINGS
#include "includes.h"

DWORD procFind(const std::wstring& processName) {
    PROCESSENTRY32W process_info;
    process_info.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE processes_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (processes_snapshot == INVALID_HANDLE_VALUE) return 0;

    if (Process32FirstW(processes_snapshot, &process_info)) {
        do {
            if (wcscmp(process_info.szExeFile, processName.c_str()) == 0) {
                CloseHandle(processes_snapshot);
                return process_info.th32ProcessID;
            }
        } while (Process32NextW(processes_snapshot, &process_info));
    }

    CloseHandle(processes_snapshot);
    return 0;
}

HANDLE getHandle(const int perms = PROCESS_ALL_ACCESS) {
    DWORD pid = procFind(L"csgo.exe");

    if (!pid) {
        std::cout << "[!] CSGO process not found!\n";
        return INVALID_HANDLE_VALUE;
    }

    std::cout << "[+] CSGO found! PID: " << pid << std::endl;
    return OpenProcess(perms, FALSE, pid);
}


int main() {
    HANDLE csgo_handle;
    std::cout << R"(
         /$$$$$$$$  /$$$$$$  /$$   /$$       /$$        /$$$$$$   /$$$$$$  /$$$$$$$  /$$$$$$$$ /$$$$$$$ 
        | $$_____/ /$$__  $$| $$  / $$      | $$       /$$__  $$ /$$__  $$| $$__  $$| $$_____/| $$__  $$
        | $$      | $$  \ $$|  $$/ $$/      | $$      | $$  \ $$| $$  \ $$| $$  \ $$| $$      | $$  \ $$
        | $$$$$   | $$$$$$$$ \  $$$$/       | $$      | $$  | $$| $$$$$$$$| $$  | $$| $$$$$   | $$$$$$$/
        | $$__/   | $$__  $$  >$$  $$       | $$      | $$  | $$| $$__  $$| $$  | $$| $$__/   | $$__  $$
        | $$      | $$  | $$ /$$/\  $$      | $$      | $$  | $$| $$  | $$| $$  | $$| $$      | $$  \ $$
        | $$$$$$$$| $$  | $$| $$  \ $$      | $$$$$$$$|  $$$$$$/| $$  | $$| $$$$$$$/| $$$$$$$$| $$  | $$
        |________/|__/  |__/|__/  |__/      |________/ \______/ |__/  |__/|_______/ |________/|__/  |__/
                                     
)";
    SetConsoleTitleA("eax");

    while ((csgo_handle = getHandle()) == INVALID_HANDLE_VALUE) {
        std::cout << "[!] Waiting for CSGO to start...\n";
        Sleep(5000);
    }

    char fullPath[MAX_PATH];
    GetFullPathNameA("eax.dll", MAX_PATH, fullPath, nullptr);

    void* csgo1_module = VirtualAllocEx(csgo_handle, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!csgo1_module) {
        std::cerr << "[!] Failed to allocate memory in target process.\n";
        return -1;
    }

    if (!WriteProcessMemory(csgo_handle, csgo1_module, fullPath, strlen(fullPath) + 1, nullptr)) {
        std::cerr << "[!] Failed to write DLL path to process memory.\n";
        return -1;
    }

    void* loadLibraryAddr = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));
    if (!loadLibraryAddr) {
        std::cerr << "[!] Failed to get address of LoadLibraryA.\n";
        return -1;
    }

    HANDLE remote_thread = CreateRemoteThread(csgo_handle, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr), csgo1_module, 0, nullptr);

    if (!remote_thread) {
        std::cerr << "[!] Failed to create remote thread.\n";
        return -1;
    }

    std::cout << "[+] Injection successful!\n";

    CloseHandle(remote_thread);
    CloseHandle(csgo_handle);
    exit(0);

    return 0;
}
