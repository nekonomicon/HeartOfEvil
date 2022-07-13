#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "mp3.h"

TYPEDESCRIPTION	CAmbientMP3::m_SaveData[] = 
{
	DEFINE_FIELD( CAmbientMP3, m_flPlayTime, FIELD_TIME ),
	DEFINE_FIELD( CAmbientMP3, m_flDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CAmbientMP3, m_fIsPlaying, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CAmbientMP3, CBaseEntity );


BOOL CAmbientMP3::m_nMp3On = MP3_UNKNOWN;

void CAmbientMP3::SetMP3()
{
	UTIL_WriteHoEConfig();
}

//==============================================
// GetMP3 - reads hoeconfig.cfg and returns the value
// of m_nMP3On or TRUE if it was not found
//==============================================

BOOL CAmbientMP3::GetMP3()
{
	int val = UTIL_SearchHoEConfig( "mp3", MP3_OFF );

	if ( val != MP3_UNKNOWN && val != MP3_OFF && val != MP3_ON )	// unhandled value - use ON as default
	{
		val = MP3_ON;
	}

	return val;
}


void CAmbientMP3::Spawn( void )
{
	if ( CAmbientMP3::m_nMp3On == MP3_UNKNOWN )
	{
		CAmbientMP3::m_nMp3On = CAmbientMP3::GetMP3();	
	}

	SetUse ( ToggleUse );

	m_flPlayTime = 0.0;

	if ( FBitSet( pev->spawnflags, SF_AMBIENT_MP3_START_ON ) )
	{
		m_fIsPlaying = TRUE;
		m_flPlayTime = 0.1;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		pev->nextthink = 0.0;
		m_fIsPlaying = FALSE;	
	}
}


void CAmbientMP3::Precache( void )
{
	if ( m_fIsPlaying )
	{
		pev->nextthink = gpGlobals->time + 1;
		m_flPlayTime += 1;
	}
}


// Tells the Client to play it each second BUGBUG - It will keep doing this after the mp3 is finished

void CAmbientMP3::Think( void )
{
	if ( m_fIsPlaying && m_flPlayTime <= m_flDuration )
	{
		UTIL_PlayMP3( STRING(pev->message), FBitSet ( pev->spawnflags, SF_AMBIENT_MP3_LOOPED ), m_flPlayTime  );

		pev->nextthink = gpGlobals->time + 1;
		m_flPlayTime += 1;
	}
}


void CAmbientMP3 :: ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType == USE_OFF )
	{
		pev->nextthink = 0.0;
		m_fIsPlaying = FALSE;

		edict_t *pClient;
	
		// manually find the single player. 
		pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	
		// Can't play if the client is not connected!
		if ( !pClient )
			return;

		CLIENT_COMMAND ( pClient, "stopmp3\n");
	}
	else if ( UTIL_PlayMP3( STRING(pev->message), FBitSet ( pev->spawnflags, SF_AMBIENT_MP3_LOOPED ), 0 ) )
	{
		m_fIsPlaying = TRUE;
		m_flPlayTime = 1;
		pev->nextthink = gpGlobals->time + 1;
	}
}



void CAmbientMP3::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq(pkvd->szKeyName, "duration") )
	{
		m_flDuration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseEntity::KeyValue( pkvd );
}


LINK_ENTITY_TO_CLASS( ambient_mp3, CAmbientMP3 );
