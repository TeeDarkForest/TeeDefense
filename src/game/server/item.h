/* (c) EDreemurr. See licence.txt in the root of the distribution for more information.      */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ITEM_H
#define GAME_SERVER_ITEM_H
#include <base/tl/array.h>

enum
{
    ITEM_LOG = 1,
    ITEM_COAL,
    ITEM_COPPER,
    ITEM_IRON,
    ITEM_GOLDEN,
    ITEM_DIAMOND,
    ITEM_ENEGRY,
    ITEM_ZOMBIEHEART,

    ITEM_SWORD_LOG,
    ITEM_AXE_LOG,
    ITEM_PICKAXE_LOG,

    ITEM_AXE_COPPER,
    ITEM_PICKAXE_COPPER,
    ITEM_SWORD_IRON,

    ITEM_AXE_IRON,
    ITEM_PICKAXE_IRON,

    ITEM_SWORD_GOLDEN,
    ITEM_AXE_GOLDEN,
    ITEM_PICKAXE_GOLDEN,

    ITEM_SWORD_DIAMOND,
    ITEM_AXE_DIAMOND,
    ITEM_PICKAXE_DIAMOND,

    ITEM_SWORD_ENEGRY,
    ITEM_PICKAXE_ENEGRY,

    ITEM_GUN_TURRET,
    ITEM_SHOTGUN_TURRET,
    ITEM_LASER_TURRET,
    ITEM_LASER_TURRET_2077,
    ITEM_FOLLOW_GRENADE_TURRET,
    ITEM_FGUN_TURRET,
    ITEM_SHOTGUN_TURRET_2077,
    NUM_ITEM,
};

enum
{
    LEVEL_LOG = 1,
    LEVEL_COAL,
    LEVEL_COPPER,
    LEVEL_IRON,
    LEVEL_GOLD,
    LEVEL_DIAMOND,
    LEVEL_ENEGRY,
    LEVEL_ALLOY,
    LEVEL_ENEGRY_CORE,
    LEVEL_StarCrusher,
    NUM_LEVELS
};

enum
{
    ITYPE_PICKAXE = 0,
    ITYPE_AXE,
    ITYPE_SWORD,
    ITYPE_TURRET,
    ITYPE_MATERIAL,
    NUM_ITYPE,
};
#endif