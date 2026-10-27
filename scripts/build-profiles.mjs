import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const commit = '9bfcea43ecf2a12fc3783ce22e63e439f422c045';

import {parseKV, merge} from './keyvalues.mjs';

const choices = {
  // Waveform type is an authored interpretation; numeric facts below come from game files.
  mp_weapon_semipistol:'Ballistic', mp_weapon_autopistol:'Ballistic', mp_weapon_wingman:'Ballistic',
  mp_weapon_wingman_n:'Ballistic', mp_weapon_smart_pistol:'Ballistic',
  mp_weapon_rspn101:'Ballistic', mp_weapon_rspn101_og:'Ballistic', mp_weapon_vinson:'Ballistic',
  mp_weapon_hemlok:'Ballistic', mp_weapon_g2:'Ballistic', mp_weapon_car:'Ballistic',
  mp_weapon_r97:'Ballistic', mp_weapon_alternator_smg:'Ballistic', mp_weapon_hemlok_smg:'Energy',
  mp_weapon_lmg:'Ballistic', mp_weapon_esaw:'Ballistic', mp_weapon_lstar:'Energy',
  mp_weapon_shotgun:'Scatter', mp_weapon_mastiff:'Scatter', mp_weapon_shotgun_pistol:'Scatter',
  mp_weapon_dmr:'Ballistic', mp_weapon_doubletake:'Energy', mp_weapon_sniper:'Ballistic',
  mp_weapon_epg:'Energy', mp_weapon_softball:'Rocket', mp_weapon_smr:'Rocket',
  mp_weapon_pulse_lmg:'Energy', mp_weapon_mgl:'Rocket', mp_weapon_arc_launcher:'Energy',
  mp_weapon_rocket_launcher:'Rocket', mp_weapon_defender:'Rail',
  mp_titanweapon_xo16:'Ballistic', mp_titanweapon_xo16_shorty:'Ballistic', mp_titanweapon_xo16_vanguard:'Ballistic',
  mp_titanweapon_sticky_40mm:'Cannon', mp_titanweapon_meteor:'Thermite', mp_titanweapon_leadwall:'Scatter',
  mp_titanweapon_particle_accelerator:'Energy', mp_titanweapon_sniper:'Rail',
  mp_titanweapon_predator_cannon:'Rotary', mp_titanweapon_predator_cannon_siege:'Rotary',
  mp_titanweapon_rocketeer_rocketstream:'Rocket', mp_titanweapon_flightcore_rockets:'Rocket',
  mp_titanweapon_shoulder_rockets:'Rocket', mp_titanweapon_tracker_rockets:'Rocket',
  mp_titanweapon_homing_rockets:'Rocket', mp_titanweapon_salvo_rockets:'Rocket',
  mp_titanweapon_dumbfire_rockets:'Rocket', mp_titanweapon_laser_lite:'Rail',
  mp_titanweapon_arc_wave:'Energy', mp_titanweapon_flame_wall:'Thermite',
  mp_titanability_power_shot:'Cannon', mp_titancore_laser_cannon:'Beam',
  mp_titancore_flame_wave:'Thermite', mp_titancore_salvo_core:'Rocket'
};
const abilities = new Set(['shoulder_rockets','tracker_rockets','homing_rockets','salvo_rockets','dumbfire_rockets','laser_lite','arc_wave','flame_wall']
  .map(n => 'mp_titanweapon_' + n));
abilities.add('mp_titanability_power_shot');
const keys = ['fire_rate','burst_fire_count','burst_fire_delay','charge_time','charge_levels','charge_cooldown_time',
  'ammo_clip_size','ammo_per_shot','viewkick_pitch_base','viewkick_yaw_base','projectile_launch_speed',
  'sustained_discharge_duration','sustained_discharge_pulse_frequency','reload_time','zoom_time_in','zoom_time_out',
  'fire_rate_max','fire_rate_max_time_speedup','damage_near_value','damage_near_value_titanarmor'];
const gameDir = path.join(root, 'research/game');
const localeBytes = fs.readFileSync(path.join(gameDir, 'r1_english.txt'));
const localeText = localeBytes[0] === 255 && localeBytes[1] === 254 ? localeBytes.toString('utf16le') : localeBytes.toString('utf8');
const names = Object.fromEntries([...localeText.matchAll(/"((?:WPN_|TITANCORE_)[A-Z0-9_]+)"\s+"([^"\r\n]+)"/g)].map(m => [m[1],m[2]]));
function definition(id, visited = []) {
  if (visited.includes(id)) throw new Error(`Cyclic #base: ${id}`);
  const parsed = parseKV(fs.readFileSync(path.join(gameDir, `${id}.txt`), 'utf8'));
  if (!parsed.WeaponData) throw new Error(`Missing WeaponData: ${id}`);
  if (!parsed['#base']) return parsed.WeaponData;
  const base = path.basename(parsed['#base'], '.txt');
  return merge(definition(base, [...visited,id]), parsed.WeaponData);
}
const facts = [];
for (const [id, style] of Object.entries(choices)) {
  const file = fs.readFileSync(path.join(gameDir, `${id}.txt`), 'utf8');
  let data;
  try { data = definition(id); } catch(e) { throw new Error(`${id}: ${e.message}`); }
  if (!data) throw new Error(`Missing WeaponData in ${id}`);
  const effective = {...data, ...data.SP_BASE};
  const numeric = {};
  for (const key of keys) {
    if (effective[key] !== undefined && typeof effective[key] === 'string' && Number.isFinite(Number(effective[key])))
      numeric[key] = Number(effective[key]);
  }
  const variants = {};
  for (const [mod, values] of Object.entries(data.Mods ?? {})) {
    const relevant = Object.fromEntries(keys.filter(k => values[k] !== undefined).map(k => [k, values[k]]));
    if (Object.keys(relevant).length) variants[mod] = relevant;
  }
  facts.push({id, name:names[String(data.printname).replace(/^#/, '')] ?? id, style,
    role: abilities.has(id) ? 'Ability' : id.startsWith('mp_titancore_') ? 'Core' : 'Primary',
    titan: !id.startsWith('mp_weapon_'),
    source:`https://github.com/Syampuuh/Titanfall2/blob/${commit}/scripts/weapons/${id}.txt`,
    sha256:crypto.createHash('sha256').update(file).digest('hex'),
    resolution:'#base recursively merged, Windows conditionals selected, root scalars overlaid with SP_BASE; named Mods retained separately, not applied.',
    values:numeric, variants});
}
fs.mkdirSync(path.join(root, 'data'), {recursive:true});
fs.writeFileSync(path.join(root, 'data/weapon-facts.json'), JSON.stringify({sourceCommit:commit,weapons:facts}, null, 2)+'\n');
const f = x => `${Number(x ?? 0).toFixed(6)}f`;
const rows = facts.map(w => `    {"${w.id}", "${w.name.replaceAll('"','\\"')}", Waveform::${w.style}, WeaponRole::${w.role}, ${w.titan}, ${f(w.values.fire_rate)}, ${f(Math.abs(w.values.viewkick_pitch_base ?? 0))}, ${f(w.values.charge_time)}, ${Math.trunc(w.values.charge_levels ?? 0)}, ${f(w.values.sustained_discharge_duration)}, ${f(w.values.burst_fire_count)}, ${f(w.values.fire_rate_max)}},`);
fs.writeFileSync(path.join(root, 'src/weapon_facts.generated.h'),
  `// Generated by scripts/build-profiles.mjs. Game facts, not physical haptic measurements.\n// Source commit: ${commit}\n#pragma once\n#include "weapon_facts.h"\nnamespace p2 {\ninline constexpr WeaponFacts kWeaponFacts[] = {\n${rows.join('\n')}\n};\n}\n`);
console.log(`Generated ${facts.length} individually identified weapon definitions.`);
const script = ['// Generated from the same catalog as the native profiles.',
  'global function P2_IsGun', 'global function P2_IsOffensiveAbility'];
for (const [fn, role] of [['P2_IsGun','Primary'], ['P2_IsOffensiveAbility','Ability']]) {
  script.push(`bool function ${fn}( string name )`, '{', '    switch ( name )', '    {');
  for (const fact of facts.filter(w => role === 'Primary' ? w.role !== 'Ability' : w.role === 'Ability'))
    script.push(`        case "${fact.id}":`);
  script.push('            return true', '    }', '    return false', '}', '');
}
fs.writeFileSync(path.join(root,'mod/Titanfall2VR_PSVR2/mod/scripts/vscripts/tf2vr_psvr2_catalog.nut'), script.join('\n'));
