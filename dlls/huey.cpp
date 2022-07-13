#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "soundent.h"
#include "customentity.h"
#include "saverestore.h"
#include "huey.h"

LINK_ENTITY_TO_CLASS( monster_huey, CHuey );

TYPEDESCRIPTION	CHuey::m_SaveData[] = 
{
	DEFINE_FIELD( CHuey, m_iRockets, FIELD_INTEGER ),
	DEFINE_FIELD( CHuey, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CHuey, m_flNextSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( CHuey, m_flNextRocket, FIELD_TIME ),
	DEFINE_FIELD( CHuey, m_flNextGrenade, FIELD_TIME ),
	DEFINE_FIELD( CHuey, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CHuey, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CHuey, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CHuey, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CHuey, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CHuey, m_angGun, FIELD_VECTOR ),
	DEFINE_FIELD( CHuey, m_angMiniGuns, FIELD_FLOAT ),
	DEFINE_FIELD( CHuey, m_angGren, FIELD_FLOAT ),
	DEFINE_FIELD( CHuey, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CHuey, m_flPrevSeen, FIELD_TIME ),
//	DEFINE_FIELD( CHuey, m_iSoundState, FIELD_INTEGER ),		// Don't save, precached
//	DEFINE_FIELD( CHuey, m_iSpriteTexture, FIELD_INTEGER ),
//	DEFINE_FIELD( CHuey, m_iExplode, FIELD_INTEGER ),
//	DEFINE_FIELD( CHuey, m_iBodyGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CHuey, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CHuey, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CHuey, m_iDoSmokePuff, FIELD_INTEGER ),
	DEFINE_FIELD( CHuey, m_iUnits, FIELD_INTEGER ),
	DEFINE_ARRAY( CHuey, m_hGrunt, FIELD_EHANDLE, MAX_CARRY ),
	DEFINE_ARRAY( CHuey, m_vecOrigin, FIELD_POSITION_VECTOR, MAX_CARRY ),
	DEFINE_ARRAY( CHuey, m_hRepel, FIELD_EHANDLE, 4 ),
	DEFINE_FIELD( CHuey, m_iMaxGrunts, FIELD_INTEGER ),
	DEFINE_FIELD( CHuey, m_iPassenger, FIELD_INTEGER ),
	DEFINE_FIELD( CHuey, m_flPassengerHealth, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CHuey, CBaseMonster );


void CHuey :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/huey.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->health			= gSkillData.hueyHealth;

	m_flFieldOfView = -0.707; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo( );
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	if (pev->spawnflags & SF_PASSENGER)
	{
		if (m_iPassenger < HUEY_PASSENGER_MIKE)
		{
			SetBodygroup(BODY_PASSENGER, HUEY_PBODY_GRUNT);
			m_flPassengerHealth = gSkillData.hgruntHealth;
		}
		else
		{
			SetBodygroup(BODY_PASSENGER, HUEY_PBODY_MIKE);
			m_flPassengerHealth = gSkillData.mikeforceHealth;
		}
	}
	else
	{
		SetBodygroup(BODY_PASSENGER, HUEY_PBODY_NONE);
		m_flPassengerHealth = 0;
	}

	
	if (pev->spawnflags & SF_GRENADE) SetBodygroup(BODY_GRENADE, TRUE);

//	SetBoneController( 0, 30 );
//	SetBoneController( 1, 30 );

	if (pev->spawnflags & SF_WAITFORTRIGGER)
	{
		SetUse( StartupUse );
	}
	else
	{
		SetThink( FindAllThink );
		SetTouch( FlyTouch );
		pev->nextthink = gpGlobals->time + 1.0;
	}

	m_iRockets = 10;
	m_flNextSoundTime = gpGlobals->time;
}


void CHuey::Precache( void )
{
	if (m_iPassenger < HUEY_PASSENGER_MIKE)
	{
		UTIL_PrecacheOther( "monster_human_grunt" );
		UTIL_PrecacheOther( "monster_human_grunt_medic" );
	}
	else
	{
		UTIL_PrecacheOther( "monster_mikeforce" );
		UTIL_PrecacheOther( "monster_mikeforce_medic" );
	}

	PRECACHE_MODEL("models/huey.mdl");

	PRECACHE_SOUND("huey/huey_rotor1.wav");

	PRECACHE_SOUND("weapons/mortarhit.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );
	m_iRopeTexture = PRECACHE_MODEL( "sprites/rope.spr" );

	PRECACHE_SOUND("huey/huey_fire1.wav");
	PRECACHE_SOUND("huey/huey_fire2.wav");

	PRECACHE_SOUND("weapons/m60_fire.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/steam1.spr");
	PRECACHE_MODEL("sprites/muzzleflash.spr");

	m_iExplode	= PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/metalplategibs_green.mdl" );

	UTIL_PrecacheOther( "hvr_rocket" );
}



void CHuey::NullThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.5;
}


void CHuey::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( HuntThink );
	SetTouch( FlyTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse( NULL );
}

void CHuey :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;

	STOP_SOUND( ENT(pev), CHAN_STATIC, "huey/huey_rotor1.wav" );

	UTIL_SetSize( pev, Vector( -32, -32, -64), Vector( 32, 32, 0) );
	SetThink( DyingThink );
	SetTouch( CrashTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	m_flNextRocket = gpGlobals->time + 15.0;
}

void CHuey :: DyingThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->avelocity = pev->avelocity * 1.02;

	// still falling?
	if (m_flNextRocket > gpGlobals->time )
	{
		// random explosions
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( RANDOM_LONG(0,29) + 30  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		// lots of smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 100 ); // scale * 10
			WRITE_BYTE( 10  ); // framerate
		MESSAGE_END();

		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 132 );

			// velocity
			WRITE_COORD( pev->velocity.x ); 
			WRITE_COORD( pev->velocity.y );
			WRITE_COORD( pev->velocity.z );

			// randomization
			WRITE_BYTE( 50 ); 

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 4 );	// let client decide

			// duration
			WRITE_BYTE( 30 );// 3.0 seconds

			// flags

			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	else
	{
		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

		/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 300 );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 8  ); // framerate
		MESSAGE_END();
		*/

		// fireball
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 256 );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 120 ); // scale * 10
			WRITE_BYTE( 255 ); // brightness
		MESSAGE_END();
		
		// big smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 512 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 250 ); // scale * 10
			WRITE_BYTE( 5  ); // framerate
		MESSAGE_END();

		// blast circle
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 2000 ); // reach damage radius over .2 seconds
			WRITE_SHORT( m_iSpriteTexture );
			WRITE_BYTE( 0 ); // startframe
			WRITE_BYTE( 0 ); // framerate
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 32 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 192 );   // r, g, b
			WRITE_BYTE( 128 ); // brightness
			WRITE_BYTE( 0 );		// speed
		MESSAGE_END();

		EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

		RadiusDamage( pev->origin, pev, pev, 300, CLASS_NONE, DMG_BLAST );

		if (/*!(pev->spawnflags & SF_NOWRECKAGE) && */(pev->flags & FL_ONGROUND))
		{
			CBaseEntity *pWreckage = Create( "cycler_wreckage", pev->origin, pev->angles );
			// SET_MODEL( ENT(pWreckage->pev), STRING(pev->model) );
			UTIL_SetSize( pWreckage->pev, Vector( -200, -200, -128 ), Vector( 200, 200, -32 ) );
			pWreckage->pev->frame = pev->frame;
			pWreckage->pev->sequence = pev->sequence;
			pWreckage->pev->framerate = 0;
			pWreckage->pev->dmgtime = gpGlobals->time + 5;
		}

		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 64);

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 128 );

			// velocity
			WRITE_COORD( 0 ); 
			WRITE_COORD( 0 );
			WRITE_COORD( 200 );

			// randomization
			WRITE_BYTE( 30 ); 

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 200 );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags

			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CHuey::FlyTouch( CBaseEntity *pOther )
{
	// bounce if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP) 
	{
		TraceResult tr = UTIL_GetGlobalTrace( );

		// UNDONE, do a real bounce
		pev->velocity = pev->velocity + tr.vecPlaneNormal * (pev->velocity.Length() + 200);
	}
}


void CHuey::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP) 
	{
		SetTouch( NULL );
		m_flNextRocket = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
	}
}



void CHuey :: GibMonster( void )
{
	// EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);		
}


void CHuey :: HuntThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	ShowDamage( );

	if ( m_pGoalEnt == NULL && !FStringNull(pev->target) )// this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if (m_pGoalEnt)
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	// if (m_hEnemy == NULL)
	{
		Look( max( HUEY_GUN_RANGE, HUEY_ROCKET_RANGE ) );
		m_hEnemy = BestVisibleEnemy( );
	}

	// generic speed up
	if (m_flGoalSpeed < 800)
		m_flGoalSpeed += 5;

	if (m_hEnemy != NULL)
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if (FVisible( m_hEnemy ))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center( );
		}
		else
		{
			m_hEnemy = NULL;
		}
	}

	m_vecTarget = (m_posTarget - pev->origin).Normalize();

	float flLength = (pev->origin - m_posDesired).Length();

	if (m_pGoalEnt)
	{
		// ALERT( at_console, "%.0f\n", flLength );

		if (flLength < 128)
		{
			if ( m_pGoalEnt->pev->speed == 0 && HasDead() )
			{
				if ( pev->velocity.Length() <= 10 )
				{
					SetThink( DeployThink );
					m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
					if (m_pGoalEnt)
					{
						m_posDesired = m_pGoalEnt->pev->origin;
						UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
						m_vecGoal = gpGlobals->v_forward;
						flLength = (pev->origin - m_posDesired).Length();
					}
				}
				else m_flGoalSpeed = 0;
			}
			else
			{
				m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
				if (m_pGoalEnt)
				{
					m_posDesired = m_pGoalEnt->pev->origin;
					UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
					m_vecGoal = gpGlobals->v_forward;
					flLength = (pev->origin - m_posDesired).Length();
				}
			}
		}
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if (flLength > 250) // 500
	{
		// float flLength2 = (m_posTarget - pev->origin).Length() * (1.5 - DotProduct((m_posTarget - pev->origin).Normalize(), pev->velocity.Normalize() ));
		// if (flLength2 < flLength)
		if (m_flLastSeen + 90 > gpGlobals->time && DotProduct( (m_posTarget - pev->origin).Normalize(), (m_posDesired - pev->origin).Normalize( )) > 0.25)
		{
			m_vecDesired = (m_posTarget - pev->origin).Normalize( );
		}
		else
		{
			m_vecDesired = (m_posDesired - pev->origin).Normalize( );
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight( );

	UTIL_MakeAimVectors( pev->angles );
	Vector vecEst = (gpGlobals->v_forward * 800 + pev->velocity).Normalize( );
	
	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if ((m_flLastSeen + 1 > gpGlobals->time) && (m_flPrevSeen + 2 < gpGlobals->time))
	{
		
		if (FireGun( FALSE ))
		{
			// slow down if we're fireing
			if (m_flGoalSpeed > 400)
				m_flGoalSpeed = 400;
		}
		
		// don't fire rockets and gun on easy mode
		if (g_iSkillLevel == SKILL_EASY)
			m_flNextRocket = gpGlobals->time + 10.0;
	}

	//UTIL_MakeAimVectors( pev->angles );	<---moved upwards
	//Vector vecEst = (gpGlobals->v_forward * 800 + pev->velocity).Normalize( );   <---moved upwards
	// ALERT( at_console, "%d %d %d %4.2f\n", pev->angles.x < 0, DotProduct( pev->velocity, gpGlobals->v_forward ) > -100, m_flNextRocket < gpGlobals->time, DotProduct( m_vecTarget, vecEst ) );

	if ( pev->spawnflags & SF_ROCKET )
	{
		if ((m_iRockets % 2) == 1)
		{
			FireRocket( );
			m_flNextRocket = gpGlobals->time + 0.5;
			if (m_iRockets <= 0)
			{
				m_flNextRocket = gpGlobals->time + 10;
				m_iRockets = 10;
			}
		}
		else if (pev->angles.x < 0 && DotProduct( pev->velocity, gpGlobals->v_forward ) > -100 && m_flNextRocket < gpGlobals->time)
		{
			if (m_flLastSeen + 60 > gpGlobals->time)
			{
				if (m_hEnemy != NULL)
				{
					// make sure it's a good shot
					if (DotProduct( m_vecTarget, vecEst) > .965)
					{
						TraceResult tr;
					
						UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, ignore_monsters, edict(), &tr );
						if ((tr.vecEndPos - m_posTarget).Length() < 512)
							FireRocket( );
					}
				}
				else
				{
					TraceResult tr;
				
					UTIL_TraceLine( pev->origin, pev->origin + vecEst * 4096, dont_ignore_monsters, edict(), &tr );
					// just fire when close
					if ((tr.vecEndPos - m_posTarget).Length() < 512)
						FireRocket( );
				}
			}
		}
	}

	FCheckAITrigger();
}


void CHuey :: Flight( void )
{

	// tilt model 5 degrees
	Vector vecAdj = Vector( 5.0, 0, 0 );

	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 2 + vecAdj);
	// Vector vecEst1 = pev->origin + pev->velocity + gpGlobals->v_up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );
	
	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );

	if (flSide < 0)
	{
		if (pev->avelocity.y < 60)
		{
			pev->avelocity.y += 8; // 9 * (3.0/2.0);
		}
	}
	else
	{
		if (pev->avelocity.y > -60)
		{
			pev->avelocity.y -= 8; // 9 * (3.0/2.0);
		}
	}
	pev->avelocity.y *= 0.98;

	// estimate where I'll be in two seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 1 + vecAdj);
	Vector vecEst = pev->origin + pev->velocity * 2.0 + gpGlobals->v_up * m_flForce * 20 - Vector( 0, 0, 384 * 2 );

	// add immediate force
	UTIL_MakeAimVectors( pev->angles + vecAdj);
	pev->velocity.x += gpGlobals->v_up.x * m_flForce;
	pev->velocity.y += gpGlobals->v_up.y * m_flForce;
	pev->velocity.z += gpGlobals->v_up.z * m_flForce;
	// add gravity
	pev->velocity.z -= 38.4; // 32ft/sec


	float flSpeed = pev->velocity.Length();
	float flDir = DotProduct( Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0 ), Vector( pev->velocity.x, pev->velocity.y, 0 ) );
	if (flDir < 0)
		flSpeed = -flSpeed;

	float flDist = DotProduct( m_posDesired - vecEst, gpGlobals->v_forward );

	// float flSlip = DotProduct( pev->velocity, gpGlobals->v_right );
	float flSlip = -DotProduct( m_posDesired - vecEst, gpGlobals->v_right );

	// fly sideways
	if (flSlip > 0)
	{
		if (pev->angles.z > -30 && pev->avelocity.z > -15)
			pev->avelocity.z -= 4;
		else
			pev->avelocity.z += 2;
	}
	else
	{

		if (pev->angles.z < 30 && pev->avelocity.z < 15)
			pev->avelocity.z += 4;
		else
			pev->avelocity.z -= 2;
	}

	// sideways drag
	pev->velocity.x = pev->velocity.x * (1.0 - fabs( gpGlobals->v_right.x ) * 0.05);
	pev->velocity.y = pev->velocity.y * (1.0 - fabs( gpGlobals->v_right.y ) * 0.05);
	pev->velocity.z = pev->velocity.z * (1.0 - fabs( gpGlobals->v_right.z ) * 0.05);

	// general drag
	pev->velocity = pev->velocity * 0.995;
	
	// apply power to stay correct height
	if (m_flForce < 80 && vecEst.z < m_posDesired.z) 
	{
		m_flForce += 12;
	}
	else if (m_flForce > 30)
	{
		if (vecEst.z > m_posDesired.z) 
			m_flForce -= 8;
	}

	// pitch forward or back to get to target
	if (flDist > 0 && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && pev->angles.x + pev->avelocity.x > -40)
	{
		// ALERT( at_console, "F " );
		// lean forward
		pev->avelocity.x -= 12.0;
	}
	else if (flDist < 0 && flSpeed > -50 && pev->angles.x + pev->avelocity.x  < 20)
	{
		// ALERT( at_console, "B " );
		// lean backward
		pev->avelocity.x += 12.0;
	}
	else if (pev->angles.x + pev->avelocity.x > 0)
	{
		// ALERT( at_console, "f " );
		pev->avelocity.x -= 4.0;
	}
	else if (pev->angles.x + pev->avelocity.x < 0)
	{
		// ALERT( at_console, "b " );
		pev->avelocity.x += 4.0;
	}

	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", pev->origin.x, pev->velocity.x, flDist, flSpeed, pev->angles.x, pev->avelocity.x, m_flForce ); 
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", pev->origin.z, pev->velocity.z, vecEst.z, m_posDesired.z, m_flForce ); 

	MakeSound();
}


void CHuey :: FireRocket( void )
{
	if ((m_hEnemy!=NULL) && (m_hEnemy->pev->spawnflags & SF_MONSTER_WAIT_TILL_SEEN)) return;
	
	if ((m_hEnemy!=NULL) && ( m_hEnemy->pev->origin - pev->origin).Length() > HUEY_ROCKET_RANGE ) return;

	static float side = 1.0;
	static int count;

	if (m_iRockets <= 0)
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + 1.5 * (gpGlobals->v_forward * 21 + gpGlobals->v_right * 70 * side + gpGlobals->v_up * -79);

	switch( m_iRockets % 5)
	{
	case 0:	vecSrc = vecSrc + gpGlobals->v_right * 10; break;
	case 1: vecSrc = vecSrc - gpGlobals->v_right * 10; break;
	case 2: vecSrc = vecSrc + gpGlobals->v_up * 10; break;
	case 3: vecSrc = vecSrc - gpGlobals->v_up * 10; break;
	case 4: break;
	}

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 20 ); // scale * 10
		WRITE_BYTE( 12 ); // framerate
	MESSAGE_END();

	CBaseEntity *pRocket = CBaseEntity::Create( "hvr_rocket", vecSrc, pev->angles, edict() );
	if (pRocket)
		pRocket->pev->velocity = pev->velocity + gpGlobals->v_forward * 100;

	m_iRockets--;

	side = - side;
}


BOOL CHuey :: FireGun( BOOL OnlyGrunt )
{
	if ((m_hEnemy!=NULL) && (m_hEnemy->pev->spawnflags & SF_MONSTER_WAIT_TILL_SEEN)) return FALSE;

	if ((m_hEnemy!=NULL) && ( m_hEnemy->pev->origin - pev->origin).Length() > HUEY_GUN_RANGE ) return FALSE;

	UTIL_MakeAimVectors( pev->angles );
		
	//Find the position of the gun	
	Vector posGun, angGun;
	GetAttachment(2, posGun, angGun );

	//Find a vector from the gun to the target
	Vector vecTarget = (m_posTarget - posGun).Normalize( );

	Vector vecOut;

	//Convert this vector into angles with respect to helicopter
	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles (vecOut);

	//Make the angles from -180 to 180 instead of 0 to 360
	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	//Move the grunt's gun towards the target
	if (angles.x > m_angGun.x)
		m_angGun.x = min( angles.x, m_angGun.x + 12 );
	if (angles.x < m_angGun.x)
		m_angGun.x = max( angles.x, m_angGun.x - 12 );
	if (angles.y > m_angGun.y)
		m_angGun.y = min( angles.y, m_angGun.y + 12 );
	if (angles.y < m_angGun.y)
		m_angGun.y = max( angles.y, m_angGun.y - 12 );

	m_angGun.y = SetBoneController( 1, m_angGun.y );
	m_angGun.x = SetBoneController( 2, m_angGun.x );


	angles.x = -angles.x;
	
	//Move the miniguns towards the target
	if (angles.x > m_angMiniGuns)
		m_angMiniGuns = min( angles.x, m_angMiniGuns + 12 );
	if (angles.x < m_angMiniGuns)
		m_angMiniGuns = max( angles.x, m_angMiniGuns - 12 );
	
	m_angMiniGuns = SetBoneController( 0, m_angMiniGuns );

	//Move the grenade launcher towards the target
	if (angles.x > m_angGren)
		m_angGren = min( angles.x, m_angGren + 12 );
	if (angles.x < m_angGren)
		m_angGren = max( angles.x, m_angGren - 12 );

	m_angGren = SetBoneController( 3, m_angGren );


	Vector vecGun, posBarrel, angBarrel;
	
	if ( pev->spawnflags & SF_PASSENGER )
	{
		// Fire the Grunt's gun
		GetAttachment( 3, posBarrel, angBarrel );
		vecGun = (posBarrel - posGun).Normalize();

		if ((DotProduct( vecGun, vecTarget ) > 0.98) && (m_angGun.y < -30 || m_angGun.y > 30))
		{
			FireBullets( 1, posGun, vecGun, VECTOR_CONE_8DEGREES, 8192, BULLET_MONSTER_762, 1 );
		
			switch  ( RANDOM_LONG(0,2) )
			{
			case 0:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/m60_fire.wav", 1, 0.3);
			}
		}
	}
	

	BOOL retval = FALSE;

	if ( !OnlyGrunt )
	{
		// Fire the Miniguns

		// Left minigun
		posGun = pev->origin + gpGlobals->v_right * -90 + gpGlobals->v_up * -128;
		GetAttachment(0, posBarrel, angBarrel);
		vecGun = (posBarrel - posGun).Normalize();

		if (DotProduct( vecGun, vecTarget ) > 0.7) 
		{
			FireBullets( 1, posGun, vecGun, VECTOR_CONE_8DEGREES, 8192, BULLET_MONSTER_12MM, 1 );
			
			// Right minigun - probably more or less the same
			FireBullets( 1, posGun + gpGlobals->v_right * 178, vecGun, VECTOR_CONE_8DEGREES, 8192, BULLET_MONSTER_12MM, 1 );
		
			if (RANDOM_LONG(0, 1))
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "huey/huey_fire1.wav", 1, 0.3);
			else
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "huey/huey_fire2.wav", 1, 0.3);

			retval = TRUE;
		}
	}

	if ( !OnlyGrunt && pev->spawnflags & SF_GRENADE )
	{
		// Fire the grenade launcher
		if (m_flNextGrenade <= gpGlobals->time)
		{
			posGun = pev->origin + gpGlobals->v_forward * 168 + gpGlobals->v_up * -115;
			GetAttachment(1, posBarrel, angBarrel);
			vecGun = (posBarrel - posGun).Normalize();

			if (DotProduct( vecGun, vecTarget ) > 0.96) 
			{
				CGrenade::ShootContact( pev, posGun, vecGun * 800 );

				m_flNextGrenade = gpGlobals->time + 0.3;	// The M75 had an rof of 215-230 rpm, apparently
			}
		}
	}

	return retval;

}



void CHuey :: ShowDamage( void )
{
	if (m_iDoSmokePuff > 0 || RANDOM_LONG(0,99) > pev->health)
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 32 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( RANDOM_LONG(0,9) + 20 ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}
	if (m_iDoSmokePuff > 0)
		m_iDoSmokePuff--;
}


int CHuey :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (pevInflictor->owner == edict())
		return 0;

	/*if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2;
	}*/

	/*
	if ( (bitsDamageType & DMG_BULLET) && flDamage > 50)
	{
		// clip bullet damage at 50
		flDamage = 50;
	}
	*/

	// ALERT( at_console, "%.0f\n", flDamage );
	return CBaseEntity::TakeDamage(  pevInflictor, pevAttacker, flDamage, bitsDamageType );
}



void CHuey::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

	// ignore blades
//	if (ptr->iHitgroup == 6 && (bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB|DMG_SLASH)))
//		return;

	BOOL hitPassenger = FALSE;

	if ( ptr->iHitgroup == 1 )
	{
		// If gunner is hit
		if ( FBitSet( pev->spawnflags, SF_PASSENGER ) )
		{
			// Spawn blood
			m_flPassengerHealth -= flDamage;
			SpawnBlood(ptr->vecEndPos, BLOOD_COLOR_RED, flDamage);

			hitPassenger = TRUE;

			// If gunner is dead
			if ( m_flPassengerHealth <= 0 )
			{
				// Get rid of gunner
				pev->spawnflags &= !SF_PASSENGER;
				SetBodygroup(BODY_PASSENGER, HUEY_PBODY_NONE);
				m_flPassengerHealth = 0;

				//Find the position of the gun(ner)
				Vector posGun, angGun;
				GetAttachment(2, posGun, angGun );

				// Create a falling dead body
				CBaseEntity * pEntity;
				if (m_iPassenger < HUEY_PASSENGER_MIKE )
				{
					pEntity = Create( "monster_mikeforce_dead", posGun, angGun );
				}
				else
				{
					pEntity = Create( "monster_hgrunt_dead", posGun, angGun );
				}
				if ( pEntity )
				{
					pEntity->pev->movetype = MOVETYPE_TOSS;
					pEntity->pev->velocity = vecDir * 100;
				}

				// Create a falling M60
				CBaseEntity *pGun;
				pGun = DropItem( "weapon_m60", posGun, angGun );
				if ( pGun )
				{
					pGun->pev->velocity = Vector (RANDOM_FLOAT(-200,200), RANDOM_FLOAT(-200,200), RANDOM_FLOAT(200,300));
					pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
				}
			}

		}
	}
	
		
	// hit hard, don't count bullets
	if (flDamage > 50 && !(bitsDamageType & (DMG_BULLET|DMG_CLUB|DMG_SLASH|DMG_POISON)) )
	{
		// ALERT( at_console, "%.0f\n", flDamage );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		m_iDoSmokePuff = 3 + (flDamage / 5.0);
	}
	else if (!hitPassenger)
	{
		// do half damage in the body
		// AddMultiDamage( pevAttacker, this, flDamage / 2.0, bitsDamageType );
		UTIL_Ricochet( ptr->vecEndPos, 2.0 );
	}
}


void CHuey::HoverThink()
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	UTIL_MakeAimVectors( pev->angles );

	pev->avelocity = Vector(0, 0, 0);
	pev->velocity = Vector(0, 0, 0);

	int i;
	for (i = 0; i < m_iMaxGrunts; i++)
	{
		if (m_hRepel[i] != NULL && m_hRepel[i]->pev->health > 0 && !(m_hRepel[i]->pev->flags & FL_ONGROUND))
		{
			break;
		}
	}

	if (i == m_iMaxGrunts)
	{
		SetThink( HuntThink );
	}

	FireGun( TRUE );
	ShowDamage();
	MakeSound();
}


void CHuey::DeployThink()
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	UTIL_MakeAimVectors( pev->angles );

	pev->avelocity = Vector(0, 0, 0);
	pev->velocity = Vector(0, 0, 0);

	if ( pev->angles.x != 0.0 )
	{
		// Hack to make it level when dropping troops
		
		if ( pev->angles.x > 1.0 ) pev->angles.x --;
		else if ( pev->angles.x < -1.0 ) pev->angles.x ++;
		else pev->angles.x = 0.0;
	}
	else
	{
		Vector vecForward = gpGlobals->v_forward;
		Vector vecRight = gpGlobals->v_right;
		Vector vecUp = gpGlobals->v_up;

		Vector vecSrc, vecAngles = Vector( 0, 0, 0 );
		
		if ( m_hEnemy )		// If the helicopter has an enemy start the grunts off facing it
		{
			vecAngles.y = UTIL_VecToYaw( m_hEnemy->pev->origin - pev->origin );
		}
		else
		{
			vecAngles.y = UTIL_VecToYaw( vecForward );
		}

		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), ignore_monsters, ENT(pev), &tr);
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, Classify(), tr.vecEndPos, 400, 0.3 );

		if ( m_iMaxGrunts > 0 )
		{
			vecSrc = pev->origin + vecForward * -16 + vecRight *  48 + vecUp * -240;
			m_hRepel[0] = MakeGrunt( vecSrc, vecAngles );
		}

		if ( m_iMaxGrunts > 1 )
		{
			vecSrc = pev->origin + vecForward * 96 + vecRight *  48 + vecUp * -240;
			m_hRepel[1] = MakeGrunt( vecSrc, vecAngles );
		}

		if ( m_iMaxGrunts > 2)
		{
			vecSrc = pev->origin + vecForward * -16 + vecRight * -36 + vecUp * -240;
			m_hRepel[2] = MakeGrunt( vecSrc, vecAngles );
		}

		if ( m_iMaxGrunts > 3 )
		{
			vecSrc = pev->origin + vecForward * 96 + vecRight * -36 + vecUp * -240;
			m_hRepel[3] = MakeGrunt( vecSrc, vecAngles );
		}

		SetThink( HoverThink );
	}

	FireGun( FALSE );
	ShowDamage();
	MakeSound();
}


void CHuey::MakeSound()
{
	if (m_flNextSoundTime > gpGlobals->time) return;

	EMIT_SOUND(ENT(pev), CHAN_STATIC, "huey/huey_rotor1.wav", 1.0, 0.3);
	m_flNextSoundTime = gpGlobals->time + 3.0f;

/*
	// make rotor, engine sounds
	if (m_iSoundState == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "huey/huey_rotor1.wav", 1.0, 0.3, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		// UNDONE: this needs to send different sounds to every player for multiplayer.	
		if (pPlayer)
		{

			float pitch = DotProduct( pev->velocity - pPlayer->pev->velocity, (pPlayer->pev->origin - pev->origin).Normalize() );

			pitch = (int)(100 + pitch / 50.0);

			if (pitch > 250) 
				pitch = 250;
			if (pitch < 50)
				pitch = 50;
			if (pitch == 100)
				pitch = 101;

			float flVol = (m_flForce / 100.0) + .1;
			if (flVol > 1.0) 
				flVol = 1.0;

			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "huey/huey_rotor1.wav", 1.0, 0.3, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
		}
	}
	*/
}


CBaseMonster *CHuey :: MakeGrunt( Vector vecSrc, Vector vecAngles )
{
	CBaseEntity *pEntity = NULL;
	CBaseMonster *pGrunt = NULL;

	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;

	for (int i = 0; i < m_iUnits; i++)
	{
		if (m_hGrunt[i] == NULL || !m_hGrunt[i]->IsAlive())
		{
			if (m_hGrunt[i] != NULL && m_hGrunt[i]->pev->rendermode == kRenderNormal)
			{
				m_hGrunt[i]->SUB_StartFadeOut( );
			}
			
			if (m_iPassenger == HUEY_PASSENGER_MIKE)
			{
				if ( RANDOM_LONG( 0, 3 ) )
					pEntity = Create( "monster_mikeforce", vecSrc, vecAngles );
				else
					pEntity = Create( "monster_mikeforce_medic", vecSrc, vecAngles );
			}
			else
			{
				if ( RANDOM_LONG( 0, 3 ) )
					pEntity = Create( "monster_human_grunt", vecSrc, vecAngles );
				else
					pEntity = Create( "monster_human_grunt_medic", vecSrc, vecAngles );
			}

			if (pEntity) pGrunt = pEntity->MyMonsterPointer( );
			
			if (pGrunt)
			{
				pGrunt->pev->movetype = MOVETYPE_FLY;
				pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
				if (m_iPassenger == HUEY_PASSENGER_GRUNT_FR) pGrunt->pev->spawnflags |= 0x0008;
				pGrunt->SetActivity( ACT_GLIDE );
				pGrunt->ApplyDefaultSettings();

				CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
				pBeam->PointEntInit( vecSrc + Vector(0,0,112), pGrunt->entindex() );
				pBeam->SetFlags( BEAM_FSOLID );
				pBeam->SetColor( 255, 255, 255 );
				pBeam->SetThink( SUB_Remove );
				pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

				pGrunt->m_vecLastPosition = m_vecOrigin[i];
				m_hGrunt[i] = pGrunt;
			}
			else
			{
				ALERT( at_console, "Null Ent created by helicopter\n" );
			}
			
			return pGrunt;
		}
	}
	// ALERT( at_console, "none dead\n");
	return NULL;
}


void CHuey :: FindAllThink( void )
{
	CBaseEntity *pEntity = NULL;

	m_iUnits = 0;
	while (m_iUnits < MAX_CARRY && (pEntity = UTIL_FindEntityByClassname( pEntity, 
		(m_iPassenger < HUEY_PASSENGER_MIKE ? "monster_human_grunt" : "monster_mikeforce") )) != NULL)
	{
		if (pEntity->IsAlive())
		{
			m_hGrunt[m_iUnits]		= pEntity;
			m_vecOrigin[m_iUnits]	= pEntity->pev->origin;
			m_iUnits++;
		}
	}

	while (m_iUnits < MAX_CARRY && (pEntity = UTIL_FindEntityByClassname( pEntity, 
		(m_iPassenger < HUEY_PASSENGER_MIKE ? "monster_human_grunt_medic" : "monster_mikeforce_medic") )) != NULL)
	{
		if (pEntity->IsAlive())
		{
			m_hGrunt[m_iUnits]		= pEntity;
			m_vecOrigin[m_iUnits]	= pEntity->pev->origin;
			m_iUnits++;
		}
	}

	SetThink( HuntThink );
	pev->nextthink = gpGlobals->time + 0.1;
}


BOOL CHuey :: HasDead( )
{
	for (int i = 0; i < m_iUnits; i++)
	{
		if (m_hGrunt[i] == NULL || !m_hGrunt[i]->IsAlive())
		{
			return TRUE;
		}
		else
		{
			m_vecOrigin[i] = m_hGrunt[i]->pev->origin;  // send them to where they died
		}
	}
	return FALSE;
}


//#endif


void CHuey::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq(pkvd->szKeyName, "maxgrunts") )
	{
		m_iMaxGrunts = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
	if ( FStrEq(pkvd->szKeyName, "passenger") )
	{
		m_iPassenger = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CHuey::ApplyDefaultSettings( void )
{
	SetBits( pev->spawnflags, SF_PASSENGER );
	SetBodygroup(BODY_PASSENGER, HUEY_PBODY_GRUNT);
	m_flPassengerHealth = gSkillData.hgruntHealth;

	m_iMaxGrunts = 2;
}

