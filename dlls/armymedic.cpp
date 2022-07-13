//=========================================================
//
//		Friendly Medic programmed for Nam by Nathan Ruck
//
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"human.h"
#include	"humanfollower.h"
#include	"humanfollowermedic.h"
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
	ARMY_MEDIC_BODYGROUP_HEAD = 0,
	ARMY_MEDIC_BODYGROUP_BODY,
	ARMY_MEDIC_BODYGROUP_WEAPON,
	ARMY_MEDIC_BODYGROUP_SYRINGE,
};

enum
{
	ARMY_MEDIC_BODY_WEAPON_COLT1911A1 = 0,
	ARMY_MEDIC_BODY_WEAPON_NONE,
};

enum
{
	ARMY_MEDIC_BODY_HEAD_BLACK = 0,
	ARMY_MEDIC_BODY_HEAD_WHITE,
};

enum
{
	ARMY_MEDIC_SKIN_WHITE = 0,
	ARMY_MEDIC_SKIN_BLACK,
};


#define NUM_HEADS 2


class CArmyMedic : public CHumanFollowerMedic
{
public:
	void Spawn( );
	void Precache();
	int Classify( void );
	void ApplyDefaultSettings( void );

	int GetWeaponNum( int bodygroup );
	int GetWeaponBodyGroup( int weapon );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/armymedichead.mdl"; };

	BOOL HasHumanGibs() { return FALSE; };
	BOOL HasAlienGibs() { return FALSE; };
	void GibMonster();

	int GetHeadGroupNum( ) { return ARMY_MEDIC_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return ARMY_MEDIC_BODYGROUP_BODY; };
	int GetWeaponGroupNum( ) { return ARMY_MEDIC_BODYGROUP_WEAPON; };
	int GetSyringeGroupNum( ) { return ARMY_MEDIC_BODYGROUP_SYRINGE; };
};

LINK_ENTITY_TO_CLASS( monster_mikeforce_medic, CArmyMedic );


//=========================================================
// Precache
//=========================================================

void CArmyMedic :: Precache()
{
    PRECACHE_MODEL("models/armymedic.mdl");
    PRECACHE_MODEL("models/armymedicgibs.mdl");
	
	PRECACHE_SOUND("mikeforce/step1.wav");
	PRECACHE_SOUND("mikeforce/step2.wav");
	PRECACHE_SOUND("mikeforce/step3.wav");
	PRECACHE_SOUND("mikeforce/step4.wav");

	m_szFriends[0] = "monster_mikeforce";
	m_szFriends[1] = "monster_mikeforce_medic";
	m_szFriends[2] = "monster_barney";
	m_szFriends[3] = "player";
	m_szFriends[4] = "monster_peasant";
	m_nNumFriendTypes = 5;

	strcpy( m_szSpeechLabel, "MF_");

	CHumanFollowerMedic::Precache();
}    


//=========================================================
// Spawn
//=========================================================

void CArmyMedic::Spawn()
{
	pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;

    Precache( );

    SET_MODEL(ENT(pev), "models/armymedic.mdl");

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

	if ( m_nHeadNum == ARMY_MEDIC_BODY_HEAD_BLACK )
	{
		pev->skin = ARMY_MEDIC_SKIN_BLACK;
	}
	else
	{
		pev->skin = ARMY_MEDIC_SKIN_WHITE;
	}

	pev->body = 0;
	SetBodygroup( GetTorsoGroupNum(), 0 );
	SetBodygroup( GetSyringeGroupNum(), FALSE );

	CHumanFollowerMedic::Spawn();
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CArmyMedic::ApplyDefaultSettings( void )
{
	m_nHeadNum = RANDOM_LONG( 0, GetNumHeads() - 1 ); 
	pev->weapons = ARMY_MEDIC_BODY_WEAPON_COLT1911A1;
	m_fHandGrenades = RANDOM_LONG(0, 1);

	if ( m_nHeadNum == ARMY_MEDIC_BODY_HEAD_BLACK )
	{
		pev->skin = ARMY_MEDIC_SKIN_BLACK;
	}
	else
	{
		pev->skin = ARMY_MEDIC_SKIN_WHITE;
	}

	CHumanFollowerMedic::ApplyDefaultSettings();
}


//=========================================================
// GetWeaponBodyGroup - return body group index for weapon
//=========================================================

int CArmyMedic :: GetWeaponBodyGroup( int weapon )
{
	switch( weapon )
	{
	case HUMAN_WEAPON_COLT1911A1: return ARMY_MEDIC_BODY_WEAPON_COLT1911A1; break;
	case HUMAN_WEAPON_NONE: return ARMY_MEDIC_BODY_WEAPON_NONE; break;
	}

	return ARMY_MEDIC_BODY_WEAPON_COLT1911A1;
}


//=========================================================
// GetWeaponNum - return weapon index for bodygroup
//=========================================================

int CArmyMedic :: GetWeaponNum( int bodygroup )
{
	switch( bodygroup )
	{
	case ARMY_MEDIC_BODY_WEAPON_COLT1911A1: return HUMAN_WEAPON_COLT1911A1; break;
	case ARMY_MEDIC_BODY_WEAPON_NONE: return HUMAN_WEAPON_NONE; break;
	}

	return HUMAN_WEAPON_COLT1911A1;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================

int	CArmyMedic :: Classify ( void )
{
	return CLASS_PLAYER_ALLY;
}



//=========================================================
// GibMonster - Army Medic has own gib model!
//=========================================================

void CArmyMedic :: GibMonster( void )
{
	TraceResult	tr;

	if ( CVAR_GET_FLOAT("violence_hgibs") != 0 )	// Only the player will ever get here
	{
		if ( m_nHeadNum != GetNumHeads() ) CGib::SpawnHeadGib( pev );
		CGib::SpawnRandomGibs( pev, ARMYMEDIC_GIB_COUNT, GIB_ARMYMEDIC );	// throw some armymedic gibs.

		TraceResult	tr;
		UTIL_TraceLine ( pev->origin, pev->origin + Vector(0, 0, -64),  ignore_monsters, ENT(pev), & tr);
		UTIL_DecalTrace( &tr, DECAL_BIGBLOOD1 + RANDOM_LONG(0,1) );
	}
	
	CHumanFollowerMedic::GibMonster();
}


//=========================================================
// ARMY MEDIC REPEL
//=========================================================

class CArmyMedicRepel : public CHumanRepel
{
public:
	virtual char * EntityName() { return "monster_mikeforce_medic"; };
};

LINK_ENTITY_TO_CLASS( monster_mikeforce_medic_repel, CArmyMedicRepel );


