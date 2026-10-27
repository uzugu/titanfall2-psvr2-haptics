#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "weapon_facts.h"

namespace p2 {
struct Profile {
    int start = 3, end = 6, strength = 4;
    float amplitude = .45f, frequency = 85.f, durationMs = 65.f;
    Waveform waveform = Waveform::Ballistic;
    bool titan = false;
    float fireRate = 0;
};
struct Config {
    bool enabled = true, loggingOnly = false, triggers = true, pcm = true, hmd = true;
    float gain = .7f, hmdScale = .65f;
    std::unordered_map<std::string, Profile> overrides;
};
// Binary ABI of the Toolkit experimental-2 trigger command, independently declared.
struct Trigger {
    int32_t mode = 0;
    uint32_t padding = 0;
    std::array<uint8_t, 48> data{};
    bool operator==(const Trigger& other) const { return mode == other.mode && data == other.data; }
    bool operator!=(const Trigger& other) const { return !(*this == other); }
};
static_assert(sizeof(Trigger) == 56);
struct Output {
    std::array<Trigger, 2> triggers{};
    std::array<std::array<uint8_t, 32>, 2> pcm{};
    std::array<bool, 2> pcmActive{};
    uint8_t hmd = 0;
};
Profile DefaultProfile(const std::string& weapon);
Profile Validate(Profile p);
class Effects {
public:
    explicit Effects(Config config = {}) : config_(std::move(config)) {}
    void State(std::string weapon, int hand, float charge, double now);
    void Event(const std::string& event, float amount, double now);
    void Tune(float fireRate, float ads, float beam, float spool, int chargeLevels, double now);
    void WeaponEvent(const std::string& weapon, float amount, double now);
    void Stop();
    Output Render(double now);
    size_t Pending() const { return impulses_.size(); }
private:
    struct Impulse { int hand; double start, duration; float amplitude, frequency; Waveform waveform; };
    void Pulse(int hand, float amplitude, float hz, double duration, double now, Waveform waveform = Waveform::Ballistic);
    void Recoil(const Profile& profile, float amount, double now);
    Config config_;
    Profile profile_;
    std::string weapon_;
    int hand_ = 1;
    float charge_ = 0;
    float ads_ = 0, spool_ = 0, fireRate_ = 0;
    int chargeStep_ = 0;
    double beamUntil_ = 0, tuningUntil_ = 0, firingUntil_ = 0;
    double heartbeat_ = -100, releaseUntil_ = 0, hmdUntil_ = 0;
    uint8_t hmdHz_ = 0;
    std::vector<Impulse> impulses_;
};
}
