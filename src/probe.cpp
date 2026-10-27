#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

// Readiness check only. Never sends trigger, PCM, or headset motor commands.
int main() {
    wchar_t temp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp);
    auto pathFile = std::filesystem::path(temp) / L"psvr2tk_capi_path.txt";
    std::ifstream input(pathFile, std::ios::binary);
    std::string location((std::istreambuf_iterator<char>(input)), {});
    if (location.size() >= 3 && location.substr(0, 3) == "\xEF\xBB\xBF") location.erase(0, 3);
    auto end = location.find_last_not_of("\r\n\t ");
    if (end == std::string::npos) { std::cerr << "Toolkit discovery file is missing or empty.\n"; return 1; }
    location.resize(end + 1);
    auto path = std::filesystem::u8path(location) / L"psvr2_toolkit_capi.dll";
    HMODULE module = LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!module) { std::cerr << "Cannot load Toolkit: " << GetLastError() << '\n'; return 2; }
    auto init = reinterpret_cast<int(*)()>(GetProcAddress(module, "psvr2_toolkit_init"));
    auto deinit = reinterpret_cast<void(*)()>(GetProcAddress(module, "psvr2_toolkit_deinit"));
    auto active = reinterpret_cast<bool(*)()>(GetProcAddress(module, "psvr2_toolkit_get_driver_active"));
    bool exports = init && deinit && active;
    for (const char* name : {"psvr2_toolkit_set_trigger_effect", "psvr2_toolkit_set_hmd_rumble", "psvr2_toolkit_write_pcm", "psvr2_toolkit_wait_for_pcm"})
        exports = GetProcAddress(module, name) && exports;
    if (!exports) { std::cerr << "Required experimental CAPI exports missing.\n"; FreeLibrary(module); return 3; }
    int result = init();
    bool running = result == 0 && active();
    std::cout << "Toolkit init=" << result << ", driverActive=" << running << ". No motor commands sent.\n";
    if (result == 0) deinit();
    FreeLibrary(module);
    return running ? 0 : 4;
}
