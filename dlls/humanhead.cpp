//=========================================================
//=========================================================
//
// CHumanHead class functions
//
//=========================================================
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"human.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"soundent.h"
#include	"animation.h"
#include	"weapons.h"


//=========================================================
// Spawn
//=========================================================

void CHumanHead :: Spawn( const char *szGibModel, int nBodyNum )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55; // deading the bounce a bit
	
	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
    pev->effects        = 0;
	pev->solid = SOLID_NOT;
	pev->classname = MAKE_STRING("humanhead");
	m_bloodColor = BLOOD_COLOR_RED;
	pev->takedamage	= DAMAGE_YES;
	pev->max_health = 10;
	pev->health = 10;

	SET_MODEL(ENT(pev), szGibModel);
	UTIL_SetSize(pev, Vector( -5, -5, -5), Vector(5, 5, 5));

	SetBodygroup( 0, nBodyNum );
	SetTouch ( BounceGibTouch );
	SetThink( SolidThink );
	SetUse( BounceUse );
	pev->nextthink = gpGlobals->time + 0.1;
}


//=========================================================
// Try and become solid once we are clear of GIs body
//=========================================================

void CHumanHead :: SolidThink( void )
{
	// If stuck in something just kill it off quick and hope no one notices

	TraceResult	tr;
	UTIL_TraceHull( pev->origin, pev->origin, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if ( tr.pHit )
	{
		if ( pev->velocity == g_vecZero || ( pev->flags & FL_ONGROUND ) )
		{
			Killed( NULL, 0 );
		}
		else
		{
			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
	else
	{
		pev->solid = SOLID_SLIDEBOX;
		SetThink( NULL );
	}
}


//=========================================================
// Touch
//=========================================================

void CHumanHead :: BounceGibTouch ( CBaseEntity *pOther )
{
	if (pev->flags & FL_ONGROUND)
	{
		pev->velocity = pev->velocity * 0.9;
		pev->avelocity.x = pev->avelocity.x * 0.9;
		pev->avelocity.z = pev->avelocity.z * 0.9;
	}
	else
	{
		Vector	vecSpot;
		TraceResult	tr;
	
		vecSpot = pev->origin + Vector ( 0 , 0 , 8 );//move up a bit, and trace down.
		UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -24 ),  ignore_monsters, ENT(pev), & tr);
		UTIL_BloodDecalTrace( &tr, m_bloodColor );
	}

	if ( pOther->pev->velocity != g_vecZero )
	{
		// Is entity standing on this head ?

		if ( FBitSet(pOther->pev->flags,FL_ONGROUND) && pOther->pev->groundentity && VARS(pOther->pev->groundentity) == pev )
		{
			pev->solid = SOLID_NOT;
			SetThink( SolidThink );
			pev->nextthink = gpGlobals->time + 0.1;
		}

		// Adopt twice speed of toucher

		pev->velocity.x += pOther->pev->velocity.x * 2;
		pev->velocity.y += pOther->pev->velocity.y * 2;

		// We only want the head to be kicked *upwards* by the toucher
		
		if ( pOther->pev->velocity.z > 0 ) pev->velocity.z += pOther->pev->velocity.z;
		pev->velocity.z += RANDOM_LONG( 200, 300 );

		// Spin a bit

		pev->avelocity = Vector( RANDOM_LONG(0, 200), RANDOM_LONG(0, 200), RANDOM_LONG(0, 200));

		// Go "splatch"

		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0: EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/machete_hitbod1.wav", 0.5, ATTN_NORM);
		case 1: EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/machete_hitbod2.wav", 0.5, ATTN_NORM);
		case 2: EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/machete_hitbod3.wav", 0.5, ATTN_NORM);
		}
	}
}


void CHumanHead :: BounceUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Is entity standing on this head ?

	if ( FBitSet(pActivator->pev->flags,FL_ONGROUND) && pActivator->pev->groundentity && 
		VARS(pActivator->pev->groundentity) == pev )
	{
		return;
	}
		
	// whack

	pev->velocity.z += RANDOM_LONG( 500, 600 );

	// Spin a bit

	pev->avelocity = Vector( RANDOM_LONG(0, 200), RANDOM_LONG(0, 200), RANDOM_LONG(0, 200));

	// Go "splatch"

	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/machete_hitbod1.wav", 0.5, ATTN_NORM);
	case 1: EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/machete_hitbod2.wav", 0.5, ATTN_NORM);
	case 2: EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/machete_hitbod3.wav", 0.5, ATTN_NORM);
	}

}


void CHumanHead::Killed( entvars_t *pevAttacker, int iGib )
{
	CGib::SpawnHeadGib( pev );
	SetThink ( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}