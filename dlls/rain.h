//LRC- the values used for the new "global states" mechanism.
typedef enum
{
	STATE_OFF = 0,	// disabled, inactive, invisible, closed, or stateless. Or non-alert monster.
	STATE_TURN_ON,  // door opening, env_fade fading in, etc.
	STATE_ON,		// enabled, active, visisble, or open. Or alert monster.
	STATE_TURN_OFF, // door closing, monster dying (?).
	STATE_IN_USE,	// player is in control (train/tank/barney/scientist).
					// In_Use isn't very useful, I'll probably remove it.
} STATE;


//=========================================================
// LRC - env_fog, extended a bit from the DMC version
//=========================================================

#define SF_FOG_ACTIVE 1
#define SF_FOG_FADING 0x8000

#define FOG_ON 1
#define FOG_OFF 0
#define FOG_UNKNOWN -1

class CEnvFog : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT Active( void );
	void EXPORT TurnOn( void );
	void EXPORT TurnOff( void );
	void EXPORT FadeInDone( void );
	void EXPORT FadeOutDone( void );
	void SendData( Vector col, int fFadeTime, int StartDist, int iEndDist);
	void KeyValue( KeyValueData *pkvd );
	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	STATE GetState( void );

	int m_iStartDist;
	int m_iEndDist;
	float m_iFadeIn;
	float m_iFadeOut;
	float m_fHoldTime;
	float m_fActiveTime;
	float m_fFadeStart; // if we're fading in/out, then when did the fade start?

	//LRC use this instead of "SetThink( NULL )" or "pev->nextthink = -1".
	void	DontThink( void );
	float	m_fNextThink; // LRC - for SetNextThink and SetPhysThink. Marks the time when a think will be performed - not necessarily the same as pev->nextthink!
	virtual void SetNextThink( float delay ) { SetNextThink(delay, FALSE); }
	virtual void SetNextThink( float delay, BOOL correctSpeed );
	BOOL ShouldToggle( USE_TYPE useType );

	static BOOL m_nFogOn;
	static void SetFog();
	static BOOL GetFog();
};



class CRain : public CBaseEntity
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
	void CreateRainParticle( void );
	void EXPORT RainThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL m_bOn;
	float m_fFreq;
	float m_iRadius;
	int m_iMaxDist;
	int m_iRainSprite;
};




//=========================================================
//Spirit of Half-Life code
//
//LRC- the long-awaited effect. (Rain, in the desert? :)
//
//FIXME: give designers a _lot_ more control.
//=========================================================

class CEnvRain : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	Think( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	STATE	m_iState;
	int		m_spriteTexture;
	float	m_burstSize;
	int		m_iMaxDist;
	float	m_flUpdateTime;
	virtual STATE GetState( void ) { return m_iState; };
};


