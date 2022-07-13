#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"


#define WEAPON_REM870 18 
#define REM870_SLOT 2
#define REM870_POSITION 4
#define REM870_WEIGHT 12


#define REM870_MODEL_1STPERSON "models/v_870.mdl" 
#define REM870_MODEL_3RDPERSON "models/p_870.mdl"
#define REM870_MODEL_WORLD "models/w_870.mdl"

#define REM870_SOUND_BUCKSHOT "weapons/870_buckshot.wav" 
#define REM870_SOUND_ELEPHANTSHOT "weapons/870_elephantshot.wav" 
#define REM870_SOUND_VOLUME 1

#define REM870_PRIMARY_FIRE_DELAY 0.35
#define REM870_SECONDARY_FIRE_DELAY 0.075
#define REM870_RELOAD_TIME 3.6
#define REM870_DEFAULT_AMMO 7 
#define REM870_MAX_CLIP 7

#define ELEPHANTSHOT_MAX_CARRY 40
#define AMMO_ELEPHANTSHOT_GIVE 8
#define Q_EMPTY 0
#define Q_BUCKSHOT 6
#define Q_ELEPHANTSHOT 30


// special deathmatch 870 spreads
#define VECTOR_CONE_DM_870	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLE870 Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum REM870_e {
	REM870_IDLE = 0,
	REM870_FIRE,
	REM870_FIRE2,
	REM870_RELOAD,
	REM870_PUMP,
	REM870_START_RELOAD,
	REM870_DRAW,
	REM870_HOLSTER,
	REM870_IDLE_DEEP
};

class C870 : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return REM870_SLOT; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( );
	void Reload( );
	void Reload1( );
	void WeaponIdle( void );
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;
	int m_iElephantShell;
//	float m_flPumpTime;

	int m_AmmoType;
	int m_AmmoQueue[REM870_MAX_CLIP];
private:
	unsigned short m_usDoubleFire;
	unsigned short m_usSingleFire;
};
LINK_ENTITY_TO_CLASS( weapon_870, C870 );


TYPEDESCRIPTION	C870::m_SaveData[] = 
{
	DEFINE_FIELD( C870, m_flNextReload, FIELD_TIME ),
	DEFINE_FIELD( C870, m_fInReload, FIELD_INTEGER ),
	DEFINE_FIELD( C870, m_flNextReload, FIELD_TIME ),
	// DEFINE_FIELD( C870, m_iShell, FIELD_INTEGER ),
//	DEFINE_FIELD( C870, m_flPumpTime, FIELD_TIME ),
	DEFINE_ARRAY( C870, m_AmmoQueue, FIELD_INTEGER, REM870_MAX_CLIP ),
	DEFINE_FIELD( C870, m_AmmoType, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE( C870, CBasePlayerWeapon );



void C870::Spawn( )
{
	pev->classname = MAKE_STRING( "weapon_870" ); 
	Precache( );
	m_iId = WEAPON_REM870;
	SET_MODEL(ENT(pev), REM870_MODEL_WORLD);

	m_iDefaultAmmo = REM870_DEFAULT_AMMO;

	FallInit();// get ready to fall

	for (int i = 0; i < REM870_MAX_CLIP; i++) m_AmmoQueue[i] = Q_EMPTY;
	for (i = 0; i < REM870_DEFAULT_AMMO; i++) m_AmmoQueue[i] = Q_BUCKSHOT;
	m_AmmoType = Q_BUCKSHOT;
}


void C870::Precache( void )
{
	PRECACHE_MODEL(REM870_MODEL_1STPERSON);
	PRECACHE_MODEL(REM870_MODEL_WORLD);
	PRECACHE_MODEL(REM870_MODEL_3RDPERSON);

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// buckshot shell
	m_iElephantShell = PRECACHE_MODEL ("models/elephantshell.mdl");// elephantshot shell

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND (REM870_SOUND_BUCKSHOT);
	PRECACHE_SOUND (REM870_SOUND_ELEPHANTSHOT);

	PRECACHE_SOUND ("weapons/reload1.wav");	// 870 reload
	PRECACHE_SOUND ("weapons/reload2.wav");	// 870 reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// 870 reload

	PRECACHE_SOUND ("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND ("weapons/870_pump.wav");	// cock gun

}

int C870::AddToPlayer( CBasePlayer *pPlayer )
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


int C870::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = "elephantshot";
	p->iMaxAmmo2 = ELEPHANTSHOT_MAX_CARRY;
	p->iMaxClip = REM870_MAX_CLIP;
	p->iSlot = REM870_SLOT;
	p->iPosition = REM870_POSITION;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_REM870;
	p->iWeight = REM870_WEIGHT;

	return 1;
}



BOOL C870::Deploy( )
{
	return DefaultDeploy( REM870_MODEL_1STPERSON, REM870_MODEL_3RDPERSON, REM870_DRAW, "shotgun" );
}


void C870::PrimaryAttack()
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
		Reload1( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector	vecShellVelocity = gpGlobals->v_right * 60 + gpGlobals->v_up * 200 + gpGlobals->v_forward * 40;
	Vector	vecShellOrigin = pev->origin + pev->view_ofs + gpGlobals->v_forward * 30 + gpGlobals->v_right * 30;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	if ( m_AmmoQueue[m_iClip - 1] == Q_ELEPHANTSHOT )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, REM870_SOUND_ELEPHANTSHOT, REM870_SOUND_VOLUME, ATTN_NORM);
	
		m_pPlayer->FireBullets( m_AmmoQueue[m_iClip - 1], vecSrc, vecAiming, VECTOR_CONE_20DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 );
		
		m_pPlayer->pev->punchangle.x -= 20.0;
		if ( FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		{
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 200;
		}
		
		EjectBrass ( vecShellOrigin, vecShellVelocity, pev->angles.y, m_iElephantShell, TE_BOUNCE_SHELL);
		
		SendWeaponAnim( REM870_FIRE2 );
		
		m_flNextPrimaryAttack = gpGlobals->time + 56.0f / 40.0f;
	}
	else
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, REM870_SOUND_BUCKSHOT, REM870_SOUND_VOLUME, ATTN_NORM);

		m_pPlayer->FireBullets( m_AmmoQueue[m_iClip - 1], vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 );
		
		EjectBrass ( vecShellOrigin, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 
		
		SendWeaponAnim( REM870_FIRE );
		
		m_flNextPrimaryAttack = gpGlobals->time + 40.0f / 40.0f;
	}

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;

	m_AmmoQueue[m_iClip - 1] = Q_EMPTY;
	m_iClip--;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

//	if (m_iClip != 0)
//		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextSecondaryAttack = gpGlobals->time + 0.75;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
	else
		m_flTimeWeaponIdle = gpGlobals->time + 0.75;
	m_fInReload = 0;
}


void C870::SecondaryAttack( void )
{
	m_AmmoType = Q_ELEPHANTSHOT;
	Reload1( );
}


void C870::Reload( )
{
	if (m_pPlayer->pev->button & IN_RELOAD) m_AmmoType = Q_BUCKSHOT;
	Reload1( );
}


void C870::Reload1( )
{
	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->time)
		return;

	if ( m_iClip == REM870_MAX_CLIP ) 
		return;
	
	if ( m_AmmoType == Q_BUCKSHOT && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_AmmoType = Q_ELEPHANTSHOT;

	if ( m_AmmoType == Q_ELEPHANTSHOT && m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] <= 0 )
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]) m_AmmoType = Q_BUCKSHOT;
		else return;
	}

	if (m_flNextReload > gpGlobals->time)
		return;

	// check to see if we're ready to reload
	if (m_fInReload == 0)
	{
		SendWeaponAnim( REM870_START_RELOAD );
		m_fInReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flTimeWeaponIdle = gpGlobals->time + 0.6;
		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flNextSecondaryAttack = gpGlobals->time + 1.0;
		return;
	}
	else if (m_fInReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->time)
			return;
		// was waiting for gun to move to side
		m_fInReload = 2;

		switch (RANDOM_LONG(0,1))
		{
		case 0:	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f)); break;
		case 1:	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload2.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f)); break;
		case 2:	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f)); break;
		}

		if (m_AmmoType == Q_BUCKSHOT)
		{
			pev->body = 0;
		}
		else
		{
			pev->body = 1;
		}

		SendWeaponAnim( REM870_RELOAD );

		m_flNextReload = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;

		if (m_AmmoType == Q_BUCKSHOT) m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		else m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] -= 1;
		
		// Add to back of queue
		//for (int i = REM870_MAX_CLIP - 1; i > 0; i--) m_AmmoQueue[i] = m_AmmoQueue[i-1];
		//m_AmmoQueue[0] = m_AmmoType;

		// Add to FRONT of queue
		m_AmmoQueue[m_iClip - 1] = m_AmmoType;

		m_fInReload = 1;
	}
}


void C870::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

/*	if (m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
		// play pumping sound
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/870_pump.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_flPumpTime = 0;
	}
*/
	if (m_flTimeWeaponIdle < gpGlobals->time)
	{
		if (m_iClip == 0 && m_fInReload == 0)
		{
			// Auto reload when clip is empty
			Reload1( );
		}
		else if (m_fInReload != 0)
		{
			if (m_iClip != REM870_MAX_CLIP 
				&& (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] || m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]))
			// if more can be loaded
			{
				Reload1( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( REM870_PUMP );
//				m_flPumpTime = gpGlobals->time + 0.5;
			
				m_fInReload = 0;
				m_flTimeWeaponIdle = gpGlobals->time + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = RANDOM_FLOAT(0, 1);
			if (flRand <= 0.8)
			{
				iAnim = REM870_IDLE_DEEP;
				m_flTimeWeaponIdle = gpGlobals->time + (64.0/20.0);// * RANDOM_LONG(2, 5);
			}
			else
			{
				iAnim = REM870_IDLE;
				m_flTimeWeaponIdle = gpGlobals->time + (32.0/20.0);
			}
			SendWeaponAnim( iAnim );
		}
	}
}


BOOL C870 :: AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry )
{
	int iIdAmmo;

	if (iMaxClip < 1)
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	else if (m_iClip == 0)
	{
		int i;
		i = min( m_iClip + iCount, iMaxClip ) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount - i, szName, iMaxCarry );
		for (int j = 0; j < i; j++) m_AmmoQueue[j] = Q_BUCKSHOT;
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	
	// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxCarry; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iPrimaryAmmoType = iIdAmmo;
		if (m_pPlayer->HasPlayerItem( this ) )
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}


class CElephantShot : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_elephant_10shells.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_elephant_10shells.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 10, "elephantshot", ELEPHANTSHOT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_elephantshot, CElephantShot );

class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_shot_10shells.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_shot_10shells.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 10, "buckshot", BUCKSHOT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo );


class CElephantShotLarge : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_elephant_22shells.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_elephant_22shells.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 22, "elephantshot", ELEPHANTSHOT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_elephantshot_large, CElephantShotLarge );

class CShotgunAmmoLarge : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_shot_22shells.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_shot_22shells.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 22, "buckshot", BUCKSHOT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_buckshot_large, CShotgunAmmoLarge );


