# Preview validation — 2026-10-27

## Native compatibility target

Steam build 10350970; Northstar 1.31.13.0. The inspected `client.dll` has PE timestamp 1512510036, image size 64274432 and SHA-256 `002B36487FEC7C98882929FBCCDB506D0146BBF3270CBB964A4D21C5EDF7EEBC`. Registration checks the timestamp, image size and executable pages, not the full hash. Other binaries require explicit offset validation.

Version 0.2.2 passed all four suites and was installed locally, with the installed DLL hash matching the tested build. The previous play test recorded incoming damage but the user felt no headset rumble. Toolkit initialization and driver activity were confirmed; those checks do not establish physical headset motor readiness. The local test configuration uses HeadsetScale=1.00; the distributed default remains 0.65. Per-bullet pushback, left-hand resets and revised headset pulses await physical confirmation.

## 0.2.2 per-bullet trigger pushback

Replaced automatic-fire trigger vibration with a feedback-mode kick on each observed shot. Feedback strength is the weapon's configured strength plus two (capped at eight; zero remains disabled). The push lasts 45% of a shot interval, capped at 45 ms, followed by a fully released interval before normal weapon resistance returns. Each new shot starts a new kick. Controller PCM recoil remains independent. Tests verify repeated push/release cycles, stronger feedback, left-hand routing and expiry; the mock CAPI checks that actual worker commands use feedback and then off. Physical feel and left-handed behavior still need a game restart and test.

## 0.2.1 feedback fixes

Added low-damage/no-weapon headset feedback, incoming headshot distinction, bounded fatal-hit tails, automatic-fire trigger motor vibration at effective cadence, unknown-hand rejection, both-trigger reset on hand changes and 100 ms trigger refresh. State changes flush immediately and event logs flush every second. Native tests cover damage pulse bounds/expiry, headshot strength, death cleanup, automatic cadence/expiry and hand switching; the mock worker additionally checks stale offhand state recovery.

The previous 0.2.0 package was installed and launched with the installer's `TF2VR_OPENXR=1` environment. Its log confirmed client script initialization, Toolkit driver activity, automatic shot events, incoming damage events, and left/right weapon-hand changes. The user reported weak/missing automatic trigger pushback and persistent right-trigger resistance in left-handed use. The precise physical cause of the latter is not yet established; reset/refresh changes need an in-game retest. No claim of physical validation for 0.2.1 is made here.

## 0.2.0 build-time record

The game-data update adds 55 catalog entries and four passing test suites. New checks cover source inheritance, Windows conditional selection, SP versus MP values, exact script/native allowlist consistency, high-rate pulse duration, both-hand Titan recoil, Legion spool expiry, Splitter ADS behavior, rail charge ticks, Laser Core continuous output/expiry and offensive-offhand filtering. A test caught beam events incorrectly releasing the trigger; the shot path now excludes beams entirely.

The baseline game files were downloaded from a pinned public client-file mirror. The local campaign loadout script confirms all eight Titan primaries; native-method availability was checked against the installed client binary. These checks do not replace Squirrel compilation or a physical play test. No new game deployment was performed in this update.

## Original 0.1.0 checks

- MSVC 19.44, Visual Studio 2022, x64 Release: built successfully with no warnings in the final build.
- `effect_tests`: passed. Covers hand routing, recoil waveform presence, charge cancellation, trigger restoration, damage/HMD limits, watchdog expiration, queue bounds, stop, disabled/logging modes and profile validation.
- `plugin_smoke`: passed. Actual DLL loaded through `CreateInterface`; Northstar identity and callbacks checked, logging-only worker initialized and unloaded.
- `runtime_test`: passed. Actual worker connected to an isolated fake CAPI DLL; checked PCM writes, hand changes, bounded HMD output, stale-heartbeat clearing, failed-write retry, disconnect/reconnect and shutdown/deinitialization. Fixed a test timing assumption by waiting for asynchronous output transitions instead of assuming completion after 30 ms.
- Actual installed Toolkit read-only probe: `init=0, driverActive=1`. This test sends no motor commands.
- Package inspected: contains only our DLL, INI, Squirrel mod, probe, installer and documentation. No game binaries, upstream reference sources or fake CAPI DLL.
- PowerShell installer/packager parsed successfully; mod manifest parsed and script path verified.
- Located installed game under `C:/Program Files (x86)/Steam/steamapps/common/Titanfall2`; read its Northstar 1.31.13.0 logs and inspected the client binary identity used by native registration.

Not yet validated: script compilation inside Titanfall's Squirrel VM, physical haptic feel, shot/reload detection in gameplay, charge weapons, Titan combat, old checkpoints and pause behavior. Titanfall was already running during development, so no deployment or game restart was performed. Use the extracted package's Install.ps1 after closing the game; LoggingOnly is available for the first script/event check.

The current package is a preview for that next test, not a claim of completed in-game compatibility.
