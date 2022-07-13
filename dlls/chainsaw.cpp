/*	Please do not use this file for evil.
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
#include "animation.h"
#include "soundent.h"
#include "mp3.h"

extern int gmsgPlayMP3;

#define SF_CHAINSAW_DOOMED	0x0001

#define WEAPON_CHAINSAW 22
#define CHAINSAW_SLOT 0
#define CHAINSAW_POSITION 2
#define CHAINSAW_WEIGHT 25

#define	CHAINSAW_BODYHIT_VOLUME 128
#define	CHAINSAW_WALLHIT_VOLUME 512

#define AMMO_GAS_MAX 80
#define AMMO_GAS_START 40
#define AMMO_GAS_GIVE 20

class CChainsaw : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return CHAINSAW_SLOT; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo(ItemInfo *p);

	void TurnOff();
	void TurnOn();

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	TraceResult m_trHit;
	TraceResult tr;
	void ItemPostFrame( void );
	void WeaponIdle( ); 
	Vector GetGunPosition( );
	inline BOOL InAttack();

	void UseFuel(int amt);
	float m_flTimeCheckDamage;
	float m_flTimeHitSound;
	int m_iAnim;
	int m_iFuel;
	BOOL m_fOn;
	void CheckDamage( TraceResult &tr );

	BOOL m_fDoomed;
};
LINK_ENTITY_TO_CLASS( weapon_chainsaw, CChainsaw );


TYPEDESCRIPTION	CChainsaw::m_SaveData[] = 
{
	DEFINE_FIELD( CChainsaw, m_flTimeCheckDamage, FIELD_TIME ),
	DEFINE_FIELD( CChainsaw, m_flTimeHitSound, FIELD_TIME ),
	DEFINE_FIELD( CChainsaw, m_fOn, FIELD_INTEGER ),
	DEFINE_FIELD( CChainsaw, m_iAnim, FIELD_INTEGER ),
	DEFINE_FIELD( CChainsaw, m_iFuel, FIELD_INTEGER ),
	DEFINE_FIELD( CChainsaw, m_fDoomed, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CChainsaw, CBasePlayerWeapon );



enum chainsaw_e {
	CHAINSAW_IDLEOFF = 0,
	CHAINSAW_IDLE,
	CHAINSAW_DRAW,
	CHAINSAW_HOLSTER,
	CHAINSAW_IDLE_HIT,
	CHAINSAW_ATTACK1,
	CHAINSAW_ATTACK2,
	CHAINSAW_ATTACK3,
	CHAINSAW_ATTACK4,
	CHAINSAW_STARTREADY,
	CHAINSAW_STARTFAIL,
	CHAINSAW_STARTEMPTY,
	CHAINSAW_START,
};


void CChainsaw::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CHAINSAW;
	SET_MODEL(ENT(pev), "models/w_chainsaw.mdl");
	m_iClip = -1;

	m_iDefaultAmmo = AMMO_GAS_START;
	m_iFuel = 60;
	m_fOn = FALSE;

	m_fDoomed = pev->spawnflags & SF_CHAINSAW_DOOMED;
	
	if ( m_fDoomed && CAmbientMP3::m_nMp3On == MP3_UNKNOWN )
	{
		CAmbientMP3::m_nMp3On = CAmbientMP3::GetMP3();
	}

	FallInit();// get ready to fall down.

}


void CChainsaw::Precache( void )
{
	PRECACHE_MODEL("models/v_chainsaw.mdl");
	PRECACHE_MODEL("models/w_chainsaw.mdl");
	PRECACHE_MODEL("models/p_chainsaw.mdl");
	PRECACHE_SOUND("weapons/chainsaw_gibs.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	PRECACHE_SOUND("weapons/chainsaw_screech1.wav");
	PRECACHE_SOUND("weapons/chainsaw_screech2.wav");
	PRECACHE_SOUND("weapons/chainsaw_idle1.wav");
	PRECACHE_SOUND("weapons/chainsaw_idle2.wav");
	PRECACHE_SOUND("weapons/chainsaw_idle3.wav");
	PRECACHE_SOUND("weapons/chainsaw_full1.wav");
	PRECACHE_SOUND("weapons/chainsaw_full2.wav");
	PRECACHE_SOUND("weapons/chainsaw_full3.wav");
	PRECACHE_SOUND("weapons/chainsaw_startfail.wav");
	PRECACHE_SOUND("weapons/chainsaw_start.wav");
}

int CChainsaw::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "gas";
	p->iMaxAmmo1 = AMMO_GAS_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = CHAINSAW_SLOT;
	p->iPosition = CHAINSAW_POSITION;
	p->iId = WEAPON_CHAINSAW;
	p->iWeight = CHAINSAW_WEIGHT;
	return 1;
}



BOOL CChainsaw::Deploy( )
{
	TurnOff();
	return DefaultDeploy( "models/v_chainsaw.mdl", "models/p_chainsaw.mdl", CHAINSAW_DRAW, "crowbar" );
}

void CChainsaw::Holster( int skiplocal /* = 0 */ )
{
	TurnOff();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( CHAINSAW_HOLSTER );
}


void CChainsaw::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CHAINSAW );
}


BOOL CChainsaw::InAttack()
{
	return (m_iAnim == CHAINSAW_ATTACK1 || m_iAnim == CHAINSAW_ATTACK2 || m_iAnim == CHAINSAW_ATTACK3 || m_iAnim == CHAINSAW_ATTACK4);
}


void CChainsaw::PrimaryAttack()
{
	if ((m_flTimeWeaponIdle < gpGlobals->time) || !InAttack())
	{
	
		switch( RANDOM_LONG(0,3) )
		{
		case 0:	SendWeaponAnim(m_iAnim = CHAINSAW_ATTACK1); break;
		case 1:	SendWeaponAnim(m_iAnim = CHAINSAW_ATTACK2); break;
		case 2:	SendWeaponAnim(m_iAnim = CHAINSAW_ATTACK3); break;
		case 3:	SendWeaponAnim(m_iAnim = CHAINSAW_ATTACK4); break;
		}

		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		if (m_fOn)
		{
			switch( RANDOM_LONG(0,2) )
			{
			case 0:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/chainsaw_full1.wav", 1.0, ATTN_NORM); break;
			case 1:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/chainsaw_full2.wav", 1.0, ATTN_NORM); break;
			case 2:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/chainsaw_full3.wav", 1.0, ATTN_NORM); break;
			}
		}
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/cbar_miss1.wav", 1.0, ATTN_NORM);
		}

		m_flTimeWeaponIdle = gpGlobals->time + (16.0f / 16.0f);
	}
}


void CChainsaw::TurnOff()
{
	m_fOn = FALSE;
	m_flTimeWeaponIdle = gpGlobals->time;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM);
}


void CChainsaw::TurnOn()
{
	m_fOn = TRUE;
	m_flTimeWeaponIdle = gpGlobals->time;
}


void CChainsaw::SecondaryAttack()
{
	if (m_fOn)
	{
		TurnOff();
		m_flNextSecondaryAttack = gpGlobals->time + 0.4f;
	}
	else
	{
		SendWeaponAnim(m_iAnim = CHAINSAW_STARTREADY);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = gpGlobals->time + 8.0f / 16.0f;
	}

}


void CChainsaw::UseFuel(int amt)
{
	// Decrement fuel

	if (m_iFuel < amt)
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		m_iFuel = m_iFuel + 60 - amt;

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0)
		{
			TurnOff();
		}
	}
	else
	{
		m_iFuel -= amt;
	}
}


void CChainsaw::ItemPostFrame( void )
{	

	// Current animation has come to an end, set new one

	if (m_flTimeWeaponIdle < gpGlobals->time)
	{
		if (m_fOn)
		{
			switch( RANDOM_LONG(0,2) )
			{
			case 0:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/chainsaw_idle1.wav", 1.0, ATTN_NORM); break;
			case 1:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/chainsaw_idle2.wav", 1.0, ATTN_NORM); break;
			case 2:	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/chainsaw_idle3.wav", 1.0, ATTN_NORM); break;
			}

			SendWeaponAnim(m_iAnim = CHAINSAW_IDLE);
			m_flTimeWeaponIdle = gpGlobals->time + (1.0f);
			
		}
		else
		{
			if (m_iAnim == CHAINSAW_STARTREADY || m_iAnim == CHAINSAW_STARTFAIL)
			{
				// Attempting to start the thing up

				if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0 || m_pPlayer->pev->waterlevel == 3)
				{
					// If it's empty or underwater send a fail animation
	
					SendWeaponAnim(m_iAnim = CHAINSAW_STARTEMPTY);
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chainsaw_startfail.wav", 1.0, ATTN_NORM);
					m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = gpGlobals->time + 26.0f / 16.0f;
				}
				else
				{
					// Randomly start or fail to start

					switch (RANDOM_LONG(0, 1))
					{
					case 0: 
						SendWeaponAnim(m_iAnim = CHAINSAW_STARTFAIL);
						EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chainsaw_startfail.wav", 1.0, ATTN_NORM);
						m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = gpGlobals->time + 16.0f / 16.0f;
						break;

					case 1:
						SendWeaponAnim(m_iAnim = CHAINSAW_START);
						
						if ( m_fDoomed && UTIL_PlayMP3( "doomsong.mp3", FALSE, 0 ) )
						{
							m_fDoomed = FALSE;
						}
						else
						{
							EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chainsaw_start.wav", 1.0, ATTN_NORM );
						}
						
						m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = gpGlobals->time + 64.0f / 21.0f;
						break;
					}
				}

			}
			else if (m_iAnim == CHAINSAW_START)
			{
				TurnOn();
			}
			else
			{
				SendWeaponAnim(m_iAnim = CHAINSAW_IDLEOFF);
				m_flTimeWeaponIdle = gpGlobals->time + (32.0f / 16.0f);
			}
		}
	}

	
	// Check for touching anything

	if (m_fOn && m_flTimeCheckDamage < gpGlobals->time)
	{
		if (!InAttack())
		{
			UseFuel(1);
		}
		else
		{
			UseFuel(2);
		}

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

		if ( tr.flFraction < 1.0 )
		{
			// Send damage message out
			
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			ClearMultiDamage( );
			
			if (!InAttack())
			{
				// Normal damage for when just held out in front of you
				
				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgChainsaw, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				
				if (pEntity->BloodColor() != DONT_BLEED)
				{
					SpawnBlood(tr.vecEndPos, pEntity->BloodColor(), 10);
					if (!RANDOM_LONG(0, 4)) 
					{
						CBaseMonster *pMonster = pEntity->MyMonsterPointer();
						if ( pMonster->HasHumanGibs() ) 
						{
							CGib::SpawnRandomGibs( pEntity->pev, 1, GIB_HUMAN_STICKY );
						}
						else if ( pMonster->HasAlienGibs() ) 
						{
							CGib::SpawnRandomGibs( pEntity->pev, 1, GIB_ALIEN_STICKY );
						}
					}
				}
			}
			else
			{
				// Twice the damage if you wave it around

				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgChainsaw * 2, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				if (pEntity->BloodColor() != DONT_BLEED)
				{
					SpawnBlood(tr.vecEndPos, pEntity->BloodColor(), 20);
					if (!RANDOM_LONG(0, 2)) 
					{
						CBaseMonster *pMonster = pEntity->MyMonsterPointer();
						if ( pMonster->HasHumanGibs() ) 
						{
							CGib::SpawnRandomGibs( pEntity->pev, 1, GIB_HUMAN_STICKY );
						}
						else if ( pMonster->HasAlienGibs() ) 
						{
							CGib::SpawnRandomGibs( pEntity->pev, 1, GIB_ALIEN_STICKY );
						}
					}
				}
			}

			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );


			// Do jiggery animation

			if ((m_iAnim == CHAINSAW_IDLE) || 
				(m_iAnim == CHAINSAW_IDLE_HIT && m_flTimeWeaponIdle < gpGlobals->time))
			{
				SendWeaponAnim(m_iAnim = CHAINSAW_IDLE_HIT);
				m_flTimeWeaponIdle = gpGlobals->time + (16.0f / 16.0f);
			}

			
			// Screen shake
			
			m_pPlayer->pev->punchangle.x += RANDOM_FLOAT( -2.0, 2.0 );
			m_pPlayer->pev->punchangle.z += RANDOM_FLOAT( -2.0, 2.0 );
				

			if (pEntity->Classify() == CLASS_NONE || pEntity->Classify() == CLASS_MACHINE || pEntity->Classify() == CLASS_HELICOPTER )
			{
				if (TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, BULLET_PLAYER_CHAINSAW) > 0.25)
				{
					// Inanimate object (sparks & screech)

					if (m_flTimeHitSound <= gpGlobals->time)
					{
						switch (RANDOM_LONG(0, 1))
						{
						case 0: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chainsaw_screech1.wav", 1, ATTN_NORM); break;
						case 1: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chainsaw_screech2.wav", 1, ATTN_NORM); break;
						}
	
						m_flTimeHitSound = gpGlobals->time + 1.0f;
					}

					if (RANDOM_LONG(0, 1)) UTIL_Sparks( tr.vecEndPos );
				}
			}
			else if ( pEntity->Classify() == CLASS_PLANT )
			{
			}
			else
			{
				// Living entity (blood)

				if (m_flTimeHitSound < gpGlobals->time)
				{
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/chainsaw_gibs.wav", 1, ATTN_NORM);

					m_flTimeHitSound = gpGlobals->time + 2.96f;
				}
			}


		}
		else
		{
			// Not hitting anything

			
			// Minor screen shake
			
			m_pPlayer->pev->punchangle.x += RANDOM_FLOAT( -0.3, 0.3 );
			m_pPlayer->pev->punchangle.z += RANDOM_FLOAT( -0.3, 0.3 );

			
			if (m_flTimeHitSound >= gpGlobals->time)
			{
				// Make the gibs or screech noise stop
			
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM);
				m_flTimeHitSound = gpGlobals->time;
			}
			
			// Make the jiggery animation stop
			
			if (m_iAnim == CHAINSAW_IDLE_HIT)
			{
				m_flTimeWeaponIdle = gpGlobals->time;
			}
		}


		// If we are underwater, turn off
		
		if (m_pPlayer->pev->waterlevel == 3)
		{
			TurnOff();
		}

		m_flTimeCheckDamage = m_flTimeCheckDamage + 0.1;

		m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
		m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.3;
	}

	
	CBasePlayerWeapon::ItemPostFrame( );
}


Vector CChainsaw::GetGunPosition( )
{
	Vector v, a;
	GetAttachment(0, v, a);

	return v;
}


void CChainsaw::WeaponIdle( )
{
}


class CAmmoGas : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_gas.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_gas.mdl");
		PRECACHE_SOUND("items/ammo_gas1.wav");
		PRECACHE_SOUND("items/ammo_gas2.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_GAS_GIVE, "gas", AMMO_GAS_MAX) != -1);
		if (bResult)
		{
			if (RANDOM_LONG(0, 1))
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/ammo_gas1.wav", 1, ATTN_NORM);
			else
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/ammo_gas2.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_gas, CAmmoGas );
