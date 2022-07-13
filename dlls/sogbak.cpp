//=========================================================
// Studies and Observations Group - stealth guy
//=========================================================

//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"util.h"
#include	"plane.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"basemonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================

#define SOG_ATTN					ATTN_NORM	// attenutation of SOG sentences
#define	SOG_SENTENCE_VOLUME			(float)0.35 // volume of SOG sentences

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define		SOG_AE_FIRE				( 2 )
#define		SOG_AE_RELOAD			( 5 )
#define		SOG_AE_CAUGHT_ENEMY		( 6 ) // SOG established sight with an enemy (player only) that had previously eluded the squad.
#define		SOG_AE_DROP_GUN			( 7 ) // SOG (probably dead) is dropping his gun.
#define		SOG_AE_GREN_TOSS		( 11 )

//=========================================================
// Monster's body types
//=========================================================

#define SOG_BODY_STANDARD	0

#define SOG_FACE_BLOND_HAIR	0
#define SOG_FACE_BROWN_HAIR	1
#define SOG_FACE_GREY_HAIR	2
#define SOG_FACE_STEALTH	3
#define SOG_FACE_ZOMBIE		4

#define	SOG_WEAPON_M21		0
#define	SOG_WEAPON_NOGUN	1

#define BODY_GROUP		0
#define FACE_GROUP		1
#define WEAPON_GROUP	2

#define NUM_BODIES  1
#define NUM_FACES	5
#define NUM_WEAPONS	1

//=========================================================
// Monster's clip sizes
//=========================================================

int SOG_CLIP_SIZE[ NUM_WEAPONS ] = { 20 };


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SOG_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_SOG_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_SOG_COVER_AND_RELOAD,
	SCHED_SOG_SWEEP,
	SCHED_SOG_FOUND_ENEMY,
	SCHED_SOG_WAIT_FACE_ENEMY,
	SCHED_SOG_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_SOG_ELOF_FAIL,
	SCHED_SOG_FIRE_ONCE_AND_RUN_LIKE_A_BASTARD,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_SOG_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_SOG_SPEAK_SENTENCE,
	TASK_SOG_CHECK_FIRE,
};

//=====================
// Spawn Flags
//=====================

#define SF_SOG_HANDGRENADES	0x0064


//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_SOG_NOFIRE			( 1 << 14 )
#define bits_COND_SOG_SEEN				( bits_COND_SPECIAL1 )
#define bits_COND_SOG_CAN_BREAK_COVER	( bits_COND_SPECIAL2 )

class CSOG : public CSquadMonster
{
public:
	int CheckEnemy ( CBaseEntity *pEnemy );
	void Crouch( void );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	Vector GetGunPosition( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void M21Fire ( void );
	void PrescheduleThink ( void );
	void GibMonster( void );
	void RunAI( void );
	void Touch( CBaseEntity *pOther );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;
	BOOL	m_fHandGrenades;

	BOOL	m_fThrowGrenade;
	BOOL	m_Crouching;
	int		m_CrouchTime;		// Time till I can stop crouching (provided no further danger is sensed)
	BOOL	m_fFirstEncounter;	// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;
	int		m_iBrassShell;
	int		m_iTargetRenderamt;
};

LINK_ENTITY_TO_CLASS( monster_sog, CSOG );

TYPEDESCRIPTION	CSOG::m_SaveData[] = 
{
	DEFINE_FIELD( CSOG, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CSOG, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CSOG, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSOG, m_fHandGrenades, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSOG, m_CrouchTime, FIELD_TIME ),
	DEFINE_FIELD( CSOG, m_Crouching, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSOG, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CSOG, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSOG, m_cClipSize, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSOG, CSquadMonster );


//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CSOG :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( WEAPON_GROUP ) != SOG_WEAPON_NOGUN )
	{// throw a gun if the SOG has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		switch ( pev->weapons )
		{
			case SOG_WEAPON_M21:		pGun = DropItem( "weapon_m21", vecGunPos, vecGunAngles );		break;
		}
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}

	CBaseMonster :: GibMonster();
}


//=========================================================
// ISoundMask - Overidden for human SOGs because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CSOG :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}


//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CSOG :: PrescheduleThink ( void )
{
	// If I can see the enemy looking at me I know I'm not in cover
	
	if ( HasConditions ( bits_COND_SEE_ENEMY ) )
	{
		Forget( bits_MEMORY_INCOVER );
		
		// If he's looking at me as well and I'm not invisible I know I've been seen

		if ( HasConditions ( bits_COND_ENEMY_FACING_ME ) && pev->renderamt > 0 )
		{
			SetConditions( bits_COND_SOG_SEEN );
		}
	}
	
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human SOGs
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================

BOOL CSOG :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckRangeAttack1 - overridden for SOG, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the SOG can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================

BOOL CSOG :: CheckRangeAttack1 ( float flDot, float flDist )
{
	ClearConditions( bits_COND_SOG_CAN_BREAK_COVER );

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 8192 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			if ( flDist > 512 && !HasConditions( bits_COND_ENEMY_FACING_ME ) ) 
				SetConditions( bits_COND_SOG_CAN_BREAK_COVER );
			return TRUE;
		}
		else if (m_Crouching)
		{
			vecSrc = pev->origin + Vector( 0, 0, 62 );

			// Check if he could hit something if he stood up
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

			if ( tr.flFraction == 1.0 )
			{
				// Go out of crouch mode
				m_Crouching = FALSE;
				pev->view_ofs		= Vector ( 0, 0, 68 );
				if ( flDist > 512 && !HasConditions( bits_COND_ENEMY_FACING_ME ) ) 
					SetConditions( bits_COND_SOG_CAN_BREAK_COVER );
				return TRUE;
			}
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the SOG's grenade
// attack. 
//=========================================================

BOOL CSOG :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! m_fHandGrenades)
	{
		return FALSE;
	}
	
	// if the SOG isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	// find feet
	if (RANDOM_LONG(0,1))
	{
		// magically know where they are
		vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
	}
	else
	{
		// toss it to where you last saw them
		vecTarget = m_vecEnemyLKP;
	}

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

		
	Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;

		// throw a hand grenade
		m_fThrowGrenade = TRUE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
	}
	else
	{
		// don't throw
		m_fThrowGrenade = FALSE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
	}

	// If he can hit something, and the enemy cannot see him

	if ( m_fThrowGrenade && 
		( ( !HasConditions( bits_COND_ENEMY_FACING_ME ) && flDist > 512 ) || HasConditions( bits_COND_ENEMY_OCCLUDED ) ) ) 
		SetConditions( bits_COND_SOG_CAN_BREAK_COVER );
	
	return m_fThrowGrenade;
}


//=========================================================
// TakeDamage - overridden for the SOG because the SOG
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================

int CSOG :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );
	SetConditions( bits_COND_SOG_SEEN );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================

void CSOG :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE_ANGRY:
		ys = 360;	
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 360;	
		break;
	default:
		ys = 300;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckAmmo - overridden for the SOG because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CSOG :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSOG :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// Shoot
//=========================================================
void CSOG :: M21Fire ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_MONSTER_762 ); // shoot +-5 degrees

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	if (RANDOM_LONG(0, 1))
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m21_shot1.wav", 1, ATTN_NORM, 0, 100 );
	}
	else
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m21_shot2.wav", 1, ATTN_NORM, 0, 100 );
	}
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================

void CSOG :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		//Drop gun

		case SOG_AE_DROP_GUN:
			{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( WEAPON_GROUP, SOG_WEAPON_NOGUN );

			switch ( pev->weapons )
			{
				case SOG_WEAPON_M21:		DropItem( "weapon_m21", vecGunPos, vecGunAngles );		break;
			}
			
			}
			break;

		//Reload
			
		case SOG_AE_RELOAD:
			switch( RANDOM_LONG( 0, 2 ) )
			{
				case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload1.wav", 1, ATTN_NORM ); break;
				case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload2.wav", 1, ATTN_NORM ); break;
				case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload3.wav", 1, ATTN_NORM ); break;
			}
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		//Fire weapon

		case SOG_AE_FIRE:
		{
			switch ( pev->weapons )
			{
				case SOG_WEAPON_M21:		
					M21Fire();
					break;
			}
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );
		}
		break;

		case SOG_AE_CAUGHT_ENEMY:
		{

		}

		case SOG_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;

	}
}

//=========================================================
// Spawn
//=========================================================
void CSOG :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/SOG.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.SOGHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;
	m_cClipSize			= SOG_CLIP_SIZE[ pev->weapons ];
	m_cAmmoLoaded		= m_cClipSize;
	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the SOG spawns, because he hasn't encountered an enemy yet.

	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;
	Remember( bits_MEMORY_INCOVER );
	
	m_HackedGunPos = Vector ( 0, 0, 55 );

	if ( pev->body == -1 )
	{// -1 chooses a random body
		pev->body = RANDOM_LONG(0, NUM_FACES - 1 );
	}
	SetBodygroup( BODY_GROUP, SOG_BODY_STANDARD );
	SetBodygroup( FACE_GROUP, pev->body );
	SetBodygroup( WEAPON_GROUP, pev->weapons );

	m_fHandGrenades = pev->spawnflags & SF_SOG_HANDGRENADES;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSOG :: Precache()
{
	PRECACHE_MODEL("models/SOG.mdl");

	PRECACHE_SOUND("weapons/reload1.wav");
	PRECACHE_SOUND("weapons/reload2.wav");
	PRECACHE_SOUND("weapons/reload3.wav");
	
	PRECACHE_SOUND("weapons/m21_shot1.wav");
	PRECACHE_SOUND("weapons/m21_shot2.wav");

	PRECACHE_SOUND("mikeforce/arrgh.wav");
	PRECACHE_SOUND("mikeforce/urgh.wav");

	PRECACHE_SOUND("mikeforce/choke1.wav");
	PRECACHE_SOUND("mikeforce/choke2.wav");

	PRECACHE_SOUND("sog/fadeout.wav");
	PRECACHE_SOUND("sog/fadein.wav");

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}	

//=========================================================
// start task
//=========================================================
void CSOG :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_SOG_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_SOG_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_SOG_SPEAK_SENTENCE:
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// SOG no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_SOG_FACE_TOSS_DIR:
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CSOG :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SOG_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CSOG :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{

		switch ( RANDOM_LONG(0,1) )
		{
			case 0: EMIT_SOUND( ENT(pev), CHAN_VOICE, "mikeforce/arrgh.wav", 1, ATTN_NORM); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_VOICE, "mikeforce/urgh.wav", 1, ATTN_NORM); break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CSOG :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
		case 0: EMIT_SOUND( ENT(pev), CHAN_VOICE, "mikeforce/choke1.wav", 1, ATTN_NORM); break;
		case 1: EMIT_SOUND( ENT(pev), CHAN_VOICE, "mikeforce/choke2.wav", 1, ATTN_NORM); break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// SOGFail
//=========================================================
Task_t	tlSOGFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slSOGFail[] =
{
	{
		tlSOGFail,
		ARRAYSIZE ( tlSOGFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"SOG Fail"
	},
};

//=========================================================
// SOG Combat Fail
//=========================================================
Task_t	tlSOGCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slSOGCombatFail[] =
{
	{
		tlSOGCombatFail,
		ARRAYSIZE ( tlSOGCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"SOG Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlSOGVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
};

Schedule_t	slSOGVictoryDance[] =
{
	{ 
		tlSOGVictoryDance,
		ARRAYSIZE ( tlSOGVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"SOGVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the SOG to ambush the enemy.
//=========================================================
Task_t tlSOGEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_SOG_ELOF_FAIL		},
	{ TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY,	(float)8192						},
	{ TASK_SOG_SPEAK_SENTENCE,				(float)0						},
	{ TASK_RUN_PATH,						(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0						},
};

Schedule_t slSOGEstablishLineOfFire[] =
{
	{ 
		tlSOGEstablishLineOfFire,
		ARRAYSIZE ( tlSOGEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"SOGEstablishLineOfFire"
	},
};

//=========================================================
// SOGFoundEnemy - SOG established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlSOGFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slSOGFoundEnemy[] =
{
	{ 
		tlSOGFoundEnemy,
		ARRAYSIZE ( tlSOGFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"SOGFoundEnemy"
	},
};

//=========================================================
// SOGCombatFace Schedule
//=========================================================
Task_t	tlSOGCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SOG_SWEEP		},
};

Schedule_t	slSOGCombatFace[] =
{
	{ 
		tlSOGCombatFace1,
		ARRAYSIZE ( tlSOGCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};


//=========================================================
// SOG wait in cover - we don't allow danger or the ability
// to attack to break a SOG's run to cover schedule, but
// when a SOG is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlSOGWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)2					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SOG_SWEEP	},
};

Schedule_t	slSOGWaitInCover[] =
{
	{ 
		tlSOGWaitInCover,
		ARRAYSIZE ( tlSOGWaitInCover ), 
		bits_COND_NEW_ENEMY	|
		bits_COND_SOG_CAN_BREAK_COVER |
		bits_COND_SOG_SEEN |
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"SOGWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlSOGTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_SOG_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_SOG_SPEAK_SENTENCE,		(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SOG_WAIT_FACE_ENEMY	},
};

Schedule_t	slSOGTakeCover[] =
{
	{ 
		tlSOGTakeCover1,
		ARRAYSIZE ( tlSOGTakeCover1 ), 
		bits_COND_HEAVY_DAMAGE,
		0,
		"TakeCover"
	},
};


//=========================================================
// toss grenade then run to cover.
//=========================================================
Task_t	tlSOGTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slSOGTossGrenadeCover[] =
{
	{ 
		tlSOGTossGrenadeCover1,
		ARRAYSIZE ( tlSOGTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};


//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlSOGTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slSOGTakeCoverFromBestSound[] =
{
	{ 
		tlSOGTakeCoverFromBestSound,
		ARRAYSIZE ( tlSOGTakeCoverFromBestSound ), 
		0,
		0,
		"SOGTakeCoverFromBestSound"
	},
};

//=========================================================
// SOG reload schedule
//=========================================================
Task_t	tlSOGHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SOG_WAIT_FACE_ENEMY	},
};

Schedule_t slSOGHideReload[] = 
{
	{
		tlSOGHideReload,
		ARRAYSIZE ( tlSOGHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"SOGHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlSOGSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slSOGSweep[] =
{
	{ 
		tlSOGSweep,
		ARRAYSIZE ( tlSOGSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_SOG_CAN_BREAK_COVER |
		bits_COND_SOG_SEEN |
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"SOG Sweep"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// SOG's grenade toss requires the enemy be occluded.
//=========================================================

Task_t	tlSOGRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOG_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOG_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSOGRangeAttack1A[] =
{
	{ 
		tlSOGRangeAttack1A,
		ARRAYSIZE ( tlSOGRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_SOG_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// SOG's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlSOGRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOG_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOG_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSOGRangeAttack1B[] =
{
	{ 
		tlSOGRangeAttack1B,
		ARRAYSIZE ( tlSOGRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_SOG_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};


//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// SOG's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlSOGRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SOG_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_WAIT,					(float)0.2					},
};

Schedule_t	slSOGRangeAttack2[] =
{
	{ 
		tlSOGRangeAttack2,
		ARRAYSIZE ( tlSOGRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// Fire Once and Run Like a Bastard
//=========================================================

Task_t tlSOGFireOnceAndRunLikeABastard[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOG_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t slSOGFireOnceAndRunLikeABastard[] =
{
	{
		tlSOGFireOnceAndRunLikeABastard,
		ARRAYSIZE ( tlSOGFireOnceAndRunLikeABastard ),
		0,
		0,
		"RunLikeBastard"
	},
};


DEFINE_CUSTOM_SCHEDULES( CSOG )
{
	slSOGFail,
	slSOGCombatFail,
	slSOGVictoryDance,
	slSOGEstablishLineOfFire,
	slSOGFoundEnemy,
	slSOGCombatFace,
	slSOGWaitInCover,
	slSOGTakeCover,
	slSOGTakeCoverFromBestSound,
	slSOGHideReload,
	slSOGSweep,
	slSOGRangeAttack1A,
	slSOGRangeAttack1B,
	slSOGRangeAttack2,
	slSOGTossGrenadeCover,
	slSOGFireOnceAndRunLikeABastard,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSOG, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CSOG :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	char seq[40];

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		switch ( pev->weapons )
		{
			case SOG_WEAPON_M21:		
				strcpy( seq, "shootm21" );
				break;
		}
		break;

	case ACT_RANGE_ATTACK2:
		strcpy( seq, "throwgrenade");
		break;

	case ACT_COWER:
		strcpy( seq, "combatidle" );
		Crouch();
		break;

	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			strcpy( seq, "combatidle" );
			NewActivity = ACT_IDLE_ANGRY;
		}
		else
		{
			strcpy( seq, "idle" );
		}
		break;

	case ACT_IDLE_ANGRY:
		strcpy( seq, "combatidle" );
		break;

	case ACT_WALK:
		strcpy( seq, "walk" );
		break;

	case ACT_RUN:
		strcpy( seq, "run" );
		break;

	case ACT_RELOAD:
		strcpy( seq, "reload" );
		Crouch();
		break;
		
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
	case ACT_FLINCH_LEFTARM:
	case ACT_FLINCH_RIGHTARM:
	case ACT_FLINCH_LEFTLEG:
	case ACT_FLINCH_RIGHTLEG:
	case ACT_SMALL_FLINCH:
		if (m_Crouching)
			strcpy ( seq, ( m_MonsterState == MONSTERSTATE_COMBAT ? "combatidle" : "idle" ) );
		else
			iSequence = LookupActivity ( NewActivity );
		break;

	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	if (iSequence == ACTIVITY_NOT_AVAILABLE)
	{
		char seq2[40];

		if ( m_Crouching )
		{
			strcpy( seq2, "crouch_" );
			strcat( seq2, seq );
			strcpy( seq, seq2 );
		}

		iSequence = LookupSequence( seq );
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}


	// Set invisible if not moving
	if ( m_iTargetRenderamt == 255 && pev->deadflag == DEAD_NO && 
		(m_Activity == ACT_IDLE || m_Activity == ACT_IDLE_ANGRY || m_Activity == ACT_COWER))
	{
		EMIT_SOUND (ENT(pev), CHAN_BODY, "sog/fadeout.wav", 1, ATTN_NORM );
		m_iTargetRenderamt = 0;
	}
	
	if (	m_iTargetRenderamt == 0 && 
			!(	m_Activity == ACT_IDLE || m_Activity == ACT_IDLE_ANGRY || m_Activity == ACT_COWER ||
				m_Activity == ACT_TURN_LEFT || m_Activity == ACT_TURN_RIGHT ||
				m_Activity == ACT_ROLL_LEFT || m_Activity == ACT_ROLL_RIGHT ) )
	{
		EMIT_SOUND (ENT(pev), CHAN_BODY, "sog/fadein.wav", 1, ATTN_NORM );
		m_iTargetRenderamt = 255;
	}

}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CSOG :: GetSchedule( void )
{
	if (m_Crouching)
	{
		// If I've been crouching for a while
		if ( m_CrouchTime < gpGlobals->time ) 
		{
			if (m_hEnemy != NULL && FVisible( m_hEnemy ))
			{
				// Stay crouching if I'm in the middle of a fight
				Crouch();
			}
			else
			{
				// Go out of crouch mode
				m_Crouching = FALSE;
				pev->view_ofs		= Vector ( 0, 0, 68 );
			}
		}
	}

	// SOGs place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			// Crouch if I hear something dodgy
			if (pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER))
				Crouch();

			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// If I've been seen
			
			if ( HasConditions( bits_COND_SOG_SEEN ) )
			{
				// no ammo
				if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
				{
					// If he's been seen and has no ammo gotta run away 
					return GetScheduleOfType ( SCHED_SOG_COVER_AND_RELOAD );
				}

				// Take cover if it looks like I might get hurt
				if ( m_hEnemy != NULL && 
					( HasConditions( bits_COND_LIGHT_DAMAGE ) || HasConditions( bits_COND_ENEMY_FACING_ME ) ) )
				{
						// only try to take cover if we actually have an enemy!

						return GetScheduleOfType( SCHED_SOG_FIRE_ONCE_AND_RUN_LIKE_A_BASTARD );
				}
			
				if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) &&
					!HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					return GetScheduleOfType ( SCHED_SOG_ESTABLISH_LINE_OF_FIRE );
				}
			}
			else
			{
				// If my enemy can't see me

				// Am I in cover or not?  If I am in cover I can do stuff, if not I want to stay invisible

				if ( HasMemory( bits_MEMORY_INCOVER ) )
				{
					//no ammo
					if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
					{
						// If he's not been seen and has no ammo and he's in cover, then reload, otherwise stay hidden
						return GetScheduleOfType ( SCHED_RELOAD );
					}
		
					// If you have a new enemy try and find him
					if ( InSquad() && HasConditions(bits_COND_NEW_ENEMY) )
					{
						return GetScheduleOfType ( SCHED_SOG_ESTABLISH_LINE_OF_FIRE );
					}

					// Hide and wait, unless someone in your squad has seen an enemy recently
					if ( !InSquad() || MySquadLeader()->m_fEnemyEluded )
					{
						return GetScheduleOfType( SCHED_SOG_SWEEP );
					}
				}

				if ( !HasConditions ( bits_COND_SOG_CAN_BREAK_COVER ) )
				{
					return GetScheduleOfType( SCHED_SOG_WAIT_FACE_ENEMY );
				}
			}

// can shoot and cover is broken
			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_SOG_FOUND_ENEMY );
					}

					if ( OccupySlot ( bits_SLOTS_HUMAN_ENGAGE ) )
					{
						// try to take an available ENGAGE slot
						return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
					{
						// throw a grenade if can and no engage slots are available
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					}
					else
					{
						// hide!
						return GetScheduleOfType( SCHED_SOG_FIRE_ONCE_AND_RUN_LIKE_A_BASTARD );
					}
				}
				else
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
			}

			// can grenade launch

			if ( m_fHandGrenades && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && 
				OccupySlot( bits_SLOTS_HUMAN_GRENADE ) && !(InSquad() && MySquadLeader()->m_fEnemyEluded ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			
	
			// If all else fails

			if ( HasConditions( bits_COND_SOG_SEEN ) )
			{
				GetScheduleOfType( SCHED_SOG_ESTABLISH_LINE_OF_FIRE );
			}
			else
			{
				return GetScheduleOfType( SCHED_SOG_WAIT_FACE_ENEMY );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CSOG :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK2:
		{
			return &slSOGRangeAttack2[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slSOGTakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slSOGTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_SOG_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HUMAN_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_SOG_ELOF_FAIL:
		{
			// human SOG is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_SOG_ESTABLISH_LINE_OF_FIRE:
		{
			return &slSOGEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slSOGRangeAttack1B[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slSOGCombatFace[ 0 ];
		}
	case SCHED_SOG_WAIT_FACE_ENEMY:
		{
			return &slSOGWaitInCover[ 0 ];
		}
	case SCHED_IDLE_STAND:
	case SCHED_ALERT_STAND:
	case SCHED_SOG_SWEEP:
		{
			return &slSOGSweep[ 0 ];
		}
	case SCHED_SOG_COVER_AND_RELOAD:
		{
			return &slSOGHideReload[ 0 ];
		}
	case SCHED_SOG_FOUND_ENEMY:
		{
			return &slSOGFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slSOGFail[ 0 ];
				}
			}

			return &slSOGVictoryDance[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// SOG has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slSOGCombatFail[ 0 ];
			}

			return &slSOGFail[ 0 ];
		}
	case SCHED_SOG_FIRE_ONCE_AND_RUN_LIKE_A_BASTARD:
		{
			return &slSOGFireOnceAndRunLikeABastard[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}


void CSOG::Crouch()
{
	if ( pev->spawnflags & SF_MONSTER_PREDISASTER ) return;

	m_Crouching = TRUE;
	m_CrouchTime = gpGlobals->time + RANDOM_LONG(15, 25);
	pev->view_ofs = Vector ( 0, 0, 40 );
}


int CSOG::CheckEnemy(CBaseEntity *pEnemy)
{
	if (!pEnemy->IsAlive() && !HasConditions( bits_COND_ENEMY_DEAD ))
	{
		Crouch();
	}
	
	return CSquadMonster :: CheckEnemy ( pEnemy );
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CSOG :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// If crouching, reduce damage from explosions
	if (m_Crouching && (bitsDamageType & DMG_BLAST)) flDamage = flDamage / 2;	

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CSOG :: GetGunPosition( )
{
	if (m_Crouching )
	{
		return pev->origin + Vector( 0, 0, 36 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 62 );
	}
}


//=========================================================
// Run AI, overridden so SOG can fade when standing still
//=========================================================

void CSOG :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if (pev->renderamt > m_iTargetRenderamt)
	{
		pev->renderamt = max( pev->renderamt - 10, m_iTargetRenderamt );
		pev->rendermode = kRenderTransTexture;
	}
	else if (pev->renderamt < m_iTargetRenderamt)
	{
		pev->renderamt = min( pev->renderamt + 50, m_iTargetRenderamt );
		if (pev->renderamt == 255)
			pev->rendermode = kRenderNormal;
	}
}


//=========================================================
// Run AI, overridden so SOG can fade when standing still
//=========================================================

void CSOG :: Touch( CBaseEntity *pOther )
{
	// Uh-oh, someone I don't like touched me... cooties...

	if ( IRelationship( pOther ) >= R_DL )
	{
		SetConditions( bits_COND_SOG_SEEN );
		Forget( bits_MEMORY_INCOVER );
	}
}
