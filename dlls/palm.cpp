#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

class CPalm : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify( void ) { return CLASS_PLANT; };
	virtual BOOL IsAlive( void ) { return FALSE; }
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	BOOL ShouldGibMonster( int iGib ) { return TRUE; };
	void GibMonster( void );
};


LINK_ENTITY_TO_CLASS( env_palm, CPalm );


void CPalm::Precache( void )
{
	PRECACHE_MODEL( "models/palm.mdl" );
	PRECACHE_MODEL( "models/palmgibs.mdl" );
}


void CPalm::Spawn( void )
{
	Precache();

	SET_MODEL( ENT(pev), "models/palm.mdl" );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_YES;
	m_bloodColor		= DONT_BLEED;
	UTIL_SetSize(pev, Vector(-6, -6, 0), Vector(6, 6, 155));

	if ( pev->body == -1 )
	{
		pev->body = RANDOM_LONG( 0, 2 );
	}
}


void CPalm::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// Spray some particles where I've been hit

	if ( ptr )
	{
		// Puff of Smoke

		Vector vecSmoke = ptr->vecEndPos + ptr->vecPlaneNormal * 10;

		// aditive sprite, plays 1 cycle
		// coord, coord, coord (position) 
		// short (sprite index) 
		// byte (scale in 0.1's) 
		// byte (brightness)

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vecSmoke );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecSmoke.x );
			WRITE_COORD( vecSmoke.y );
			WRITE_COORD( vecSmoke.z );
			WRITE_SHORT( ( RANDOM_LONG(0,1) ? g_sSpriteIndexWallPuff1 : g_sSpriteIndexWallPuff2 ) );
			WRITE_BYTE( 10 );	//scale
			WRITE_BYTE( 150 );	// brightness
		MESSAGE_END();
	}

	// Make a wood damaged sound

	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0: EMIT_SOUND( edict(), CHAN_BODY, "debris/wood1.wav", 1.0, ATTN_NORM ); break;
	case 1: EMIT_SOUND( edict(), CHAN_BODY, "debris/wood2.wav", 1.0, ATTN_NORM ); break;
	case 2: EMIT_SOUND( edict(), CHAN_BODY, "debris/wood3.wav", 1.0, ATTN_NORM ); break;
	}
		

	// No damage caused by bullets -  or anything else in this ridiculous list

	if ( FBitSet( bitsDamageType, ( DMG_BULLET | DMG_CLUB | DMG_FREEZE | DMG_FALL | DMG_SHOCK | DMG_DROWN | 
			DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWFREEZE ) ) )
	{
		flDamage = 0;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CPalm :: GibMonster( void )
{
	CGib::SpawnRandomGibs( pev, 4 + pev->body, GIB_PALM );
}

