#pragma once
#include <string_view>
namespace p2 {
enum class Waveform { Ballistic, Scatter, Cannon, Energy, Rail, Thermite, Rotary, Rocket, Beam };
enum class WeaponRole { Primary, Ability, Core };
struct WeaponFacts {
    const char* id;
    const char* name;
    Waveform waveform;
    WeaponRole role;
    bool titan;
    float fireRate, pitchKick, chargeTime;
    int chargeLevels;
    float sustainedDuration, burstCount, maxFireRate;
};
const WeaponFacts* FindWeaponFacts(std::string_view id);
}
