//=========================================================
//
//		Superzombie programmed for Nam by Nathan Ruck
//
//=========================================================

//=========================================================
// monster template
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"animation.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"math.h"
#include	"squadmonster.h"
#include	"rpg7.h"
#include	"human.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

// first flag is barney dying for scripted sequences?
#define		SUPERZOMBIE_AE_BURST1		( 2 )
#define		SUPERZOMBIE_AE_BURST2		( 3 )
#define		SUPERZOMBIE_AE_BURST3		( 4 )
#define		SUPERZOMBIE_AE_RELOAD		( 5 )
#define		SUPERZOMBIE_AE_BITE			( 6 )
#define		SUPERZOMBIE_AE_KICK			( 7 )

//=========================================================
// Monster's body types
//=========================================================

#define	SUPERZOMBIE_WEAPON_M16		0
#define	SUPERZOMBIE_WEAPON_SHOTGUN	1
#define	SUPERZOMBIE_WEAPON_M60		2
#define	SUPERZOMBIE_WEAPON_RPG		3
#define SUPERZOMBIE_WEAPON_NOGUN	4

#define HEAD_GROUP		0
#define BODY_GROUP		1
#define WEAPON_GROUP	2
#define AMMO_GROUP		3

#define SUPERZOMBIE_NUM_WEAPONS	5
#define SUPERZOMBIE_NUM_HEADS	2
#define SUPERZOMBIE_NUM_BODIES	2

//=====================
// Spawn Flags
//=====================

#define SF_SUPERZOMBIE_FRIENDLY		0x008


//=========================================================
// Monster's clip sizes
//=========================================================

int SUPERZOMBIE_CLIP_SIZE[ SUPERZOMBIE_NUM_WEAPONS ] = { 30, 7, 50, 1, 0 };

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SUPERZOMBIE_EAT = LAST_COMMON_SCHEDULE + 1,
	SCHED_SUPERZOMBIE_MOVE_AWAY,
};

//=====================
// Monsters Class Definition
//=====================

#define	bits_COND_PUSHED ( bits_COND_SPECIAL1 )


class CSuperZombie : public CBaseMonster
{
public:

    virtual void Spawn( void );
    virtual void Precache( void );

	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void PainSound( void );
	void EatSound( void );
    
	void SetYawSpeed ( void );
	virtual void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual void SetActivity ( Activity NewActivity );
	int	ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() | FCAP_IMPULSE_USE; };
	void BecomeDead( void );

	void M16Fire( void );
	void M60Fire( void );
	void ShotgunFire( void );
	void RPGFire( void );
	virtual void Killed( entvars_t *pevAttacker, int iGib );
	void CheckAmmo ( void );
	void RunAI( void );
	void MonsterThink ( void );
	void GetBloodDirection( Vector &pos, Vector &dir );
	void GetHeadPosition( Vector &pos, Vector &angle );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int ISoundMask ( void );
	Schedule_t* GetScheduleOfType ( int Type ) ;
	CBaseEntity *Kick( void );
	CBaseEntity *Bite( void );
	virtual int GetNumHeads() { return SUPERZOMBIE_NUM_HEADS; };
	virtual char * GetHeadModelName() { return "models/superzombiehead.mdl"; };
	
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckMeleeAttack2( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );

	BOOL IsFollowing() { return m_hTargetEnt != NULL; };
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Touch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

    int Classify ( void );
	int IRelationship( CBaseEntity *pTarget );
	BOOL HasHumanGibs() { return TRUE; };
	BOOL HasAlienGibs() { return FALSE; };

	virtual Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	int		m_cClipSize;
	int		m_iBrassShell;
	int		m_iShotgunShell;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
	float	m_LastVocalTime;
	int		m_nHeadNum;				// Index of head in model
	float   m_LastBloodTime;

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pEatSounds[];

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_superzombie, CSuperZombie );

TYPEDESCRIPTION	CSuperZombie::m_SaveData[] = 
{
	DEFINE_FIELD( CSuperZombie, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CSuperZombie, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CSuperZombie, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSuperZombie, m_cClipSize, FIELD_FLOAT ),
	DEFINE_FIELD( CSuperZombie, m_nHeadNum, FIELD_INTEGER ),
	DEFINE_FIELD( CSuperZombie, m_LastBloodTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CSuperZombie, CBaseMonster );

const char *CSuperZombie::pAttackSounds[] = 
{
	"superzombie/zo_attack1.wav",
	"superzombie/zo_attack2.wav",
	"superzombie/zo_attack3.wav",
	"superzombie/zo_attack4.wav",
};

const char *CSuperZombie::pIdleSounds[] = 
{
	"superzombie/zo_idle1.wav",
	"superzombie/zo_idle2.wav",
	"superzombie/zo_idle3.wav",
	"superzombie/zo_idle4.wav",
	"superzombie/zo_idle5.wav",
	"superzombie/zo_idle6.wav",
	"superzombie/zo_idle7.wav",
	"superzombie/zo_idle8.wav",
};

const char *CSuperZombie::pAlertSounds[] = 
{
	"superzombie/zo_alert10.wav",
	"superzombie/zo_alert20.wav",
	"superzombie/zo_alert30.wav",
};

const char *CSuperZombie::pPainSounds[] = 
{
	"superzombie/zo_pain1.wav",
	"superzombie/zo_pain2.wav",
};

const char *CSuperZombie::pEatSounds[] = 
{
	"superzombie/eat1.wav",
	"superzombie/eat2.wav",
};


void CSuperZombie :: StartTask( Task_t *pTask )
{

	switch ( pTask->iTask )
	{
	
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	default:
		CBaseMonster::StartTask( pTask );	
		break;
	}

}

void CSuperZombie :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CBaseMonster::RunTask( pTask );
		break;

	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================

int    CSuperZombie :: Classify ( void )
{
	if ( FBitSet( pev->spawnflags, SF_SUPERZOMBIE_FRIENDLY ) )
	{
		return CLASS_PLAYER_ALLY;
	}
	else
	{
		return	CLASS_ALIEN_PREDATOR;
	}
}


//=========================================================
// IRelationship - do I love or hate pTarget?
//=========================================================

int CSuperZombie::IRelationship( CBaseEntity *pTarget )
{
	if ( pev->weapons == SUPERZOMBIE_WEAPON_RPG && pTarget->Classify() == CLASS_HELICOPTER )
		return R_HT;

	return CBaseMonster::IRelationship( pTarget );
}


void CSuperZombie :: PainSound( void )
{
	if ( m_nHeadNum == GetNumHeads() ) return;	// Don't make noise if you don't have a head

	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM );
}

void CSuperZombie :: AlertSound( void )
{
	if ( m_nHeadNum == GetNumHeads() ) return;	// Don't make noise if you don't have a head

	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM );
}

void CSuperZombie :: IdleSound( void )
{
	if ( m_nHeadNum == GetNumHeads() ) return;	// Don't make noise if you don't have a head

	// Play a random idle sound
	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM );
}

void CSuperZombie :: AttackSound( void )
{
	if ( m_nHeadNum == GetNumHeads() ) return;	// Don't make noise if you don't have a head

	// Play a random attack sound
	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM );
}


void CSuperZombie :: EatSound( void )
{
	if ( m_nHeadNum == GetNumHeads() ) return;	// Don't make noise if you don't have a head

	// Play a random eat sound
	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pEatSounds[ RANDOM_LONG(0,ARRAYSIZE(pEatSounds)-1) ], 1.0, ATTN_NORM );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================

void CSuperZombie :: SetYawSpeed ( void )
{
    int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 50;
		break;
	case ACT_WALK:
		ys = 40;
		break;
	case ACT_RUN:
		ys = 40;
		break;
	default:
		ys = 50;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckAmmo - overridden for SuperZombie because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CSuperZombie :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================

BOOL CSuperZombie :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( ( flDist <= 1024 || ( m_hEnemy != NULL && m_hEnemy->Classify() == CLASS_HELICOPTER ) ) && flDot >= 0.5 )
	{
		// Prefer to chase unless we've been doing it for a long time already or are being shot
		if ( IsMoving() && !HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) 
			&& gpGlobals->time < m_checkAttackTime + 5 )
		{
			return FALSE;
		}

		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


//=========================================================
// CheckMeleeAttack1
//=========================================================

BOOL CSuperZombie :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_nHeadNum == GetNumHeads() ) return FALSE; // Cannot bite if I have no head
	
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	// Also check whether the enemy is horizontally in range of my mouth, since this is a bite attack

	if ( flDist <= 50 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) &&
		m_hEnemy->pev->absmax.z >= EyePosition().z && m_hEnemy->pev->absmin.z <= EyePosition().z )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================

BOOL CSuperZombie :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	// Also check whether the enemy is horizontally in range of my foot, since this is a kick attack

	if ( flDist <= 50 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) &&
		m_hEnemy->pev->absmax.z >= (pev->origin.z + pev->size.z*0.5) && m_hEnemy->pev->absmin.z <= (pev->origin.z + pev->size.z*0.5) )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================

void CSuperZombie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case SUPERZOMBIE_AE_BURST1:
		switch ( pev->weapons )
		{
			case SUPERZOMBIE_WEAPON_M16:		M16Fire();		break;
			case SUPERZOMBIE_WEAPON_SHOTGUN:	ShotgunFire();	break;
			case SUPERZOMBIE_WEAPON_M60:		M60Fire();		break;
			case SUPERZOMBIE_WEAPON_RPG:		RPGFire();		break;
		}
		break;

	case SUPERZOMBIE_AE_BURST2:
		if ( pev->weapons == SUPERZOMBIE_WEAPON_M16 ) M16Fire();
		break;

	case SUPERZOMBIE_AE_BURST3:
		switch ( pev->weapons )
		{
			case SUPERZOMBIE_WEAPON_M16:		
				M16Fire();		
				break;

			case SUPERZOMBIE_WEAPON_SHOTGUN:	
				EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "weapons/870_pump.wav", 1, ATTN_NORM, 0, 100 );
				break;
		}
		break;

	case SUPERZOMBIE_AE_RELOAD:
		switch( RANDOM_LONG( 0, 2 ) )
		{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload3.wav", 1, ATTN_NORM ); break;
		}
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case SUPERZOMBIE_AE_BITE:
		{
			CBaseEntity *pHurt = Bite();

			if ( pHurt )
			{
				pHurt->TakeDamage( pev, pev, gSkillData.superzombieDmgBite, DMG_CLUB );
			}
		}
		break;

	case SUPERZOMBIE_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "zombie/claw_miss2.wav", 1, ATTN_NORM );
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.superzombieDmgKick, DMG_CLUB );
			}
		}
		break;
	
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
	}
}


//=========================================================
// Fire procedures - Shoots one round from designated weapon 
// at the enemy SuperZombie is facing.
//=========================================================


void CSuperZombie :: M16Fire ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_MONSTER_M16 ); // shoot +-5 degrees

	m_cAmmoLoaded--;// take away a bullet!

	switch ( RANDOM_LONG( 0, 2 ) ) {
		case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m16_fire1.wav", 1, ATTN_NORM, 0, 100 ); break;
		case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m16_fire2.wav", 1, ATTN_NORM, 0, 100 ); break;
		case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m16_fire3.wav", 1, ATTN_NORM, 0, 100 ); break;
	}

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );

	pev->effects |= EF_MUZZLEFLASH;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	
}


void CSuperZombie :: ShotgunFire( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL); 

	FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	m_cAmmoLoaded--;// take away a bullet!

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/870_buckshot.wav", 1, ATTN_NORM, 0, 100 );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );
	
	pev->effects |= EF_MUZZLEFLASH;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	
}


void CSuperZombie:: M60Fire( void )
{

	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_762 ); // shoot +-5 degrees

	m_cAmmoLoaded--;// take away a bullet!

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m60_fire.wav", 1, ATTN_NORM, 0, 100 );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );

	pev->effects |= EF_MUZZLEFLASH;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}


		
void CSuperZombie :: RPGFire( void )
{

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + Vector( 0, 0, 55 );

	Vector vecShootDir = ShootAtEnemy( vecSrc );
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	angDir.x = -angDir.x;
	
	CRpg7Rocket *pRocket = CRpg7Rocket::CreateRocket( vecSrc, angDir, this );
		
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 1, ATTN_NORM, 0, 100 );
	EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "weapons/glauncher.wav", 1, ATTN_NORM, 0, 100 );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );

	m_cAmmoLoaded--;// take away a bullet!

}

		
//=========================================================
// Spawn
//=========================================================

void CSuperZombie :: Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/superzombie.mdl");

	// The rest of this method was taken from the Barney spawn() method
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
    pev->effects        = 0;
	pev->health			= gSkillData.superzombieHealth;
	pev->view_ofs		= Vector ( 0, 0, 63 );// position of the eyes relative to monster's origin.
    pev->yaw_speed      = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_cClipSize			= SUPERZOMBIE_CLIP_SIZE[ pev->weapons ];
	m_cAmmoLoaded		= m_cClipSize;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	if ( pev->body == -1 )
	{// -1 chooses a random body
		m_nHeadNum = RANDOM_LONG(0, GetNumHeads() - 1 ); 
	}
	else
	{
		m_nHeadNum = pev->body;
	}
	
	SetBodygroup( HEAD_GROUP, m_nHeadNum );
	SetBodygroup( BODY_GROUP, m_nHeadNum );
	SetBodygroup( WEAPON_GROUP, pev->weapons );

	if (pev->weapons == SUPERZOMBIE_WEAPON_M60)
		SetBodygroup(AMMO_GROUP, TRUE);
	else
		SetBodygroup(AMMO_GROUP, FALSE);

	MonsterInit();
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================

void CSuperZombie :: Precache()
{
	int i;
    
	PRECACHE_MODEL("models/superzombie.mdl");
	PRECACHE_MODEL(GetHeadModelName());
	
	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl"); //shotgun shell

	UTIL_PrecacheOther( "rpg7_rocket" );

	PRECACHE_SOUND("weapons/reload1.wav");
	PRECACHE_SOUND("weapons/reload2.wav");
	PRECACHE_SOUND("weapons/reload3.wav");
	
	PRECACHE_SOUND("weapons/m16_fire1.wav");
	PRECACHE_SOUND("weapons/m16_fire2.wav");
	PRECACHE_SOUND("weapons/m16_fire3.wav");

	PRECACHE_SOUND("weapons/m60_fire.wav");

	PRECACHE_SOUND("weapons/870_pump.wav");
	PRECACHE_SOUND("weapons/870_buckshot.wav");

	PRECACHE_SOUND("weapons/rocketfire1.wav");
	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");	// kick

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pEatSounds ); i++ )
		PRECACHE_SOUND((char *)pEatSounds[i]);
}    


//=========================================================
// SetActivity 
//=========================================================

void CSuperZombie :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
		case ACT_RANGE_ATTACK1:
			switch ( pev->weapons )
			{
				case SUPERZOMBIE_WEAPON_M16:		iSequence = LookupSequence( "shootm16" );		break;
				case SUPERZOMBIE_WEAPON_SHOTGUN:	iSequence = LookupSequence( "shootshotgun" );	break;
				case SUPERZOMBIE_WEAPON_M60:		iSequence = LookupSequence( "shootm60" );		break;
				case SUPERZOMBIE_WEAPON_RPG:		iSequence = LookupSequence( "shootrpg7" );		break;
			}
			break;

		case ACT_WALK:
			{
				if ( pev->health < pev->max_health / 3 ) 
				{
					iSequence = LookupActivity( ACT_WALK_HURT );
				}
				else
				{
					iSequence = LookupActivity( ACT_WALK );
				}
			}
			break;

		default:
			iSequence = LookupActivity ( NewActivity );
			break;
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


void CSuperZombie::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( pev->weapons < SUPERZOMBIE_WEAPON_NOGUN )
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		switch ( pev->weapons )
		{
			case SUPERZOMBIE_WEAPON_M16:		pGun = DropItem( "weapon_m16", vecGunPos, vecGunAngles );		break;
			case SUPERZOMBIE_WEAPON_SHOTGUN:	pGun = DropItem( "weapon_870", vecGunPos, vecGunAngles );	break;
			case SUPERZOMBIE_WEAPON_M60:		pGun = DropItem( "weapon_m60", vecGunPos, vecGunAngles );		break;
			case SUPERZOMBIE_WEAPON_RPG:		pGun = DropItem( "weapon_rpg7", vecGunPos, vecGunAngles );		break;
		}
		
		pev->weapons = SUPERZOMBIE_WEAPON_NOGUN;
		SetBodygroup( WEAPON_GROUP, pev->weapons );
	}

	CBaseMonster::Killed( pevAttacker, iGib );
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================

Schedule_t *CSuperZombie :: GetSchedule ( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// Dead or new enemy code centralised in BaseMonster
			if ( HasConditions( bits_COND_ENEMY_DEAD | bits_COND_NEW_ENEMY ) )
			{
				return CBaseMonster::GetSchedule();
			}
			// If hit with chainsaw or machete, always flinch
			else if ( HasConditions(bits_COND_LIGHT_DAMAGE) && 
				( !HasMemory( bits_MEMORY_FLINCHED) || ( FBitSet( m_bitsDamageType, DMG_SLASH ) )  ) )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			//If he has no ammo left then reload
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED )  && pev->weapons!=SUPERZOMBIE_WEAPON_NOGUN )
			{
				return GetScheduleOfType ( SCHED_RELOAD );
			}
			else if ( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				if ( HasConditions(bits_COND_CAN_MELEE_ATTACK1) )	// Prefer biting
				{
					AttackSound();
					return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
				}
				if ( HasConditions(bits_COND_CAN_MELEE_ATTACK2) )	// to kicking
				{
					AttackSound();
					return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
				}
				if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) )	// and last of all shooting
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
			}
			else if ( HasConditions( bits_COND_SMELL_FOOD ) && HasConditions( bits_COND_ENEMY_OCCLUDED )
				 && ( m_nHeadNum != GetNumHeads() ) )	// Foooood
			{
				return GetScheduleOfType( SCHED_SUPERZOMBIE_EAT );
			}
		}
		break;

	case MONSTERSTATE_ALERT:
		if ( HasConditions( bits_COND_ENEMY_DEAD ) && ( m_nHeadNum != GetNumHeads() ) )
		{
			return GetScheduleOfType ( SCHED_VICTORY_DANCE );
		}


	case MONSTERSTATE_IDLE:
		{
			if ( HasConditions(bits_COND_SMELL_FOOD) && ( m_nHeadNum != GetNumHeads() ) )	//Smell Food?
			{
				return GetScheduleOfType( SCHED_SUPERZOMBIE_EAT );
			}

			if ( HasConditions( bits_COND_PUSHED ) )
			{
				return GetScheduleOfType( SCHED_SUPERZOMBIE_MOVE_AWAY );
			}

			if ( IsFollowing() && m_hEnemy == NULL )
			{
				if ( !m_hTargetEnt->IsAlive() )
				{
					m_hTargetEnt = NULL;
				}
				else
				{
					return GetScheduleOfType( SCHED_TARGET_FACE );
				}
			}
		}
		break;
	}

	return CBaseMonster::GetSchedule();
}


MONSTERSTATE CSuperZombie :: GetIdealState ( void )
{
	return CBaseMonster::GetIdealState();
}

void CSuperZombie::RunAI( void )
{
	// Override because we want this kid to be more vocal than the others
	// cos he's so ominous and stuff
	
/*	if ( m_LastVocalTime + 4 < gpGlobals->time && RANDOM_LONG(0,20) == 0 && !(pev->flags & SF_MONSTER_GAG) )
	{
		if ( m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_ALERT ) 
		{
			IdleSound();
		}
		else if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			AttackSound();
		}

		m_LastVocalTime = gpGlobals->time;
	}
*/
	CBaseMonster::RunAI();
}


//=========================================================
// TraceAttack - for chopping off heads
//=========================================================

void CSuperZombie::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( (ptr->iHitgroup == HITGROUP_HEAD) && (m_nHeadNum != GetNumHeads()) && (bitsDamageType & DMG_SLASH) )
	{
		// If hit by blow from machete or chainsaw to head, chop off head.... but stay living!

		CHumanHead * pHead = GetClassPtr( (CHumanHead *)NULL );
		pHead->Spawn( GetHeadModelName(), m_nHeadNum );
			
		pHead->pev->velocity = vecDir;
		pHead->pev->velocity = pHead->pev->velocity * RANDOM_FLOAT ( 100, 150 );
		pHead->pev->velocity.z += 300;
			
		pHead->pev->avelocity = Vector( RANDOM_LONG(0, 200), RANDOM_LONG(0, 200), RANDOM_LONG(0, 200));

		GetHeadPosition( pHead->pev->origin, pHead->pev->angles );
			
		m_nHeadNum = GetNumHeads();
		SetBodygroup( HEAD_GROUP, m_nHeadNum );
		SetActivity( m_Activity );
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// GetHeadPosition - what it says
//=========================================================

void CSuperZombie :: GetHeadPosition( Vector &pos, Vector &angle )
{
	GetAttachment(1, pos, angle);
}


//=========================================================
// GetBloodDirection - for when blood is spurting out of neck
//=========================================================

void CSuperZombie :: GetBloodDirection( Vector &pos, Vector &dir )
{
	Vector angle, head;

	GetAttachment(1, head, angle);
	GetAttachment(2, pos, angle);

	dir = head - pos;
}


//=========================================================
// MonsterThink - if head is cut off we need to spawn some blood here
//=========================================================

void CSuperZombie :: MonsterThink ( void )
{
	if (m_nHeadNum == GetNumHeads() && gpGlobals->time > m_LastBloodTime + 0.4)
	{
		Vector pos, dir;
		GetBloodDirection( pos, dir );

		UTIL_BloodStream( pos, dir, 70, 100 );
		m_LastBloodTime = gpGlobals->time;

		if ( pev->health > 0 )
		{
			pev->health--;	// HACKHACK - Just lower health without going through the TakeDamage proc
							// because we don't want him to react every time, only when it kills him
		}
		else
		{
			TakeDamage( pev, pev, 1, DMG_GENERIC );
		}
	}
	
	CBaseMonster::MonsterThink();
}


//=========================================================
// Use - Toggle follow on and off
//=========================================================

void CSuperZombie::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( Classify() == CLASS_PLAYER_ALLY && 
		 pCaller != NULL && 
		 pCaller->IsPlayer() && 
		 m_MonsterState != MONSTERSTATE_SCRIPT )
	{
		if ( !IsFollowing() )
		{
			m_hTargetEnt = pCaller;
		}
		else
		{
			m_hTargetEnt = NULL;
		}
		IdleSound();
		ClearSchedule();
	}
}


//=========================================================
// Touched
//=========================================================

void CSuperZombie :: Touch( CBaseEntity *pOther )
{
	// Did a friendly touch me?
	if ( IRelationship( pOther ) == R_AL )
	{
		// Heuristic for determining if the other guy is pushing me away
		float speed = fabs(pOther->pev->velocity.x) + fabs(pOther->pev->velocity.y);
		if ( speed > 50 )
		{
			SetConditions( bits_COND_PUSHED );
			MakeIdealYaw( pOther->pev->origin );
		}
	}
}


//=========================================================
// Zombie walks to something tasty and eats it.
//=========================================================

Task_t tlSuperZombieEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_DONT_EAT,				(float)10				},// this is in case the squid can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_SOUND_EAT,				(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)20				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSuperZombieEat[] =
{
	{
		tlSuperZombieEat,
		ARRAYSIZE( tlSuperZombieEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SuperZombieEat"
	}
};


//=========================================================
// Zombie walks to enemy corpse and eats it.
//=========================================================

Task_t tlSuperZombieVictoryDance[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,(float)0				},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_FACE_ENEMY,				(float)0				},
	{ TASK_SOUND_EAT,				(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)20				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSuperZombieVictoryDance[] =
{
	{
		tlSuperZombieVictoryDance,
		ARRAYSIZE( tlSuperZombieVictoryDance ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SuperZombieVictoryDance"
	}
};


//=========================================================
// TargetChase - Move to within 128 of target ent
//=========================================================

Task_t	tlTargetChase[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128					},	// Move within 128 of target ent
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE	},
};

Schedule_t	slTargetChase[] =
{
	{
		tlTargetChase,
		ARRAYSIZE ( tlTargetChase ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PUSHED |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"TargetChase"
	},
};


//=========================================================
// Follow the target ent
//=========================================================

Task_t	tlTargetFace[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slTargetFace[] =
{
	{
		tlTargetFace,
		ARRAYSIZE ( tlTargetFace ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PUSHED |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"TargetFace"
	},
};


//=========================================================
// MoveAwayFollow
//=========================================================

Task_t	tlSuperZombieMoveAway[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_TARGET_FACE	},
	{ TASK_STORE_LASTPOSITION,		(float)0					},
	{ TASK_MOVE_AWAY_PATH,			(float)128					},
	{ TASK_WALK_PATH_FOR_UNITS,		(float)128					},
	{ TASK_STOP_MOVING,				(float)0					},
};

Schedule_t	slSuperZombieMoveAway[] =
{
	{
		tlSuperZombieMoveAway,
		ARRAYSIZE ( tlSuperZombieMoveAway ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"MoveAway"
	},
};


DEFINE_CUSTOM_SCHEDULES( CSuperZombie ) 
{
	slSuperZombieEat,
	slSuperZombieVictoryDance,
	slSuperZombieMoveAway,
	slTargetChase,
	slTargetFace,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSuperZombie, CBaseMonster );


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================

int CSuperZombie :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}


//=========================================================
// GetScheduleOfType
//=========================================================

Schedule_t* CSuperZombie :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_SUPERZOMBIE_EAT:
		{
			return &slSuperZombieEat[ 0 ];
		}
		break;

	case SCHED_VICTORY_DANCE:
		{
			return &slSuperZombieVictoryDance[ 0 ];
		}
		break;

	case SCHED_SUPERZOMBIE_MOVE_AWAY:
		{
			return &slSuperZombieMoveAway[ 0 ];
		}
		break;

	case SCHED_TARGET_CHASE:
		{
			return &slTargetChase[ 0 ];
		}
		break;
	
	case SCHED_TARGET_FACE:
		{
			return &slTargetFace[ 0 ];
		}
		break;

	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}


//=========================================================
// Kick and return if you hit anything
//=========================================================

CBaseEntity *CSuperZombie :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}


//=========================================================
// Bite and return if you hit anything
//=========================================================

CBaseEntity *CSuperZombie :: Bite( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = EyePosition();
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}


//=========================================================
//
//	Barneyzombie programmed for Nam by Nathan Ruck
//
//=========================================================

#define BARNEYZOMBIE_NUM_HEADS 1
#define BARNEYZOMBIE_AE_HEAD 9

class CBarneyZombie : public CSuperZombie
{
    void Spawn( void );
    void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void Killed( entvars_t *pevAttacker, int iGib );
	void SetActivity ( Activity NewActivity );
    
	int Classify ( void ) { return CLASS_PLAYER_ALLY; };
	int GetNumHeads() { return BARNEYZOMBIE_NUM_HEADS; };
	char * GetHeadModelName() { return "models/barneyzombiehead.mdl"; };
};

LINK_ENTITY_TO_CLASS( monster_barneyzombie, CBarneyZombie );

//=========================================================
// Spawn
//=========================================================

void CBarneyZombie :: Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/barneyzombie.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
    pev->effects        = 0;
	pev->health			= gSkillData.superzombieHealth;
	pev->view_ofs		= Vector ( 0, 0, 63 );// position of the eyes relative to monster's origin.
    pev->yaw_speed      = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_cClipSize			= 7;
	m_cAmmoLoaded		= m_cClipSize;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_nHeadNum = 0;
	SetBodygroup( HEAD_GROUP, 1 );
	SetBodygroup( BODY_GROUP, 0 );
	SetBodygroup( WEAPON_GROUP, 0 );

	MonsterInit();
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================

void CBarneyZombie :: Precache()
{
	int i;
    
	PRECACHE_MODEL("models/barneyzombie.mdl");
	PRECACHE_MODEL(GetHeadModelName());
	
	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("weapons/colt_fire1.wav");
	PRECACHE_SOUND("weapons/colt_fire2.wav");

	PRECACHE_SOUND("weapons/reload1.wav");
	PRECACHE_SOUND("weapons/reload2.wav");
	PRECACHE_SOUND("weapons/reload3.wav");
	
	PRECACHE_SOUND("zombie/claw_miss2.wav");	// kick

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pEatSounds ); i++ )
		PRECACHE_SOUND((char *)pEatSounds[i]);
}    


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================

void CBarneyZombie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case SUPERZOMBIE_AE_BURST1:
		{
			Vector vecShootOrigin;

			UTIL_MakeVectors(pev->angles);
			vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
			Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

			Vector angDir = UTIL_VecToAngles( vecShootDir );
			SetBlending( 0, angDir.x );
			pev->effects = EF_MUZZLEFLASH;

			FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 1024, BULLET_MONSTER_1143 );
	
			int pitchShift = RANDOM_LONG( 0, 20 );
	
			if ( RANDOM_LONG(0, 1) )
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/colt_fire1.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/colt_fire2.wav", 1, ATTN_NORM );

			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );

			// UNDONE: Reload?
			m_cAmmoLoaded--;// take away a bullet!
		}
		break;

	case BARNEYZOMBIE_AE_HEAD:
		{
			SetBodygroup( HEAD_GROUP, 0 );	
			m_nHeadNum = 0;
			CBaseEntity *pPlayer = NULL;
			pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
			if (pPlayer)
			{
				m_hTargetEnt = pPlayer;
			}
		}
		break;

	default:
		CSuperZombie::HandleAnimEvent( pEvent );
		break;

	}
}


//=========================================================
// Killed
//=========================================================

void CBarneyZombie::Killed( entvars_t *pevAttacker, int iGib )
{
	Vector vecGunPos;
	Vector vecGunAngles;

	GetAttachment( 0, vecGunPos, vecGunAngles );
		
	CBaseEntity *pGun;
	pGun = DropItem( "weapon_colt1911a1", vecGunPos, vecGunAngles );
	SetBodygroup( WEAPON_GROUP, 1 );

	CBaseMonster::Killed( pevAttacker, iGib );
}


//=========================================================
// SetActivity - over-rides superzombie for shoot anim
//=========================================================

void CBarneyZombie :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
		case ACT_RANGE_ATTACK1:
			iSequence = LookupActivity( ACT_RANGE_ATTACK1 );
			break;

		default:
			CSuperZombie::SetActivity( NewActivity );
			return;
			break;
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


void CSuperZombie::BecomeDead( void )
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.
	
	// give the corpse half of the monster's original maximum health. 
	pev->health = 40;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.

	// make the corpse fly away from the attack vector
	pev->movetype = MOVETYPE_TOSS;
}

