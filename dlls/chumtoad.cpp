//=========================================================
// chumtoad.cpp - friendly one-eyed alien toad
//=========================================================

#include    "extdll.h"
#include    "util.h"
#include    "cbase.h"
#include    "monsters.h"
#include    "schedule.h"
#include	"decals.h"
#include	"soundent.h"


//=====================
// Monsters Class Definition
//=====================
class CChumtoad : public CBaseMonster
{
public:
	Schedule_t* GetSchedule( void );
	void Killed( entvars_t *pevAttacker, int iGib );
    void Spawn( void );
    void Precache( void );
    void SetYawSpeed ( void );
	void Touch ( CBaseEntity *pOther );

    int Classify ( void );
};

LINK_ENTITY_TO_CLASS( monster_chumtoad, CChumtoad );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int    CChumtoad :: Classify ( void )
{
    return    CLASS_ALIEN_PASSIVE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChumtoad :: SetYawSpeed ( void )
{
    int ys;
    ys = 360;
    pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CChumtoad :: Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/chumtoad.mdl");

	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 8 ) );

    pev->solid             = SOLID_SLIDEBOX;
    pev->movetype        = MOVETYPE_STEP;
    m_bloodColor        = BLOOD_COLOR_GREEN;
    pev->effects        = 0;
    pev->health             = gSkillData.chumtoadHealth;
    pev->view_ofs        = Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
    pev->yaw_speed        = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
    m_flFieldOfView        = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
    m_MonsterState        = MONSTERSTATE_NONE;

    MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChumtoad :: Precache()
{
    PRECACHE_MODEL("models/chumtoad.mdl");
}    


void CChumtoad :: Touch ( CBaseEntity *pOther )
{
	if ( pOther->pev->velocity == g_vecZero || pev->health <= 0)
	{
		return;
	}

		Vector		vecSpot;
		TraceResult	tr;
		vecSpot = pev->origin + Vector ( 0 , 0 , 8 );//move up a bit, and trace down.
		UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -24 ),  ignore_monsters, ENT(pev), & tr);
		UTIL_DecalTrace( &tr, DECAL_YBLOOD1 +RANDOM_LONG(0,5) );

		TakeDamage( pOther->pev, pOther->pev, pev->health, DMG_CRUSH );
}

void CChumtoad::Killed(entvars_t *pevAttacker, int iGib)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_die1.wav", 0.8, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );

	CBaseMonster::Killed( pevAttacker, iGib);
}



Schedule_t* CChumtoad::GetSchedule()
{
	if (HasConditions(bits_COND_SEE_FEAR))
	{
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	}

	return CBaseMonster::GetSchedule();
}
