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
	NUM_RESOURCE,
};

enum
{
    LEVEL_LOG=0,
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

class CItem
{
public:
    CItem(){};
    void Load(int ID, int Log, int Coal, int Copper, int Iron, int Gold, int Diamond, int Enegry);
    const char* m_Name;
    int m_Type;
    int m_NeedResource[NUM_RESOURCE];
    int m_Proba;
    int m_Level;
    int m_Damage;
    int m_Speed;
    int m_ID;
    int m_TurretType;
};

class CItemSystem
{
    class CGameContext *m_pGameServer;
    int m_IDs;
public:

    CItemSystem();

    CItem m_ItemList[128];

    bool CreateItem(const char* pItemName, int ID, int Type, int Damage, int Level, int TurretType, int Proba, 
        int Speed, int Log, int Coal, int Copper, int Iron, int Gold, int Diamond, int Enegry);

    int GetItemId(const char* pItemName);

    CItem GetItem(int ItemID) { return m_ItemList[ItemID]; };

    void MakeItem(const char* pItemName, int ClientID);

    bool CheckItemName(const char* pItemName);

    void SendCantMakeItemChat(int To, int *Resource);

    void SendMakeItemChat(int To, CItem Item);

    void SendMakeItemFailedChat(int To, int* Resource);
};


#endif