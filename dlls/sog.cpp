//=========================================================
//
//		SOG guy programmed for Nam by Nathan Ruck
//
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
#include	"decals.h"

//=====================
// BodyGroups
//=====================

enum 
{
	SOG_BODYGROUP_HEAD = 0,
	SOG_BODYGROUP_BODY,
	SOG_BODYGROUP_WEAPON,
};

enum
{
	SOG_BODY_WEAPON_M21 = 0,
	SOG_BODY_WEAPON_NONE,
};

#define NUM_HEADS 5


//=========================================================
// monster-specific schedule types
//=========================================================

enum
{
	SCHED_SOG_FREEZE = LAST_HUMAN_SCHEDULE + 1,
	SCHED_SOG_FADEOUT,
	SCHED_SOG_FADEIN,
	SCHED_SOG_FADEIN_SCRIPT,
	SCHED_SOG_FADEIN_COWER,
	SCHED_SOG_FADEIN_HIT_AND_RUN,
	SCHED_SOG_FADEIN_GRENADE_AND_RUN,
	SCHED_SOG_FADEIN_TAKE_COVER_FROM_ENEMY,
	SCHED_SOG_FADEIN_TAKE_COVER_FROM_BEST_SOUND,
	SCHED_SOG_FADEIN_TAKE_COVER_FROM_ORIGIN,
	SCHED_SOG_FADEIN_ESTABLISH_AMBUSH_POSITION,
	SCHED_SOG_ESTABLISH_AMBUSH_POSITION,
};


//=========================================================
// monster-specific tasks
//=========================================================

enum 
{
	TASK_SOG_FADE_IN = LAST_HUMAN_TASK + 1,	// Become visible
	TASK_SOG_FADE_OUT,						// Become invisible
};


#define bits_COND_SOG_CAN_AMBUSH	( bits_COND_SPECIAL2 )
#define bits_MEMORY_SOG_INVISIBLE   ( bits_MEMORY_CUSTOM1 )

#define SOG_MIN_AMBUSH_RANGE 384
#define SOG_DIST_TOO_FAR 2048.0


class CSOG : public CHuman
{
public:
	void Spawn( );
	void Precache();
	int Classify( void );
	void ApplyDefaultSettings( void );

	Schedule_t *GetSchedule ( void );
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetScheduleFromSquadCommand ( void );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void SquadReceiveCommand( SquadCommand Cmd );

	void PlayLabelledSentence( const char *pszSentence );
	void Touch( CBaseEntity *pOther );
	int CheckEnemy ( CBaseEntity *pEnemy );

	int GetWeaponNum( int bodygroup );
	int GetWeaponBodyGroup( int weapon );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/soghead.mdl"; };

	BOOL HasHumanGibs() { return FALSE; };
	BOOL HasAlienGibs() { return FALSE; };
	void GibMonster();

	int GetHeadGroupNum( ) { return SOG_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return SOG_BODYGROUP_BODY; };
	int GetWeaponGroupNum( ) { return SOG_BODYGROUP_WEAPON; };

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_sog, CSOG );


//=========================================================
// Precache
//=========================================================

void CSOG :: Precache()
{
    PRECACHE_MODEL("models/SOG.mdl");
    PRECACHE_MODEL("models/soggibs.mdl");
	
	PRECACHE_SOUND("sog/fadeout.wav");
	PRECACHE_SOUND("sog/fadein.wav");

	PRECACHE_SOUND("sog/step1.wav");
	PRECACHE_SOUND("sog/step2.wav");
	PRECACHE_SOUND("sog/step3.wav");
	PRECACHE_SOUND("sog/step4.wav");

	m_szFriends[0] = "monster_sog";
	m_nNumFriendTypes = 2;

	strcpy( m_szSpeechLabel, "HG_");

	CHuman::Precache();
}    


//=========================================================
// Spawn
//=========================================================

void CSOG::Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/SOG.mdl");

	pev->health			= gSkillData.SOGHealth;
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_flDistTooFar		= SOG_DIST_TOO_FAR;
	pev->renderamt		= 255;

	if ( pev->body == -1 )
	{
		m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	}
	else 
	{
		m_nHeadNum = pev->body;
	}

	pev->body = 0;
	SetBodygroup( GetTorsoGroupNum(), 0 );

	CHuman::Spawn();

	m_bloodColor = BLOOD_COLOR_GREEN;
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CSOG::ApplyDefaultSettings( void )
{
	m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	SetBodygroup( GetTorsoGroupNum(), 0 );

	pev->weapons = SOG_BODY_WEAPON_M21;
	m_fHandGrenades = RANDOM_LONG(0, 1);

	CHuman::ApplyDefaultSettings();
}


//=========================================================
// GetWeaponNum - return body group index for weapon
//=========================================================

int CSOG :: GetWeaponBodyGroup( int weapon )
{
	switch( weapon )
	{
	case HUMAN_WEAPON_M21: return SOG_BODY_WEAPON_M21; break;
	case HUMAN_WEAPON_NONE: return SOG_BODY_WEAPON_NONE; break;
	}

	return SOG_BODY_WEAPON_M21;
}


//=========================================================
// GetWeaponNum - return body group index for weapon
//=========================================================

int CSOG :: GetWeaponNum( int weapon )
{
	switch( weapon )
	{
	case SOG_BODY_WEAPON_M21: return HUMAN_WEAPON_M21; break;
	case SOG_BODY_WEAPON_NONE: return HUMAN_WEAPON_NONE; break;
	}

	return HUMAN_WEAPON_M21;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================

int	CSOG :: Classify ( void )
{
	return CLASS_ALIEN_MILITARY;
}


//=========================================================
// PlayLabelledSentence - Over-ridden because SOG don't
// say anything except making aargh noises when he's hit
// or dies
//=========================================================

void CSOG::PlayLabelledSentence( const char *pszSentence )
{
/*	if ( strcmp( pszSentence, "PAIN") || strcmp( pszSentence, "DIE" ) )
	{
		char szSentence[32];
		strcpy( szSentence, m_szSpeechLabel );
		strcat( szSentence, pszSentence );

		PlaySentence( szSentence, duration, volume, attenuation );
	}*/

	CHuman::PlayLabelledSentence( pszSentence );
}


//=========================================================
// GibMonster - SOG has own gib model!
//=========================================================

void CSOG :: GibMonster( void )
{
	TraceResult	tr;

	if ( CVAR_GET_FLOAT("violence_hgibs") != 0 )	// Only the player will ever get here
	{
		if ( m_nHeadNum != GetNumHeads() ) CGib::SpawnHeadGib( pev );
		CGib::SpawnRandomGibs( pev, SOG_GIB_COUNT, GIB_SOG );	// throw some SOG gibs.

		TraceResult	tr;
		UTIL_TraceLine ( pev->origin, pev->origin + Vector(0, 0, -64),  ignore_monsters, ENT(pev), & tr);
		UTIL_DecalTrace( &tr, DECAL_YBIGBLOOD1 + RANDOM_LONG(0,1) );
	}
	
	CHuman::GibMonster();
}


//=========================================================
// Touch
//=========================================================

void CSOG :: Touch( CBaseEntity *pOther )
{
	// Uh-oh, someone I don't like touched me... cooties...

	if ( IRelationship( pOther ) >= R_DL )
	{
		SetConditions( bits_COND_PUSHED );
	}
}


//=========================================================
// CheckEnemy - part of the Condition collection process,
// gets and stores data and conditions pertaining to a monster's
// enemy. Returns TRUE if Enemy LKP was updated.
//
// Over-ridden because SOG needs to check if the enemy is
// in a suitable position to be ambushed
//=========================================================

int CSOG :: CheckEnemy ( CBaseEntity *pEnemy )
{
	int ret = CHuman::CheckEnemy( pEnemy );

	ClearConditions( bits_COND_SOG_CAN_AMBUSH );

	if ( HasMemory( bits_MEMORY_SOG_INVISIBLE ) )
	{
		Vector vecEnemyPos = pEnemy->pev->origin;
		float flDistToEnemy = ( vecEnemyPos - pev->origin ).Length(); 	// distance to enemy's origin

		if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_ENEMY_FACING_ME ) &&
			( flDistToEnemy > SOG_MIN_AMBUSH_RANGE ) )
		{
			SetConditions( bits_COND_SOG_CAN_AMBUSH );
		}
	}

	return ret;
}
	
	
//=========================================================
// StartTask
//=========================================================

void CSOG :: StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SOG_FADE_IN:
		{
			if (pev->renderamt == 255)
			{
				TaskComplete();
			}
			else
			{
				EMIT_SOUND (ENT(pev), CHAN_BODY, "sog/fadein.wav", 1, ATTN_NORM );
			}
		}
		break;

	case TASK_SOG_FADE_OUT:
		{
			if (pev->renderamt == 0)
			{
				TaskComplete();
			}
			else
			{
				EMIT_SOUND (ENT(pev), CHAN_BODY, "sog/fadeout.wav", 1, ATTN_NORM );
				pev->rendermode = kRenderTransTexture;
			}
		}
		break;

	default:
		{
			CHuman::StartTask( pTask );
			return;
		}
		break;
	}
}


//=========================================================
// RunTask
//=========================================================

void CSOG :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SOG_FADE_IN:
		{
			if (pev->renderamt < 255)
			{
				pev->renderamt = min( pev->renderamt + 50, 255 );
			}
			else
			{
				pev->rendermode = kRenderNormal;
				TaskComplete();
			}
		}
		break;

	case TASK_SOG_FADE_OUT:
		{
			if (pev->renderamt > 0)
			{
				pev->renderamt = max( pev->renderamt - 10, 0 );
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	default:
		{
			CHuman::RunTask( pTask );
			return;
		}
		break;
	}
}


//=========================================================
// SquadReceiveCommand - over-ridden because sometimes we
// need to fade in to execute a command
//=========================================================

void CSOG :: SquadReceiveCommand( SquadCommand Cmd )
{
	if ( !IsAlive() || pev->deadflag == DEAD_DYING ) return;
	m_fSquadCmdAcknowledged = FALSE;

	switch ( Cmd )
	{
	case SQUADCMD_RETREAT:				// Run away from whatever you are attacking
		m_nLastSquadCommand = Cmd;
		m_flLastSquadCmdTime = gpGlobals->time;
		// Change schedule immediately as this could be important
	
		if ( SafeToChangeSchedule() )
		{
			if ( HasMemory( bits_MEMORY_SOG_INVISIBLE ) )
			{
				ChangeSchedule( GetScheduleOfType( SCHED_SOG_FADEIN_TAKE_COVER_FROM_ENEMY ) );	
			}
			else
			{
				ChangeSchedule( GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY ) );	
			}
		}
		break;

	default:
		CHuman::SquadReceiveCommand( Cmd );
		break;
	}
}


//=========================================================
// GetScheduleFromSquadCommand - Over-ridden because if we
// are invisible we need to over-ride these
//=========================================================

Schedule_t *CSOG :: GetScheduleFromSquadCommand ( void )
{
	if ( !IsAlive() || pev->deadflag == DEAD_DYING ) return NULL;

	switch ( m_nLastSquadCommand )
	{
	case SQUADCMD_DEFENSE:
		{
			if ( HasMemory(bits_MEMORY_SOG_INVISIBLE) )
			{
				if ( SquadGetWounded() )
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN );	// Fade in, so we can go to defense
				}
				else
				{
					// There is no wounded, so defense was successful.  Yay!  Or he could have died, of course.
					m_nLastSquadCommand = SQUADCMD_NONE;
					if ( IsFollowingHuman() ) StopFollowing( FALSE );
				}
			}
			else
			{
				return CHuman::GetScheduleFromSquadCommand();
			}
		}
		break;

	case SQUADCMD_SURPRESSING_FIRE:
	case SQUADCMD_ATTACK:
		{
			if ( HasMemory(bits_MEMORY_SOG_INVISIBLE) )
			{
				if ( ( m_hEnemy == NULL || !m_hEnemy->IsAlive() ) && !SquadGetCommanderEnemy() ) 
				{
					m_nLastSquadCommand = SQUADCMD_NONE;
				}
				else
				{
					if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HUMAN_ENGAGE ) )
					{
						return GetScheduleOfType( SCHED_SOG_FADEIN_HIT_AND_RUN );	// Try normal attack
					}
					else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
					{
						return GetScheduleOfType( SCHED_SOG_FADEIN_GRENADE_AND_RUN ); // Try grenade attack
					}
				}
			}
			else
			{
				return CHuman::GetScheduleFromSquadCommand();
			}
		}
		break;

	case SQUADCMD_SEARCH_AND_DESTROY:
		{
			if ( HasMemory(bits_MEMORY_SOG_INVISIBLE) )
			{
				if ( m_hEnemy != NULL && m_hEnemy->IsAlive() )	
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN_ESTABLISH_AMBUSH_POSITION );
				}
				else
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN );
				}
			}
			else
			{
				return CHuman::GetScheduleFromSquadCommand();
			}
		}
		break;
	
	case SQUADCMD_COME_HERE:
		{
			if ( HasMemory(bits_MEMORY_SOG_INVISIBLE) )
			{
				return GetScheduleOfType( SCHED_SOG_FADEIN );
			}
			else
			{
				return CHuman::GetScheduleFromSquadCommand();
			}
		}
		break;

	default:
		{
			if ( HasMemory(bits_MEMORY_SOG_INVISIBLE) )
			{
				return NULL;
			}
			else
			{
				return CHuman::GetScheduleFromSquadCommand();
			}
		}
		break;
	}

	return NULL;
}



//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================

Schedule_t *CSOG :: GetSchedule ( void )
{
	// Flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 

	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		return CHuman::GetSchedule();
	}


	// Humans place HIGH priority on running away from danger sounds.

	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				if ( HasMemory( bits_MEMORY_SOG_INVISIBLE ) )
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN_TAKE_COVER_FROM_BEST_SOUND );
				}
				else
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
				}
			}
		}
	}


	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// Invisible

			if ( HasMemory( bits_MEMORY_SOG_INVISIBLE ) )
			{
				Schedule_t * pSchedule = GetScheduleFromSquadCommand();
				if ( pSchedule != NULL )	// Important squad command to carry out
				{	
					return pSchedule;
				}
				else if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )	// Damaged?  Run away
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN_TAKE_COVER_FROM_ENEMY );
				}
				else if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )	// No Ammo ? Fade in and deal with it then
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN );
				}
				else if ( HasConditions( bits_COND_SOG_CAN_AMBUSH ) )	// Enemy is facing away from me and not too close?
				{
					if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HUMAN_ENGAGE ) )
					{
						return GetScheduleOfType( SCHED_SOG_FADEIN_HIT_AND_RUN );	// Try normal attack
					}
					else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
					{
						return GetScheduleOfType( SCHED_SOG_FADEIN_GRENADE_AND_RUN ); // Try grenade attack
					}
					else
					{
						return GetScheduleOfType( SCHED_SOG_FADEIN );	// Just fade in and deal with it then
					}
				}
				else if ( HasConditions( bits_COND_ENEMY_TOOFAR ) )	// Enemy has gotten a long way away
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN_ESTABLISH_AMBUSH_POSITION ); // Get closer but still in cover
				}
				else if ( HasConditions( bits_COND_PUSHED ) )	// Enemy touched me
				{
					ClearConditions( bits_COND_PUSHED );
					return GetScheduleOfType( SCHED_SOG_FADEIN );	// Fade in and deal with it
				}
				else
				{
					return GetScheduleOfType( SCHED_SOG_FADEOUT );
				}
			}

			// Visible
			
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NO_AMMO_LOADED
				| bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY ) )	// These conditions take precedence over enemy occluded
			{
				return CHuman::GetSchedule();
			}

			else
			{
				if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					Schedule_t * pSchedule = GetScheduleFromSquadCommand();
					if ( pSchedule != NULL ) 
					{
						return pSchedule;
					}
					else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
					{
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					}
					else if ( HasConditions( bits_COND_ENEMY_TOOFAR ) )
					{
						return GetScheduleOfType( SCHED_SOG_ESTABLISH_AMBUSH_POSITION ); // Get closer but still in cover
					}
					else
					{
						return GetScheduleOfType( SCHED_SOG_FADEOUT );
					}
				}
			}
		}
		break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		{
			// Invisible

			if ( HasMemory( bits_MEMORY_SOG_INVISIBLE ) )
			{
				if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )	// Damaged?  Run away
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN_TAKE_COVER_FROM_ORIGIN );
				}
				else if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )	// No Ammo ? Fade in and deal with it then
				{
					return GetScheduleOfType( SCHED_SOG_FADEIN );
				}
				else if ( HasConditions( bits_COND_PUSHED ) )	// Enemy touched me
				{
					ClearConditions( bits_COND_PUSHED );
					return GetScheduleOfType( SCHED_SOG_FADEIN_TAKE_COVER_FROM_ORIGIN ); // Fade in and run away
				}
				else
				{
					if ( HasConditions( bits_COND_ENEMY_TOOFAR ) ) // If I have no enemy then he can't be too far away
					{
						ClearConditions( bits_COND_ENEMY_TOOFAR );
					}

					return GetScheduleOfType( SCHED_SOG_FADEOUT );
				}
			}
			else if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
			{
				return GetScheduleOfType( SCHED_RELOAD );
			}
			else
			{
				return GetScheduleOfType( SCHED_SOG_FADEOUT );
			}
		}
		break;

	}

	return CHuman::GetSchedule();
}


//=========================================================
// Freeze - stay rooted to the spot
//=========================================================

Task_t tlSOGFreeze[] =
{
	{ TASK_WAIT_INDEFINITE,				(float)0						},

};

Schedule_t slSOGFreeze[] =
{
	{
		tlSOGFreeze,
		ARRAYSIZE( tlSOGFreeze ),
		bits_COND_NEW_ENEMY			|		// Have to include this otherwise it won't be able to see new enemies
		bits_COND_SOG_CAN_AMBUSH	|		// If enemy is facing other direction, ambush him
		bits_COND_PUSHED			|		// If enemy touches me, my cover is blown
		bits_COND_ENEMY_TOOFAR		|		// If too far away, move to a closer position
		bits_COND_NO_AMMO_LOADED	|		// If no ammo loaded, we will want to reload
		bits_COND_LIGHT_DAMAGE		|		// If I'm hit, break cover
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,					// If there's a grenade nearby we need to run away
		"SOG Freeze"
	},
};


//=========================================================
// FadeOut - Become invisible
//=========================================================

Task_t tlSOGFadeOut[] =
{
	{ TASK_STOP_MOVING,					(float)0						},
	{ TASK_SET_ACTIVITY,				(float)ACT_IDLE					},
	{ TASK_SOG_FADE_OUT,				(float)0						},
	{ TASK_REMEMBER,					(float)bits_MEMORY_SOG_INVISIBLE},
	{ TASK_SET_SCHEDULE,				(float)SCHED_SOG_FREEZE			},
};

Schedule_t slSOGFadeOut[] =
{
	{
		tlSOGFadeOut,
		ARRAYSIZE( tlSOGFadeOut ),
		0,		
		0,
		"SOG Fade Out"
	},
};


//=========================================================
// FadeIn - Become visible
//=========================================================

Task_t tlSOGFadeIn[] =
{
	{ TASK_STOP_MOVING,					(float)0						},
	{ TASK_SOG_FADE_IN,					(float)0						},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE},
};

Schedule_t slSOGFadeIn[] =
{
	{
		tlSOGFadeIn,
		ARRAYSIZE( tlSOGFadeIn ),
		0,		
		0,
		"SOG Fade In"
	},
};


//=========================================================
// FadeInHitAndRun - Become visible, shoot enemy, then take
// cover
//=========================================================

Task_t tlSOGFadeInHitAndRun[] =
{
	{ TASK_STOP_MOVING,					(float)0							},
	{ TASK_SOG_FADE_IN,					(float)0							},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE	},
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_TAKE_COVER_FROM_ENEMY	},
	{ TASK_FACE_ENEMY,					(float)0							},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0							},
	{ TASK_RANGE_ATTACK1,				(float)0							},
	{ TASK_FACE_ENEMY,					(float)0							},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0							},
	{ TASK_RANGE_ATTACK1,				(float)0							},
	{ TASK_SET_SCHEDULE,				(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t slSOGFadeInHitAndRun[] =
{
	{
		tlSOGFadeInHitAndRun,
		ARRAYSIZE( tlSOGFadeInHitAndRun ),
		bits_COND_NO_AMMO_LOADED	|		// If no ammo loaded, we will want to reload
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,					// If there's a grenade nearby we need to run away
		"SOG Fade In Hit And Run"
	},
};


//=========================================================
// FadeInGrenadeAndRun - Become visible, throw grenade, then take
// cover
//=========================================================

Task_t tlSOGFadeInGrenadeAndRun[] =
{
	{ TASK_SOG_FADE_IN,					(float)0							},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE	},
	{ TASK_HUMAN_FACE_TOSS_DIR,			(float)0							},
	{ TASK_RANGE_ATTACK2,				(float)0							},
	{ TASK_SET_SCHEDULE,				(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t slSOGFadeInGrenadeAndRun[] =
{
	{
		tlSOGFadeInGrenadeAndRun,
		ARRAYSIZE( tlSOGFadeInGrenadeAndRun ),
		0,
		0,
		"SOG Fade In Grenade And Run"
	},
};


//=========================================================
// FadeInTakeCoverFromEnemy - Become visible, take cover, if you can't find cover then attack
//=========================================================

Task_t tlSOGFadeInTakeCoverFromEnemy[] =
{
	{ TASK_SOG_FADE_IN,					(float)0							},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE	},
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_SOG_FADEIN_HIT_AND_RUN	},	// Normal fail schedule would just make
	{ TASK_FIND_COVER_FROM_ENEMY,		(float)0							},	// me fade out again - we want to attack
	{ TASK_HUMAN_SOUND_COVER,			(float)0.5							},	// if cover cannot be found
	{ TASK_RUN_PATH,					(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0							},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER			},
	{ TASK_TURN_LEFT,					(float)179							},
};

Schedule_t slSOGFadeInTakeCoverFromEnemy[] =
{
	{
		tlSOGFadeInTakeCoverFromEnemy,
		ARRAYSIZE( tlSOGFadeInTakeCoverFromEnemy ),
		0,		
		0,
		"SOG Fade In Take Cover From Enemy"
	},
};


//=========================================================
// FadeInScript - Become visible, do AI script
//=========================================================

Task_t tlSOGFadeInScript[] =
{
	{ TASK_SOG_FADE_IN,					(float)0							},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE	},
	{ TASK_SET_SCHEDULE,				(float)SCHED_AISCRIPT				},
};

Schedule_t slSOGFadeInScript[] =
{
	{
		tlSOGFadeInScript,
		ARRAYSIZE( tlSOGFadeInScript ),
		0,		
		0,
		"SOG Fade In Script"
	},
};


//=========================================================
// FadeInCower - Become visible, cower
//=========================================================

Task_t tlSOGFadeInCower[] =
{
	{ TASK_SOG_FADE_IN,					(float)0							},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE	},
	{ TASK_SET_SCHEDULE,				(float)SCHED_COWER					},
};

Schedule_t slSOGFadeInCower[] =
{
	{
		tlSOGFadeInCower,
		ARRAYSIZE( tlSOGFadeInCower ),
		0,		
		0,
		"SOG Fade In Cower"
	},
};


//=========================================================
// FadeInTakeCoverFromBestSound - Become visible, take cover
//=========================================================

Task_t tlSOGFadeInTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_SOG_FADEIN_COWER	},// fade in, duck and cover if cannot move from explosion
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0						},// Have to do this first before sound disappears
	{ TASK_SOG_FADE_IN,					(float)0						},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE	},
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_HUMAN_SOUND_GRENADE,			(float)0.5					},
	{ TASK_HUMAN_CROUCH,				(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t slSOGFadeInTakeCoverFromBestSound[] =
{
	{
		tlSOGFadeInTakeCoverFromBestSound,
		ARRAYSIZE( tlSOGFadeInTakeCoverFromBestSound ),
		0,		
		0,
		"SOG Fade In Take Cover From BestSound"
	},
};


//=========================================================
// FadeInTakeCoverFromOrigin - Become visible, take cover
//=========================================================

Task_t tlSOGFadeInTakeCoverFromOrigin[] =
{
	{ TASK_SOG_FADE_IN,					(float)0							},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE	},
	{ TASK_SET_SCHEDULE,				(float)SCHED_TAKE_COVER_FROM_ORIGIN	},
};

Schedule_t slSOGFadeInTakeCoverFromOrigin[] =
{
	{
		tlSOGFadeInTakeCoverFromOrigin,
		ARRAYSIZE( tlSOGFadeInTakeCoverFromOrigin ),
		0,		
		0,
		"SOG Fade In Take Cover From Origin"
	},
};


//=========================================================
// FadeInEstablishAmbushPosition - Become visible, establish
// ambush position
//=========================================================

Task_t tlSOGFadeInEstablishAmbushPosition[] =
{
	{ TASK_SOG_FADE_IN,					(float)0							},
	{ TASK_FORGET,						(float)bits_MEMORY_SOG_INVISIBLE	},
	{ TASK_SET_SCHEDULE,				(float)SCHED_SOG_ESTABLISH_AMBUSH_POSITION	},
};

Schedule_t slSOGFadeInEstablishAmbushPosition[] =
{
	{
		tlSOGFadeInEstablishAmbushPosition,
		ARRAYSIZE( tlSOGFadeInEstablishAmbushPosition ),
		0,		
		0,
		"SOG Fade In Establish Ambush Position"
	},
};


//=========================================================
// Establish ambush postion - move to a position that allows
// the SOG to ambush the enemy.
//=========================================================

Task_t tlSOGEstablishAmbushPosition[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_HUMAN_ESTABLISH_LINE_OF_FIRE	}, // If we can't ambush, try attacking normally
	{ TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY,	(float)SOG_DIST_TOO_FAR			},
	{ TASK_RUN_PATH,						(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0						},
	{ TASK_SET_SCHEDULE,					(float)SCHED_SOG_FADEOUT		},
};

Schedule_t slSOGEstablishAmbushPosition[] =
{
	{ 
		tlSOGEstablishAmbushPosition,
		ARRAYSIZE ( tlSOGEstablishAmbushPosition ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"SOGEstablishAmbushPosition"
	},
};


DEFINE_CUSTOM_SCHEDULES( CSOG )
{
	slSOGFreeze,
	slSOGFadeOut,
	slSOGFadeIn,
	slSOGFadeInScript,
	slSOGFadeInCower,
	slSOGFadeInHitAndRun,
	slSOGFadeInGrenadeAndRun,
	slSOGFadeInTakeCoverFromEnemy,
	slSOGFadeInTakeCoverFromBestSound,
	slSOGFadeInTakeCoverFromOrigin,
	slSOGFadeInEstablishAmbushPosition,
	slSOGEstablishAmbushPosition,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSOG, CHuman );

	
//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CSOG :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_FAIL:		// This means I will freeze if I can't find cover - even if I am in full view
	case SCHED_IDLE_STAND:
	case SCHED_ALERT_STAND:
	case SCHED_STANDOFF:
	case SCHED_SOG_FADEOUT:
		{
			return &slSOGFadeOut[ 0 ];
		}
		break;

	case SCHED_SOG_FREEZE:
		{
			return &slSOGFreeze[ 0 ];
		}
		break;

	case SCHED_SOG_FADEIN:
		{
			return &slSOGFadeIn[ 0 ];
		}
		break;

	case SCHED_SOG_FADEIN_SCRIPT:
		{
			return &slSOGFadeInScript[ 0 ];
		}
		break;

	case SCHED_SOG_FADEIN_COWER:
		{
			return &slSOGFadeInCower[ 0 ];
		}
		break;

	case SCHED_RANGE_ATTACK1:
	case SCHED_SOG_FADEIN_HIT_AND_RUN:
		{
			return &slSOGFadeInHitAndRun[ 0 ];
		}
		break;

	case SCHED_RANGE_ATTACK2:
	case SCHED_SOG_FADEIN_GRENADE_AND_RUN:
		{
			return &slSOGFadeInGrenadeAndRun[ 0 ];
		}
		break;

	case SCHED_SOG_FADEIN_TAKE_COVER_FROM_ENEMY:
		{
			return &slSOGFadeInTakeCoverFromEnemy[ 0 ];
		}
		break;

	case SCHED_SOG_FADEIN_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slSOGFadeInTakeCoverFromBestSound[ 0 ];
		}
		break;

	case SCHED_SOG_FADEIN_TAKE_COVER_FROM_ORIGIN:
		{
			return &slSOGFadeInTakeCoverFromOrigin[ 0 ];
		}
		break;

	case SCHED_SOG_FADEIN_ESTABLISH_AMBUSH_POSITION:
		{
			return &slSOGFadeInEstablishAmbushPosition[ 0 ];
		}
		break;

	case SCHED_SOG_ESTABLISH_AMBUSH_POSITION:
		{
			if ( HasMemory( bits_MEMORY_SOG_INVISIBLE ) ) return &slSOGFadeInEstablishAmbushPosition[ 0 ];

			return &slSOGEstablishAmbushPosition[ 0 ];
		}
		break;

	case SCHED_AISCRIPT:
		{
			if ( HasMemory( bits_MEMORY_SOG_INVISIBLE ) )
			{
				return GetScheduleOfType( SCHED_SOG_FADEIN_SCRIPT );
			}
			else
			{
				return CHuman::GetScheduleOfType( Type );
			}
		}
		break;

	default:
		{
			if ( HasMemory( bits_MEMORY_SOG_INVISIBLE ) ) 
			{
				ALERT( at_console, "Invisible SOG trying to move\n");
				EMIT_SOUND (ENT(pev), CHAN_BODY, "sog/fadein.wav", 1, ATTN_NORM );
				pev->renderamt = 255;
				pev->rendermode = kRenderNormal;			
			}

			return CHuman::GetScheduleOfType( Type );
		}
		break;
	}
}


//=========================================================
// CHARLIE REPEL
//=========================================================

class CSOGRepel : public CHumanRepel
{
public:
	virtual char * EntityName() { return "monster_sog"; };
};

LINK_ENTITY_TO_CLASS( monster_sog_repel, CSOGRepel );



//=========================================================
// DEAD CHARLIE PROP
//=========================================================

class CDeadSOG : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_ALIEN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[7];
};

char *CDeadSOG::m_szPoses[] = {	"lying_on_back",	"lying_on_side",	"lying_on_stomach", 
									"hanging_byfeet",	"hanging_byarms",	"hanging_byneck",
									"deadsitting"	};

void CDeadSOG::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_sog_dead, CDeadSOG );


//=========================================================
// ********** DeadCharlie SPAWN **********
//=========================================================

void CDeadSOG :: Spawn( void )
{
	PRECACHE_MODEL("models/sog.mdl");
	SET_MODEL(ENT(pev), "models/sog.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead sog with bad pose\n" );
	}

	int nHeadNum;
	if ( pev->body == -1 )
	{
		nHeadNum = RANDOM_LONG( 0, NUM_HEADS - 1 ); 
	}
	else 
	{
		nHeadNum = pev->body;
	}

	pev->body = 0;
	SetBodygroup( SOG_BODYGROUP_HEAD, nHeadNum );
	SetBodygroup( SOG_BODYGROUP_BODY, 0 );
	SetBodygroup( SOG_BODYGROUP_WEAPON, SOG_BODY_WEAPON_NONE );


	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();

	if ( m_iPose >=3 || m_iPose <6 ) 
	{
		pev->movetype = MOVETYPE_NONE;
		pev->framerate = 1;
	}
}
