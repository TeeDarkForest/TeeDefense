/* (c) EDreemurr. See licence.txt in the root of the distribution for more information.      */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ITEM_H
#define GAME_SERVER_ITEM_H
#include <base/tl/array.h>

enum
{
    // Level 1
	RESOURCE_LOG=0,
	RESOURCE_COAL,
	RESOURCE_COPPER,
	RESOURCE_IRON,
	RESOURCE_GOLD,
	RESOURCE_DIAMOND,
	RESOURCE_ENEGRY,
	RESOURCE_ZOMBIEHEART,

    // Level 2 : D e e p   A b y s s
    Abyss_LumSub, // Luminous Substance
    Abyss_Agar,
    Abyss_ScrapMetal,
    Abyss_ScrapMatal_S,
    Abyss_NuclearWaste_S,
    Abyss_Remnant,
    Abyss_MoonlightIngot,
    Abyss_Alloy,
    Abyss_Yuerks,
    Abyss_StarLightIngot,
    Abyss_Enegry_CORE,
    Abyss_NuclearWaste_CORE,

    // Level End.
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
    LEVEL_AlloyPickaxe,
    LEVEL_ENEGRY_CORE,
    NUM_LEVELS
};

enum
{
    ITYPE_PICKAXE=0,
    ITYPE_AXE,
    ITYPE_SWORD,
    ITYPE_TURRET,
    ITYPE_MATERIAL,
};
#endif