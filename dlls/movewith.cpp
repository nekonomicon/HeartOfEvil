#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "rain.h"	// for the toggle state thing

#define PI 3.14159265359
#define RAD(deg) ((2 * PI * deg) / 360.0)
#define DEG(rad) ((360.0 * rad) / (2 * PI))

#define SF_MOVEWITH_START_ON 1
#define SF_MOVEWITH_MAKE_NOT_SOLID 2

class CMoveWith : public CBaseEntity
{
public:
	void Spawn( void );
	EXPORT void FindThink( void );
	EXPORT void MoveThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hMover;	// Mover
	EHANDLE m_hMovee;	// Moved
	Vector m_vecOffset;
	float m_flOffsetYaw;
	float m_flOldMoverYaw;
	STATE	m_iState;
	int m_iMoveeMoveType;
	int m_iMoveeSolid;

};

TYPEDESCRIPTION	CMoveWith::m_SaveData[] = 
{
	DEFINE_FIELD( CMoveWith, m_flOldMoverYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CMoveWith, m_flOffsetYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CMoveWith, m_vecOffset, FIELD_VECTOR ),
	DEFINE_FIELD( CMoveWith, m_hMover, FIELD_EHANDLE ),
	DEFINE_FIELD( CMoveWith, m_hMovee, FIELD_EHANDLE ),
	DEFINE_FIELD( CMoveWith, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CMoveWith, m_iMoveeMoveType, FIELD_INTEGER ),
	DEFINE_FIELD( CMoveWith, m_iMoveeSolid, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMoveWith, CBaseEntity );

LINK_ENTITY_TO_CLASS( info_movewith, CMoveWith );


void CMoveWith::Spawn( void )
{
	if ( FBitSet( pev->spawnflags, SF_MOVEWITH_START_ON ) )
	{
		SetThink( FindThink );
		pev->nextthink = gpGlobals->time + 1.5;
		m_iState = STATE_ON;
	}
	else
	{
		m_iState = STATE_OFF;
	}
}


void CMoveWith::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!ShouldToggle(useType, m_iState)) return;

	if (m_iState == STATE_ON)
	{
		m_iState = STATE_OFF;
		pev->nextthink = -1;
		if ( FBitSet( pev->spawnflags, SF_MOVEWITH_MAKE_NOT_SOLID ) )
		{
			m_hMovee->pev->movetype = m_iMoveeMoveType;
			m_hMovee->pev->solid = m_iMoveeSolid;
		}
	}
	else
	{
		m_iState = STATE_ON;
		SetThink( FindThink );
		pev->nextthink = gpGlobals->time + 0;
	}
}


void CMoveWith::FindThink( void )
{
	m_hMover = NULL;
	m_hMovee = NULL;

	// Get mover 

	if ( pev->target )
	{
		edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));
		
		if (!FNullEnt(pentTarget))
		{
			m_hMover = Instance( pentTarget );
		}  
	}

	// Get movee

	if ( pev->message )
	{
		edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->message));
		
		if (!FNullEnt(pentTarget))
		{
			m_hMovee = Instance( pentTarget );
		}  
	}

	if ( m_hMover != NULL && m_hMovee != NULL )
	{
		SetThink( MoveThink );
		pev->nextthink = gpGlobals->time + 0.1;

		// Get offset
		m_vecOffset = m_hMovee->pev->origin - m_hMover->pev->origin;

		// Get mover yaws
		m_flOffsetYaw = m_flOldMoverYaw = m_hMover->pev->angles.y;
		
		// Make movee not solid
		if ( FBitSet( pev->spawnflags, SF_MOVEWITH_MAKE_NOT_SOLID ) )
		{
			m_iMoveeMoveType = m_hMovee->pev->movetype;
			m_iMoveeSolid = m_hMovee->pev->solid;
			m_hMovee->pev->movetype = MOVETYPE_NOCLIP;
			m_hMovee->pev->solid = SOLID_NOT;
		}

	}
}


void CMoveWith::MoveThink( void )
{
	if ( m_hMover != NULL && m_hMovee != NULL )
	{
		pev->nextthink = gpGlobals->time + gpGlobals->frametime;

		// Mover Yaw in radians
		float rad = RAD( UTIL_AngleDiff( m_flOffsetYaw, m_hMover->pev->angles.y ) );
		Vector vecOffset = m_vecOffset;

		// get offset vector rotated by yaw
		vecOffset.x = m_vecOffset.x * cos( rad ) +  m_vecOffset.y * sin( rad );
		vecOffset.y = m_vecOffset.y * cos( rad ) -  m_vecOffset.x * sin( rad );

		// Update movee origin
		Vector vecDest = m_hMover->pev->origin + vecOffset;
		UTIL_SetOrigin( m_hMovee->pev, vecDest );
		
		// Update movee yaw
		m_hMovee->pev->angles.y += UTIL_AngleDiff( m_hMover->pev->angles.y, m_flOldMoverYaw );
		m_flOldMoverYaw = m_hMover->pev->angles.y;

		// Update movee velocity
		m_hMovee->pev->velocity = m_hMover->pev->velocity;

	}
}
