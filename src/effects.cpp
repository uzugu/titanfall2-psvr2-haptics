#include "effects.h"
#include "weapon_facts.generated.h"
#include <algorithm>
#include <cmath>

namespace p2 {
namespace {
float Finite(float x, float fallback) { return std::isfinite(x) ? x : fallback; }
}
const WeaponFacts* FindWeaponFacts(std::string_view id) {
    for (const auto& fact : kWeaponFacts) if (id == fact.id) return &fact;
    return nullptr;
}
Profile Validate(Profile p) {
    p.start = std::clamp(p.start, 2, 7);
    p.end = std::clamp(p.end, p.start + 1, 8);
    p.strength = std::clamp(p.strength, 0, 8);
    p.amplitude = std::clamp(Finite(p.amplitude, .4f), 0.f, 1.f);
    p.frequency = std::clamp(Finite(p.frequency, 85.f), 20.f, 500.f);
    p.durationMs = std::clamp(Finite(p.durationMs, 65.f), 10.f, 250.f);
    p.fireRate = std::clamp(Finite(p.fireRate, 0), 0.f, 100.f);
    return p;
}
Profile DefaultProfile(const std::string& w) {
    Profile p;
    const auto* fact = FindWeaponFacts(w);
    if (!fact) return p;
    // Facts determine cadence and relative recoil. Conversion to motor strength is authored tuning.
    p.waveform = fact->waveform; p.titan = fact->titan; p.fireRate = fact->fireRate;
    float kick = std::min(fact->pitchKick, 6.f);
    p.strength = std::clamp(static_cast<int>(std::lround(3 + kick * .65f + (p.titan ? 1 : 0))), 2, 7);
    p.start = p.fireRate >= 10 ? 2 : 3; p.end = p.titan ? 7 : 6;
    p.amplitude = std::clamp(.28f + .07f * kick + (p.titan ? .14f : 0.f), .25f, .8f);
    p.durationMs = 45.f + 18.f * kick + (p.titan ? 30.f : 0.f);
    switch (p.waveform) {
        case Waveform::Ballistic: p.frequency = p.titan ? 65.f : 100.f; break;
        case Waveform::Scatter: p.frequency = 58; p.durationMs += 20; break;
        case Waveform::Cannon: p.frequency = 48; p.durationMs += 30; p.amplitude += .1f; break;
        case Waveform::Energy: p.frequency = 135; break;
        case Waveform::Rail: p.frequency = 95; p.durationMs += 35; break;
        case Waveform::Thermite: p.frequency = 42; p.durationMs += 80; break;
        case Waveform::Rotary: p.frequency = 75; break;
        case Waveform::Rocket: p.frequency = 55; p.durationMs += 35; break;
        case Waveform::Beam: p.frequency = 150; break;
    }
    if (p.fireRate > 0) p.durationMs = std::min(p.durationMs, 750.f / p.fireRate);
    return Validate(p);
}
void Effects::Stop() {
    weapon_.clear(); charge_ = 0; heartbeat_ = -100;
    impulses_.clear(); releaseUntil_ = hmdUntil_ = 0; hmdHz_ = 0;
    beamUntil_ = tuningUntil_ = firingUntil_ = 0; ads_ = spool_ = fireRate_ = 0; chargeStep_ = 0;
}
void Effects::State(std::string weapon, int hand, float charge, double now) {
    if (hand < 0 || hand > 1 || !std::isfinite(now)) { Stop(); return; }
    if (weapon != weapon_ || hand != hand_) {
        impulses_.clear(); releaseUntil_ = 0;
        beamUntil_ = tuningUntil_ = firingUntil_ = 0; ads_ = spool_ = 0; chargeStep_ = 0;
        profile_ = DefaultProfile(weapon);
        auto it = config_.overrides.find(weapon);
        if (it != config_.overrides.end()) profile_ = it->second;
        profile_ = Validate(profile_);
        fireRate_ = profile_.fireRate;
    }
    weapon_ = std::move(weapon); hand_ = hand;
    charge_ = std::clamp(Finite(charge, 0), 0.f, 1.f);
    heartbeat_ = now;
}
void Effects::Pulse(int hand, float amplitude, float hz, double duration, double now, Waveform waveform) {
    if (impulses_.size() >= 64) impulses_.erase(impulses_.begin());
    impulses_.push_back({hand, now, duration, amplitude, hz, waveform});
}
void Effects::Tune(float fireRate, float ads, float beam, float spool, int chargeLevels, double now) {
    if (now - heartbeat_ > .30 || !std::isfinite(now)) return;
    fireRate_ = fireRate > 0 && std::isfinite(fireRate) ? std::clamp(fireRate, .1f, 100.f) : profile_.fireRate;
    ads_ = std::clamp(Finite(ads, 0), 0.f, 1.f);
    spool_ = profile_.waveform == Waveform::Rotary ? std::clamp(Finite(spool, 0), 0.f, 1.f) : 0;
    beamUntil_ = profile_.waveform == Waveform::Beam && beam > .5f ? now + .15 : 0;
    tuningUntil_ = now + .15;
    int step = static_cast<int>(charge_ * std::clamp(chargeLevels, 0, 10));
    if (profile_.waveform == Waveform::Rail && step > chargeStep_ && config_.enabled && !config_.loggingOnly)
        Pulse(hand_, .12f, 170.f + step * 8.f, .018, now, Waveform::Energy);
    chargeStep_ = step;
}
void Effects::Recoil(const Profile& p, float amount, double now) {
    if (p.waveform == Waveform::Beam) return; // sustained beam has its own refreshed state
    float intensity = std::clamp(amount, .1f, 1.f);
    float duration = p.durationMs;
    float rate = &p == &profile_ ? fireRate_ : p.fireRate;
    if (rate > 0) duration = std::min(duration, std::max(10.f, 750.f / rate));
    float modeScale = &p == &profile_ && weapon_ == "mp_titanweapon_particle_accelerator" ? 1.f + .25f * ads_ : 1.f;
    if (&p == &profile_ && weapon_ == "mp_titanweapon_rocketeer_rocketstream") modeScale = 1.15f - .25f * ads_;
    Pulse(hand_, p.amplitude * intensity * modeScale, p.frequency, duration / 1000., now, p.waveform);
    if (p.titan) Pulse(1 - hand_, p.amplitude * intensity * modeScale * .7f, p.frequency, duration / 1000., now, p.waveform);
}
void Effects::WeaponEvent(const std::string& weapon, float amount, double now) {
    if (!config_.enabled || config_.loggingOnly || now - heartbeat_ > .30 || !std::isfinite(amount)) return;
    const auto* fact = FindWeaponFacts(weapon);
    if (!fact || !fact->titan || fact->role != WeaponRole::Ability) return;
    auto p = DefaultProfile(weapon);
    auto it = config_.overrides.find(weapon);
    if (it != config_.overrides.end()) p = Validate(it->second);
    Recoil(p, amount, now);
}
void Effects::Event(const std::string& event, float amount, double now) {
    if (!config_.enabled || config_.loggingOnly || !std::isfinite(now) || !std::isfinite(amount)) return;
    if (event == "death") {
        auto until = hmdUntil_; auto hz = hmdHz_;
        Stop(); hmdUntil_ = until; hmdHz_ = hz;
        return;
    }
    bool damage = event == "damage" || event == "explosion" || event == "headshot";
    if (!damage && (now - heartbeat_ > .30 || now < heartbeat_)) return;
    if (damage && amount <= 0) return;
    amount = std::clamp(amount, 0.f, 1.f);
    if (event == "shot" && !weapon_.empty() && profile_.waveform != Waveform::Beam) {
        Recoil(profile_, amount, now);
        releaseUntil_ = now + .025; // short trigger release, then restore equipped profile
        if (fireRate_ >= 4.f) {
            // Each observed bullet pushes back, then releases before the next bullet.
            // No free-running oscillation: holding fire without a shot adds no kick.
            firingUntil_ = now + std::min(.045, .45 / fireRate_);
            releaseUntil_ = now + std::min(.080, .85 / fireRate_);
        }
    } else if (event == "mag_out" || event == "mag_in" || event == "rack" || event == "dry") {
        Pulse(hand_, event == "mag_in" ? .4f : .25f, event == "rack" ? 150.f : 100.f, .035, now);
    } else if (damage) {
        float intensity = std::clamp(amount, .1f, 1.f);
        for (int h = 0; h < 2; ++h) Pulse(h, .25f + .4f * intensity, 65.f, .09, now);
        float scale = std::clamp(Finite(config_.hmdScale, 0), 0.f, 1.f);
        if (config_.hmd && scale > 0) {
            bool headshot = event == "headshot";
            uint8_t hz = static_cast<uint8_t>(10 + std::lround((headshot ? 15.f : 6.f + 5.f * intensity) * scale));
            if (now >= hmdUntil_) hmdHz_ = hz;
            else hmdHz_ = std::max(hmdHz_, hz);
            hmdUntil_ = std::max(hmdUntil_, now + (headshot ? .22 : .10 + .06 * intensity) * scale + .04);
        }
    }
}
Output Effects::Render(double now) {
    Output out;
    if (!std::isfinite(now) || now < heartbeat_) { Stop(); return out; }
    if (now - heartbeat_ > .30) {
        // Damage is independently bounded and may arrive on the fatal frame.
        auto until = hmdUntil_; auto hz = hmdHz_;
        Stop(); hmdUntil_ = until; hmdHz_ = hz;
        out.hmd = config_.enabled && !config_.loggingOnly && config_.hmd && now < until ? hz : 0;
        return out;
    }
    if (!config_.enabled || config_.loggingOnly) return out;
    if (config_.triggers && !weapon_.empty() && (now >= releaseUntil_ || now < firingUntil_) &&
        (profile_.waveform != Waveform::Beam || charge_ > .01f || now < beamUntil_)) {
        auto& t = out.triggers[hand_];
        if (now < firingUntil_) {
            t.mode = 1; // Feedback pushes against a held trigger beyond the weapon break point.
            t.data[0] = static_cast<uint8_t>(profile_.start);
            t.data[1] = static_cast<uint8_t>(profile_.strength == 0 ? 0 : std::min(8, profile_.strength + 2));
        } else if (charge_ > .01f) {
            t.mode = 5; // slope: positions 0..9, strengths 1..8
            t.data[0] = 2; t.data[1] = 8; t.data[2] = 2;
            t.data[3] = static_cast<uint8_t>(3 + std::lround(5 * charge_));
        } else if (profile_.waveform == Waveform::Beam) {
            t.mode = 1; t.data[0] = 2; t.data[1] = 3;
        } else {
            t.mode = 2;
            t.data[0] = static_cast<uint8_t>(profile_.start);
            t.data[1] = static_cast<uint8_t>(profile_.end);
            t.data[2] = static_cast<uint8_t>(profile_.strength);
        }
    }
    out.hmd = config_.hmd && now < hmdUntil_ ? hmdHz_ : 0;
    impulses_.erase(std::remove_if(impulses_.begin(), impulses_.end(), [now](const Impulse& p) {
        return now >= p.start + p.duration;
    }), impulses_.end());
    if (!config_.pcm) return out;
    constexpr double tau = 6.283185307179586;
    float gain = std::clamp(Finite(config_.gain, 0), 0.f, 1.f);
    for (int h = 0; h < 2; ++h) {
        for (size_t i = 0; i < 32; ++i) {
            double time = now + i / 3000.;
            double sample = 0;
            if ((h == hand_ || profile_.titan) && charge_ > .01f && !weapon_.empty()) {
                out.pcmActive[h] = true;
                sample = (.04 + .12 * charge_) * (h == hand_ ? 1. : .65) * std::sin(tau * (80 + charge_ * 80) * time);
            }
            if (profile_.titan && now < tuningUntil_) {
                if (now < beamUntil_) {
                    out.pcmActive[h] = true;
                    sample += .23 * (1 + .2 * std::sin(tau * 10 * time)) * std::sin(tau * 150 * time);
                }
                if (spool_ > .01f) {
                    out.pcmActive[h] = true;
                    sample += .075 * spool_ * std::sin(tau * (35 + spool_ * 55) * time);
                }
            }
            for (const auto& p : impulses_) {
                double age = time - p.start;
                if (p.hand != h || age < 0 || age >= p.duration) continue;
                out.pcmActive[h] = true;
                double u = age / p.duration;
                double wave = 0;
                switch (p.waveform) {
                    case Waveform::Energy: wave = std::exp(-4*u)*std::cos(tau*(p.frequency*age + 120*age*age/p.duration)); break;
                    case Waveform::Rail: wave = .65*std::exp(-4*u)*std::cos(tau*(220*age - 70*age*age/p.duration)) + .35*std::exp(-3*u)*std::cos(tau*45*age); break;
                    case Waveform::Thermite: wave = std::exp(-2.8*u)*(.65*std::cos(tau*p.frequency*age)+.2*std::sin(tau*113*age)+.15*std::sin(tau*191*age)); break;
                    case Waveform::Rocket: wave = .35*std::exp(-12*u)*std::cos(tau*190*age)+.65*std::exp(-3*u)*std::cos(tau*p.frequency*age); break;
                    case Waveform::Cannon: wave = std::exp(-4*u)*std::cos(tau*p.frequency*age)+.2*std::exp(-60*std::abs(age-.018))*std::cos(tau*110*age); break;
                    case Waveform::Scatter: wave = .7*std::exp(-4*u)*std::cos(tau*p.frequency*age)+.3*std::exp(-12*u)*std::cos(tau*230*age); break;
                    case Waveform::Rotary: wave = std::exp(-7*u)*(.8*std::cos(tau*p.frequency*age)+.2*std::cos(tau*210*age)); break;
                    default: wave = std::exp(-5*u)*(.8*std::cos(tau*p.frequency*age)+.2*std::cos(tau*230*age)); break;
                }
                sample += p.amplitude * wave;
            }
            int v = static_cast<int>(std::lround(127 * std::tanh(sample * gain)));
            out.pcm[h][i] = static_cast<uint8_t>(static_cast<int8_t>(std::clamp(v, -127, 127)));
        }
    }
    return out;
}
}
