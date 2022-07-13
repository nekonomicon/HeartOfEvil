//=========================================================
//
//		Human Grunt guy programmed for Nam by Nathan Ruck
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
	HGRUNT_BODYGROUP_HEAD = 0,
	HGRUNT_BODYGROUP_TORSO,
	HGRUNT_BODYGROUP_ARMS,
	HGRUNT_BODYGROUP_LEGS,
	HGRUNT_BODYGROUP_WEAPON,
};

enum
{
	HGRUNT_BODY_WEAPON_M16 = 0,
	HGRUNT_BODY_WEAPON_870,
	HGRUNT_BODY_WEAPON_M79,
	HGRUNT_BODY_WEAPON_NONE,
};

enum
{
	HGRUNT_BODY_HEAD_HELMET = 0,
	HGRUNT_BODY_HEAD_BLACK,
	HGRUNT_BODY_HEAD_BOONIE,
	HGRUNT_BODY_HEAD_CAP,
	HGRUNT_BODY_HEAD_ANGRY,
	HGRUNT_BODY_HEAD_HELMET_EVIL,
	HGRUNT_BODY_HEAD_HELMET2_EVIL,
	HGRUNT_BODY_HEAD_BOONIE_EVIL,
	HGRUNT_BODY_HEAD_CAP_EVIL,
};

enum
{
	HGRUNT_SKIN_WHITE = 0,
	HGRUNT_SKIN_BLACK,
};

#define NUM_HEADS 9

//=====================
// Spawn Flags
//=====================

#define SF_HGRUNT_FRIENDLY		0x008


class CHGrunt : public CHuman
{
public:
	void Spawn( );
	void Precache();
	int Classify( void );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void ApplyDefaultSettings( void );

	BOOL HasHumanGibs() { return FALSE; };
	BOOL HasAlienGibs() { return FALSE; };
	void GibMonster();

	int GetWeaponNum( int bodygroup );
	int GetWeaponBodyGroup( int weapon );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/namgrunthead.mdl"; };

	int GetHeadGroupNum( ) { return HGRUNT_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return HGRUNT_BODYGROUP_TORSO; };
	int GetArmsGroupNum( ) { return HGRUNT_BODYGROUP_ARMS; };
	int GetLegsGroupNum( ) { return HGRUNT_BODYGROUP_LEGS; };
	int GetWeaponGroupNum( ) { return HGRUNT_BODYGROUP_WEAPON; };
};

LINK_ENTITY_TO_CLASS( monster_human_grunt, CHGrunt );


//=========================================================
// Precache
//=========================================================

void CHGrunt :: Precache()
{
    PRECACHE_MODEL("models/namGrunt.mdl");
    PRECACHE_MODEL("models/namgruntgibs.mdl");
	
	PRECACHE_SOUND("hgrunt/step1.wav");
	PRECACHE_SOUND("hgrunt/step2.wav");
	PRECACHE_SOUND("hgrunt/step3.wav");
	PRECACHE_SOUND("hgrunt/step4.wav");

	m_szFriends[0] = "monster_human_grunt";
	m_szFriends[1] = "monster_human_grunt_medic";
	m_nNumFriendTypes = 2;

	strcpy( m_szSpeechLabel, "HG_");

	CHuman::Precache();
}    


//=========================================================
// Spawn
//=========================================================

void CHGrunt::Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/namGrunt.mdl");

	pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;

	if ( pev->body == -1 )
	{
		if ( FBitSet( pev->spawnflags, SF_HGRUNT_FRIENDLY ) )
		{
			m_nHeadNum = RANDOM_LONG( 0, 4 );// If I am friendly, don't use the evil heads
		}
		else
		{
			m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
		}
	}
	else 
	{
		m_nHeadNum = pev->body;
	}

	if ( m_nHeadNum == HGRUNT_BODY_HEAD_BLACK )
	{
		pev->skin = HGRUNT_SKIN_BLACK;
	}
	else
	{
		pev->skin = HGRUNT_SKIN_WHITE;
	}

	pev->body = 0;
	switch ( m_nHeadNum )
	{
	case HGRUNT_BODY_HEAD_HELMET:
	case HGRUNT_BODY_HEAD_BLACK: 
	case HGRUNT_BODY_HEAD_HELMET_EVIL:
	case HGRUNT_BODY_HEAD_HELMET2_EVIL: SetBodygroup( GetTorsoGroupNum(), 0 ); break;
	
	case HGRUNT_BODY_HEAD_CAP:
	case HGRUNT_BODY_HEAD_CAP_EVIL: SetBodygroup( GetTorsoGroupNum(), 1 ); break;
	
	case HGRUNT_BODY_HEAD_BOONIE:
	case HGRUNT_BODY_HEAD_BOONIE_EVIL:
	case HGRUNT_BODY_HEAD_ANGRY:	SetBodygroup( GetTorsoGroupNum(), 2 ); break;
	}

	SetBodygroup( GetLegsGroupNum(), 0 );
	SetBodygroup( GetArmsGroupNum(), 0 );

	CHuman::Spawn();
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CHGrunt::ApplyDefaultSettings( void )
{
	m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	switch ( m_nHeadNum )
	{
	case 0: 
	case 1: SetBodygroup( GetTorsoGroupNum(), 0 ); break;
	case 2: 
	case 3: SetBodygroup( GetTorsoGroupNum(), 1 ); break;
	case 4: SetBodygroup( GetTorsoGroupNum(), 2 ); break;
	}

	switch ( RANDOM_LONG( 0, 5 ) )
	{
	case 0:
	case 1:
	case 2: pev->weapons = HGRUNT_BODY_WEAPON_M16; break;
	case 3:
	case 4: pev->weapons = HGRUNT_BODY_WEAPON_870; break;
	case 5: pev->weapons = HGRUNT_BODY_WEAPON_M79; break;
	}
	m_fHandGrenades = RANDOM_LONG(0, 1);

	CHuman::ApplyDefaultSettings();
}


//=========================================================
// GetWeaponBodyGroup - return body group index for weapon
//=========================================================

int CHGrunt :: GetWeaponBodyGroup( int weapon )
{
	switch( weapon )
	{
	case HUMAN_WEAPON_M16: return HGRUNT_BODY_WEAPON_M16; break;
	case HUMAN_WEAPON_870: return HGRUNT_BODY_WEAPON_870; break;
	case HUMAN_WEAPON_M79: return HGRUNT_BODY_WEAPON_M79; break;
	case HUMAN_WEAPON_NONE: return HGRUNT_BODY_WEAPON_NONE; break;
	}

	return HGRUNT_BODY_WEAPON_M16;
}


//=========================================================
// GetWeaponNum - return weapon index for body group
//=========================================================

int CHGrunt :: GetWeaponNum( int bodygroup )
{
	switch( bodygroup )
	{
	case HGRUNT_BODY_WEAPON_M16: return HUMAN_WEAPON_M16; break;
	case HGRUNT_BODY_WEAPON_870: return HUMAN_WEAPON_870; break;
	case HGRUNT_BODY_WEAPON_M79: return HUMAN_WEAPON_M79; break;
	case HGRUNT_BODY_WEAPON_NONE: return HUMAN_WEAPON_NONE; break;
	}

	return HUMAN_WEAPON_M16;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================

int	CHGrunt :: Classify ( void )
{
	if (pev->spawnflags & SF_HGRUNT_FRIENDLY)
	{
		return CLASS_PLAYER_ALLY;
	}
	else
	{
		return CLASS_HUMAN_MILITARY;
	}
}



//=========================================================
// TakeDamage
//=========================================================

int CHGrunt :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if ((pev->spawnflags & SF_HGRUNT_FRIENDLY) && pevAttacker && m_MonsterState != MONSTERSTATE_PRONE && FBitSet(pevAttacker->flags, FL_CLIENT))
	{
		Remember( bits_MEMORY_PROVOKED );
		AlertFriends();
	}

	return CHuman::TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


//=========================================================
// GibMonster - HGrunt has own gib model!
//=========================================================

void CHGrunt :: GibMonster( void )
{
	TraceResult	tr;

	if ( CVAR_GET_FLOAT("violence_hgibs") != 0 )	// Only the player will ever get here
	{
		if ( m_nHeadNum != GetNumHeads() ) CGib::SpawnHeadGib( pev );
		CGib::SpawnRandomGibs( pev, HGRUNT_GIB_COUNT, GIB_HGRUNT );	// throw some hgrunt gibs.

		TraceResult	tr;
		UTIL_TraceLine ( pev->origin, pev->origin + Vector(0, 0, -64),  ignore_monsters, ENT(pev), & tr);
		UTIL_DecalTrace( &tr, DECAL_BIGBLOOD1 + RANDOM_LONG(0,1) );
	}
	
	CHuman::GibMonster();
}


//=========================================================
// HGRUNT REPEL
//=========================================================

class CHGruntRepel : public CHumanRepel
{
public:
	virtual char * EntityName() { return "monster_human_grunt"; };
};

LINK_ENTITY_TO_CLASS( monster_grunt_repel, CHGruntRepel );



//=========================================================
// DEAD HGRUNT PROP
//=========================================================

class CDeadHGrunt : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[8];
};

char *CDeadHGrunt::m_szPoses[] = {	"lying_on_back",	"lying_on_side",	"lying_on_stomach", 
									"hanging_byfeet",	"hanging_byarms",	"hanging_byneck",
									"deadsitting",		"deadseated"	};

void CDeadHGrunt::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_hgrunt_dead, CDeadHGrunt );


//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================

void CDeadHGrunt :: Spawn( void )
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
		ALERT ( at_console, "Dead hgrunt with bad pose\n" );
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
	switch ( nHeadNum )
	{
	case HGRUNT_BODY_HEAD_HELMET:
	case HGRUNT_BODY_HEAD_BLACK: 
	case HGRUNT_BODY_HEAD_HELMET_EVIL:
	case HGRUNT_BODY_HEAD_HELMET2_EVIL: SetBodygroup( HGRUNT_BODYGROUP_TORSO, 0 ); break;
	
	case HGRUNT_BODY_HEAD_CAP:
	case HGRUNT_BODY_HEAD_CAP_EVIL: SetBodygroup( HGRUNT_BODYGROUP_TORSO, 1 ); break;
	
	case HGRUNT_BODY_HEAD_BOONIE:
	case HGRUNT_BODY_HEAD_BOONIE_EVIL:
	case HGRUNT_BODY_HEAD_ANGRY:	SetBodygroup( HGRUNT_BODYGROUP_TORSO, 2 ); break;
	}

	SetBodygroup( HGRUNT_BODYGROUP_LEGS, 0 );
	SetBodygroup( HGRUNT_BODYGROUP_ARMS, 0 );
	SetBodygroup( HGRUNT_BODYGROUP_HEAD, nHeadNum );
	SetBodygroup( HGRUNT_BODYGROUP_WEAPON, HGRUNT_BODY_WEAPON_NONE );


	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();

	if ( m_iPose >=3 || m_iPose <6 ) 
	{
		pev->movetype = MOVETYPE_NONE;
		pev->framerate = 1;
	}
}
