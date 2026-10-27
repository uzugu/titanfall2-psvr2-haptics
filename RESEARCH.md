# Titanfall 2 VR: PSVR2 integration assessment

Reviewed 2026-10-27. This document records the initial research. A subsequent 0.1.0 preview is now implemented and packaged; see README.md and VALIDATION.md for its narrower current scope and test status. No game installation or driver changes have been made.

## Conclusion

A separate Northstar native C++ plug-in plus Squirrel scripts is a practical route for PSVR2 adaptive triggers, controller PCM haptics, and jailbroken headset rumble in CircuitLord's Titanfall 2 VR campaign. The public bHaptics/ProTube add-on demonstrates the game integration route. Our GTFO implementation supplies relevant output and effect-design experience, but its Unity/C# hooks cannot be dropped into Titanfall.

The installer is open source; the VR mod itself is closed source. Build an independent add-on under `TF2VR/plugins` and `TF2VR/mods`, using exposed game/script facilities. There is no need to modify the installer or redistribute the VR mod.

## Versions and sources

- [Installer](https://github.com/CircuitLord/CircuitLordVRModInstaller): fetched manifest-v3.json advertises stable Titanfall2VR 1.0.25, with a separate crashreport-fix 1.0.26 channel. Installer tree SHA observed: 46e02ae0c84cb187c6374fe61d38034085cd6224.
- [Existing haptics add-on](https://github.com/floh-bhaptics/Titanfall2VR_bhaptics): source inspected on master, reporting v1.6.0. Reference files in `research/` are upstream snapshots, not our implementation or a release package. Branch content can change.
- [Toolkit experimental #2](https://github.com/BnuuySolutions/PSVR2Toolkit/releases/tag/v1.0.0-experimental-2): inspected tagged CAPI, implementation, common constants and trigger definitions.
- [vr2jb 1.0.2](https://github.com/BnuuySolutions/vr2jb/releases/tag/v1.0.2): enables eye-camera access and HMD rumble. Its patcher is separate from the game integration.

## Hardware controls verified in the tagged API

| Output | Available control | Design implication |
|---|---|---|
| Sense adaptive triggers | Off, feedback, weapon, vibration, slope, and ten-position feedback/vibration modes; left/right/both targeting | Different mechanical feel per gun and per hand |
| Weapon trigger mode | Start 2–7, end start+1 through 8, strength 0–8 | Validate per-mode limits; equal start/end is invalid in this mode |
| Trigger vibration | Position 0–9, amplitude 0–8, frequency 0–255 Hz | Trigger-motor pulse, separate from controller-body PCM |
| Controller PCM | 3,000 samples/sec, 32-sample chunks | Custom recoil snap, low kick, energy sweep and reload clicks; stream on a worker |
| HMD rumble | Frequency command: 0 stops, effective running range 10–25 Hz; 1–9 maps to 10; above 25 rejected | Shape pulses by frequency and timing; no independent amplitude or arbitrary headset PCM in this API |
| Gaze | Gaze status and image API | Separate future work; does not automatically add eye aiming or foveated rendering to Titanfall |

Adaptive triggers/controller PCM are Toolkit features; the jailbreak specifically adds the relevant HMD rumble access. Target SteamVR with the Toolkit driver active. Verify hardware capability at runtime rather than assuming CAPI initialization means the headset motor is available.

Experimental #2 returns `int` results from trigger, HMD, PCM write and PCM wait calls. Our GTFO CAPI wrapper currently declares these as `void`: do not reuse that signature unchanged. Handle inactive driver, no slot, timeout and invalid parameter results. Initialization can precede driver startup, so retry availability without blocking gameplay.

The workspace's `PSVR2Toolkit/include/psvr2_toolkit_capi.h` is an older API. Use the tagged `projects/psvr2_toolkit_capi/psvr2tk_capi.h` as the version reference.

## Existing work to reuse conceptually

- `GTFO_VR_Plugin/GTFO_VR/Core/PSVR2/PSVR2HapticsManager.cs`: weapon profiles, charging, damage, transient trigger restoration, main-hand routing and HMD sequencing.
- `PSVR2PcmHaptics.cs`: bounded overlapping impulses, 3 kHz/32-sample streaming, kick/snap synthesis, energy sweeps and support-hand recoil.
- `PSVR2CapiClient.cs`: installed DLL discovery through the temporary path file, dynamic exports and cleanup. Update signatures and error handling for experimental #2.
- `projects/VRChat-PSVR2-Headpat`: timed HMD envelopes, periodic refresh and release behavior. Higher frequency is a tuning control, not a calibrated independent force/amplitude setting.
- `Pistol-Whip-Adaptive-Triggers/Core.cs`: weapon/hand-dependent trigger profiles. Its older IPC and Unity integration are less suitable than direct CAPI for this target.

## Game signals and proposed effects

These signals are evidenced in the reference add-on's source; PSVR2 behavior below is proposed and has not been tested in game.

| Event | Signal available in reference | Proposed effect |
|---|---|---|
| Gunfire | Weapon class, clip transitions, next attack time; special charge/Flight Core logic | Gun-specific trigger break plus PCM recoil |
| Weapon hand | `TF2VR_WeaponHand()`; 0 left, 1 right | Route recoil/resistance to the actual firing hand |
| Charge Rifle / Plasma Railgun | `GetWeaponChargeFraction()` plus weapon-specific input state | Rising resistance and energy vibration; distinct discharge |
| Reload | `tf2vr_reload_magazine_out`, clip increases, `tf2vr_reload_rounds` | Separate extraction, insertion and bolt clicks |
| Player damage | `AddLocalPlayerTookDamageCallback` by damage source | Short HMD hit pulse and controller feedback |
| Grenades / holsters | VR convar tickets and command notifications | Pickup, stow, draw and throw cues |
| Movement | Server movement callbacks for jump, double jump, dash, land, mantle, wallrun; zipline callbacks | Restrained controller movement effects; brief landing HMD pulse |
| Titan combat | Cockpit callback, shield/health events, weapon/ordnance state, core state | Heavy recoil, shield impact, core charge and activation patterns |
| Titan steps / Titanfall | Animation-cycle crossings; nearby falling Titan tracking | Low thumps and a distance-scaled landing impact |
| Effect and Cause | Time-shift callback and map-specific displacement detection | Short rising transition effect |

Enemy-hit confirmation, headshots, kills credited to the local player and pilot melee contact need additional validated damage/hit hooks. A melee swing is not a confirmed contact. Likewise, the reference's active explosion path chiefly covers explosive damage to the player, not every harmless nearby explosion. Do not advertise universal hit/explosion coverage from these signals alone.

Two-handed support grip state needs validation: weapon-hand information alone does not prove that the other hand is holding the weapon. Reload event side in the reference is the weapon hand; identifying the physical magazine hand is a separate refinement.

## Recommended implementation

1. Create `Titanfall2VR_PSVR2.dll` as an x64 Northstar plug-in and a separately named Squirrel mod with its own dependency identifier and native functions. Start with event logging and runtime CAPI capability checks.
2. Implement one central effect scheduler, per-hand trigger state, a PCM mixer/worker, and an HMD pulse scheduler. Clear effects on death, pause, VM teardown, weapon/hand changes and shutdown. Bound queues and retry rates.
3. First playable scope: gun equip, actual firing, reload stages, player damage, Charge Rifle and Titan primary fire. Use configurable weapon-family profiles and individual overrides.
4. Expand to movement, Titan footsteps/ordnance/cores, grenades and time shifts after checkpoint compatibility is proven. Add confirmed enemy-hit feedback after its event source is verified.
5. Keep HMD intensity/duration settings separate from controller settings. Prefer hands for routine firing, with optional subtle HMD recoil; reserve prominent headset pulses for damage, heavy blasts and Titan impacts.

Suggested family tuning: crisp pistol break; lighter automatic pulses; heavier shotgun kick; increasing charge resistance; sustained energy texture for L-STAR; slower, weightier Titan recoil. These are design choices requiring physical tuning, not known optimal parameters.

## Integration pitfalls and acceptance checks

- Magazine reduction can be a chambered round being ejected. The reference uses the rack counter to suppress false shots. Ammo spent is not necessarily projectile count.
- Charge cancellation must not produce a discharge effect. The reference checks attack/cooldown evidence as well as charge drop.
- Old campaign saves restore old server script state and may skip new SERVER initialization. Test both fresh chapter starts and pre-mod saves; use client fallbacks where practical and disclose missing coverage.
- Automatic weapons may discharge multiple times between observations. Frame polling is not a guaranteed per-projectile callback; test cadence under low frame rate.
- Trigger modes are alternatives. Sending a slope after a weapon effect should not be described as layering both simultaneously. Use scheduling/restoration, with PCM supplying independent controller-body recoil.
- Check overlap with the VR mod's existing controller rumble so effects do not become unintentionally doubled or masked.
- Validate pause/menu behavior, left-handed use, switching weapons while charging, map transitions, death/respawn, SteamVR disconnect/reconnect and normal unload.
- Keep upstream research files out of distributable builds. Verify reuse terms before copying third-party implementation; an independent implementation can use these observed integration points.

Next concrete milestone: a logging-only Northstar add-on that proves weapon, reload and player-damage signals in the installed VR build, followed by one tuned gun profile and one headset damage pulse.
