// Northstar interface/VM ABI declarations: R2Northstar/NorthstarLauncher (MIT).
// See THIRD_PARTY_NOTICES.md. Gameplay and output implementation are independent.
#include "runtime.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
struct VM;
struct InitData { HMODULE handle; uint64_t size; };
enum class StringId : int { Name, Log, Dependency };
enum class FieldId : int { Context, Color };
struct IdInterface {
    virtual const char* GetString(StringId) = 0;
    virtual int64_t GetField(FieldId) = 0;
};
struct CallbackInterface {
    virtual void Init(HMODULE, const InitData*, bool) = 0;
    virtual void Finalize() = 0;
    virtual bool Unload() = 0;
    virtual void OnSqvmCreated(VM*) = 0;
    virtual void OnSqvmDestroying(VM*) = 0;
    virtual void OnLibraryLoaded(HMODULE, const char*) = 0;
    virtual void RunFrame() = 0;
};
using Native = int32_t(*)(void*);
struct Registration {
    const char *name, *cppName, *help, *returnName, *args;
    uint32_t unknown1, devLevel;
    const char* shortName;
    uint32_t unknown2;
    int32_t returnType;
    uint32_t* buffer;
    uint64_t size, unknown3, unknown4;
    Native function;
};
static_assert(sizeof(Registration) == 0x68);
static_assert(offsetof(Registration, function) == 0x60);
static_assert(offsetof(Registration, returnType) == 0x3c);
using RegisterFn = int64_t(*)(VM*, Registration*, char);
using StringFn = const char*(*)(void*, int32_t);
using FloatFn = float(*)(void*, int32_t);
StringFn getString = nullptr;
FloatFn getFloat = nullptr;
VM* clientVm = nullptr;
HMODULE selfModule = nullptr;
int Context(VM* vm) { return *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(vm) + 0x3c); }
int32_t NativeState(void* vm) { p2::State(getString(vm, 1), getString(vm, 2), getFloat(vm, 3)); return 0; }
int32_t NativeEvent(void* vm) { p2::Event(getString(vm, 1), getFloat(vm, 2)); return 0; }
int32_t NativeStop(void*) { p2::StopEffects(); return 0; }
int32_t NativeLog(void* vm) { const char* s = getString(vm, 1); if (s) p2::Log(s); return 0; }
int32_t NativeTune(void* vm) { p2::Tune(getFloat(vm, 1), getFloat(vm, 2), getFloat(vm, 3), getFloat(vm, 4), getFloat(vm, 5)); return 0; }
int32_t NativeWeaponEvent(void* vm) { p2::WeaponEvent(getString(vm, 1), getFloat(vm, 2)); return 0; }
Registration registrations[6]{};
bool ExecutableAddress(uintptr_t address) {
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info))) return false;
    return info.State == MEM_COMMIT && !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
        (info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));
}
void Register(VM* vm) {
    const auto module = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!module) { p2::Log("ERROR: client.dll unavailable; natives not registered."); return; }
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    auto pe = reinterpret_cast<const IMAGE_NT_HEADERS64*>(module + dos->e_lfanew);
    // Fail closed on unknown game binaries rather than calling arbitrary offsets.
    // Verified local Steam build 10350970, client.dll SHA256 in README.
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.TimeDateStamp != 1512510036 ||
        pe->OptionalHeader.SizeOfImage != 64274432 ||
        !ExecutableAddress(module + 0x108e0) || !ExecutableAddress(module + 0x60c0) || !ExecutableAddress(module + 0x6100)) {
        p2::Log("ERROR: unsupported client.dll identity. Disable the script mod; native registration refused."); return;
    }
    auto registerFn = reinterpret_cast<RegisterFn>(module + 0x108e0);
    getString = reinterpret_cast<StringFn>(module + 0x60c0);
    getFloat = reinterpret_cast<FloatFn>(module + 0x6100);
    const char* names[] = {"P2_State", "P2_Event", "P2_Stop", "P2_Log", "P2_Tune", "P2_WeaponEvent"};
    const char* args[] = {"string weapon, string hand, float charge", "string eventName, float amount", "", "string message",
        "float rate, float ads, float beam, float spool, float levels", "string weapon, float amount"};
    Native callbacks[] = {NativeState, NativeEvent, NativeStop, NativeLog, NativeTune, NativeWeaponEvent};
    for (int i = 0; i < 6; ++i) {
        auto& r = registrations[i];
        r = {};
        r.name = r.cppName = names[i]; r.help = "PSVR2 haptics bridge";
        r.returnName = "void"; r.args = args[i]; r.returnType = 0x20; r.function = callbacks[i];
        registerFn(vm, &r, 1);
    }
    clientVm = vm;
    p2::Log("Registered six PSVR2 natives in CLIENT VM.");
}
struct Identity final : IdInterface {
    const char* GetString(StringId id) override {
        switch (id) {
            case StringId::Name: return "Titanfall2VR_PSVR2";
            case StringId::Log: return "PSVR2";
            case StringId::Dependency: return "TF2VR_PSVR2";
        }
        return nullptr;
    }
    int64_t GetField(FieldId id) override { return id == FieldId::Context ? 2 : 0xE0B050; }
} identity;
struct Callbacks final : CallbackInterface {
    void Init(HMODULE, const InitData* data, bool) override {
        if (data && data->handle) selfModule = data->handle;
        p2::StartRuntime(selfModule);
    }
    void Finalize() override {}
    bool Unload() override {
        p2::StopEffects();
        // The VM retains function pointers into this module; never hot-unload underneath it.
        if (clientVm) { p2::Log("Unload refused while CLIENT VM owns native pointers."); return false; }
        return p2::StopRuntime();
    }
    void OnSqvmCreated(VM* vm) override { if (vm && Context(vm) == 1) Register(vm); }
    void OnSqvmDestroying(VM* vm) override {
        if (vm == clientVm) { p2::StopEffects(); clientVm = nullptr; p2::Log("CLIENT VM destroyed; effects stopped."); }
    }
    void OnLibraryLoaded(HMODULE, const char*) override {}
    void RunFrame() override {}
} callbacks;
}
extern "C" __declspec(dllexport) void* CreateInterface(const char* name, int* status) {
    void* result = nullptr;
    if (name && std::strcmp(name, "PluginId001") == 0) result = &identity;
    if (name && std::strcmp(name, "PluginCallbacks001") == 0) result = &callbacks;
    if (status) *status = result ? 0 : 1;
    return result;
}
BOOL WINAPI DllMain(HMODULE module, DWORD reason, void*) {
    if (reason == DLL_PROCESS_ATTACH) { selfModule = module; DisableThreadLibraryCalls(module); }
    return TRUE;
}
