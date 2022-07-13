#include "bush.h"

void CLeafGib::Spawn( void )
{
	SET_MODEL( ENT(pev), "models/leafgibs.mdl" );
	
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	pev->avelocity = Vector( RANDOM_LONG(0, 200), RANDOM_LONG(0, 200), RANDOM_LONG(0, 200));

	m_lifeTime = 25;

	SetThink ( FlyThink );
	pev->nextthink = gpGlobals->time + 1;
}


#define TERMINAL_VELOCITY -100

void CLeafGib::FlyThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	if ( pev->velocity == g_vecZero )
	{
		SetThink (SUB_StartFadeOut);
		pev->nextthink = gpGlobals->time + m_lifeTime;
	}

	if (pev->velocity.z > TERMINAL_VELOCITY ) pev->velocity.z-=10;
	else if (pev->velocity.z < TERMINAL_VELOCITY ) pev->velocity.z+=10;
	
	if (pev->velocity.x > 0 ) pev->velocity.x-=10;
	else if (pev->velocity.x < 0 ) pev->velocity.x+=10;
	
	if (pev->velocity.y > 0 ) pev->velocity.y-=10;
	else if (pev->velocity.y < 0 ) pev->velocity.y+=10;
	
	pev->nextthink = gpGlobals->time + 0.1;

}



#define NUM_BUSHES 3


LINK_ENTITY_TO_CLASS( env_bush, CBush );

TYPEDESCRIPTION	CBush::m_SaveData[] = 
{
	DEFINE_FIELD( CBush, m_iModelNum, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBush, CBaseMonster );



void CBush::Spawn( void )
{
	if ( FStringNull( pev->model ) )
	{
		if (m_iModelNum == -1) m_iModelNum = RANDOM_LONG( 0, NUM_BUSHES - 1 );

		switch (m_iModelNum)
		{
		case 0: pev->model = MAKE_STRING("Sprites/bush1.spr"); break;
		case 1: pev->model = MAKE_STRING("Sprites/bush2.spr"); break;
		case 2: pev->model = MAKE_STRING("Sprites/bush3.spr"); break;
		}
	}

	Precache();
	SET_MODEL( ENT(pev), STRING(pev->model) );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_YES;
	pev->effects		= 0;
	pev->rendermode		= kRenderTransTexture; //kRenderTransAlpha;  Don't ask, just do it.
	pev->renderamt		= 255;
	pev->rendercolor	= Vector(0, 0, 0);
	pev->renderfx       = kRenderFxNone;
	m_bloodColor		= DONT_BLEED;

	pev->origin = pev->origin + Vector(0, 0, 64);
	UTIL_SetSize(pev, Vector(-16, -16, -64), Vector(16, 16, 16));
}


void CBush::Precache( void )
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
	PRECACHE_MODEL( "models/leafgibs.mdl" );

	PRECACHE_SOUND( "ambience/bush1.wav" );
	PRECACHE_SOUND( "ambience/bush2.wav" );
}


void CBush :: Touch ( CBaseEntity *pOther )
{
	if ( pOther->pev->velocity == g_vecZero) return;

	if ( m_flLastTouched > gpGlobals->time - 1.0 ) return;
	m_flLastTouched = gpGlobals->time;

	// Play a rustling sound

	switch (RANDOM_LONG(0, 1))
	{
	case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "ambience/bush1.wav", 1.0, ATTN_NORM ); break;
	case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "ambience/bush2.wav", 1.0, ATTN_NORM ); break;
	}
}


#define MAX_DEATH_GIBS 20

int CBush :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	for (int i = 0; i < (int) ceil(flDamage / 10.0f); i++ ) CreateGib();

	if (bitsDamageType == DMG_BULLET) flDamage = 0;  // bullets don't kill a bush

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CBush :: Killed(entvars_t *pevAttacker, int iGib)
{
	for (int i = 0; i < MAX_DEATH_GIBS; i++ ) CreateGib();
	UTIL_Remove( this );
}

void CBush::CreateGib( void )
{
	CLeafGib *pGib = GetClassPtr( (CLeafGib *)NULL );

	pGib->Spawn( );
	pGib->pev->body = RANDOM_LONG(0, LEAF_GIB_COUNT-1);

	// spawn the gib somewhere in the bush's bounding volume
	pGib->pev->origin.x = pev->absmin.x + pev->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
	pGib->pev->origin.y = pev->absmin.y + pev->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
	pGib->pev->origin.z = pev->absmin.z + pev->size.z * (RANDOM_FLOAT ( 0 , 1 ) ) + 1;	// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

	// mix in some noise
	pGib->pev->velocity.x += RANDOM_FLOAT ( -0.25, 0.25 );
	pGib->pev->velocity.y += RANDOM_FLOAT ( -0.25, 0.25 );
	pGib->pev->velocity.z += RANDOM_FLOAT ( -0.25, 0.25 );

	pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT ( 300, 400 );

	pGib->pev->avelocity.x = RANDOM_FLOAT ( 100, 200 );
	pGib->pev->avelocity.y = RANDOM_FLOAT ( 100, 200 );

	if ( pev->health > -50)
	{
		pGib->pev->velocity = pGib->pev->velocity * 0.7;
	}
	else if ( pev->health > -200)
	{
		pGib->pev->velocity = pGib->pev->velocity * 2;
	}
	else
	{
		pGib->pev->velocity = pGib->pev->velocity * 4;
	}
}

void CBush::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq(pkvd->szKeyName, "modelnum") )
	{
		m_iModelNum = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


#define SF_JUNGLE_SOLID 1
#define SF_JUNGLE_BULLETDAMAGE 2

#define LEAFGIB_TOUCHED 1
#define LEAFGIB_DAMAGED 2

LINK_ENTITY_TO_CLASS( func_jungle, CFuncJungle );

TYPEDESCRIPTION	CFuncJungle::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncJungle, m_iLeafGib, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncJungle, CBaseEntity );


void CFuncJungle::Spawn( void )
{
	pev->angles = g_vecZero;

	if (FBitSet( pev->spawnflags, SF_JUNGLE_SOLID) )
	{
		pev->solid = SOLID_BSP;
		pev->movetype = MOVETYPE_PUSH;  
	}
	else
	{
		pev->solid = SOLID_TRIGGER;
		pev->movetype = MOVETYPE_NONE;  
	}
	
	pev->takedamage	= DAMAGE_YES;
	if (pev->health == -1) pev->health = 8000;

	//ALERT(at_console, "Health: %f\n", pev->health);

	Precache();
	SET_MODEL( ENT(pev), STRING(pev->model) );

}


void CFuncJungle::Precache( void )
{
	PRECACHE_MODEL( "models/leafgibs.mdl" );

	PRECACHE_SOUND( "ambience/bush1.wav" );
	PRECACHE_SOUND( "ambience/bush2.wav" );
}


#define JUNGLE_TOUCH_LEAVES 10

void CFuncJungle::Touch ( CBaseEntity *pOther )
{
	if ( pOther->pev->velocity == g_vecZero) return;

	if ( m_flLastTouched > gpGlobals->time - 1.0 ) return;
	m_flLastTouched = gpGlobals->time;

	// Play a rustling sound

	switch (RANDOM_LONG(0, 1))
	{
	case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "ambience/bush1.wav", 1.0, ATTN_NORM ); break;
	case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "ambience/bush2.wav", 1.0, ATTN_NORM ); break;
	}

	if (FBitSet( m_iLeafGib, LEAFGIB_TOUCHED))
		for (int i = 0; i < JUNGLE_TOUCH_LEAVES; i++) 
	{
		CreateGib( pOther->pev->absmin, pOther->pev->size );
	}
}


int CFuncJungle :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (!FBitSet( pev->spawnflags, SF_JUNGLE_BULLETDAMAGE) &&
		bitsDamageType == DMG_BULLET) flDamage = 0;

	if (pev->health == 8000) flDamage = 0;

	//ALERT(at_console, "Health: %f, Damage: %f\n", pev->health, flDamage);

	return CBaseEntity::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CFuncJungle::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if (FBitSet( m_iLeafGib, LEAFGIB_DAMAGED) )
		for (int i = 0; i < (int) ceil(flDamage / 10.0f); i++ ) 
			CreateGib( ptr->vecEndPos + (ptr->vecPlaneNormal * 40) - Vector(20, 20, 20), Vector(20, 20, 20) );
}


void CFuncJungle :: Killed(entvars_t *pevAttacker, int iGib)
{
	for (int i = 0; i < (int) ceil( pev->size.x * pev->size.y * pev->size.z / 32768.0f) ; i++ ) 
		CreateGib( pev->absmin, pev->size );
	UTIL_Remove( this );
}


void CFuncJungle::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq(pkvd->szKeyName, "leafgib") )
	{
		m_iLeafGib = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


void CFuncJungle::CreateGib( Vector min, Vector size )
{
	CLeafGib *pGib = GetClassPtr( (CLeafGib *)NULL );

	pGib->Spawn( );
	pGib->pev->body = RANDOM_LONG(0, LEAF_GIB_COUNT-1);

	// spawn the gib somewhere in the bush's bounding volume
	pGib->pev->origin.x = min.x + size.x * (RANDOM_FLOAT ( 0 , 1 ) );
	pGib->pev->origin.y = min.y + size.y * (RANDOM_FLOAT ( 0 , 1 ) );
	pGib->pev->origin.z = min.z + size.z * (RANDOM_FLOAT ( 0 , 1 ) ) + 1;	// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

	// mix in some noise
	pGib->pev->velocity.x += RANDOM_FLOAT ( -0.25, 0.25 );
	pGib->pev->velocity.y += RANDOM_FLOAT ( -0.25, 0.25 );
	pGib->pev->velocity.z += RANDOM_FLOAT ( -0.25, 0.25 );

	pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT ( 100, 200 );
}


