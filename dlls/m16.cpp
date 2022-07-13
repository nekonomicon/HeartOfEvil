//*****************************************************************
//
//	M16 5.56 mm assault rifle
//
//	by Nathan Ruck for Heart of Evil
//
//*****************************************************************

#include "extdll.h" 
#include "decals.h" 
#include "util.h" 
#include "cbase.h" 
#include "monsters.h" 
#include "weapons.h" 
#include "nodes.h" 
#include "player.h" 
#include "soundent.h" 
#include "shake.h" 
#include "gamerules.h"


#define WEAPON_M16 16 
#define M16_SLOT 2
#define M16_POSITION 2
#define M16_WEIGHT 15


#define M16_MODEL_1STPERSON "models/v_m16.mdl" 
#define M16_MODEL_3RDPERSON "models/p_m16.mdl"
#define M16_MODEL_WORLD "models/w_m16.mdl"

#define M16_SOUND_SHOOT1 "weapons/m16_fire1.wav" 
#define M16_SOUND_SHOOT2 "weapons/m16_fire2.wav" 
#define M16_SOUND_SHOOT3 "weapons/m16_fire3.wav" 
#define M16_SOUND_CLIPIN "weapons/m16_clipinsert1.wav"
#define M16_SOUND_CLIPOUT "weapons/m16_cliprelease1.wav"
#define M16_SOUND_VOLUME 1

#define M16_PRIMARY_FIRE_DELAY 0.085
#define M16_SECONDARY_FIRE_DELAY 0.35
#define M16_SWITCH_FIRE_DELAY 0.5
#define M16_DEFAULT_AMMO 30 
#define M16_MAX_AMMO 250
#define M16_MAX_CLIP 30

#define AMMO_M16BOX_GIVE 150
#define AMMO_M16CLIP_GIVE 30


enum m16_animations
{
	M16_LONGIDLE = 0,
	M16_IDLE1,
	M16_RELOAD,
	M16_RELOAD_EMPTY,
	M16_DEPLOY,
	M16_HOLSTER,
	M16_FIRE1,
};


class CM16 : public CBasePlayerWeapon 
{ 
public: 
	void Spawn( ); 
	void Precache( ); 
	int iItemSlot( ); 
	int GetItemInfo( ItemInfo* ); 
	BOOL Deploy( ); 
	void Reload( void );
	void PrimaryAttack( ); 
	void SecondaryAttack( void );
	void WeaponIdle( ); 
	int AddToPlayer( CBasePlayer *pPlayer );
	Vector GetGunPosition( );
	int	m_iBrassShell;

private:
	BOOL Fire(float flSpread, Vector vecAiming);
}; 

LINK_ENTITY_TO_CLASS( weapon_m16, CM16 );


void CM16 :: Spawn( ) 
{ 
	pev->classname = MAKE_STRING( "weapon_m16" ); 
	Precache( ); 

	m_iId = WEAPON_M16; 
	SET_MODEL( ENT(pev), M16_MODEL_WORLD ); 

	m_iDefaultAmmo = M16_DEFAULT_AMMO; 

	FallInit( ); 
}


void CM16 :: Precache( ) 
{ 
	PRECACHE_MODEL( M16_MODEL_1STPERSON ); 
	PRECACHE_MODEL( M16_MODEL_3RDPERSON ); 
	PRECACHE_MODEL( M16_MODEL_WORLD ); 
	
	PRECACHE_SOUND( M16_SOUND_SHOOT1 ); 
	PRECACHE_SOUND( M16_SOUND_SHOOT2 ); 
	PRECACHE_SOUND( M16_SOUND_SHOOT3 ); 
	PRECACHE_SOUND( M16_SOUND_CLIPIN ); 
	PRECACHE_SOUND( M16_SOUND_CLIPOUT ); 
	PRECACHE_SOUND( "weapons/m16_slide1.wav" ); 

	PRECACHE_SOUND ("weapons/357_cock1.wav");
	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

}


int CM16 :: iItemSlot( ) 
{ 
	return M16_SLOT; 
}


int CM16 :: GetItemInfo( ItemInfo* Info ) 
{ 
	Info->pszName = STRING( pev->classname ); 
	Info->pszAmmo1 = "5_56mm"; 
	Info->iMaxAmmo1 = M16_MAX_AMMO; 
	Info->pszAmmo2 = NULL; 
	Info->iMaxAmmo2 = -1; 
	Info->iMaxClip = M16_MAX_CLIP; 
	Info->iSlot = M16_SLOT;
	Info->iPosition = M16_POSITION; 
	Info->iFlags = 0; 
	Info->iId = WEAPON_M16; 
	Info->iWeight = M16_WEIGHT;

	return 1; 
}


BOOL CM16 :: Deploy( ) 
{ 
return DefaultDeploy( M16_MODEL_1STPERSON, 
                      M16_MODEL_3RDPERSON, 
                      M16_DEPLOY, 
                      "mp5" ); 
}


Vector CM16::GetGunPosition( )
{
	Vector v, a;
	GetAttachment(0, v, a);

	return v;
}


BOOL CM16::Fire(float flSpread, Vector vecAiming)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		return FALSE;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		return FALSE;
	}

	switch(RANDOM_LONG(0, 2))
	{
	case 0:	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, M16_SOUND_SHOOT1, M16_SOUND_VOLUME, ATTN_NORM); break;
	case 1:	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, M16_SOUND_SHOOT2, M16_SOUND_VOLUME, ATTN_NORM); break;
	case 2:	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, M16_SOUND_SHOOT3, M16_SOUND_VOLUME, ATTN_NORM); break;
	}

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;
	
	Vector	vecShellVelocity = gpGlobals->v_right * 60 + gpGlobals->v_up * 200 + gpGlobals->v_forward * 40;
	Vector	vecShellOrigin = pev->origin + pev->view_ofs + gpGlobals->v_forward * 30 + gpGlobals->v_right * 30;
	EjectBrass ( vecShellOrigin, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	
	m_iClip--;

    // player "shoot" animation
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	SendWeaponAnim( M16_FIRE1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	
	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_M16, 2 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	return TRUE;
}


void CM16::PrimaryAttack()
{
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (Fire(0.1, vecAiming))
	{
	}
//	m_flNextSecondaryAttack = gpGlobals->time + M16_SWITCH_FIRE_DELAY;
	m_flNextPrimaryAttack = m_flNextPrimaryAttack + M16_PRIMARY_FIRE_DELAY;
	if (m_flNextPrimaryAttack < gpGlobals->time)
		m_flNextPrimaryAttack = gpGlobals->time + M16_PRIMARY_FIRE_DELAY;
}


void CM16::SecondaryAttack()
{
/*	Vector vecAiming = gpGlobals->v_forward;

	if (Fire(0.1, vecAiming))
	{
		m_pPlayer->pev->punchangle.x += RANDOM_FLOAT( -1.0, 1.0 );
		m_pPlayer->pev->punchangle.z += RANDOM_FLOAT( -1.0, 1.0 );
	}
	m_flNextPrimaryAttack = gpGlobals->time + M16_SWITCH_FIRE_DELAY;
	m_flNextSecondaryAttack = m_flNextSecondaryAttack + M16_SECONDARY_FIRE_DELAY;
	if (m_flNextSecondaryAttack < gpGlobals->time)
		m_flNextSecondaryAttack = gpGlobals->time + M16_SECONDARY_FIRE_DELAY;*/
}


void CM16::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 &&
		min(M16_MAX_CLIP - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]) != 0)
	{

		if (m_iClip != 0)
		{
			SendWeaponAnim( M16_RELOAD, UseDecrement() ? 1 : 0 );
			m_pPlayer->m_flNextAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 108.0f / 40.0f;
		}
		else
		{
			SendWeaponAnim( M16_RELOAD_EMPTY, UseDecrement() ? 1 : 0 );
			m_pPlayer->m_flNextAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 128.0f / 40.0f;
		}

		m_fInReload = TRUE;

	}
}


void CM16::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = M16_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = M16_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
}


int CM16::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}





class CM16AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m16clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m16clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_M16CLIP_GIVE, "5_56mm", M16_MAX_AMMO) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_m16clip, CM16AmmoClip );


class CM16AmmoBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m16box.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m16box.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_M16BOX_GIVE, "5_56mm", M16_MAX_AMMO) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_m16box, CM16AmmoBox );
