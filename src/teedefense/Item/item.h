#pragma once
#include <cstring>

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

    ITEM_SWORD_IRON,
    ITEM_AXE_COPPER,
    ITEM_PICKAXE_COPPER,

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

    ITEM_TURRET_BEGINNER,
    ITEM_TURRET_INTERMEDIATE,
    ITEM_TURRET_ADVANCED,
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

enum
{
    CARD_LIGHTNING = 1,
    CARD_FLAME,
    CARD_SHOOT,
    CARD_LASER_BEAM,
    CARD_WATER,
};

struct CItem
{
    int m_Type;
    int m_ID;
    char m_ItemName[128];
    int m_Proba;
    int m_Damage;
    int m_Formula[NUM_ITEM];
    int m_Capacity;
    int m_Unplaceable;

    CItem()
    {
        std::memset(this, 0, sizeof(CItem));
    }
};

class CItem_F
{
private:
    class CGameContext *m_pGameServer;

    class CGameContext *GameServer() const { return m_pGameServer; }

public:
    CItem_F(class CGameContext *pGameServer);
    void LoadIndex();
    void LoadItem(const char *FileName);
    void LoadFormula(const char *FileName);

    CItem m_Items[NUM_ITEM];

    int FindItem(const char *ItemName);
};