#ifndef TOD_RPG7_H
#define TOD_RPG7_H

#define WEAPON_RPG7 24
#define RPG7_SLOT 3
#define RPG7_POSITION 2
#define RPG7_WEIGHT 2

#define RPG7_RELOAD_TIME 4
#define RPG7_AMMO_CLIP 1
#define RPG7_MAX_AMMO 10

#define RPG7_MODEL_ROCKET "models/rpg7rocket.mdl"
#define RPG7_MODEL_WORLD "models/w_rpg7.mdl"
#define RPG7_MODEL_1STPERSON "models/v_rpg7.mdl"
#define RPG7_MODEL_3RDPERSON "models/p_rpg7.mdl"

#define RPG7_SMOKE_TRAIL  "sprites/smoke.spr"

#define RPG7_SOUND_ROCKET "weapons/rocket1.wav"
#define RPG7_SOUND_FIRE1 "weapons/rpg7_fire1.wav"


enum rpg7_e {
	RPG7_IDLE = 0,
	RPG7_FIRE1,
	RPG7_FIRE2,
	RPG7_RELOAD,
	RPG7_RELOAD2,
	RPG7_DEPLOY,
	RPG7_EMPTY,
	RPG7_IDLE_LONG,
};


class CRpg7 : public CBasePlayerWeapon
{
public:
    virtual void Spawn( void );
    virtual void Precache( void );
    virtual void Reload( void );
    virtual int iItemSlot( void ) { return RPG7_SLOT; }
    virtual int GetItemInfo(ItemInfo *p);
    virtual int AddToPlayer( CBasePlayer *pPlayer );
    
    virtual BOOL Deploy( void );
    virtual void Holster( int skiplocal = 0 );
    
    virtual void PrimaryAttack( void );
    virtual void WeaponIdle( void );
    
    virtual BOOL ShouldWeaponIdle( void ) { return TRUE; };
    
    virtual BOOL UseDecrement( void )
    { 
#if defined( CLIENT_WEAPONS )
	return TRUE;
#else
	return FALSE;
#endif
    }
    
private:
//    unsigned short m_usRpg;
    
};

class CRpg7Rocket : public CGrenade
{
public:
    virtual void Spawn( void );
    virtual void Precache( void );
    void EXPORT FlightThink( void );
    void EXPORT IgniteThink( void );
    void EXPORT RocketTouch( CBaseEntity *pOther );
    static CRpg7Rocket *CreateRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner );
    
    int   m_iTrail;
    float m_flIgniteTime;
};

#endif