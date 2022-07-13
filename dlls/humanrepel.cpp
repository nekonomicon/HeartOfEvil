//=========================================================
//=========================================================
//
// CHumanRepel class functions
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
#include	"effects.h"
#include	"customentity.h"


//=========================================================
// Spawn
//=========================================================

void CHumanRepel::Spawn( void )
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse( RepelUse );
}

void CHumanRepel::Precache( void )
{
	UTIL_PrecacheOther( EntityName() );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CHumanRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	edict_t	*pent;
	CBaseEntity *pEntity;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( EntityName() ));
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in HumanRepel Create!\n" );
		return;
	}
	pEntity = Instance( pent );
	pEntity->pev->owner = NULL;
	pEntity->pev->origin = pev->origin;
	pEntity->pev->angles = pev->angles;
	pEntity->pev->spawnflags = pev->spawnflags;
	pEntity->pev->body = pev->body;
	pEntity->pev->weapons = pev->weapons;
	pEntity->pev->netname = pev->netname;
	
	DispatchSpawn( pEntity->edict() );

	pEntity->pev->movetype = MOVETYPE_FLY;
	pEntity->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );

	CBaseMonster *pHuman = pEntity->MyMonsterPointer( );
	pHuman->SetActivity( ACT_GLIDE );
	pHuman->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pEntity->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pEntity->pev->velocity.z + 0.5;

	UTIL_Remove( this );
}
