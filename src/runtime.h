#pragma once
#include <windows.h>
#include <string>

namespace p2 {
void StartRuntime(HMODULE self);
bool StopRuntime();
void State(const char* weapon, const char* hand, float charge);
void Event(const char* event, float amount);
void Tune(float rate, float ads, float beam, float spool, float levels);
void WeaponEvent(const char* weapon, float amount);
void StopEffects();
void Log(const std::string& text);
}
