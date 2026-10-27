#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

struct Id {
    virtual const char* GetString(int) = 0;
    virtual int64_t GetField(int) = 0;
};
struct InitData { HMODULE handle; uint64_t size; };
struct Callbacks {
    virtual void Init(HMODULE, const InitData*, bool) = 0;
    virtual void Finalize() = 0;
    virtual bool Unload() = 0;
    virtual void OnSqvmCreated(void*) = 0;
    virtual void OnSqvmDestroying(void*) = 0;
    virtual void OnLibraryLoaded(HMODULE, const char*) = 0;
    virtual void RunFrame() = 0;
};
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 1;
    auto dir = std::filesystem::temp_directory_path() / (L"tf2vr_psvr2_smoke_" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::create_directory(dir);
    auto dll = dir / L"Titanfall2VR_PSVR2.dll";
    auto ini = dir / L"Titanfall2VR_PSVR2.ini";
    auto log = dir / L"Titanfall2VR_PSVR2.log";
    std::filesystem::copy_file(argv[1], dll);
    { std::ofstream out(ini); out << "[General]\nLoggingOnly=1\n"; }
    HMODULE module = LoadLibraryW(dll.c_str());
    if (!module) { std::cerr << "LoadLibrary failed " << GetLastError(); return 1; }
    using Factory = void*(*)(const char*, int*);
    auto create = reinterpret_cast<Factory>(GetProcAddress(module, "CreateInterface"));
    if (!create) return 1;
    int status = -1;
    auto id = static_cast<Id*>(create("PluginId001", &status));
    if (!id || status || std::strcmp(id->GetString(2), "TF2VR_PSVR2") || id->GetField(0) != 2) return 1;
    if (create("InvalidInterface", &status) || status != 1) return 1;
    auto callbacks = static_cast<Callbacks*>(create("PluginCallbacks001", nullptr));
    if (!callbacks) return 1;
    InitData data{module, sizeof(InitData)};
    callbacks->Init(nullptr, &data, false);
    callbacks->Finalize();
    callbacks->OnSqvmCreated(nullptr);
    callbacks->RunFrame();
    callbacks->OnSqvmDestroying(nullptr);
    Sleep(70);
    if (!callbacks->Unload()) return 1;
    FreeLibrary(module);
    std::ifstream in(log);
    std::string text((std::istreambuf_iterator<char>(in)), {});
    if (text.find("logging only (no device writes)") == std::string::npos || text.find("Output worker stopped") == std::string::npos)
        return 1;
    // The runtime logger deliberately has process lifetime; keep its diagnostic file.
    std::cout << "Plugin ABI, load, logging-only worker and clean unload passed. Log: " << log << '\n';
}
