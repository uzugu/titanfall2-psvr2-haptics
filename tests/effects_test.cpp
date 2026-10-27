#include "effects.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
bool Nonzero(const std::array<uint8_t, 32>& data) {
    for (auto sample : data) if (sample != 0) return true;
    return false;
}
int main() {
    using namespace p2;
    Effects e;
    Check(e.Render(0).triggers[1].mode == 0, "idle starts released");
    e.State("mp_weapon_shotgun", 1, 0, 1);
    auto idle = e.Render(1);
    Check(idle.triggers[1].mode == 2 && idle.triggers[0].mode == 0, "right hand weapon only");
    e.Event("shot", 1, 1.01);
    auto shot = e.Render(1.01);
    Check(shot.triggers[1].mode == 0, "trigger release on shot");
    Check(Nonzero(shot.pcm[1]) && !Nonzero(shot.pcm[0]), "recoil only in weapon hand");
    Check(shot.hmd == 0, "ordinary gunfire does not rumble headset");
    Check(e.Render(1.05).triggers[1].mode == 2, "trigger resistance restored");
    e.State("mp_weapon_defender", 0, 1, 1.1);
    auto charge = e.Render(1.1);
    Check(charge.triggers[0].mode == 5 && charge.triggers[0].data[3] == 8, "full charge resistance");
    Check(charge.triggers[1].mode == 0 && e.Pending() == 0, "hand change clears old recoil");
    e.State("mp_weapon_defender", 0, 0, 1.15);
    Check(!Nonzero(e.Render(1.15).pcm[0]), "charge cancellation has no discharge");
    e.Event("damage", 1, 1.16);
    auto hit = e.Render(1.16);
    Check(hit.hmd >= 10 && hit.hmd <= 25, "HMD frequency within actual motor limits");
    Check(Nonzero(hit.pcm[0]) && Nonzero(hit.pcm[1]), "damage affects both hands");
    Check(e.Render(1.31).hmd == 0, "headset pulse expires");
    auto stale = e.Render(1.46);
    Check(stale.triggers[0].mode == 0 && stale.hmd == 0 && e.Pending() == 0, "watchdog clears all effects");
    e.Event("shot", 1, 1.5);
    Check(e.Pending() == 0, "stale events ignored");
    e.State("mp_weapon_car", 1, 0, 2);
    for (int i = 0; i < 1000; ++i) e.Event("shot", 1, 2);
    Check(e.Pending() == 64, "recoil queue bounded");
    e.Stop();
    Check(e.Pending() == 0 && e.Render(2).triggers[1].mode == 0, "explicit stop releases");
    Config logging; logging.loggingOnly = true;
    Effects dry(logging); dry.State("mp_weapon_car", 1, 1, 3); dry.Event("damage", 1, 3);
    auto output = dry.Render(3);
    Check(output.triggers[1].mode == 0 && output.hmd == 0 && !Nonzero(output.pcm[1]), "logging only has no outputs");
    Config disabled; disabled.enabled = false;
    Effects off(disabled); off.State("mp_weapon_car", 1, 1, 4); off.Event("shot", 1, 4);
    Check(off.Render(4).triggers[1].mode == 0 && off.Pending() == 0, "disabled mode");
    Config toggles; toggles.triggers = toggles.pcm = toggles.hmd = false;
    Effects toggled(toggles); toggled.State("mp_weapon_car", 1, 1, 5); toggled.Event("damage", 1, 5);
    output = toggled.Render(5);
    Check(output.triggers[1].mode == 0 && output.hmd == 0 && !output.pcmActive[1], "independent output toggles");
    Profile bad{99, -3, 255, std::numeric_limits<float>::quiet_NaN(), -10, 10000};
    auto safe = Validate(bad);
    Check(safe.start == 7 && safe.end == 8 && safe.strength == 8 && std::isfinite(safe.amplitude) && safe.durationMs == 250, "profile validation");
    Config custom; custom.overrides["mp_weapon_car"] = {4, 7, 7, .2f, 90, 30};
    Effects overrideTest(custom); overrideTest.State("mp_weapon_car", 0, 0, 6);
    Check(overrideTest.Render(6).triggers[0].data[2] == 7, "exact weapon override");
    overrideTest.State("", 0, 0, 6.1);
    Check(overrideTest.Render(6.1).triggers[0].mode == 0, "holstering releases trigger");
    const auto* xo = FindWeaponFacts("mp_titanweapon_xo16_shorty");
    const auto* rail = FindWeaponFacts("mp_titanweapon_sniper");
    const auto* laser = FindWeaponFacts("mp_titancore_laser_cannon");
    Check(xo && xo->fireRate == 12, "XO16 Shorty inherits base weapon facts");
    Check(rail && rail->chargeLevels == 5 && rail->chargeTime == 2.25f, "railgun base charge facts exclude named upgrades");
    Check(laser && laser->sustainedDuration == 6.5f, "Laser Core uses SP duration rather than 3.5-second MP value");
    Check(FindWeaponFacts("mp_weapon_hemlok")->burstCount == 3, "Hemlok real three-round burst metadata");
    Check(FindWeaponFacts("mp_weapon_esaw")->maxFireRate == 15, "Devotion max spool rate metadata");
    auto predator = DefaultProfile("mp_titanweapon_predator_cannon");
    auto leadwall = DefaultProfile("mp_titanweapon_leadwall");
    auto thermite = DefaultProfile("mp_titanweapon_meteor");
    Check(predator.waveform != leadwall.waveform && thermite.waveform != leadwall.waveform, "Titan mechanisms have distinct waveform types");
    Check(predator.durationMs <= 50 && leadwall.durationMs > predator.durationMs, "high-rate cannon recoil fits actual shot interval");
    Effects titan;
    titan.State("mp_titanweapon_xo16_shorty", 1, 0, 10);
    titan.Event("shot", 1, 10);
    auto titanShot = titan.Render(10);
    Check(Nonzero(titanShot.pcm[0]) && Nonzero(titanShot.pcm[1]), "Titan chassis recoil reaches both controllers");
    titan.State("mp_titanweapon_predator_cannon", 1, 0, 11);
    titan.Tune(15, 1, 0, 1, 0, 11);
    auto spin = titan.Render(11.01);
    Check(spin.pcmActive[0] && spin.pcmActive[1], "Legion spin motor follows actual zoom/spool state");
    Check(!titan.Render(11.16).pcmActive[1], "spool refresh timeout expires without new tuning");
    titan.State("mp_titanweapon_particle_accelerator", 1, 0, 12);
    titan.Tune(9, 0, 0, 0, 0, 12);
    titan.Event("shot", 1, 12);
    auto hip = titan.Render(12);
    titan.Stop(); titan.State("mp_titanweapon_particle_accelerator", 1, 0, 12);
    titan.Tune(9, 1, 0, 0, 0, 12); titan.Event("shot", 1, 12);
    auto split = titan.Render(12);
    Check(split.pcm[1][0] > hip.pcm[1][0] && titan.Pending() == 2, "Splitter ADS strengthens one discharge, not fake sequential shots");
    titan.Stop(); titan.State("mp_titancore_laser_cannon", 1, 0, 13);
    titan.Tune(8, 0, 1, 0, 0, 13);
    titan.Event("shot", 1, 13);
    auto beam = titan.Render(13.01);
    Check(titan.Pending() == 0 && beam.pcmActive[0] && beam.pcmActive[1], "Laser Core uses sustained waveform rather than discrete recoil");
    Check(beam.triggers[1].mode == 1, "beam feedback differs from firearm trigger break");
    Check(!titan.Render(13.16).pcmActive[1], "beam ends on missing core-state refresh");
    titan.Stop(); titan.State("mp_titanweapon_sticky_40mm", 1, 0, 14);
    titan.WeaponEvent("mp_titanweapon_vortex_shield", 1, 14);
    Check(titan.Pending() == 0, "shield cannot emit offensive recoil");
    titan.WeaponEvent("mp_titanweapon_tracker_rockets", 1, 14);
    Check(titan.Pending() == 2 && titan.Render(14).triggers[1].mode == 2, "ordnance recoil preserves primary trigger profile");
    titan.Stop(); titan.State("mp_titanweapon_sniper", 0, .6f, 15);
    titan.Tune(.9f, 1, 0, 0, 5, 15);
    Check(titan.Pending() == 1, "railgun charge-stage tick");
    titan.Tune(.9f, 1, 0, 0, 5, 15.01);
    Check(titan.Pending() == 1, "holding same rail charge does not repeat ticks");
    Effects damage;
    damage.Event("damage", .001f, 20);
    Check(damage.Render(20.05).hmd >= 10, "small damage works without equipped gun or heartbeat");
    damage.Stop(); damage.Event("damage", 1, 21);
    int bodyHz = damage.Render(21).hmd;
    damage.Stop(); damage.Event("headshot", .001f, 22);
    Check(damage.Render(22).hmd > bodyHz, "even small headshot is stronger than full body hit");
    damage.Event("death", 0, 22.01);
    Check(damage.Render(22.15).hmd > 0 && damage.Render(22.15).triggers[0].mode == 0,
        "fatal hit tail survives death while triggers clear");
    Check(damage.Render(22.3).hmd == 0, "fatal hit tail expires independently");
    damage.Event("damage", 0, 23);
    Check(damage.Render(23).hmd == 0, "zero damage does not activate headset");
    Effects automatic;
    automatic.State("mp_weapon_rspn101", 0, 0, 30);
    automatic.Tune(12, 0, 0, 0, 0, 30);
    for (int i = 0; i < 3; ++i) {
        double time = 30 + i / 12.;
        automatic.Event("shot", 1, time);
        auto firing = automatic.Render(time + .02);
        Check(firing.triggers[0].mode == 1 && firing.triggers[0].data[1] > DefaultProfile("mp_weapon_rspn101").strength && firing.triggers[1].mode == 0,
            "each automatic bullet pushes back on weapon trigger with stronger feedback");
        Check(automatic.Render(time + .055).triggers[0].mode == 0,
            "trigger releases between automatic bullet kicks");
    }
    automatic.State("mp_weapon_rspn101", 0, 0, 30.28);
    Check(automatic.Render(30.28).triggers[0].mode == 2, "normal resistance returns after last bullet without more kicks");
    automatic.State("mp_weapon_rspn101", 1, 0, 30.29);
    Check(automatic.Render(30.29).triggers[0].mode == 0, "hand switch clears automatic effect");
    automatic.State("mp_weapon_rspn101", -1, 0, 30.3);
    Check(automatic.Render(30.3).triggers[1].mode == 0, "unknown hand cannot default to right resistance");
    std::cout << "All effect lifecycle, routing, bounds, and waveform checks passed.\n";
}
