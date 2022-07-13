//=========================================================
// gorilla.cpp - another tiny, jumpy alien parasite
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"decals.h"
#include	"soundent.h"
#include	"nodes.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define	GORILLA_AE_JUMPATTACK	( 2 )
#define GORILLA_AE_ATTACK_RIGHT ( 3 )
#define GORILLA_AE_ATTACK_LEFT  ( 4 )
#define GORILLA_AE_GALLOP		( 5 )
#define GORILLA_AE_SMACKDOWN	( 6 )

//=========================================================
// monster-specific schedule types
//=========================================================

enum
{
	SCHED_GORILLA_EAT = LAST_COMMON_SCHEDULE + 1,
	SCHED_GORILLA_SNIFF_AND_EAT,
};

#define MAX_JUMP_SPEED 1000

class CGorilla : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	Schedule_t* GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule( void );
	BOOL FValidateHintType ( short sHint );
	int ISoundMask ( void );
	BOOL HasHumanGibs( void ) { return TRUE; };
	BOOL HasAlienGibs( void ) { return FALSE; };
	virtual int GetVoicePitch( void ) { return 100; }
	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AlertSound( void );
	void AttackSound( void );
	void EatSound( void );
	void VictorySound( void );

	CUSTOM_SCHEDULES;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pEatSounds[];
	static const char *pFootStepSounds[];
	static const char *pVictorySounds[];
	static const char *pPunchSounds[];
};

LINK_ENTITY_TO_CLASS( monster_gorilla, CGorilla );

const char *CGorilla::pIdleSounds[] = 
{
	"gorilla/idle1.wav",
	"gorilla/idle2.wav",
	"gorilla/idle3.wav",
	"gorilla/idle4.wav",
	"gorilla/idle5.wav",
};
const char *CGorilla::pAlertSounds[] = 
{
	"gorilla/alert1.wav",
	"gorilla/alert2.wav",
	"gorilla/alert3.wav",
};
const char *CGorilla::pPainSounds[] = 
{
	"gorilla/pain1.wav",
	"gorilla/pain2.wav",
	"gorilla/pain3.wav",
};
const char *CGorilla::pAttackSounds[] = 
{
	"gorilla/attack1.wav",
	"gorilla/attack2.wav",
	"gorilla/attack3.wav",
};

const char *CGorilla::pDeathSounds[] = 
{
	"gorilla/die1.wav",
	"gorilla/die2.wav",
	"gorilla/die3.wav",
};

const char *CGorilla::pEatSounds[] = 
{
	"gorilla/eat.wav",
};

const char *CGorilla::pFootStepSounds[] = 
{
	"gorilla/footstep1.wav",
	"gorilla/footstep2.wav",
};

const char *CGorilla::pVictorySounds[] = 
{
	"gorilla/beatchest_roar.wav",
};

const char *CGorilla::pPunchSounds[] = 
{
	"gorilla/punch1.wav",
	"gorilla/punch2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================

int	CGorilla :: Classify ( void )
{
	return	CLASS_ALIEN_PREDATOR;
}



//=========================================================
// Spawn
//=========================================================

void CGorilla :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/gorilla.mdl");
	UTIL_SetSize(pev, Vector(-24, -24, 0), Vector(24, 24, 72));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.gorillaHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 120;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================

void CGorilla :: Precache()
{
	PRECACHE_MODEL("models/gorilla.mdl");

	PRECACHE_SOUND("gorilla/beatchest.wav");
	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");

	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pEatSounds);
	PRECACHE_SOUND_ARRAY(pFootStepSounds);
	PRECACHE_SOUND_ARRAY(pVictorySounds);
	PRECACHE_SOUND_ARRAY(pPunchSounds);
}	


//=========================================================
// IdleSound
//=========================================================
void CGorilla :: IdleSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CGorilla :: AlertSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// AttackSound 
//=========================================================
void CGorilla :: AttackSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// PainSound 
//=========================================================
void CGorilla :: PainSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CGorilla :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// EatSound 
//=========================================================
void CGorilla :: EatSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pEatSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// VictorySound 
//=========================================================
void CGorilla :: VictorySound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pVictorySounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================

void CGorilla :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:			
		ys = 120;
		break;
	
	case ACT_RUN:			
		ys = 30;
		break;

	case ACT_WALK:			
		ys = 120;
		break;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;

	case ACT_RANGE_ATTACK1:	
		ys = 30;
		break;
	default:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================

void CGorilla :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case GORILLA_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = CVAR_GET_FLOAT( "sv_gravity" );
				if (gravity <= 1)
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				float height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
				if (height < 16)
					height = 16;
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
				vecJumpDir = vecJumpDir * ( 1.0 / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();
				
				if (distance > MAX_JUMP_SPEED )
				{
					vecJumpDir = vecJumpDir * ( MAX_JUMP_SPEED / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			AttackSound();

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
		}
		break;

		case GORILLA_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.gorillaDmgPunch, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 300 - gpGlobals->v_up * 100;
				}
				// Play a random punch sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pPunchSounds ), 1.0, ATTN_NORM, 0, GetVoicePitch() );
			}
		}
		break;

		case GORILLA_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.gorillaDmgPunch, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 300 - gpGlobals->v_up * 100;
				}

				// Play a random punch sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pPunchSounds ), 1.0, ATTN_NORM, 0, GetVoicePitch() );
			}
		}
		break;

		case GORILLA_AE_SMACKDOWN:
		{
			TraceResult tr;
			UTIL_MakeAimVectors( pev->angles );

			Vector vecStart = pev->origin + Vector( 0, 0, 16 );
			Vector vecEnd = vecStart + (gpGlobals->v_forward * 70 );

			UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
			if ( tr.pHit )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

				pEntity->TakeDamage( pev, pev, gSkillData.gorillaDmgPunch * 2, DMG_CLUB );
	
				// Play a random punch sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pPunchSounds ), 1.0, ATTN_NORM, 0, GetVoicePitch() );
			}
		}
		break;

		case GORILLA_AE_GALLOP:
		{
			EMIT_SOUND( edict(), CHAN_BODY, RANDOM_SOUND_ARRAY(pFootStepSounds), 1, ATTN_NORM );
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}


//=========================================================
// RunTask 
//=========================================================

void CGorilla :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouch( NULL );
				m_IdealActivity = ACT_IDLE;
			}
		}
		break;

	default:
		{
			CBaseMonster :: RunTask(pTask);
		}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================

void CGorilla :: LeapTouch ( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage )
	{
		return;
	}

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		pOther->TakeDamage( pev, pev, gSkillData.gorillaDmgJump, DMG_CLUB );
	}

	SetTouch( NULL );
}


//=========================================================
// RunTask 
//=========================================================

void CGorilla :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch ( LeapTouch );
			break;
		}

	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================

BOOL CGorilla :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 512 && flDist > 64 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}


//=========================================================
// CheckMeleeAttack1
//=========================================================

BOOL CGorilla :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= 64 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND )
		 && m_hEnemy->pev->absmax.z > pev->origin.z + 48 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================

BOOL CGorilla :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND )
		 && m_hEnemy->pev->absmax.z <= pev->origin.z + 48 )
	{
		return TRUE;
	}
	return FALSE;
}


//=========================================================
// GetSchedule 
//=========================================================

Schedule_t *CGorilla :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		if ( HasConditions( bits_COND_ENEMY_DEAD ))
		{
			return GetScheduleOfType ( SCHED_VICTORY_DANCE );
		}

	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_COMBAT:
		
		if ( HasConditions( bits_COND_ENEMY_DEAD | 
							bits_COND_LIGHT_DAMAGE | 
							bits_COND_HEAVY_DAMAGE |
							bits_COND_HEAR_SOUND |
							bits_COND_NEW_ENEMY |
							bits_COND_CAN_RANGE_ATTACK1 ) )
			return CBaseMonster :: GetSchedule();	// Base class handles all this stuff

		if ( HasConditions(bits_COND_SMELL_FOOD) )	// Won't smell food if he's not hungry, base class ensures this
		{
			CSound		*pSound;
			pSound = PBestScent();
				
			if ( pSound && (!FInViewCone ( &pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )
			{
				// scent is behind or occluded
				return GetScheduleOfType( SCHED_GORILLA_SNIFF_AND_EAT );
			}
	
			// food is right out in the open. Just go get it.
			return GetScheduleOfType( SCHED_GORILLA_EAT );
		}
		
		break;
	}

	return CBaseMonster :: GetSchedule();
}


//=========================================================
//  FValidateHintType 
//=========================================================

BOOL CGorilla :: FValidateHintType ( short sHint )
{
	int i;

	static short sGorillaHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for ( i = 0 ; i < ARRAYSIZE ( sGorillaHints ) ; i++ )
	{
		if ( sGorillaHints[ i ] == sHint )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================

int CGorilla :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}


//=========================================================
// RangeAttack1
//=========================================================

Task_t	tlGorillaRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_WAIT_RANDOM,			(float)0.5		},
};

Schedule_t	slGorillaRangeAttack1[] =
{
	{ 
		tlGorillaRangeAttack1,
		ARRAYSIZE ( tlGorillaRangeAttack1 ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"GorillaRangeAttack1"
	},
};


//=========================================================
// gorilla walks to something tasty and eats it.
//=========================================================

Task_t tlGorillaEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_DONT_EAT,				(float)10				},// this is in case he can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_SOUND_EAT,				(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_SOUND_EAT,				(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_SOUND_EAT,				(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slGorillaEat[] =
{
	{
		tlGorillaEat,
		ARRAYSIZE( tlGorillaEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"GorillaEat"
	}
};


//=========================================================
// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the squid. This schedule plays a sniff animation before going to the source of food.
//=========================================================

Task_t tlGorillaSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_DONT_EAT,				(float)10				},// this is in case the squid can't get to the food
	{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_SOUND_EAT,				(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_SOUND_EAT,				(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_SOUND_EAT,				(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slGorillaSniffAndEat[] =
{
	{
		tlGorillaSniffAndEat,
		ARRAYSIZE( tlGorillaSniffAndEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"GorillaSniffAndEat"
	}
};


//=========================================================
// Victory Dance (Go up to enemy corpse and eat it)
//=========================================================

Task_t tlGorillaVictoryDance[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SOUND_VICTORY,			(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_VICTORY_DANCE	},
	{ TASK_WAIT,					(float)0					},
};

Schedule_t slGorillaVictoryDance[] =
{
	{
		tlGorillaVictoryDance,
		ARRAYSIZE( tlGorillaVictoryDance ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY,
		0,
		"Gorilla Victory Dance"
	},
};


//=========================================================
// primary melee attack
//=========================================================

Task_t	tlGorillaPrimaryMeleeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOUND_ANGRY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slGorillaPrimaryMeleeAttack[] =
{
	{ 
		tlGorillaPrimaryMeleeAttack1,
		ARRAYSIZE ( tlGorillaPrimaryMeleeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Gorilla Primary Melee Attack"
	},
};


DEFINE_CUSTOM_SCHEDULES( CGorilla )
{
	slGorillaRangeAttack1,
	slGorillaEat,
	slGorillaSniffAndEat,
	slGorillaVictoryDance,
	slGorillaPrimaryMeleeAttack,
};

IMPLEMENT_CUSTOM_SCHEDULES( CGorilla, CBaseMonster );


//=========================================================
// GetScheduleOfType
//=========================================================

Schedule_t* CGorilla :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_MELEE_ATTACK1:
		{
			return &slGorillaPrimaryMeleeAttack[ 0 ];
		}
		break;

		case SCHED_RANGE_ATTACK1:
		{
			return &slGorillaRangeAttack1[ 0 ];
		}
		break;

		case SCHED_GORILLA_EAT:
		{
			return &slGorillaEat[ 0 ];
		}
		break;

		case SCHED_GORILLA_SNIFF_AND_EAT:
		{
			return &slGorillaSniffAndEat[ 0 ];
		}
		break;

		case SCHED_VICTORY_DANCE:
		{
			return &slGorillaVictoryDance[ 0 ];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}


