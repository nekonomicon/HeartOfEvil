/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define WEAPON_M79 20
#define M79_SLOT 3
#define M79_POSITION 0
#define M79_WEIGHT 8

#define M79_MODEL_1STPERSON "models/v_m79.mdl" 
#define M79_MODEL_3RDPERSON "models/p_m79.mdl"
#define M79_MODEL_WORLD "models/w_m79.mdl"

#define M79_DEFAULT_GIVE 1
#define M79_MAX_CLIP 1


enum m79_e
{
	M79_LONGIDLE = 0,
	M79_IDLE1,
	M79_RELOAD,
	M79_DEPLOY,
	M79_HOLSTER,
	M79_FIRE1,
};


class CM79 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return M79_SLOT; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_m79, CM79 );


//=========================================================
//=========================================================

void CM79::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m79"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), M79_MODEL_WORLD);
	m_iId = WEAPON_M79;

	m_iDefaultAmmo = M79_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM79::Precache( void )
{
	PRECACHE_MODEL(M79_MODEL_1STPERSON);
	PRECACHE_MODEL(M79_MODEL_WORLD);
	PRECACHE_MODEL(M79_MODEL_3RDPERSON);

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL
	PRECACHE_SOUND ("weapons/357_cock1.wav");

	PRECACHE_SOUND ("weapons/m79_open.wav");
	PRECACHE_SOUND ("weapons/m79_eject.wav");
	PRECACHE_SOUND ("weapons/m79_shelldrop.wav");
	PRECACHE_SOUND ("weapons/m79_insert.wav");
	PRECACHE_SOUND ("weapons/m79_close.wav");

	PRECACHE_MODEL("models/grenade.mdl");	// grenade

	PRECACHE_SOUND( "weapons/m79_fire1.wav" );
	PRECACHE_SOUND( "weapons/m79_fire2.wav" );

}

int CM79::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ARgrenades";
	p->iMaxAmmo1 = M203_GRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M79_MAX_CLIP;
	p->iSlot = M79_SLOT;
	p->iPosition = M79_POSITION;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M79;
	p->iWeight = M79_WEIGHT;

	return 1;
}

int CM79::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CM79::Deploy( )
{
	return DefaultDeploy( M79_MODEL_1STPERSON, M79_MODEL_3RDPERSON, M79_DEPLOY, "shotgun" );
}


void CM79::PrimaryAttack()
{

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	m_iClip--;

	SendWeaponAnim( M79_FIRE1 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;

	if ( RANDOM_LONG(0,1) )
	{
		// play this sound through BODY channel so we can hear it if player didn't stop firing MP3
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m79_fire1.wav", 0.8, ATTN_NORM);
	}
	else
	{
		// play this sound through BODY channel so we can hear it if player didn't stop firing MP3
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m79_fire2.wav", 0.8, ATTN_NORM);
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
	CGrenade::ShootContact( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16, 
							gpGlobals->v_forward * 800 );
	
	m_flNextPrimaryAttack = gpGlobals->time + 1;
	m_flTimeWeaponIdle = gpGlobals->time + 5;// idle pretty soon after shooting.

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_pPlayer->pev->punchangle.x -= 10;
	
}


void CM79::SecondaryAttack( void )
{
}


void CM79::Reload( void )
{
	DefaultReload( M79_MAX_CLIP, M79_RELOAD, 3.0 );
}


void CM79::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = M79_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = M79_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
}


#define M79_VEST_GIVE 20

class CM79Vest : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_m79vest.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_m79vest.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( M79_VEST_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_m79vest, CM79Vest );



class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ARgrenade.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_M203BOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY ) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5grenades, CMP5AmmoGrenade );
LINK_ENTITY_TO_CLASS( ammo_ARgrenades, CMP5AmmoGrenade );
