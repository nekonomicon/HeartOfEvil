//=========================================================
//
//		Charlie guy programmed for Nam by Nathan Ruck
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


//=========================================================
// Propaganda Card Class
//=========================================================

class CPropagandaCard : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT FlyThink( void );

	float m_lifeTime;
};


void CPropagandaCard::Spawn( void )
{
	SET_MODEL( ENT(pev), "models/propagandacard.mdl" );
	
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	pev->avelocity = Vector( RANDOM_LONG(150, 200), RANDOM_LONG(150, 200), RANDOM_LONG(150, 200));

	m_lifeTime = 120;

	SetThink ( FlyThink );
	pev->nextthink = gpGlobals->time + 0.1;

	UTIL_SetSize( pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ) );
}


#define TERMINAL_VELOCITY -100

void CPropagandaCard::FlyThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

/*	if ( pev->velocity == g_vecZero )
	{
		SetThink (SUB_StartFadeOut);
		pev->nextthink = gpGlobals->time + m_lifeTime;
	}*/

	if (pev->velocity.z > TERMINAL_VELOCITY ) pev->velocity.z-=10;
	else if (pev->velocity.z < TERMINAL_VELOCITY ) pev->velocity.z+=10;
	
	if (pev->velocity.x > 0 ) pev->velocity.x-=10;
	else if (pev->velocity.x < 0 ) pev->velocity.x+=10;
	
	if (pev->velocity.y > 0 ) pev->velocity.y-=10;
	else if (pev->velocity.y < 0 ) pev->velocity.y+=10;
	
	pev->nextthink = gpGlobals->time + 0.1;

}




//=====================
// BodyGroups
//=====================

enum 
{
	CHARLIE_BODYGROUP_HEAD = 0,
	CHARLIE_BODYGROUP_BODY,
	CHARLIE_BODYGROUP_WEAPON,
};

enum
{
	CHARLIE_BODY_WEAPON_AK47 = 0,
	CHARLIE_BODY_WEAPON_RPG7,
	CHARLIE_BODY_WEAPON_NONE,
};

enum
{
	CHARLIE_BODY_NVA_BROWN = 0,
	CHARLIE_BODY_NVA_GREEN,
	CHARLIE_BODY_VC,
	CHARLIE_BODY_VC2,
};


#define NUM_HEADS 4


//=====================
// Animation Events
//=====================

enum
{
	CHARLIE_AE_DROP_CARD = LAST_HUMAN_ANIM_EVENT + 1, // 11
};


class CCharlie : public CHuman
{
public:
	void Spawn( );
	void Precache();
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	Schedule_t* GetScheduleOfType ( int Type );
	void ApplyDefaultSettings( void );

	int GetWeaponNum( int bodygroup );
	int GetWeaponBodyGroup( int weapon );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/charliehead.mdl"; };
	float GetDuration( const char *pszSentence );

	BOOL HasHumanGibs() { return FALSE; };
	BOOL HasAlienGibs() { return FALSE; };
	void GibMonster();

	int GetHeadGroupNum( ) { return CHARLIE_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return CHARLIE_BODYGROUP_BODY; };
	int GetWeaponGroupNum( ) { return CHARLIE_BODYGROUP_WEAPON; };

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_charlie, CCharlie );


//=========================================================
// Precache
//=========================================================

void CCharlie :: Precache()
{
    PRECACHE_MODEL("models/charlie.mdl");
    PRECACHE_MODEL("models/charliegibs.mdl");
	PRECACHE_MODEL( "models/propagandacard.mdl" );
	
	PRECACHE_SOUND("gook/step1.wav");
	PRECACHE_SOUND("gook/step2.wav");
	PRECACHE_SOUND("gook/step3.wav");
	PRECACHE_SOUND("gook/step4.wav");

	m_szFriends[0] = "monster_charlie";
	m_szFriends[1] = "monster_charlie_medic";
	m_nNumFriendTypes = 2;

	strcpy( m_szSpeechLabel, "CH_");

	CHuman::Precache();
}    


//=========================================================
// Spawn
//=========================================================

void CCharlie::Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/charlie.mdl");

	pev->health			= gSkillData.charlieHealth;
	m_flFieldOfView		= 0.2;

	if ( pev->body == -1 )
	{
		m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	}
	else 
	{
		m_nHeadNum = pev->body;
	}

	pev->body = 0;
	SetBodygroup( GetTorsoGroupNum(), m_nHeadNum );

	CHuman::Spawn();
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CCharlie::ApplyDefaultSettings( void )
{
	m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	SetBodygroup( GetTorsoGroupNum(), m_nHeadNum );

	pev->weapons = CHARLIE_BODY_WEAPON_AK47;
	m_fHandGrenades = RANDOM_LONG(0, 1);

	CHuman::ApplyDefaultSettings();
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================

void CCharlie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case CHARLIE_AE_DROP_CARD:
		{
			CPropagandaCard *pPropagandaCard = GetClassPtr( (CPropagandaCard *)NULL );

			if ( pPropagandaCard )
			{
				pPropagandaCard->Spawn( );
				pPropagandaCard->pev->origin = pev->origin + ( gpGlobals->v_forward * 22 ) + ( gpGlobals->v_up * 48 );
			}
		}
		break;

	default:
		{
			CHuman::HandleAnimEvent( pEvent );
		}
		break;
	}
}


//=========================================================
// GibMonster - HGrunt has own gib model!
//=========================================================

void CCharlie :: GibMonster( void )
{
	TraceResult	tr;

	if ( CVAR_GET_FLOAT("violence_hgibs") != 0 )	// Only the player will ever get here
	{
		if ( m_nHeadNum != GetNumHeads() ) CGib::SpawnHeadGib( pev );
		
		switch ( GetBodygroup( GetTorsoGroupNum() ) )
		{
		case CHARLIE_BODY_NVA_BROWN:
			CGib::SpawnRandomGibs( pev, CHARLIE_GIB_COUNT, GIB_CHARLIE_NVA_BROWN );
			break;
		case CHARLIE_BODY_NVA_GREEN:
			CGib::SpawnRandomGibs( pev, CHARLIE_GIB_COUNT, GIB_CHARLIE_NVA_GREEN );
			break;
		case CHARLIE_BODY_VC:
			CGib::SpawnRandomGibs( pev, CHARLIE_GIB_COUNT, GIB_CHARLIE_VC );
			break;
		case CHARLIE_BODY_VC2:
			CGib::SpawnRandomGibs( pev, CHARLIE_GIB_COUNT, GIB_CHARLIE_VC2 );
			break;
		}

		TraceResult	tr;
		UTIL_TraceLine ( pev->origin, pev->origin + Vector(0, 0, -64),  ignore_monsters, ENT(pev), & tr);
		UTIL_DecalTrace( &tr, DECAL_BIGBLOOD1 + RANDOM_LONG(0,1) );
	}
	
	CHuman::GibMonster();
}


//=========================================================
// Victory Dance
// Over-rides base because we want charlie to drop a
// propaganda card on the enemy corpse
//=========================================================

Task_t tlCharlieVictoryDance[] =
{
	{ TASK_STOP_MOVING,					0							},
	{ TASK_HUMAN_UNCROUCH,				(float)0					},
	{ TASK_HUMAN_SOUND_VICTORY,			(float)0					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,	(float)0					},
	{ TASK_WALK_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FACE_ENEMY,					(float)0					},
	{ TASK_PLAY_SEQUENCE,				(float)ACT_VICTORY_DANCE	},
	{ TASK_SUGGEST_STATE,				(float)MONSTERSTATE_IDLE	},
};

Schedule_t slCharlieVictoryDance[] =
{
	{
		tlCharlieVictoryDance,
		ARRAYSIZE( tlCharlieVictoryDance ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Charlie Victory Dance"
	},
};


DEFINE_CUSTOM_SCHEDULES( CCharlie )
{
	slCharlieVictoryDance,
};

IMPLEMENT_CUSTOM_SCHEDULES( CCharlie, CHuman );



//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CCharlie :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_VICTORY_DANCE:
		{
			return &slCharlieVictoryDance[ 0 ];
		}
		break;
	}
	return CHuman::GetScheduleOfType( Type );
}


//=========================================================
// GetWeaponBodyGroup - return body group index for weapon
//=========================================================

int CCharlie :: GetWeaponBodyGroup( int weapon )
{
	switch( weapon )
	{
	case HUMAN_WEAPON_AK47: return CHARLIE_BODY_WEAPON_AK47; break;
	case HUMAN_WEAPON_RPG7: return CHARLIE_BODY_WEAPON_RPG7; break;
	case HUMAN_WEAPON_NONE: return CHARLIE_BODY_WEAPON_NONE; break;
	}

	return CHARLIE_BODY_WEAPON_AK47;
}


//=========================================================
// GetWeaponNum - return weapon index for bodygroup
//=========================================================

int CCharlie :: GetWeaponNum( int bodygroup )
{
	switch( bodygroup )
	{
	case CHARLIE_BODY_WEAPON_AK47: return HUMAN_WEAPON_AK47; break;
	case CHARLIE_BODY_WEAPON_RPG7: return HUMAN_WEAPON_RPG7; break;
	case CHARLIE_BODY_WEAPON_NONE: return HUMAN_WEAPON_NONE; break;
	}

	return HUMAN_WEAPON_AK47;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================

int	CCharlie :: Classify ( void )
{
	return CLASS_HUMAN_CHARLIE;
}


//=========================================================
// GetDuration - Lengths of sentences
//=========================================================

float CCharlie :: GetDuration( const char *pszSentence )
{
	if ( !strcmp( pszSentence, "TAUNT" ) ) return 5;
	if ( !strcmp( pszSentence, "DEAD" ) ) return 7;
	if ( !strcmp( pszSentence, "HELP" ) ) return 5;
	if ( !strcmp( pszSentence, "MORTAL" ) ) return 6;
	if ( !strcmp( pszSentence, "MEDIC" ) ) return 4;
	if ( !strcmp( pszSentence, "HURTA" ) ) return 5;

	return CHuman::GetDuration( pszSentence );
}


//=========================================================
// CHARLIE REPEL
//=========================================================

class CCharlieRepel : public CHumanRepel
{
public:
	virtual char * EntityName() { return "monster_charlie"; };
};

LINK_ENTITY_TO_CLASS( monster_charlie_repel, CCharlieRepel );



//=========================================================
// DEAD CHARLIE PROP
//=========================================================

class CDeadCharlie : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_CHARLIE; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[8];
};

char *CDeadCharlie::m_szPoses[] = {	"lying_on_back",	"lying_on_side",	"lying_on_stomach", 
									"hanging_byfeet",	"hanging_byarms",	"hanging_byneck",
									"deadsitting",		"deadseated"	};

void CDeadCharlie::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_charlie_dead, CDeadCharlie );


//=========================================================
// ********** DeadCharlie SPAWN **********
//=========================================================

void CDeadCharlie :: Spawn( void )
{
	PRECACHE_MODEL("models/charlie.mdl");
	SET_MODEL(ENT(pev), "models/charlie.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead charlie with bad pose\n" );
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
	SetBodygroup( CHARLIE_BODYGROUP_HEAD, nHeadNum );
	SetBodygroup( CHARLIE_BODYGROUP_BODY, nHeadNum );
	SetBodygroup( CHARLIE_BODYGROUP_WEAPON, CHARLIE_BODY_WEAPON_NONE );


	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();

	if ( m_iPose >=3 || m_iPose <6 ) 
	{
		pev->movetype = MOVETYPE_NONE;
		pev->framerate = 1;
	}
}
