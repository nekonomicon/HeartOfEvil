#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "rain.h"

#define SF_RAIN_START_OFF		0x0001

//==============================================================
// Rain entity
//==============================================================

TYPEDESCRIPTION	CRain::m_SaveData[] = 
{
	DEFINE_FIELD( CRain, m_fFreq, FIELD_FLOAT ),
	DEFINE_FIELD( CRain, m_iMaxDist, FIELD_INTEGER ),
	DEFINE_FIELD( CRain, m_iRadius, FIELD_INTEGER ),
	DEFINE_FIELD( CRain, m_bOn, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CRain, CBaseEntity );


void CRain :: Precache( void )
{
	m_iRainSprite = PRECACHE_MODEL("models/raindrop.mdl");
}

void CRain :: Spawn( void )
{
	Precache();

	SetThink(RainThink);
	
	m_bOn = !(pev->spawnflags & SF_RAIN_START_OFF);
	
	if ( m_bOn )
	{
		pev->nextthink = gpGlobals->time + 1;
	}
	else 
	{
		pev->nextthink = 0.0;
	}
}


void CRain :: RainThink( void )
{
	edict_t *pPlayer;

	// Get edict for one player
	pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );


	if ( ( pPlayer->v.origin - pev->origin ).Length2D() < m_iMaxDist )
		CreateRainParticle();

	pev->nextthink = gpGlobals->time + max(0.0, m_fFreq - RANDOM_FLOAT(-0.2, 0.2));
}


void CRain :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	switch (useType)
	{
	case USE_TOGGLE: m_bOn = !m_bOn; break;
	case USE_SET: m_bOn = !m_bOn; break;
	case USE_ON: m_bOn = TRUE; break;
	case USE_OFF: m_bOn = FALSE; break;
	}
	
	if (m_bOn)
	{
		pev->nextthink = gpGlobals->time + 1;
	}
	else
	{
		pev->nextthink = 0.0;		
	}
}


void CRain :: CreateRainParticle( void )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_PROJECTILE );		

		// Start Position
		WRITE_COORD( pev->origin.x + RANDOM_FLOAT(-m_iRadius, m_iRadius) );
		WRITE_COORD( pev->origin.y + RANDOM_FLOAT(-m_iRadius, m_iRadius) );
		WRITE_COORD( pev->origin.z );

		// Velocity
		WRITE_COORD( 0.0 );
		WRITE_COORD( 0.0 );
		WRITE_COORD( -1000.0 );

		// Sprite Index
		WRITE_SHORT( m_iRainSprite );

		// Life
		WRITE_BYTE( 2 );

		// Owner
		WRITE_BYTE( 0 );

	MESSAGE_END();
}



void CRain::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq(pkvd->szKeyName, "frequency") )
	{
		m_fFreq = 1 / atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "distance") )
	{
		m_iMaxDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "radius") )
	{
		m_iRadius = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseEntity::KeyValue( pkvd );

}

LINK_ENTITY_TO_CLASS( env_rain, CRain );


//==============================================================
// EnvRain
//==============================================================


TYPEDESCRIPTION	CEnvRain::m_SaveData[] = 
{
	DEFINE_FIELD( CEnvRain, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvRain, m_burstSize, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvRain, m_flUpdateTime, FIELD_FLOAT ),
	DEFINE_FIELD( CRain, m_iMaxDist, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEnvRain, CBaseEntity );

void CEnvRain::Precache( void )
{
	m_spriteTexture = PRECACHE_MODEL("models/raindrop.mdl");
}

void CEnvRain::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_burstSize"))
	{
		m_burstSize = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flUpdateTime"))
	{
		m_flUpdateTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "distance") )
	{
		m_iMaxDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvRain::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!ShouldToggle(useType, m_iState)) return;

	if (m_iState == STATE_ON)
	{
		m_iState = STATE_OFF;
		pev->nextthink = -1;
	}
	else
	{
		m_iState = STATE_ON;
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CEnvRain::Spawn( void )
{
	Precache();
	SET_MODEL( ENT(pev), STRING(pev->model) );		// Set size
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if (m_burstSize == 0) // in case the level designer forgot to set it.
		m_burstSize = 2;

	if (pev->spawnflags & SF_RAIN_START_OFF)
		m_iState = STATE_OFF;
	else
	{
		m_iState = STATE_ON;
		pev->nextthink = gpGlobals->time + 1;
	}
}


#define MAX(a,b)        (((a) < (b)) ? (b) : (a))
#define MIN(a,b)        (((a) > (b)) ? (b) : (a))

void CEnvRain::Think( void )
{
	edict_t *pPlayer;

	// Get edict for one player
	pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	
	Vector vecSrc;

	float	MinX = pev->mins.x, MaxX = pev->maxs.x, 
			MinY = pev->mins.y, MaxY = pev->maxs.y;

	if ( MaxX > pPlayer->v.origin.x + (float) m_iMaxDist ) MaxX = MAX( MinX, pPlayer->v.origin.x + (float) m_iMaxDist );
	if ( MinX < pPlayer->v.origin.x - (float) m_iMaxDist ) MinX = MIN( MaxX, pPlayer->v.origin.x - (float) m_iMaxDist );
	if ( MaxY > pPlayer->v.origin.y + (float) m_iMaxDist ) MaxY = MAX( MinY, pPlayer->v.origin.y + (float) m_iMaxDist );
	if ( MinY < pPlayer->v.origin.y - (float) m_iMaxDist ) MinY = MIN( MaxY, pPlayer->v.origin.y - (float) m_iMaxDist );

	UTIL_MakeVectors(pev->angles);

	int repeats;
	repeats = (float) m_burstSize * (float) ((MaxX - MinX) * (MaxY - MinY)) / (256.0f * 256.0f);

	for (int i = 0; i < repeats; i++)
	{
		vecSrc.x = RANDOM_LONG(MinX, MaxX);
		vecSrc.y = RANDOM_LONG(MinY, MaxY);
		vecSrc.z = pev->maxs.z;

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_PROJECTILE );		

			// Start Position
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );

			// Velocity
			WRITE_COORD( 0.0 );
			WRITE_COORD( 0.0 );
			WRITE_COORD( -1000.0 );

			// Sprite Index
			WRITE_SHORT( m_spriteTexture );

			// Life
			WRITE_BYTE( 2 );

			// Owner
			WRITE_BYTE( 0 );

		MESSAGE_END();

	}

	if (m_flUpdateTime)
		pev->nextthink = gpGlobals->time + m_flUpdateTime;

}

LINK_ENTITY_TO_CLASS( env_rainarea, CEnvRain );


//==============================================================
// Fog
//==============================================================


BOOL CEnvFog::m_nFogOn = FOG_UNKNOWN;

void CEnvFog::SetFog()
{
	UTIL_WriteHoEConfig();
}

//==============================================
// GetFog - reads config.cfg and returns the value
// of m_nFogOn or FALSE if it was not found
//==============================================

BOOL CEnvFog::GetFog()
{
	int val = UTIL_SearchHoEConfig( "fog", FOG_OFF );

	if ( val != FOG_UNKNOWN && val != FOG_OFF && val != FOG_ON )	// unhandled value - use OFF as default
	{
		val = FOG_OFF;
	}

	return val;
}


TYPEDESCRIPTION	CEnvFog::m_SaveData[] = 
{
	DEFINE_FIELD( CEnvFog, m_iStartDist, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFog, m_iEndDist, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFog, m_iFadeIn, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_iFadeOut, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_fHoldTime, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_fActiveTime, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFog, m_fFadeStart, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CEnvFog, CBaseEntity );

void CEnvFog :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "startdist"))
	{
		m_iStartDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "enddist"))
	{
		m_iEndDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_iFadeIn = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_iFadeOut = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		m_fHoldTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

STATE CEnvFog::GetState( void )
{
	if (pev->spawnflags & SF_FOG_ACTIVE)
	{
		if (pev->spawnflags & SF_FOG_FADING)
			return STATE_TURN_ON;
		else
			return STATE_ON;
	}
	else
	{
		if (pev->spawnflags & SF_FOG_FADING)
			return STATE_TURN_OFF;
		else
			return STATE_OFF;
	}
}

void CEnvFog :: Spawn ( void )
{
	if ( CEnvFog::m_nFogOn == FOG_UNKNOWN )
	{
		CEnvFog::m_nFogOn = CEnvFog::GetFog();	
	}

	pev->effects |= EF_NODRAW;

	if (pev->targetname == 0)
		pev->spawnflags |= SF_FOG_ACTIVE;

	if (pev->spawnflags & SF_FOG_ACTIVE)
	{
		SetNextThink( 1.5 );
		SetThink( TurnOn );
	}

// Precache is now used only to continue after a game has loaded.
//	Precache();
}

void CEnvFog :: Precache ( void )
{
	if ( CEnvFog::m_nFogOn == FOG_UNKNOWN )
	{
		CEnvFog::m_nFogOn = CEnvFog::GetFog();	
	}

	// lazy approach: if a game is loaded, and a fade is in progress,
	// just skip to the end of the fade.
	if (pev->spawnflags & SF_FOG_FADING)
	{
		DontThink();
		Think();
	}
	else if (pev->spawnflags & SF_FOG_ACTIVE)
	{
		SetNextThink( 0 );
		SetThink( Active );
	}
}


void CEnvFog :: Active( void )
{
	SetNextThink( 1 );
	m_fActiveTime += 1;

	if ( m_fHoldTime && m_fActiveTime > m_fHoldTime )	// Turn off at end of hold time
	{
		SetThink( TurnOff );
		Think();
	}
	else	// Else make sure it's still on
	{
		SendData( pev->rendercolor, 0, m_iStartDist, m_iEndDist);
	}
}

void CEnvFog :: TurnOn ( void )
{
//	ALERT(at_console, "Fog turnon\n");

	pev->spawnflags |= SF_FOG_ACTIVE;

	if( m_iFadeIn )
	{
		pev->spawnflags |= SF_FOG_FADING;

		SendData( pev->rendercolor, m_iFadeIn, m_iStartDist, m_iEndDist);

		SetNextThink( m_iFadeIn );
		SetThink( FadeInDone );
	}
	else
	{
		pev->spawnflags &= ~SF_FOG_FADING;

		SendData( pev->rendercolor, 0, m_iStartDist, m_iEndDist);

		if (m_fHoldTime)
		{
			SetNextThink( m_fHoldTime );
			SetThink( TurnOff );
		}
	}
}

void CEnvFog :: TurnOff ( void )
{
//	ALERT(at_console, "Fog turnoff\n");

	pev->spawnflags &= ~SF_FOG_ACTIVE;

	if( m_iFadeOut )
	{
		pev->spawnflags |= SF_FOG_FADING;

		SendData( pev->rendercolor, -m_iFadeOut, m_iStartDist, m_iEndDist);

		SetNextThink( m_iFadeOut );
		SetThink( FadeOutDone );
	}
	else
	{
		pev->spawnflags &= ~SF_FOG_FADING;

		SendData( g_vecZero, 0, 0, 0 );

		DontThink();
	}
}

void CEnvFog :: FadeInDone ( void )
{
	pev->spawnflags &= ~SF_FOG_FADING;
	SendData( pev->rendercolor, 0, m_iStartDist, m_iEndDist);

	m_fActiveTime = 0;

	SetThink( Active );
	SetNextThink( 1 );
}

void CEnvFog :: FadeOutDone ( void )
{
	pev->spawnflags &= ~SF_FOG_FADING;

	SendData( g_vecZero, 0, 0, 0);
}

void CEnvFog :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
//	ALERT(at_console, "Fog use\n");
	if (ShouldToggle(useType))
	{
		if (pev->spawnflags & SF_FOG_ACTIVE)
			TurnOff();
		else
			TurnOn();
	}
}

void CEnvFog :: SendData ( Vector col, int iFadeTime, int iStartDist, int iEndDist )
{
	//ALERT(at_console, "Fog send (%d %d %d), %d - %d\n", col.x, col.y, col.z, iStartDist, iEndDist);

	if ( CEnvFog::m_nFogOn == FOG_UNKNOWN )
	{
		CEnvFog::m_nFogOn = CEnvFog::GetFog();
	}
	UTIL_Fog( CEnvFog::m_nFogOn );
	UTIL_SetFog( col, iFadeTime, iStartDist, iEndDist);
}


//LRC
void CEnvFog::DontThink( void )
{
	m_fNextThink = 0;
	pev->nextthink = 0;

//	ALERT(at_console, "DontThink for %s\n", STRING(pev->targetname));
}


//LRC - for getting round the engine's preconceptions.
// MoveWith entities have to be able to think independantly of moving.
// This is how we do so.
void CEnvFog :: SetNextThink( float delay, BOOL correctSpeed )
{
	pev->nextthink = gpGlobals->time + delay;
}


BOOL CEnvFog::ShouldToggle( USE_TYPE useType )
{
	STATE currentState = GetState();
	if ( useType != USE_TOGGLE && useType != USE_SET )
	{
		switch(currentState)
		{
		case STATE_ON:
		case STATE_TURN_ON:
			if (useType == USE_ON) return FALSE;
			break;
		case STATE_OFF:
		case STATE_TURN_OFF:
			if (useType == USE_OFF) return FALSE;
			break;
		}
	}
	return TRUE;
}


LINK_ENTITY_TO_CLASS( env_fog, CEnvFog );

