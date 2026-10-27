#include "runtime.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <functional>

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; p2::StopRuntime(); std::exit(1); }
}
bool Await(const std::function<bool()>& condition) {
    auto deadline = GetTickCount64() + 250;
    do { if (condition()) return true; Sleep(5); } while (GetTickCount64() < deadline);
    return condition();
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 1;
    auto dir = std::filesystem::temp_directory_path() / (L"tf2vr_psvr2_mock_" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::create_directory(dir);
    SetEnvironmentVariableW(L"TMP", dir.c_str());
    SetEnvironmentVariableW(L"TEMP", dir.c_str());
    auto mockPath = std::filesystem::absolute(argv[1]);
    { std::ofstream out(dir / L"psvr2tk_capi_path.txt"); out << mockPath.parent_path().u8string(); }
    HMODULE module = LoadLibraryW(mockPath.c_str());
    Check(module != nullptr, "mock CAPI loads");
    auto read = reinterpret_cast<int(*)(int)>(GetProcAddress(module, "mock_read"));
    auto active = reinterpret_cast<void(*)(int)>(GetProcAddress(module, "mock_active"));
    auto failure = reinterpret_cast<void(*)(int)>(GetProcAddress(module, "mock_failure"));
    auto staleRight = reinterpret_cast<void(*)()>(GetProcAddress(module, "mock_stale_right"));
    Check(read && active && failure && staleRight, "mock diagnostic exports");
    p2::StartRuntime(GetModuleHandleW(nullptr));
    // Keep state fresh while the asynchronous loader initializes.
    for (int i = 0; i < 20 && read(1) != 2; ++i) { p2::State("mp_weapon_car", "right", 0); Sleep(25); }
    Check(read(1) == 2 && read(0) == 0, "native ABI sends weapon command to right hand");
    int writes = read(3);
    p2::Event("shot", 1);
    Check(Await([&] { return read(3) > writes; }), "worker streams PCM through CAPI");
    p2::State("mp_weapon_car", "left", 0);
    Check(Await([&] { return read(0) == 2 && read(1) == 0; }), "worker releases former firing hand");
    staleRight();
    Check(Await([&] { return read(1) == 0; }), "periodic reset clears stale offhand hardware state");
    p2::State("mp_weapon_car", "left", 0);
    // The worker's brief release can fall entirely between test-thread polls.
    // Count actual mock commands rather than sampling a transient motor state.
    int releases = read(5);
    int kicks = read(6);
    p2::Event("shot", 1);
    Check(Await([&] { return read(6) > kicks && read(1) == 0; }), "automatic bullet sends left-trigger pushback command");
    Check(Await([&] { return read(5) > releases; }), "automatic bullet pushback releases");
    p2::Event("damage", 1);
    Check(Await([&] { return read(2) >= 10 && read(2) <= 25; }), "worker sends bounded headset command");
    Sleep(350);
    Check(read(0) == 0 && read(1) == 0 && read(2) == 0, "watchdog clears physical output commands");
    failure(-3); p2::State("mp_weapon_car", "right", 0); Sleep(30);
    Check(read(1) == 0, "failed trigger write is not treated as physical success");
    failure(0); p2::State("mp_weapon_car", "right", 0);
    Check(Await([&] { return read(1) == 2; }), "failed output is retried");
    active(0); Sleep(160);
    active(1); Sleep(160);
    Check(read(1) == 0, "reconnect does not resurrect stale weapon effect");
    p2::State("mp_weapon_car", "right", 1);
    Check(Await([&] { return read(1) == 5; }), "charging after reconnect");
    Check(p2::StopRuntime(), "worker shuts down within timeout");
    Check(read(0) == 0 && read(1) == 0 && read(2) == 0 && read(4) == 1, "shutdown clears all outputs and releases slot");
    FreeLibrary(module);
    std::cout << "Mock CAPI: streaming, watchdog, hand routing, API failure, reconnect and shutdown passed.\n";
}
