# Profiles based on Titanfall 2 game definitions

Version 0.2.0 replaces the broad family-name heuristics with 55 exact game-class entries: 31 pilot weapons and 24 Titan weapons, offensive abilities and cores. Numeric facts are separate from the authored haptic mapping.

## Sources and scope

The baseline comes from the [Titanfall 2 client-file mirror](https://github.com/Syampuuh/Titanfall2/tree/9bfcea43ecf2a12fc3783ce22e63e439f422c045/scripts/weapons), pinned to commit `9bfcea43ecf2a12fc3783ce22e63e439f422c045`. The extraction follows `#base` inheritance, Windows conditionals and `SP_BASE`. It retains named upgrade blocks separately rather than mixing their values into the base weapon. These are snapshot defaults, not a claim to have unpacked every installed VPK.

The installed VR campaign `_sp_loadouts.nut` confirms all eight loadout classes below. The installed Northstar Predator Cannon definition was inspected as well. In-game effective `fire_rate`, actual next-attack transitions, ADS, charge fraction, maximum charge levels and core state feed the live effect scheduler. Thus a changed firing cadence does not rely solely on the archived base value.

`data/weapon-facts.json` records the source URL, content hash, selected numeric values and upgrade variants for every entry. `scripts/build-profiles.mjs` generates the native catalog and the script allowlist from the same input. Full game scripts are excluded from the release.

## Campaign Titan loadouts

| Loadout / gun | Source baseline | Haptic interpretation |
|---|---|---|
| Expedition — XO-16 Shorty | Inherits XO-16; 12 shots/s | Short mechanical kicks, both controllers; no generic slow Titan thud |
| Tone — 40 mm Tracker Cannon | 1.75 shots/s; pitch kick magnitude 1.2 | Low cannon kick with a brief mechanical tail; tracking rockets are separate offhand events |
| Scorch — T-203 Thermite Launcher | 1.7 shots/s; one-round base magazine | Heavy discharge with a longer textured thermite tail; no invented repeating shots while the projectile burns |
| Ronin — Leadwall | 2.5 shots/s; four-round base magazine; pitch kick magnitude 2.25 | Strong initial scattergun snap and low tail, distinct from the 40 mm |
| Brute — Quad Rocket | 1.5 attacks/s; four projectiles/ammo per hip-fire attack | One combined discharge, not four fictional sequential shots; ADS single-rocket mode is lighter |
| Ion — Splitter Rifle | 9 shots/s; ADS script emits three simultaneous bolts (five with upgrade) | Energy sweep; stronger ADS texture rather than false burst pulses |
| Northstar — Plasma Railgun | 0.9 shots/s; base charge 2.25 s / five levels | Rising resistance, stage ticks and charge-scaled discharge with a sharp energy component and low recoil tail |
| Legion — Predator Cannon | 15 shots/s; campaign magazine 150 | Short rotary pulses; spin texture driven by actual zoom/spool state; Power Shot is a separate cannon effect |

Shot timing comes from observed game events. The source `fire_rate` is used to keep the pulse shorter than a shot interval, not to fabricate events while a trigger is held. Missing multiple shots between frames remains a polling limitation.

## Titan abilities and cores

- Laser Core is a refreshed continuous two-controller waveform and steady trigger feedback. Its campaign definition lasts 6.5 seconds versus 3.5 in multiplayer; playback follows actual core activity instead of blindly running either fixed timer. Beam state expires if refreshes stop.
- Flight Core rockets, Salvo Core and Flame Core have separate catalog entries and waveform types. Flight Core can use next-attack advancement when the clip does not decrease. Physical/event validation remains required for each core.
- Multi-Target Missiles, Tracking Rockets, Slaved Warheads, Rocket Salvo, Cluster Missile, Laser Shot, Arc Wave, Firewall and Power Shot use a separate offensive-offhand event path. They preserve the primary weapon's trigger profile. Cooldown advancement is a fallback signal; charge cancellation alone is not a shot, and charging suppresses offhand discharge feedback.
- Vortex/Heat Shield, Particle Wall and other defensive abilities are excluded from the firearm/offensive allowlists. They must not receive recoil merely because they become the active weapon.
- Titan recoil reaches both controllers (support side at 70% of the main side). No repeating headset fire/beam effect is enabled; headset output remains incoming damage/explosions.

## Pilot weapons

The catalog individually identifies P2016, RE-45, both Wingmans, Smart Pistol, R-201, R-101, Flatline, Hemlok, G2, CAR, R-97, Alternator, Volt, Spitfire, Devotion, L-STAR, EVA-8, Mastiff, Mozambique, Longbow, Double Take, Kraber, EPG, Softball, SMR, Cold War, MGL, Thunderbolt, Archer and Charge Rifle.

Hemlok's three-shot burst and Cold War's burst/charge metadata are preserved. Each observed discharge drives recoil; the program does not synthesize an extra entire burst for every ammo change. Devotion's source ramp is 5 to 15 shots/s over 1.75 seconds; actual attack intervals control pulse length as it accelerates. Energy, ballistic, scatter and rocket mechanisms use different waveform shapes.

## What is still an authored choice

Titanfall does not contain PSVR2 trigger-force profiles or measured physical recoil. Trigger strength, waveform carrier frequencies, gain and the choice of low thud versus energy sweep are designed mappings. Default strength and amplitude scale with source pitch kick, while pulse duration is bounded by firing cadence. They still need physical testing and can be overridden in the INI.

Source `charge_time` is not automatically interpreted as a wind-up for every ability: some charge fields represent cooldown/resource behavior (for example Arc Wave). Only explicitly identified charge mechanisms receive charge feedback.

This update covers weapon feel and offensive Titan actions. It does not yet add Titan steps, movement, sword contact, shield impacts distinguished from damage, or confirmed enemy-hit feedback.

## Updating the catalog

Run `scripts/fetch-game-data.ps1` to fetch the pinned source files into ignored `research/game`, then run `node scripts/build-profiles.mjs`. Normal builds use the checked-in generated catalog and do not require network access or the research files.

If upgrading from preview 0.1.0, the installer preserves the existing INI. Its example shotgun/CAR/Charge Rifle sections were active overrides; remove those sections if you want the new generated defaults for those guns. The 0.2.0 sample config enables no overrides.
