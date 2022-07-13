//=========================================================
//
//		Medic programmed for Nam by Nathan Ruck
//
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"human.h"
#include	"humanmedic.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"soundent.h"
#include	"animation.h"
#include	"weapons.h"
#include	"decals.h"


//=====================
// Spawn Flags
//=====================

#define SF_HGRUNT_MEDIC_FRIENDLY		0x008


//=====================
// BodyGroups
//=====================

enum 
{
	HGRUNT_MEDIC_BODYGROUP_HEAD = 0,
	HGRUNT_MEDIC_BODYGROUP_BODY,
	HGRUNT_MEDIC_BODYGROUP_WEAPON,
	HGRUNT_MEDIC_BODYGROUP_SYRINGE,
};

enum
{
	HGRUNT_MEDIC_BODY_WEAPON_COLT1911A1 = 0,
	HGRUNT_MEDIC_BODY_WEAPON_NONE,
};

enum
{
	HGRUNT_MEDIC_BODY_HEAD_BLACK = 0,
	HGRUNT_MEDIC_BODY_HEAD_WHITE,
};

enum
{
	HGRUNT_MEDIC_SKIN_WHITE = 0,
	HGRUNT_MEDIC_SKIN_BLACK,
};


#define NUM_HEADS 2


class CHGruntMedic : public CHumanMedic
{
public:
	void Spawn( );
	void Precache();
	int Classify( void );
	void ApplyDefaultSettings( void );

	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	int GetWeaponNum( int bodygroup );
	int GetWeaponBodyGroup( int weapon );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/armymedichead.mdl"; };

	BOOL HasHumanGibs() { return FALSE; };
	BOOL HasAlienGibs() { return FALSE; };
	void GibMonster();

	int GetHeadGroupNum( ) { return HGRUNT_MEDIC_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return HGRUNT_MEDIC_BODYGROUP_BODY; };
	int GetWeaponGroupNum( ) { return HGRUNT_MEDIC_BODYGROUP_WEAPON; };
	int GetSyringeGroupNum( ) { return HGRUNT_MEDIC_BODYGROUP_SYRINGE; };
};

LINK_ENTITY_TO_CLASS( monster_human_grunt_medic, CHGruntMedic );


//=========================================================
// Precache
//=========================================================

void CHGruntMedic :: Precache()
{
    PRECACHE_MODEL("models/gruntmedic.mdl");
    PRECACHE_MODEL("models/gruntmedicgibs.mdl");
	
	PRECACHE_SOUND("hgrunt/step1.wav");
	PRECACHE_SOUND("hgrunt/step2.wav");
	PRECACHE_SOUND("hgrunt/step3.wav");
	PRECACHE_SOUND("hgrunt/step4.wav");

	m_szFriends[0] = "monster_human_grunt";
	m_szFriends[1] = "monster_human_grunt_medic";
	m_nNumFriendTypes = 2;

	strcpy( m_szSpeechLabel, "HG_");

	CHumanMedic::Precache();
}    


//=========================================================
// Spawn
//=========================================================

void CHGruntMedic::Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/gruntmedic.mdl");

	pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;

	if ( pev->body == -1 )
	{
		m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	}
	else 
	{
		m_nHeadNum = pev->body;
	}

	if ( m_nHeadNum == HGRUNT_MEDIC_BODY_HEAD_BLACK )
	{
		pev->skin = HGRUNT_MEDIC_SKIN_BLACK;
	}
	else
	{
		pev->skin = HGRUNT_MEDIC_SKIN_WHITE;
	}

	pev->body = 0;
	SetBodygroup( GetTorsoGroupNum(), 0 );
	SetBodygroup( GetSyringeGroupNum(), FALSE );

	CHumanMedic::Spawn();
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CHGruntMedic::ApplyDefaultSettings( void )
{
	m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	pev->weapons = HGRUNT_MEDIC_BODY_WEAPON_COLT1911A1;
	m_fHandGrenades = RANDOM_LONG(0, 1);

	if ( m_nHeadNum == HGRUNT_MEDIC_BODY_HEAD_BLACK )
	{
		pev->skin = HGRUNT_MEDIC_SKIN_BLACK;
	}
	else
	{
		pev->skin = HGRUNT_MEDIC_SKIN_WHITE;
	}

	CHumanMedic::ApplyDefaultSettings();
}


//=========================================================
// GetWeaponBodyGroup - return body group index for weapon
//=========================================================

int CHGruntMedic :: GetWeaponBodyGroup( int weapon )
{
	switch( weapon )
	{
	case HUMAN_WEAPON_COLT1911A1: return HGRUNT_MEDIC_BODY_WEAPON_COLT1911A1; break;
	case HUMAN_WEAPON_NONE: return HGRUNT_MEDIC_BODY_WEAPON_NONE; break;
	}

	return HGRUNT_MEDIC_BODY_WEAPON_COLT1911A1;
}


//=========================================================
// GetWeaponNum - return weapon index for bodygroup
//=========================================================

int CHGruntMedic :: GetWeaponNum( int bodygroup )
{
	switch( bodygroup )
	{
	case HGRUNT_MEDIC_BODY_WEAPON_COLT1911A1: return HUMAN_WEAPON_COLT1911A1; break;
	case HGRUNT_MEDIC_BODY_WEAPON_NONE: return HUMAN_WEAPON_NONE; break;
	}

	return HUMAN_WEAPON_COLT1911A1;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================

int	CHGruntMedic :: Classify ( void )
{
	if (pev->spawnflags & SF_HGRUNT_MEDIC_FRIENDLY)
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

int CHGruntMedic :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if ((pev->spawnflags & SF_HGRUNT_MEDIC_FRIENDLY) && pevAttacker && m_MonsterState != MONSTERSTATE_PRONE && FBitSet(pevAttacker->flags, FL_CLIENT))
	{
		Remember( bits_MEMORY_PROVOKED );
		AlertFriends();
	}

	return CHuman::TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}



//=========================================================
// GibMonster - Medic has own gib model!
//=========================================================

void CHGruntMedic :: GibMonster( void )
{
	TraceResult	tr;

	if ( CVAR_GET_FLOAT("violence_hgibs") != 0 )	// Only the player will ever get here
	{
		if ( m_nHeadNum != GetNumHeads() ) CGib::SpawnHeadGib( pev );
		CGib::SpawnRandomGibs( pev, HGRUNTMEDIC_GIB_COUNT, GIB_HGRUNTMEDIC );	// throw some gruntmedic gibs.

		TraceResult	tr;
		UTIL_TraceLine ( pev->origin, pev->origin + Vector(0, 0, -64),  ignore_monsters, ENT(pev), & tr);
		UTIL_DecalTrace( &tr, DECAL_BIGBLOOD1 + RANDOM_LONG(0,1) );
	}
	
	CHumanMedic::GibMonster();
}


//=========================================================
// HGRUNT REPEL
//=========================================================

class CHGruntMedicRepel : public CHumanRepel
{
public:
	virtual char * EntityName() { return "monster_human_grunt_medic"; };
};

LINK_ENTITY_TO_CLASS( monster_grunt_medic_repel, CHGruntMedicRepel );


