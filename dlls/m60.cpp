//*****************************************************************
//
//	M60 7.62 mm machine gun
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


#define WEAPON_M60 17
#define M60_SLOT 3
#define M60_POSITION 1
#define M60_WEIGHT 10


#define M60_MODEL_1STPERSON "models/v_m60.mdl" 
#define M60_MODEL_3RDPERSON "models/p_m60.mdl"
#define M60_MODEL_WORLD "models/w_m60.mdl"

#define M60_SOUND_SHOOT "weapons/m60_fire.wav" 
#define M60_SOUND_VOLUME 1

#define M60_FIRE_DELAY 0.1
#define M60_FIRE_TIME 0.2
#define M60_RELOAD_TIME 2.3
#define M60_DEFAULT_AMMO 50 
#define M60_MAX_CLIP 50

#define M60_BODY_GUN 0
#define M60_BODY_AMMO 1


enum m60_animations
{
	M60_LONGIDLE = 0,
	M60_IDLE1,
	M60_RELOAD1,
	M60_RELOAD1_EMPTY,
	M60_RELOAD2,
	M60_DEPLOY,
	M60_FIRE1,
	M60_HOLSTER,
};


class CM60 : public CBasePlayerWeapon 
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
	void ItemPostFrame( void );
	void UpdateAmmoBelt( );
	BOOL AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry );
	BOOL Fire();
	float m_flReload2Time;
	float m_flUpdateAmmoTime;

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
}; 

LINK_ENTITY_TO_CLASS( weapon_m60, CM60 );


TYPEDESCRIPTION	CM60::m_SaveData[] = 
{
	DEFINE_FIELD( CM60, m_flReload2Time, FIELD_TIME ),
	DEFINE_FIELD( CM60, m_flUpdateAmmoTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CM60, CBasePlayerWeapon );


BOOL CM60::AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry )
{
	BOOL ret = CBasePlayerWeapon::AddPrimaryAmmo( iCount, szName, iMaxClip, iMaxCarry );
	UpdateAmmoBelt();
	return ret;
}


void CM60::UpdateAmmoBelt( )
{
	pev->body = ( m_iClip <= 0 ? 0 : m_iClip < 18 ? m_iClip : 18 );
}


void CM60 :: Spawn( ) 
{ 
	pev->classname = MAKE_STRING( "weapon_m60" ); 
	Precache( ); 

	m_iId = WEAPON_M60; 
	SET_MODEL( ENT(pev), M60_MODEL_WORLD ); 

	m_iDefaultAmmo = M60_DEFAULT_AMMO; 

	FallInit( ); 
	m_flReload2Time = 0.0;
	m_flUpdateAmmoTime = 0.0;
}


void CM60 :: Precache( ) 
{ 
	PRECACHE_MODEL( M60_MODEL_1STPERSON ); 
	PRECACHE_MODEL( M60_MODEL_3RDPERSON ); 
	PRECACHE_MODEL( M60_MODEL_WORLD ); 
	
	PRECACHE_SOUND( M60_SOUND_SHOOT ); 

	PRECACHE_SOUND( "weapons/m60_close.wav" ); 
	PRECACHE_SOUND( "weapons/m60_reload_open.wav" ); 
	PRECACHE_SOUND( "weapons/m60_reload_empty.wav" ); 
	PRECACHE_SOUND( "weapons/m60_reload_full.wav" ); 

	PRECACHE_SOUND ("weapons/357_cock1.wav");
	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

}


int CM60 :: iItemSlot( ) 
{ 
	return M60_SLOT; 
}


int CM60 :: GetItemInfo( ItemInfo* Info ) 
{ 
	Info->pszName = STRING( pev->classname ); 
	Info->pszAmmo1 = "7_62mm"; 
	Info->iMaxAmmo1 = AMMO_762_MAX; 
	Info->pszAmmo2 = NULL; 
	Info->iMaxAmmo2 = -1; 
	Info->iMaxClip = M60_MAX_CLIP; 
	Info->iSlot = M60_SLOT;
	Info->iPosition = M60_POSITION; 
	Info->iFlags = 0; 
	Info->iId = WEAPON_M60; 
	Info->iWeight = M60_WEIGHT;

	return 1; 
}


BOOL CM60 :: Deploy( ) 
{ 
return DefaultDeploy( M60_MODEL_1STPERSON, 
                      M60_MODEL_3RDPERSON, 
                      M60_DEPLOY, 
                      "mp5" ); 
}


Vector CM60::GetGunPosition( )
{
	Vector v, a;
	GetAttachment(0, v, a);

	return v;
}


BOOL CM60::Fire()
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

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, M60_SOUND_SHOOT, M60_SOUND_VOLUME, ATTN_NORM);
	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;
	
	Vector	vecShellVelocity = gpGlobals->v_right * 60 + gpGlobals->v_up * 200 + gpGlobals->v_forward * 40;
	Vector	vecShellOrigin = pev->origin + pev->view_ofs + gpGlobals->v_forward * 30 + gpGlobals->v_right * 30;
	EjectBrass ( vecShellOrigin, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	
	UpdateAmmoBelt();
	m_iClip--;
	m_flUpdateAmmoTime = gpGlobals->time + M60_FIRE_TIME;

    // player "shoot" animation
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	SendWeaponAnim( M60_FIRE1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	
	if ( g_pGameRules->IsDeathmatch() )
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 8192, BULLET_PLAYER_762, 2 );
	}
	else
	{
		// single player spread
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_762, 2 );
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	return TRUE;
}


void CM60::PrimaryAttack()
{
	if (Fire())
	{
		UTIL_MakeAimVectors( pev->angles );
		m_pPlayer->pev->punchangle.x += RANDOM_FLOAT( -2.0, 2.0 );
		m_pPlayer->pev->punchangle.z += RANDOM_FLOAT( -2.0, 2.0 );
		if ( FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		{
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 40;
		}
	}
	m_flNextPrimaryAttack = m_flNextPrimaryAttack + M60_FIRE_DELAY;
	if (m_flNextPrimaryAttack < gpGlobals->time)
		m_flNextPrimaryAttack = gpGlobals->time + M60_FIRE_DELAY;
}


void CM60::SecondaryAttack()
{
}


void CM60::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 &&
		min(M60_MAX_CLIP - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]) != 0)
	{
		UpdateAmmoBelt();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + M60_RELOAD_TIME;

		if (m_iClip)
			SendWeaponAnim( M60_RELOAD1, UseDecrement() ? 1 : 0 );
		else
			SendWeaponAnim( M60_RELOAD1_EMPTY, UseDecrement() ? 1 : 0 );

		m_fInReload = TRUE;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + M60_RELOAD_TIME * 2;
		m_flReload2Time = UTIL_WeaponTimeBase() + M60_RELOAD_TIME;
	}
}


void CM60::ItemPostFrame( void )
{
	if (m_flUpdateAmmoTime != 0.0 && m_flUpdateAmmoTime <= gpGlobals->time)
	{
		UpdateAmmoBelt();
		m_flUpdateAmmoTime = 0.0;
	}

	if (m_flReload2Time != 0.0 && m_flReload2Time <= gpGlobals->time)
	{
		pev->body = ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 ? 0 : 
					  m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 18 ? m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] : 18 );
		SendWeaponAnim( M60_RELOAD2, UseDecrement() ? 1 : 0 );
		m_flReload2Time = 0.0;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + M60_RELOAD_TIME;
		return;
	}

	CBasePlayerWeapon::ItemPostFrame( );
}


void CM60::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = M60_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = M60_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
}


int CM60::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		UpdateAmmoBelt( );
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}


class C762AmmoBox : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m60box.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m60box.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_762BOX_GIVE, "7_62mm", AMMO_762_MAX) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_762mmbox, C762AmmoBox );
