/**
*   $Log: m21.cpp,v $
*   Revision 1.24  2002/03/05 22:17:03  vertigo
*   Fixed the Net game recoil problem
*
*   Revision 1.23  2002/02/26 07:05:43  vertigo
*   Fixed mistake in the next primary fire time
*
*   Revision 1.22  2002/02/24 08:32:00  vertigo
*   Pistol and rifle shells
*
*   Revision 1.21  2002/02/19 07:08:01  vertigo
*   Created an ammo crate class that give players full ammo and removed the individual ammo clips.
*
*   Revision 1.20  2002/02/19 03:25:53  vertigo
*   All weapons now come with max ammo, so we don't have to give ammo too
*
*   Revision 1.19  2002/02/13 00:20:54  vertigo
*   All weapons now recoil up and to the right by varying amounts and the prediction is the same on the client and the server
*
*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"

#include "m21.h"

LINK_ENTITY_TO_CLASS( weapon_m21, CM21 );

//=========================================================
//=========================================================
void CM21::Spawn( )
{
    pev->classname = MAKE_STRING("weapon_m21"); // hack to allow for old names
    Precache( );
    SET_MODEL(ENT(pev), M21_MODEL_WORLD);

    m_iId          = WEAPON_M21;    
    m_iDefaultAmmo = M21_AMMO_CLIP;
    m_InZoom	   = false;
    
    FallInit();// get ready to fall down.
}

void CM21::Precache( void )
{
    PRECACHE_MODEL(M21_MODEL_1STPERSON);
    PRECACHE_MODEL(M21_MODEL_3RDPERSON);
    PRECACHE_MODEL(M21_MODEL_WORLD);
    
    m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL
           
    PRECACHE_SOUND (M21_SOUND_FIRE1);
    PRECACHE_SOUND (M21_SOUND_FIRE2);

    PRECACHE_SOUND (M21_SOUND_CLIPIN);
    PRECACHE_SOUND (M21_SOUND_CLIPOUT);
    
//    m_event   = PRECACHE_EVENT( 1, "events/m21.sc" );    
//    m_event_z = PRECACHE_EVENT( 1, "events/m21zoom.sc" );
}

int CM21::GetItemInfo(ItemInfo *p)
{
    p->pszName   = STRING(pev->classname);
    p->pszAmmo1  = "7_62mm";
    p->iMaxAmmo1 = AMMO_762_MAX;
    p->pszAmmo2  = NULL;
    p->iMaxAmmo2 = -1;
    p->iMaxClip  = M21_AMMO_CLIP;
    p->iSlot     = M21_SLOT;
    p->iPosition = M21_POSITION;
    p->iFlags    = 0;//ITEM_FLAG_NOAUTORELOAD | ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_NOAUTOSWITCHEMPTY;
    p->iId       = m_iId = WEAPON_M21;
    p->iWeight   = M21_WEIGHT;
    
    return 1;
}

int CM21::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CM21::Deploy( )
{
    // FIXME: What should the last parameter be?
    return DefaultDeploy( M21_MODEL_1STPERSON, M21_MODEL_3RDPERSON, M21_DEPLOY, "bow" );
}

void CM21::Holster( int skiplocal /* = 0 */ )
{
    m_fInReload = FALSE;// cancel any reload in progress.
    
    if ( m_InZoom )
    {
	SecondaryAttack( );
    }
    
    m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CM21::PrimaryAttack()
{
#ifdef CLIENT_DLL
    static int clip = 0;
#endif

    if (!(m_pPlayer->m_afButtonPressed & IN_ATTACK))
	return;

    // don't fire underwater
    if (m_pPlayer->pev->waterlevel == 3)
    {
	PlayEmptySound( );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
	return;
    }
    if (m_iClip <= 0)
    {
	PlayEmptySound();
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
	return;
    }

    m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
    
    // Weapon sound
//    m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
//    m_pPlayer->m_iWeaponFlash  = NORMAL_GUN_FLASH;
    
    m_iClip--;
    
#ifdef CLIENT_DLL
    if(clip != m_iClip)
    {
	clip = m_iClip;
#endif    
	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	
	Vector vecSrc(m_pPlayer->GetGunPosition());
	Vector vecAim(m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES ));

	m_pPlayer->FireBullets( 1, vecSrc, vecAim, Accuracy(), 8192, BULLET_PLAYER_M21, 2 );
	
//	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), (m_InZoom ? m_event_z :m_event), 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, (m_iClip ? 0 : 1), 0 );
        
//	Recoil(M21_RECOIL_UP, M21_RECOIL_RIGHT);
#ifdef CLIENT_DLL
    }
#endif

    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + M21_FIRE_DELAY;
    m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat ( m_pPlayer->random_seed, M21_FIRE_DELAY + 1, M21_FIRE_DELAY + 2 );


	// Stuff that should be in the client dll
	
	SendWeaponAnim( M21_FIRE1 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	switch(RANDOM_LONG(0, 1))
	{
	case 0:	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, M21_SOUND_FIRE1, 1, ATTN_NORM); break;
	case 1:	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, M21_SOUND_FIRE2, 1, ATTN_NORM); break;
	}
	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;
	
	Vector	vecShellVelocity = gpGlobals->v_right * 60 + gpGlobals->v_up * 200 + gpGlobals->v_forward * 40;
	Vector	vecShellOrigin = pev->origin + pev->view_ofs + gpGlobals->v_forward * 30 + gpGlobals->v_right * 30;
	EjectBrass ( vecShellOrigin, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 
}

Vector CM21::Accuracy( void )
{
    return (m_InZoom ? g_vecZero : VECTOR_CONE_1DEGREES );
}

void CM21::SecondaryAttack( void )
{
    if ( m_pPlayer->pev->fov != 0 )
    {
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	m_InZoom = false;
    }
    else if ( m_pPlayer->pev->fov != 20 )
    {
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
	m_InZoom = true;
    }

    pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
    m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
}

void CM21::Reload( void )
{
    if ( m_InZoom )
    {
		SecondaryAttack();
    }
    if(DefaultReload( M21_AMMO_CLIP, M21_RELOAD, M21_RELOAD_TIME ))
    {
	//EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/xbow_reload1.wav", UTIL_SharedRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
    }
}

void CM21::WeaponIdle( void )
{
    ResetEmptySound( );
    
    m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
    
    if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
	return;
        
    SendWeaponAnim( M21_IDLE );
    
    m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat ( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
}
