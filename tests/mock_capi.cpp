#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "effects.h"
#include <atomic>
namespace {
std::atomic<int> active{1}, left{0}, right{0}, hmd{0}, pcmWrites{0}, deinitializations{0}, failure{0};
std::atomic<int> leftReleases{0}, leftKicks{0};
}
extern "C" {
__declspec(dllexport) int psvr2_toolkit_init() { return 0; }
__declspec(dllexport) void psvr2_toolkit_deinit() { ++deinitializations; }
__declspec(dllexport) bool psvr2_toolkit_get_driver_active() { return active != 0; }
__declspec(dllexport) int psvr2_toolkit_wait_for_pcm() { Sleep(10); return failure; }
__declspec(dllexport) int psvr2_toolkit_set_trigger_effect(uint8_t hand, const p2::Trigger& command) {
    if (failure) return failure;
    if ((hand == 0 || hand == 2) && command.mode == 0 && left != 0) ++leftReleases;
    if (hand == 0 && command.mode == 1) ++leftKicks;
    if (hand == 0 || hand == 2) left = command.mode;
    if (hand == 1 || hand == 2) right = command.mode;
    return 0;
}
__declspec(dllexport) int psvr2_toolkit_set_hmd_rumble(uint8_t hz) { if (failure) return failure; hmd = hz; return 0; }
__declspec(dllexport) int psvr2_toolkit_write_pcm(uint8_t, const unsigned char*) { ++pcmWrites; return failure; }
__declspec(dllexport) int mock_read(int field) {
    switch (field) { case 0: return left; case 1: return right; case 2: return hmd; case 3: return pcmWrites; case 4: return deinitializations; case 5: return leftReleases; case 6: return leftKicks; }
    return -1;
}
__declspec(dllexport) void mock_active(int value) { active = value; }
__declspec(dllexport) void mock_failure(int value) { failure = value; }
__declspec(dllexport) void mock_stale_right() { right = 2; }
}
