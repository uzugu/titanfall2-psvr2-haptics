// Generated from the same catalog as the native profiles.
global function P2_IsGun
global function P2_IsOffensiveAbility
bool function P2_IsGun( string name )
{
    switch ( name )
    {
        case "mp_weapon_semipistol":
        case "mp_weapon_autopistol":
        case "mp_weapon_wingman":
        case "mp_weapon_wingman_n":
        case "mp_weapon_smart_pistol":
        case "mp_weapon_rspn101":
        case "mp_weapon_rspn101_og":
        case "mp_weapon_vinson":
        case "mp_weapon_hemlok":
        case "mp_weapon_g2":
        case "mp_weapon_car":
        case "mp_weapon_r97":
        case "mp_weapon_alternator_smg":
        case "mp_weapon_hemlok_smg":
        case "mp_weapon_lmg":
        case "mp_weapon_esaw":
        case "mp_weapon_lstar":
        case "mp_weapon_shotgun":
        case "mp_weapon_mastiff":
        case "mp_weapon_shotgun_pistol":
        case "mp_weapon_dmr":
        case "mp_weapon_doubletake":
        case "mp_weapon_sniper":
        case "mp_weapon_epg":
        case "mp_weapon_softball":
        case "mp_weapon_smr":
        case "mp_weapon_pulse_lmg":
        case "mp_weapon_mgl":
        case "mp_weapon_arc_launcher":
        case "mp_weapon_rocket_launcher":
        case "mp_weapon_defender":
        case "mp_titanweapon_xo16":
        case "mp_titanweapon_xo16_shorty":
        case "mp_titanweapon_xo16_vanguard":
        case "mp_titanweapon_sticky_40mm":
        case "mp_titanweapon_meteor":
        case "mp_titanweapon_leadwall":
        case "mp_titanweapon_particle_accelerator":
        case "mp_titanweapon_sniper":
        case "mp_titanweapon_predator_cannon":
        case "mp_titanweapon_predator_cannon_siege":
        case "mp_titanweapon_rocketeer_rocketstream":
        case "mp_titanweapon_flightcore_rockets":
        case "mp_titancore_laser_cannon":
        case "mp_titancore_flame_wave":
        case "mp_titancore_salvo_core":
            return true
    }
    return false
}

bool function P2_IsOffensiveAbility( string name )
{
    switch ( name )
    {
        case "mp_titanweapon_shoulder_rockets":
        case "mp_titanweapon_tracker_rockets":
        case "mp_titanweapon_homing_rockets":
        case "mp_titanweapon_salvo_rockets":
        case "mp_titanweapon_dumbfire_rockets":
        case "mp_titanweapon_laser_lite":
        case "mp_titanweapon_arc_wave":
        case "mp_titanweapon_flame_wall":
        case "mp_titanability_power_shot":
            return true
    }
    return false
}
