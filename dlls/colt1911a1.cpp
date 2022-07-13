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

#define WEAPON_COLT1911A1 21
#define COLT1911A1_SLOT 1
#define COLT1911A1_POSITION 2
#define COLT1911A1_WEIGHT 12

#define COLT1911A1_MODEL_1STPERSON "models/v_colt1911a1.mdl" 
#define COLT1911A1_MODEL_3RDPERSON "models/p_colt1911a1.mdl"
#define COLT1911A1_MODEL_WORLD "models/w_colt1911a1.mdl"

#define COLT1911A1_DEFAULT_GIVE 7
#define COLT1911A1_MAX_CLIP 7
#define COLT1911A1_MAX_CARRY 200


enum COLT1911A1_e
{
	COLT1911A1_IDLE1 = 0,
	COLT1911A1_IDLE_EMPTY,
	COLT1911A1_RELOAD,
	COLT1911A1_RELOAD_EMPTY,
	COLT1911A1_DEPLOY,
	COLT1911A1_DEPLOY_EMPTY,
	COLT1911A1_HOLSTER,
	COLT1911A1_HOLSTER_EMPTY,
	COLT1911A1_FIRE,
	COLT1911A1_FIRE_LAST,
	COLT1911A1_PISTOLWHIP,
};


class CColt1911A1 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return COLT1911A1_SLOT; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
	BOOL m_fTrigger;
	void ItemPostFrame( void );
	void EXPORT PistolWhipThink( void );
};
LINK_ENTITY_TO_CLASS( weapon_colt1911a1, CColt1911A1 );


//=========================================================
//=========================================================

void CColt1911A1::ItemPostFrame( void )
{
	if (m_fTrigger && !(m_pPlayer->pev->button & IN_ATTACK)) m_fTrigger = FALSE;
	
	CBasePlayerWeapon::ItemPostFrame( );
}


void CColt1911A1::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_colt1911a1"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), COLT1911A1_MODEL_WORLD);
	m_iId = WEAPON_COLT1911A1;

	m_iDefaultAmmo = COLT1911A1_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
	m_fTrigger = FALSE;
}


void CColt1911A1::Precache( void )
{
	PRECACHE_MODEL(COLT1911A1_MODEL_1STPERSON);
	PRECACHE_MODEL(COLT1911A1_MODEL_WORLD);
	PRECACHE_MODEL(COLT1911A1_MODEL_3RDPERSON);

	PRECACHE_MODEL("models/w_1143mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/colt_insert.wav");
	PRECACHE_SOUND ("weapons/colt_release.wav");
	PRECACHE_SOUND ("weapons/colt_fire1.wav");
	PRECACHE_SOUND ("weapons/colt_fire2.wav");
	PRECACHE_SOUND ("weapons/colt_slide.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");

	PRECACHE_SOUND("weapons/colt_swing1.wav");
	PRECACHE_SOUND("weapons/colt_swing2.wav");
	PRECACHE_SOUND("weapons/colt_smash1.wav");
	PRECACHE_SOUND("weapons/colt_smash2.wav");
}

int CColt1911A1::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "11_43mm";
	p->iMaxAmmo1 = COLT1911A1_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = COLT1911A1_MAX_CLIP;
	p->iSlot = COLT1911A1_SLOT;
	p->iPosition = COLT1911A1_POSITION;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_COLT1911A1;
	p->iWeight = COLT1911A1_WEIGHT;

	return 1;
}

int CColt1911A1::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CColt1911A1::Deploy( )
{
	return DefaultDeploy( COLT1911A1_MODEL_1STPERSON, COLT1911A1_MODEL_3RDPERSON, COLT1911A1_DEPLOY, "python" );
}


void CColt1911A1::PrimaryAttack()
{
	if (m_fTrigger) return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		}

		return;
	}


	m_iClip--;

	if (m_iClip)
		SendWeaponAnim( COLT1911A1_FIRE );
	else
		SendWeaponAnim( COLT1911A1_FIRE_LAST );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_2DEGREES, 8192, BULLET_PLAYER_1143, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time;
	m_flNextSecondaryAttack = gpGlobals->time + 16.0 / 40.0;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	m_fTrigger = TRUE;

	m_pPlayer->pev->punchangle.x -= 2;
	
	switch (RANDOM_LONG(0, 1))
	{
	case 0:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/colt_fire1.wav", 0.8, ATTN_NORM); break;
	case 1:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/colt_fire2.wav", 0.8, ATTN_NORM); break;
	}
	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;
}


void CColt1911A1::SecondaryAttack( void )
{
	SendWeaponAnim( COLT1911A1_PISTOLWHIP );

	m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 20.0 / 30.0;
	
	// play wiff or swish sound
	switch( RANDOM_LONG(0,1) )
	{
	case 0:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/colt_swing1.wav", 1, ATTN_NORM); break;
	case 1:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/colt_swing2.wav", 1, ATTN_NORM); break;
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	SetThink( PistolWhipThink );
	pev->nextthink = gpGlobals->time + 10.0 / 30.0;

}


void CColt1911A1::PistolWhipThink( void )
{
	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	if ( tr.flFraction < 1.0 )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		
		if ( pEntity )
		{
			ClearMultiDamage( );
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgPistolWhip, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
		}

		if (pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)	// hit flesh
		{
			// play thwack or smack sound
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/colt_smash1.wav", 1, ATTN_NORM); break;
			case 1:
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/colt_smash2.wav", 1, ATTN_NORM); break;
			}
		}
		else	// hit world or machine
		{
			TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_PISTOLWHIP);
			DecalGunshot( &tr, BULLET_PLAYER_PISTOLWHIP );
		}
	}
}


void CColt1911A1::Reload( void )
{
	DefaultReload( COLT1911A1_MAX_CLIP, (m_iClip ? COLT1911A1_RELOAD : COLT1911A1_RELOAD_EMPTY), 48.0 / 30.0 );
}


void CColt1911A1::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;

	if (m_iClip)
		iAnim = COLT1911A1_IDLE1;
	else
		iAnim = COLT1911A1_IDLE_EMPTY;

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
}



class C1143Ammo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_1143mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_1143mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( COLT1911A1_DEFAULT_GIVE, "11_43mm", COLT1911A1_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_1143mm, C1143Ammo );


class C1143Ammo_40Box : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_1143mm_40box.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_1143mm_40box.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 40, "11_43mm", COLT1911A1_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_1143mm_40box, C1143Ammo_40Box );


class C1143Ammo_50Box : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_1143mm_50box.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_1143mm_50box.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 50, "11_43mm", COLT1911A1_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_1143mm_50box, C1143Ammo_50Box );
