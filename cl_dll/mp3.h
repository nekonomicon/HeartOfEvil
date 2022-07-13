/*===============================================
         -= http://www.poke646.com =-
The same old trouble in a brand new environment

MP3 player headerfile

TheTinySteini
===============================================*/

#ifndef MP3_H
#define MP3_H

#include "fmod.h"
#include "fmod_errors.h"
#include "windows.h"

class CMP3
{
private:
	signed char		(_stdcall * SCL)	(FSOUND_STREAM *stream);
	signed char		(_stdcall * SOP)	(int outputtype);
	signed char		(_stdcall * SBS)	(int len_ms);
	signed char		(_stdcall * SDRV)	(int driver);
	signed char		(_stdcall * INIT)	(int mixrate, int maxsoftwarechannels, unsigned int flags);
	FSOUND_STREAM*		(_stdcall * SOF)	(const char *filename, unsigned int mode, int memlength);
	int 			(_stdcall * SPLAY)	(int channel, FSOUND_STREAM *stream);
	signed char		(_stdcall * SETTIME)	(FSOUND_STREAM *stream, int ms);
	int				(_stdcall * GETTIME)	(FSOUND_STREAM *stream);
	int				(_stdcall * GETLENGTH)	(FSOUND_STREAM *stream);
	void			(_stdcall * CLOSE)	( void );
	
	FSOUND_STREAM  *m_Stream;
	int		m_iIsPlaying;
	char	m_szFileName[260];
	HINSTANCE	m_hFMod;

public:
	int		Initialize();
	int		Shutdown();
	int		PlayMP3( const char *pszSong, BOOL Loop, long ms );
	int		StopMP3();
};

extern CMP3 gMP3;
#endif

