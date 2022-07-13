// Human.cpp: implementation of the CHuman and CHumanHead classes.
//
//////////////////////////////////////////////////////////////////////

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"basemonster.h"
#include	"squadmonster.h"
#include	"human.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"soundent.h"
#include	"animation.h"
#include	"weapons.h"
#include	"monsters.h"
#include	"rpg7.h"


//=========================================================
//=========================================================
//
// CHuman class functions
//
//=========================================================
//=========================================================


TYPEDESCRIPTION	CHuman::m_SaveData[] = 
{
	DEFINE_FIELD( CHuman, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CHuman, m_fHandGrenades, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHuman, m_fCrouching, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHuman, m_fStopCrouching, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHuman, m_flCrouchTime, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_nHeadNum, FIELD_INTEGER ),
	DEFINE_FIELD( CHuman, m_nLastSquadCommand, FIELD_INTEGER ),
	DEFINE_FIELD( CHuman, m_flLastSquadCmdTime, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_fSquadCmdAcknowledged, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHuman, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_flPlayerDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CHuman, m_szSpeechLabel, FIELD_STRING ),
	DEFINE_FIELD( CHuman, m_flNextAttack1Check, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_LastAttack1Check, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHuman, m_flNextAttack2Check, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_LastAttack2Check, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHuman, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHuman, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHuman, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CHuman, m_flStopTalkTime, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_bitsSaid, FIELD_INTEGER ),
	DEFINE_FIELD( CHuman, m_flLastTalkTime, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_expldir, FIELD_INTEGER ),
	DEFINE_FIELD( CHuman, m_LastBloodTime, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_iWeapon, FIELD_INTEGER ),
	DEFINE_FIELD( CHuman, m_flLastMedicSearch, FIELD_TIME ),
	DEFINE_FIELD( CHuman, m_flLastKick, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CHuman, CSquadMonster );



float	CHuman::g_talkWaitTime = 0;		// time delay until it's ok to speak: used so that two NPCs don't talk at once

//=========================================================
// Precache - called by inherited class spawn function
//=========================================================

void CHuman :: Precache( void )
{
	m_iWeapon = GetWeaponNum( pev->weapons );

	switch ( m_iWeapon )
	{
	case HUMAN_WEAPON_M16: 
		{
			PRECACHE_SOUND("weapons/m16_burst.wav");
			PRECACHE_SOUND("weapons/m16_clipinsert1.wav");
			m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
		}
		break;
	
	case HUMAN_WEAPON_M60: 
		{
			PRECACHE_SOUND("weapons/m60_fire.wav");
			PRECACHE_SOUND("weapons/m60_reload_full.wav");
			m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
		}
		break;

	case HUMAN_WEAPON_870: 
		{
			PRECACHE_SOUND("weapons/870_buckshot.wav");
			PRECACHE_SOUND("weapons/870_pump.wav");
			PRECACHE_SOUND("weapons/reload1.wav");
			PRECACHE_SOUND("weapons/reload2.wav");
			PRECACHE_SOUND("weapons/reload3.wav");
			m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl"); //shotgun shell
		}
		break;

	case HUMAN_WEAPON_M79: 
		{
			PRECACHE_SOUND("weapons/m79_fire1.wav");
			PRECACHE_SOUND("weapons/m79_fire2.wav");
			PRECACHE_SOUND("weapons/m79_open.wav");
		}
		break;

	case HUMAN_WEAPON_RPG7: 
		{ 
			PRECACHE_SOUND("weapons/rocket1.wav");
			PRECACHE_SOUND("weapons/rpg7_fire1.wav");
		}
		break;

	case HUMAN_WEAPON_AK47: 
		{
			PRECACHE_SOUND("weapons/ak47_burst.wav");
			PRECACHE_SOUND("weapons/ak47_cliprelease1.wav");
			m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
		}
		break;

	case HUMAN_WEAPON_M21: 
		{
			PRECACHE_SOUND("weapons/m21_shot1.wav");
			PRECACHE_SOUND("weapons/m21_shot2.wav");
			m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
		}
		break;

	case HUMAN_WEAPON_COLT1911A1: 
		{
			PRECACHE_SOUND("weapons/colt_fire1.wav");
			PRECACHE_SOUND("weapons/colt_fire2.wav");
			PRECACHE_SOUND("weapons/colt_release.wav");
			m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
		}
		break;

	case HUMAN_WEAPON_NONE: 
		{
		}
		break;
	}

	PRECACHE_SOUND("zombie/claw_miss2.wav");	// kick

	PRECACHE_MODEL( GetHeadModelName() );
}


//=========================================================
// Spawn - called by inherited class spawn function
//=========================================================

void CHuman :: Spawn()
{
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	m_iWeapon			= GetWeaponNum( pev->weapons );
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
    pev->effects        = 0;
	pev->view_ofs		= Vector ( 0, 0, 68 );// position of the eyes relative to monster's origin.
    pev->yaw_speed      = 5;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_cClipSize			= GetClipSize();
	m_cAmmoLoaded		= m_cClipSize;
	m_afCapability		= bits_CAP_SQUAD | bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fHandGrenades		= pev->spawnflags & SF_HUMAN_HANDGRENADES;
	m_voicePitch		= 100 + RANDOM_LONG(-5,5);
	m_fCrouching		= FALSE;
	m_fStopCrouching	= FALSE;
	m_nLastSquadCommand  = SQUADCMD_NONE;

	SetBodygroup( GetHeadGroupNum(), m_nHeadNum );
	SetBodygroup( GetWeaponGroupNum(), pev->weapons );

	CHuman::g_talkWaitTime = 0;
	MonsterInit();
}


//=============================================
// Center - over-ridden because of crouching
//=============================================

Vector CHuman::Center()
{
	if ( m_fCrouching)
	{
		return pev->origin + Vector(0, 0, 18);
	}
	else
	{
		return pev->origin + Vector(0, 0, 36);
	}
}


//=============================================
// For if a monster is created by a monstermaker
// or whatever we may want some more interesting
// settings than just 0 for everything
//=============================================

void CHuman::ApplyDefaultSettings( void )
{
	SetBodygroup( GetHeadGroupNum(), m_nHeadNum );
	SetBodygroup( GetWeaponGroupNum(), pev->weapons );

	m_iWeapon = GetWeaponNum( pev->weapons );
	m_cClipSize 	= GetClipSize();
	m_cAmmoLoaded	= m_cClipSize;
}


//=========================================================
// StartMonster - final bit of initization before a monster 
// is turned over to the AI.  Over-ridden because base class
// calls model file to try and set weapons capabilities
//=========================================================

void CHuman :: StartMonster ( void )
{
	CSquadMonster::StartMonster();

	if (m_iWeapon != HUMAN_WEAPON_NONE )
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK1;
	}
	
	if (m_fHandGrenades)
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK2;
	}
	
	if ( LookupSequence ( "frontkick" ) != ACTIVITY_NOT_AVAILABLE ) 
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK1;
	}
}


//=========================================================
// GetClipSize - return size of clip
//=========================================================

int CHuman::GetClipSize()
{
	switch( m_iWeapon )
	{
	case HUMAN_WEAPON_M16: return 30; break;
	case HUMAN_WEAPON_M60: return 50; break;
	case HUMAN_WEAPON_870: return 7; break;
	case HUMAN_WEAPON_M79: return 1; break;
	case HUMAN_WEAPON_RPG7: return 1; break;
	case HUMAN_WEAPON_AK47: return 30; break;
	case HUMAN_WEAPON_M21: return 20; break;
	case HUMAN_WEAPON_COLT1911A1: return 7; break;
	case HUMAN_WEAPON_NONE: 
	default: return 1; break;
	}
}


//=========================================================
// Weapon Entity Name
//=========================================================

char * CHuman::WeaponEntityName()
{
	switch( m_iWeapon )
	{
	case HUMAN_WEAPON_M16: return "weapon_m16"; break;
	case HUMAN_WEAPON_M60: return "weapon_m60"; break;
	case HUMAN_WEAPON_870: return "weapon_870"; break;
	case HUMAN_WEAPON_M79: return "weapon_m79"; break;
	case HUMAN_WEAPON_RPG7: return "weapon_rpg7"; break;
	case HUMAN_WEAPON_AK47: return "weapon_ak47"; break;
	case HUMAN_WEAPON_M21: return "weapon_m21"; break;
	case HUMAN_WEAPON_COLT1911A1: return "weapon_colt1911a1"; break;
	case HUMAN_WEAPON_NONE: 
	default: return ""; break;
	}
}


//=========================================================
// IRelationship - do I love or hate pTarget?
//=========================================================

int CHuman::IRelationship( CBaseEntity *pTarget )
{
	if ( pTarget->IsPlayer() )
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return R_HT;

	if ( m_iWeapon == HUMAN_WEAPON_RPG7 && pTarget->Classify() == CLASS_HELICOPTER )
		return R_HT;

	return CBaseMonster::IRelationship( pTarget );
}


//=========================================================
// ALertSound - monster says "Freeze!"
//=========================================================

void CHuman :: AlertSound( void )
{
	if ( m_hEnemy == NULL ) return;
	if ( !FOkToShout() ) return;

	PlayLabelledSentence( "ALERT" );
}

//=========================================================
// PainSound - when hit but not dead
//=========================================================

void CHuman :: PainSound ( void )
{
	// Make sure he doesn't just repeatedly go uh-uh-uh-uh-uh when hit by rapid fire
	if (gpGlobals->time < m_painTime) return;
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	PlayLabelledSentence( "PAIN" );
	SquadIssueCommand( SQUADCMD_DISTRESS ); 
}

//=========================================================
// DeathSound - on death
//=========================================================

void CHuman :: DeathSound ( void )
{
	PlayLabelledSentence( "DIE" );
	SquadIssueCommand( SQUADCMD_DISTRESS ); 
}


//=========================================================
// IdleSound - chat about stuff when idle
//=========================================================

void CHuman :: IdleSound ( void )
{
	// try to start a conversation, or make statement
	BOOL Spoke = TRUE;

	if (!FOkToSpeak())
		return;

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return
	CBaseEntity *pEntity = FindNearestFriend( !HasConditions(bits_COND_PROVOKED) );	// include players unless I have been provoked
	
	CHuman * pFriend = NULL;
	if (pEntity) pFriend = pEntity->MyHumanPointer();

	if ( pFriend != NULL && FInViewCone(pFriend) && FVisible(pFriend) )
	{
		m_hTalkTarget = pFriend;

		if ( pFriend->pev->deadflag == DEAD_NO )
		{
			if ( !(pFriend->IsMoving()) )
			{
				if ( !FBitSet( m_bitsSaid, bit_saidHello ) && ( pFriend->IsPlayer() || pFriend->IsLeader() ) ) // Friend is Commander
				{
					PlayLabelledSentence( "HELLO" );
					SetBits( m_bitsSaid, bit_saidHello );
				}
				else if ( !FBitSet( m_bitsSaid, bit_saidDamageHeavy ) &&	// Friend is heavily wounded
					pFriend->pev->health <= pFriend->pev->max_health / 8 )
				{
					PlayLabelledSentence( "HURTC" );
					SetBits( m_bitsSaid, bit_saidDamageHeavy );
				}
				else if ( !FBitSet( m_bitsSaid, bit_saidDamageMedium ) &&	// Friend is medium wounded
					pFriend->pev->health <= pFriend->pev->max_health / 4 && 
					pFriend->pev->health > pFriend->pev->max_health / 8 )
				{
					PlayLabelledSentence( "HURTB" );
					SetBits( m_bitsSaid, bit_saidDamageMedium );
				}
				else if ( !FBitSet( m_bitsSaid, bit_saidDamageLight ) &&	// Friend is lightly wounded
					pFriend->pev->health <= pFriend->pev->max_health / 2 && 
					pFriend->pev->health > pFriend->pev->max_health / 4)
				{
					PlayLabelledSentence( "HURTA" );
					SetBits( m_bitsSaid, bit_saidDamageLight );
				}
				else if ( !FBitSet(m_bitsSaid, bit_saidWoundLight ) &&	// I am lightly wounded
					(pev->health <= (2 * pev->max_health / 3)) && 
					(pev->health > (pev->max_health / 3)))
				{
					PlayLabelledSentence( "WOUND" );
					SetBits( m_bitsSaid, bit_saidWoundLight );
				}
				else if ( !FBitSet(m_bitsSaid, bit_saidWoundHeavy ) && // I am heavily wounded
					(pev->health <= (pev->max_health / 3)))
				{
					PlayLabelledSentence( "MORTAL" );
					SetBits( m_bitsSaid, bit_saidWoundHeavy );
				}
				else if ( !FBitSet( m_bitsSaid, bit_saidQuestion ) && !RANDOM_LONG(0, 4) )	// Ask a question
				{
					PlayLabelledSentence( "QUESTION" ); 
					SetBits( m_bitsSaid, bit_saidQuestion );
					
					CHuman *pHuman = pFriend->MyHumanPointer();
					if (pHuman) pHuman->SetAnswerQuestion( this, GetDuration( "QUESTION" ) );
				}
				else if ( !FBitSet( m_bitsSaid, bit_saidIdle ) && !RANDOM_LONG(0, 4) )	// Say something idly
				{
					PlayLabelledSentence( "IDLE" ); 
					SetBits( m_bitsSaid, bit_saidIdle );
				}
				else if ( pFriend->IsPlayer() && !FBitSet( m_bitsSaid, bit_saidStare ) && !RANDOM_LONG(0, 4) ) // Player is staring at me
				{
					if ( ( pFriend->pev->origin - pev->origin ).Length2D() < 128 )
					{
						UTIL_MakeVectors( pFriend->pev->angles );
						if ( UTIL_DotPoints( pFriend->pev->origin, pev->origin, gpGlobals->v_forward ) > m_flFieldOfView )
						{
							PlayLabelledSentence( "STARE" ); 
							SetBits( m_bitsSaid, bit_saidStare );
						}
					}
				}
				else Spoke = FALSE;
			}
		}
		else if ( !FBitSet( m_bitsSaid, bit_saidDead ) )	// Friend is dead
		{
			PlayLabelledSentence( "DEAD" );
			SetBits( m_bitsSaid, bit_saidDead );
		}
		else Spoke = FALSE;
	}
	else Spoke = FALSE;

	if ( !Spoke )
	{
		// didn't speak
		m_hTalkTarget = NULL;
	}
}


//=========================================================
// FOkToShout - is it ok for me to shout?
//=========================================================

int CHuman :: FOkToShout( void )
{
	// if in the grip of a barnacle, don't speak
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
	{
		return FALSE;
	}

	// if not alive, certainly don't speak
	if ( pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	//Don't talk if you have no head
	if ( m_nHeadNum == GetNumHeads() )
	{
		return FALSE;
	}

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		return FALSE;
	}

	if ( IsTalking() )	// Don't say anything if I'm already talking
	{
		return FALSE;
	}

	// if player is not in pvs, don't speak
	if ( FNullEnt(FIND_CLIENT_IN_PVS(edict())))
		return FALSE;

	return TRUE;
}


//=========================================================
// FOkToSpeak - is it ok for me to speak in a chatty way?
//=========================================================

int CHuman :: FOkToSpeak( void )
{
	// If it's not okay to shout then it's definitely not okay to speak
	if ( !FOkToShout() )
	{
		return FALSE;
	}

	// if someone else is talking, don't speak
	if (gpGlobals->time <= CHuman::g_talkWaitTime)
		return FALSE;

	// If I have said something not very long ago
	if ( m_flLastTalkTime > gpGlobals->time - 10 )
	{
		return FALSE;
	}

	// If I'm in combat then I don't want to chat but can still shout
	if ( m_MonsterState == MONSTERSTATE_COMBAT )
		return FALSE;

	return TRUE;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.  Humans mostly have same
// yaw speeds, although some could be over-ridden?
//=========================================================

void CHuman :: SetYawSpeed ( void )
{
    int ys;

	ys = 0;

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
	case ACT_RANGE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================

void CHuman :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case HUMAN_AE_BURST1:
		switch ( m_iWeapon )
		{
			case HUMAN_WEAPON_M16: 
				Fire( BULLET_MONSTER_M16, 1, VECTOR_CONE_8DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, "weapons/m16_burst.wav");
				break;
			case HUMAN_WEAPON_870:	
				Fire( BULLET_MONSTER_SHOT, 8, VECTOR_CONE_15DEGREES, m_iShotgunShell, TE_BOUNCE_SHOTSHELL, "weapons/870_buckshot.wav");
				break;
			case HUMAN_WEAPON_M60:
				Fire( BULLET_MONSTER_762, 1, VECTOR_CONE_15DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, "weapons/m60_fire.wav");
				break;
			case HUMAN_WEAPON_M21:
				Fire( BULLET_MONSTER_762, 1, VECTOR_CONE_1DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, 
					( RANDOM_LONG(0, 1) ? "weapons/m21_shot1.wav" : "weapons/m21_shot2.wav" ) );
				break;
			case HUMAN_WEAPON_AK47: 
				Fire( BULLET_MONSTER_AK47, 1, VECTOR_CONE_10DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, "weapons/ak47_burst.wav");
				break;
			case HUMAN_WEAPON_COLT1911A1: 
				Fire( BULLET_MONSTER_1143, 1, VECTOR_CONE_6DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, 
					( RANDOM_LONG(0, 1) ? "weapons/colt_fire1.wav" : "weapons/colt_fire2.wav" ) );
				break;
			case HUMAN_WEAPON_M79:
				M79Fire();
				break;
			case HUMAN_WEAPON_RPG7:
				RPGFire();
				break;
		}
		break;

	case HUMAN_AE_BURST2:
		switch ( m_iWeapon )
		{
			case HUMAN_WEAPON_M16: 
				Fire( BULLET_MONSTER_M16, 1, VECTOR_CONE_8DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, "");
				break;
			case HUMAN_WEAPON_AK47: 
				Fire( BULLET_MONSTER_AK47, 1, VECTOR_CONE_10DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, "");
				break;
		}
		break;

	case HUMAN_AE_BURST3:
		switch ( m_iWeapon )
		{
			case HUMAN_WEAPON_M16:		
				Fire( BULLET_MONSTER_M16, 1, VECTOR_CONE_8DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, "");
				break;
			case HUMAN_WEAPON_AK47: 
				Fire( BULLET_MONSTER_AK47, 1, VECTOR_CONE_10DEGREES, m_iBrassShell, TE_BOUNCE_SHELL, "");
				break;
			case HUMAN_WEAPON_870:	
				EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/870_pump.wav", 1, HUMAN_ATTN );
				break;
		}
		break;

	case HUMAN_AE_RELOAD:
		{
			// Make weapon-specific reload sound

			switch ( m_iWeapon )
			{
			case HUMAN_WEAPON_M16:
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/m16_clipinsert1.wav", 1, HUMAN_ATTN );
				break;

			case HUMAN_WEAPON_870:	
				switch( RANDOM_LONG( 0, 2 ) )
				{
					case 0: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload1.wav", 1, HUMAN_ATTN ); break;
					case 1: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload2.wav", 1, HUMAN_ATTN ); break;
					case 2: EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload3.wav", 1, HUMAN_ATTN ); break;
				}
				break;

			case HUMAN_WEAPON_M60:
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/m60_reload_full.wav", 1, HUMAN_ATTN );
				break;

			case HUMAN_WEAPON_M79:
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/m79_open.wav", 1, HUMAN_ATTN );
				break;

			case HUMAN_WEAPON_AK47:
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/ak47_cliprelease1.wav", 1, HUMAN_ATTN );
				break;

			case HUMAN_WEAPON_COLT1911A1: 
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/colt_release.wav", 1, HUMAN_ATTN );
				break;
			}
			
			// Reload

			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
		}
		break;

	case HUMAN_AE_GREN_TOSS:
	{
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

		m_LastAttack2Check = FALSE;
		m_flNextAttack2Check = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
	}
	break;

	case HUMAN_AE_GREN_DROP:
	{
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
	}
	break;

	case HUMAN_AE_DROP_GUN:
	if ( m_iWeapon != HUMAN_WEAPON_NONE )
	{// throw a gun if he has one

		Vector	vecGunPos;
		Vector	vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );
		DropItem( WeaponEntityName(), vecGunPos, vecGunAngles );

		m_iWeapon = HUMAN_WEAPON_NONE;
		pev->weapons = GetWeaponBodyGroup( m_iWeapon );
		SetBodygroup( GetWeaponGroupNum(), pev->weapons );
		
	}
	break;

	case HUMAN_AE_KICK:
	{
		CBaseEntity *pHurt = Kick();

		if ( pHurt )
		{
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "zombie/claw_miss2.wav", 1, HUMAN_ATTN );
			UTIL_MakeVectors( pev->angles );
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
		}
	}
	break;
	
	default:
		CSquadMonster::HandleAnimEvent( pEvent );
	}
}


//=========================================================
// Get Voice Pitch
//=========================================================

int CHuman :: GetVoicePitch( void )
{
	return m_voicePitch;
}


//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================

Activity CHuman :: GetDeathActivity ( void )
{
	if ( pev->deadflag != DEAD_NO )
	{
		// don't run this while dying.
		return m_IdealActivity;
	}

	Activity deathActivity = ACT_DIESIMPLE;

	switch ( m_LastHitGroup )
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;

	case HITGROUP_CHEST:
		deathActivity = ACT_DIE_CHESTSHOT;
		break;

	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;

	default:
		// try to pick a death based on attack direction
	
		Vector vecSrc = Center();
		TraceResult	tr;
		float flDot = DotProduct ( gpGlobals->v_forward, g_vecAttackDir * -1 );
		
		if ( flDot > 0.3 )
		{
			// make sure there's room to fall forward
			UTIL_TraceHull ( vecSrc, vecSrc + gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr );

			if ( tr.flFraction == 1.0 )
			{
				deathActivity = ACT_DIEFORWARD;
			}
		}
		else if ( flDot <= -0.3 )
		{
			// make sure there's room to fall backward
			UTIL_TraceHull ( vecSrc, vecSrc - gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr );

			if ( tr.flFraction == 1.0 )
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
		break;
	}
	
	return deathActivity;
}


//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================

Activity CHuman :: GetSmallFlinchActivity ( void )
{
	Activity	flinchActivity;

	switch ( m_LastHitGroup )
	{
		// pick a region-specific flinch
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}

	return flinchActivity;
}


//=========================================================
// SetTurnActivity - measures the difference between the way
// the monster is facing and determines whether or not to
// select one of the 180 turn animations.
//=========================================================

void CHuman :: SetTurnActivity ( void )
{
	float flYD;
	flYD = FlYawDiff();

	if ( pev->movetype == MOVETYPE_FLY ) 
	{
		return;
	}

	if ( flYD <= -45 )
	{// big right turn
		m_IdealActivity = ACT_TURN_RIGHT;
	}
	else if ( flYD > 45 )
	{// big left turn
		m_IdealActivity = ACT_TURN_LEFT;
	}
}


//=========================================================
// SetActivity - totally overrides basemonster SetActivities
// because of the necessity to pick a crouch animation etc
//
// If you want an activity to have several "modes", i.e. 
// crouched and standing, don't give it an ACT_ title
//=========================================================

void CHuman :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	char seq[40];

	// First try just looking up the activity normally

	iSequence = LookupActivity ( NewActivity );

	// If that doesn't work, look up a named activity

	if (iSequence == ACTIVITY_NOT_AVAILABLE) switch ( NewActivity)
	{
		case ACT_RANGE_ATTACK1:
			switch ( m_iWeapon )
			{
				case HUMAN_WEAPON_M16:	strcpy( seq, "shootm16" );		break;
				case HUMAN_WEAPON_870:	strcpy( seq, "shootshotgun" );	break;
				case HUMAN_WEAPON_M60:	strcpy( seq, "shootm60" );		break;
				case HUMAN_WEAPON_M79: 	strcpy( seq, "shootm79" );		break;
				case HUMAN_WEAPON_RPG7: strcpy( seq, "shootrpg7" ); 	break;
				case HUMAN_WEAPON_AK47: strcpy( seq, "shootak47" );		break;
				case HUMAN_WEAPON_M21:	strcpy( seq, "shootm21" );		break;
				case HUMAN_WEAPON_COLT1911A1:	strcpy( seq, "shootcolt1911a1" );	break;
			}
			break;

		case ACT_VICTORY_DANCE:
			iSequence = LookupActivity ( NewActivity );
			break;
	
		case ACT_RANGE_ATTACK2:
			strcpy( seq, "throwgrenade");
			if ( m_iWeapon == HUMAN_WEAPON_M60 && !m_fCrouching ) strcat( seq, "_hip" );
			break;

		case ACT_SPECIAL_ATTACK1:
			strcpy( seq, "dropgrenade" );
			if ( m_iWeapon == HUMAN_WEAPON_M60 && !m_fCrouching ) strcat( seq, "_hip" );
			break;
	
		case ACT_MELEE_ATTACK1:
			iSequence = LookupSequence ( "frontkick" );
			break;

		case ACT_CROUCH:
			iSequence = LookupSequence ( "crouch" );
			break;

		case ACT_UNCROUCH:
			iSequence = LookupSequence ( "uncrouch" );
			break;

		case ACT_COWER:
			strcpy( seq, "cower" );
			break;

		case ACT_SIGNAL1:
			strcpy( seq, "signal_advance");
			break;

		case ACT_SIGNAL2:
			strcpy( seq, "signal_flank");
			break;

		case ACT_SIGNAL3:
			strcpy( seq, "signal_retreat");
			break;

		case ACT_IDLE:
			strcpy( seq, "idle" );
			break;

		case ACT_COMBAT_IDLE:
		case ACT_IDLE_ANGRY:
			strcpy( seq, "combatidle" );
			if ( m_iWeapon == HUMAN_WEAPON_M60 && !m_fCrouching ) strcat( seq, "_hip" );
			break;

		case ACT_WALK:
			if ( pev->health >= pev->max_health / 3 )
			{
				strcpy( seq, "walk" );
			}
			else
			{
				strcpy( seq, "limp_walk" );
			}
			break;

		case ACT_RUN:
			if ( pev->health >= pev->max_health / 3 )
			{
				strcpy( seq, "run" );
			}
			else
			{
				strcpy( seq, "limp_run" );
			}
			break;

		case ACT_WALK_HURT:
			strcpy( seq, "limp_walk" );
			break;

		case ACT_RUN_HURT:
			strcpy( seq, "limp_run" );
			break;

		case ACT_RELOAD:
			strcpy( seq, "reload" );
			break;
		
		case ACT_TURN_LEFT:
			strcpy( seq, "turnleft" );
			break;

		case ACT_TURN_RIGHT:
			strcpy( seq, "turnright" );
			break;

		case ACT_FLINCH_LEFTARM:
			strcpy( seq, "laflinch" );
			break;

		case ACT_FLINCH_RIGHTARM:
			strcpy( seq, "raflinch" );
			break;

		case ACT_FLINCH_LEFTLEG:
			strcpy( seq, "llflinch" );
			break;

		case ACT_FLINCH_RIGHTLEG:
			strcpy( seq, "rlflinch" );
			break;

		case ACT_SMALL_FLINCH:
			strcpy( seq, "smlflinch" );
			break;

		case ACT_DIE_HEADSHOT:
			strcpy( seq, "die_headshot" );
			break;

		case ACT_DIE_CHESTSHOT:
			strcpy( seq, "die_chestshot" );
			break;

		case ACT_DIE_GUTSHOT:
			strcpy( seq, "die_gutshot" );
			break;

		case ACT_DIEFORWARD:
			strcpy( seq, "die_forward" );
			break;

		case ACT_DIEBACKWARD:
			strcpy( seq, "die_backward" );
			break;

		case ACT_DIESIMPLE:
			strcpy( seq, "die_simple" );
			break;

		case ACT_EXPLOSION_HIT:
		case ACT_EXPLOSION_LAND:
		case ACT_EXPLOSION_FLY:
			if ( NewActivity == ACT_EXPLOSION_HIT ) strcpy( seq, "explosion_hit" );
			if ( NewActivity == ACT_EXPLOSION_LAND ) strcpy( seq, "explosion_land" );
			if ( NewActivity == ACT_EXPLOSION_FLY ) strcpy( seq, "explosion_fly" );

			switch ( m_expldir )
			{
			case 1: strcat( seq, "_forward" ); break;
			case 2: strcat( seq, "_backward" ); break;
			case 3: strcat( seq, "_left" ); break;
			case 4: strcat( seq, "_right" ); break;
			}

			iSequence = LookupSequence ( seq );
			break;

		case ACT_HOVER:
			iSequence = LookupSequence ( "repel_jump" );
			break;

		case ACT_GLIDE:
			iSequence = LookupSequence ( "repel_repel" );
			break;

		case ACT_LAND:
			iSequence = LookupSequence ( "repel_land" );
			break;

		case ACT_FLY:
			iSequence = LookupSequence ( "repel_shoot" );
			break;
	}

	// If we still don't have a sequence, try and see if crouching is needed
	
	if (iSequence == ACTIVITY_NOT_AVAILABLE)
	{
		char seq2[40];

		if ( m_fCrouching )
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
// Fire procedures - Shoots one round from designated weapon 
// at the enemy human is facing.
//=========================================================

void CHuman :: Fire ( int nBulletType, int Num, Vector vecAccuracy, int nShellModel, int nShellBounce, const char * szSnd )
{
	// Get a vector to the target
	
	UTIL_MakeVectors(pev->angles);
	Vector vecShootDir, vecShootOrigin = GetGunPosition();
	
	if ( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		vecShootDir = gpGlobals->v_forward;
	}
	else
	{
		vecShootDir = ShootAtEnemy( vecShootOrigin );
	}
	
	// Fire
	
	FireBullets( Num, GetGunEndPosition(), vecShootDir, vecAccuracy, HUMAN_RIFLE_RANGE, nBulletType ); // shoot +-5 degrees

	// Eject a shell if necessary
	
	if ( nShellModel )
	{
		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecShootOrigin + vecShootDir * 24, vecShellVelocity, pev->angles.y, nShellModel, nShellBounce ); 
	}

	// Sound
	
	if ( strcmp( szSnd, "" ) != 0 )
	{
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, szSnd, 1, HUMAN_ATTN );
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );
	}

	// take away a bullet!

	m_cAmmoLoaded--;
	
	// Shows muzzleflash sprite and dynamic light

	pev->effects |= EF_MUZZLEFLASH;
	
	// Points gun in right direction on model

	Vector angDir = UTIL_VecToAngles( vecShootDir ); 
	SetBlending( 0, angDir.x );
	
}


//=========================================================
// Fire M79 grenade
//=========================================================

void CHuman::M79Fire()
{
	if ( m_cAmmoLoaded <= 0 ) return;

	if (RANDOM_LONG(0, 1))
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/m79_fire1.wav", 0.8, HUMAN_ATTN);
	else
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/m79_fire2.wav", 0.8, HUMAN_ATTN);

	UTIL_MakeVectors(pev->angles);
	if ( m_MonsterState == MONSTERSTATE_SCRIPT ) m_vecTossVelocity = gpGlobals->v_forward * 800;

	CGrenade::ShootContact( pev, GetGunEndPosition(), m_vecTossVelocity );
	m_LastAttack1Check = FALSE;
	m_cAmmoLoaded--;
	m_flNextAttack1Check = gpGlobals->time + 1;
	
	Vector angDir = UTIL_VecToAngles( m_vecTossVelocity );
	SetBlending( 0, angDir.x );
}


//=========================================================
// Fire RPG Rocket
//=========================================================

void CHuman :: RPGFire ( void )
{
	if ( m_cAmmoLoaded <= 0 ) return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecSrc );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	angDir.x = -angDir.x;

	CRpg7Rocket *pRocket = CRpg7Rocket::CreateRocket( GetGunPosition(), angDir, this );
	
	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/rpg7_fire1.wav", 1, HUMAN_ATTN );
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, Classify(), pev->origin, 384, 0.3 );

	m_LastAttack1Check = FALSE;
	m_cAmmoLoaded--;// take away a bullet!
	m_flNextAttack1Check = gpGlobals->time + 1;

}


//=========================================================
// GetGunEndPosition - position of the END of the gun
//=========================================================

Vector CHuman :: GetGunEndPosition( )
{
	Vector v, a;
	GetAttachment(0, v, a);

	return v;
}


//=========================================================
// GetGunPosition - height of the gun above the origin of the monster
//=========================================================

Vector CHuman :: GetGunPosition( )
{
	if (m_fCrouching )
	{
		return pev->origin + Vector( 0, 0, 36 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 62 );
	}
}


//=========================================================
// GetHeadPosition - what it says
//=========================================================

void CHuman :: GetHeadPosition( Vector &pos, Vector &angle )
{
	GetAttachment(1, pos, angle);
}


//=========================================================
// GetBloodDirection - for when blood is spurting out of neck
//=========================================================

void CHuman :: GetBloodDirection( Vector &pos, Vector &dir )
{
	Vector angle, head;

	GetAttachment(1, head, angle);
	GetAttachment(2, pos, angle);

	dir = head - pos;
}


//=========================================================
// Kick and return if you hit anything
//=========================================================

CBaseEntity *CHuman :: Kick( void )
{
	m_flLastKick = gpGlobals->time;

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
// MonsterThink - if head is cut off we need to spawn some blood here
//=========================================================

void CHuman :: MonsterThink ( void )
{
	// This is to ensure that if the human is repelling and for some reason his velocity is aimed upwards
	// he will not go shooting off into space

	if ( pev->movetype == MOVETYPE_FLY && pev->velocity.z >= 0 ) 
	{
		pev->velocity.z -= 10;
	}

	if (m_nHeadNum == GetNumHeads() && gpGlobals->time > m_LastBloodTime + 0.2)
	{
		Vector pos, dir;
		GetBloodDirection( pos, dir );

		UTIL_BloodStream( pos, dir, 70, 100 );
		m_LastBloodTime = gpGlobals->time;
	}
	
	CBaseMonster::MonsterThink();
}


//=========================================================
// TraceAttack - not to be confused with TakeDamage!
//=========================================================

void CHuman::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ((bitsDamageType & DMG_BLAST) && m_fCrouching) 
	{
		// If crouching, reduce damage from explosions by one quarter
		flDamage -= flDamage / 4;	

		// If cowering then reduce by a further quarter
		if ( m_Activity == ACT_COWER ) flDamage -= flDamage / 4;
	}

	if ( m_MonsterState != MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_PRONE && m_MonsterState != MONSTERSTATE_NONE &&
		ptr && (ptr->iHitgroup == HITGROUP_HEAD) && (m_nHeadNum != GetNumHeads()) && (bitsDamageType & DMG_SLASH) )
	{
		
		// If killed by blow from machete or chainsaw to head, chop off head

		CHumanHead * pHead = GetClassPtr( (CHumanHead *)NULL );
		
		if (pHead)
		{
			pHead->Spawn( GetHeadModelName(), m_nHeadNum );
			
			pHead->pev->velocity = vecDir;
			pHead->pev->velocity = pHead->pev->velocity * RANDOM_FLOAT ( 100, 150 );
			pHead->pev->velocity.z += 300;
			
			pHead->pev->avelocity = Vector( RANDOM_LONG(0, 200), RANDOM_LONG(0, 200), RANDOM_LONG(0, 200));

			GetHeadPosition( pHead->pev->origin, pHead->pev->angles );
		}
			
		m_nHeadNum = GetNumHeads();
		SetBodygroup( GetHeadGroupNum(), m_nHeadNum );
		SetActivity( m_Activity );
		
		SentenceStop();	// Don't talk if you have no head

		flDamage = pev->health;

		// BaseMonster TraceAttack procedure without the hitgroups since a machete to the head kills instantly
		
		m_LastHitGroup = ptr->iHitgroup;
		
		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
	else
	{
		CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
	}
}


//=========================================================
// TakeDamage
//=========================================================

int CHuman :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Forget( bits_MEMORY_INCOVER );

  	int ret = CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	if ( m_MonsterState == MONSTERSTATE_SCRIPT ||
		 m_MonsterState == MONSTERSTATE_PRONE ||
		 m_MonsterState == MONSTERSTATE_NONE ) return ret;

	if ( pevInflictor && IsAlive() && ( m_IdealMonsterState == MONSTERSTATE_DEAD ) && 
		( FBitSet( bitsDamageType, DMG_BLAST ) || flDamage > 100 ) )
	{
		Vector dir = pev->origin - pevInflictor->origin;

		//MakeIdealYaw( dir );
		float Yaw = VecToYaw( dir ) + ( 180 - pev->angles.y );
		if ( Yaw < 0 ) Yaw += 360;
		if ( Yaw >= 360 ) Yaw -= 360;

		if ( Yaw >= 180 - 45 && Yaw < 180 + 45 ) m_expldir = 1; // his back is to the explosion
		else if ( Yaw >= 180 + 45 && Yaw < 180 + 90 + 45 ) m_expldir = 3; // his right side is to the explosion
		else if ( Yaw >= 180 - 90 - 45 && Yaw < 180 - 45 ) m_expldir = 4; // his left side is to the explosion
		else m_expldir = 2; // his front is to the explosion

		if ( FBitSet( bitsDamageType, DMG_BLAST ) )
		{
			pev->velocity = dir.Normalize() * flDamage * 3;
			pev->velocity.z = flDamage * 5;
		}
		else
		{
			pev->velocity = dir.Normalize() * ( 100 + flDamage );
			pev->velocity.z = 200;
		}
		ChangeSchedule( GetScheduleOfType( SCHED_HUMAN_EXPLOSION_DIE ) );
		SetState( MONSTERSTATE_DEAD );
	}

	return ret;
}


//=========================================================
// Detect when I have hit the ground after an explosion
//=========================================================

void CHuman::ExplFlyTouch( CBaseEntity *pOther )
{
//	if ( FBitSet( pev->flags, FL_ONGROUND )  )
//	{
	m_IdealActivity = ACT_EXPLOSION_LAND;
	SetActivity( m_IdealActivity );
	TaskComplete();
	SetTouch( NULL );
//	}
}


//=========================================================
// Criteria for gibbing instead of just dying normally
//=========================================================

BOOL CHuman::ShouldGibMonster( int iGib )
{
	if ( ( iGib == GIB_NORMAL && pev->health < HUMAN_GIB_HEALTH_VALUE ) || ( iGib == GIB_ALWAYS ) )
		return TRUE;
	
	return FALSE;
}


//=========================================================
// Killed - called when he's killed
//=========================================================

void CHuman::Killed( entvars_t *pevAttacker, int iGib )
{
	SentenceStop();
	CSquadMonster::Killed( pevAttacker, iGib );
}


//=========================================================
// AlertFriends - when killed by player, tell friends player is evil
//=========================================================

void CHuman::AlertFriends( void )
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for ( i = 0; i < m_nNumFriendTypes; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, TRUE ))
		{
			CHuman *pHuman = pFriend->MyHumanPointer();
			if ( pHuman != NULL && pHuman->IsAlive() && pHuman->pev->deadflag != DEAD_DYING	&& pHuman->FVisible( this ) ) 
				
				// don't provoke a friend that's playing a death animation. They're a goner
				// also don't provoke friends that can't see you
			{
				pHuman->Remember( bits_MEMORY_PROVOKED );
				if ( pHuman->IsFollowingPlayer() ) pHuman->StopFollowing( TRUE );
			}
		}
	}
}


//=========================================================
// ShutUpFriends - when I'm talking as part of a script, 
// tell friends to shut it
//=========================================================

void CHuman::ShutUpFriends( void )
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for ( i = 0; i < m_nNumFriendTypes; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, TRUE ))
		{
			CBaseMonster *pMonster = pFriend->MyMonsterPointer();
			if ( pMonster )
			{
				pMonster->SentenceStop();
			}
		}
	}
}


//=========================================================
// CanPlaySentence - used by scripted sentences
//=========================================================

int CHuman::CanPlaySentence( BOOL fDisregardState ) 
{ 
	if ( fDisregardState )
		return CBaseMonster::CanPlaySentence( fDisregardState );
	return FOkToSpeak(); 
}


//=========================================================
// PlayScriptedSentence - what it says
//=========================================================

void CHuman::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	if ( !bConcurrent )
		ShutUpFriends();

	ClearConditions( bits_COND_PUSHED );	// Forget about moving!  I've got something to say!
	PlaySentence( pszSentence, duration, volume, attenuation, 100 );

	m_hTalkTarget = pListener;
}


//=========================================================
// PlayLabelledSentence - Adds a monster-specific label to
// the front of pszSentence and then calls PlaySentence
//=========================================================

void CHuman::PlayLabelledSentence( const char *pszSentence )
{
	if ( m_nHeadNum == GetNumHeads() ) return;

	char szSentence[32];
	strcpy( szSentence, m_szSpeechLabel );
	strcat( szSentence, pszSentence );

	PlaySentence( szSentence, GetDuration( pszSentence ), VOL_NORM, HUMAN_ATTN, GetVoicePitch() );
}


//=========================================================
// PlaySentence - Plays a sentence from sentences.txt
//=========================================================

void CHuman::PlaySentence( const char *pszSentence, float duration, float volume, float attenuation, float pitch )
{
	if ( !pszSentence )
		return;

	CHuman::g_talkWaitTime = gpGlobals->time + duration;
	if ( pszSentence[0] == '!' )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, pitch );
	else
		SENTENCEG_PlayRndSz( edict(), pszSentence, volume, attenuation, 0, pitch );

	Talk ( duration );

//	ALERT( at_console, "PlaySentence: %s\n", pszSentence );
}


//=========================================================
// Talk - set a timer that tells us when the monster is done
// talking.
//=========================================================

void CHuman :: Talk( float flDuration )
{
	if ( flDuration <= 0 )
	{
		// no duration :( 
		m_flStopTalkTime = gpGlobals->time + 3;
	}
	else
	{
		m_flStopTalkTime = gpGlobals->time + flDuration;
	}

	m_flLastTalkTime = gpGlobals->time;
}


//=========================================================
// turn head towards supplied origin
//=========================================================

void CHuman :: IdleHeadTurn( Vector &vecFriend )
{
	 // turn head in desired direction only if ent has a turnable head
	if (m_afCapability & bits_CAP_TURN_HEAD)
	{
		float yaw = VecToYaw(vecFriend - pev->origin) - pev->angles.y;

		if (yaw > 180) yaw -= 360;
		if (yaw < -180) yaw += 360;

		// turn towards vector
		SetBoneController( 0, yaw );
	}
}


//=========================================================
// Prepare this monster to answer question
//=========================================================

void CHuman :: SetAnswerQuestion( CHuman *pSpeaker, float duration )
{
	if ( SafeToChangeSchedule() )	ChangeSchedule( GetScheduleOfType( SCHED_HUMAN_IDLE_RESPONSE ) );
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
	m_flStopTalkTime = duration;
}


//=========================================================
// CheckAmmo - overridden for Human because he actually
// uses ammo! (base class doesn't)
//=========================================================

void CHuman :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}


//=========================================================
// Will a bullet fired from my gun hit the enemy?
//=========================================================

BOOL CHuman::CheckBulletAttack( float flDot, float flDist )
{
	if ( gpGlobals->time < m_flNextAttack1Check ) return m_LastAttack1Check;

	m_LastAttack1Check = FALSE;
	
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && 
		flDist <= HUMAN_RIFLE_RANGE && flDot >= 0.5 && NoFriendlyFire() )
	{
		// verify that a bullet fired from the gun will hit an enemy before the world.

		TraceResult	tr;
		Vector vecSrc = GetGunPosition();
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), dont_ignore_monsters, ignore_glass, ENT(pev), &tr);
		
		if ( tr.flFraction == 1.0 || (tr.pHit != NULL && IRelationship( CBaseEntity::Instance(tr.pHit) ) > R_NO ) )
		{
			m_LastAttack1Check = TRUE;
			m_fStopCrouching = FALSE;
		}
		else if (m_fCrouching)
		{
			vecSrc = pev->origin + Vector( 0, 0, 62 );
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), dont_ignore_monsters, ignore_glass, ENT(pev), &tr);

			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && IRelationship( CBaseEntity::Instance(tr.pHit) ) > R_NO ) )
			{
				m_LastAttack1Check = TRUE;
				m_fStopCrouching = TRUE;
			}
		}

		m_flNextAttack1Check = gpGlobals->time + 1.5;
	}

	return m_LastAttack1Check;
}


//=========================================================
// Will a rocket hit the enemy?
//=========================================================

BOOL CHuman::CheckRocketAttack( float flDot, float flDist )
{
	if ( gpGlobals->time < m_flNextAttack1Check ) return m_LastAttack1Check;
	m_LastAttack1Check = FALSE;

	// Don't shoot if enemy is too far away or too close

	if ( flDist > HUMAN_EXPLOSIVE_MAX_RANGE || flDist <= HUMAN_EXPLOSIVE_MIN_RANGE ) return m_LastAttack1Check;

	// if he isn't moving, it's ok to check.

	if ( m_flGroundSpeed != 0 )	return m_LastAttack1Check;

	// Will a rocket hit the enemy?

	TraceResult	tr;
	Vector vecSrc = GetGunPosition();
	Vector vecTarget = m_hEnemy->BodyTarget(vecSrc);
	UTIL_TraceLine( vecSrc, vecTarget, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr);

	if ( tr.flFraction == 1.0 || (tr.pHit != NULL && IRelationship( CBaseEntity::Instance(tr.pHit) ) > R_NO ) )
	{
		// Am I or any of my squad members near the impact area?

		Vector vecImpact = vecSrc + tr.flFraction * ( vecTarget - vecSrc );

		if (	( ( vecImpact - pev->origin ).Length2D() <= HUMAN_EXPLOSIVE_MIN_RANGE )
			||	( InSquad() && SquadMemberInRange( vecImpact, HUMAN_EXPLOSIVE_MIN_RANGE ) ) )
		{
			m_flNextAttack1Check = gpGlobals->time + 1;
			m_LastAttack1Check = FALSE;
			return m_LastAttack1Check;
		}

		m_LastAttack1Check = TRUE;
	}

	return m_LastAttack1Check;
}


//=========================================================
// Will a contact grenade hit the enemy?
//=========================================================

BOOL CHuman::CheckContactGrenadeAttack( float flDot, float flDist )
{
	if ( m_hEnemy == NULL ) return FALSE;

	// if he isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_LastAttack1Check = FALSE;
		return m_LastAttack1Check;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextAttack1Check )
	{
		return m_LastAttack1Check;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_LastAttack1Check = FALSE;
		return m_LastAttack1Check;
	}

	Vector vecTarget;

	// find target
	vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
	
	// estimate position
	if (HasConditions( bits_COND_SEE_ENEMY))
		vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;

	// Am I or any of my squad members near the intended grenade impact area?

	if (	( ( vecTarget - pev->origin ).Length2D() <= HUMAN_EXPLOSIVE_MIN_RANGE )
		||	( InSquad() && SquadMemberInRange( vecTarget, HUMAN_EXPLOSIVE_MIN_RANGE ) ) )
	{
		m_flNextAttack1Check = gpGlobals->time + 1;
		m_LastAttack1Check = FALSE;
		return m_LastAttack1Check;
	}

	Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5 );

	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;
		m_LastAttack1Check = TRUE;
		m_flNextAttack1Check = gpGlobals->time + 0.3; // 1/3 second.
	}
	else
	{
		m_LastAttack1Check = FALSE;
		m_flNextAttack1Check = gpGlobals->time + 1; // one full second.
	}

	return m_LastAttack1Check;
}


//=========================================================
// Will a grenade from my gun hit the enemy?
//=========================================================

BOOL CHuman::CheckTimedGrenadeAttack( float flDot, float flDist )
{
	if ( m_hEnemy == NULL ) return FALSE;

	// if he isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_LastAttack2Check = FALSE;
		return m_LastAttack2Check;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextAttack2Check )
	{
		return m_LastAttack2Check;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_LastAttack2Check = FALSE;
		return m_LastAttack2Check;
	}

	Vector vecTarget;

	vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
	vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;

	// Am I or any of my squad members near the intended grenade impact area?

	if (	( ( vecTarget - pev->origin ).Length2D() <= HUMAN_EXPLOSIVE_MIN_RANGE )
		||	( InSquad() && SquadMemberInRange( vecTarget, HUMAN_EXPLOSIVE_MIN_RANGE ) ) )
	{
		m_flNextAttack2Check = gpGlobals->time + 1;
		m_LastAttack2Check = FALSE;
		return m_LastAttack2Check;
	}


	Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;

		m_LastAttack2Check = TRUE;						// throw a hand grenade
		m_flNextAttack2Check = gpGlobals->time + 6; // don't check again for a while.
	}
	else
	{
		m_LastAttack2Check = FALSE;					// don't throw
		m_flNextAttack2Check = gpGlobals->time + 1; // don't check again for a while.
	}

	return m_LastAttack2Check;
}


//=========================================================
// CheckRangeAttack1
//=========================================================

BOOL CHuman :: CheckRangeAttack1 ( float flDot, float flDist )
{
	switch ( m_iWeapon )
	{
	case HUMAN_WEAPON_M79:
		return CheckContactGrenadeAttack( flDot, flDist );
		break;

	case HUMAN_WEAPON_RPG7:
		return CheckRocketAttack( flDot, flDist );
		break;

	default:
		return CheckBulletAttack( flDot, flDist );
		break;
	}
}


//=========================================================
// CheckRangeAttack2 - handgrenades
//=========================================================

BOOL CHuman :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( m_fHandGrenades )
	{
		return CheckTimedGrenadeAttack( flDot, flDist );
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================

BOOL CHuman :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Don't repeat kicks too often, unless we have an explosive weapon (i.e. might not have a choice)

	if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && m_flLastKick + KICK_INTERVAL > gpGlobals->time ) return FALSE;

	// Check if enemy is in range

	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= HUMAN_KICK_RANGE && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}


//=========================================================
// FCanCheckAttacks - this is overridden for humans
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

BOOL CHuman :: FCanCheckAttacks ( void )
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
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================

int CHuman :: ISoundMask ( void) 
{
	// If friendly to player, ignore sounds player makes when I'm talking so I don't keep turning to look at him
	// Also ignore player if I am in combat - I have more important things to worry about

	CBaseEntity *pPlayer = NULL;
	pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	if ( pPlayer && IRelationship( pPlayer ) < R_NO && 
		( IsTalking() || m_hTalkTarget != NULL || m_MonsterState == MONSTERSTATE_COMBAT ) )
	{
		return	bits_SOUND_WORLD	|
				bits_SOUND_COMBAT	|
				bits_SOUND_CARCASS	|
				bits_SOUND_MEAT		|
				bits_SOUND_GARBAGE	|
				bits_SOUND_DANGER;
	}
	else
	{
		return	bits_SOUND_WORLD	|
				bits_SOUND_COMBAT	|
				bits_SOUND_CARCASS	|
				bits_SOUND_MEAT		|
				bits_SOUND_GARBAGE	|
				bits_SOUND_DANGER	|
				bits_SOUND_PLAYER;
	}
}


//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================

void CHuman :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( m_iWeapon != HUMAN_WEAPON_NONE )
	{// throw a gun if he has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		pGun = DropItem( WeaponEntityName(), vecGunPos, vecGunAngles );
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		m_iWeapon = HUMAN_WEAPON_NONE;
		pev->weapons = GetWeaponBodyGroup( m_iWeapon );
	}

	CBaseMonster :: GibMonster();
}


//=========================================================
// IsTalking - am I saying a sentence right now?
//=========================================================

BOOL CHuman :: IsTalking( void )
{
	if ( m_flStopTalkTime > gpGlobals->time )
	{
		return TRUE;
	}

	return FALSE;
}


//=========================================================
// FindNearestFriend
// Scan for nearest visible friend - including those not in squad
//=========================================================

CBaseEntity *CHuman :: FindNearestFriend(BOOL fPlayer)
{
	CBaseEntity *pFriend = NULL;
	CBaseEntity *pNearest = NULL;
	float range = 10000000.0;
	TraceResult tr;
	Vector vecStart = pev->origin;
	Vector vecCheck;
	int i;
	char *pszFriend;

	vecStart.z = pev->absmax.z;
	
	// for each type of friend...

	for (i = m_nNumFriendTypes - 1; i > -1; i--)
	{
		pszFriend = m_szFriends[i];

		if (!pszFriend)
			continue;

		if (!fPlayer && pszFriend == "player")
			continue;

		// for each friend in this bsp...
		while (pFriend = UTIL_FindEntityByClassname( pFriend, pszFriend ))
		{
			if (pFriend == this )
				// don't talk to self - sound advice for anyone I would have thought
				continue;

			CBaseMonster *pMonster = pFriend->MyMonsterPointer();

			// If not a monster for some reason, or in a script, or prone
			if ( !pMonster || pMonster->m_MonsterState == MONSTERSTATE_SCRIPT || pMonster->m_MonsterState == MONSTERSTATE_PRONE )
				continue;

			vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;

			// if closer than previous friend, and in range, see if he's visible

			if (range > (vecStart - vecCheck).Length())
			{
				UTIL_TraceLine(vecStart, vecCheck, ignore_monsters, ENT(pev), &tr);

				if (tr.flFraction == 1.0)
				{
					// visible and in range, this is the new nearest friend
					if ((vecStart - vecCheck).Length() < TALKRANGE_MIN)
					{
						pNearest = pFriend;
						range = (vecStart - vecCheck).Length();
					}
				}
			}
		}
	}
	return pNearest;
}


//=========================================================
// EnumFriends - Loop through friends, includes those not in squad
//=========================================================

CBaseEntity	*CHuman::EnumFriends( CBaseEntity *pPrevious, int listNumber, BOOL bTrace )
{
	CBaseEntity *pFriend = pPrevious;
	char *pszFriend;
	TraceResult tr;
	Vector vecCheck;

	pszFriend = m_szFriends[ listNumber ];
	while (pFriend = UTIL_FindEntityByClassname( pFriend, pszFriend ))
	{
		if (pFriend == this || !pFriend->IsAlive())
			// don't talk to self or dead people
			continue;
		if ( bTrace )
		{
			vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;

			UTIL_TraceLine( pev->origin, vecCheck, ignore_monsters, ENT(pev), &tr);
		}
		else
			tr.flFraction = 1.0;

		if (tr.flFraction == 1.0)
		{
			return pFriend;
		}
	}

	return NULL;
}


//=========================================================
// StopFollowing
//=========================================================

void CHuman::StopFollowing( BOOL clearSchedule )
{
	if ( IsFollowing() )
	{
		if ( IsFollowingPlayer() )
		{
			m_nLastSquadCommand = SQUADCMD_NONE;
			
			if ( !HasMemory( bits_MEMORY_PROVOKED ) && m_hTargetEnt->IsAlive() )
			{
				PlayLabelledSentence( "UNUSE" );
				m_hTalkTarget = m_hTargetEnt;
			}
		}	

		if ( m_movementGoal == MOVEGOAL_TARGETENT )
			RouteClear(); // Stop him from walking toward the leader
		m_hTargetEnt = NULL;
		if ( clearSchedule && SafeToChangeSchedule() )
			ClearSchedule();
		if ( m_hEnemy != NULL )
			m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}
}


//=========================================================
// StartFollowing
//=========================================================

void CHuman::StartFollowing( CBaseEntity *pLeader )
{
	if (m_hTargetEnt == pLeader) return; // Don't start following someone I'm already following

	if ( m_pCine )
		m_pCine->CancelScript();

	if ( m_hEnemy != NULL )
		m_IdealMonsterState = MONSTERSTATE_ALERT;

	m_hTargetEnt = pLeader;
	m_hTalkTarget = m_hTargetEnt;
	ClearConditions( bits_COND_PUSHED );

	if ( SafeToChangeSchedule() )
	{
		ClearSchedule();
	}
}


//=========================================================
// LimitFollowers - otherwise it could get really silly
//=========================================================

void CHuman::LimitFollowers( CBaseEntity *pCaller, int maxFollowers )
{
	CBaseEntity *pFriend = NULL;
	int i, count;

	count = 0;
	// for each friend in this bsp...
	for ( i = 0; i < m_nNumFriendTypes; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, FALSE ))
		{
			CBaseMonster *pMonster = pFriend->MyMonsterPointer();
			if ( pMonster )
			{
				if ( pMonster->m_hTargetEnt == pCaller )
				{
					count++;
					if ( count > maxFollowers )
						pMonster->StopFollowing( TRUE );
				}
			}
		}
	}
}


//=========================================================
// CanFollow
//=========================================================

BOOL CHuman::CanFollow( void )
{
	if ( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		if ( !m_pCine->CanInterrupt() )
			return FALSE;
	}

	if ( pev->movetype == MOVETYPE_FLY ) return FALSE;
	
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return FALSE;

	return !IsFollowingPlayer();
}


//=========================================================
// SquadCmdLegalForNonLeader - returns true if Cmd can be
// given by members of the squad who are not the 
// designated leader
//=========================================================

BOOL CHuman :: SquadCmdLegalForNonLeader( SquadCommand Cmd )
{
	if ( Cmd == SQUADCMD_FOUND_ENEMY ||
		 Cmd == SQUADCMD_DEFENSE || 
		 Cmd == SQUADCMD_DISTRESS ) return TRUE;
	
	return FALSE;
}


//=========================================================
// SquadIssueCommand - Issues a command to everyone in the
// squad
//=========================================================

void CHuman :: SquadIssueCommand ( SquadCommand Cmd )
{
	if (!InSquad())
		return;

	if (IsFollowingPlayer() && !SquadCmdLegalForNonLeader( Cmd )) 
		return;

	CSquadMonster *pSquadLeader = MySquadLeader( );
	if ( !pSquadLeader ) return;

	CHuman *pHumanLeader = pSquadLeader->MyHumanPointer();
	if ( !pHumanLeader ) return;

	// Squad Leader refuses your request if the command has a lower priority than the one he is currently on
	if ( Cmd < pHumanLeader->m_nLastSquadCommand ) return;

	switch ( Cmd )
	{
	case SQUADCMD_NONE:					ALERT( at_console, "%s Issuing Squad Command: None\n", STRING( pev->classname ) ); break;
	case SQUADCMD_CHECK_IN:				ALERT( at_console, "%s Issuing Squad Command: Check In\n", STRING( pev->classname ) ); break;
	case SQUADCMD_SEARCH_AND_DESTROY:	ALERT( at_console, "%s Issuing Squad Command: Search and Destroy\n", STRING( pev->classname ) ); break;
	case SQUADCMD_OUTTA_MY_WAY:			ALERT( at_console, "%s Issuing Squad Command: Outta my way\n", STRING( pev->classname ) ); break;
	case SQUADCMD_FOUND_ENEMY:			ALERT( at_console, "%s Issuing Squad Command: Found Enemy\n", STRING( pev->classname ) ); break;
	case SQUADCMD_DISTRESS:				ALERT( at_console, "%s Issuing Squad Command: Distress\n", STRING( pev->classname ) ); break;
	case SQUADCMD_COME_HERE:			ALERT( at_console, "%s Issuing Squad Command: Come here\n", STRING( pev->classname ) ); break;
	case SQUADCMD_SURPRESSING_FIRE:		ALERT( at_console, "%s Issuing Squad Command: Surpressing Fire\n", STRING( pev->classname ) ); break;
	case SQUADCMD_ATTACK:				ALERT( at_console, "%s Issuing Squad Command: Attack\n", STRING( pev->classname ) ); break;
	case SQUADCMD_DEFENSE:				ALERT( at_console, "%s Issuing Squad Command: Defense\n", STRING( pev->classname ) ); break;
	case SQUADCMD_RETREAT:				ALERT( at_console, "%s Issuing Squad Command: Retreat\n", STRING( pev->classname ) ); break;
	case SQUADCMD_GET_DOWN:				ALERT( at_console, "%s Issuing Squad Command: Get Down\n", STRING( pev->classname ) ); break;
	case SQUADCMD_BEHIND_YOU:			ALERT( at_console, "%s Issuing Squad Command: Behind You\n", STRING( pev->classname ) ); break;
	}
	
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster *pMember = pHumanLeader->MySquadMember(i);
		if (pMember && ( pMember->pev->origin - pev->origin).Length() <= SQUAD_COMMAND_RANGE )
		{
			CHuman *pHuman = pMember->MyHumanPointer();
			if ( pHuman && ( !pHuman->IsFollowingPlayer() || SquadCmdLegalForNonLeader( Cmd ) ) )
			{
				pHuman->SquadReceiveCommand( Cmd );
			}
		}
	}
}


//=========================================================
// SquadReceiveCommand - Receives a command and takes an
// appropriate action
//=========================================================

void CHuman :: SquadReceiveCommand( SquadCommand Cmd )
{
	if ( !IsAlive() || pev->deadflag == DEAD_DYING ) return;

	m_fSquadCmdAcknowledged = FALSE;

	switch ( Cmd )
	{
	case SQUADCMD_ATTACK:				// Attack a designated target, if you are not attacking something else
		
		m_nLastSquadCommand = Cmd;
		m_flLastSquadCmdTime = gpGlobals->time;
		if ( !SafeToChangeSchedule() ) break;
		
		if ( !HasConditions( bits_COND_SEE_ENEMY) )
		{
			if ( SquadGetCommanderEnemy() )	// Try and set my enemy to my commander's enemy
			{ 
				if ( FOkToShout() )
				{
					PlayLabelledSentence( "AFFIRMATIVE" );
					m_fSquadCmdAcknowledged = TRUE;
				}
				ClearSchedule();
			}
			else
			{
				SquadReceiveCommand( SQUADCMD_SEARCH_AND_DESTROY ); // If I can't, try to search for a new enemy
			}
		}
		break;

	case SQUADCMD_SEARCH_AND_DESTROY:	// Search for a target to attack
		
		m_nLastSquadCommand = Cmd;
		m_flLastSquadCmdTime = gpGlobals->time;
		if ( !SafeToChangeSchedule() ) break;

		if ( !HasConditions( bits_COND_SEE_ENEMY) )
		{
			ClearSchedule();	// If not fighting, dump out of current schedule
		}
		break;

	case SQUADCMD_DEFENSE:				// Defend a designated squad member (i.e. someone who is wounded)
		
		m_nLastSquadCommand = Cmd;
		m_flLastSquadCmdTime = gpGlobals->time;
		if ( !SafeToChangeSchedule() ) break;

		if ( !HasConditions( bits_COND_SEE_ENEMY) )
		{
			ClearSchedule();	// If not fighting, dump out of current schedule
		}
		break;
	
	case SQUADCMD_RETREAT:				// Run away from whatever you are attacking
		
		m_nLastSquadCommand = Cmd;
		m_flLastSquadCmdTime = gpGlobals->time;
		if ( !SafeToChangeSchedule() ) break;

		if ( m_hEnemy != NULL )
		{
			// Change schedule immediately as this could be important
			ChangeSchedule( GetScheduleOfType( SCHED_HUMAN_RETREAT ) );	
		}
		else
		{
			// Have no enemy in Alert or idle state
			ChangeSchedule( GetScheduleOfType( SCHED_TAKE_COVER_FROM_ORIGIN ) );	
		}
		break;
	
	case SQUADCMD_SURPRESSING_FIRE:		// Fire at a designated place
		
		if ( !HasConditions( bits_COND_SEE_ENEMY) )	// If I am already fighting someone ignore this command
		{
			if ( SquadGetCommanderEnemy() ||		// Try and get new enemy
				 SquadGetCommanderLineOfSight() )	// Fire at the position my commander is looking at
			{
				m_nLastSquadCommand = Cmd;
				m_flLastSquadCmdTime = gpGlobals->time;
				if ( !SafeToChangeSchedule() ) break;
				ClearSchedule();
			}
		}
		break;

	case SQUADCMD_COME_HERE:			// Come to Squad Leader
		
		m_nLastSquadCommand = Cmd;
		m_flLastSquadCmdTime = gpGlobals->time;
		if ( !SafeToChangeSchedule() ) break;

		if ( InSquad() && !IsFollowingPlayer() ) 
		{
			StartFollowing( MySquadLeader() );
		}
		if ( !HasConditions( bits_COND_SEE_ENEMY) )
		{
			if ( FOkToShout() )
			{
				PlayLabelledSentence( "AFFIRMATIVE" );
				m_fSquadCmdAcknowledged = TRUE;
			}
			ClearSchedule();	// If not fighting, dump out of current schedule
		}
		break;

	case SQUADCMD_GET_DOWN:				// Crouch	- likely to be only given by player
		if ( SafeToChangeSchedule() )
		{
			ChangeSchedule( GetScheduleOfType( SCHED_COWER ) );
		}
		break;

	case SQUADCMD_BEHIND_YOU:			// Turn around - likely to be only given by player
		if ( SafeToChangeSchedule() )
		{
			ChangeSchedule( GetScheduleOfType( SCHED_HUMAN_TURN_ROUND ) );
		}
		break;

	case SQUADCMD_CHECK_IN:				// Tell everyone to check in

		m_nLastSquadCommand = Cmd;
		m_flLastSquadCmdTime = gpGlobals->time;
		if ( !SafeToChangeSchedule() ) break;

		if ( !HasConditions( bits_COND_SEE_ENEMY )  )	// If I have no enemy, give the all clear
		{
			ChangeSchedule( GetScheduleOfType( SCHED_HUMAN_CHECK_IN ) );
		}
		else	// If I have an enemy, inform my squad
		{
			ChangeSchedule( GetScheduleOfType( SCHED_HUMAN_FOUND_ENEMY ) );
		}
		break;

	case SQUADCMD_FOUND_ENEMY:
		{
			if ( m_hEnemy == NULL || !m_hEnemy->IsAlive() && SquadGetMemberEnemy() && SafeToChangeSchedule() )
			{
				ClearSchedule();
			}
		}
		break;

	case SQUADCMD_DISTRESS:
		{
			if ( m_hEnemy == NULL || !m_hEnemy->IsAlive() && SquadGetMemberEnemy() && SafeToChangeSchedule() )
			{
				ClearSchedule();
			}
		}
		break;
	}
}


//=========================================================
// SquadGetCommanderLineOfSight - Get the position the
// commander is looking at
//=========================================================

BOOL CHuman :: SquadGetCommanderLineOfSight()
{
	CBaseEntity *pCommander = NULL;

	if (IsFollowingPlayer())
	{
		pCommander = UTIL_FindEntityByClassname( NULL, "player" );
	}
	else if (InSquad())
	{
		pCommander = MySquadLeader();
	}

	if ( pCommander == NULL ) return FALSE;

	// Draw a line in front of the commander and point to that place

	UTIL_MakeAimVectors( pCommander->pev->angles );

	TraceResult tr;
	UTIL_TraceLine( pCommander->EyePosition(), gpGlobals->v_forward * HUMAN_RIFLE_RANGE, dont_ignore_monsters, 
		dont_ignore_glass, ENT(pCommander->pev), &tr);

	m_vecEnemyLKP = pCommander->EyePosition() + tr.flFraction * gpGlobals->v_forward * HUMAN_RIFLE_RANGE;

	return TRUE;
}


//=========================================================
// SquadGetCommanderEnemy - Get's the commander's enemy
//=========================================================

BOOL CHuman :: SquadGetCommanderEnemy()
{
	if (IsFollowingPlayer())
	{
		// Try and figure out what the player's enemy is

		CBaseEntity *pPlayer = NULL;
		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		if (pPlayer)
		{
			CBaseMonster * pPlayerMonPtr = pPlayer->MyMonsterPointer();
			if ( pPlayerMonPtr )
			{
				pPlayerMonPtr->Look( 2048 );

				CBaseEntity * pEnemy = pPlayerMonPtr->BestVisibleEnemy();
				if ( pEnemy )
				{
					if ( m_hEnemy != NULL ) 
					{
						// remember the current enemy
						PushEnemy( m_hEnemy, m_vecEnemyLKP );
					}
					// get a new enemy
					m_hEnemy = pEnemy;
					m_vecEnemyLKP = pEnemy->pev->origin;
					CheckEnemy( pEnemy );
					SetConditions ( bits_COND_NEW_ENEMY );

					return TRUE;
				}
			}
		}
	}
	
	if (InSquad())
	{
		// Get Squad Leader's enemy

		CSquadMonster *pSquadLeader = MySquadLeader();
		if ( !pSquadLeader ) return FALSE;

		CHuman *pCommander = pSquadLeader->MyHumanPointer();
		if ( !pCommander ) return FALSE;

		if ( pCommander->m_hEnemy == NULL ) return FALSE;

		if ( m_hEnemy != NULL ) 
		{
			// remember the current enemy
			PushEnemy( m_hEnemy, m_vecEnemyLKP );
		}
		// get a new enemy
		m_hEnemy = pCommander->m_hEnemy;
		m_vecEnemyLKP = m_hEnemy->pev->origin;
		CheckEnemy( m_hEnemy );
		SetConditions ( bits_COND_NEW_ENEMY );

		return TRUE;
	}
	
	return FALSE;
}


//=========================================================
// SquadGetWounded - Gets the guy with the lowest health,
// or returns false if they are all over 66%
//=========================================================

BOOL CHuman :: SquadGetWounded()
{
	if ( InSquad() && !IsFollowingPlayer() )
	{
		CSquadMonster *pWounded = NULL;
		CSquadMonster *pSquadLeader = MySquadLeader();
		if ( !pSquadLeader ) return FALSE;

		for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
		{
			CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
			if (pMember)
			{
				if ( !pWounded && !( pMember->pev->health > 2*pMember->pev->max_health / 3 ) )
				{
					pWounded = pMember;
				}
				else if ( pWounded && pWounded->pev->health > pMember->pev->health )
				{
					pWounded = pMember;
				}
			}
		}

		if ( pWounded && pWounded != this )  // No need to follow myself
		{
			StartFollowing( pWounded );
			return TRUE;	// if you found someone with low enough health, start following them and return true
		}
		else
		{
			return FALSE;
		}
	}
	else if (IsFollowingPlayer())
	{
		// NOTE - Try and find most damaged follower of player

		return FALSE;
	}
	else return FALSE;
}


//=========================================================
// SquadIsHealthy - determine whether the squad is more or less
// fit for action
//=========================================================

BOOL CHuman :: SquadIsHealthy()
{
	if (!InSquad())
		return ( pev->health > ( 2 * pev->max_health ) / 3 );

	int cnt = 0;

	CSquadMonster *pSquadLeader = MySquadLeader( );
	if ( !pSquadLeader ) return ( pev->health > ( 2 * pev->max_health ) / 3 );

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
		if (pMember)
		{
			// If one member of the squad is very sick (lower than 33%) then squad is not healthy
			if ( pMember->pev->health < pMember->pev->max_health / 3 ) return FALSE;

			cnt += pMember->pev->health;
		}
	}

	// If average health of each member is higher than 66% then squad is healthy
	return cnt > ( 2 * pev->max_health * pSquadLeader->SquadCount() ) / 3;
}


//=========================================================
// SquadGetMemberEnemy - Find a squad member who has an 
// enemy and adopt his enemy
//=========================================================

BOOL CHuman :: SquadGetMemberEnemy()
{
	if (!InSquad()) return FALSE;

	CSquadMonster *pSquadLeader = MySquadLeader( );
	if ( !pSquadLeader ) return FALSE;

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
		if (pMember && pMember->m_hEnemy != NULL && pMember->m_hEnemy->IsAlive() )
		{
			// get a new enemy
			m_hEnemy = pMember->m_hEnemy;
			m_vecEnemyLKP = pMember->m_vecEnemyLKP;
			SetConditions ( bits_COND_NEW_ENEMY );

			return TRUE;
		}
	}
	return FALSE;
}


//=========================================================
// SquadAnyIdle - Find out if there are any squad members
// who are doing bugger all and who can see me
//=========================================================

BOOL CHuman :: SquadAnyIdle()
{
	if (!InSquad()) return FALSE;

	CSquadMonster *pSquadLeader = MySquadLeader( );
	if ( !pSquadLeader ) return FALSE;

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
		if (pMember && pMember->IsAlive() && 
			( pMember->m_MonsterState == MONSTERSTATE_IDLE || pMember->m_MonsterState == MONSTERSTATE_ALERT )
			&& pMember->FVisible( this ))
		{
			return TRUE;
		}
	}
	return FALSE;
}


//=========================================================
// SquadIsScattered - is my squad scattered, or more accurately
// are they a long way away from me?
//=========================================================

BOOL CHuman :: SquadIsScattered()
{
	if (!InSquad()) return FALSE;

	float cnt = 0;

	CSquadMonster *pSquadLeader = MySquadLeader( );
	if ( !pSquadLeader ) return FALSE;

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
		if (pMember)
		{
			float flDist = (pMember->pev->origin - pev->origin).Length();

			// If one member of the squad is very far away (further than 768) then squad is scattered
			if ( flDist > 768 ) return FALSE;

			cnt += flDist;
		}
	}

	// If average distance of each member is further than 512 then squad is scattered
	return cnt > ( 512 * pSquadLeader->SquadCount() );
}


//=========================================================
// StartTask
//=========================================================

void CHuman :: StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_HUMAN_CHECK_FIRE:
		if ( NoFriendlyFire() )
		{
			TaskComplete();
		}
		else
		{
			TaskFail();
		}
		break;

	case TASK_HUMAN_CROUCH:
		m_flCrouchTime = gpGlobals->time + CROUCH_TIME;
		if (m_fCrouching || FBitSet( pev->spawnflags, SF_MONSTER_PREDISASTER ) || pev->waterlevel == 3 )
		{
			TaskComplete();
		}
		else
		{
			m_IdealActivity = ACT_CROUCH;
		}
		break;

	case TASK_HUMAN_UNCROUCH:
		if (!m_fCrouching)
		{
			TaskComplete();
		}
		else
		{
			m_IdealActivity = ACT_UNCROUCH;
		}
		break;

	case TASK_HUMAN_SOUND_EXPL:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) && m_nHeadNum != GetNumHeads() )
			{
				PlayLabelledSentence( "EXPL" );
				SquadIssueCommand( SQUADCMD_DISTRESS ); 
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_GRENADE:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout())
			{
				PlayLabelledSentence( "GREN" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_HEAR:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 !FBitSet( m_bitsSaid, bit_saidHeard ) && FOkToSpeak() )
			{
				PlayLabelledSentence( "HEAR" );
				SetBits( m_bitsSaid, bit_saidHeard );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_VICTORY:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 FOkToShout() )
			{
				PlayLabelledSentence( "KILL" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_MEDIC:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 FOkToShout() )
			{
				PlayLabelledSentence( "MEDIC" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_HELP:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 FOkToShout() )
			{
				PlayLabelledSentence( "HELP" );
				SquadIssueCommand( SQUADCMD_DEFENSE );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_CHECK_IN:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 FOkToShout() )
			{
				m_flWaitFinished = gpGlobals->time + 1;
				PlayLabelledSentence( "CHECK" );
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	case TASK_HUMAN_SOUND_CLEAR:
		{
			CSquadMonster *pSquadLeader = MySquadLeader();

			if ( InSquad() && pSquadLeader != this )
			{
				for (int i = 0; i < MAX_SQUAD_MEMBERS; i++ ) if ( pSquadLeader->m_hSquadMember[ i ] == this ) break;

				m_flWaitFinished = gpGlobals->time + i;
			}
			else
			{
				TaskFail();
			}
		}
		break;

	case TASK_HUMAN_SOUND_ATTACK:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 FOkToShout() )	
			{
				PlayLabelledSentence( "ATTACK" );
				m_IdealActivity = ACT_SIGNAL1;	// Advance signal
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	case TASK_HUMAN_SOUND_FOUND_ENEMY:
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 FOkToShout() )	
			{
				PlayLabelledSentence( "FOUND" );
				m_IdealActivity = ACT_SIGNAL1;	// Advance signal
			}
			else
			{
				TaskComplete();
			}
		break;

	case TASK_HUMAN_SOUND_SURPRESS:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 FOkToShout())
			{
				PlayLabelledSentence( "SURPRESS" );
				m_IdealActivity = ACT_SIGNAL1;	// Advance signal
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	case TASK_HUMAN_SOUND_SURPRESSING:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				!m_fSquadCmdAcknowledged && FOkToShout())
			{
				m_hTalkTarget = MySquadLeader();
				PlayLabelledSentence( "SURPRESSING" );
				m_fSquadCmdAcknowledged = TRUE;
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_SEARCHING:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				!m_fSquadCmdAcknowledged && FOkToShout())
			{
				m_hTalkTarget = MySquadLeader();
				PlayLabelledSentence( "SEARCHING" );
				m_fSquadCmdAcknowledged = TRUE;
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_RETREATING:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				!m_fSquadCmdAcknowledged && FOkToShout())
			{
				m_hTalkTarget = MySquadLeader();
				PlayLabelledSentence( "RETREATING" );
				m_fSquadCmdAcknowledged = TRUE;
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_AFFIRMATIVE:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				!m_fSquadCmdAcknowledged && FOkToShout())
			{
				m_hTalkTarget = MySquadLeader();
				PlayLabelledSentence( "AFFIRMATIVE" );
				m_fSquadCmdAcknowledged = TRUE;
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_THROW:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout())
			{
				PlayLabelledSentence( "THROW" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_COVER:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout() )
			{
				m_hTalkTarget = MySquadLeader();
				PlayLabelledSentence( "COVER" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_RETREAT:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout())
			{
				m_IdealActivity = ACT_SIGNAL3;	// Retreat signal
				PlayLabelledSentence( "RETREAT" );
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	case TASK_HUMAN_SOUND_SEARCH_AND_DESTROY:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout())
			{
				m_IdealActivity = ACT_SIGNAL2;	// Flank signal
				PlayLabelledSentence( "SEARCH" );
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	case TASK_HUMAN_SOUND_COME_TO_ME:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout())
			{
				m_IdealActivity = ACT_SIGNAL3;	// Retreat signal
				PlayLabelledSentence( "COME" );
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	case TASK_HUMAN_SOUND_RESPOND:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToSpeak())
			{
				PlayLabelledSentence( "ANSWER" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_CHARGE:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				 FOkToShout() && m_hEnemy != NULL )	
			{
				m_hTalkTarget = m_hEnemy;
				switch ( m_hEnemy->Classify() )
				{
				case CLASS_ALIEN_MONSTER: 
					PlayLabelledSentence( "ZOMBIE" );
					break;
			
				default:
					PlayLabelledSentence( "CHARGE" );
					break;
				}
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_TAUNT:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout())
			{
				m_hTalkTarget = m_hEnemy;
				PlayLabelledSentence( "TAUNT" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_HEALED:
		{
			if ( (pTask->flData == 0 || RANDOM_FLOAT(0,1) <= pTask->flData) &&
				FOkToShout())
			{
				m_hTalkTarget = m_hTargetEnt;
				PlayLabelledSentence( "HEALED" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_FIND_MEDIC:
		{
			// First try looking for a medic in my squad

			if ( InSquad() )
			{
				CSquadMonster *pSquadLeader = MySquadLeader( );
				if ( pSquadLeader ) for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
				{
					CSquadMonster *pMember = pSquadLeader->MySquadMember(i);
					if ( pMember && pMember != this )
					{
						CHuman *pHuman = pMember->MyHumanPointer();
						if ( pHuman && pHuman->IsMedic() && pHuman->pev->deadflag == DEAD_NO )
						{
							if ( !pHuman->IsFollowing() ) 
							{
								StartFollowing( pMember );
								pHuman->StartFollowing( this );
								TaskComplete();
							}
						}
					}
				}
			}

			// If that doesn't work, just look around and see if I can SEE a medic

			if ( !TaskIsComplete() ) 
			{
				CBaseEntity *pFriend = NULL;
				int i;

				// for each friend in this bsp...
				for ( i = 0; i < m_nNumFriendTypes; i++ )
				{
					while (pFriend = EnumFriends( pFriend, i, TRUE ))
					{
						CHuman *pHuman = pFriend->MyHumanPointer();
						if ( pHuman && pHuman->IsMedic() && pHuman != this && pHuman->pev->deadflag == DEAD_NO )
						{
							if ( !pHuman->IsFollowing() ) 
							{
								StartFollowing( pHuman );
								pHuman->StartFollowing( this );
								TaskComplete();
							}
						}
					}
				}
			}

			// If still can't find one, suffer in silence
			// And don't try to look for one again for a while

			if ( !TaskIsComplete() ) 
			{
				m_flLastMedicSearch = gpGlobals->time;
				TaskFail();
			}
		}
		break;

	case TASK_HUMAN_FORGET_SQUAD_COMMAND:
		{
			m_nLastSquadCommand = SQUADCMD_NONE;
			TaskComplete();
		}
		break;

/*	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			// Try and get to a place where you can see the enemy last known position by searching for progressively
			// closer positions to it NOTE - a bit time-consuming, would be better to create a dedicated procedure

			if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, 768, (m_vecEnemyLKP - pev->origin).Length() )
				&& FVisible( m_vecEnemyLKP ) )
			{
				TaskComplete();
			}
			else if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, 512, (m_vecEnemyLKP - pev->origin).Length() )
				&& FVisible( m_vecEnemyLKP ) )
			{
				TaskComplete();
			}
			else if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, 256, (m_vecEnemyLKP - pev->origin).Length() )
				&& FVisible( m_vecEnemyLKP ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemyLKP failed!!\n" );
				TaskFail();
			}
		}
		break;*/

	case TASK_RANGE_ATTACK1:
		{
			if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
			{
				TaskFail();
			}
			else
			{
				CSquadMonster::StartTask( pTask );	
			}
		}
		break;

	case TASK_HUMAN_EYECONTACT:
		break;

	case TASK_HUMAN_IDEALYAW:
		if (m_hTalkTarget != NULL)
		{
			pev->yaw_speed = 60;
			float yaw = VecToYaw(m_hTalkTarget->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			if (yaw < 0)
			{
				pev->ideal_yaw = min( yaw + 45, 0 ) + pev->angles.y;
			}
			else
			{
				pev->ideal_yaw = max( yaw - 45, 0 ) + pev->angles.y;
			}
		}
		TaskComplete();
		break;

	case TASK_HUMAN_EXPLOSION_FLY:
		{
			m_IdealActivity = ACT_EXPLOSION_FLY;
			pev->movetype = MOVETYPE_TOSS;
			pev->deadflag = DEAD_DYING;
			SetTouch( ExplFlyTouch );
		}
		break;

	case TASK_RUN_PATH:	// over-ridden because base class checks model file for ACT_RUN, which we don't have
		{
			m_movementActivity = ACT_RUN;
			TaskComplete();
		}
		break;

	case TASK_HUMAN_FACE_TOSS_DIR:
		{
		}
		break;

	case TASK_HUMAN_GET_EXPLOSIVE_PATH_TO_ENEMY:
		{
			if ( m_hEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if ( BuildNearestRoute( m_vecEnemyLKP, m_hEnemy->pev->view_ofs, 
				HUMAN_EXPLOSIVE_MIN_RANGE, HUMAN_EXPLOSIVE_MAX_RANGE ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail();
			}
		}
		break;

	case TASK_HUMAN_GET_MELEE_PATH_TO_ENEMY:
		{
			if ( m_hEnemy == NULL )
			{
				TaskFail();
				return;
			}

			CBaseEntity *pEnemy = m_hEnemy;

			if ( BuildRoute ( pEnemy->pev->origin, bits_MF_TO_ENEMY, pEnemy ) )
			{
				TaskComplete();
			}
			else if ( BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, 0, HUMAN_KICK_RANGE ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail();
			}
		}
		break;

	case TASK_HUMAN_RETREAT_FACE:
		{
			if ( m_hEnemy != NULL && HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				MakeIdealYaw( m_hEnemy->pev->origin );
				SetTurnActivity(); 
			}
			else
			{
				float flCurrentYaw;
			
				flCurrentYaw = UTIL_AngleMod( pev->angles.y );
				pev->ideal_yaw = UTIL_AngleMod( flCurrentYaw + pTask->flData );
				SetTurnActivity();
			}
		}
		break;

	case TASK_HUMAN_WAIT_GOAL_VISIBLE:
		{
		}
		break;

	case TASK_RUN_TO_TARGET:
	case TASK_WALK_TO_TARGET:
		{
			Activity newActivity;

			if ( (m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
			{
				TaskComplete();
			}
			else
			{
				if ( pTask->iTask == TASK_WALK_TO_TARGET )
					newActivity = ACT_WALK;
				else
					newActivity = ACT_RUN;

				if ( m_hTargetEnt == NULL || !MoveToTarget( newActivity, 2 ) )
				{
					TaskFail();
					ALERT( at_aiconsole, "%s Failed to reach target!!!\n", STRING(pev->classname) );
					RouteClear();
				}
			}
			TaskComplete();
			break;
		}

	default:
		if (IsTalking() && m_hTalkTarget != NULL)
		{
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		}
		else
		{
			SetBoneController( 0, 0 );
		}
		CSquadMonster::StartTask( pTask );	
		break;
	}
}


//=========================================================
// RunTask
//=========================================================

void CHuman :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HUMAN_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_HUMAN_CROUCH:
		if ( m_fSequenceFinished )
		{
			pev->view_ofs = Vector( 0, 0, 36 );
			m_fCrouching = TRUE;
			TaskComplete();
		}
		break;

	case TASK_HUMAN_UNCROUCH:
		if ( m_fSequenceFinished )
		{
			pev->view_ofs = Vector( 0, 0, 62 );
			m_fCrouching = FALSE;
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_ATTACK:
		if ( m_fSequenceFinished )
		{
			SquadIssueCommand( SQUADCMD_ATTACK ); 
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_FOUND_ENEMY:
		if ( m_fSequenceFinished )
		{
			SquadIssueCommand( SQUADCMD_FOUND_ENEMY ); 
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_SURPRESS:
		if ( m_fSequenceFinished )
		{
			SquadIssueCommand( SQUADCMD_SURPRESSING_FIRE ); 
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_SEARCH_AND_DESTROY:
		if ( m_fSequenceFinished )
		{
			SquadIssueCommand( SQUADCMD_SEARCH_AND_DESTROY ); 
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_RETREAT:
		if ( m_fSequenceFinished )
		{
			SquadIssueCommand( SQUADCMD_RETREAT ); 
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_COME_TO_ME:
		if ( m_fSequenceFinished )
		{
			SquadIssueCommand( SQUADCMD_COME_HERE ); 
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_CHECK_IN:
		if ( gpGlobals->time >= m_flWaitFinished )
		{
			SquadIssueCommand( SQUADCMD_CHECK_IN ); 
			TaskComplete();
		}
		break;

	case TASK_HUMAN_SOUND_CLEAR:
		if ( gpGlobals->time >= m_flWaitFinished )
		{
			if ( FOkToShout() )
			{
				PlayLabelledSentence( "CLEAR" );
			}
			TaskComplete();
		}
		break;

	case TASK_HUMAN_EYECONTACT:
		if (!IsMoving() && IsTalking() && m_hTalkTarget != NULL)
		{
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		}
		else
		{
			TaskComplete();
		}
		break;

	case TASK_HUMAN_EXPLOSION_FLY:
		{
			if ( FBitSet( pev->flags, FL_ONGROUND ) || pev->velocity == g_vecZero ) 
			{
				m_IdealActivity = ACT_EXPLOSION_LAND;
				SetActivity( m_IdealActivity );
				SetTouch( NULL );
				TaskComplete();
			}
		}
		break;

	case TASK_HUMAN_EXPLOSION_LAND:
		{
			if ( m_fSequenceFinished && pev->frame >= 255 )
			{
				pev->deadflag = DEAD_DEAD;
				
				SetThink ( NULL );
				StopAnimation();

				if ( !BBoxFlat() )
				{
					// a bit of a hack. If a corpses' bbox is positioned such that being left solid so that it can be attacked will
					// block the player on a slope or stairs, the corpse is made nonsolid. 
//					pev->solid = SOLID_NOT;
					UTIL_SetSize ( pev, Vector ( -4, -4, 0 ), Vector ( 4, 4, 1 ) );
				}
				else // !!!HACKHACK - put monster in a thin, wide bounding box until we fix the solid type/bounding volume problem
					UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + 1 ) );

				if ( ShouldFadeOnDeath() )
				{
					// this monster was created by a monstermaker... fade the corpse out.
					SUB_StartFadeOut();
				}
				else
				{
					// body is gonna be around for a while, so have it stink for a bit.
					CSoundEnt::InsertSound ( bits_SOUND_CARCASS, Classify(), pev->origin, 384, 30 );
				}
			}
		}
		break;

	case TASK_HUMAN_RETREAT_FACE:
		{
			if ( m_hEnemy != NULL && HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				MakeIdealYaw( m_hEnemy->pev->origin );
				ChangeYaw( pev->yaw_speed );

				if ( FacingIdeal() )
				{
					TaskComplete();
				}
			}
			else
			{
				ChangeYaw( pev->yaw_speed );
	
				if ( FacingIdeal() )
				{
					TaskComplete();
				}
			}
		}
		break;

	case TASK_HUMAN_WAIT_GOAL_VISIBLE:
		{
			if ( FVisible( m_vecMoveGoal ) ) 
			{
				TaskComplete();
			}
			else if ( ( m_vecMoveGoal - pev->origin).Length() < 256 )
			{
				TaskFail();
			}
		}
		break;

	case TASK_MOVE_TO_TARGET_RANGE:
		{
			float distance;

			if ( m_hTargetEnt == NULL || !m_hTargetEnt->IsAlive() || m_hTargetEnt->pev->deadflag == DEAD_DYING )
			{
				TaskFail();
			}
			else
			{
				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				// Re-evaluate when you think your finished, or the target has moved too far
				if ( (distance < pTask->flData) || (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length() > pTask->flData * 0.5 )
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					FRefreshRoute();
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if ( distance < pTask->flData )
				{
					TaskComplete();
					RouteClear();		// Stop moving
				}
				else 
				{
					// If a long way away or in a combat situation, run, otherwise walk

					CBaseMonster * pTargetMonster = m_hTargetEnt->MyMonsterPointer();

					if (  distance >= 270 ||
						  m_MonsterState == MONSTERSTATE_COMBAT || 
						  m_MonsterState == MONSTERSTATE_ALERT || 
							( pTargetMonster != NULL && 
								( pTargetMonster->m_MonsterState == MONSTERSTATE_COMBAT || 
								  pTargetMonster->m_MonsterState == MONSTERSTATE_ALERT	
								) 
							)
						)
					{
						m_movementActivity = ACT_RUN;
					}
					else if ( distance < 190 )
					{
						m_movementActivity = ACT_WALK;
					}
				}
			}
			break;
		}
	default:
		CSquadMonster::RunTask( pTask );
		break;
	}

}


//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================

void CHuman :: PrescheduleThink ( void )
{
	// If it's been a long time since my last squad command was given, forget about it

	if ( m_flLastSquadCmdTime && m_flLastSquadCmdTime < gpGlobals->time - SQUAD_COMMAND_MEMORY_TIME )
	{
		m_nLastSquadCommand = SQUADCMD_NONE;
		m_flLastSquadCmdTime = 0;
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

	CSquadMonster::PrescheduleThink();
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current squad command and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================

Schedule_t *CHuman :: GetScheduleFromSquadCommand ( void )
{
	if ( !IsAlive() || pev->deadflag == DEAD_DYING ) return NULL;

	switch ( m_nLastSquadCommand )
	{
	case SQUADCMD_RETREAT:
		if ( m_hEnemy == NULL || HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		{
			// I can't see anything bad so the retreat was successful.  Yay!
			m_nLastSquadCommand = SQUADCMD_NONE;
			
			if (!IsFollowingPlayer() && InSquad() && IsLeader())
			{
				// Now tell everyone to regroup for another attack
				return GetScheduleOfType( SCHED_HUMAN_SIGNAL_COME_TO_ME );
			}
		}
		else if ( !HasMemory( bits_MEMORY_HUMAN_NO_COVER ) )
		{
			return GetScheduleOfType( SCHED_HUMAN_RETREAT );
		}
		break;

	case SQUADCMD_COME_HERE:
		if ( IsFollowingPlayer() && (m_hTargetEnt->pev->origin - pev->origin).Length() > 64 ) 
		{
			return GetScheduleOfType( SCHED_HUMAN_FOLLOW_CLOSE );
		}
		else if	( IsFollowingHuman() && (m_hTargetEnt->pev->origin - pev->origin).Length() > 256  )
		{
			return GetScheduleOfType( SCHED_HUMAN_FOLLOW );
		}
		else
		{
			// I am now close to my target (or have stopped following)
			m_nLastSquadCommand = SQUADCMD_NONE;
			if ( IsFollowingHuman() ) StopFollowing( FALSE );
		}
		break;

	case SQUADCMD_DEFENSE:
		if ( !SquadGetWounded() )
		{
			// There is no wounded, so defense was successful.  Yay!  Or he could have died, of course.
			m_nLastSquadCommand = SQUADCMD_NONE;
			if ( IsFollowingHuman() ) StopFollowing( FALSE );
		}
		else
		{
			return GetScheduleOfType( SCHED_TARGET_CHASE );
		}
		break;

	case SQUADCMD_ATTACK:
		if ( ( m_hEnemy != NULL && m_hEnemy->IsAlive() ) || SquadGetCommanderEnemy() )	// Try and get new enemy
		{
			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HUMAN_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			else
			{
				return GetScheduleOfType( SCHED_HUMAN_ESTABLISH_LINE_OF_FIRE );
			}
		}

		// Squad Commander has no enemy, so attack was successful.  Yay!
		m_nLastSquadCommand = SQUADCMD_NONE;
		break;

	case SQUADCMD_SEARCH_AND_DESTROY:
		if ( m_hEnemy != NULL && m_hEnemy->IsAlive() )	// If I have an enemy then go after him
		{
			return GetScheduleOfType( SCHED_HUMAN_ESTABLISH_LINE_OF_FIRE );
		}
		else if ( HasConditions(bits_COND_HEAR_SOUND) )
		{
			return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
		}
		else
		{
			return GetScheduleOfType( SCHED_HUMAN_SEARCH_AND_DESTROY );
		}
		break;

	case SQUADCMD_SURPRESSING_FIRE:
		if ( m_vecEnemyLKP != g_vecZero && FVisible( m_vecEnemyLKP ) )
		{
			if ( NoFriendlyFire() && OccupySlot( bits_SLOTS_HUMAN_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_HUMAN_SURPRESS );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && m_fHandGrenades && 
				OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			else
			{
				// If there is someone in the way then give up
				m_nLastSquadCommand = SQUADCMD_NONE;
			}
		}
		else
		{
			return GetScheduleOfType( SCHED_HUMAN_MOVE_TO_ENEMY_LKP );
		}
		break;
	}

	return NULL;
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================

Schedule_t *CHuman :: GetSchedule ( void )
{
	// Flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 

	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// If I have landed on top of someone

			if ( pev->groundentity )	
			{
				CBaseEntity * pEntity = Instance( pev->groundentity );
				
				// If it is an enemy, shoot them
				
				if ( pEntity && IRelationship( pEntity ) > R_NO && pEntity->IsAlive() )	
				{
					CBaseMonster *pMonster = pEntity->MyMonsterPointer();
					if ( pMonster && pMonster->pev->deadflag == DEAD_NO && pMonster->pev->movetype != MOVETYPE_FLY &&
						pMonster->m_MonsterState == MONSTERSTATE_IDLE &&
						pMonster->m_MonsterState == MONSTERSTATE_ALERT &&
						pMonster->m_MonsterState == MONSTERSTATE_COMBAT )
					{
						pMonster->ChangeSchedule( pMonster->GetScheduleOfType( SCHED_TAKE_COVER_FROM_ORIGIN ) );
					}

					m_hEnemy = pEntity;
					return GetScheduleOfType ( SCHED_HUMAN_REPEL_ATTACK );
				}
				
				// If it is a friend, tell them to get out the way

				else if ( pEntity && IRelationship( pEntity ) == R_AL && pEntity->IsAlive() )	
				{
					CHuman * pHuman = pEntity->MyHumanPointer();
					
					if ( pHuman && !pHuman->IsMoving() && pHuman->SafeToChangeSchedule() )
					{
						pHuman->ChangeSchedule( pHuman->GetScheduleOfType( SCHED_TAKE_COVER_FROM_ORIGIN ) );
						return GetScheduleOfType ( SCHED_HUMAN_REPEL );	// Keep repelling a while longer till they are gone
					}
				}
			}

			// just landed

			pev->movetype = MOVETYPE_STEP;

			// If squad leader, tell my squad to go looking for trouble as soon as we've landed

			if ( InSquad() && IsLeader() && m_hEnemy == NULL )
			{
				return GetScheduleOfType ( SCHED_HUMAN_REPEL_LAND_SEARCH_AND_DESTROY );
			}
			else
			{
				return GetScheduleOfType ( SCHED_HUMAN_REPEL_LAND );
			}
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT && !HasConditions( bits_COND_NO_AMMO_LOADED ) &&
				HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) ) 
			{
				return GetScheduleOfType ( SCHED_HUMAN_REPEL_ATTACK );
			}
			else
			{
				return GetScheduleOfType ( SCHED_HUMAN_REPEL );
			}
		}
	}


	// Humans place HIGH priority on running away from danger sounds.

	CSound *pSound = NULL;

	if ( HasConditions(bits_COND_HEAR_SOUND) && m_MonsterState != MONSTERSTATE_SCRIPT )
	{
		pSound = PBestSound();
		if ( pSound )
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}

	
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// Squad Commands that are urgent enough to respond to even in the thick of combat

			if ( m_nLastSquadCommand >= SQUADCMD_RETREAT ) 
			{
				Schedule_t * pSchedule = GetScheduleFromSquadCommand();
				if ( pSchedule != NULL ) return pSchedule;
			}

			// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				m_bitsSaid = 0;	// Clear conversation bits

				// call base class, all code to handle dead enemies is centralized there.
				return CSquadMonster :: GetSchedule();
			}

			// no ammo
			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_HUMAN_COVER_AND_RELOAD );
			}

			// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( !FBitSet( m_afCapability, ( bits_CAP_RANGE_ATTACK1 | bits_CAP_RANGE_ATTACK2 | bits_CAP_MELEE_ATTACK1 ) ) )
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else if ( InSquad() )
				{
					CSquadMonster *pSquadLeader = MySquadLeader();
					if ( pSquadLeader ) pSquadLeader->m_fEnemyEluded = FALSE;
					
					if (!IsLeader())
					{
						if ( SquadAnyIdle() )	// If anyone in my squad isn't doing anything, inform them
						{
							return GetScheduleOfType ( SCHED_HUMAN_FOUND_ENEMY );
						}
					}
					else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						if ( SquadIsHealthy() )
						{
							// Attack this target and signal to my squad to do likewise

							if ( m_nLastSquadCommand < SQUADCMD_ATTACK && OccupySlot( bits_SLOTS_HUMAN_ENGAGE ) )
							{
								return GetScheduleOfType( SCHED_HUMAN_SIGNAL_ATTACK );
							}
						}
						else if ( m_nLastSquadCommand < SQUADCMD_RETREAT )
						{
							// Decide to fire a few shots and signal an orderly retreat
							return GetScheduleOfType( SCHED_HUMAN_SIGNAL_RETREAT );
						}
					}
				}

				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}

			// damaged
			if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			{
				if ( pev->health < pev->max_health / 3 && m_flLastMedicSearch + MEDIC_SEARCH_TIME < gpGlobals->time )
				{
					// Find a medic, this schedule will also call my squad to defend me (or at least the weakest member)
					return GetScheduleOfType( SCHED_HUMAN_FIND_MEDIC_COMBAT );
				}
				else if ( pev->health < pev->max_health - ( pev->max_health / 3 ) && !HasMemory( bits_MEMORY_HUMAN_NO_COVER ) )
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}

			// can kick
			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
			
			// can shoot
			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					CSquadMonster *pSquadLeader = MySquadLeader();
					if ( pSquadLeader ) pSquadLeader->m_fEnemyEluded = FALSE;

					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( !HasConditions ( bits_COND_ENEMY_FACING_ME ) && SquadAnyIdle() )
					{
						return GetScheduleOfType ( SCHED_HUMAN_FOUND_ENEMY );
					}

					if ( IsLeader() )
					{
						if ( SquadIsHealthy() && m_nLastSquadCommand < SQUADCMD_ATTACK )
						{
							// We are fit so pro-actively attack the bastards
							return GetScheduleOfType( SCHED_HUMAN_SIGNAL_ATTACK );
						}
						else if ( m_nLastSquadCommand < SQUADCMD_SURPRESSING_FIRE )
						{
							// Things are not looking so good, we are less confident, so lay down surpressing fire
							return GetScheduleOfType( SCHED_HUMAN_SIGNAL_SURPRESS );
						}
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

			// can't see enemy (even if I turn round)
			if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				// Squad Commands that I don't care about if I'm in the thick of battle but may pay attention to
				// if I can't see my enemy even if he's not dead

				Schedule_t * pSchedule = GetScheduleFromSquadCommand();
				if ( pSchedule != NULL ) return pSchedule;

				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( pev->health < pev->max_health / 3 && m_flLastMedicSearch + MEDIC_SEARCH_TIME < gpGlobals->time )
				{
					// Find a medic
					return GetScheduleOfType( SCHED_HUMAN_FIND_MEDIC_COMBAT );
				}
				else if ( FBitSet( m_afCapability, bits_CAP_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_HUMAN_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}

			// If we can see the enemy but can't attack him then we need to establish a line of fire whatever our
			// slot or squad command is
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( FBitSet( m_afCapability, bits_CAP_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_HUMAN_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
		}
		break;

	case MONSTERSTATE_ALERT:
		{
			// Taking damage code is in the BaseMonster Schedule and has high priority

			if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) )
			{
				return CSquadMonster::GetSchedule();
			}

			// If we have just killed the enemy and haven't got anyone else to shoot
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				Forget( bits_MEMORY_HUMAN_NO_COVER ); // If my enemy is killed the cover situation may have changed

				if ( InSquad() && !IsFollowingPlayer() && IsLeader() )
				{
					// If I am the squad leader, now would be the time to issue some kind of squad order

					if ( m_nLastSquadCommand < SQUADCMD_CHECK_IN )	
					{
						// Try and get a new enemy from one of my squad members
						return GetScheduleOfType( SCHED_HUMAN_SIGNAL_CHECK_IN );
					}
					else if ( SquadIsScattered() && m_nLastSquadCommand < SQUADCMD_COME_HERE )
					{
						// Order my squad to report back in and regroup
						return GetScheduleOfType( SCHED_HUMAN_SIGNAL_COME_TO_ME );
					} 
					else if ( m_nLastSquadCommand < SQUADCMD_SEARCH_AND_DESTROY )
					{
						// If we have no enemies and are close together it's time for some action
						return GetScheduleOfType( SCHED_HUMAN_SIGNAL_SEARCH_AND_DESTROY );
					}
				}

				if ( pev->health <= ( 2 * pev->max_health ) / 3 && m_flLastMedicSearch + MEDIC_SEARCH_TIME < gpGlobals->time )
				{
					return GetScheduleOfType( SCHED_HUMAN_FIND_MEDIC );
				}

				return GetScheduleOfType ( SCHED_VICTORY_DANCE );
			}

		}

	case MONSTERSTATE_IDLE:
		{
			// no ammo
			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				return GetScheduleOfType ( SCHED_HUMAN_RELOAD );
			}

			// Squad Commands that only need a response when not fighting
			// ( This applies to all squad commands )

			Schedule_t * pSchedule = GetScheduleFromSquadCommand();
			if ( pSchedule != NULL ) return pSchedule;

			// If have been crouching for long time get up
			
			if ( m_fCrouching && ( m_flCrouchTime <= gpGlobals->time || pev->waterlevel == 3 ) )
			{
				return GetScheduleOfType( SCHED_HUMAN_UNCROUCH );
			}

			// Follow leader

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
					return GetScheduleOfType( SCHED_TARGET_FACE );
				}
			}

			// If you hear a sound that you can't see the source of (and isn't a grenade) 
			// then crouch and say "Shhh... I hear something".   

			if ( pSound && pSound->FIsSound() )
			{
				if ( !FVisible( pSound->m_vecOrigin ) )
				{
					return GetScheduleOfType( SCHED_HUMAN_HEAR_SOUND );
				}
				else
				{
					return GetScheduleOfType( SCHED_ALERT_FACE );
				}
			}
	
		}
		break;
	}

	return CSquadMonster::GetSchedule();
}


//=========================================================
// GetIdealState - well you know
//=========================================================

MONSTERSTATE CHuman :: GetIdealState ( void )
{
	return CSquadMonster::GetIdealState();
}


//=========================================================
// Custom Schedules
//=========================================================

//=========================================================
// run to cover.
//=========================================================

Task_t	tlHumanTakeCover[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_HUMAN_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_HUMAN_SOUND_COVER,		(float)0.5							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HUMAN_WAIT_FACE_ENEMY	},
};

Schedule_t	slHumanTakeCover[] =
{
	{ 
		tlHumanTakeCover,
		ARRAYSIZE ( tlHumanTakeCover ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanTakeCover"
	},
};

//=========================================================
// Retreat - like slHumanTakeCover except in response to an
// order by squad commander
//=========================================================

Task_t	tlHumanRetreat[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_HUMAN_TAKECOVER_FAILED	},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_HUMAN_SOUND_RETREATING,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HUMAN_WAIT_FACE_ENEMY	},
};

Schedule_t	slHumanRetreat[] =
{
	{ 
		tlHumanRetreat,
		ARRAYSIZE ( tlHumanRetreat ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanRetreat"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================

Task_t	tlHumanGrenadeCover[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_HUMAN_TAKE_COVER_FROM_ENEMY_NO_GRENADE },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_HUMAN_SOUND_COVER,				(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_HUMAN_WAIT_FACE_ENEMY	},
};

Schedule_t	slHumanGrenadeCover[] =
{
	{ 
		tlHumanGrenadeCover,
		ARRAYSIZE ( tlHumanGrenadeCover ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanGrenadeCover"
	},
};


//=========================================================
// toss grenade then run to cover.
//=========================================================
Task_t	tlHumanTossGrenadeCover[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_HUMAN_SOUND_THROW,				(float)0.5							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_HUMAN_TAKE_COVER_FROM_ENEMY_NO_GRENADE	},
};

Schedule_t	slHumanTossGrenadeCover[] =
{
	{ 
		tlHumanTossGrenadeCover,
		ARRAYSIZE ( tlHumanTossGrenadeCover ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanTossGrenadeCover"
	},
};


//=========================================================
// hide from the loudest sound source (to run from grenade)
// Over-rides base because we want human to crouch and say
// "grenade!"
//=========================================================

Task_t	tlHumanTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_HUMAN_SOUND_GRENADE,			(float)0.5					},
	{ TASK_HUMAN_CROUCH,				(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slHumanTakeCoverFromBestSound[] =
{
	{ 
		tlHumanTakeCoverFromBestSound,
		ARRAYSIZE ( tlHumanTakeCoverFromBestSound ), 
		0,
		0,
		"HumanTakeCoverFromBestSound"
	},
};


//=========================================================
// Cower - this is what is usually done when attempts
// to escape danger fail.
// Over-rides base Cower because we want the human to crouch
//=========================================================

Task_t	tlHumanCower[] =
{
	{ TASK_STOP_MOVING,			0					},
	{ TASK_HUMAN_CROUCH,		(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_COWER	},
};

Schedule_t	slHumanCower[] =
{
	{
		tlHumanCower,
		ARRAYSIZE ( tlHumanCower ),
		0,
		0,
		"HumanCower"
	},
};


//=========================================================
// Hear Sound Schedule - crouch and go "Shh"
//=========================================================

Task_t	tlHumanHearSound[] =
{
	{ TASK_STOP_MOVING,				0				},
	{ TASK_HUMAN_CROUCH,			(float)0		},
	{ TASK_HUMAN_SOUND_HEAR,		(float)0.5		},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE	},
	{ TASK_FACE_IDEAL,				(float)0		},
	{ TASK_WAIT,					(float)3		},
};

Schedule_t	slHumanHearSound[] =
{
	{ 
		tlHumanHearSound,
		ARRAYSIZE ( tlHumanHearSound ),
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_FEAR		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_PROVOKED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Human Hear Sound"
	},
};


//=========================================================
// HumanInvestigateSound - sends a monster to the location of the
// sound that was just heard, to check things out. 
// Over-rides base InvestigateSound because we want the human
// to crouch and say "Shh"
//=========================================================

Task_t tlHumanInvestigateSound[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSOUND,	(float)0				},
	{ TASK_HUMAN_CROUCH,			(float)0				},
	{ TASK_HUMAN_SOUND_HEAR,		(float)0.5				},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE_ANGRY	},
	{ TASK_WAIT,					(float)10				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slHumanInvestigateSound[] =
{
	{ 
		tlHumanInvestigateSound,
		ARRAYSIZE ( tlHumanInvestigateSound ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanInvestigateSound"
	},
};


//=========================================================
// Victory Dance
//=========================================================

Task_t tlHumanVictoryDance[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_HUMAN_UNCROUCH,		(float)0					},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_HUMAN_SOUND_VICTORY,	(float)0					},
	{ TASK_WAIT,				(float)4					},
	{ TASK_SUGGEST_STATE,		(float)MONSTERSTATE_IDLE	},
};

Schedule_t slHumanVictoryDance[] =
{
	{
		tlHumanVictoryDance,
		ARRAYSIZE( tlHumanVictoryDance ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Human Victory Dance"
	},
};


//=========================================================
// Hide and reload schedule
//=========================================================

Task_t	tlHumanHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_HUMAN_CROUCH,			(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slHumanHideReload[] = 
{
	{
		tlHumanHideReload,
		ARRAYSIZE ( tlHumanHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"HumanHideReload"
	}
};


//=========================================================
// Reload schedule, overrides base class because we need
// to crouch
//=========================================================

Task_t	tlHumanReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_HUMAN_CROUCH,			(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slHumanReload[] = 
{
	{
		tlHumanReload,
		ARRAYSIZE ( tlHumanReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"HumanReload"
	}
};


//=========================================================
// Signal squad to defend me, Find Medic, 
// or if you can't just run like a bastard
//=========================================================

Task_t tlHumanFindMedicCombat[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_TAKE_COVER_FROM_ENEMY	},
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_HUMAN_SOUND_HELP,			(float)0					},
	{ TASK_HUMAN_FIND_MEDIC,			(float)0					},
	{ TASK_GET_PATH_TO_TARGET,			(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_FACE_TARGET,					(float)179					},
};

Schedule_t	slHumanFindMedicCombat[] =
{
	{ 
		tlHumanFindMedicCombat,
		ARRAYSIZE ( tlHumanFindMedicCombat ), 
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"HumanFindMedicCombat"
	},
};


//=========================================================
// Find Medic
//=========================================================

Task_t tlHumanFindMedic[] =
{
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_HUMAN_SOUND_MEDIC,			(float)0					},
	{ TASK_HUMAN_FIND_MEDIC,			(float)0					},
	{ TASK_GET_PATH_TO_TARGET,			(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_FACE_TARGET,					(float)179					},
};

Schedule_t	slHumanFindMedic[] =
{
	{ 
		tlHumanFindMedic,
		ARRAYSIZE ( tlHumanFindMedic ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"HumanFindMedic"
	},
};


//=========================================================
// SignalAttack - Signal Attack and then start shooting
//=========================================================

Task_t	tlHumanSignalAttack[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_HUMAN_SOUND_ATTACK,			(float)0				},		// This will do a hand signal as well
	{ TASK_SET_SCHEDULE,				(float)SCHED_RANGE_ATTACK1 },
};

Schedule_t	slHumanSignalAttack[] =
{
	{ 
		tlHumanSignalAttack,
		ARRAYSIZE ( tlHumanSignalAttack ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"HumanSignalAttack"
	},
};


//=========================================================
// SignalSurpress - Signal to lay down supressing fire and then 
// lay some down yourself
//=========================================================

Task_t	tlHumanSignalSurpress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_HUMAN_SOUND_SURPRESS,		(float)0				},
	{ TASK_SET_SCHEDULE,				(float)SCHED_HUMAN_SURPRESS },
};

Schedule_t	slHumanSignalSurpress[] =
{
	{ 
		tlHumanSignalSurpress,
		ARRAYSIZE ( tlHumanSignalSurpress ), 
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"HumanSignalSurpress"
	},
};


//=========================================================
// SignalSearchAndDestroy - Signal to Search and Destroy
// lay some down yourself
//=========================================================

Task_t	tlHumanSignalSearchAndDestroy[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_HUMAN_SOUND_SEARCH_AND_DESTROY,	(float)0			},
	{ TASK_SET_ACTIVITY,				(float)ACT_IDLE_ANGRY	},
	{ TASK_WAIT,						(float)2				}, // Wait for your troops to get going before you follow
	{ TASK_SET_SCHEDULE,				(float)SCHED_HUMAN_SEARCH_AND_DESTROY },
};

Schedule_t	slHumanSignalSearchAndDestroy[] =
{
	{ 
		tlHumanSignalSearchAndDestroy,
		ARRAYSIZE ( tlHumanSignalSearchAndDestroy ), 
		bits_COND_HEAR_SOUND		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,

		bits_SOUND_DANGER,
		"HumanSignalSearchAndDestroy"
	},
};


//=========================================================
// SignalRetreat - Signal to retreat, then shoot at the
// enemy for a bit so the others get a chance to run before
// running yourself
//=========================================================

Task_t	tlHumanSignalRetreat[] =
{
	{ TASK_STOP_MOVING,				0									},
	{ TASK_FACE_IDEAL,				(float)0							},
	{ TASK_HUMAN_SOUND_RETREAT,		(float)0							},	// Plays retreat signal as well
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_TAKE_COVER_FROM_ENEMY	}, // May fail if CheckFire fails, 
	{ TASK_FACE_ENEMY,				(float)0							},// i.e. I could hit a friendly
	{ TASK_HUMAN_CHECK_FIRE,		(float)0							},
	{ TASK_RANGE_ATTACK1,			(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_HUMAN_CHECK_FIRE,		(float)0							},
	{ TASK_RANGE_ATTACK1,			(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_HUMAN_CHECK_FIRE,		(float)0							},
	{ TASK_RANGE_ATTACK1,			(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_HUMAN_CHECK_FIRE,		(float)0							},
	{ TASK_RANGE_ATTACK1,			(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_HUMAN_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HUMAN_WAIT_FACE_ENEMY	},
};

Schedule_t	slHumanSignalRetreat[] =
{
	{ 
		tlHumanSignalRetreat,
		ARRAYSIZE ( tlHumanSignalRetreat ), 
		bits_COND_HEAR_SOUND		|
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_PROVOKED,

		bits_SOUND_DANGER,
		"HumanSignalRetreat"
	},
};


//=========================================================
// SignalComeToMe - Stand up and signal to come to me
//=========================================================

Task_t	tlHumanSignalComeToMe[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_HUMAN_UNCROUCH,				(float)0				},
	{ TASK_HUMAN_SOUND_COME_TO_ME,		(float)0				},
	{ TASK_SET_ACTIVITY,				(float)ACT_IDLE			},
	{ TASK_WAIT,						(float)10				},	// Wait a good long time for everyone to get there
};

Schedule_t	slHumanSignalComeToMe[] =
{
	{ 
		tlHumanSignalComeToMe,
		ARRAYSIZE ( tlHumanSignalComeToMe ), 
		bits_COND_HEAR_SOUND		|
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_SEE_DISLIKE		|
		bits_COND_SEE_HATE			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_PROVOKED,

		bits_SOUND_DANGER,
		"HumanSignalComeToMe"
	},
};


//=========================================================
// SignalCheckIn - get reports from all my troops to find
// if they are fighting anyone
//=========================================================

Task_t	tlHumanSignalCheckIn[] =
{
	{ TASK_STOP_MOVING,					0							},
	{ TASK_SET_ACTIVITY,				(float)ACT_IDLE				},
	{ TASK_HUMAN_SOUND_CHECK_IN,		(float)0					},
	{ TASK_WAIT,						(float)MAX_SQUAD_MEMBERS	},	// Wait for my squad to reply
};

Schedule_t	slHumanSignalCheckIn[] =
{
	{ 
		tlHumanSignalCheckIn,
		ARRAYSIZE ( tlHumanSignalCheckIn ), 
		bits_COND_HEAR_SOUND		|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_PROVOKED,

		bits_SOUND_DANGER,
		"HumanSignalCheckIn"
	},
};


//=========================================================
// SignalCheckIn - get reports from all my troops to find
// if they are fighting anyone
//=========================================================

Task_t	tlHumanCheckIn[] =
{
	{ TASK_STOP_MOVING,					0							},
	{ TASK_SET_ACTIVITY,				(float)ACT_IDLE				},
	{ TASK_HUMAN_SOUND_CLEAR,			(float)0					},
	{ TASK_WAIT,						(float)1					},
};

Schedule_t	slHumanCheckIn[] =
{
	{ 
		tlHumanCheckIn,
		ARRAYSIZE ( tlHumanCheckIn ), 
		bits_COND_HEAR_SOUND		|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_PROVOKED,

		bits_SOUND_DANGER,
		"HumanCheckIn"
	},
};


//=========================================================
// HumanFoundEnemy - human established sight with an enemy
// that was hiding from the squad.
//=========================================================

Task_t	tlHumanFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_HUMAN_SOUND_FOUND_ENEMY,	(float)0					},
};

Schedule_t	slHumanFoundEnemy[] =
{
	{ 
		tlHumanFoundEnemy,
		ARRAYSIZE ( tlHumanFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanFoundEnemy"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// human's grenade toss requires the enemy be occluded.
//=========================================================

Task_t	tlHumanRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_HUMAN_ESTABLISH_LINE_OF_FIRE }, // May fail if CheckFire fails, 
	{ TASK_FACE_ENEMY,			(float)0		},// i.e. I could hit a friendly
	{ TASK_HUMAN_CHECK_FIRE,	(float)0		},
	{ TASK_HUMAN_SOUND_CHARGE,	(float)0.1		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HUMAN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HUMAN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HUMAN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HUMAN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slHumanRangeAttack1[] =
{
	{ 
		tlHumanRangeAttack1,
		ARRAYSIZE ( tlHumanRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1"
	},
};


//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grenade toss requires the enemy be occluded.
//=========================================================

Task_t	tlHumanRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_HUMAN_FACE_TOSS_DIR,		(float)0					},
	{ TASK_HUMAN_SOUND_THROW,		(float)0.5					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HUMAN_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slHumanRangeAttack2[] =
{
	{ 
		tlHumanRangeAttack2,
		ARRAYSIZE ( tlHumanRangeAttack2 ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanRangeAttack2"
	},
};


//=========================================================
// Establish line of fire - move to a position that allows
// the human to attack.
//=========================================================

Task_t tlHumanEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_HUMAN_ELOF_FAIL	},
	{ TASK_STOP_MOVING,			(float)0						},
	{ TASK_FACE_ENEMY,			(float)0						},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_HUMAN_SOUND_CHARGE,	(float)0.7						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slHumanEstablishLineOfFire[] =
{
	{ 
		tlHumanEstablishLineOfFire,
		ARRAYSIZE ( tlHumanEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanEstablishLineOfFire"
	},
};


//=========================================================
// Explosive Establish line of fire - move to a position 
// that allows the human to attack with an explosive weapon 
// and not damage himself.
//=========================================================

Task_t tlHumanExplosiveELOF[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_HUMAN_MELEE_ELOF	},
	{ TASK_HUMAN_GET_EXPLOSIVE_PATH_TO_ENEMY,	(float)0		},
	{ TASK_HUMAN_SOUND_CHARGE,	(float)0.7						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slHumanExplosiveELOF[] =
{
	{ 
		tlHumanExplosiveELOF,
		ARRAYSIZE ( tlHumanExplosiveELOF ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanExplosiveELOF"
	},
};


//=========================================================
// Melee Establish line of fire - move to a position 
//	where I can kick the enemy
//=========================================================

Task_t tlHumanMeleeELOF[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TAKE_COVER_FROM_ENEMY	},
	{ TASK_HUMAN_GET_MELEE_PATH_TO_ENEMY,	(float)0			},
	{ TASK_HUMAN_SOUND_CHARGE,	(float)0.7						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slHumanMeleeELOF[] =
{
	{ 
		tlHumanMeleeELOF,
		ARRAYSIZE ( tlHumanMeleeELOF ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanMeleeELOF"
	},
};


//=========================================================
// Standoff schedule. Used in combat when a monster is 
// hiding in cover or the enemy has moved out of sight. 
// Should we look around in this schedule?
// Over-ridden because we want human to crouch and possibly taunt enemy
//=========================================================

Task_t	tlHumanStandoff[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_HUMAN_CROUCH,			(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_HUMAN_SOUND_TAUNT,		(float)0.2					},
	{ TASK_WAIT_FACE_ENEMY,			(float)3					},
};

Schedule_t slHumanStandoff[] = 
{
	{
		tlHumanStandoff,
		ARRAYSIZE ( tlHumanStandoff ),
		bits_COND_CAN_MELEE_ATTACK1		|
		bits_COND_CAN_MELEE_ATTACK2		|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2		|
		bits_COND_ENEMY_DEAD			|
		bits_COND_NEW_ENEMY				|
		bits_COND_LIGHT_DAMAGE			|
		bits_COND_HEAVY_DAMAGE			|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_PLAYER				|
		bits_SOUND_DANGER,
		"HumanStandoff"
	}
};


//=========================================================
// UnCrouch - get up and make a remark
//=========================================================

Task_t tlHumanUnCrouch[] =
{
	{ TASK_HUMAN_UNCROUCH,			(float)0					},
	{ TASK_SOUND_IDLE,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT,					(float)3					},
};

Schedule_t slHumanUnCrouch[] =
{
	{ 
		tlHumanUnCrouch,
		ARRAYSIZE ( tlHumanUnCrouch ),

		bits_COND_PROVOKED			|
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanUnCrouch"
	}
};


//=========================================================
// UnCrouch - get up and perform a scripted sequence
//=========================================================

Task_t tlHumanUnCrouchScript[] =
{
	{ TASK_HUMAN_UNCROUCH,			(float)0					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_AISCRIPT		},
};

Schedule_t slHumanUnCrouchScript[] =
{
	{ 
		tlHumanUnCrouchScript,
		ARRAYSIZE ( tlHumanUnCrouchScript ),
		0,
		0,
		"HumanUnCrouchScript"
	}
};


//=========================================================
// Human wait in cover - we don't allow danger or the ability
// to attack to break a human's run to cover schedule, but
// when a human is in cover, we do want them to attack if they can.
//=========================================================

Task_t	tlHumanWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slHumanWaitInCover[] =
{
	{ 
		tlHumanWaitInCover,
		ARRAYSIZE ( tlHumanWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"HumanWaitInCover"
	},
};


//=========================================================
// HumanFollow - Move to within 128 of target ent
//=========================================================

Task_t	tlHumanFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128					},	// Move within 128 of target ent
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE	},
};

Schedule_t	slHumanFollow[] =
{
	{
		tlHumanFollow,
		ARRAYSIZE ( tlHumanFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"HumanFollow"
	},
};


//=========================================================
// HumanFollowClose - Move to within 64 of target ent
//=========================================================

Task_t	tlHumanFollowClose[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)64					},	// Move within 64 of target ent
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE	},
};

Schedule_t	slHumanFollowClose[] =
{
	{
		tlHumanFollowClose,
		ARRAYSIZE ( tlHumanFollowClose ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"HumanFollowClose"
	},
};


//=========================================================
// HumanTurnRound - spin round very fast
//=========================================================

Task_t	tlHumanTurnRound[] =
{
	{ TASK_STOP_MOVING,			(float)0				},
	{ TASK_TURN_LEFT,			(float)179				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE_ANGRY	},
	{ TASK_WAIT,				(float)1				},
};

Schedule_t	slHumanTurnRound[] =
{
	{
		tlHumanTurnRound,
		ARRAYSIZE ( tlHumanTurnRound ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_PROVOKED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanTurnRound"
	},
};


//=========================================================
// Surpress - just fire like a mad bastard at the last 
// place you saw the enemy until your clip is all gone
//=========================================================

Task_t	tlHumanSuppress[] =
{
	{ TASK_STOP_MOVING,					(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_HUMAN_SOUND_SURPRESSING,		(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
	{ TASK_FACE_ENEMY,					(float)0			},
	{ TASK_HUMAN_CHECK_FIRE,			(float)0			},
	{ TASK_RANGE_ATTACK1,				(float)0			},
};

Schedule_t	slHumanSurpress[] =
{
	{ 
		tlHumanSuppress,
		ARRAYSIZE ( tlHumanSuppress ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_PROVOKED,

		bits_SOUND_DANGER,
		"HumanSuppress"
	},
};


//=========================================================
// Human - move to within range of the enemy's last known position
//=========================================================

Task_t tlHumanMoveToEnemyLKP[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_HUMAN_SURPRESS		},
	{ TASK_GET_PATH_TO_ENEMY_LKP,		(float)0						},
	{ TASK_RUN_PATH,					(float)0						},
	{ TASK_HUMAN_WAIT_GOAL_VISIBLE,		(float)0						},
	{ TASK_STOP_MOVING,					(float)0						},
};

Schedule_t slHumanMoveToEnemyLKP[] =
{
	{ 
		tlHumanMoveToEnemyLKP,
		ARRAYSIZE ( tlHumanMoveToEnemyLKP ),
		bits_COND_NEW_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanMoveToEnemyLKP"
	},
};


//=========================================================
// HumanSearchAndDestroy - Go on a wide loop in the hopes of
// locating an enemy
//=========================================================

Task_t tlHumanSearchAndDestroy[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ORIGIN,  (float)512		},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_HUMAN_SOUND_SEARCHING,	(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_IDLE_ANGRY	},
	{ TASK_TURN_RIGHT,				(float)179				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_IDLE_ANGRY	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slHumanSearchAndDestroy[] =
{
	{ 
		tlHumanSearchAndDestroy,
		ARRAYSIZE ( tlHumanSearchAndDestroy ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_CAN_ATTACK		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"HumanSearchAndDestroy"
	},
};


//=========================================================
// IdleStand - If standing idly, try and say something
//=========================================================

Task_t	tlHumanIdleStand[] =
{
	{ TASK_STOP_MOVING,		0				},
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE },
	{ TASK_SOUND_IDLE,		(float)0		},// Try to say something
	{ TASK_HUMAN_IDEALYAW,	(float)0		},
	{ TASK_FACE_IDEAL,		(float)0		}, 
	{ TASK_HUMAN_EYECONTACT,(float)0		},
	{ TASK_WAIT,			(float)10		},// wait a bit
};

Schedule_t	slHumanIdleStand[] =
{
	{ 
		tlHumanIdleStand,
		ARRAYSIZE ( tlHumanIdleStand ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_SEE_HATE			|
		bits_COND_SEE_DISLIKE		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_PUSHED			|
		bits_COND_PROVOKED			|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_WORLD	|
		bits_SOUND_COMBAT	|
		bits_SOUND_CARCASS	|
		bits_SOUND_MEAT		|
		bits_SOUND_GARBAGE	|
		bits_SOUND_DANGER	|
		bits_SOUND_PLAYER,
		
		"Human Idle Stand"
	},
};


//=========================================================
// IdleResponse - reply to someone else
//=========================================================

Task_t	tlHumanIdleResponse[] =
{
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},// Stop and listen
	{ TASK_WAIT,			(float)7.5		},// Wait until sure it's me they are talking to
	{ TASK_HUMAN_EYECONTACT,(float)0		},// Wait until speaker is done
	{ TASK_HUMAN_SOUND_RESPOND,	(float)0	},// Wait and then say my response
	{ TASK_HUMAN_IDEALYAW,	(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,		(float)0		}, 
	{ TASK_HUMAN_EYECONTACT,(float)0		},// Wait until speaker is done
};

Schedule_t	slHumanIdleResponse[] =
{
	{ 
		tlHumanIdleResponse,
		ARRAYSIZE ( tlHumanIdleResponse ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Human Idle Response"

	},
};


//=========================================================
// WaitHeal - Stand still while the doctor jabs me
//=========================================================

Task_t	tlHumanWaitHeal[] =
{
	{ TASK_STOP_MOVING,		(float)0		},
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},// Stop and listen
	{ TASK_WAIT,			(float)1		},// Wait while he stabs me with his fucking great needle
	{ TASK_HUMAN_EYECONTACT,(float)0		},
	{ TASK_HUMAN_SOUND_HEALED,(float)0		},// Wait and then say thanks doc
	{ TASK_HUMAN_IDEALYAW,	(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,		(float)0		}, 
	{ TASK_HUMAN_EYECONTACT,(float)0		},// Wait until speaker is done
};

Schedule_t	slHumanWaitHeal[] =
{
	{ 
		tlHumanWaitHeal,
		ARRAYSIZE ( tlHumanWaitHeal ), 
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Human Wait Heal"

	},
};


//=========================================================
// Follow the player or a squad member who I am defending
//=========================================================

Task_t	tlHumanFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slHumanFaceTarget[] =
{
	{
		tlHumanFaceTarget,
		ARRAYSIZE ( tlHumanFaceTarget ),
		bits_COND_PUSHED		|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"HumanFaceTarget"
	},
};


//=========================================================
// Fly through the air
//=========================================================

Task_t tlHumanExplosionDie[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_HUMAN_SOUND_EXPL,	(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_EXPLOSION_HIT	},
	{ TASK_HUMAN_EXPLOSION_FLY,	(float)0					},
	{ TASK_HUMAN_EXPLOSION_LAND,(float)0					},
};

Schedule_t slHumanExplosionDie[] =
{
	{
		tlHumanExplosionDie,
		ARRAYSIZE( tlHumanExplosionDie ),
		0,
		0,
		"HumanExplosionDie"
	},
};


//=========================================================
// Kick (over-rides base Primary Melee Attack to uncrouch)
//=========================================================

Task_t	tlHumanPrimaryMeleeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_HUMAN_UNCROUCH,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slHumanPrimaryMeleeAttack[] =
{
	{ 
		tlHumanPrimaryMeleeAttack1,
		ARRAYSIZE ( tlHumanPrimaryMeleeAttack1 ), 
		0,
		0,
		"Human Primary Melee Attack"
	},
};


//=========================================================
// Popup Attack - pop up from behind cover, fire off a few
// rounds, and then duck back down again before the bastards
// know what hit them
//=========================================================

Task_t	tlHumanPopupAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},// i.e. I could hit a friendly
	{ TASK_HUMAN_UNCROUCH,		(float)0		},
	{ TASK_HUMAN_CHECK_FIRE,	(float)0		},
	{ TASK_HUMAN_SOUND_CHARGE,	(float)0.3		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HUMAN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_HUMAN_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_HUMAN_CROUCH,		(float)0		},
	{ TASK_WAIT,				(float)0.5		},
};

Schedule_t	slHumanPopupAttack[] =
{
	{ 
		tlHumanPopupAttack,
		ARRAYSIZE ( tlHumanPopupAttack ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Human Popup Attack"
	},
};


//=========================================================
// repel 
//=========================================================

Task_t	tlHumanRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_SET_ACTIVITY,		(float)ACT_GLIDE 	},
	{ TASK_FACE_IDEAL,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slHumanRepel[] =
{
	{ 
		tlHumanRepel,
		ARRAYSIZE ( tlHumanRepel ), 
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

Task_t	tlHumanRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_SET_ACTIVITY,		(float)ACT_GLIDE	},
	{ TASK_FACE_ENEMY,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 		},
};

Schedule_t	slHumanRepelAttack[] =
{
	{ 
		tlHumanRepelAttack,
		ARRAYSIZE ( tlHumanRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlHumanRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
	{ TASK_STOP_MOVING,			(float)0					},
};

Schedule_t	slHumanRepelLand[] =
{
	{ 
		tlHumanRepelLand,
		ARRAYSIZE ( tlHumanRepelLand ), 
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


//=========================================================
// repel land search and destroy
//=========================================================
Task_t	tlHumanRepelLandSearch[] =
{
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_IDLE_ANGRY		},	// Do a sweep of the area, giving my squad time to land
	{ TASK_TURN_RIGHT,			(float)179					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_IDLE_ANGRY		},
	{ TASK_TURN_RIGHT,			(float)179					},
	{ TASK_SET_SCHEDULE,		(float)SCHED_HUMAN_SIGNAL_SEARCH_AND_DESTROY },	// move out
};

Schedule_t	slHumanRepelLandSearch[] =
{
	{ 
		tlHumanRepelLandSearch,
		ARRAYSIZE ( tlHumanRepelLandSearch ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_PLAYER, 
		"Repel Land Search and Destroy"
	},
};


//=========================================================
// Fail - over-rides default so human faces his enemy when failed
//=========================================================

Task_t	tlHumanFail[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
	{ TASK_FACE_IDEAL,			(float)0					},
	{ TASK_WAIT,				(float)2					},
	{ TASK_WAIT_PVS,			(float)0					},
};

Schedule_t	slHumanFail[] =
{
	{
		tlHumanFail,
		ARRAYSIZE ( tlHumanFail ),
		bits_COND_CAN_ATTACK,
		0,
		"HumanFail"
	},
};


DEFINE_CUSTOM_SCHEDULES( CHuman )
{
	slHumanTakeCover,
	slHumanGrenadeCover,
	slHumanTossGrenadeCover,
	slHumanTakeCoverFromBestSound,
	slHumanCower,
	slHumanHearSound,
	slHumanInvestigateSound,
	slHumanVictoryDance,
	slHumanHideReload,
	slHumanReload,
	slHumanFindMedic,
	slHumanFindMedicCombat,
	slHumanSignalAttack,
	slHumanSignalSurpress,
	slHumanSignalSearchAndDestroy,
	slHumanSignalRetreat,
	slHumanSignalComeToMe,
	slHumanSignalCheckIn,
	slHumanCheckIn,
	slHumanFoundEnemy,
	slHumanRangeAttack1,
	slHumanRangeAttack2,
	slHumanEstablishLineOfFire,
	slHumanExplosiveELOF,
	slHumanMeleeELOF,
	slHumanStandoff,
	slHumanUnCrouch,
	slHumanUnCrouchScript,
	slHumanWaitInCover,
	slHumanFollow,
	slHumanFollowClose,
	slHumanTurnRound,
	slHumanSurpress,
	slHumanMoveToEnemyLKP,
	slHumanSearchAndDestroy,
	slHumanIdleStand,
	slHumanIdleResponse,
	slHumanFaceTarget,
	slHumanExplosionDie,
	slHumanPrimaryMeleeAttack,
	slHumanWaitHeal,
	slHumanPopupAttack,
	slHumanRepel,
	slHumanRepelAttack,
	slHumanRepelLand,
	slHumanRepelLandSearch,
	slHumanFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHuman, CSquadMonster );



//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CHuman :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_HUMAN_COVER_AND_RELOAD:
		{
			return &slHumanHideReload[ 0 ];
		}
		break;

	case SCHED_HUMAN_RELOAD:
		{
			return &slHumanReload[ 0 ];
		}
		break;

	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slHumanTakeCoverFromBestSound[ 0 ];
		}
		break;

	case SCHED_HUMAN_TAKECOVER_FAILED:
		{
			Remember( bits_MEMORY_HUMAN_NO_COVER );
			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;

	case SCHED_COWER:
		{
			return &slHumanCower[ 0 ];
		}
		break;

	case SCHED_HUMAN_HEAR_SOUND:
		{
			return &slHumanHearSound[ 0 ];
		}
		break;

	case SCHED_INVESTIGATE_SOUND:
		{
			return &slHumanInvestigateSound[ 0 ];
		}
		break;

	case SCHED_VICTORY_DANCE:
		{
			return &slHumanVictoryDance[ 0 ];
		}
		break;

	case SCHED_HUMAN_FIND_MEDIC:
		{
			return &slHumanFindMedic[ 0 ];
		}
		break;

	case SCHED_HUMAN_FIND_MEDIC_COMBAT:
		{
			return &slHumanFindMedicCombat[ 0 ];
		}
		break;

	case SCHED_HUMAN_SIGNAL_ATTACK:
		{
			return &slHumanSignalAttack[ 0 ];
		}
		break;

	case SCHED_HUMAN_SIGNAL_SURPRESS:
		{
			return &slHumanSignalSurpress[ 0 ];
		}
		break;

	case SCHED_HUMAN_SIGNAL_SEARCH_AND_DESTROY:
		{
			return &slHumanSignalSearchAndDestroy[ 0 ];
		}
		break;

	case SCHED_HUMAN_SIGNAL_RETREAT:
		{
			return &slHumanSignalRetreat[ 0 ];
		}
		break;

	case SCHED_HUMAN_SIGNAL_COME_TO_ME:
		{
			return &slHumanSignalComeToMe[ 0 ];
		}
		break;

	case SCHED_HUMAN_SIGNAL_CHECK_IN:
		{
			return &slHumanSignalCheckIn[ 0 ];
		}
		break;

	case SCHED_HUMAN_CHECK_IN:
		{
			return &slHumanCheckIn[ 0 ];
		}
		break;

	case SCHED_HUMAN_FOUND_ENEMY:
		{
			return &slHumanFoundEnemy[ 0 ];
		}
		break;

	case SCHED_RANGE_ATTACK1:
		{
			if ( m_fStopCrouching )
			{
				return GetScheduleOfType( SCHED_HUMAN_POPUP_ATTACK );
			}
			else
			{
				return &slHumanRangeAttack1[ 0 ];
			}
		}
		break;

	case SCHED_RANGE_ATTACK2:
		{
			return &slHumanRangeAttack2[ 0 ];
		}
		break;

	case SCHED_HUMAN_ESTABLISH_LINE_OF_FIRE:
		{
			switch ( m_iWeapon )
			{
			case HUMAN_WEAPON_M79:
			case HUMAN_WEAPON_RPG7:
				return &slHumanExplosiveELOF[ 0 ];
				break;

			default:
				return &slHumanEstablishLineOfFire[ 0 ];
				break;
			}
		}
		break;

	case SCHED_HUMAN_MELEE_ELOF:
		{
			return &slHumanMeleeELOF[ 0 ];
		}
		break;

	case SCHED_HUMAN_ELOF_FAIL:
		{
			// human is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_STANDOFF );
		}
		break;

	case SCHED_STANDOFF:
		{
			return &slHumanStandoff[ 0 ];
		}
		break;

	case SCHED_HUMAN_UNCROUCH:
		{
			return &slHumanUnCrouch[ 0 ];
		}
		break;

	case SCHED_HUMAN_UNCROUCH_SCRIPT:
		{
			return &slHumanUnCrouchScript[ 0 ];
		}
		break;

	case SCHED_HUMAN_WAIT_FACE_ENEMY:
		{
			return &slHumanWaitInCover[ 0 ];
		}
		break;

	case SCHED_TARGET_CHASE:
	case SCHED_HUMAN_FOLLOW:
		{
			return &slHumanFollow[ 0 ];
		}
		break;

	case SCHED_HUMAN_FOLLOW_CLOSE:
		{
			return &slHumanFollowClose[ 0 ];
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( m_fHandGrenades )
			{
				if ( RANDOM_LONG(0,1) && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && 
					!HasConditions( bits_COND_HEAVY_DAMAGE | bits_COND_LIGHT_DAMAGE ) && 
					OccupySlot( bits_SLOTS_HUMAN_GRENADE ) )
				{
					return &slHumanTossGrenadeCover[ 0 ];
				}

				if ( RANDOM_LONG(0,1) && !( InSquad() && SquadMemberInRange( pev->origin, 256 ) ) )
				{
					return &slHumanGrenadeCover[ 0 ];
				}
			}
			else
			{
				return &slHumanTakeCover[ 0 ];
			}
		}
		break;

	case SCHED_HUMAN_RETREAT:
		{
			return &slHumanRetreat[ 0 ];
		}
		break;

	case SCHED_HUMAN_TAKE_COVER_FROM_ENEMY_NO_GRENADE:
		{
			return &slHumanTakeCover[ 0 ];
		}
		break;

	case SCHED_HUMAN_TURN_ROUND:
		{
			return &slHumanTurnRound[ 0 ];
		}
		break;

	case SCHED_HUMAN_SURPRESS:
		{
			if ( m_fStopCrouching )
			{
				return GetScheduleOfType( SCHED_HUMAN_POPUP_ATTACK );
			}
			return &slHumanSurpress[ 0 ];
		}
		break;

	case SCHED_HUMAN_MOVE_TO_ENEMY_LKP:
		{
			return &slHumanMoveToEnemyLKP[ 0 ];
		}
		break;

	case SCHED_HUMAN_SEARCH_AND_DESTROY:
		{
			return &slHumanSearchAndDestroy[ 0 ];
		}
		break;

	case SCHED_IDLE_STAND:
		{
			return &slHumanIdleStand[ 0 ];
		}
		break;

	case SCHED_HUMAN_IDLE_RESPONSE:
		{
			return &slHumanIdleResponse[ 0 ];
		}
		break;

	case SCHED_TARGET_FACE:
		{
			return &slHumanFaceTarget[ 0 ];
		}
		break;

	case SCHED_HUMAN_EXPLOSION_DIE:
		{
			return &slHumanExplosionDie[ 0 ];
		}
		break;

	case SCHED_MELEE_ATTACK1:
		{
			return &slHumanPrimaryMeleeAttack[ 0 ];
		}
		break;

	case SCHED_HUMAN_WAIT_HEAL:
		{
			return &slHumanWaitHeal[ 0 ];
		}
		break;

	case SCHED_HUMAN_POPUP_ATTACK:
		{
			return &slHumanPopupAttack[ 0 ];
		}
		break;

	case SCHED_HUMAN_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slHumanRepel[ 0 ];
		}
		break;

	case SCHED_HUMAN_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slHumanRepelAttack[ 0 ];
		}
		break;

	case SCHED_HUMAN_REPEL_LAND:
		{
			m_flCrouchTime = gpGlobals->time + CROUCH_TIME;
			pev->view_ofs = Vector( 0, 0, 36 );
			m_fCrouching = TRUE;
			return &slHumanRepelLand[ 0 ];
		}
		break;

	case SCHED_HUMAN_REPEL_LAND_SEARCH_AND_DESTROY:
		{
			m_flCrouchTime = gpGlobals->time + CROUCH_TIME;
			pev->view_ofs = Vector( 0, 0, 36 );
			m_fCrouching = TRUE;
			return &slHumanRepelLandSearch[ 0 ];
		}
		break;

	case SCHED_FAIL:
		{
			return &slHumanFail[ 0 ];
		}
		break;

	case SCHED_AISCRIPT:
		{
			if ( m_fCrouching )
			{
				return GetScheduleOfType( SCHED_HUMAN_UNCROUCH_SCRIPT );
			}
			else
			{
				return CSquadMonster::GetScheduleOfType( Type );
			}
		}
		break;
	}

	return CSquadMonster::GetScheduleOfType( Type );
}



