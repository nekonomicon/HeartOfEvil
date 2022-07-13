
#define SF_AMBIENT_MP3_LOOPED 1
#define SF_AMBIENT_MP3_START_ON 2

#define MP3_ON 1
#define MP3_OFF 0
#define MP3_UNKNOWN -1

class CAmbientMP3 : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	virtual int	ObjectCaps( void ) { return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	void KeyValue(KeyValueData *pkvd);

	static BOOL m_nMp3On;
	static void SetMP3();
	static BOOL GetMP3();

	float m_flPlayTime;
	float m_flDuration;
	BOOL m_fIsPlaying;
};


