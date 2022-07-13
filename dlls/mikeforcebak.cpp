//=========================================================
//
//		Mikeforce guy programmed for Nam by Nathan Ruck
//
//=========================================================

//=========================================================
// monster template
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"animation.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

// first flag is mikeforce dying for scripted sequences?
#define		MIKEFORCE_AE_BURST1		( 2 )
#define		MIKEFORCE_AE_BURST2		( 3 )
#define		MIKEFORCE_AE_BURST3		( 4 )
#define		MIKEFORCE_AE_RELOAD		( 5 )
#define		MIKEFORCE_AE_PICKUP_M16		( 8 )
#define		MIKEFORCE_AE_PICKUP_SHOTGUN   ( 9 )
#define		MIKEFORCE_AE_PICKUP_M60		( 10 )
#define		MIKEFORCE_AE_GREN_TOSS		( 11 )
#define		MIKEFORCE_AE_WHISKY_SHOW	( 12 )
#define		MIKEFORCE_AE_WHISKY_HIDE	( 13 )

//=========================================================
// Monster's body types
//=========================================================

#define	MIKEFORCE_BODY_M16		0
#define	MIKEFORCE_BODY_SHOTGUN	1
#define	MIKEFORCE_BODY_M60		2
#define MIKEFORCE_BODY_NOGUN	3

#define BODY_GROUP	 0
#define GUN_GROUP	 1
#define WHISKY_GROUP 2
#define AMMO_GROUP	 3

#define NUM_WEAPONS	4
#define NUM_BODIES  5

//=========================================================
// Monster's clip sizes
//=========================================================

int MIKEFORCE_CLIP_SIZE[ NUM_WEAPONS ] = { 30, 7, 50, 0 };

//=========================================================
// monster-specific schedule types
//=========================================================

enum
{
	SCHED_MIKEFORCE_COVER_AND_RELOAD = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_MIKEFORCE_REPEL,
	SCHED_MIKEFORCE_REPEL_ATTACK,
	SCHED_MIKEFORCE_REPEL_LAND,
};

//=========================================================
// monster-specific task types
//=========================================================

enum 
{
	TASK_MIKEFORCE_FACE_TOSS_DIR = LAST_TALKMONSTER_TASK + 1,
};

//=====================
// Spawn Flags
//=====================

#define SF_MIKEFORCE_HANDGRENADES	0x0064

//=====================
// Monsters Class Definition
//=====================

class CMikeForce : public CTalkMonster
{
public:

    void Spawn( void );
    void Precache( void );
	void AlertSound( void );
    void SetYawSpeed ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	void SetActivity ( Activity NewActivity );
	void TalkInit( void );
	void DeclineFollowing( void );
	void DeathSound( void );
	void PainSound( void );
	void M16Fire( void );
	void ShotgunFire( void );
	void M60Fire( void );
	void PickUpGun( int gun );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	void CheckAmmo ( void );
	void Crouch( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	Vector GetGunPosition( );
	BOOL FCanCheckAttacks ( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
    int Classify ( void );
	int ISoundMask( void );
	int CheckEnemy ( CBaseEntity *pEnemy );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	int		m_cClipSize;
	int		m_iBrassShell;
	int		m_iShotgunShell;
	float	m_painTime;
	float	m_flPlayerDamage;   // how much pain has the player inflicted on me?
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	int		m_CrouchTime;		// Time till I can stop crouching (provided no further danger is sensed)
	BOOL	m_Crouching;		// am I crouching?

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	Vector	m_vecTossVelocity;
	BOOL	m_fThrowGrenade;
	BOOL	m_fHandGrenades;

	void PlayerCommand( int iCommand );

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_mikeforce, CMikeForce );

TYPEDESCRIPTION	CMikeForce::m_SaveData[] = 
{
	DEFINE_FIELD( CMikeForce, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CMikeForce, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CMikeForce, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMikeForce, m_CrouchTime, FIELD_TIME ),
	DEFINE_FIELD( CMikeForce, m_Crouching, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMikeForce, m_fHandGrenades, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMikeForce, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CMikeForce, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CMikeForce, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMikeForce, m_cClipSize, FIELD_FLOAT ),
	DEFINE_FIELD( CMikeForce, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CMikeForce, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Task_t	tlMFFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slMFFollow[] =
{
	{
		tlMFFollow,
		ARRAYSIZE ( tlMFFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};


Task_t	tlMFFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slMFFaceTarget[] =
{
	{
		tlMFFaceTarget,
		ARRAYSIZE ( tlMFFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleMFStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleMFStand[] =
{
	{ 
		tlIdleMFStand,
		ARRAYSIZE ( tlIdleMFStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};


//=========================================================
// MikeForce reload schedule
//=========================================================
Task_t	tlMFHideReload[] =
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

Schedule_t slMFHideReload[] = 
{
	{
		tlMFHideReload,
		ARRAYSIZE ( tlMFHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"MFHideReload"
	}
};


//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlMFRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_MIKEFORCE_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
};

Schedule_t	slMFRangeAttack2[] =
{
	{ 
		tlMFRangeAttack2,
		ARRAYSIZE ( tlMFRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// toss grenade then run to cover.
//=========================================================
Task_t	tlMFTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slMFTossGrenadeCover[] =
{
	{ 
		tlMFTossGrenadeCover1,
		ARRAYSIZE ( tlMFTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};



//=========================================================
// repel 
//=========================================================
Task_t	tlMikeForceRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slMikeForceRepel[] =
{
	{ 
		tlMikeForceRepel,
		ARRAYSIZE ( tlMikeForceRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel"
	},
};

//=========================================================
// repel 
//=========================================================
Task_t	tlMikeForceRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slMikeForceRepelAttack[] =
{
	{ 
		tlMikeForceRepelAttack,
		ARRAYSIZE ( tlMikeForceRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlMikeForceRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slMikeForceRepelLand[] =
{
	{ 
		tlMikeForceRepelLand,
		ARRAYSIZE ( tlMikeForceRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};


DEFINE_CUSTOM_SCHEDULES( CMikeForce )
{
	slMFFollow,
	slMFFaceTarget,
	slIdleMFStand,
	slMFHideReload,
	slMFRangeAttack2,
	slMFTossGrenadeCover,
	slMikeForceRepel,
	slMikeForceRepelAttack,
	slMikeForceRepelLand,
};


IMPLEMENT_CUSTOM_SCHEDULES( CMikeForce, CTalkMonster );

void CMikeForce :: StartTask( Task_t *pTask )
{

	switch ( pTask->iTask )
	{
	
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_MIKEFORCE_FACE_TOSS_DIR:
		break;

	default:
		CTalkMonster::StartTask( pTask );	
		break;
	}

}

void CMikeForce :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;

	case TASK_MIKEFORCE_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
		}
		break;

	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CMikeForce :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int    CMikeForce :: Classify ( void )
{
    return    CLASS_PLAYER_ALLY;
}


//=========================================================
// ALertSound - monster says "Freeze!"
//=========================================================
void CMikeForce :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "MF_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}

}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================

void CMikeForce :: SetYawSpeed ( void )
{
    int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckAmmo - overridden for MikeForce because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CMikeForce :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================

BOOL CMikeForce :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = GetGunPosition();
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			
			else if (m_Crouching)
			{
				// If he's crouching check whether he could hit something if he stood up

				Vector shootOrigin = pev->origin + Vector( 0, 0, 62 );
				CBaseEntity *pEnemy = m_hEnemy;
				Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
				UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
				m_checkAttackTime = gpGlobals->time + 1;
			
				if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				{
					m_Crouching = FALSE;
					pev->view_ofs = Vector ( 0, 0, 68 );
					m_lastAttackCheck = TRUE;
				}
				else
					m_lastAttackCheck = FALSE;
			}
			else
				m_lastAttackCheck = FALSE;
			
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


void CMikeForce :: PickUpGun( int gun )
{
	pev->weapons = gun;
	SetBodygroup( GUN_GROUP, pev->weapons );
	EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/gunpickup1.wav", 1, ATTN_NORM, 0, 100 );
	m_cClipSize			= MIKEFORCE_CLIP_SIZE[ pev->weapons ];
	m_cAmmoLoaded		= m_cClipSize;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================

void CMikeForce :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case MIKEFORCE_AE_BURST1:
		switch ( pev->weapons )
		{
			case MIKEFORCE_BODY_M16:		
				M16Fire();		
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m16_burst.wav", 1, ATTN_NORM, 0, 100 );
				CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );
				break;
			case MIKEFORCE_BODY_SHOTGUN:	ShotgunFire();	break;
			case MIKEFORCE_BODY_M60:		M60Fire();		break;
		}
		break;

	case MIKEFORCE_AE_BURST2:
		if ( pev->weapons == MIKEFORCE_BODY_M16 ) M16Fire();
		break;

	case MIKEFORCE_AE_BURST3:
		switch ( pev->weapons )
		{
			case MIKEFORCE_BODY_M16:		
				M16Fire();		
				break;

			case MIKEFORCE_BODY_SHOTGUN:	
				EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "weapons/870_pump.wav", 1, ATTN_NORM, 0, 100 );
				break;
		}
		break;

	case MIKEFORCE_AE_RELOAD:
		switch( RANDOM_LONG( 0, 2 ) )
		{
			case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload1.wav", 1, ATTN_NORM ); break;
			case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload2.wav", 1, ATTN_NORM ); break;
			case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload3.wav", 1, ATTN_NORM ); break;
		}
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case MIKEFORCE_AE_PICKUP_M16:
		PickUpGun(MIKEFORCE_BODY_M16);
		break;
		
	case MIKEFORCE_AE_PICKUP_SHOTGUN:
		PickUpGun(MIKEFORCE_BODY_SHOTGUN);
		break;

	case MIKEFORCE_AE_PICKUP_M60:
		PickUpGun(MIKEFORCE_BODY_M60);
		SetBodygroup(AMMO_GROUP, TRUE);
		break;

	case MIKEFORCE_AE_GREN_TOSS:
	{
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
	}
	break;

	case MIKEFORCE_AE_WHISKY_SHOW:
		SetBodygroup( WHISKY_GROUP, TRUE );
		break;

	case MIKEFORCE_AE_WHISKY_HIDE:
		SetBodygroup( WHISKY_GROUP, FALSE );
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}


Vector CMikeForce :: GetGunPosition( )
{
/*	if (m_Crouching )
	{
		return pev->origin + Vector( 0, 0, 36 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 62 );
	}*/

	Vector v, a;
	GetAttachment(0, v, a);

	return v;

}


//=========================================================
// Fire procedures - Shoots one round from designated weapon 
// at the enemy MikeForce is facing.
//=========================================================

void CMikeForce :: M16Fire ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_M16 ); // shoot +-5 degrees

	m_cAmmoLoaded--;// take away a bullet!

	pev->effects |= EF_MUZZLEFLASH;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	
}


void CMikeForce :: ShotgunFire( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL); 

	FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_MONSTER_SHOT, 0 ); // shoot +-7.5 degrees

	m_cAmmoLoaded--;// take away a bullet!

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/870_buckshot.wav", 1, ATTN_NORM, 0, 100 );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );
	
	pev->effects |= EF_MUZZLEFLASH;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	
}

		
void CMikeForce :: M60Fire( void )
{

	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = GetGunPosition();
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

		
//=========================================================
// Spawn
//=========================================================

void CMikeForce :: Spawn()
{
    Precache( );

    SET_MODEL(ENT(pev), "models/mikeforce.mdl");

	// The rest of this method was taken from the Barney spawn() method
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
    pev->effects        = 0;
	pev->health			= gSkillData.mikeforceHealth;
	pev->view_ofs		= Vector ( 0, 0, 68 );// position of the eyes relative to monster's origin.
    pev->yaw_speed      = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_Crouching			= FALSE;

	m_cClipSize			= MIKEFORCE_CLIP_SIZE[ pev->weapons ];
	m_cAmmoLoaded		= m_cClipSize;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	if ( pev->body == -1 )
	{// -1 chooses a random body
		pev->body = RANDOM_LONG(0, NUM_BODIES - 1 );
	}
	SetBodygroup( BODY_GROUP, pev->body );

	SetBodygroup( GUN_GROUP, pev->weapons );

	if (pev->weapons == MIKEFORCE_BODY_M60)
		SetBodygroup(AMMO_GROUP, TRUE);
	else
		SetBodygroup(AMMO_GROUP, FALSE);

	m_fHandGrenades = pev->spawnflags & SF_MIKEFORCE_HANDGRENADES;

	MonsterInit();
	SetUse( FollowerUse );
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================

void CMikeForce :: Precache()
{
    PRECACHE_MODEL("models/mikeforce.mdl");
	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl"); //shotgun shell

	PRECACHE_SOUND("weapons/reload1.wav");
	PRECACHE_SOUND("weapons/reload2.wav");
	PRECACHE_SOUND("weapons/reload3.wav");
	
	PRECACHE_SOUND("weapons/m16_burst.wav");
	PRECACHE_SOUND("weapons/m60_fire.wav");
	PRECACHE_SOUND("weapons/870_buckshot.wav");
	PRECACHE_SOUND("weapons/870_pump.wav");

	PRECACHE_SOUND("mikeforce/arrgh.wav");
	PRECACHE_SOUND("mikeforce/urgh.wav");

	PRECACHE_SOUND("mikeforce/choke1.wav");
	PRECACHE_SOUND("mikeforce/choke2.wav");

	// every new talkmonster must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}    


//=========================================================
// Init talk data
//=========================================================

void CMikeForce :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// MikeForce speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"MF_ANSWER";
	m_szGrp[TLK_QUESTION] =	"MF_QUESTION";
	m_szGrp[TLK_IDLE] =		"MF_IDLE";
	m_szGrp[TLK_STARE] =	"MF_STARE";
	m_szGrp[TLK_USE] =		"MF_OK";
	m_szGrp[TLK_UNUSE] =	"MF_WAIT";
	m_szGrp[TLK_STOP] =		"MF_STOP";

	m_szGrp[TLK_NOSHOOT] =	"MF_SCARED";
	m_szGrp[TLK_HELLO] =	"MF_HELLO";

	m_szGrp[TLK_PLHURT1] =	"!MF_CUREA";
	m_szGrp[TLK_PLHURT2] =	"!MF_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"!MF_CUREC";

	m_szGrp[TLK_PHELLO] =	NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = NULL ;		// UNDONE

	m_szGrp[TLK_SMELL] =	"MF_SMELL";
	
	m_szGrp[TLK_WOUND] =	"MF_WOUND";
	m_szGrp[TLK_MORTAL] =	"MF_MORTAL";

}


//=========================================================
// PainSound
//=========================================================

void CMikeForce :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,1))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "mikeforce/arrgh.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "mikeforce/urgh.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================

void CMikeForce :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,1))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "mikeforce/choke1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "mikeforce/choke2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int CMikeForce :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Forget( bits_MEMORY_INCOVER );

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "MF_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "MF_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "MF_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}


//=========================================================
// SetActivity 
//=========================================================

void CMikeForce :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	char seq[40];

	switch ( NewActivity)
	{
		case ACT_RANGE_ATTACK1:
			switch ( pev->weapons )
			{
				case MIKEFORCE_BODY_M16:		
					strcpy( seq, "shootm16" );
					break;
				case MIKEFORCE_BODY_SHOTGUN:
					strcpy( seq, "shootshotgun" );
					break;
				case MIKEFORCE_BODY_M60:	
					strcpy( seq, "shootm60" );
					break;
			}
			break;

		case ACT_VICTORY_DANCE:
			iSequence = LookupActivity ( NewActivity );
			m_Crouching = FALSE;
			break;
	
		case ACT_RANGE_ATTACK2:
			strcpy( seq, "throwgrenade");
			break;

		case ACT_SIGNAL3:
		case ACT_IDLE:
			strcpy( seq, ( m_MonsterState == MONSTERSTATE_COMBAT ? "combatidle" : "idle" ));
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

		if ( GetBodygroup( GUN_GROUP ) == MIKEFORCE_BODY_NOGUN )
		{
			strcpy( seq2, "nogun_" );
			strcat( seq2, seq );
			strcpy( seq, seq2 );
		}
		else if ( m_Crouching )
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
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CMikeForce :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that mikeforce will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slMFFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slMFFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that mikeforce will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleMFStand;
		}
		else
			return psched;	

	case SCHED_MIKEFORCE_COVER_AND_RELOAD:
		return &slMFHideReload[ 0 ];

	case SCHED_RANGE_ATTACK2:
		return &slMFRangeAttack2[ 0 ];

	case SCHED_TAKE_COVER_FROM_ENEMY:
		if (HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
			return &slMFTossGrenadeCover[ 0 ];
		else
			return CTalkMonster::GetScheduleOfType( Type );

	case SCHED_VICTORY_DANCE:	// Don't do a victory dance when crouching
		if ( !m_Crouching )
			return CTalkMonster::GetScheduleOfType( Type );
		else
			return GetScheduleOfType( SCHED_ALERT_STAND );

	case SCHED_MIKEFORCE_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slMikeForceRepel[ 0 ];
		}
	case SCHED_MIKEFORCE_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slMikeForceRepelAttack[ 0 ];
		}
	case SCHED_MIKEFORCE_REPEL_LAND:
		{
			Crouch();
			return &slMikeForceRepelLand[ 0 ];
		}

			
	}

	return CTalkMonster::GetScheduleOfType( Type );
}


void CMikeForce::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// If crouching, reduce damage from explosions
	if (m_Crouching && (bitsDamageType & DMG_BLAST)) flDamage = flDamage / 2;	

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CMikeForce::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( pev->weapons < MIKEFORCE_BODY_NOGUN )
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		switch ( pev->weapons )
		{
			case MIKEFORCE_BODY_M16:		pGun = DropItem( "weapon_m16", vecGunPos, vecGunAngles );	break;
			case MIKEFORCE_BODY_SHOTGUN:	pGun = DropItem( "weapon_870", vecGunPos, vecGunAngles );	break;
			case MIKEFORCE_BODY_M60:		pGun = DropItem( "weapon_m60", vecGunPos, vecGunAngles );		break;
		}
		
		pev->weapons = MIKEFORCE_BODY_NOGUN;
		SetBodygroup( GUN_GROUP, pev->weapons );
	}

	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}


// CheckEnemy over-ridden for Mikeforce so we can tell if we have just killed someone

int CMikeForce :: CheckEnemy ( CBaseEntity *pEnemy )
{
	if (!pEnemy->IsAlive() && !HasConditions( bits_COND_ENEMY_DEAD ))
	{
		Crouch();
	}
	
	return CTalkMonster :: CheckEnemy ( pEnemy );
}


void CMikeForce::Crouch( void )
{
	if ( GetBodygroup( GUN_GROUP ) != MIKEFORCE_BODY_NOGUN )
	{
		m_Crouching = TRUE;
		m_CrouchTime = gpGlobals->time + RANDOM_LONG(15, 25);
		pev->view_ofs = Vector ( 0, 0, 40 );
	}
};

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================

Schedule_t *CMikeForce :: GetSchedule ( void )
{
	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_MIKEFORCE_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_MIKEFORCE_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_MIKEFORCE_REPEL );
		}
	}

	
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );

		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
		{
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}

		// Crouch if it sounds like things are getting a bit intense
		if ( pSound && (pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT) ))
		{
			Crouch();
		}

		if ( pSound && (pSound->m_iType & (bits_SOUND_WORLD | bits_SOUND_COMBAT ) ) && FOkToSpeak() )
		{
			PlaySentence( "MF_HEAR", 4, VOL_NORM, ATTN_NORM );
		}

	}

	if ( HasConditions( bits_COND_ENEMY_DEAD ) )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "MF_KILL", 4, VOL_NORM, ATTN_NORM );
		}
	}

	if (m_Crouching)
	{
		// If I've been crouching for a while
		if ( m_CrouchTime < gpGlobals->time ) 
		{
			if (m_hEnemy != NULL && !HasConditions( bits_COND_ENEMY_DEAD ))
			{
				// Stay crouching if I'm in the middle of a fight
				Crouch();
			}
			else
			{
				// Go out of crouch mode
				m_Crouching = FALSE;
				pev->view_ofs		= Vector ( 0, 0, 68 );
				if (RANDOM_LONG(0, 1)) return GetScheduleOfType( SCHED_VICTORY_DANCE );
			}
		}
	}
	else
	{
		// If player is crouching and I am following then crouch also
		if ( IsFollowing() && (m_hTargetEnt->pev->flags & FL_DUCKING) )
		{
			Crouch();
			SetActivity( m_Activity );
		}
	}

	//If his gun is drawn but he has no ammo left then reload
	if ( HasConditions ( bits_COND_NO_AMMO_LOADED )  && pev->weapons!=MIKEFORCE_BODY_NOGUN )
	{
		return GetScheduleOfType ( SCHED_MIKEFORCE_COVER_AND_RELOAD );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions( bits_COND_NEW_ENEMY ) && !(
					gpGlobals->time <= CTalkMonster::g_talkWaitTime ||
					pev->spawnflags & SF_MONSTER_GAG ||
					m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ||
					!IsAlive() || FNullEnt(FIND_CLIENT_IN_PVS(edict()))
					) )
			{
				switch ( m_hEnemy->Classify() )
				{
				case CLASS_ALIEN_MONSTER: 
					PlaySentence( "MF_ZOMBIE", 4, VOL_NORM, ATTN_IDLE );
					break;
				
				case CLASS_HUMAN_MILITARY:
				case CLASS_HUMAN_CHARLIE: 
					PlaySentence( "MF_ATTACK", 4, VOL_NORM, ATTN_IDLE );
					break;
				}
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( IsFollowing() )
		{
			if ( m_hEnemy == NULL )
			{
				if ( !m_hTargetEnt->IsAlive() )
				{
					// UNDONE: Comment about the recently dead player here?
					StopFollowing( FALSE );
					break;
				}
				else
				{
					if ( HasConditions( bits_COND_CLIENT_PUSH ) )
					{
						return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
					}
					return GetScheduleOfType( SCHED_TARGET_FACE );
				}
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CMikeForce :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}



void CMikeForce::DeclineFollowing( void )
{
	PlaySentence( "MF_POK", 2, VOL_NORM, ATTN_NORM );
}




//=========================================================
// DEAD MIKEFORCE PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadMikeForce : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadMikeForce::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadMikeForce::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_mikeforce_dead, CDeadMikeForce );

//=========================================================
// ********** DeadMikeForce SPAWN **********
//=========================================================
void CDeadMikeForce :: Spawn( )
{
	PRECACHE_MODEL("models/mikeforce.mdl");
	SET_MODEL(ENT(pev), "models/mikeforce.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	if ( pev->body == -1 )
	{// -1 chooses a random body
		pev->body = RANDOM_LONG(0, NUM_BODIES - 1 );
	}
	SetBodygroup( BODY_GROUP, pev->body );
	SetBodygroup( GUN_GROUP, MIKEFORCE_BODY_NOGUN );

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead MikeForce with bad pose\n" );
	}
	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();
}


BOOL CMikeForce :: FCanCheckAttacks ( void )
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
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================

BOOL CMikeForce :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! m_fHandGrenades)
	{
		return FALSE;
	}
	
	// if the mike isn't moving, it's ok to check.
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

	// BUGBUG - HGrunt checks to see if any squad members are within range of a grenade before throwing
	// Mike can't do this

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
// CMikeForceRepel - when triggered, spawns a mikeforce
// repelling down a line.
//=========================================================

class CMikeForceRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
};

LINK_ENTITY_TO_CLASS( monster_mikeforce_repel, CMikeForceRepel );

void CMikeForceRepel::Spawn( void )
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse( RepelUse );
}

void CMikeForceRepel::Precache( void )
{
	UTIL_PrecacheOther( "monster_mikeforce" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CMikeForceRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	CBaseEntity *pEntity = Create( "monster_mikeforce", pev->origin, pev->angles );
	CBaseMonster *pGrunt = pEntity->MyMonsterPointer( );
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pGrunt->SetActivity( ACT_GLIDE );
	// UNDONE: position?
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pGrunt->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

	UTIL_Remove( this );
}


void CMikeForce::PlayerCommand( int iCommand )
{
	if ( m_afMemory & bits_MEMORY_PROVOKED ) return;

	BOOL OkToSpeak = !(
		gpGlobals->time <= CTalkMonster::g_talkWaitTime ||
		pev->spawnflags & SF_MONSTER_GAG ||
		m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ||
		!IsAlive() || FNullEnt(FIND_CLIENT_IN_PVS(edict())) 
	);

	switch ( iCommand )
	{
	case TLK_COMMAND_GET_DOWN:
		Crouch();
		SetActivity( m_Activity );
		if (OkToSpeak) PlaySentence( "MF_GETDOWN", 2, VOL_NORM, ATTN_IDLE );
		break;

	case TLK_COMMAND_COME_HERE:	
		if (OkToSpeak) PlaySentence( "MF_COMEHERE", 2, VOL_NORM, ATTN_IDLE );
		break;
	
	case TLK_COMMAND_ATTACK:
		if (m_hEnemy == NULL || HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			if (OkToSpeak) PlaySentence( "MF_SEARCH", 4, VOL_NORM, ATTN_IDLE );
		}
		else
		{
			if (OkToSpeak) PlaySentence( "MF_ATTACK", 4, VOL_NORM, ATTN_IDLE );
		}
		break;

	case TLK_COMMAND_RETREAT:
		if (OkToSpeak) PlaySentence( "MF_RETREAT", 3, VOL_NORM, ATTN_IDLE );
		break;

	case TLK_COMMAND_SURPRESSING_FIRE:
		if (OkToSpeak) PlaySentence( "MF_SURPRESS", 3, VOL_NORM, ATTN_IDLE );
		break;

	case TLK_COMMAND_OUTTA_MY_WAY:
		if (OkToSpeak) PlaySentence( "MF_OUTWAY", 2, VOL_NORM, ATTN_IDLE );
		break;
	}

	CTalkMonster::PlayerCommand( iCommand );
}

