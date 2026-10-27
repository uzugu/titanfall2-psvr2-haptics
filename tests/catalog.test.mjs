import assert from 'node:assert/strict';
import {parseKV, merge} from '../scripts/keyvalues.mjs';
import fs from 'node:fs';
const parsed = parseKV(`WeaponData {
  "fire_rate" "12" // baseline
  "kick" "3"[$PS4]
  "kick" "1"[$WINDOWS]
  MP_BASE { "duration" "3.5" }
  SP_BASE { "duration" "6.5" }
  Mods { upgrade { "fire_rate" "*1.5" } }
}`);
const sp = {...parsed.WeaponData, ...parsed.WeaponData.SP_BASE};
assert.equal(sp.duration, '6.5');
assert.equal(sp.fire_rate, '12');
assert.equal(sp.kick, '1');
assert.equal(sp.Mods.upgrade.fire_rate, '*1.5');
assert.equal(merge({SP_BASE:{rate:12,clip:30}}, {SP_BASE:{clip:40}}).SP_BASE.rate,12);
assert.equal(parseKV('#base "base.txt" WeaponData { rate 15 }')['#base'], 'base.txt');
assert.throws(()=>parseKV('WeaponData { key }'));
assert.throws(()=>parseKV('WeaponData { key value'));
const catalog = JSON.parse(fs.readFileSync(new URL('../data/weapon-facts.json', import.meta.url)));
assert.equal(catalog.weapons.length, 55);
assert.equal(new Set(catalog.weapons.map(w=>w.id)).size,55);
for (const w of catalog.weapons) {
  assert.match(w.source, new RegExp(catalog.sourceCommit));
  assert.match(w.sha256,/^[a-f0-9]{64}$/);
}
const find = id => catalog.weapons.find(w=>w.id===id);
assert.equal(find('mp_titanweapon_xo16_shorty').values.fire_rate,12);
assert.equal(find('mp_titanweapon_predator_cannon').values.ammo_clip_size,150);
assert.equal(find('mp_titancore_laser_cannon').values.sustained_discharge_duration,6.5);
assert.equal(find('mp_weapon_esaw').values.fire_rate_max_time_speedup,1.75);
assert.equal(find('mp_titanweapon_rocketeer_rocketstream').values.ammo_per_shot,4);
const script = fs.readFileSync(new URL('../mod/Titanfall2VR_PSVR2/mod/scripts/vscripts/tf2vr_psvr2_catalog.nut',import.meta.url),'utf8');
for (const w of catalog.weapons) assert.ok(script.includes(`case "${w.id}"`));
console.log('Catalog provenance, nested SP/MP separation, inheritance, platform selection and script coverage passed.');
