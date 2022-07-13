/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//#ifndef OEM_BUILD

#ifndef _HUEY_H
#define _HUEY_H

extern DLL_GLOBAL int		g_iSkillLevel;

#define SF_WAITFORTRIGGER	0x0004 // UNDONE: Fix!
#define SF_GRENADE			0x0008
#define SF_ROCKET			0x0032
#define SF_PASSENGER		0x0064

#define BODY_HUEY			0
#define BODY_PASSENGER		1
#define BODY_GRENADE		2

#define HUEY_PBODY_NONE		0
#define HUEY_PBODY_GRUNT	1
#define HUEY_PBODY_MIKE		2

#define HUEY_PASSENGER_GRUNT	0
#define HUEY_PASSENGER_GRUNT_FR	1
#define HUEY_PASSENGER_MIKE		2

#define HUEY_GUN_RANGE 2048
#define HUEY_ROCKET_RANGE 4096

#define MAX_CARRY 11


class CHuey : public CBaseMonster
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int  Classify( void ) { if (m_iPassenger > HUEY_PASSENGER_GRUNT) return CLASS_PLAYER_ALLY; else return CLASS_HELICOPTER; };
	int  BloodColor( void ) { return DONT_BLEED; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );
	void ApplyDefaultSettings( void );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -300, -300, -172);
		pev->absmax = pev->origin + Vector(300, 300, 8);
	}

	void EXPORT FindAllThink( void );
	void EXPORT DeployThink( void );
	void EXPORT HuntThink( void );
	void EXPORT FlyTouch( CBaseEntity *pOther );
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void EXPORT DyingThink( void );
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT NullThink( void );
	void EXPORT HoverThink( void );

	void MakeSound( void );
	void ShowDamage( void );
	void Flight( void );
	void FireRocket( void );
	BOOL FireGun( BOOL OnlyGrunt );
	BOOL HasDead( void );
	CBaseMonster *MakeGrunt( Vector vecSrc, Vector vecAngles );
	
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	
	Vector GetVectorFromAngles( Vector Angles );

	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;
	float m_flNextGrenade;

	int	m_iUnits;
	EHANDLE m_hGrunt[MAX_CARRY];
	Vector m_vecOrigin[MAX_CARRY];
	EHANDLE m_hRepel[4];
	
	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	Vector m_angGun;
	float m_angMiniGuns;
	float m_angGren;
	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iSoundState; // don't save this

	int m_iSpriteTexture;
	int m_iRopeTexture;
	int m_iExplode;
	int m_iBodyGibs;

	float m_flNextSoundTime;

	float m_flGoalSpeed;

	int m_iMaxGrunts;
	int m_iPassenger;
	float m_flPassengerHealth;

	int m_iDoSmokePuff;
	CBeam *m_pBeam;

	void KeyValue(KeyValueData *pkvd);
};

#endif // _HUEY_H


