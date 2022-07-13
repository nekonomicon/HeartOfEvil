//=========================================================
//
//		Peasant programmed for Nam by Nathan Ruck
//
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"human.h"
#include	"humanfollower.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"soundent.h"
#include	"animation.h"
#include	"weapons.h"

//=====================
// BodyGroups
//=====================

enum 
{
	PEASANT_BODYGROUP_HEAD = 0,
	PEASANT_BODYGROUP_TORSO,
	PEASANT_BODYGROUP_WEAPON,
};

enum
{
	PEASANT_BODY_WEAPON_NONE = 0,
};

#define BODY_RANDOM			-1
#define BODY_RANDOM_MALE	-2
#define BODY_RANDOM_FEMALE	-3

#define NUM_HEADS 6


//=====================
// Animation Events
//=====================

enum
{
	PEASANT_AE_DROPGRENADE = LAST_HUMAN_ANIM_EVENT + 1, // 11
};


//=========================================================
// Custom Schedules
//=========================================================

enum
{
	SCHED_PEASANT_STARTLE = LAST_HUMAN_FOLLOWER_SCHEDULE + 1,
};


//=========================================================
// Custom Tasks
//=========================================================

enum
{
	TASK_PEASANT_SOUND_SCREAM = LAST_HUMAN_FOLLOWER_TASK + 1,
	TASK_PEASANT_SOUND_FEAR, 
	TASK_PEASANT_SOUND_STARTLE,
};


#define PEASANT_FEAR_TIME 15

class CPeasant : public CHumanFollower
{
public:
	void Spawn( );
	void Precache();
	int Classify( void ) { return CLASS_HUMAN_PASSIVE; };
	void ApplyDefaultSettings( void );
	void StartTask( Task_t *pTask );
	BOOL IsScared() { return m_flFearTime > gpGlobals->time - PEASANT_FEAR_TIME; };
	Activity GetStoppedActivity( void );
	Schedule_t *GetSchedule ( void );
	Schedule_t *GetScheduleOfType ( int Type );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void IdleSound ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL InSquad() { return FALSE; };	//HACKHACK No peasant squads for now
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	int GetWeaponNum( int bodygroup );
	int GetWeaponBodyGroup( int weapon );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/peasanthead.mdl"; };

	int GetHeadGroupNum( ) { return PEASANT_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return PEASANT_BODYGROUP_TORSO; };
	int GetWeaponGroupNum( ) { return PEASANT_BODYGROUP_WEAPON; };

	float m_flFearTime;

	// Save functions
	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_peasant, CPeasant );
LINK_ENTITY_TO_CLASS( monster_scientist, CPeasant );


TYPEDESCRIPTION	CPeasant::m_SaveData[] = 
{
	DEFINE_FIELD( CPeasant, m_flFearTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CPeasant, CHumanFollower );


//=========================================================
// Precache
//=========================================================

void CPeasant :: Precache()
{
    PRECACHE_MODEL("models/peasant.mdl");
	
	m_szFriends[0] = "monster_peasant";
	m_szFriends[1] = "monster_mikeforce";
	m_szFriends[2] = "monster_mikeforce_medic";
	m_szFriends[3] = "monster_barney";
	m_szFriends[4] = "player";
	m_nNumFriendTypes = 5;

	strcpy( m_szSpeechLabel, "PS_");

	CHumanFollower::Precache();
}


//=========================================================
// Spawn
//=========================================================

void CPeasant::Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/peasant.mdl");

	pev->health			= gSkillData.scientistHealth;
	m_flFieldOfView		= VIEW_FIELD_WIDE;	// NOTE: we need a wide field of view so npc will notice player and say hello

	if ( pev->body == BODY_RANDOM )
	{
		pev->body = RANDOM_LONG( BODY_RANDOM_FEMALE, BODY_RANDOM_MALE );
	}

	if ( pev->body == BODY_RANDOM_MALE )
	{
		pev->body = 0;
		m_nHeadNum = RANDOM_LONG( 0, 2 );
		SetBodygroup( GetTorsoGroupNum(), RANDOM_LONG( 0, 2 ) );
	}
	else if ( pev->body == BODY_RANDOM_FEMALE )
	{
		pev->body = 0;
		m_nHeadNum = RANDOM_LONG( 3, 5 );
		SetBodygroup( GetTorsoGroupNum(), RANDOM_LONG( 3, 5 ) );
	}

	CHumanFollower::Spawn();
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CPeasant::ApplyDefaultSettings( void )
{
	if ( RANDOM_LONG(0, 1) )
	{
		m_nHeadNum = RANDOM_LONG( 0, 2 );
		SetBodygroup( GetTorsoGroupNum(), RANDOM_LONG( 0, 2 ) );
	}
	else
	{
		m_nHeadNum = RANDOM_LONG( 3, 5 );
		SetBodygroup( GetTorsoGroupNum(), RANDOM_LONG( 3, 5 ) );
	}

	CHumanFollower::ApplyDefaultSettings();
}


//=========================================================
// GetWeaponBodyGroup - return body group index for weapon
//=========================================================

int CPeasant :: GetWeaponBodyGroup( int weapon )
{
	return PEASANT_BODY_WEAPON_NONE;
}


//=========================================================
// GetWeaponNum - return weapon index for body group
//=========================================================

int CPeasant :: GetWeaponNum( int bodygroup )
{
	return HUMAN_WEAPON_NONE;
}


//=========================================================
// HandleAnimEvent
//=========================================================

void CPeasant :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case PEASANT_AE_DROPGRENADE:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 10 - gpGlobals->v_right * -7 + gpGlobals->v_up * 38, Vector(0, 0, 0), 8 );
		}
		break;

	default:
		{
			CHumanFollower::HandleAnimEvent( pEvent );
		}
		break;
	}
}


//=========================================================
// TakeDamage - Over-rides human and humanfollower because
// peasant has no explosion-flying animation and no MAD and
// SHOT speeches
//=========================================================

int CPeasant :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		// If player shoots me try and get friends to talk about it

		CBaseEntity *pFriend = FindNearestFriend( FALSE );

		if (pFriend && pFriend->IsAlive() && pFriend->pev->deadflag != DEAD_DYING )
		{
			// only if not dead or dying!
			CHuman *pHuman = pFriend->MyHumanPointer();
			if ( pHuman && pHuman->SafeToChangeSchedule() )
			{
				pHuman->m_hTalkTarget = Instance( pevAttacker );
				pHuman->ChangeSchedule( GetScheduleOfType( SCHED_HUMAN_STOP_SHOOTING ) );
			}
		}
		
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( HasMemory( bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				if ( m_MonsterState != MONSTERSTATE_DEAD ) 
				{
					Remember( bits_MEMORY_PROVOKED );
					StopFollowing( TRUE );
					m_hEnemy = Instance( pevAttacker );
					m_vecEnemyLKP = pevAttacker->origin;
					SetConditions ( bits_COND_NEW_ENEMY );
				}

				// Anyone who can SEE me is alerted
				AlertFriends();
			}
			else if ( m_MonsterState != MONSTERSTATE_DEAD )
			{
				// Hey, be careful with that
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && m_MonsterState != MONSTERSTATE_DEAD )
		{
			Remember( bits_MEMORY_SUSPICIOUS );
		}
	}

	return ret;
}


//=========================================================
// StartTask - Over-rides base because if he has no gun, 
// he can't crouch can he?
//=========================================================

void CPeasant::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HUMAN_CROUCH:
		{
			TaskComplete();
		}
		break;

	case TASK_HUMAN_UNCROUCH:
		{
			TaskComplete();
		}
		break;

	case TASK_WALK_PATH:
		{
			if ( IsScared() )
			{
				m_movementActivity = ACT_WALK_SCARED;
			}
			else
			{
				m_movementActivity = ACT_WALK;
			}
			TaskComplete();
		}
		break;

	case TASK_RUN_PATH:
		{
			if ( IsScared() )
			{
				m_movementActivity = ACT_RUN_SCARED;
			}
			else
			{
				m_movementActivity = ACT_RUN;
			}
			TaskComplete();
		}
		break;

	case TASK_SOUND_WAKE:
	case TASK_HUMAN_SOUND_GRENADE:
	case TASK_HUMAN_SOUND_HELP:
	case TASK_HUMAN_SOUND_MEDIC:
	case TASK_HUMAN_SOUND_COVER:
	case TASK_HUMAN_SOUND_FOUND_ENEMY:
	case TASK_HUMAN_SOUND_STOP_SHOOTING:
	case TASK_PEASANT_SOUND_SCREAM:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) )//&&
				//FOkToShout())
			{
				m_hTalkTarget = m_hEnemy;
				PlayLabelledSentence( "SCREAM" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_VICTORY:
	case TASK_PEASANT_SOUND_FEAR:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) )//&&
				//FOkToShout())
			{
				m_hTalkTarget = m_hEnemy;
				PlayLabelledSentence( "FEAR" );
			}
			TaskComplete();
		}
		break;

	case TASK_PEASANT_SOUND_STARTLE:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) )//&&
				//FOkToShout())
			{
				m_hTalkTarget = m_hEnemy;
				PlayLabelledSentence( "STARTLE" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_RESPOND:
		{
			if ( !IsScared() && m_nHeadNum < 3 && FOkToSpeak() )	// Male
			{
				PlayLabelledSentence( "MALE" );
			}
			else	// Female
			{
				PlayLabelledSentence( "FEMALE" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_SURPRESS:
	case TASK_HUMAN_SOUND_SURPRESSING:
	case TASK_HUMAN_SOUND_SEARCHING:
	case TASK_HUMAN_SOUND_ATTACK:
	case TASK_HUMAN_SOUND_THROW:
	case TASK_HUMAN_SOUND_RETREAT:
	case TASK_HUMAN_SOUND_SEARCH_AND_DESTROY:
	case TASK_HUMAN_SOUND_COME_TO_ME:
	case TASK_HUMAN_SOUND_CHARGE:
	case TASK_HUMAN_SOUND_TAUNT:
	case TASK_HUMAN_SOUND_HEALED:
		{
			TaskComplete();
		}
		break;

	default:
		CHumanFollower::StartTask( pTask );
		break;
	}
}


//=========================================================
// GetStoppedActivity
//=========================================================

Activity CPeasant::GetStoppedActivity( void )
{ 
	if ( IsScared() ) 
		return ACT_EXCITED;
	
	return CHumanFollower::GetStoppedActivity();
}


//=========================================================
// IdleSound - chat about stuff when idle
//=========================================================

void CPeasant :: IdleSound ( void )
{
	if ( IsScared() || FBitSet( m_bitsSaid, bit_saidIdle ) || RANDOM_LONG(0, 4) || !FOkToSpeak() ) return;

	// if there is a player nearby to speak to, play sentence
	CBaseEntity *pFriend = FindNearestFriend( !HasConditions(bits_COND_PROVOKED) );

	if ( pFriend != NULL && pFriend->IsPlayer() && pFriend->pev->deadflag == DEAD_NO && FVisible(pFriend) )
	{
		m_hTalkTarget = pFriend;

		if ( m_nHeadNum < 3 )	// Male
		{
			PlayLabelledSentence( "MALE" );
		}
		else	// Female
		{
			PlayLabelledSentence( "FEMALE" );
		}
	}
	else
	{
		m_hTalkTarget = NULL;
	}
}


//=========================================================
// FollowerUse - for when player USEs friendly humans
//=========================================================

void CPeasant :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Don't allow use during a scripted_sentence
	if ( m_useTime > gpGlobals->time )
		return;

	if ( !IsMoving() && pCaller != NULL && pCaller->IsPlayer() && !HasConditions( bits_COND_PROVOKED ) && FOkToShout() )
	{
		m_hTalkTarget = pCaller;
		if ( m_nHeadNum < 3 )	// Male
		{
			PlayLabelledSentence( "MALE" );
		}
		else	// Female
		{
			PlayLabelledSentence( "FEMALE" );
		}
	}
}


//=========================================================
// GetSchedule
//=========================================================

Schedule_t *CPeasant :: GetSchedule ( void )
{
	// Humans place HIGH priority on running away from danger sounds.

	CSound *pSound;

	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		pSound = PBestSound();
		if ( pSound)
		{
			if (pSound->m_iType & ( bits_SOUND_DANGER | bits_SOUND_COMBAT ))
			{
				m_flFearTime = gpGlobals->time;
			}
			
			if ( pSound->m_iType & bits_SOUND_DANGER )
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY ) )
			{
				m_flFearTime = gpGlobals->time;
				return CHumanFollower::GetSchedule();
			}
			else if ( !IsScared() )
			{
				m_IdealMonsterState = MONSTERSTATE_ALERT;
			}
		}

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				m_flFearTime = 0;
			}

			if ( pSound && FBitSet( pSound->m_iType, bits_SOUND_COMBAT ) )
			{
				return GetScheduleOfType( SCHED_PEASANT_STARTLE );
			}
		}
	}
	
	return CHumanFollower::GetSchedule();
}


Task_t	tlPeasantStartle[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_PEASANT_SOUND_STARTLE,	(float)0							},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_FEAR_DISPLAY				},
	{ TASK_SET_ACTIVITY,				(float)ACT_IDLE						},
	{ TASK_WAIT,					(float)2.0							},
	{ TASK_WAIT_RANDOM,				(float)1.0							},
};

Schedule_t	slPeasantStartle[] =
{
	{ 
		tlPeasantStartle,
		ARRAYSIZE ( tlPeasantStartle ), 
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"PeasantStartle"
	},
};


Task_t	tlPeasantPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PEASANT_SOUND_SCREAM,			(float)0.3			},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_EXCITED	},	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slPeasantPanic[] =
{
	{
		tlPeasantPanic,
		ARRAYSIZE ( tlPeasantPanic ),
		0,
		0,
		"PeasantPanic"
	},
};


Task_t	tlPeasantCower[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_PEASANT_SOUND_FEAR,		(float)0.3							},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE				},
	{ TASK_WAIT,					(float)2.0							},
	{ TASK_WAIT_RANDOM,				(float)10.0							},
};

Schedule_t	slPeasantCower[] =
{
	{ 
		tlPeasantCower,
		ARRAYSIZE ( tlPeasantCower ), 
		bits_COND_NEW_ENEMY,
		0,
		"PeasantCower"
	},
};


DEFINE_CUSTOM_SCHEDULES( CPeasant )
{
	slPeasantCower,
	slPeasantPanic,
	slPeasantStartle,
};

IMPLEMENT_CUSTOM_SCHEDULES( CPeasant, CHumanFollower );


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CPeasant :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_COWER:
		{
			return &slPeasantCower[ 0 ];
		}
		break;

	case SCHED_STANDOFF:
		{
			return &slPeasantCower[ 0 ];
		}
		break;

	case SCHED_FAIL:
		{
			return &slPeasantPanic[ 0 ];
		}
		break;

	case SCHED_PEASANT_STARTLE:
		{
			return &slPeasantStartle[ 0 ];
		}
		break;

	case SCHED_HUMAN_EXPLOSION_DIE:
		{
			return GetScheduleOfType( SCHED_DIE );
		}
		break;

	case SCHED_VICTORY_DANCE:
		{
			return GetScheduleOfType( SCHED_ALERT_STAND );
		}
		break;

	default:
		{
			return CHumanFollower::GetScheduleOfType( Type );
		}
		break;
	}
}



//=========================================================
// Dead Peasant PROP
//=========================================================
class CDeadPeasant : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue( KeyValueData *pkvd );
	int	m_iPose;// which sequence to display
	static char *m_szPoses[7];
};
char *CDeadPeasant::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadPeasant::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_peasant_dead, CDeadPeasant );

//
// ********** DeadPeasant SPAWN **********
//
void CDeadPeasant :: Spawn( )
{
	PRECACHE_MODEL("models/peasant.mdl");
	SET_MODEL(ENT(pev), "models/peasant.mdl");
	
	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 8;//gSkillData.scientistHealth;
	
	m_bloodColor = BLOOD_COLOR_RED;

	if ( pev->body == BODY_RANDOM )
	{
		pev->body = RANDOM_LONG( BODY_RANDOM_FEMALE, BODY_RANDOM_MALE );
	}

	if ( pev->body == BODY_RANDOM_MALE )
	{
		pev->body = 0;
		SetBodygroup( PEASANT_BODYGROUP_HEAD, RANDOM_LONG( 0, 2 ) );
		SetBodygroup( PEASANT_BODYGROUP_TORSO, RANDOM_LONG( 0, 2 ) );
	}
	else if ( pev->body == BODY_RANDOM_FEMALE )
	{
		pev->body = 0;
		SetBodygroup( PEASANT_BODYGROUP_HEAD, RANDOM_LONG( 3, 5 ) );
		SetBodygroup( PEASANT_BODYGROUP_TORSO, RANDOM_LONG( 3, 5 ) );
	}
	
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead peasant with bad pose\n" );
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


