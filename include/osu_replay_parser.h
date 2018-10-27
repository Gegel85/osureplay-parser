#ifndef OSUREPLAY_PARSER_LIBRARY_H
#define OSUREPLAY_PARSER_LIBRARY_H

#include <stdbool.h>
#include <stdio.h>

#define DELNEG(x) (x > 0 ? x : 0)

enum OsuGameMode {
	GAME_OSU_STANDARD,
	GAME_TAIKO,
	GAME_CATCH_THE_BEAT,
	GAME_OSU_MANIA,
	GAME_STD	= GAME_OSU_STANDARD,
	GAME_CTB	= GAME_CATCH_THE_BEAT,
	GAME_MANIA	= GAME_OSU_MANIA,
};

enum OsuInputs {
	INPUT_NONE	= 0,
	INPUT_MOUSE1	= 1 << 0,
	INPUT_MOUSE2	= 1 << 1,
	INPUT_KEY1	= 1 << 2,
	INPUT_KEY2	= 1 << 3,
	INPUT_SMOKE	= 1 << 4,
	INPUT_M1	= INPUT_MOUSE1,
	INPUT_M2	= INPUT_MOUSE2,
	INPUT_K1	= INPUT_KEY1,
	INPUT_K2	= INPUT_KEY2,
} OsuInputs;

enum OsuMode {
	MODE_NONE		= 0,
	MODE_NO_FAIL		= 1 << 0,
	MODE_EASY		= 1 << 1,
	MODE_TOUCH_DEVICE	= 1 << 2,
	MODE_HIDDEN		= 1 << 3,
	MODE_HARD_ROCK		= 1 << 4,
	MODE_SUDDEN_DEATH	= 1 << 5,
	MODE_DOUBLE_TIME	= 1 << 6,
	MODE_RELAX		= 1 << 7,
	MODE_HALF_TIME		= 1 << 8,
	MODE_NIGHTCORE		= 1 << 9,
	MODE_FLASHLIGHT		= 1 << 10,
	MODE_AUTOPLAY		= 1 << 11,
	MODE_SPUNOUT		= 1 << 12,
	MODE_AUTOPILOT		= 1 << 13,
	MODE_PERFECT		= 1 << 14,
	MODE_KEY4		= 1 << 15,
	MODE_KEY5		= 1 << 16,
	MODE_KEY6		= 1 << 17,
	MODE_KEY7		= 1 << 18,
	MODE_KEY8		= 1 << 19,
	MODE_KEYMOD		= MODE_KEY4 + MODE_KEY5 + MODE_KEY6 + MODE_KEY7 + MODE_KEY8,
	MODE_FADEIN		= 1 << 20,
	MODE_RANDOM		= 1 << 21,
	MODE_CINEMA		= 1 << 22,
	MODE_TARGET_PRACTICE	= 1 << 23,
	MODE_KEY9		= 1 << 24,
	MODE_COOP		= 1 << 25,
	MODE_KEY1		= 1 << 26,
	MODE_KEY3		= 1 << 27,
	MODE_KEY2		= 1 << 28,
};

typedef struct OsuScore {
	unsigned short	nbOf300;
	unsigned short	nbOf100;
	unsigned short	nbOf50;
	unsigned short	nbOfGekis;
	unsigned short	nbOfKatus;
	unsigned short	nbOfMiss;
	unsigned int	totalScore;
	unsigned short	maxCombo;
	bool		noComboBreak;
} OsuScore;

typedef struct OsuFloatVector {
	float	x;
	float	y;
} OsuFloatVector;

typedef struct OsuLifeEvent {
	unsigned long	timeToHappen;
	float		newValue;
} OsuLifeEvent;

typedef struct OsuGameEvent {
	unsigned long	timeToHappen;
	OsuFloatVector	cursorPos;
	unsigned int	keysPressed;
} OsuGameEvent;

typedef struct OsuReplayArray {
	size_t		length;
	unsigned char	*content;
} OsuString;

typedef struct OsuLifeEventArray {
	size_t		length;
	OsuLifeEvent	*content;
} OsuLifeEventArray;

typedef struct OsuGameEventArray {
	size_t		length;
	OsuGameEvent	*content;
} OsuGameEventArray;

typedef struct OsuReplay {
	char			*error;
	unsigned char		mode;
	unsigned int		version;
	char			*mapHash;
	char			*playerName;
	char			*replayHash;
	OsuScore		score;
	unsigned int		mods;
	OsuLifeEventArray	lifeBar;
	OsuGameEventArray	gameEvents;
	unsigned long		timestamp;
	unsigned long		something;
} OsuReplay;

OsuReplay	OsuReplay_parseReplayFile(char *path);
char		*OsuReplay_gameModeToString(unsigned char mode);
OsuReplay	OsuReplay_parseReplayString(unsigned char *string, size_t size);

#endif