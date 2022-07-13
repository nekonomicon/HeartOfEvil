/*===============================================
         -= http://www.poke646.com =-
The same old trouble in a brand new environment

MP3 player sourcefile

TheTinySteini
===============================================*/

#include "hud.h"
#include "cl_util.h"
#include "mp3.h"

int CMP3::Initialize()
{
	char fmodlib[256];
	
	m_iIsPlaying = 0;
	sprintf( fmodlib, "%s/fmod.dll", gEngfuncs.pfnGetGameDirectory());
	// replace forward slashes with backslashes
	for( int i=0; i < 256; i++ )
		if( fmodlib[i] == '/' ) fmodlib[i] = '\\';
	
	m_hFMod = LoadLibrary( fmodlib );

	if( m_hFMod != NULL )
	{
		// fill in the function pointers
		(FARPROC&)SCL = GetProcAddress(m_hFMod, "_FSOUND_Stream_Close@4");
		(FARPROC&)SOP = GetProcAddress(m_hFMod, "_FSOUND_SetOutput@4");
		(FARPROC&)SBS = GetProcAddress(m_hFMod, "_FSOUND_SetBufferSize@4");
		(FARPROC&)SDRV = GetProcAddress(m_hFMod, "_FSOUND_SetDriver@4");
		(FARPROC&)INIT = GetProcAddress(m_hFMod, "_FSOUND_Init@12");
		(FARPROC&)SOF = GetProcAddress(m_hFMod, "_FSOUND_Stream_OpenFile@12");
		(FARPROC&)SPLAY = GetProcAddress(m_hFMod, "_FSOUND_Stream_Play@8");
		(FARPROC&)CLOSE = GetProcAddress(m_hFMod, "_FSOUND_Close@0");
		(FARPROC&)SETTIME = GetProcAddress(m_hFMod, "_FSOUND_Stream_SetTime@8");
		(FARPROC&)GETTIME = GetProcAddress(m_hFMod, "_FSOUND_Stream_GetTime@4");
		(FARPROC&)GETLENGTH = GetProcAddress(m_hFMod, "_FSOUND_Stream_GetLengthMs@4");
		
		if( !(SCL && SOP && SBS && SDRV && INIT && SOF && SPLAY && CLOSE && SETTIME) )
		{
			FreeLibrary( m_hFMod );
			gEngfuncs.Con_Printf("Fatal Error: FMOD functions couldn't be loaded!\n");
			return 0;
		}
	} else
	{
		gEngfuncs.Con_Printf("Fatal Error: FMOD library couldn't be loaded!\n");
		return 0;
	}

	return 1;
}


int CMP3::Shutdown()
{
	if( m_hFMod )
	{
		CLOSE();

		FreeLibrary( m_hFMod );
		m_hFMod = NULL;
		m_iIsPlaying = 0;
		return 1;
	} else
		return 0;
}


int CMP3::StopMP3( void )
{
	SCL( m_Stream );
	m_iIsPlaying = 0;
	return 1;
}


int CMP3::PlayMP3( const char *pszSong, BOOL Loop, long ms )
{
	if( m_iIsPlaying )
	{	
		if ( ms > 0 && abs( GETTIME( m_Stream ) - ms ) <= 1000 && strcmp( m_szFileName, pszSong ) == 0 )	
			// Already playing at this spot - no need to do anything
		{
			return 1;
		}
		
		// sound system is already initialized
		SCL( m_Stream );
	} else
	{
		SOP( FSOUND_OUTPUT_DSOUND );
		SBS( 200 );
		SDRV( 0 );
		INIT( 44100, 1, 0 ); // we need just one channel, multiple mp3s at a time would be, erm, strange...
	}

	char song[256];
	sprintf( song, "%s/sound/mp3/%s", gEngfuncs.pfnGetGameDirectory(), pszSong);

	if (Loop)
	{
		m_Stream = SOF( song, FSOUND_NORMAL | FSOUND_LOOP_NORMAL, 1 );
	}
	else 
	{
		m_Stream = SOF( song, FSOUND_NORMAL, 1 );
	}
	
	if( m_Stream )	// Play
	{
		int length = GETLENGTH( m_Stream );		// total length of song

		if ( Loop && ms >= length )	// If looped and we are past the end of the loop go back to the beginning
		{
			ms -= (ms / length) * length;
		}

		if ( ms >= 0 && ms < length ) // Make sure we are not starting after the song is finished
		{
			SPLAY( 0, m_Stream );
			SETTIME( m_Stream, ms );

			m_iIsPlaying = 1;
			strcpy( m_szFileName, pszSong );	// Store the name of the song we are playing

			return 1;
		}
		else
		{
			m_iIsPlaying = 0;
			return 0;
		}
	} else
	{
		gEngfuncs.Con_Printf( "ERROR: Couldn't open file %s\n", song );
		m_iIsPlaying = 0;
		return 0;
	}
}


