global function TF2VR_PSVR2_Init

void function TF2VR_PSVR2_Init()
{
#if TF2VR_PSVR2
    P2_Log( "PSVR2 client script initialized (v0.2.3)" )
    for ( int source = -1; source <= 511; source++ )
        AddLocalPlayerTookDamageCallback( source, P2_OnDamage )
    thread P2_Watch()
    thread P2_WatchTitanAbilities()
#endif
}

#if TF2VR_PSVR2
void function P2_OnDamage( float damage, vector origin, int damageType, int source, entity attacker )
{
    if ( damage <= 0.0 )
        return
    entity player = GetLocalClientPlayer()
    if ( !IsValid( player ) )
        return
    float reference = player.IsTitan() ? 500.0 : 75.0
    string effect = ( damageType & DF_EXPLOSION ) != 0 ? "explosion" : "damage"
    if ( ( damageType & DF_HEADSHOT ) != 0 )
        effect = "headshot"
    P2_Event( effect, min( damage / reference, 1.0 ) )
}

void function P2_Watch()
{
    entity previousPlayer = null
    entity previousWeapon = null
    string previousHand = ""
    int previousAmmo = -1
    float previousAttack = 0.0
    float previousCharge = 0.0
    float lastTime = -1.0
    float lastRack = -100.0
    float nextState = 0.0
    int racks = GetConVarInt( "tf2vr_reload_rounds" )
    int ejections = GetConVarInt( "tf2vr_reload_ejections" )
    int magOut = GetConVarInt( "tf2vr_reload_magazine_out" )
    bool wasAlive = false
    while ( true )
    {
        WaitFrame()
        float now = Time()
        // Paused game time must not refresh the native watchdog.
        if ( now == lastTime )
            continue
        bool gap = lastTime < 0.0 || now < lastTime || now - lastTime > 0.25
        lastTime = now
        entity player = GetLocalClientPlayer()
        int newRacks = GetConVarInt( "tf2vr_reload_rounds" )
        int newEjections = GetConVarInt( "tf2vr_reload_ejections" )
        int newMagOut = GetConVarInt( "tf2vr_reload_magazine_out" )
        bool racked = newRacks != racks || newEjections != ejections
        if ( racked )
            lastRack = now
        bool removed = newMagOut != magOut && newMagOut > 0
        racks = newRacks
        ejections = newEjections
        magOut = newMagOut
        if ( !IsValid( player ) || !IsAlive( player ) )
        {
            if ( wasAlive )
                P2_Event( "death", 0.0 )
            wasAlive = false
            previousPlayer = null
            previousWeapon = null
            nextState = 0.0
            continue
        }
        wasAlive = true
        entity weapon = player.GetActiveWeapon()
        int weaponHand = TF2VR_WeaponHand()
        string hand = weaponHand == 0 ? "left" : ( weaponHand == 1 ? "right" : "none" )
        bool changed = gap || player != previousPlayer || weapon != previousWeapon || hand != previousHand
        if ( !IsValid( weapon ) )
        {
            if ( changed || now >= nextState )
            {
                P2_State( "", hand, 0.0 )
                nextState = now + 0.05
            }
            previousPlayer = player
            previousWeapon = null
            previousHand = hand
            continue
        }
        string name = weapon.GetWeaponClassName()
        bool gun = P2_IsGun( name )
        int ammo = weapon.GetWeaponPrimaryClipCount()
        float attack = weapon.GetNextAttackAllowedTime()
        float fraction = weapon.GetWeaponChargeFraction()
        bool chargeWeapon = name == "mp_weapon_defender" || name == "mp_titanweapon_sniper" || name == "mp_weapon_pulse_lmg"
            || name == "mp_titancore_laser_cannon" || name == "mp_titancore_flame_wave" || name == "mp_titancore_salvo_core"
        bool heldCharge = chargeWeapon && ( name == "mp_titanweapon_sniper" ? weapon.IsWeaponInAds() : weapon.IsWeaponCharging() )
        float charge = heldCharge ? fraction : 0.0
        float rate = gun ? weapon.GetWeaponSettingFloat( eWeaponVar.fire_rate ) : 0.0
        float ads = weapon.IsWeaponInAds() ? 1.0 : 0.0
        float beam = 0.0
        if ( name == "mp_titancore_laser_cannon" && player.IsTitan() )
        {
            entity soul = player.GetTitanSoul()
            if ( IsValid( soul ) && SoulTitanCore_GetExpireTime( soul ) > now && !weapon.IsWeaponCharging() )
                beam = 1.0
        }
        float spool = name == "mp_titanweapon_predator_cannon" || name == "mp_titanweapon_predator_cannon_siege" ? player.GetZoomFrac() : 0.0
        float levels = name == "mp_titanweapon_sniper" ? float( weapon.GetWeaponChargeLevelMax() ) : 0.0
        if ( changed || now >= nextState || charge != previousCharge )
        {
            P2_State( gun ? name : "", hand, charge )
            P2_Tune( rate, ads, beam, spool, levels )
            nextState = now + 0.05
        }
        if ( !changed && gun )
        {
            bool shot = previousAmmo > 0 && ammo >= 0 && ammo < previousAmmo
            if ( !shot && chargeWeapon && player.IsInputCommandHeld( IN_ATTACK ) )
                shot = previousCharge > 0.0 && fraction <= 0.0 && attack > previousAttack + 0.001 && attack > now
            if ( !shot && name == "mp_titanweapon_flightcore_rockets" )
                shot = attack > previousAttack + 0.001 && attack > now
            // Racks eject live ammunition, and removing a magazine can also lower the clip.
            if ( !player.IsTitan() && ( now - lastRack < 0.2 || removed || newMagOut > 0 ) )
                shot = false
            if ( shot && name != "mp_titancore_laser_cannon" )
            {
                // Use observed attack timing for spool-up/upgrade cadence when available.
                float observedInterval = attack - previousAttack
                float effectiveRate = observedInterval > 0.01 && observedInterval < 1.0 ? 1.0 / observedInterval : rate
                P2_Tune( effectiveRate, ads, beam, spool, levels )
                float strength = chargeWeapon ? 0.5 + 0.5 * max( charge, previousCharge ) : 1.0
                P2_Event( "shot", strength )
            }
            if ( !player.IsTitan() )
            {
                if ( removed )
                    P2_Event( "mag_out", 1.0 )
                if ( racked )
                    P2_Event( "rack", 1.0 )
                if ( ammo > previousAmmo && previousAmmo >= 0 && name != "mp_weapon_lstar" )
                    P2_Event( "mag_in", 1.0 )
            }
        }
        previousPlayer = player
        previousWeapon = weapon
        previousHand = hand
        previousAmmo = ammo
        previousAttack = attack
        previousCharge = charge
    }
}

// Offensive offhands are separate from the active primary. Baseline every entity
// before emitting; loadout changes and cooldown restoration must not become shots.
void function P2_WatchTitanAbilities()
{
    entity owner = null
    table<entity, int> clips = {}
    table<entity, float> attacks = {}
    float previousTime = -1.0
    while ( true )
    {
        WaitFrame()
        float now = Time()
        if ( now == previousTime )
            continue
        entity player = GetLocalClientPlayer()
        bool reset = player != owner || previousTime < 0.0 || now < previousTime || now - previousTime > 0.25
        previousTime = now
        owner = player
        if ( reset || !IsValid( player ) || !IsAlive( player ) || !player.IsTitan() )
        {
            clips.clear()
            attacks.clear()
        }
        if ( !IsValid( player ) || !IsAlive( player ) || !player.IsTitan() )
            continue
        table<entity, int> nextClips = {}
        table<entity, float> nextAttacks = {}
        foreach ( entity ability in player.GetOffhandWeapons() )
        {
            if ( !IsValid( ability ) || !P2_IsOffensiveAbility( ability.GetWeaponClassName() ) )
                continue
            int ammo = ability.GetWeaponPrimaryClipCount()
            float attack = ability.GetNextAttackAllowedTime()
            if ( ability in clips && ability in attacks )
            {
                bool fired = clips[ability] > 0 && ammo >= 0 && ammo < clips[ability]
                bool cooldownStarted = attack > attacks[ability] + 0.001 && attack > now
                // Charge loss alone is deliberately not interpreted as firing.
                if ( ( fired || cooldownStarted ) && !ability.IsWeaponCharging() )
                    P2_WeaponEvent( ability.GetWeaponClassName(), 1.0 )
            }
            nextClips[ability] <- ammo
            nextAttacks[ability] <- attack
        }
        clips = nextClips
        attacks = nextAttacks
    }
}
#endif
