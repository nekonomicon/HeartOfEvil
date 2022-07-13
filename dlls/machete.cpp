/*	This file was created by looting rather than the productive genius of free men!  Holy fucking shit!
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

#define WEAPON_MACHETE 19 
#define MACHETE_SLOT 0
#define MACHETE_POSITION 1
#define MACHETE_WEIGHT 4

#define	MACHETE_BODYHIT_VOLUME 128
#define	MACHETE_WALLHIT_VOLUME 512

class CMachete : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return MACHETE_SLOT; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	int Swing( int fFirst );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	int m_iSwing;
	TraceResult m_trHit;
};
LINK_ENTITY_TO_CLASS( weapon_machete, CMachete );



enum machete_e {
	MACHETE_IDLE = 0,
	MACHETE_DRAW,
	MACHETE_HOLSTER,
	MACHETE_ATTACK1HIT,
	MACHETE_ATTACK1MISS,
	MACHETE_ATTACK2MISS,
	MACHETE_ATTACK2HIT,
	MACHETE_ATTACK3MISS,
	MACHETE_ATTACK3HIT
};


void CMachete::Spawn( )
{
	Precache( );
	m_iId = WEAPON_MACHETE;
	SET_MODEL(ENT(pev), "models/w_machete.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CMachete::Precache( void )
{
	PRECACHE_MODEL("models/v_machete.mdl");
	PRECACHE_MODEL("models/w_machete.mdl");
	PRECACHE_MODEL("models/p_machete.mdl");
	PRECACHE_SOUND("weapons/machete_hit1.wav");
	PRECACHE_SOUND("weapons/machete_hit2.wav");
	PRECACHE_SOUND("weapons/machete_hitbod1.wav");
	PRECACHE_SOUND("weapons/machete_hitbod2.wav");
	PRECACHE_SOUND("weapons/machete_hitbod3.wav");
	PRECACHE_SOUND("weapons/machete_metal1.wav");
	PRECACHE_SOUND("weapons/machete_metal2.wav");
	PRECACHE_SOUND("weapons/machete_metal3.wav");
	PRECACHE_SOUND("weapons/machete_miss1.wav");
}

int CMachete::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = MACHETE_SLOT;
	p->iPosition = MACHETE_POSITION;
	p->iId = WEAPON_MACHETE;
	p->iWeight = MACHETE_WEIGHT;
	return 1;
}



BOOL CMachete::Deploy( )
{
	return DefaultDeploy( "models/v_machete.mdl", "models/p_machete.mdl", MACHETE_DRAW, "crowbar" );
}

void CMachete::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( MACHETE_HOLSTER );
}


void CMachete::PrimaryAttack()
{
	if (! Swing( 1 ))
	{
		SetThink( SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CMachete::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_MACHETE );
}


void CMachete::SwingAgain( void )
{
	Swing( 0 );
}


int CMachete::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

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

	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			switch( (m_iSwing++) % 3 )
			{
			case 0:
				SendWeaponAnim( MACHETE_ATTACK1MISS ); break;
			case 1:
				SendWeaponAnim( MACHETE_ATTACK2MISS ); break;
			case 2:
				SendWeaponAnim( MACHETE_ATTACK3MISS ); break;
			}
			m_flNextPrimaryAttack = gpGlobals->time + 0.5;
			// play wiff or swish sound
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/machete_miss1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xF));

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		// hit
		fDidHit = TRUE;

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( MACHETE_ATTACK1HIT ); break;
		case 1:
			SendWeaponAnim( MACHETE_ATTACK2HIT ); break;
		case 2:
			SendWeaponAnim( MACHETE_ATTACK3HIT ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage( );
		if ( (m_flNextPrimaryAttack + 1 < gpGlobals->time) || g_pGameRules->IsMultiplayer() )
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgMachete, gpGlobals->v_forward, &tr, DMG_SLASH ); 
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgMachete / 2, gpGlobals->v_forward, &tr, DMG_SLASH ); 
		}	
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		m_flNextPrimaryAttack = gpGlobals->time + 0.25;

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/machete_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/machete_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/machete_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = MACHETE_BODYHIT_VOLUME;
				if (!pEntity->IsAlive() )
					return TRUE;
				else
					flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_MACHETE);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				//UTIL_EmitAmbientSound(ENT(0), ptr->vecEndPos, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/machete_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			case 1:
				//UTIL_EmitAmbientSound(ENT(0), ptr->vecEndPos, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/machete_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			}
		}

		// delay the decal a bit
		m_trHit = tr;
		SetThink( Smack );
		pev->nextthink = gpGlobals->time + 0.2;

		m_pPlayer->m_iWeaponVolume = flVol * MACHETE_WALLHIT_VOLUME;
	}
	return fDidHit;
}



