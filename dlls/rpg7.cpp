/**
*   $Log: rpg7.cpp,v $
*   Revision 1.16  2002/03/12 07:04:35  vertigo
*   Merging in Botman's patches so I can compile on gcc 2.95
*
*   Revision 1.15.2.1  2002/03/12 05:54:14  vertigo
*   Incorporated Botman's changes to allow to compile under gcc 2.95
*
*   Revision 1.15  2002/03/05 22:17:03  vertigo
*   Fixed the Net game recoil problem
*
*   Revision 1.14  2002/02/22 19:55:30  sluggo
*   Created a really crappy RPG7 v_model.
*
*   This will likely be redone by cochise at some point. If I can get everything into Max
*   at home, then maybe I can help him.. dunno
*
*   Revision 1.13  2002/02/19 07:08:01  vertigo
*   Created an ammo crate class that give players full ammo and removed the individual ammo clips.
*
*   Revision 1.12  2002/02/19 03:25:53  vertigo
*   All weapons now come with max ammo, so we don't have to give ammo too
*
*   Revision 1.11  2002/02/13 00:20:54  vertigo
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

#include "rpg7.h"

LINK_ENTITY_TO_CLASS( weapon_rpg7, CRpg7 );

#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS( rpg7_rocket, CRpg7Rocket );

//=========================================================
//=========================================================
CRpg7Rocket *CRpg7Rocket::CreateRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner )
{
    CRpg7Rocket *pRocket = GetClassPtr( (CRpg7Rocket *)NULL );
    
    UTIL_SetOrigin( pRocket->pev, vecOrigin );
    pRocket->pev->angles = vecAngles;
    pRocket->Spawn();
    pRocket->SetTouch( &CRpg7Rocket::RocketTouch );
    pRocket->pev->owner = pOwner->edict();
    
    return pRocket;
}

//=========================================================
//=========================================================
void CRpg7Rocket :: Spawn( void )
{
    Precache( );
    // motor
    pev->movetype = MOVETYPE_BOUNCE;
    pev->solid    = SOLID_BBOX;
    
    SET_MODEL(ENT(pev), RPG7_MODEL_ROCKET);
    UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
    UTIL_SetOrigin( pev, pev->origin );
    
    pev->classname = MAKE_STRING("rpg7_rocket");
    
    SetThink( &CRpg7Rocket::IgniteThink );
    SetTouch( &CRpg7Rocket::ExplodeTouch );
    
    pev->angles.x -= 30;
    UTIL_MakeVectors( pev->angles );
    pev->angles.x  = -(pev->angles.x + 30);    
    pev->velocity  = gpGlobals->v_forward * 250;
    pev->gravity   = 0.5;    
    pev->nextthink = gpGlobals->time + 0.4;
    pev->dmg       = /* gSkillData.plrDmgRpg7*/ 100; // PAT: FIXME!!! update skill info!!!
}

//=========================================================
//=========================================================
void CRpg7Rocket :: RocketTouch ( CBaseEntity *pOther )
{    
    STOP_SOUND( edict(), CHAN_VOICE, RPG7_SOUND_ROCKET );
    ExplodeTouch( pOther );
}

//=========================================================
//=========================================================
void CRpg7Rocket :: Precache( void )
{
    PRECACHE_MODEL(RPG7_MODEL_ROCKET);
    m_iTrail = PRECACHE_MODEL(RPG7_SMOKE_TRAIL);
    PRECACHE_SOUND (RPG7_SOUND_ROCKET);
}


void CRpg7Rocket :: IgniteThink( void  )
{
    pev->movetype = MOVETYPE_FLY;
    pev->effects |= EF_LIGHT;
    
    // make rocket sound
    EMIT_SOUND( ENT(pev), CHAN_VOICE, RPG7_SOUND_ROCKET, 1, 0.5 );
    
    // rocket trail
    MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );    
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT(entindex());	// entity
	WRITE_SHORT(m_iTrail );		// model
	WRITE_BYTE( 40 );		// life
	WRITE_BYTE( 5 );		// width
	WRITE_BYTE( 224 );		// r, g, b
	WRITE_BYTE( 224 );		// r, g, b
	WRITE_BYTE( 255 );		// r, g, b
	WRITE_BYTE( 255 );		// brightness    
    MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
    
    m_flIgniteTime = gpGlobals->time;
    
    // set to follow laser spot
    SetThink( &CRpg7Rocket::FlightThink );
    pev->nextthink = gpGlobals->time + 0.1;
}


void CRpg7Rocket :: FlightThink( void  )
{
	Vector vecTarget;

	UTIL_MakeAimVectors( pev->angles );
	vecTarget = gpGlobals->v_forward;
	
	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0)
	{
		pev->velocity = pev->velocity * 0.2 + vecTarget * (flSpeed * 0.8 + 800);
		if (pev->waterlevel == 3)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 4 );
		} 
		else 
		{
			if (pev->velocity.Length() > 2000)
			{
				pev->velocity = pev->velocity.Normalize() * 2000;
			}
		}
	}
	else
	{
		if (pev->effects & EF_LIGHT)
		{
			pev->effects = 0;
			STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav" );
		}
		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;
		if ((pev->waterlevel == 0 || pev->watertype == CONTENT_FOG) && pev->velocity.Length() < 1500)
		{
			Detonate( );
		}
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

    pev->nextthink = gpGlobals->time + 0.1;
}
#endif



void CRpg7::Reload( void )
{
    // Only reload if there is no ammo
    if ( m_iClip == 0 && DefaultReload( 1, RPG7_RELOAD, RPG7_RELOAD_TIME )) 
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
    }    
}

void CRpg7::Spawn( )
{
    Precache( );
    m_iId = WEAPON_RPG7;    
    SET_MODEL(ENT(pev), RPG7_MODEL_WORLD);    
    m_iDefaultAmmo = RPG7_AMMO_CLIP;    
    FallInit();// get ready to fall down.
}


void CRpg7::Precache( void )
{
    PRECACHE_MODEL(RPG7_MODEL_WORLD);
    PRECACHE_MODEL(RPG7_MODEL_1STPERSON);
    PRECACHE_MODEL(RPG7_MODEL_3RDPERSON);
    
    PRECACHE_SOUND("items/9mmclip1.wav");
    PRECACHE_SOUND("weapons/rpg_load.wav");
    
    UTIL_PrecacheOther( "rpg7_rocket" );
    
    PRECACHE_SOUND(RPG7_SOUND_FIRE1);
    
//    m_usRpg = PRECACHE_EVENT ( 1, "events/rpg7.sc" );
}


int CRpg7::GetItemInfo(ItemInfo *p)
{
    p->pszName     = STRING(pev->classname);
    p->pszAmmo1    = "rockets";
    p->iMaxAmmo1   = RPG7_MAX_AMMO;
    p->pszAmmo2    = NULL;
    p->iMaxAmmo2   = -1;
    p->iMaxClip    = RPG7_AMMO_CLIP;
    p->iSlot       = RPG7_SLOT;
    p->iPosition   = RPG7_POSITION;
    p->iId = m_iId = WEAPON_RPG7;
    p->iFlags      = 0;//ITEM_FLAG_NOAUTORELOAD  | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_NOAUTOSWITCHEMPTY;;
    p->iWeight     = RPG7_WEIGHT;
    
    return 1;
}

int CRpg7::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CRpg7::Deploy( )
{
//    if ( m_iClip == 0 )
//    {
//	return DefaultDeploy( RPG7_MODEL_1STPERSON, RPG7_MODEL_3RDPERSON, RPG7_DRAW_UL, "rpg", 1, pev->body );
//    }
    // PAT: just regular deploy for now FIXME!!
    return DefaultDeploy( RPG7_MODEL_1STPERSON, RPG7_MODEL_3RDPERSON, RPG7_DEPLOY, "rpg" );
}

void CRpg7::Holster( int skiplocal /* = 0 */ )
{
    m_fInReload = FALSE;    // cancel any reload in progress.
    
    m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
    
    // PAT: FIXME! No holster
    SendWeaponAnim( RPG7_IDLE );
}

void CRpg7::PrimaryAttack()
{
    // don't fire underwater
    if (m_pPlayer->pev->waterlevel == 3)
    {
	PlayEmptySound( );
	m_flNextPrimaryAttack = 0.15;
	return;
    }
    
    if (m_iClip <= 0)
    {
	PlayEmptySound();
	m_flNextPrimaryAttack = 0.15;
	return;
    }
    
    m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
    m_pPlayer->m_iWeaponFlash  = BRIGHT_GUN_FLASH;
    
    // player "shoot" animation
    m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	
	if ( RANDOM_LONG( 0, 1 ) )
	{
		SendWeaponAnim( RPG7_FIRE1 );
	}
	else
	{
		SendWeaponAnim( RPG7_FIRE2 );
	}

    UTIL_MakeVectors( m_pPlayer->pev->v_angle );
    Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 8 + gpGlobals->v_right * 4 + gpGlobals->v_up * -8;
    
    CRpg7Rocket *pRocket = CRpg7Rocket::CreateRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer );
    
    UTIL_MakeVectors( m_pPlayer->pev->v_angle );  // LawRocket::Create stomps on globals, so remake.
    pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
    
    m_iClip--; 
    
    m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5;
    m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 1.5;

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, RPG7_SOUND_FIRE1, 1, ATTN_NORM);

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;

	m_pPlayer->pev->punchangle.x -= 20.0;
	if ( FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
	{
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 200;
	}
}

void CRpg7::WeaponIdle( void )
{    
    ResetEmptySound( );
    
    if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
	return;
    
	int iAnim;

    if ( m_iClip == 0 )
	{
		iAnim = RPG7_EMPTY;
	}
	else if ( RANDOM_LONG( 0, 1 ) )
	{
		iAnim = RPG7_IDLE;
	}
	else
	{
		iAnim = RPG7_IDLE_LONG;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 120.0 / 30.0;
	SendWeaponAnim( iAnim );
}



class CRpgAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_rpgammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_rpgammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int iGive;

		if ( g_pGameRules->IsMultiplayer() )
		{
			// hand out more ammo per rocket in multiplayer.
			iGive = AMMO_RPGCLIP_GIVE * 2;
		}
		else
		{
			iGive = AMMO_RPGCLIP_GIVE;
		}

		if (pOther->GiveAmmo( iGive, "rockets", RPG7_MAX_AMMO ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_rpgclip, CRpgAmmo );

