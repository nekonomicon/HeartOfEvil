//=========================================================
//
//		Barney
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
#include	"bush.h"

#define NUM_LEAFGIBS 10

//=====================
// Animation Events
//=====================

enum
{
	BARNEY_AE_LEAFGIB = LAST_HUMAN_ANIM_EVENT + 1, // 11
};


//=====================
// BodyGroups
//=====================

enum 
{
	BARNEY_BODYGROUP_HEAD = 0,
	BARNEY_BODYGROUP_TORSO,
	BARNEY_BODYGROUP_WEAPON,
};

enum
{
	BARNEY_BODY_WEAPON_COLT1911A1 = 0,
	BARNEY_BODY_WEAPON_NONE,
};

#define NUM_HEADS 1


class CBarney : public CHumanFollower
{
public:
	void Spawn( );
	void Precache();
	int Classify( void ) { return CLASS_PLAYER_ALLY; };
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void ApplyDefaultSettings( void );
	float GetDuration( const char *pszSentence );

	int GetWeaponNum( int bodygroup );
	int GetWeaponBodyGroup( int weapon );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/barneyhead.mdl"; };

	int GetHeadGroupNum( ) { return BARNEY_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return BARNEY_BODYGROUP_TORSO; };
	int GetWeaponGroupNum( ) { return BARNEY_BODYGROUP_WEAPON; };
};

LINK_ENTITY_TO_CLASS( monster_barney, CBarney );


//=========================================================
// Precache
//=========================================================

void CBarney :: Precache()
{
    PRECACHE_MODEL("models/barney.mdl");
	
	PRECACHE_MODEL( "models/leafgibs.mdl" );
	PRECACHE_SOUND( "ambience/bush1.wav" );
	PRECACHE_SOUND( "ambience/bush2.wav" );

	m_szFriends[0] = "monster_mikeforce";
	m_szFriends[1] = "monster_mikeforce_medic";
	m_szFriends[2] = "monster_barney";
	m_szFriends[3] = "player";
	m_szFriends[4] = "monster_peasant";
	m_nNumFriendTypes = 5;

	strcpy( m_szSpeechLabel, "BA_");

	CHumanFollower::Precache();
}    


//=========================================================
// Spawn
//=========================================================

void CBarney::Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/barney.mdl");

	pev->health			= gSkillData.barneyHealth;
	m_flFieldOfView		= VIEW_FIELD_WIDE;	// NOTE: we need a wide field of view so npc will notice player and say hello
	pev->weapons		= BARNEY_BODY_WEAPON_COLT1911A1;

	m_nHeadNum = 0;
	SetBodygroup( GetTorsoGroupNum(), 0 );

	CHumanFollower::Spawn();
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CBarney::ApplyDefaultSettings( void )
{
	m_nHeadNum = 0; 
	pev->weapons = BARNEY_BODY_WEAPON_COLT1911A1;
	m_fHandGrenades = TRUE;

	CHumanFollower::ApplyDefaultSettings();
}


//=========================================================
// HandleAnimEvent - Over-ridden for barney's sog_grabpull
// scripted sequence anim
//=========================================================

void CBarney :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_LEAFGIB:
		{
			Vector a, v;
			GetAttachment(1, v, a);

			for (int i = 0; i < NUM_LEAFGIBS; i++) 
			{
				CLeafGib *pGib = GetClassPtr( (CLeafGib *)NULL );

				pGib->Spawn( );
				pGib->pev->body = RANDOM_LONG(0, LEAF_GIB_COUNT-1);

				pGib->pev->origin = v;

				// mix in some noise
				pGib->pev->velocity.x += RANDOM_FLOAT ( -0.25, 0.25 );
				pGib->pev->velocity.y += RANDOM_FLOAT ( -0.25, 0.25 );
				pGib->pev->velocity.z += RANDOM_FLOAT ( 0, 0.3 );

				pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT ( 300, 400 );
			}
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
// GetWeaponBodyGroup - return body group index for weapon
//=========================================================

int CBarney :: GetWeaponBodyGroup( int weapon )
{
	switch( weapon )
	{
	case HUMAN_WEAPON_COLT1911A1: return BARNEY_BODY_WEAPON_COLT1911A1; break;
	case HUMAN_WEAPON_NONE: return BARNEY_BODY_WEAPON_NONE; break;
	}

	return BARNEY_BODY_WEAPON_COLT1911A1;
}


//=========================================================
// GetWeaponNum - return weapon index for body group
//=========================================================

int CBarney :: GetWeaponNum( int bodygroup )
{
	switch( bodygroup )
	{
	case BARNEY_BODY_WEAPON_COLT1911A1: return HUMAN_WEAPON_COLT1911A1; break;
	case BARNEY_BODY_WEAPON_NONE: return HUMAN_WEAPON_NONE; break;
	}

	return HUMAN_WEAPON_COLT1911A1;
}


//=========================================================
// GetDuration - Lengths of sentences
//=========================================================

float CBarney :: GetDuration( const char *pszSentence )
{
	if ( !strcmp( pszSentence, "QUESTION" ) ) return 6;
	if ( !strcmp( pszSentence, "UNUSE" ) ) return 4;
	if ( !strcmp( pszSentence, "SMELL" ) ) return 5;

	return CHumanFollower::GetDuration( pszSentence );
}


//=========================================================
// BARNEY REPEL
//=========================================================

class CBarneyRepel : public CHumanRepel
{
public:
	virtual char * EntityName() { return "monster_barney"; };
};

LINK_ENTITY_TO_CLASS( monster_barney_repel, CBarneyRepel );


//=========================================================
// DEAD Barney PROP
//=========================================================

class CDeadBarney : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[8];
};

char *CDeadBarney::m_szPoses[] = {	"lying_on_back",	"lying_on_side",	"lying_on_stomach", 
									"hanging_byfeet",	"hanging_byarms",	"hanging_byneck",
									"deadsitting",		"deadseated"	};

void CDeadBarney::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_barney_dead, CDeadBarney );


//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================

void CDeadBarney :: Spawn( void )
{
	PRECACHE_MODEL("models/barney.mdl");
	SET_MODEL(ENT(pev), "models/barney.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead Barney with bad pose\n" );
	}

	SetBodygroup( BARNEY_BODYGROUP_HEAD, 0 );
	SetBodygroup( BARNEY_BODYGROUP_TORSO, 0 );
	SetBodygroup( BARNEY_BODYGROUP_WEAPON, BARNEY_BODY_WEAPON_NONE );

	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();

	if ( m_iPose >=3 || m_iPose <6 ) 
	{
		pev->movetype = MOVETYPE_NONE;
		pev->framerate = 1;
	}
}

