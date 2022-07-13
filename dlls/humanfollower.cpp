//
// Implementation of the CHumanFollower class
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


TYPEDESCRIPTION	CHumanFollower::m_SaveData[] = 
{
	DEFINE_FIELD( CHumanFollower, m_useTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CHumanFollower, CHuman );


void CHumanFollower :: Spawn( void )
{
	m_useTime = gpGlobals->time;
	SetTouch( FollowerTouch );

	CHuman::Spawn();
}


void CHumanFollower :: Precache( void )
{

	CHuman::Precache();
}


//=========================================================
// IsFacing
//=========================================================

BOOL CHumanFollower::IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


//=========================================================
// IdleSound
//=========================================================

void CHumanFollower :: IdleSound()
{
	if ( !FOkToSpeak() ) return;

	if ( FBitSet( pev->spawnflags, SF_MONSTER_PREDISASTER ))
	{
		BOOL Spoke = FALSE;

		// if there is a friend nearby to speak to, play sentence
		CBaseEntity *pFriend = FindNearestFriend( !HasConditions(bits_COND_PROVOKED) );

		if ( pFriend != NULL && pFriend->pev->deadflag == DEAD_NO && FInViewCone(pFriend) && FVisible(pFriend) )
		{
			m_hTalkTarget = pFriend;

			if ( pFriend->IsPlayer() && !FBitSet( m_bitsSaid, bit_saidStare ) && FBitSet( m_bitsSaid, bit_saidIdle ) )
			{
				if ( ( pFriend->pev->origin - pev->origin ).Length2D() < 128 )
				{
					UTIL_MakeVectors( pFriend->pev->angles );
					if ( UTIL_DotPoints( pFriend->pev->origin, pev->origin, gpGlobals->v_forward ) > m_flFieldOfView )
					{
						PlayLabelledSentence( "STARE" ); 
						SetBits( m_bitsSaid, bit_saidStare );
						Spoke = TRUE;
					}
				}
			}
			else if ( !RANDOM_LONG( 0, 4 ) )
			{
				PlayLabelledSentence( "PIDLE" );
				SetBits( m_bitsSaid, bit_saidIdle );
				Spoke = TRUE;
			}
		}

		if ( !Spoke )
		{
			// didn't speak
			m_hTalkTarget = NULL;
		}
	}
	else
	{
		CHuman :: IdleSound();
	}
}

//=========================================================
// TakeDamage - if the player shot me get angry
//=========================================================

int CHumanFollower :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CHuman::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

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
					PlayLabelledSentence( "MAD" );
					m_hTalkTarget = Instance( pevAttacker );
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
				PlayLabelledSentence( "SHOT" );
				m_hTalkTarget = Instance( pevAttacker );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && m_MonsterState != MONSTERSTATE_DEAD )
		{
			PlayLabelledSentence( "SHOT" );
			m_hTalkTarget = Instance( pevAttacker );
			Remember( bits_MEMORY_SUSPICIOUS );
		}
	}

	return ret;
}


//=========================================================
// Killed - called when he's killed
//=========================================================

void CHumanFollower::Killed( entvars_t *pevAttacker, int iGib )
{
	// If a client killed me (unless I was already Barnacle'd), make everyone else mad/afraid of him
	if ( m_MonsterState != MONSTERSTATE_PRONE && pevAttacker && FBitSet(pevAttacker->flags, FL_CLIENT))
	{
		AlertFriends();
	}

	SetUse( NULL );
	CHuman::Killed( pevAttacker, iGib );
}


//=========================================================
// FollowerUse - for when player USEs friendly humans
//=========================================================

void CHumanFollower :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Don't allow use during a scripted_sentence
	if ( m_useTime > gpGlobals->time )
		return;

	if ( pCaller != NULL && pCaller->IsPlayer() )
	{
		// Pre-disaster followers can't be used
		if ( pev->spawnflags & SF_MONSTER_PREDISASTER )
		{
			DeclineFollowing();
		}
		else if ( CanFollow() )
		{
			LimitFollowers( pCaller , MAX_SQUAD_MEMBERS );

			if ( m_afMemory & bits_MEMORY_PROVOKED )
				ALERT( at_console, "I'm not following you, you evil person!\n" );
			else
			{
				StartFollowing( pCaller );
				PlayLabelledSentence( "USE" );
			}
		}
		else
		{
			StopFollowing( TRUE );
		}
	}
}


//=========================================================
// Touched
//=========================================================

void CHumanFollower :: FollowerTouch( CBaseEntity *pOther )
{
	// Did a friendly touch me?
	if ( IRelationship( pOther ) == R_AL )
	{
		// Stay put during speech
		if ( IsTalking() )
			return;

		// Heuristic for determining if the other guy is pushing me away
		float speed = fabs(pOther->pev->velocity.x) + fabs(pOther->pev->velocity.y);
		if ( speed > 50 )
		{
			SetConditions( bits_COND_PUSHED );
			MakeIdealYaw( pOther->pev->origin );
		}
	}
}


//=========================================================
// DeclineFollowing
//=========================================================

void CHumanFollower::DeclineFollowing( void )
{
	if (FOkToShout())
	{
		PlayLabelledSentence( "POK" );
	}
}


//=========================================================
// PlayScriptedSentence - what it says
//=========================================================

void CHumanFollower::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	m_useTime = gpGlobals->time + duration;
	CHuman::PlayScriptedSentence( pszSentence, duration, volume, attenuation, bConcurrent, pListener);
}


//=========================================================
// SquadReceiveCommand - Receives a command and takes an
// appropriate action
//=========================================================

void CHumanFollower :: SquadReceiveCommand( SquadCommand Cmd )
{
	if ( !IsAlive() || pev->deadflag == DEAD_DYING ) return;

	m_fSquadCmdAcknowledged = FALSE;

	switch ( Cmd )
	{
	case SQUADCMD_OUTTA_MY_WAY:			// Bugger off
		
		m_nLastSquadCommand = Cmd;
		m_flLastSquadCmdTime = gpGlobals->time;
		if ( !SafeToChangeSchedule() ) break;

		if ( !HasConditions( bits_COND_SEE_ENEMY ) )	
			// If I'm fighting I'm not going to want to worry about this
		{
			ChangeSchedule( GetScheduleOfType( SCHED_HUMAN_MOVE_AWAY_FOLLOW ) );
		}
		break;

	default:
		{
			CHuman::SquadReceiveCommand( Cmd );
		}
		break;
	}
}


//=========================================================
// PlayerSquadCommand - Called by ClientCommand on 
// recieving a 'talk' message
//=========================================================

#define NUM_PLAYER_FRIENDS 3
void CHumanFollower::PlayerSquadCommand( SquadCommand Cmd )
{
	// Look for humans

	CBaseEntity * pFriend = NULL;
	char *szPlayerFriends[NUM_PLAYER_FRIENDS] = { "monster_mikeforce", "monster_mikeforce_medic", "monster_barney" };
	char *pszFriend;
		
	for (int i=0; i < NUM_PLAYER_FRIENDS; i++)
	{
		pszFriend = szPlayerFriends[ i ];
		while (pFriend = UTIL_FindEntityByClassname( pFriend, pszFriend ))
		{
			CHumanFollower * pHumanFollower = pFriend->MyHumanFollowerPointer();

			if (pHumanFollower && pHumanFollower->IsFollowingPlayer() )
			{
				pHumanFollower->SquadReceiveCommand( Cmd );
			}
		}
	}
}


//=========================================================
// StartTask
//=========================================================

void CHumanFollower :: StartTask( Task_t *pTask )
{

	switch ( pTask->iTask )
	{
	case TASK_HUMAN_SOUND_STOP_SHOOTING:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout())
			{
				PlayLabelledSentence( "NOSHOOT" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_WALK_PATH_FOR_UNITS:
		m_movementActivity = ACT_WALK;
		break;

	case TASK_HUMAN_MOVE_AWAY_PATH:
		{
			Vector dir = pev->angles;
			dir.y = pev->ideal_yaw + 180;
			Vector move;

			UTIL_MakeVectorsPrivate( dir, move, NULL, NULL );
			dir = pev->origin + move * pTask->flData;
			if ( MoveToLocation( ACT_WALK, 2, dir ) )
			{
				TaskComplete();
			}
			else if ( FindCover( pev->origin, pev->view_ofs, 0, CoverRadius() ) )
			{
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + 2;
				TaskComplete();
			}
			else
			{
				// nowhere to go?
				TaskFail();
			}
		}
		break;

	case TASK_HUMAN_FACE_PLAYER:
		// track head to the client for a while.
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		break;

	case TASK_HUMAN_SOUND_OUTWAY:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				!m_fSquadCmdAcknowledged && FOkToShout())
			{
				m_hTalkTarget = MySquadLeader();
				PlayLabelledSentence( "OUTWAY" );
				m_fSquadCmdAcknowledged = TRUE;
			}
			TaskComplete();
		}
		break;

		break;

	default:
		CHuman::StartTask( pTask );
		break;

	}
}


//=========================================================
// RunTask
//=========================================================

void CHumanFollower :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HUMAN_WALK_PATH_FOR_UNITS:
		{
			float distance;

			distance = (m_vecLastPosition - pev->origin).Length2D();

			// Walk path until far enough away
			if ( distance > pTask->flData || MovementIsComplete() )
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
		}
		break;

	case TASK_HUMAN_FACE_PLAYER:
		{
			// Get edict for one player
			edict_t *pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );

			if ( pPlayer )
			{
				MakeIdealYaw ( pPlayer->v.origin );
				ChangeYaw ( pev->yaw_speed );
				IdleHeadTurn( pPlayer->v.origin );
				if ( gpGlobals->time > m_flWaitFinished && FlYawDiff() < 10 )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail();
			}
		}
		break;

	default:
		CHuman::RunTask( pTask );
		break;
	}
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current squad command and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================

Schedule_t *CHumanFollower :: GetScheduleFromSquadCommand ( void )
{
	switch ( m_nLastSquadCommand )
	{
	case SQUADCMD_OUTTA_MY_WAY:
		if ( ( m_hTargetEnt != NULL ) && ( pev->origin - m_hTargetEnt->pev->origin ).Length() < 96 ) 
		{
			return GetScheduleOfType( SCHED_HUMAN_MOVE_AWAY_FOLLOW );
		}
		else
		{
			// I am far enough away from player
			m_nLastSquadCommand = SQUADCMD_NONE;
		}
		break;

	}

	return CHuman::GetScheduleFromSquadCommand();
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================

Schedule_t *CHumanFollower :: GetSchedule ( void )
{
	// Flying? If PRONE, barnacle has me. IF not, it's assumed I am repelling. 

	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		return CHuman::GetSchedule();	// Repelling code is handled in human class
	}

	// Humans place HIGH priority on running away from danger sounds.

	CSound *pSound = NULL;
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		pSound = PBestSound();
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}
	
	switch( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		{
			if ( HasConditions( bits_COND_PUSHED ) )
			{
				if ( IsFollowing() ) 
				{
					return GetScheduleOfType( SCHED_HUMAN_MOVE_AWAY_FOLLOW );
				}
				else
				{
					return GetScheduleOfType( SCHED_HUMAN_MOVE_AWAY );
				}
			}
		}
		break;
	}

	return CHuman::GetSchedule();
}


//=========================================================
// MoveAwayFollow
//=========================================================

Task_t	tlHumanMoveAwayFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_TARGET_FACE },
	{ TASK_HUMAN_SOUND_OUTWAY,		(float)0				},
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_HUMAN_MOVE_AWAY_PATH,	(float)128				},
	{ TASK_HUMAN_WALK_PATH_FOR_UNITS,(float)128				},
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TARGET_FACE },
};

Schedule_t	slHumanMoveAwayFollow[] =
{
	{
		tlHumanMoveAwayFollow,
		ARRAYSIZE ( tlHumanMoveAwayFollow ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"MoveAwayFollow"
	},
};


//=========================================================
// MoveAway
//=========================================================

Task_t	tlHumanMoveAway[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_HUMAN_MOVE_AWAY_FAIL },
	{ TASK_STORE_LASTPOSITION,		(float)0		},
	{ TASK_HUMAN_MOVE_AWAY_PATH,	(float)128		},
	{ TASK_HUMAN_WALK_PATH_FOR_UNITS,(float)128		},
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_HUMAN_FACE_PLAYER,		(float)0.5		},
};

Schedule_t	slHumanMoveAway[] =
{
	{
		tlHumanMoveAway,
		ARRAYSIZE ( tlHumanMoveAway ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"MoveAway"
	},
};


//=========================================================
// MoveAwayFail
//=========================================================

Task_t	tlHumanMoveAwayFail[] =
{
	{ TASK_STOP_MOVING,					(float)0		},
	{ TASK_HUMAN_FORGET_SQUAD_COMMAND,	(float)0		},
	{ TASK_HUMAN_FACE_PLAYER,			(float)0.5		},
};

Schedule_t	slHumanMoveAwayFail[] =
{
	{
		tlHumanMoveAwayFail,
		ARRAYSIZE ( tlHumanMoveAwayFail ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"MoveAwayFail"
	},
};


//=========================================================
// StopShooting - tell player to stop shooting my friends, you bastard
//=========================================================

Task_t	tlHumanIdleStopShooting[] =
{
	{ TASK_HUMAN_SOUND_STOP_SHOOTING,	(float)0		},// tell player to stop shooting friend
};

Schedule_t	slHumanIdleStopShooting[] =
{
	{ 
		tlHumanIdleStopShooting,
		ARRAYSIZE ( tlHumanIdleStopShooting ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Human Idle Stop Shooting"
	},
};


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CHumanFollower :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_HUMAN_MOVE_AWAY:
		{
			return &slHumanMoveAway[ 0 ];
		}
		break;

	case SCHED_HUMAN_MOVE_AWAY_FOLLOW:
		{
			return &slHumanMoveAwayFollow[ 0 ];
		}
		break;

	case SCHED_HUMAN_MOVE_AWAY_FAIL:
		{
			return &slHumanMoveAwayFail[ 0 ];
		}
		break;

	case SCHED_HUMAN_STOP_SHOOTING:
		{
			return &slHumanIdleStopShooting[ 0 ];
		}
		break;
	}

	return CHuman::GetScheduleOfType( Type );
}
		
		
DEFINE_CUSTOM_SCHEDULES( CHumanFollower )
{
	slHumanMoveAway,
	slHumanMoveAwayFollow,
	slHumanMoveAwayFail,
	slHumanIdleStopShooting,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHumanFollower, CHuman );

