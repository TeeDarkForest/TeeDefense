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

    ITEM_SWORD_LOG = 9,
    ITEM_AXE_LOG,
    ITEM_PICKAXE_LOG,

    ITEM_SWORD_IRON = 12,
    ITEM_AXE_COPPER,
    ITEM_PICKAXE_COPPER,

    ITEM_AXE_IRON = 15,
    ITEM_PICKAXE_IRON,

    ITEM_SWORD_GOLDEN = 17,
    ITEM_AXE_GOLDEN,
    ITEM_PICKAXE_GOLDEN,

    ITEM_SWORD_DIAMOND = 20,
    ITEM_AXE_DIAMOND,
    ITEM_PICKAXE_DIAMOND,

    ITEM_SWORD_ENEGRY = 23,
    ITEM_PICKAXE_ENEGRY,

    ITEM_TURRET_BEGINNER = 25,
    ITEM_TURRET_INTERMEDIATE,
    ITEM_TURRET_ADVANCED,

    ITEM_CARD_QUICKLY_FIRE = 28,
    ITEM_CARD_QUICKLY_LOADING,
    ITEM_CARD_DAMAGE,
    ITEM_CARD_EXPLOSION,
    ITEM_CARD_ELECTRON,
    ITEM_CARD_FUSION,
    ITEM_CARD_FORCE,
    ITEM_CARD_MANUAL,
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
    ITYPE_CARD,
    NUM_ITYPE,
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
    int m_Max;
    int m_Placeable[NUM_ITYPE];

    CItem()
    {
        std::memset(this, 0, sizeof(CItem));
    }
};

struct SPlayerItemData
{
    int m_Num;
    int m_Cards;
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

    int GetType(int ID);
    const char *GetItemName(int ID);
    int GetItemID(const char ItemName[64]);
    int GetDmg(int ID);
    int GetProba(int ID);
    int GetCapacity(int ID);
    void GetFormula(int ID, int *Formula);
    int GetMax(int ID);

    bool CheckItemVaild(int ID)
    {
        if (ID <= 0 || ID >= NUM_ITEM)
            return true;
        return false;
    }
};