//=========================================================
//
//		Mikeforce guy programmed for Nam by Nathan Ruck
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
#include	"decals.h"

//=====================
// BodyGroups
//=====================

enum 
{
	MIKEFORCE_BODYGROUP_HEAD = 0,
	MIKEFORCE_BODYGROUP_TORSO,
	MIKEFORCE_BODYGROUP_ARMS,
	MIKEFORCE_BODYGROUP_LEGS,
	MIKEFORCE_BODYGROUP_WEAPON,
	MIKEFORCE_BODYGROUP_WHISKY,
};

enum
{
	MIKEFORCE_BODY_WEAPON_M16 = 0,
	MIKEFORCE_BODY_WEAPON_870,
	MIKEFORCE_BODY_WEAPON_M60,
	MIKEFORCE_BODY_WEAPON_NONE,
};

#define NUM_HEADS 5

//=====================
// Animation Events
//=====================

enum
{
	MIKEFORCE_AE_PICKUP_M16 = LAST_HUMAN_ANIM_EVENT + 1, // 11
	MIKEFORCE_AE_PICKUP_SHOTGUN, // 12
	MIKEFORCE_AE_PICKUP_M60, // 13
	MIKEFORCE_AE_WHISKY_SHOW, // 14
	MIKEFORCE_AE_WHISKY_HIDE, // 15
	MIKEFORCE_AE_WHISKY_DRINK, // 16
};


class CMikeForce : public CHumanFollower
{
public:
	void Spawn( );
	void Precache();
	int Classify( void ) { return CLASS_PLAYER_ALLY; };
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void StartTask( Task_t *pTask );
	Schedule_t *GetSchedule ( void );
	Schedule_t *GetScheduleOfType ( int Type );
	void SetActivity ( Activity NewActivity );
	void ApplyDefaultSettings( void );

	BOOL HasHumanGibs() { return FALSE; };
	BOOL HasAlienGibs() { return FALSE; };
	void GibMonster();

	void PickUpGun( int gun );
	
	int GetWeaponNum( int bodygroup );
	int GetWeaponBodyGroup( int weapon );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/mikeforcehead.mdl"; };
	float GetDuration( const char *pszSentence );

	int GetHeadGroupNum( ) { return MIKEFORCE_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return MIKEFORCE_BODYGROUP_TORSO; };
	int GetArmsGroupNum( ) { return MIKEFORCE_BODYGROUP_ARMS; };
	int GetLegsGroupNum( ) { return MIKEFORCE_BODYGROUP_LEGS; };
	int GetWeaponGroupNum( ) { return MIKEFORCE_BODYGROUP_WEAPON; };
	int GetWhiskyGroupNum( ) { return MIKEFORCE_BODYGROUP_WHISKY; };

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_mikeforce, CMikeForce );


//=========================================================
// Precache
//=========================================================

void CMikeForce :: Precache()
{
    PRECACHE_MODEL("models/mikeforce.mdl");
    PRECACHE_MODEL("models/mikeforcegibs.mdl");
	
	PRECACHE_SOUND("items/gunpickup1.wav");
	PRECACHE_SOUND("mikeforce/glug1.wav");
	PRECACHE_SOUND("mikeforce/glug2.wav");

	PRECACHE_SOUND("mikeforce/step1.wav");
	PRECACHE_SOUND("mikeforce/step2.wav");
	PRECACHE_SOUND("mikeforce/step3.wav");
	PRECACHE_SOUND("mikeforce/step4.wav");

	m_iWeapon = GetWeaponNum( pev->weapons );

	if ( m_iWeapon == HUMAN_WEAPON_NONE )	// If no weapons precache all possible weapon sounds because we could pick up any one
	{
		// m16
		PRECACHE_SOUND("weapons/m16_burst.wav");
		PRECACHE_SOUND("weapons/m16_clipinsert1.wav");

		// m60
		PRECACHE_SOUND("weapons/m60_fire.wav");
		PRECACHE_SOUND("weapons/m60_reload_full.wav");

		// 870
		PRECACHE_SOUND("weapons/870_buckshot.wav");
		PRECACHE_SOUND("weapons/870_pump.wav");
		PRECACHE_SOUND("weapons/reload1.wav");
		PRECACHE_SOUND("weapons/reload2.wav");
		PRECACHE_SOUND("weapons/reload3.wav");

		m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
		m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl"); //shotgun shell
	}
	
	m_szFriends[0] = "monster_mikeforce";
	m_szFriends[1] = "monster_mikeforce_medic";
	m_szFriends[2] = "monster_barney";
	m_szFriends[3] = "player";
	m_szFriends[4] = "monster_peasant";
	m_nNumFriendTypes = 5;


	strcpy( m_szSpeechLabel, "MF_");

	CHumanFollower::Precache();
}    


//=========================================================
// Spawn
//=========================================================

void CMikeForce::Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/mikeforce.mdl");

	pev->health			= gSkillData.mikeforceHealth;
	m_flFieldOfView		= VIEW_FIELD_WIDE;	// NOTE: we need a wide field of view so npc will notice player and say hello

	if ( pev->body == -1 )
	{
		m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	}
	else 
	{
		m_nHeadNum = pev->body;
	}

	pev->body = 0;
	if ( pev->weapons == MIKEFORCE_BODY_WEAPON_M60 ) 
	{
		SetBodygroup( GetTorsoGroupNum(), 4 );
	}
	else
	{
		SetBodygroup( GetTorsoGroupNum(), RANDOM_LONG( 0, 3 ) );
	}
	SetBodygroup( GetLegsGroupNum(), 0 );
	SetBodygroup( GetArmsGroupNum(), 0 );
	SetBodygroup( GetWhiskyGroupNum(), FALSE );

	CHumanFollower::Spawn();
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CMikeForce::ApplyDefaultSettings( void )
{
	m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 

	switch ( RANDOM_LONG( 0, 6 ) )
	{
	case 0:
	case 1:
	case 2: pev->weapons = MIKEFORCE_BODY_WEAPON_M16; break;
	case 3:
	case 4: pev->weapons = MIKEFORCE_BODY_WEAPON_870; break;
	case 5: pev->weapons = MIKEFORCE_BODY_WEAPON_M60; break;
	}

	m_fHandGrenades = RANDOM_LONG(0, 1);

	if ( pev->weapons == MIKEFORCE_BODY_WEAPON_M60 ) 
	{
		SetBodygroup( GetTorsoGroupNum(), 4 );
	}
	else
	{
		SetBodygroup( GetTorsoGroupNum(), RANDOM_LONG( 0, 3 ) );
	}

	CHumanFollower::ApplyDefaultSettings();
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================

void CMikeForce :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case MIKEFORCE_AE_PICKUP_M16:
		PickUpGun( HUMAN_WEAPON_M16 );
		break;
		
	case MIKEFORCE_AE_PICKUP_SHOTGUN:
		PickUpGun( HUMAN_WEAPON_870 );
		break;

	case MIKEFORCE_AE_PICKUP_M60:
		PickUpGun( HUMAN_WEAPON_M60 );
		SetBodygroup( GetTorsoGroupNum(), 4 );
		break;

	case MIKEFORCE_AE_WHISKY_SHOW:
		SetBodygroup( GetWhiskyGroupNum(), TRUE );
		break;

	case MIKEFORCE_AE_WHISKY_HIDE:
		SetBodygroup( GetWhiskyGroupNum(), FALSE );
		break;

	case MIKEFORCE_AE_WHISKY_DRINK:
		{
			if (RANDOM_LONG(0, 1))
				EMIT_SOUND(ENT(pev), CHAN_BODY, "mikeforce/glug1.wav", 1, ATTN_NORM);
			else
				EMIT_SOUND(ENT(pev), CHAN_BODY, "mikeforce/glug2.wav", 1, ATTN_NORM);
		}
		break;

	default:
		CHumanFollower::HandleAnimEvent( pEvent );
	}
}


//=========================================================
// StartTask - Over-rides base because if he has no gun, 
// mike can't crouch can he?
//=========================================================

void CMikeForce::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HUMAN_CROUCH:
		if ( m_iWeapon == HUMAN_WEAPON_NONE )
		{
			TaskComplete();
		}
		else
		{
			CHumanFollower::StartTask( pTask );
		}
		break;

	case TASK_HUMAN_UNCROUCH:
		if ( m_iWeapon == HUMAN_WEAPON_NONE )
		{
			TaskComplete();
		}
		else
		{
			CHumanFollower::StartTask( pTask );
		}
		break;

	default:
		{
			CHumanFollower::StartTask( pTask );
		}
		break;
	}
}


//=========================================================
// SetActivity - totally overrides basemonster SetActivities
// because of the necessity to pick a crouch animation etc
//=========================================================

void CMikeForce :: SetActivity ( Activity NewActivity )
{
	// Make sure he's not holding a whisky

	if ( NewActivity != ACT_VICTORY_DANCE && GetBodygroup( GetWhiskyGroupNum() ) == TRUE )
	{
		SetBodygroup( GetWhiskyGroupNum(), FALSE );
	}

	if ( m_iWeapon == HUMAN_WEAPON_NONE )
	{
		int	iSequence = ACTIVITY_NOT_AVAILABLE;
		void *pmodel = GET_MODEL_PTR( ENT(pev) );
		char seq[40];

		switch ( NewActivity)
		{
			case ACT_IDLE:
				strcpy( seq, "idle" );
				break;

			case ACT_WALK:
				strcpy( seq, "walk" );
				break;

			case ACT_RUN:
				strcpy( seq, "run" );
				break;

			case ACT_TURN_LEFT:
				strcpy( seq, "turnleft" );
				break;

			case ACT_TURN_RIGHT:
				strcpy( seq, "turnright" );
				break;

			case ACT_FLINCH_LEFTARM:
				strcpy( seq, "laflinch" );
				break;

			case ACT_FLINCH_RIGHTARM:
				strcpy( seq, "raflinch" );
				break;

			case ACT_FLINCH_LEFTLEG:
				strcpy( seq, "llflinch" );
				break;

			case ACT_FLINCH_RIGHTLEG:
				strcpy( seq, "rlflinch" );
				break;

			case ACT_SMALL_FLINCH:
				strcpy( seq, "smlflinch" );
				break;

			case ACT_DIE_HEADSHOT:
				strcpy( seq, "die_headshot" );
				break;

			case ACT_DIE_CHESTSHOT:
				strcpy( seq, "die_chestshot" );
				break;

			case ACT_DIE_GUTSHOT:
				strcpy( seq, "die_gutshot" );
				break;

			case ACT_DIEFORWARD:
				strcpy( seq, "die_forward" );
				break;

			case ACT_DIEBACKWARD:
				strcpy( seq, "die_backward" );
				break;

			case ACT_DIESIMPLE:
				strcpy( seq, "die_simple" );
				break;
		}

		char seq2[40];
		strcpy( seq2, "nogun_" );
		strcat( seq2, seq );
		strcpy( seq, seq2 );

		iSequence = LookupSequence( seq );

		if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			if ( pev->sequence != iSequence || !m_fSequenceLoops )
			{
				pev->frame = 0;
			}

			pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
			ResetSequenceInfo( );
			SetYawSpeed();
			m_Activity = NewActivity; 
		}
		else
		{
			CHumanFollower::SetActivity( NewActivity );
		}

	}
	else
	{
		CHumanFollower::SetActivity( NewActivity );
	}
}


//=========================================================
// GetSchedule - Over-rides base if mike has no gun
//=========================================================

Schedule_t *CMikeForce :: GetSchedule ( void )
{

/*	if ( m_iWeapon == HUMAN_WEAPON_NONE )
	{
		// With no weapons mikeforce is basically just a big wuss who runs frantically around

		if ( HasConditions(bits_COND_HEAR_SOUND) )
		{
			CSound *pSound;
			pSound = PBestSound();
			if ( pSound)
			{	
				if (pSound->m_iType & bits_SOUND_DANGER)
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ORIGIN );
				}
			}
		}

		if ( IsFollowingPlayer() )	// Try and follow the player unless attacked
		{
			if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ORIGIN );
			}
			else
			{
				return GetScheduleOfType( SCHED_TARGET_CHASE );
			}
		}

		// If not following player run away from anything in sight

		if ( HasConditions( bits_COND_SEE_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ORIGIN );
		}

	}*/

	return CHumanFollower::GetSchedule();
}

	
//=========================================================
// PickUpGun - Anim event for episode d prison scripts
//=========================================================

void CMikeForce :: PickUpGun( int gun )
{
	m_iWeapon = gun;
	pev->weapons = GetWeaponBodyGroup( m_iWeapon );
	SetBodygroup( GetWeaponGroupNum(), pev->weapons );
	EMIT_SOUND( ENT(pev), CHAN_ITEM, "items/gunpickup1.wav", 1, ATTN_NORM );
	m_cClipSize	= GetClipSize();
	m_cAmmoLoaded = m_cClipSize;
	m_afCapability |= bits_CAP_RANGE_ATTACK1;
}


//=========================================================
// GetWeaponBodyGroup - return body group index for weapon
//=========================================================

int CMikeForce :: GetWeaponBodyGroup( int weapon )
{
	switch( weapon )
	{
	case HUMAN_WEAPON_M16: return MIKEFORCE_BODY_WEAPON_M16; break;
	case HUMAN_WEAPON_870: return MIKEFORCE_BODY_WEAPON_870; break;
	case HUMAN_WEAPON_M60: return MIKEFORCE_BODY_WEAPON_M60; break;
	case HUMAN_WEAPON_NONE: return MIKEFORCE_BODY_WEAPON_NONE; break;
	}

	//default
	return MIKEFORCE_BODY_WEAPON_M16;
}


//=========================================================
// GetWeaponNum - return weapon index for body group
//=========================================================

int CMikeForce :: GetWeaponNum( int bodygroup )
{
	switch( bodygroup )
	{
	case MIKEFORCE_BODY_WEAPON_M16: return HUMAN_WEAPON_M16; break;
	case MIKEFORCE_BODY_WEAPON_870: return HUMAN_WEAPON_870; break;
	case MIKEFORCE_BODY_WEAPON_M60: return HUMAN_WEAPON_M60; break;
	case MIKEFORCE_BODY_WEAPON_NONE: return HUMAN_WEAPON_NONE; break;
	}

	//default
	return HUMAN_WEAPON_M16;
}


//=========================================================
// GetDuration - Lengths of sentences
//=========================================================

float CMikeForce :: GetDuration( const char *pszSentence )
{
	if ( !strcmp( pszSentence, "CHARGE" ) ) return 4;
	if ( !strcmp( pszSentence, "COVER" ) ) return 5;
	if ( !strcmp( pszSentence, "ZOMBIE" ) ) return 10;
	if ( !strcmp( pszSentence, "TAUNT" ) ) return 7;
	if ( !strcmp( pszSentence, "KILL" ) ) return 11;
	if ( !strcmp( pszSentence, "WOUND" ) ) return 9;
	if ( !strcmp( pszSentence, "MORTAL" ) ) return 8;
	if ( !strcmp( pszSentence, "SMELL" ) ) return 9;
	if ( !strcmp( pszSentence, "HURTA" ) ) return 7;
	if ( !strcmp( pszSentence, "HURTB" ) ) return 4;
	if ( !strcmp( pszSentence, "DEAD" ) ) return 7;
	if ( !strcmp( pszSentence, "STARE" ) ) return 15;
	if ( !strcmp( pszSentence, "QUESTION" ) ) return 9;
	if ( !strcmp( pszSentence, "ANSWER" ) ) return 14;
	if ( !strcmp( pszSentence, "IDLE" ) ) return 10;
	if ( !strcmp( pszSentence, "MEDIC" ) ) return 7;
	if ( !strcmp( pszSentence, "HEAL" ) ) return 5;
	if ( !strcmp( pszSentence, "HEALED" ) ) return 7;
	if ( !strcmp( pszSentence, "HELP" ) ) return 7;
	if ( !strcmp( pszSentence, "SEARCH" ) ) return 7;
	if ( !strcmp( pszSentence, "RETREAT" ) ) return 5;
	if ( !strcmp( pszSentence, "UNUSE" ) ) return 7;
	if ( !strcmp( pszSentence, "PIDLE" ) ) return 14;
	if ( !strcmp( pszSentence, "POK" ) ) return 5;
	if ( !strcmp( pszSentence, "SHOT" ) ) return 16;

	return CHumanFollower::GetDuration( pszSentence );
}


//=========================================================
// GibMonster - Mikeforce has own gib model!
//=========================================================

void CMikeForce :: GibMonster( void )
{
	TraceResult	tr;

	if ( CVAR_GET_FLOAT("violence_hgibs") != 0 )	// Only the player will ever get here
	{
		if ( m_nHeadNum != GetNumHeads() ) CGib::SpawnHeadGib( pev );
		CGib::SpawnRandomGibs( pev, MIKEFORCE_GIB_COUNT, GIB_MIKEFORCE );	// throw some mikeforce gibs.

		TraceResult	tr;
		UTIL_TraceLine ( pev->origin, pev->origin + Vector(0, 0, -64),  ignore_monsters, ENT(pev), & tr);
		UTIL_DecalTrace( &tr, DECAL_BIGBLOOD1 + RANDOM_LONG(0,1) );
	}
	
	CHumanFollower::GibMonster();
}


//=========================================================
// Victory Dance
// Over-rides base because we want mike to drink whisky
//=========================================================

Task_t tlMikeForceVictoryDance[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_HUMAN_UNCROUCH,		(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_VICTORY_DANCE	},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_HUMAN_SOUND_VICTORY,	(float)0					},
	{ TASK_SUGGEST_STATE,		(float)MONSTERSTATE_IDLE	},
};

Schedule_t slMikeForceVictoryDance[] =
{
	{
		tlMikeForceVictoryDance,
		ARRAYSIZE( tlMikeForceVictoryDance ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"MikeForce Victory Dance"
	},
};


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CMikeForce :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_VICTORY_DANCE:
		{
			m_hTalkTarget = NULL;
			SetBoneController( 0, 0 );
			return &slMikeForceVictoryDance[ 0 ];
		}
		break;
	}

	return CHumanFollower::GetScheduleOfType( Type );
}
		
		
DEFINE_CUSTOM_SCHEDULES( CMikeForce )
{
	slMikeForceVictoryDance,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMikeForce, CHumanFollower );


//=========================================================
// MIKEFORCE REPEL
//=========================================================

class CMikeForceRepel : public CHumanRepel
{
public:
	virtual char * EntityName() { return "monster_mikeforce"; };
};

LINK_ENTITY_TO_CLASS( monster_mikeforce_repel, CMikeForceRepel );



//=========================================================
// DEAD MikeForce PROP
//=========================================================

class CDeadMikeForce : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[8];
};

char *CDeadMikeForce::m_szPoses[] = {	"lying_on_back",	"lying_on_side",	"lying_on_stomach", 
										"hanging_byfeet",	"hanging_byarms",	"hanging_byneck",
										"deadsitting",		"deadseated"	};

void CDeadMikeForce::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_mikeforce_dead, CDeadMikeForce );


//=========================================================
// ********** DeadMikeForce SPAWN **********
//=========================================================

void CDeadMikeForce :: Spawn( void )
{
	PRECACHE_MODEL("models/namGrunt.mdl");
	SET_MODEL(ENT(pev), "models/namGrunt.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead MikeForce with bad pose\n" );
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
	SetBodygroup( MIKEFORCE_BODYGROUP_HEAD, nHeadNum );
	SetBodygroup( MIKEFORCE_BODYGROUP_TORSO, nHeadNum );
	SetBodygroup( MIKEFORCE_BODYGROUP_LEGS, 0 );
	SetBodygroup( MIKEFORCE_BODYGROUP_ARMS, 0 );
	SetBodygroup( MIKEFORCE_BODYGROUP_WEAPON, MIKEFORCE_BODY_WEAPON_NONE );
	SetBodygroup( MIKEFORCE_BODYGROUP_WHISKY, FALSE );


	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();

	if ( m_iPose >=3 || m_iPose <6 ) 
	{
		pev->movetype = MOVETYPE_NONE;
		pev->framerate = 1;
	}
}

