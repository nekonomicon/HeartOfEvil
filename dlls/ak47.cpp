/***
*
* AK 47 code by Nathan Ruck
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


#define WEAPON_AK47 23
#define AK47_SLOT 2
#define AK47_POSITION 3
#define AK47_WEIGHT 11


#define AK47_DEFAULT_GIVE 30
#define AK47_MAX_CARRY 250
#define AK47_MAX_CLIP 30

#define AMMO_AK47CLIP_GIVE 30


enum AK47_e
{
	AK47_LONGIDLE = 0,
	AK47_IDLE1,
	AK47_RELOAD,
	AK47_RELOAD_EMPTY,
	AK47_DEPLOY,
	AK47_HOLSTER,
	AK47_FIRE1,
};


class CAK47 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return AK47_SLOT; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int SecondaryAmmoIndex( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_ak47, CAK47 );


//=========================================================
//=========================================================
int CAK47::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CAK47::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_ak47.mdl");
	m_iId = WEAPON_AK47;

	m_iDefaultAmmo = AK47_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CAK47::Precache( void )
{
	PRECACHE_MODEL("models/v_ak47.mdl");
	PRECACHE_MODEL("models/w_ak47.mdl");
	PRECACHE_MODEL("models/p_ak47.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("weapons/ak47_clipinsert1.wav");
	PRECACHE_SOUND("weapons/ak47_cliprelease1.wav");
	PRECACHE_SOUND("weapons/ak47_slide.wav");
//	PRECACHE_SOUND("items/guncock1.wav");

	PRECACHE_SOUND ("weapons/ak47_fire1.wav");// H to the K
	PRECACHE_SOUND ("weapons/ak47_fire2.wav");// H to the K
	PRECACHE_SOUND ("weapons/ak47_fire3.wav");// H to the K

	PRECACHE_SOUND ("weapons/357_cock1.wav");
}

int CAK47::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "7_62x39mm_M1943";
	p->iMaxAmmo1 = AK47_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AK47_MAX_CLIP;
	p->iSlot = AK47_SLOT;
	p->iPosition = AK47_POSITION;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AK47;
	p->iWeight = AK47_WEIGHT;

	return 1;
}

int CAK47::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CAK47::Deploy( )
{
	return DefaultDeploy( "models/v_ak47.mdl", "models/p_ak47.mdl", AK47_DEPLOY, "mp5" );
}


void CAK47::PrimaryAttack()
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

	switch(RANDOM_LONG(0, 2))
	{
	case 0:	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ak47_fire1.wav", 1, ATTN_NORM); break;
	case 1:	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ak47_fire2.wav", 1, ATTN_NORM); break;
	case 2:	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ak47_fire3.wav", 1, ATTN_NORM); break;
	}
	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;
	
	Vector	vecShellVelocity = gpGlobals->v_right * 60 + gpGlobals->v_up * 200 + gpGlobals->v_forward * 40;
	Vector	vecShellOrigin = pev->origin + pev->view_ofs + gpGlobals->v_forward * 30 + gpGlobals->v_right * 30;
	EjectBrass ( vecShellOrigin, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	
	m_iClip--;


	SendWeaponAnim( AK47_FIRE1 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	
	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_4DEGREES, 8192, BULLET_PLAYER_AK47, 2 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.1;
	if (m_flNextPrimaryAttack < gpGlobals->time)
		m_flNextPrimaryAttack = gpGlobals->time + 0.1;

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}



void CAK47::SecondaryAttack( void )
{
}

void CAK47::Reload( void )
{
	if (m_iClip != 0)
	{
		DefaultReload( AK47_MAX_CLIP, AK47_RELOAD, 82.0f/40.0f );
	}
	else
	{
		DefaultReload( AK47_MAX_CLIP, AK47_RELOAD_EMPTY, 108.0f/40.0f );
	}
}



void CAK47::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = AK47_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = AK47_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
}



class CAK47AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ak47clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ak47clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_AK47CLIP_GIVE, "7_62x39mm_M1943", AK47_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_ak47clip, CAK47AmmoClip );

