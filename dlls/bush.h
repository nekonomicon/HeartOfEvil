#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "animation.h"
#include "effects.h"

#define LEAF_GIB_COUNT 5

class CLeafGib : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT FlyThink( void );

	float m_lifeTime;
};


class CBush : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue(KeyValueData *pkvd);
	void Touch ( CBaseEntity *pOther );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Killed(entvars_t *pevAttacker, int iGib);
	void CreateGib( void );
	int  Classify( void ) { return CLASS_PLANT; };
	virtual BOOL IsAlive( void ) { return FALSE; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];	
	int m_iModelNum;
	float m_flLastTouched;
};


class CFuncJungle : public CBaseEntity
{
	void	Spawn( void );
	void	Precache( void );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void Touch ( CBaseEntity *pOther );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void Killed(entvars_t *pevAttacker, int iGib);
	void KeyValue(KeyValueData *pkvd);

	void CreateGib( Vector min, Vector size );
	int m_iLeafGib;
	float m_flLastTouched;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];	
};

