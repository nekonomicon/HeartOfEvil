//=========================================================
//
//	Kurtz
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

//=====================
// BodyGroups
//=====================

enum 
{
	KURTZ_BODYGROUP_HEAD = 0,
	KURTZ_BODYGROUP_TORSO,
	KURTZ_BODYGROUP_WEAPON,
};

//=====================
// Animation Events
//=====================

enum
{
	KURTZ_AE_THROWHEAD = LAST_HUMAN_ANIM_EVENT + 1, // 11
};


#define NUM_HEADS 2


class CKurtz : public CHuman
{
	void Spawn( );
	void Precache();
	int Classify( void ) { return CLASS_NONE; };
	void StartTask( Task_t *pTask );
	Schedule_t *GetSchedule ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void PlayLabelledSentence( const char *pszSentence );
	int GetNumHeads() { return NUM_HEADS; };
	char * GetHeadModelName() { return "models/kurtzhead.mdl"; };

	int GetHeadGroupNum( ) { return KURTZ_BODYGROUP_HEAD; };
	int GetTorsoGroupNum( ) { return KURTZ_BODYGROUP_TORSO; };
	int GetWeaponGroupNum( ) { return KURTZ_BODYGROUP_WEAPON; };
};

LINK_ENTITY_TO_CLASS( monster_kurtz, CKurtz );


//=========================================================
// Precache
//=========================================================

void CKurtz :: Precache()
{
    PRECACHE_MODEL("models/kurtz.mdl");
    PRECACHE_MODEL("models/barneyhead.mdl");
	m_nNumFriendTypes = 0;				// kurtz has no friends!
	strcpy( m_szSpeechLabel, "");
	CHuman::Precache();
}    


//=========================================================
// Spawn
//=========================================================

void CKurtz::Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/kurtz.mdl");

	pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= VIEW_FIELD_WIDE;

	switch ( pev->body )
	{
	case 0: // Black Pajamas
		{
			m_nHeadNum = 0;							// Bald
			SetBodygroup( GetTorsoGroupNum(), 1 );	// Pajamas
		}
		break;

	case 1: // Uniform
		{
			m_nHeadNum = 1;							// Beret
			SetBodygroup( GetTorsoGroupNum(), 0 );	// Uniform
		}
		break;
	}

	CHuman::Spawn();
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================

void CKurtz :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case KURTZ_AE_THROWHEAD:
		{
			UTIL_MakeVectors( pev->angles );

			CHumanHead * pHead = GetClassPtr( (CHumanHead *)NULL );
		
			if (pHead)
			{
				pHead->Spawn( "models/barneyhead.mdl", 0 );
				pHead->pev->velocity = gpGlobals->v_forward * 150 + gpGlobals->v_up * 250;
				pHead->pev->angles = Vector( 90, pev->angles.y, 0 );
				UTIL_SetOrigin( pHead->pev, GetGunPosition() );
			}
		}
		break;
		
	default:
		CHuman::HandleAnimEvent( pEvent );
	}
}


//=========================================================
// StartTask - Over-rides base because Kurtz don't crouch
//=========================================================

void CKurtz::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HUMAN_CROUCH:
		TaskComplete();
		break;

	case TASK_HUMAN_UNCROUCH:
		TaskComplete();
		break;

	default:
		CHuman::StartTask( pTask );
		break;
	}
}


//=========================================================
// GetSchedule - Totally over-rides base because Kurtz
// doesn't do anything unless scripted to
//=========================================================

Schedule_t *CKurtz :: GetSchedule ( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_COMBAT:
		{
			// Do nothing except flinch if attacked

			if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE)/* && !HasMemory( bits_MEMORY_FLINCHED)*/ )
			{
				return CBaseMonster::GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			else
			{
				return CBaseMonster::GetScheduleOfType( SCHED_IDLE_STAND );
			}
		}
		break;

	case MONSTERSTATE_PRONE:
	case MONSTERSTATE_NONE:
	case MONSTERSTATE_DEAD:
	case MONSTERSTATE_SCRIPT:
	default:
		{
			return CBaseMonster::GetSchedule();
		}
		break;

	}
}


//=========================================================
// PlayLabelledSentence - Over-ridden because Kurtz don't
// say anything unless scripted to
//=========================================================

void CKurtz::PlayLabelledSentence( const char *pszSentence )
{
	
}