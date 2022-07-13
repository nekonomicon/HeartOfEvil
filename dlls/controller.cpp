/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

//=========================================================
// CONTROLLER
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"squadmonster.h"
#include	"explode.h"
#include	"decals.h"
#include	"soundent.h"

int	iAcidSpitSprite;
int iAcidSteamSprite;

//=========================================================
// Acid spit projectile
//=========================================================

class CAcidSpit : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS( acidspit, CAcidSpit );

TYPEDESCRIPTION	CAcidSpit::m_SaveData[] = 
{
	DEFINE_FIELD( CAcidSpit, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAcidSpit, CBaseEntity );

void CAcidSpit:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "acidspit" );
	
	pev->solid = SOLID_BBOX;
//	pev->rendermode = kRenderTransAlpha;
//	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "models/acidspit.mdl");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
}

void CAcidSpit::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->frame++ )
	{
		if ( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

void CAcidSpit::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CAcidSpit *pSpit = GetClassPtr( (CAcidSpit *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->angles = UTIL_VecToAngles( vecVelocity );
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink ( Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CAcidSpit :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "spx_mature/spithit1.wav", 1, ATTN_NORM, 0, iPitch );	

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "spx_mature/acid1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "spx_mature/acid2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}

	if ( !pOther->pev->takedamage )
	{

		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_BLACKSPIT1 + RANDOM_LONG(0,1));

		// Show a steam sprite

		Vector vecSteam = tr.vecEndPos + tr.vecPlaneNormal * 6 + Vector(0,0,28);

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSteam );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecSteam.x);	// pos
			WRITE_COORD( vecSteam.y);	
			WRITE_COORD( vecSteam.z);	
			WRITE_SHORT( iAcidSteamSprite );// model
			WRITE_BYTE ( 10 );				// scale
			WRITE_BYTE ( 100 );				// brightness
		MESSAGE_END();
	}
	else
	{
		pOther->TakeDamage ( pev, pev, gSkillData.controllerDmgSpit, DMG_POISON );
	}

	SetThink ( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}


//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define	CONTROLLER_AE_SHOOT				2
#define	CONTROLLER_AE_PUNCH_RIGHT		3
#define	CONTROLLER_AE_PUNCH_LEFT		4
#define	CONTROLLER_AE_PUNCH_DOWN_RIGHT	5
#define	CONTROLLER_AE_PUNCH_DOWN_LEFT	6

#define CONTROLLER_FLINCH_DELAY	2		// at most one flinch every n secs

#define TASK_FLY_PATH TASK_WALK_PATH	// If pev->movetype==MOVETYPE_FLY, TASK_WALK_PATH sets m_movementActivity=ACT_FLY

#define MAX_SPEED 500
#define MIN_SPEED 200


//=========================================================
// Controller Class
//=========================================================

class CController : public CSquadMonster
{
public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunAI( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );	// spit
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );	// punch forward
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );	// punch down
	Schedule_t* GetSchedule ( void );
	Schedule_t* GetScheduleOfType ( int Type );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	CUSTOM_SCHEDULES;

	float HearingSensitivity( void ) { return 10.0; };
	void Stop( void );
	void Move ( float flInterval );
	int  CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void SetActivity ( Activity NewActivity );
	BOOL ShouldAdvanceRoute( float flWaypointDist );
	int LookupFloat( );
	float GetAcceleration( float flGroundSpeed, float flIdealSpeed, Vector &vecDir );
	float GetIdealSpeed( float flWayPointDist, Vector &vecDir );

	float m_flNextFlinch;

	float m_flShootTime;
	float m_flShootEnd;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void DeathSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pPunchSounds[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	Vector m_velocity;
	int m_fInCombat;

	float m_flNextSpitTime;
};

LINK_ENTITY_TO_CLASS( monster_alien_controller, CController );

TYPEDESCRIPTION	CController::m_SaveData[] = 
{
	DEFINE_FIELD( CController, m_flNextSpitTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CController, CSquadMonster );


const char *CController::pAttackSounds[] = 
{
	"spx_mature/attack1.wav",
	"spx_mature/attack2.wav",
	"spx_mature/attack3.wav",
};

const char *CController::pIdleSounds[] = 
{
	"spx_mature/idle1.wav",
	"spx_mature/idle2.wav",
	"spx_mature/idle3.wav",
	"spx_mature/idle4.wav",
	"spx_mature/idle5.wav",
};

const char *CController::pAlertSounds[] = 
{
	"spx_mature/alert1.wav",
	"spx_mature/alert2.wav",
};

const char *CController::pPainSounds[] = 
{
	"spx_mature/pain1.wav",
	"spx_mature/pain2.wav",
};

const char *CController::pDeathSounds[] = 
{
	"spx_mature/die1.wav",
	"spx_mature/die2.wav",
};

const char *CController::pPunchSounds[] = 
{
	"gorilla/punch1.wav",
	"gorilla/punch2.wav",
};


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CController :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CController :: SetYawSpeed ( void )
{
	int ys;

	if ( IsMoving() )
	{
		ys = 60;
	}
	else
	{
		ys = 180;
	}

	pev->yaw_speed = ys;
}

int CController :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


void CController::Killed( entvars_t *pevAttacker, int iGib )
{
	CSquadMonster::Killed( pevAttacker, iGib );
}


void CController::GibMonster( void )
{
	CSquadMonster::GibMonster( );
}




void CController :: PainSound( void )
{
	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds ); 
}	

void CController :: AlertSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAlertSounds ); 
}

void CController :: IdleSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pIdleSounds ); 
}

void CController :: AttackSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAttackSounds ); 
}

void CController :: DeathSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pDeathSounds ); 
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================

void CController :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case CONTROLLER_AE_SHOOT:
		{
			UTIL_MakeVectors ( pev->angles );

			// Get position of mouth

			Vector vecStart, angleGun;
			GetAttachment( 0, vecStart, angleGun );

			// Get direction to shoot at

			Vector vecSpitDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecStart ).Normalize();

			vecSpitDir.x += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.y += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.z += RANDOM_FLOAT( -0.05, 0 );

			// do stuff for this event.
			AttackSound();

			// spew the spittle temporary ents.
/*			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecStart );
				WRITE_BYTE( TE_SPRITE_SPRAY );
				WRITE_COORD( vecStart.x);	// pos
				WRITE_COORD( vecStart.y);	
				WRITE_COORD( vecStart.z);	
				WRITE_COORD( vecSpitDir.x);	// dir
				WRITE_COORD( vecSpitDir.y);	
				WRITE_COORD( vecSpitDir.z);	
				WRITE_SHORT( iAcidSpitSprite );	// model
				WRITE_BYTE ( 15 );			// count
				WRITE_BYTE ( 210 );			// speed
				WRITE_BYTE ( 25 );			// noise ( client will divide by 100 )
			MESSAGE_END();*/

			CAcidSpit::Shoot( pev, vecStart, vecSpitDir * 900 );
		}
		break;

		case CONTROLLER_AE_PUNCH_RIGHT:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.controllerDmgPunch, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 200 - gpGlobals->v_up * 50;
				}
				// Play a random punch sound
				EMIT_SOUND ( ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pPunchSounds ), 1.0, ATTN_NORM );
			}
		}
		break;

		case CONTROLLER_AE_PUNCH_LEFT:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.controllerDmgPunch, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200 - gpGlobals->v_up * 50;
				}

				// Play a random punch sound
				EMIT_SOUND ( ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pPunchSounds ), 1.0, ATTN_NORM );
			}
		}
		break;

		case CONTROLLER_AE_PUNCH_DOWN_RIGHT:
		case CONTROLLER_AE_PUNCH_DOWN_LEFT:
		{
			TraceResult tr;
			UTIL_MakeAimVectors( pev->angles );

			Vector vecStart = pev->origin;
			Vector vecEnd = vecStart - (gpGlobals->v_up * 96 );

			UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, human_hull, ENT(pev), &tr );
	
			if ( tr.pHit )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
				pEntity->TakeDamage( pev, pev, gSkillData.controllerDmgPunch, DMG_SLASH );

				EMIT_SOUND ( ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pPunchSounds ), 1.0, ATTN_NORM );
			}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CController :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/spx_mature.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 64 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags			|= FL_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= gSkillData.controllerHealth;
	pev->view_ofs		= Vector( 0, 0, -2 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CController :: Precache()
{
	PRECACHE_MODEL("models/spx_mature.mdl");

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND_ARRAY( pPunchSounds );

	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.
	
	iAcidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.
	iAcidSteamSprite = PRECACHE_MODEL("sprites/xssmke1.spr");// client side spittle.
	PRECACHE_MODEL("models/acidspit.mdl");

	PRECACHE_SOUND("spx_mature/spithit1.wav");
	PRECACHE_SOUND("spx_mature/acid1.wav");
	PRECACHE_SOUND("spx_mature/acid2.wav");
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Chase enemy schedule
//=========================================================

Task_t tlControllerChaseEnemy[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1		},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_SOUND_ANGRY,			(float)0.2						},
	{ TASK_FLY_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
	{ TASK_STOP_MOVING,			0								},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE					},
	{ TASK_FACE_ENEMY,			(float)0						},
};

Schedule_t slControllerChaseEnemy[] =
{
	{ 
		tlControllerChaseEnemy,
		ARRAYSIZE ( tlControllerChaseEnemy ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_NEW_ENEMY			|
		bits_COND_TASK_FAILED,
		0,
		"ControllerChaseEnemy"
	},
};


//==================================================================
// Primary melee attack
//==================================================================

Task_t	tlControllerMeleeAttack1[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_SOUND_ANGRY,			(float)0.2					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_MELEE_ATTACK1	},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_SET_SCHEDULE,		SCHED_TAKE_COVER_FROM_ENEMY },
};

Schedule_t	slControllerMeleeAttack1[] =
{
	{ 
		tlControllerMeleeAttack1,
		ARRAYSIZE ( tlControllerMeleeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Controller Melee Attack1"
	},
};


//==================================================================
// Secondary melee attack
//==================================================================

Task_t	tlControllerMeleeAttack2[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_SOUND_ANGRY,			(float)0.2					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_MELEE_ATTACK2	},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_SET_SCHEDULE,		SCHED_TAKE_COVER_FROM_ENEMY },
};

Schedule_t	slControllerMeleeAttack2[] =
{
	{ 
		tlControllerMeleeAttack2,
		ARRAYSIZE ( tlControllerMeleeAttack2 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Controller Melee Attack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CController )
{
	slControllerChaseEnemy,
	slControllerMeleeAttack1,
	slControllerMeleeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CController, CSquadMonster );



//=========================================================
// StartTask
//=========================================================
void CController :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		CSquadMonster :: StartTask ( pTask );
		break;
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, pTask->flData, (m_vecEnemyLKP - pev->origin).Length() + 1024 ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemyLKP failed!!\n" );
				TaskFail();
			}
			break;
		}
	
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			float dist;
			if ( CheckLocalMove( pev->origin, m_hEnemy->pev->origin, m_hEnemy, &dist ) == LOCALMOVE_VALID )
			{
				RouteNew();
				m_vecMoveGoal = m_hEnemy->pev->origin;
				m_movementGoal = RouteClassify( bits_MF_TO_ENEMY );
				m_Route[ 0 ].vecLocation = m_hEnemy->pev->origin;
				m_Route[ 0 ].iType = bits_MF_TO_ENEMY | bits_MF_IS_GOAL;

				TaskComplete();
			}
			else if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, pTask->flData, (pEnemy->pev->origin - pev->origin).Length() + 1024 ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}

	case TASK_STOP_MOVING:
		{
			m_vecMoveGoal = pev->origin;
			CSquadMonster :: StartTask ( pTask );
		}
		break;

	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		{
			m_movementActivity = ACT_FLY;
			TaskComplete();
			break;
		}

	default:
		CSquadMonster :: StartTask ( pTask );
		break;
	}
}


int CController::LookupFloat( )
{
	UTIL_MakeAimVectors( pev->angles );
	float x = DotProduct( gpGlobals->v_forward, m_velocity );
	float y = DotProduct( gpGlobals->v_right, m_velocity );
	float z = DotProduct( gpGlobals->v_up, m_velocity );

	if (fabs(x) > fabs(y) && fabs(x) > fabs(z))
	{
		if (x > 0)
			return LookupSequence( "forward");
		else
			return LookupSequence( "backward");
	}
	else if (fabs(y) > fabs(z))
	{
		if (y > 0)
			return LookupSequence( "right");
		else
			return LookupSequence( "left");
	}
	else
	{
		if (z > 0)
			return LookupSequence( "up");
		else
		{
			if ( pev->deadflag != DEAD_NO ) return LookupSequence("fall");

			return LookupSequence( "down");
		}
	}
}


//=========================================================
// RunTask 
//=========================================================

void CController :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
		{
			if ( m_hEnemy != NULL && m_hEnemy->IsAlive() )
			{
				MakeIdealYaw ( m_vecEnemyLKP );
				SetYawSpeed();
				ChangeYaw( pev->yaw_speed ); 
			}

			CSquadMonster :: RunTask ( pTask );
		}
		break;

	default:
		{
			CSquadMonster :: RunTask ( pTask );
		}
		break;
	}
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================

Schedule_t *CController :: GetSchedule ( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		break;

	case MONSTERSTATE_ALERT:
		break;

	case MONSTERSTATE_COMBAT:
		if ( HasConditions( bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY ) )
		{
			return CSquadMonster :: GetSchedule();
		}
		else
		{
			if ( !HasConditions(bits_COND_SEE_ENEMY) )
			{
				// we can't see the enemy
				if ( !HasConditions(bits_COND_ENEMY_OCCLUDED) )
				{
					if ( FInViewCone( &m_vecEnemyLKP ) && HasConditions( bits_COND_HEAR_SOUND ) )
					{
						CSound *pSound = NULL;
						pSound = PBestSound();
						if ( pSound )
						{
							MakeIdealYaw ( pSound->m_vecOrigin );
							SetYawSpeed();
							return GetScheduleOfType( SCHED_ALERT_FACE );
						}
					}
				}
			}
		}
		break;
	}

	return CSquadMonster :: GetSchedule();
}


//=========================================================
// GetScheduleOfType
//=========================================================

Schedule_t* CController :: GetScheduleOfType ( int Type ) 
{
	// ALERT( at_console, "%d\n", m_iFrustration );
	switch	( Type )
	{
	case SCHED_CHASE_ENEMY:
		{
			return slControllerChaseEnemy;
		}
		break;

	case SCHED_MELEE_ATTACK1:
		{
			return slControllerMeleeAttack1;
		}
		break;

	case SCHED_MELEE_ATTACK2:
		{
			return slControllerMeleeAttack2;
		}
		break;

	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL && m_hEnemy->IsAlive() && !HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
		}
		break;
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}


//=========================================================
// CheckRangeAttack1  - shoot spit
//=========================================================

BOOL CController :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
	{
		return FALSE;
	}

	if ( flDot > 0.5 && flDist > 256 && flDist <= 2048 && gpGlobals->time >= m_flNextSpitTime )
	{
		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}
	return FALSE;
}


BOOL CController :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( fabs(pev->origin.z - m_vecEnemyLKP.z) > 32 ) return FALSE;

	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= 64 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CController :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( pev->origin.z - m_vecEnemyLKP.z >= 32 && pev->origin.z - m_vecEnemyLKP.z < 96 && flDist <= 48 ) 
	{
		return TRUE;
	}
	return FALSE;
}


void CController :: SetActivity ( Activity NewActivity )
{
	switch ( NewActivity )
	{
	case ACT_FLY:
		{
			int iFloat = LookupFloat( );
			if (m_fSequenceFinished || iFloat != pev->sequence)
			{
				if ( pev->sequence != iFloat || !m_fSequenceLoops )
				{
					pev->frame = 0;
					m_flGroundSpeed = MIN_SPEED;
				}
				pev->sequence = iFloat;
				ResetSequenceInfo( );
				SetYawSpeed();
			}
			m_Activity = NewActivity;
		}
		break;

	default:
		{
			CBaseMonster::SetActivity( NewActivity );
			m_flGroundSpeed = MIN_SPEED;
		}
		break;
	}

}



//=========================================================
// RunAI
//=========================================================
void CController :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if ( HasMemory( bits_MEMORY_KILLED ) )
		return;
}


extern void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b );

void CController::Stop( void ) 
{ 
	m_IdealActivity = GetStoppedActivity(); 
}


#define DIST_TO_CHECK	200
void CController :: Move ( float flInterval ) 
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	float		flMoveDist;
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if ( FRouteClear() )
	{
		ALERT( at_aiconsole, "Tried to move with no route!\n" );
		TaskFail();
		return;
	}
	
	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;

// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT("stopmove" ) != 0 )
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();
		return;
	}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 0, 255 );
#endif

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	if (m_flGroundSpeed == 0)
	{
		m_flGroundSpeed = MIN_SPEED;
		// TaskFail( );
		// return;
	}

	flMoveDist = m_flGroundSpeed * flInterval;

//	do 
//	{
		// local move to waypoint.
		vecDir = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
		flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length();
		
		// MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );
		// ChangeYaw ( pev->yaw_speed );

		// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
		if ( flWaypointDist < DIST_TO_CHECK )
		{
			flCheckDist = flWaypointDist;
		}
		else
		{
			flCheckDist = DIST_TO_CHECK;
		}
		
		if ( (m_Route[ m_iRouteIndex ].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY )
		{
			// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
			pTargetEnt = m_hEnemy;
		}
		else if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT )
		{
			pTargetEnt = m_hTargetEnt;
		}

		// !!!BUGBUG - CheckDist should be derived from ground speed.
		// If this fails, it should be because of some dynamic entity blocking this guy.
		// We've already checked this path, so we should wait and time out if the entity doesn't move
		flDist = 0;
		if ( CheckLocalMove ( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
		{
			CBaseEntity *pBlocker;

			// Can't move, stop
			Stop();
			// Blocking entity is in global trace_ent
			pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
			if (pBlocker)
			{
				DispatchBlocked( edict(), pBlocker->edict() );
			}
			if ( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0 )
			{
				// Can we still move toward our target?
				if ( flDist < m_flGroundSpeed )
				{
					// Wait for a second
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
	//				ALERT( at_aiconsole, "Move %s!!!\n", STRING( pBlocker->pev->classname ) );
					return;
				}
			}
			else 
			{
				// try to triangulate around whatever is in the way.
				if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist, pTargetEnt, &vecApex ) )
				{
					InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
					RouteSimplify( pTargetEnt );
				}
				else
				{
	 			    ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
					Stop();
					if ( m_moveWaitTime > 0 )
					{
						FRefreshRoute();
						m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime * 0.5;
					}
					else
					{
						TaskFail();
						ALERT( at_aiconsole, "Failed to move!\n" );
						//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z );
					}
					return;
				}
			}
		}

		// UNDONE: this is a hack to quit moving farther than it has looked ahead.
		if (flCheckDist < flMoveDist)
		{
			MoveExecute( pTargetEnt, vecDir, flCheckDist / m_flGroundSpeed );

			// ALERT( at_console, "%.02f\n", flInterval );
			AdvanceRoute( flWaypointDist );
			flMoveDist -= flCheckDist;
		}
		else
		{
			MoveExecute( pTargetEnt, vecDir, flMoveDist / m_flGroundSpeed );

			if ( ShouldAdvanceRoute( flWaypointDist - flMoveDist ) )
			{
				AdvanceRoute( flWaypointDist );
			}
			flMoveDist = 0;
		}

		if ( MovementIsComplete() )
		{
			Stop();
			RouteClear();
		}
//	} while (flMoveDist > 0 && flCheckDist > 0);

	// cut corner?
	if (flWaypointDist < 128)
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();
	}

	m_flGroundSpeed += GetAcceleration( m_flGroundSpeed, GetIdealSpeed( flWaypointDist, vecDir ), vecDir );
}


//============================================================
// Ideal speed - faster if going downards or far from target
//============================================================

float CController::GetIdealSpeed( float flWayPointDist, Vector &vecDir )
{
	if (flWayPointDist < 128)
	{
		return 100;
	}
	else
	{
		if (vecDir.z >= 0)
		{
			return 400 - vecDir.z * 100;
		}
		else
		{
			return 400 - vecDir.z * 600;
		}
	}
}


//============================================================
// Acceleration per second
//============================================================

float CController::GetAcceleration( float flGroundSpeed, float flIdealSpeed, Vector &vecDir )
{
	float flDiff = flIdealSpeed - m_flGroundSpeed;

	if ( flDiff > 0 ) // Speeding up
	{
		// Faster acceleration if going downwards
		float a;
		if ( vecDir.z >= 0 )
			a = 100 + vecDir.z * 100;
		else
			a = 100 + vecDir.z * 60;
		if ( flDiff < a ) return flDiff; else return a;
	}
	else if ( flDiff < 0 )	// Slowing down
	{
		// Slower deceleration if going downwards
		float a = -150 + vecDir.z * 50;
		if ( flDiff > a ) return flDiff; else return a;
	}
	else
	{
		return 0;
	}
}


BOOL CController:: ShouldAdvanceRoute( float flWaypointDist )
{
	if ( flWaypointDist <= 32  )
	{
		return TRUE;
	}

	return FALSE;
}


int CController :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0, 0, 32), vecEnd + Vector( 0, 0, 32), dont_ignore_monsters, large_hull, edict(), &tr );

	// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if (pflDist)
	{
		*pflDist = ( (tr.vecEndPos - Vector( 0, 0, 32 )) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if (tr.fStartSolid || tr.flFraction < 1.0)
	{
		if ( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}


void CController::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	if ( m_Activity == ACT_FLY && LookupFloat() != pev->sequence )
	{
		SetActivity( m_Activity );
	}

	// ALERT( at_console, "move %.4f %.4f %.4f : %f\n", vecDir.x, vecDir.y, vecDir.z, flInterval );

	// float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	// UTIL_MoveToOrigin ( ENT(pev), m_Route[ m_iRouteIndex ].vecLocation, flTotal, MOVE_STRAFE );

	m_velocity = m_velocity * 0.8 + m_flGroundSpeed * vecDir * 0.2;

	UTIL_MoveToOrigin ( ENT(pev), pev->origin + m_velocity, m_velocity.Length() * flInterval, MOVE_STRAFE );
}




