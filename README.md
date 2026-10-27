# Titanfall 2 VR · PSVR2 Haptics

**Adaptive trigger pushback, weapon-specific recoil, and incoming-damage headset feedback for CircuitLord’s Titanfall 2 VR campaign.**

[![Preview](https://img.shields.io/badge/preview-0.2.3-orange)](https://github.com/uzugu/titanfall2-psvr2-haptics/releases/tag/v0.2.3-preview)
![Platform](https://img.shields.io/badge/platform-Windows_x64-0078D4)
![Profiles](https://img.shields.io/badge/game_data_profiles-55-38bdf8)
[![Build](https://github.com/uzugu/titanfall2-psvr2-haptics/actions/workflows/build.yml/badge.svg)](https://github.com/uzugu/titanfall2-psvr2-haptics/actions/workflows/build.yml)

[Download preview](https://github.com/uzugu/titanfall2-psvr2-haptics/releases/tag/v0.2.3-preview) · [Installation](#installation) · [Weapon profiles](GAME_PROFILES.md) · [Report an issue](https://github.com/uzugu/titanfall2-psvr2-haptics/issues)

From a Pilot’s CAR to BT’s Predator Cannon, weapon profiles use Titanfall 2’s firing rates, recoil values and charge mechanics to drive distinct PSVR2 effects. This is a separate Northstar add-on for the VR mod, with no extra bridge app to launch.

> **Preview status:** Version 0.2.0 loaded in-game and logged shots, damage and hand changes. Version 0.2.3 passes four automated test suites; physical testing is pending. Its per-bullet pushback and left-hand reset changes still need physical confirmation. Headset rumble was not felt in the earlier play test; longer damage pulses are now implemented, but physical headset output remains unconfirmed.

## What it adds

- **Per-bullet trigger pushback:** each detected shot from weapons firing at least four rounds per second produces a strong resistance kick, then releases between bullets. Timing follows the effective firing cadence. Slower weapons use a short trigger release and restore their resistance.
- **55 game-data profiles:** 31 Pilot weapons and 24 Titan weapon, offensive ability and core entries. Ballistic, energy, scatter, rail, thermite, rotary and rocket effects use different waveform shapes.
- **Controller recoil:** PCM haptics accompany observed discharges. Titan recoil reaches both controllers, with a lighter support-hand effect.
- **Charge feedback:** rising trigger resistance and charge texture for selected weapons, including Plasma Railgun charge-stage ticks.
- **Reload feedback:** magazine removal, insertion and rack clicks. Racking a live round or removing a magazine is filtered out of shot detection.
- **Incoming damage:** controller pulses plus configurable headset rumble. Being hit in the head produces a higher-frequency, longer headset pulse. This is incoming headshot feedback, not a reward for shooting enemies in the head.
- **Hand routing and cleanup:** follows the VR mod’s reported weapon hand, resets both triggers when hands change, refreshes trigger state and releases stale gameplay output after 300 ms.
- **Automatic connection:** connects to the installed Toolkit CAPI in the background and retries when the driver is unavailable.

Effects follow observed game state. Holding fire without a detected shot does not generate extra kicks. Polling can miss multiple shots between frames, particularly at low frame rates.

## Built for Titans too

| Campaign loadout | Weapon | Implemented feedback |
| --- | --- | --- |
| Expedition | XO-16 Shorty | Fast mechanical kicks and both-controller recoil |
| Tone | 40 mm Tracker Cannon | Low cannon kick; separate tracking-rocket events |
| Scorch | T-203 Thermite Launcher | Heavy discharge with a textured thermite tail |
| Ronin | Leadwall | Scattergun snap and low recoil tail |
| Brute | Quad Rocket | Combined rocket discharge with lighter ADS feedback |
| Ion | Splitter Rifle | Energy sweep and stronger ADS texture |
| Northstar | Plasma Railgun | Rising resistance, charge-stage ticks and charged recoil |
| Legion | Predator Cannon | Short rotary kicks, spin texture and separate Power Shot |

Laser Core uses a sustained two-controller waveform. Selected offensive offhands and other cores have their own event paths. Defensive shields are excluded from gun recoil. See [GAME_PROFILES.md](GAME_PROFILES.md) for exact coverage and sources.

Game definitions supply numeric facts; the conversion to motor strength and waveform shape is authored tuning, not a physical measurement of the weapons.

## Requirements

1. **Titanfall 2 on Windows x64** with [CircuitLord’s VR mod](https://github.com/CircuitLord/CircuitLordVRModInstaller) installed into the game’s `TF2VR` folder.
2. **PSVR2 and Sense controllers**, using SteamVR on PC.
3. **[PSVR2 Toolkit experimental #2](https://github.com/BnuuySolutions/PSVR2Toolkit/releases/tag/v1.0.0-experimental-2)** or a compatible experimental CAPI installation.
4. **An unlocked headset for headset rumble**, using the relevant [vr2jb release](https://github.com/BnuuySolutions/vr2jb/releases/tag/v1.0.2). Controller effects use Toolkit; this add-on does not install or perform the headset unlock.
5. Microsoft Visual C++ 2015–2022 x64 runtime.

The native bridge targets Steam build **10350970** and the inspected Northstar **1.31.13.0** installation. It checks the client binary identity before registering game functions. Other binaries require offset validation; see [VALIDATION.md](VALIDATION.md).

## Installation

1. Close Titanfall 2.
2. Download and extract **[Titanfall2VR_PSVR2-0.2.3-preview.zip](https://github.com/uzugu/titanfall2-psvr2-haptics/releases/download/v0.2.3-preview/Titanfall2VR_PSVR2-0.2.3-preview.zip)** from Releases. GitHub’s “Source code” ZIP does not contain the built plugin.
3. Run `Install.ps1` from the extracted package:

   ```powershell
   .\Install.ps1 -GameDirectory 'C:\Program Files (x86)\Steam\steamapps\common\Titanfall2'
   ```

4. Launch VR through **CircuitLord’s installer**. It supplies the VR launch environment; starting the game executable directly may fail to initialize VR scripts.

The installer backs up this add-on’s existing files under `TF2VR/psvr2-backups` and preserves your INI settings. A fresh installation enables hardware output by default.

For manual installation, copy the package’s `plugins` and `mods` folders into `TF2VR`, preserving any INI customizations:

```text
Titanfall2/
└── TF2VR/
    ├── plugins/
    │   ├── Titanfall2VR_PSVR2.dll
    │   └── Titanfall2VR_PSVR2.ini
    └── mods/
        └── Titanfall2VR_PSVR2/
            ├── mod.json
            └── mod/scripts/vscripts/...
```

To uninstall, close the game and remove `TF2VR/plugins/Titanfall2VR_PSVR2.dll` and `TF2VR/mods/Titanfall2VR_PSVR2`. You can retain the INI and log.

## Configuration

Edit `TF2VR/plugins/Titanfall2VR_PSVR2.ini`, then restart the game.

| Setting | Default | Purpose |
| --- | --- | --- |
| `Enabled` | `1` | Enable device output |
| `LoggingOnly` | `0` | Set to `1` to log events without device commands |
| `AdaptiveTriggers` | `1` | Trigger resistance and per-shot pushback |
| `ControllerPCM` | `1` | Controller-body recoil and textures |
| `HeadsetRumble` | `1` | Incoming-damage headset pulses |
| `ControllerGain` | `0.70` | Controller PCM gain, from `0` to `1` |
| `HeadsetScale` | `0.65` | Headset frequency/duration scale, from `0` to `1` |

Headset commands run within **10–25 Hz**. `HeadsetScale` changes frequency and duration; the API does not provide independent headset amplitude. Ordinary gunfire uses the controllers.

Override an individual weapon using its exact class name:

```ini
[mp_titanweapon_leadwall]
Strength=6
Amplitude=0.75
DurationMs=110
```

Available overrides: `Start` (2–7), `End` (greater than Start, up to 8), `Strength` (0–8), `Amplitude` (0–1), `Frequency` (20–500 Hz, controller PCM), and `DurationMs` (10–250). Missing keys retain derived defaults. Automatic kicks add two strength steps, capped at eight; zero strength remains disabled. Firing cadence can shorten recoil duration.

## Troubleshooting and feedback

**No output:** check `Enabled=1` and `LoggingOnly=0`, and look for `Toolkit driver active` in `TF2VR/plugins/Titanfall2VR_PSVR2.log`. The release’s `tools/psvr2_probe.exe` checks CAPI discovery and driver activity without moving any motors.

**No headset rumble:** check `HeadsetRumble=1`, a nonzero `HeadsetScale`, and that the headset unlock is working. Look for `event=damage`, `event=explosion` or `event=headshot` in the log. An active Toolkit connection confirms the driver connection, not headset motor readiness. Physical headset output remains an open validation item.

**Wrong hand or stuck resistance:** report the weapon, handedness setting and whether switching hands or pausing clears it. Include nearby `state weapon=... hand=...` log lines. Avoid another utility continuously commanding the same triggers during testing.

**Script startup problems:** inspect the newest `TF2VR/logs/nslog*.txt`. Successful startup includes `Registered six PSVR2 natives in CLIENT VM` and `PSVR2 client script initialized` in the plugin log.

When [reporting an issue](https://github.com/uzugu/titanfall2-psvr2-haptics/issues), include the mod version, game/VR mod version, weapon or Titan loadout, the action that triggered it, and relevant log lines.

### Current limits

- Campaign-only client scripts; multiplayer is not supported.
- Shot detection observes ammo and attack timing, rather than receiving a native callback for every projectile.
- Reload feedback follows the weapon hand; separate magazine-hand routing is not implemented.
- No confirmed enemy-hit feedback, melee-contact effects, Titan footsteps, movement/time-shift effects or general nearby-explosion detection.
- Titan core/offhand coverage is limited to the mechanisms in [GAME_PROFILES.md](GAME_PROFILES.md).
- Pause cleanup depends on simulation time or script heartbeats stopping. Forced process termination cannot guarantee a final device-clear command.

## Build from source

Install Visual Studio 2022 C++ Build Tools, a Windows SDK, CMake 3.20+ and Node.js (for the catalog test), then run:

```powershell
git clone https://github.com/uzugu/titanfall2-psvr2-haptics.git
cd titanfall2-psvr2-haptics
.\package.ps1
```

The script builds x64 Release, runs the tests and writes the ZIP under `dist/`. Normal builds use the checked-in generated catalog; no game installation or research downloads are required.

The four suites cover effect timing/routing, catalog consistency, Northstar plugin loading and the real output worker against an isolated mock Toolkit DLL. The mock is never included in the release. Tests cannot verify how hardware feels or compile Squirrel inside the game VM.

| Folder | Contents |
| --- | --- |
| `src/` | Native plugin, effect scheduler and Toolkit output worker |
| `mod/` | Client Squirrel event detection and generated weapon allowlist |
| `data/` | Weapon facts with source hashes and provenance |
| `config/` | Default INI configuration |
| `scripts/` | Profile generation and pinned source download tools |
| `tests/` | Native and catalog tests, plus mock hardware |

## Credits and related projects

- [CircuitLord](https://github.com/CircuitLord/CircuitLordVRModInstaller) — Titanfall 2 VR installer and integration environment.
- [BnuuySolutions](https://github.com/BnuuySolutions) — PSVR2 Toolkit and vr2jb.
- [R2Northstar](https://github.com/R2Northstar/NorthstarLauncher) — Northstar plugin and Squirrel interfaces.
- [floh-bhaptics](https://github.com/floh-bhaptics/Titanfall2VR_bhaptics) — public Titanfall 2 VR haptics integration reference.
- [Syampuuh’s Titanfall 2 mirror](https://github.com/Syampuuh/Titanfall2) — pinned weapon-definition sources.

More PSVR2 projects by [uzugu](https://github.com/uzugu): **[GTFO VR](https://github.com/uzugu/GTFO_VR_Plugin)** · **[Into the Radius 2 Haptics](https://github.com/uzugu/itr2-psvr2-haptics)**.

Independent community add-on; not affiliated with Respawn, EA or Sony. Game binaries, Toolkit, jailbreak utilities and downloaded research sources are not included. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for attribution and third-party terms.
