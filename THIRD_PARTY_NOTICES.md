# Third-party references

The Northstar interface and Squirrel ABI declarations/offsets in `src/plugin.cpp` are based on R2Northstar/NorthstarLauncher, https://github.com/R2Northstar/NorthstarLauncher, under the MIT license below.

MIT License

Copyright (c) 2021 R2Northstar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

PSVR2 Toolkit is an external runtime dependency, not bundled:
https://github.com/BnuuySolutions/PSVR2Toolkit
Its project is licensed CC BY-NC-SA 4.0. This add-on independently declares the
small binary interface required to call an already installed CAPI DLL.
No Toolkit driver, jailbreak utility, or upstream SDK header is distributed here.

Integration research used the public Titanfall2VR_bhaptics source and build notes:
https://github.com/floh-bhaptics/Titanfall2VR_bhaptics
The add-on's Squirrel scripts and output implementation were written separately;
the upstream reference snapshots in `research/` are not packaged.

CircuitLord's installer and VR mod are external dependencies:
https://github.com/CircuitLord/CircuitLordVRModInstaller
The closed-source VR mod and game binaries are not included.

Weapon metadata was extracted from the Titanfall 2 client-file mirror at
https://github.com/Syampuuh/Titanfall2, commit
9bfcea43ecf2a12fc3783ce22e63e439f422c045. The catalog contains selected numeric
facts, identifiers and names; original game scripts and weapon-definition files
are not bundled. Titanfall 2 and its game content belong to their respective
rights holders. See GAME_PROFILES.md for the source-to-effect mapping.
