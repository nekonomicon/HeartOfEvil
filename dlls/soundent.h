/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// Soundent.h - the entity that spawns when the world 
// spawns, and handles the world's active and free sound
// lists.
//=========================================================

#define	MAX_WORLD_SOUNDS	64 // maximum number of sounds handled by the world at one time.

#define bits_SOUND_NONE		0
#define	bits_SOUND_COMBAT	( 1 << 0 )// gunshots, explosions
#define bits_SOUND_WORLD	( 1 << 1 )// door opening/closing, glass breaking
#define bits_SOUND_PLAYER	( 1 << 2 )// all noises generated by player. walking, shooting, falling, splashing
#define bits_SOUND_CARCASS	( 1 << 3 )// dead body
#define bits_SOUND_MEAT		( 1 << 4 )// gib or pork chop
#define bits_SOUND_DANGER	( 1 << 5 )// pending danger. Grenade that is about to explode, explosive barrel that is damaged, falling crate
#define bits_SOUND_GARBAGE	( 1 << 6 )// trash cans, banana peels, old fast food bags.

#define bits_SOUND_VOCAL_GENERIC	( 1 << 7 )	// Generic vocal
#define bits_SOUND_VOCAL_HELP		( 1 << 8 )  // Requesting assistance
#define bits_SOUND_VOCAL_IDLE		( 1 << 9 )	// Any idle sound that does not expect a response
#define bits_SOUND_VOCAL_QUESTION	( 1 << 10 )	// Any idle sound that expects a response
#define bits_SOUND_VOCAL_PAIN		( 1 << 11 )	// Pain
#define bits_SOUND_VOCAL_DEATH		( 1 << 12 )	// Death
#define bits_SOUND_VOCAL_MEDIC		( 1 << 13 ) // Calling for a medic
#define bits_SOUND_VOCAL_ENEMY		( 1 << 14 ) // Indicating the presence of an enemy nearby ( alert, charge, take cover )
#define bits_SOUND_VOCAL_CLEAR		( 1 << 15 ) // Indicating no enemies in this area

#define bits_SOUND_SQUADCMD_CHECK_IN			( 1 << 16 ) // Ordering squad to check in
#define bits_SOUND_SQUADCMD_COME_HERE			( 1 << 17 ) // Ordering squad to come to me
#define bits_SOUND_SQUADCMD_SEARCH_AND_DESTROY	( 1 << 18 ) // Ordering squad to go on a search and destroy mission
#define bits_SOUND_SQUADCMD_SURPRESSING_FIRE	( 1 << 19 ) // Ordering squad to lay down surpressing fire
#define bits_SOUND_SQUADCMD_ATTACK				( 1 << 20 ) // Ordering squad to attack
#define bits_SOUND_SQUADCMD_DEFENSE				( 1 << 21 ) // Ordering squad to defend the wounded
#define bits_SOUND_SQUADCMD_RETREAT				( 1 << 22 ) // Ordering squad to retreat
#define bits_SOUND_SQUADCMD_OUTTA_MY_WAY		( 1 << 23 ) // Telling someone to get out of my way
#define bits_SOUND_SQUADCMD_GET_DOWN			( 1 << 24 ) // Telling someone to get down
#define bits_SOUND_SQUADCMD_BEHIND_YOU			( 1 << 25 ) // Telling someone to look behind them
#define bits_SOUND_SQUADCMD_RECRUIT				( 1 << 26 ) // Recruiting a squad

#define bits_ALL_SOUNDS 0xFFFFFFFF

// Squad Commands
#define bits_SOUND_SQUADCMD ( bits_SOUND_SQUADCMD_CHECK_IN | bits_SOUND_SQUADCMD_COME_HERE | bits_SOUND_SQUADCMD_SEARCH_AND_DESTROY | bits_SOUND_SQUADCMD_SURPRESSING_FIRE | bits_SOUND_SQUADCMD_ATTACK | bits_SOUND_SQUADCMD_DEFENSE | bits_SOUND_SQUADCMD_RETREAT | bits_SOUND_SQUADCMD_OUTTA_MY_WAY | bits_SOUND_SQUADCMD_GET_DOWN | bits_SOUND_SQUADCMD_BEHIND_YOU | bits_SOUND_SQUADCMD_RECRUIT )

// All noises generated by monsters or humans talking or grunting or whatever
#define bits_SOUND_VOCAL ( bits_SOUND_VOCAL_GENERIC | bits_SOUND_VOCAL_HELP | bits_SOUND_VOCAL_IDLE | bits_SOUND_VOCAL_QUESTION | bits_SOUND_VOCAL_PAIN | bits_SOUND_VOCAL_DEATH | bits_SOUND_VOCAL_MEDIC | bits_SOUND_VOCAL_ENEMY | bits_SOUND_VOCAL_CLEAR  | bits_SOUND_SQUADCMD )

#define SOUNDLIST_EMPTY	-1

#define SOUNDLISTTYPE_FREE	1// identifiers passed to functions that can operate on either list, to indicate which list to operate on.
#define SOUNDLISTTYPE_ACTIVE 2

#define	SOUND_NEVER_EXPIRE	-1 // with this set as a sound's ExpireTime, the sound will never expire.


//=========================================================
// CSound - an instance of a sound in the world.
//=========================================================
class CSound
{
public:

	void	Clear ( void );
	void	Reset ( void );

	Vector	 m_vecOrigin;	// sound's location in space
	int		 m_iType;		// what type of sound this is
	int		 m_iVolume;		// how loud the sound is
	float	 m_flExpireTime;// when the sound should be purged from the list
	int		 m_iNext;		// index of next sound in this list ( Active or Free )
	int		 m_iNextAudible;// temporary link that monsters use to build a list of audible sounds
	int		 m_iClass;		// The classification of the entity that made the sound
	string_t m_netname;		// The name of the squad that the guy who emitted the sound was in

	BOOL	FIsSound( void );
	BOOL	FIsScent( void );
	
	inline int Classify() { return m_iClass; };
};

//=========================================================
// CSoundEnt - a single instance of this entity spawns when
// the world spawns. The SoundEnt's job is to update the 
// world's Free and Active sound lists.
//=========================================================
class CSoundEnt : public CBaseEntity 
{
public:

	void Precache ( void );
	void Spawn( void );
	void Think( void );
	void Initialize ( void );
	
	static void		InsertSound ( int iType, int iClass, const Vector &vecOrigin, int iVolume, float flDuration );
	static void		FreeSound ( int iSound, int iPrevious );
	static int		ActiveList( void );// return the head of the active list
	static int		FreeList( void );// return the head of the free list
	static CSound*	SoundPointerForIndex( int iIndex );// return a pointer for this index in the sound list
	static int		ClientSoundIndex ( edict_t *pClient );

	BOOL	IsEmpty( void ) { return m_iActiveSound == SOUNDLIST_EMPTY; }
	int		ISoundsInList ( int iListType );
	int		IAllocSound ( void );
	virtual int		ObjectCaps( void ) { return FCAP_DONT_SAVE; }
	
	int		m_iFreeSound;	// index of the first sound in the free sound list
	int		m_iActiveSound; // indes of the first sound in the active sound list
	int		m_cLastActiveSounds; // keeps track of the number of active sounds at the last update. (for diagnostic work)
	BOOL	m_fShowReport; // if true, dump information about free/active sounds.

private:
	CSound		m_SoundPool[ MAX_WORLD_SOUNDS ];
};