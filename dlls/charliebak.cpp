//=========================================================
// charlie, damn him!  Altered from the hgrunt code
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

int g_fCharlieQuestion;				// true if an idle charlie asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================

#define CHARLIE_VOL						0.35		// volume of charlie sounds
#define CHARLIE_ATTN					ATTN_NORM	// attenutation of charlie sentences
#define	CHARLIE_SENTENCE_VOLUME			(float)0.35 // volume of charlie sentences

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define		CHARLIE_AE_BURST1			( 2 )
#define		CHARLIE_AE_BURST2			( 3 ) 
#define		CHARLIE_AE_BURST3			( 4 ) 
#define		CHARLIE_AE_RELOAD			( 5 )
#define		CHARLIE_AE_CAUGHT_ENEMY		( 6 ) // charlie established sight with an enemy (player only) that had previously eluded the squad.
#define		CHARLIE_AE_DROP_GUN			( 7 ) // charlie (probably dead) is dropping his gun.*/
#define		CHARLIE_AE_PICKUP_AK47		( 8 )
#define		CHARLIE_AE_PICKUP_SHOTGUN   ( 9 )
#define		CHARLIE_AE_PICKUP_RPG		( 10 )
#define		CHARLIE_AE_GREN_TOSS		( 11 )

//=========================================================
// Monster's body types
//=========================================================

#define CHARLIE_BODY_NVA		0
#define CHARLIE_BODY_NVA_GREEN	1
#define CHARLIE_BODY_VC			2
#define CHARLIE_BODY_VC2		3

#define	CHARLIE_WEAPON_AK47		0
//#define	CHARLIE_WEAPON_SHOTGUN	1
#define	CHARLIE_WEAPON_RPG		1
#define CHARLIE_WEAPON_NOGUN	2

#define BODY_GROUP		0
#define WEAPON_GROUP	1

#define NUM_BODIES  5
#define NUM_WEAPONS	3

//=========================================================
// Monster's clip sizes
//=========================================================

int CHARLIE_CLIP_SIZE[ NUM_WEAPONS ] = { 30, 1 };


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_CHARLIE_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_CHARLIE_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_CHARLIE_COVER_AND_RELOAD,
	SCHED_CHARLIE_SWEEP,
	SCHED_CHARLIE_FOUND_ENEMY,
	SCHED_CHARLIE_WAIT_FACE_ENEMY,
	SCHED_CHARLIE_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_CHARLIE_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_CHARLIE_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_CHARLIE_SPEAK_SENTENCE,
	TASK_CHARLIE_CHECK_FIRE,
};

//=====================
// Spawn Flags
//=====================

#define SF_CHARLIE_HANDGRENADES	0x0064


//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_CHARLIE_NOFIRE	( bits_COND_SPECIAL1 )

class CCharlie : public CSquadMonster
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
	void IdleSound ( void );
	Vector GetGunPosition( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void AK47Fire ( void );
//	void ShotgunFire ( void );
	void RPGFire ( void );
	void PrescheduleThink ( void );
	void GibMonster( void );
//	void Killed( entvars_t *pevAttacker, int iGib );
	void SpeakSentence( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

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

	int m_voicePitch;

	int		m_iBrassShell;
	int		m_iShotgunShell;

	int		m_iSentence;

	static const char *pCharlieSentences[];
};

LINK_ENTITY_TO_CLASS( monster_charlie, CCharlie );

TYPEDESCRIPTION	CCharlie::m_SaveData[] = 
{
	DEFINE_FIELD( CCharlie, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CCharlie, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CCharlie, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCharlie, m_fHandGrenades, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCharlie, m_CrouchTime, FIELD_TIME ),
	DEFINE_FIELD( CCharlie, m_Crouching, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCharlie, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CCharlie, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CCharlie, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCharlie, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CCharlie, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CCharlie, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CCharlie, CSquadMonster );

const char *CCharlie::pCharlieSentences[] = 
{
	"CH_GREN", // grenade scared charlie
	"CH_ALERT", // sees player
	"CH_MONST", // sees monster
	"CH_COVER", // running to cover
	"CH_THROW", // about to throw grenade
	"CH_CHARGE",  // running out to get the enemy
	"CH_TAUNT", // say rude things
};

enum
{
	CHARLIE_SENT_NONE = -1,
	CHARLIE_SENT_GREN = 0,
	CHARLIE_SENT_ALERT,
	CHARLIE_SENT_MONSTER,
	CHARLIE_SENT_COVER,
	CHARLIE_SENT_THROW,
	CHARLIE_SENT_CHARGE,
	CHARLIE_SENT_TAUNT,
} CHARLIE_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some charlie sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a charlie says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the charlie has 
// started moving.
//=========================================================

void CCharlie :: SpeakSentence( void )
{
	if ( m_iSentence == CHARLIE_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pCharlieSentences[ m_iSentence ], CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}


//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CCharlie :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( WEAPON_GROUP ) != CHARLIE_WEAPON_NOGUN )
	{// throw a gun if the charlie has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		switch ( pev->weapons )
		{
			case CHARLIE_WEAPON_AK47:		pGun = DropItem( "weapon_ak47", vecGunPos, vecGunAngles );		break;
//			case CHARLIE_WEAPON_SHOTGUN:	pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );	break;
			case CHARLIE_WEAPON_RPG:		pGun = DropItem( "weapon_rpg7", vecGunPos, vecGunAngles );		break;
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
// ISoundMask - Overidden for human charlies because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CCharlie :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CCharlie :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;
	
	return TRUE;
}

//=========================================================
//=========================================================
void CCharlie :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = CHARLIE_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CCharlie :: PrescheduleThink ( void )
{
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
// FCanCheckAttacks - this is overridden for human charlies
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

BOOL CCharlie :: FCanCheckAttacks ( void )
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
// CheckRangeAttack1 - overridden for Charlie, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the Charlie can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================

BOOL CCharlie :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
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
				return TRUE;
			}
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Charlie's grenade
// attack. 
//=========================================================

BOOL CCharlie :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! m_fHandGrenades)
	{
		return FALSE;
	}
	
	// if the charlie isn't moving, it's ok to check.
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

	return m_fThrowGrenade;
}


//=========================================================
// TakeDamage - overridden for the charlie because the charlie
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================

int CCharlie :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================

void CCharlie :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 150;		
		break;
	case ACT_RUN:	
		ys = 150;	
		break;
	case ACT_WALK:	
		ys = 180;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CCharlie :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fCharlieQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fCharlieQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "CH_CHECK", CHARLIE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fCharlieQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "CH_QUEST", CHARLIE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fCharlieQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "CH_IDLE", CHARLIE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fCharlieQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "CH_CLEAR", CHARLIE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "CH_ANSWER", CHARLIE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fCharlieQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the charlie because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CCharlie :: CheckAmmo ( void )
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
int	CCharlie :: Classify ( void )
{
	return	CLASS_HUMAN_CHARLIE;
}

//=========================================================
// Shoot
//=========================================================
void CCharlie :: AK47Fire ( void )
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
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_AK47 ); // shoot +-5 degrees

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// Shoot
//=========================================================
/*void CCharlie :: ShotgunFire ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	UTIL_MakeVectors ( pev->angles );
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL); 
	FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!
}
*/

void CCharlie :: RPGFire ( void )
{
	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecSrc );
	
	CBaseEntity *pRocket = CBaseEntity::Create( "rpg7_rocket", vecSrc, pev->angles, edict() );
	if (pRocket)
		pRocket->pev->velocity = pev->velocity + gpGlobals->v_forward * 100;
	
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 1, ATTN_NORM, 0, 100 );
	EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "weapons/glauncher.wav", 1, ATTN_NORM, 0, 100 );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================

void CCharlie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		//Drop gun

		case CHARLIE_AE_DROP_GUN:
			{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( WEAPON_GROUP, CHARLIE_WEAPON_NOGUN );

			switch ( pev->weapons )
			{
				case CHARLIE_WEAPON_AK47:		DropItem( "weapon_ak47", vecGunPos, vecGunAngles );		break;
//				case CHARLIE_WEAPON_SHOTGUN:	DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );	break;
				case CHARLIE_WEAPON_RPG:		DropItem( "weapon_rpg7", vecGunPos, vecGunAngles );		break;
			}
			
			}
			break;

		//Reload
			
		case CHARLIE_AE_RELOAD:
			switch( RANDOM_LONG( 0, 2 ) )
			{
				case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload1.wav", 1, ATTN_NORM ); break;
				case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload2.wav", 1, ATTN_NORM ); break;
				case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload3.wav", 1, ATTN_NORM ); break;
			}
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		//Fire weapon (in bursts if it's an AK47)

		case CHARLIE_AE_BURST1:
		{
			switch ( pev->weapons )
			{
				case CHARLIE_WEAPON_AK47:		
					AK47Fire();		
					EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/ak47_burst.wav", 1, ATTN_NORM, 0, 100 );
					break;

/*				case CHARLIE_WEAPON_SHOTGUN:	
					ShotgunFire();	
					EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM, 0, 100 );
					break;
*/
				case CHARLIE_WEAPON_RPG:		
					RPGFire();		
					break;
			}
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );
		}
		break;

		case CHARLIE_AE_BURST2:
		case CHARLIE_AE_BURST3:
//			if (pev->weapon == CHARLIE_WEAPON_AK47) 
				AK47Fire();
//			else
//				EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "weapons/870_pump.wav", 1, ATTN_NORM, 0, 100 );
			break;

		case CHARLIE_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "CH_ALERT", CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}

		case CHARLIE_AE_PICKUP_AK47:
			SetBodygroup( WEAPON_GROUP, CHARLIE_WEAPON_AK47 );
			break;

/*		case CHARLIE_AE_PICKUP_SHOTGUN:
			SetBodygroup( WEAPON_GROUP, CHARLIE_WEAPON_SHOTGUN );
			break;*/

		case CHARLIE_AE_PICKUP_RPG:
			SetBodygroup( WEAPON_GROUP, CHARLIE_WEAPON_RPG );
			break;

		case CHARLIE_AE_GREN_TOSS:
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
void CCharlie :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/charlie.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.charlieHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= CHARLIE_SENT_NONE;
	m_cClipSize			= CHARLIE_CLIP_SIZE[ pev->weapons ];
	m_cAmmoLoaded		= m_cClipSize;
	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the charlie spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	if ( pev->body == -1 )
	{// -1 chooses a random body
		pev->body = RANDOM_LONG(0, NUM_BODIES - 1 );
	}
	SetBodygroup( BODY_GROUP, pev->body );

	SetBodygroup( WEAPON_GROUP, pev->weapons ); 

	m_fHandGrenades = pev->spawnflags & SF_CHARLIE_HANDGRENADES;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCharlie :: Precache()
{
	PRECACHE_MODEL("models/charlie.mdl");

	UTIL_PrecacheOther( "rpg7_rocket" );

	PRECACHE_SOUND("weapons/reload1.wav");
	PRECACHE_SOUND("weapons/reload2.wav");
	PRECACHE_SOUND("weapons/reload3.wav");
	
	PRECACHE_SOUND("weapons/ak47_burst.wav");

//	PRECACHE_SOUND("weapons/sbarrel1.wav");
//	PRECACHE_SOUND("weapons/scock1.wav");

	PRECACHE_SOUND("weapons/rocketfire1.wav");
	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("gook/best_you_got.wav");
	PRECACHE_SOUND("gook/piss_me_off.wav");

	PRECACHE_SOUND("gook/die_yankee_choke.wav");
	PRECACHE_SOUND("gook/choke_roundeye.wav");
	PRECACHE_SOUND("gook/babble_choke.wav");

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");
}	

//=========================================================
// start task
//=========================================================
void CCharlie :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_CHARLIE_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_CHARLIE_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_CHARLIE_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// charlie no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_CHARLIE_FACE_TOSS_DIR:
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
void CCharlie :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CHARLIE_FACE_TOSS_DIR:
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
void CCharlie :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{

		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "gook/best_you_got.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "gook/piss_me_off.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CCharlie :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gook/die_yankee_choke.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gook/choke_roundeye.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gook/babble_choke.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// CharlieFail
//=========================================================
Task_t	tlCharlieFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slCharlieFail[] =
{
	{
		tlCharlieFail,
		ARRAYSIZE ( tlCharlieFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Charlie Fail"
	},
};

//=========================================================
// Charlie Combat Fail
//=========================================================
Task_t	tlCharlieCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slCharlieCombatFail[] =
{
	{
		tlCharlieCombatFail,
		ARRAYSIZE ( tlCharlieCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Charlie Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlCharlieVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
};

Schedule_t	slCharlieVictoryDance[] =
{
	{ 
		tlCharlieVictoryDance,
		ARRAYSIZE ( tlCharlieVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"CharlieVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the charlie to attack.
//=========================================================
Task_t tlCharlieEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CHARLIE_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_CHARLIE_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slCharlieEstablishLineOfFire[] =
{
	{ 
		tlCharlieEstablishLineOfFire,
		ARRAYSIZE ( tlCharlieEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"CharlieEstablishLineOfFire"
	},
};

//=========================================================
// CharlieFoundEnemy - charlie established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlCharlieFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slCharlieFoundEnemy[] =
{
	{ 
		tlCharlieFoundEnemy,
		ARRAYSIZE ( tlCharlieFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"CharlieFoundEnemy"
	},
};

//=========================================================
// CharlieCombatFace Schedule
//=========================================================
Task_t	tlCharlieCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_CHARLIE_SWEEP	},
};

Schedule_t	slCharlieCombatFace[] =
{
	{ 
		tlCharlieCombatFace1,
		ARRAYSIZE ( tlCharlieCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or charlie gets hurt.
//=========================================================
Task_t	tlCharlieSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CHARLIE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CHARLIE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CHARLIE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CHARLIE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CHARLIE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slCharlieSignalSuppress[] =
{
	{ 
		tlCharlieSignalSuppress,
		ARRAYSIZE ( tlCharlieSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_CHARLIE_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlCharlieSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slCharlieSuppress[] =
{
	{ 
		tlCharlieSuppress,
		ARRAYSIZE ( tlCharlieSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_CHARLIE_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// charlie wait in cover - we don't allow danger or the ability
// to attack to break a charlie's run to cover schedule, but
// when a charlie is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlCharlieWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slCharlieWaitInCover[] =
{
	{ 
		tlCharlieWaitInCover,
		ARRAYSIZE ( tlCharlieWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"CharlieWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlCharlieTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_CHARLIE_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_CHARLIE_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_CHARLIE_WAIT_FACE_ENEMY	},
};

Schedule_t	slCharlieTakeCover[] =
{
	{ 
		tlCharlieTakeCover1,
		ARRAYSIZE ( tlCharlieTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// toss grenade then run to cover.
//=========================================================
Task_t	tlCharlieTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slCharlieTossGrenadeCover[] =
{
	{ 
		tlCharlieTossGrenadeCover1,
		ARRAYSIZE ( tlCharlieTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};


//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlCharlieTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slCharlieTakeCoverFromBestSound[] =
{
	{ 
		tlCharlieTakeCoverFromBestSound,
		ARRAYSIZE ( tlCharlieTakeCoverFromBestSound ), 
		0,
		0,
		"CharlieTakeCoverFromBestSound"
	},
};

//=========================================================
// Charlie reload schedule
//=========================================================
Task_t	tlCharlieHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slCharlieHideReload[] = 
{
	{
		tlCharlieHideReload,
		ARRAYSIZE ( tlCharlieHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"CharlieHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlCharlieSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slCharlieSweep[] =
{
	{ 
		tlCharlieSweep,
		ARRAYSIZE ( tlCharlieSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Charlie Sweep"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// charlie's grenade toss requires the enemy be occluded.
//=========================================================

Task_t	tlCharlieRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slCharlieRangeAttack1A[] =
{
	{ 
		tlCharlieRangeAttack1A,
		ARRAYSIZE ( tlCharlieRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_CHARLIE_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// charlie's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlCharlieRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)ACT_IDLE_ANGRY		},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CHARLIE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slCharlieRangeAttack1B[] =
{
	{ 
		tlCharlieRangeAttack1B,
		ARRAYSIZE ( tlCharlieRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_CHARLIE_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};


//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// charlie's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlCharlieRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_CHARLIE_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_CHARLIE_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slCharlieRangeAttack2[] =
{
	{ 
		tlCharlieRangeAttack2,
		ARRAYSIZE ( tlCharlieRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CCharlie )
{
	slCharlieFail,
	slCharlieCombatFail,
	slCharlieVictoryDance,
	slCharlieEstablishLineOfFire,
	slCharlieFoundEnemy,
	slCharlieCombatFace,
	slCharlieSignalSuppress,
	slCharlieSuppress,
	slCharlieWaitInCover,
	slCharlieTakeCover,
	slCharlieTakeCoverFromBestSound,
	slCharlieHideReload,
	slCharlieSweep,
	slCharlieRangeAttack1A,
	slCharlieRangeAttack1B,
	slCharlieRangeAttack2,
	slCharlieTossGrenadeCover,
};

IMPLEMENT_CUSTOM_SCHEDULES( CCharlie, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CCharlie :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	char seq[40];

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		switch ( pev->weapons )
		{
			case CHARLIE_WEAPON_AK47:		
				strcpy( seq, "shootak47" );
				break;
/*			case CHARLIE_WEAPON_SHOTGUN:	
				strcpy( seq, "shootshotgun" );	
				break;*/
			case CHARLIE_WEAPON_RPG:		
				m_Crouching = FALSE;
				strcpy( seq, "shootrpg" );		
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
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CCharlie :: GetSchedule( void )
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

	// clear old sentence
	m_iSentence = CHARLIE_SENT_NONE;

	// charlies place HIGH priority on running away from danger sounds.
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
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "CH_GREN", CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
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

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else 
					{
						//!!!KELLY - the leader of a squad of charlies has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "CH_ALERT", CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "CH_MONST", CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_CHARLIE_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_CHARLIE_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_CHARLIE_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this charlie was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "CH_COVER", CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);
						m_iSentence = CHARLIE_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}

// can grenade launch

			else if ( m_fHandGrenades && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && 
				OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}

// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_CHARLIE_FOUND_ENEMY );
					}
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
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
				{
					//!!!KELLY - this charlie is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_THROW", CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HUMAN_ENGAGE ) )
				{
					//!!!KELLY - charlie cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "CH_CHARGE", CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);
						m_iSentence = CHARLIE_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_CHARLIE_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					Crouch();

					//!!!KELLY - charlie is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// charlie's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "CH_TAUNT", CHARLIE_SENTENCE_VOLUME, CHARLIE_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_CHARLIE_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CCharlie :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK2:
		{
			return &slCharlieRangeAttack2[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slCharlieTakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slCharlieTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_CHARLIE_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HUMAN_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_CHARLIE_ELOF_FAIL:
		{
			// human charlie is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_CHARLIE_ESTABLISH_LINE_OF_FIRE:
		{
			return &slCharlieEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slCharlieRangeAttack1B[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slCharlieCombatFace[ 0 ];
		}
	case SCHED_CHARLIE_WAIT_FACE_ENEMY:
		{
			return &slCharlieWaitInCover[ 0 ];
		}
	case SCHED_CHARLIE_SWEEP:
		{
			return &slCharlieSweep[ 0 ];
		}
	case SCHED_CHARLIE_COVER_AND_RELOAD:
		{
			return &slCharlieHideReload[ 0 ];
		}
	case SCHED_CHARLIE_FOUND_ENEMY:
		{
			return &slCharlieFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slCharlieFail[ 0 ];
				}
			}

			return &slCharlieVictoryDance[ 0 ];
		}
	case SCHED_CHARLIE_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slCharlieSignalSuppress[ 0 ];
			}
			else
			{
				return &slCharlieSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// charlie has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slCharlieCombatFail[ 0 ];
			}

			return &slCharlieFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}


void CCharlie::Crouch()
{
	if ( pev->spawnflags & SF_MONSTER_PREDISASTER ) return;

	m_Crouching = TRUE;
	m_CrouchTime = gpGlobals->time + RANDOM_LONG(15, 25);
	pev->view_ofs = Vector ( 0, 0, 40 );
}


int CCharlie::CheckEnemy(CBaseEntity *pEnemy)
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
void CCharlie :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// If crouching, reduce damage from explosions
	if (m_Crouching && (bitsDamageType & DMG_BLAST)) flDamage = flDamage / 2;	

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CCharlie :: GetGunPosition( )
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
