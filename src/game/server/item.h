/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ITEM_H
#define GAME_SERVER_ITEM_H
#include <base/tl/array.h>

enum
{
	RESOURCE_LOG=0, // STEP 1
	RESOURCE_COAL, // STEP 2
	RESOURCE_COPPER, // STEP 3
	RESOURCE_IRON, // STEP 4
	RESOURCE_GOLD, // STEP 5
	RESOURCE_DIAMOND, // STEP 6
	RESOURCE_ENEGRY,
	RESOURCE_ZOMBIEHEART,
    NUM_RESOURCE,
};

enum
{
    LEVEL_LOG=1,
    LEVEL_COAL,
    LEVEL_COPPER,
    LEVEL_IRON,
    LEVEL_GOLD,
    LEVEL_DIAMOND,
    LEVEL_ENEGRY,
    NUM_LEVELS
};

enum
{
    ITYPE_PICKAXE=0,
    ITYPE_AXE,
    ITYPE_SWORD,
    ITYPE_TURRET,
};
#endif