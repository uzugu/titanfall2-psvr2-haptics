#include "runtime.h"
#include "effects.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <cmath>

namespace p2 {
namespace {
double Now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct Runtime {
    std::mutex mutex, logMutex;
    std::unique_ptr<Effects> effects;
    std::ofstream log;
    std::atomic<bool> run{false};
    HANDLE worker = nullptr;
    bool loggingOnly = false;
    std::string lastWeapon, lastHand;
};
// Intentionally process lifetime: no joining threads under Windows loader lock.
Runtime& R() { static auto* r = new Runtime; return *r; }
using InitFn = int(*)();
using DeinitFn = void(*)();
using ActiveFn = bool(*)();
using TriggerFn = int(*)(uint8_t, const Trigger&);
using HmdFn = int(*)(uint8_t);
using PcmFn = int(*)(uint8_t, const unsigned char*);
struct Capi {
    HMODULE dll = nullptr;
    InitFn init = nullptr, wait = nullptr;
    DeinitFn deinit = nullptr;
    ActiveFn active = nullptr;
    TriggerFn trigger = nullptr;
    HmdFn hmd = nullptr;
    PcmFn pcm = nullptr;
    bool initialized = false;
    template<class T> T Symbol(const char* name) { return reinterpret_cast<T>(GetProcAddress(dll, name)); }
    bool Load() {
        wchar_t temp[MAX_PATH]{};
        if (!GetTempPathW(MAX_PATH, temp)) return false;
        std::ifstream input(std::filesystem::path(temp) / L"psvr2tk_capi_path.txt", std::ios::binary);
        std::string location((std::istreambuf_iterator<char>(input)), {});
        if (location.size() >= 3 && location.substr(0, 3) == "\xEF\xBB\xBF") location.erase(0, 3);
        auto end = location.find_last_not_of("\r\n\t ");
        if (end == std::string::npos) return false;
        location.resize(end + 1);
        auto path = std::filesystem::u8path(location) / L"psvr2_toolkit_capi.dll";
        dll = LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!dll) return false;
        init = Symbol<InitFn>("psvr2_toolkit_init");
        deinit = Symbol<DeinitFn>("psvr2_toolkit_deinit");
        active = Symbol<ActiveFn>("psvr2_toolkit_get_driver_active");
        trigger = Symbol<TriggerFn>("psvr2_toolkit_set_trigger_effect");
        hmd = Symbol<HmdFn>("psvr2_toolkit_set_hmd_rumble");
        pcm = Symbol<PcmFn>("psvr2_toolkit_write_pcm");
        wait = Symbol<InitFn>("psvr2_toolkit_wait_for_pcm");
        if (!init || !deinit || !active || !trigger || !hmd || !pcm || !wait) { Close(); return false; }
        initialized = init() == 0;
        if (!initialized) { Close(); return false; }
        Log("Toolkit experimental CAPI loaded; waiting for active driver.");
        return true;
    }
    void Clear() {
        if (!initialized || !active()) return;
        Trigger off;
        std::array<uint8_t, 32> zero{};
        trigger(2, off); hmd(0); pcm(2, zero.data());
    }
    void Close() {
        if (initialized) { Clear(); deinit(); }
        initialized = false;
        if (dll) FreeLibrary(dll);
        dll = nullptr;
    }
};
DWORD WINAPI Worker(void*) {
    Capi capi;
    try {
        Output previous;
        bool connected = false, force = true;
        double nextLoad = 0, lastRefresh = 0, lastError = 0, lastTriggerRefresh = 0, lastLogFlush = 0;
        while (R().run) {
            double now = Now();
            if (now - lastLogFlush >= 1) {
                std::lock_guard<std::mutex> lock(R().logMutex);
                R().log.flush(); lastLogFlush = now;
            }
            if (R().loggingOnly) { Sleep(20); continue; }
            if (!capi.initialized) {
                if (now >= nextLoad) {
                    nextLoad = now + 2;
                    if (!capi.Load() && now - lastError > 10) { Log("Toolkit unavailable; retrying in background."); lastError = now; }
                }
                Sleep(20); continue;
            }
            if (!capi.active()) {
                if (connected) { Log("Toolkit driver disconnected; clearing local effects."); StopEffects(); }
                connected = false; force = true;
                Sleep(100); continue;
            }
            if (!connected) { Log("Toolkit driver active."); connected = true; force = true; }
            Output out;
            {
                std::lock_guard<std::mutex> lock(R().mutex);
                out = R().effects->Render(now);
            }
            auto check = [&](int result, const char* operation) {
                if (result == 0) return true;
                if (now - lastError > 2) {
                    Log(std::string(operation) + " returned " + std::to_string(result)); lastError = now;
                }
                return false;
            };
            bool success = true;
            bool refreshTriggers = now - lastTriggerRefresh >= .10;
            // Clear both physical triggers before applying a changed hand's profile.
            bool changedHand = (previous.triggers[0].mode && !out.triggers[0].mode && out.triggers[1].mode) ||
                (previous.triggers[1].mode && !out.triggers[1].mode && out.triggers[0].mode);
            if (changedHand) success = check(capi.trigger(2, Trigger{}), "hand-change reset") && success;
            for (uint8_t hand = 0; hand < 2; ++hand) {
                if (force || changedHand || refreshTriggers || out.triggers[hand] != previous.triggers[hand])
                    success = check(capi.trigger(hand, out.triggers[hand]), "trigger") && success;
            }
            if (refreshTriggers) lastTriggerRefresh = now;
            if (force || out.hmd != previous.hmd || (out.hmd && now - lastRefresh >= .025)) {
                success = check(capi.hmd(out.hmd), "HMD") && success;
                lastRefresh = now;
            }
            bool streaming = out.pcmActive[0] || out.pcmActive[1];
            for (uint8_t hand = 0; hand < 2; ++hand) {
                if (force || out.pcmActive[hand] || previous.pcmActive[hand])
                    success = check(capi.pcm(hand, out.pcm[hand].data()), "PCM write") && success;
            }
            previous = out; force = !success;
            if (streaming && success) {
                int result = capi.wait();
                if (!check(result, "PCM wait")) Sleep(20);
                // Some driver states can signal immediately: avoid a busy loop.
                else Sleep(1);
            } else Sleep(success ? 5 : 50);
        }
    } catch (const std::exception& e) { Log(std::string("Worker stopped: ") + e.what()); }
    capi.Close();
    Log("Output worker stopped; effects cleared.");
    return 0;
}
float ReadFloat(const std::filesystem::path& path, const wchar_t* section, const wchar_t* key, float fallback) {
    wchar_t text[64]{};
    GetPrivateProfileStringW(section, key, L"", text, 64, path.c_str());
    if (!text[0]) return fallback;
    try { return std::stof(text); } catch (...) { return fallback; }
}
Config ReadConfig(const std::filesystem::path& path) {
    Config c;
    auto flag = [&](const wchar_t* key, bool fallback) {
        return GetPrivateProfileIntW(L"General", key, fallback ? 1 : 0, path.c_str()) != 0;
    };
    c.enabled = flag(L"Enabled", true); c.loggingOnly = flag(L"LoggingOnly", false);
    c.triggers = flag(L"AdaptiveTriggers", true); c.pcm = flag(L"ControllerPCM", true); c.hmd = flag(L"HeadsetRumble", true);
    c.gain = ReadFloat(path, L"General", L"ControllerGain", .7f);
    c.hmdScale = ReadFloat(path, L"General", L"HeadsetScale", .65f);
    wchar_t sections[16384]{};
    GetPrivateProfileSectionNamesW(sections, 16384, path.c_str());
    for (auto* section = sections; *section; section += wcslen(section) + 1) {
        std::wstring s(section);
        if (s.rfind(L"mp_", 0) != 0) continue;
        if (std::any_of(s.begin(), s.end(), [](wchar_t ch) { return ch > 127; })) continue;
        std::string weapon;
        for (wchar_t ch : s) weapon.push_back(static_cast<char>(ch));
        Profile p = DefaultProfile(weapon);
        p.start = GetPrivateProfileIntW(section, L"Start", p.start, path.c_str());
        p.end = GetPrivateProfileIntW(section, L"End", p.end, path.c_str());
        p.strength = GetPrivateProfileIntW(section, L"Strength", p.strength, path.c_str());
        p.amplitude = ReadFloat(path, section, L"Amplitude", p.amplitude);
        p.frequency = ReadFloat(path, section, L"Frequency", p.frequency);
        p.durationMs = ReadFloat(path, section, L"DurationMs", p.durationMs);
        c.overrides.emplace(weapon, Validate(p));
    }
    return c;
}
}
void Log(const std::string& text) {
    std::lock_guard<std::mutex> lock(R().logMutex);
    if (R().log) {
        R().log << GetTickCount64() << " [PSVR2] " << text << '\n';
        // Avoid flushing disk on every automatic-weapon shot from the game thread.
        if (text.rfind("event=", 0) != 0) R().log.flush();
    }
}
void StartRuntime(HMODULE self) {
    if (R().worker) return;
    wchar_t buffer[32768]{};
    GetModuleFileNameW(self, buffer, 32768);
    auto directory = std::filesystem::path(buffer).parent_path();
    R().log.open(directory / L"Titanfall2VR_PSVR2.log", std::ios::app);
    Config config = ReadConfig(directory / L"Titanfall2VR_PSVR2.ini");
    R().loggingOnly = config.loggingOnly || !config.enabled;
    R().effects = std::make_unique<Effects>(config);
    Log(std::string("v0.2.2 started; ") + (R().loggingOnly ? "logging only (no device writes)." : "hardware output enabled."));
    R().run = true;
    R().worker = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
    if (!R().worker) { R().run = false; Log("Could not start output worker."); }
}
bool StopRuntime() {
    R().run = false;
    if (R().worker) {
        if (WaitForSingleObject(R().worker, 2000) != WAIT_OBJECT_0) return false;
        CloseHandle(R().worker); R().worker = nullptr;
    }
    { std::lock_guard<std::mutex> lock(R().logMutex); R().log.close(); }
    return true;
}
void State(const char* weapon, const char* hand, float charge) {
    std::lock_guard<std::mutex> lock(R().mutex);
    if (!R().effects || !weapon || !hand) return;
    if (R().lastWeapon != weapon || R().lastHand != hand) {
        R().lastWeapon = weapon; R().lastHand = hand;
        Log(std::string("state weapon=") + weapon + " hand=" + hand);
    }
    int physicalHand = std::string(hand) == "left" ? 0 : (std::string(hand) == "right" ? 1 : -1);
    R().effects->State(weapon, physicalHand, charge, Now());
}
void Event(const char* event, float amount) {
    std::lock_guard<std::mutex> lock(R().mutex);
    if (!R().effects || !event) return;
    Log(std::string("event=") + event + " amount=" + std::to_string(amount));
    R().effects->Event(event, amount, Now());
}
void StopEffects() {
    std::lock_guard<std::mutex> lock(R().mutex);
    if (R().effects) R().effects->Stop();
    R().lastWeapon.clear(); R().lastHand.clear();
}
void Tune(float rate, float ads, float beam, float spool, float levels) {
    std::lock_guard<std::mutex> lock(R().mutex);
    if (R().effects) R().effects->Tune(rate, ads, beam, spool,
        std::isfinite(levels) ? static_cast<int>(std::clamp(levels, 0.f, 10.f)) : 0, Now());
}
void WeaponEvent(const char* weapon, float amount) {
    std::lock_guard<std::mutex> lock(R().mutex);
    if (!R().effects || !weapon) return;
    Log(std::string("ability=") + weapon);
    R().effects->WeaponEvent(weapon, amount, Now());
}
}
