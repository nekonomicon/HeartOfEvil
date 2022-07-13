//=========================================================
// kophyaeger.cpp - another tiny, jumpy alien parasite
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"decals.h"
#include	"soundent.h"
#include	"nodes.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define	KOPHYAEGER_AE_JUMPATTACK	( 2 )

//=========================================================
// monster-specific schedule types
//=========================================================

enum
{
	SCHED_KOPHYAEGER_EAT = LAST_COMMON_SCHEDULE + 1,
	SCHED_KOPHYAEGER_SNIFF_AND_EAT,
};


#define MAX_JUMP_SPEED 1000
#define ADULT_MAX_JUMP_SPEED 3000

class CKophyaeger : public CSquadMonster
{
	virtual void Spawn( void );
	virtual void Precache( void );
	virtual int  Classify ( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	Schedule_t* GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule( void );
	BOOL FValidateHintType ( short sHint );
	int ISoundMask ( void );
	virtual int GetVoicePitch( void ) { return 110; }
	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AlertSound( void );
	virtual float GetBiteDamage( void ) { return gSkillData.kophyaegerDmgBite; }
	virtual float GetMaxJumpSpeed( void ) { return MAX_JUMP_SPEED; }

	CUSTOM_SCHEDULES;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pEatSounds[];
};

class CAdultKophyaeger : public CKophyaeger
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void ) { return CLASS_ALIEN_PREDATOR; };
	int IRelationship( CBaseEntity *pTarget );
	int GetVoicePitch( void ) { return 90; }
	void GibMonster( void ) { CSquadMonster::GibMonster(); }
	float GetBiteDamage( void ) { return gSkillData.kophyaegerDmgBite * 3; }
	virtual float GetMaxJumpSpeed( void ) { return ADULT_MAX_JUMP_SPEED; }
};

LINK_ENTITY_TO_CLASS( monster_kophyaeger, CKophyaeger );
LINK_ENTITY_TO_CLASS( monster_kophyaeger_adult, CAdultKophyaeger );

const char *CKophyaeger::pIdleSounds[] = 
{
	"kophyaeger/idle1.wav",
	"kophyaeger/idle2.wav",
	"kophyaeger/idle3.wav",
};
const char *CKophyaeger::pAlertSounds[] = 
{
	"kophyaeger/alert.wav",
};
const char *CKophyaeger::pPainSounds[] = 
{
	"kophyaeger/pain1.wav",
	"kophyaeger/pain2.wav",
};
const char *CKophyaeger::pAttackSounds[] = 
{
	"kophyaeger/attack1.wav",
	"kophyaeger/attack2.wav",
	"kophyaeger/attack3.wav",
};

const char *CKophyaeger::pDeathSounds[] = 
{
	"kophyaeger/death1.wav",
	"kophyaeger/death2.wav",
};

const char *CKophyaeger::pEatSounds[] = 
{
	"kophyaeger/eat.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================

int	CKophyaeger :: Classify ( void )
{
	return	CLASS_ALIEN_PREY;
}



//=========================================================
// Spawn
//=========================================================

void CKophyaeger :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/kophyaeger.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 32));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.kophyaegerHealth;
	pev->view_ofs		= Vector ( 0, 0, 26 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 120;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_SQUAD;

	MonsterInit();
}


void CAdultKophyaeger :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/kophadult.mdl");
	UTIL_SetSize(pev, Vector(-24, -24, 0), Vector(24, 24, 80));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.kophyaegerHealth * 3;
	pev->view_ofs		= Vector ( 0, 0, 72 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 120;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_SQUAD;

	MonsterInit();
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================

void CKophyaeger :: Precache()
{
	PRECACHE_MODEL("models/kophyaeger.mdl");
	PRECACHE_MODEL("models/stickygib.mdl");

	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pEatSounds);
}	


void CAdultKophyaeger :: Precache()
{
	PRECACHE_MODEL("models/kophadult.mdl");

	PRECACHE_SOUND_ARRAY(CKophyaeger::pIdleSounds);
	PRECACHE_SOUND_ARRAY(CKophyaeger::pAlertSounds);
	PRECACHE_SOUND_ARRAY(CKophyaeger::pPainSounds);
	PRECACHE_SOUND_ARRAY(CKophyaeger::pAttackSounds);
	PRECACHE_SOUND_ARRAY(CKophyaeger::pDeathSounds);
	PRECACHE_SOUND_ARRAY(CKophyaeger::pEatSounds);
}	


//=========================================================
// IdleSound
//=========================================================
void CKophyaeger :: IdleSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CKophyaeger :: AlertSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CKophyaeger :: PainSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CKophyaeger :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================

void CKophyaeger :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:			
		ys = 60;
		break;
	case ACT_RUN:			
	case ACT_WALK:			
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 60;
		break;
	default:
		ys = 60;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================

void CKophyaeger :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case KOPHYAEGER_AE_JUMPATTACK:
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
				
				if (distance > GetMaxJumpSpeed() )
				{
					vecJumpDir = vecJumpDir * ( GetMaxJumpSpeed() / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1, ATTN_NORM, 0, GetVoicePitch() );

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}


//=========================================================
// RunTask 
//=========================================================

void CKophyaeger :: RunTask ( Task_t *pTask )
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
			break;
		}
	default:
		{
			CSquadMonster :: RunTask(pTask);
		}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================

void CKophyaeger :: LeapTouch ( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage )
	{
		return;
	}

	if ( IRelationship( pOther ) <= R_NO )
	{
		return;
	}

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		pOther->TakeDamage( pev, pev, GetBiteDamage(), DMG_SLASH );
	}

	SetTouch( NULL );
}


//=========================================================
// RunTask 
//=========================================================

void CKophyaeger :: StartTask ( Task_t *pTask )
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
			CSquadMonster :: StartTask( pTask );
		}
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================

BOOL CKophyaeger :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 256 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}


//=========================================================
// GetSchedule 
//=========================================================

Schedule_t *CKophyaeger :: GetSchedule( void )
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
			return CSquadMonster :: GetSchedule();	// Base class handles all this stuff

		if ( HasConditions(bits_COND_SMELL_FOOD) )	// Won't smell food if he's not hungry, base class ensures this
		{
			CSound		*pSound;
			pSound = PBestScent();
				
			if ( pSound && (!FInViewCone ( &pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )
			{
				// scent is behind or occluded
				return GetScheduleOfType( SCHED_KOPHYAEGER_SNIFF_AND_EAT );
			}
	
			// food is right out in the open. Just go get it.
			return GetScheduleOfType( SCHED_KOPHYAEGER_EAT );
		}
		
		break;
	}

	return CSquadMonster :: GetSchedule();
}


//=========================================================
//  FValidateHintType 
//=========================================================

BOOL CKophyaeger :: FValidateHintType ( short sHint )
{
	int i;

	static short sKophyaegerHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for ( i = 0 ; i < ARRAYSIZE ( sKophyaegerHints ) ; i++ )
	{
		if ( sKophyaegerHints[ i ] == sHint )
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

int CKophyaeger :: ISoundMask ( void )
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

Task_t	tlKophyaegerRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_WAIT_RANDOM,			(float)0.5		},
};

Schedule_t	slKophyaegerRangeAttack1[] =
{
	{ 
		tlKophyaegerRangeAttack1,
		ARRAYSIZE ( tlKophyaegerRangeAttack1 ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"KophyaegerRangeAttack1"
	},
};


//=========================================================
// kophyaeger walks to something tasty and eats it.
//=========================================================

Task_t tlKophyaegerEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_DONT_EAT,				(float)10				},// this is in case he can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_IDLE			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)30				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slKophyaegerEat[] =
{
	{
		tlKophyaegerEat,
		ARRAYSIZE( tlKophyaegerEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"KophyaegerEat"
	}
};


//=========================================================
// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the squid. This schedule plays a sniff animation before going to the source of food.
//=========================================================

Task_t tlKophyaegerSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_DONT_EAT,				(float)10				},// this is in case the squid can't get to the food
	{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_IDLE			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)30				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slKophyaegerSniffAndEat[] =
{
	{
		tlKophyaegerSniffAndEat,
		ARRAYSIZE( tlKophyaegerSniffAndEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"KophyaegerSniffAndEat"
	}
};


//=========================================================
// Victory Dance (Go up to enemy corpse and eat it)
//=========================================================

Task_t tlKophyaegerVictoryDance[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,(float)0					},
	{ TASK_WALK_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_IDLE				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT				},
	{ TASK_EAT,						(float)30					},
	{ TASK_WAIT,					(float)0					},
};

Schedule_t slKophyaegerVictoryDance[] =
{
	{
		tlKophyaegerVictoryDance,
		ARRAYSIZE( tlKophyaegerVictoryDance ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY,
		0,
		"Kophyaeger Victory Dance"
	},
};


DEFINE_CUSTOM_SCHEDULES( CKophyaeger )
{
	slKophyaegerRangeAttack1,
	slKophyaegerEat,
	slKophyaegerSniffAndEat,
	slKophyaegerVictoryDance,
};

IMPLEMENT_CUSTOM_SCHEDULES( CKophyaeger, CSquadMonster );


//=========================================================
// GetScheduleOfType
//=========================================================

Schedule_t* CKophyaeger :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slKophyaegerRangeAttack1[ 0 ];
		}
		break;

		case SCHED_KOPHYAEGER_EAT:
		{
			return &slKophyaegerEat[ 0 ];
		}
		break;

		case SCHED_KOPHYAEGER_SNIFF_AND_EAT:
		{
			return &slKophyaegerSniffAndEat[ 0 ];
		}
		break;

		case SCHED_VICTORY_DANCE:
		{
			return &slKophyaegerVictoryDance[ 0 ];
		}
		break;
	}

	return CSquadMonster::GetScheduleOfType( Type );
}


//=========================================================
// IRelationship - over-ridden for adult so I don't attack other 
// members of my squad
//=========================================================

int CAdultKophyaeger::IRelationship( CBaseEntity *pTarget )
{
	CSquadMonster *pSquadLeader = MySquadLeader( );
	if ( !pSquadLeader ) return CKophyaeger::IRelationship( pTarget );;

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
		if ( pMember == pTarget ) return R_AL;
	}

	return CKophyaeger::IRelationship( pTarget );
}


