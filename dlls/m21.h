#ifndef TOD_M21_H
#define TOD_M21_H

#define WEAPON_M21 25
#define M21_SLOT 2
#define M21_POSITION 5
#define M21_WEIGHT 6

#define M21_AMMO_CLIP 20
#define M21_FIRE_DELAY 1
#define M21_RELOAD_TIME ( 135.0 / 45.0 )

#define M21_MODEL_WORLD "models/w_m21.mdl"
#define M21_MODEL_1STPERSON "models/v_m21.mdl"
#define M21_MODEL_3RDPERSON "models/p_m21.mdl"

#define M21_SOUND_FIRE1 "weapons/m21_shot1.wav"
#define M21_SOUND_FIRE2	"weapons/m21_shot2.wav"
#define M21_SOUND_CLIPIN "weapons/m21_clipin.wav"
#define M21_SOUND_CLIPOUT	"weapons/m21_clipout.wav"

enum m21_e {
	M21_IDLE = 0,
	M21_FIRE1,
	M21_RELOAD,
	M21_DEPLOY,
	M21_HOLSTER,
};


class CM21 : public CBasePlayerWeapon
{
public:
    virtual void Spawn( void );
    virtual void Precache( void );
    virtual int  iItemSlot( void ) { return M21_SLOT; }
    virtual int  GetItemInfo(ItemInfo *p);
    virtual int  AddToPlayer( CBasePlayer *pPlayer );
    
    virtual void PrimaryAttack( void );
    virtual void SecondaryAttack( void );  // Zoom in/out
    virtual BOOL Deploy( void );
    virtual void Holster( int skiplocal = 0 );
    virtual void Reload( void );
    virtual void WeaponIdle( void );
    
    virtual BOOL UseDecrement( void ) { return TRUE; }
    virtual Vector Accuracy( void );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
    int   m_iShell;
    bool  m_InZoom;
//    unsigned short m_event;
//    unsigned short m_event_z;
};

TYPEDESCRIPTION	CM21::m_SaveData[] = 
{
	DEFINE_FIELD( CM21, m_InZoom, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CM21, CBasePlayerWeapon );


#endif