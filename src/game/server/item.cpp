/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/math.h>
#include <string.h>
#include "item.h"
#include "gamecontext.h"
#include "gamecontroller.h"
#include "entities/turret.h"

CItem::CItem(int ID, int Log, int Coal, int Copper, int Iron, int Gold, int Diamond, int Enegry)
{
    m_NeedResource[RESOURCE_LOG] = Log;
    m_NeedResource[RESOURCE_COAL] = Coal;
    m_NeedResource[RESOURCE_COPPER] = Copper;
    m_NeedResource[RESOURCE_IRON] = Iron;
    m_NeedResource[RESOURCE_GOLD] = Gold;
    m_NeedResource[RESOURCE_DIAMOND] = Diamond;
    m_NeedResource[RESOURCE_ENEGRY] = Enegry;
    m_ID = ID;
}

CItemSystem::CItemSystem(CGameContext *GameServer)
{
    m_pGameServer = GameServer;
    m_IDs = 0;
    
    // Register Items.
    CreateItem("wooden sword",// Name
     m_IDs,// ID
      ITYPE_SWORD,// ItemType
       2,// Damage
        LEVEL_LOG,// Level
         -1,// TurretType
          90,// Proba
           -1,// Speed
            25,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("wooden axe",// Name
     m_IDs,// ID
      ITYPE_AXE,// ItemType
       0,// Damage
        LEVEL_LOG,// Level
         -1,// TurretType
          90,// Proba
           10,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("wooden pickaxe",// Name
     m_IDs,// ID
      ITYPE_PICKAXE,// ItemType
       0,// Damage
        LEVEL_LOG,// Level
         -1,// TurretType
          90,// Proba
           200,// Speed
            25,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("copper sword",// Name
     m_IDs,// ID
      ITYPE_SWORD,// ItemType
       5,// Damage
        LEVEL_COPPER,// Level
         -1,// TurretType
          90,// Proba
           0,// Speed
            10,// Log
             0,// Coal
              25,//Copper
               0,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("copper axe",// Name
     m_IDs,// ID
      ITYPE_AXE,// ItemType
       0,// Damage
        LEVEL_COPPER,// Level
         -1,// TurretType
          90,// Proba
           17,// Speed
            10,// Log
             0,// Coal
              25,//Copper
               0,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("copper pickaxe",// Name
     m_IDs,// ID
      ITYPE_PICKAXE,// ItemType
       0,// Damage
        LEVEL_COPPER,// Level
         -1,// TurretType
          90,// Proba
           500,// Speed
            10,// Log
             0,// Coal
              25,//Copper
               0,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("iron sword",// Name
     m_IDs,// ID
      ITYPE_SWORD,// ItemType
       7,// Damage
        LEVEL_IRON,// Level
         -1,// TurretType
          90,// Proba
           0,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               25,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("iron axe",// Name
     m_IDs,// ID
      ITYPE_AXE,// ItemType
       0,// Damage
        LEVEL_IRON,// Level
         -1,// TurretType
          20,// Proba
           24,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               25// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("iron pickaxe",// Name
     m_IDs,// ID
      ITYPE_PICKAXE,// ItemType
       0,// Damage
        LEVEL_IRON,// Level
         -1,// TurretType
          90,// Proba
           500,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               25,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("golden sword",// Name
     m_IDs,// ID
      ITYPE_SWORD,// ItemType
       10,// Damage
        LEVEL_GOLD,// Level
         -1,// TurretType
          80,// Proba
           1500,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                25,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("golden axe",// Name
     m_IDs,// ID
      ITYPE_AXE,// ItemType
       0,// Damage
        LEVEL_GOLD,// Level
         -1,// TurretType
          80,// Proba
           30,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("golden pickaxe",// Name
     m_IDs,// ID
      ITYPE_PICKAXE,// ItemType
       0,// Damage
        LEVEL_GOLD,// Level
         -1,// TurretType
          90,// Proba
           1500,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                25,// Gold
                 0,// Diamond
                  0//Enegry
    );
    CreateItem("diamond sword",// Name
     m_IDs,// ID
      ITYPE_SWORD,// ItemType
       20,// Damage
        LEVEL_DIAMOND,// Level
         -1,// TurretType
        90,// Proba
           0,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 25,// Diamond
                  0//Enegry
    );
    CreateItem("diamond axe",// Name
     m_IDs,// ID
      ITYPE_AXE,// ItemType
       0,// Damage
        LEVEL_DIAMOND,// Level
         -1,// TurretType
          20,// Proba
           50,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 25,// Diamond
                  0//Enegry
    );
    CreateItem("diamond pickaxe",// Name
     m_IDs,// ID
      ITYPE_PICKAXE,// ItemType
       0,// Damage
        LEVEL_DIAMOND,// Level
         -1,// TurretType
            80,// Proba
           3500,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 25,// Diamond
                  0//Enegry
    );
    CreateItem("enegry sword",// Name
     m_IDs,// ID
      ITYPE_SWORD,// ItemType
       38,// Damage
        LEVEL_ENEGRY,// Level
         -1,// TurretType
          90,// Proba
           0,// Speed
            10,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 50,// Diamond
                  100//Enegry
    );
    CreateItem("enegry pickaxe",// Name
     m_IDs,// ID
      ITYPE_PICKAXE,// ItemType
       0,// Damage
        LEVEL_ENEGRY,// Level
         -1,// TurretType
          100,// Proba
           5000,// Speed
            5,// Log
             0,// Coal
              0,//Copper
               0,// Iron
                0,// Gold
                 1000,// Diamond
                  500//Enegry
    );
    
    CreateItem("gun turret", // Name
     m_IDs, // ID
      ITYPE_TURRET, // ItemType
       0, // Damage
        LEVEL_LOG, // Level
         TURRET_GUN, // TurretType
          90, // Proba
           0, // Speed
            20, // Log
             0, // Coal
              1, // Copper
               0, // Iron
                0, // Gold
                 0, // Diamond
                  0 // Enegry
    );

    CreateItem("shotgun turret", // Name
     m_IDs, // ID
      ITYPE_TURRET, // ItemType
       0, // Damage
        LEVEL_LOG, // Level
         TURRET_SHOTGUN, // TurretType
          90, // Proba
           0, // Speed
            25, // Log
             0, // Coal
              5, // Copper
               0, // Iron
                0, // Gold
                 0, // Diamond
                  0 // Enegry
    );

    CreateItem("laser turret", // Name
     m_IDs, // ID
      ITYPE_TURRET, // ItemType
       0, // Damage
        LEVEL_DIAMOND, // Level
         TURRET_LASER, // TurretType
          90, // Proba
           0, // Speed
            0, // Log
             0, // Coal
              0, // Copper
               0, // Iron
                20, // Gold
                 1, // Diamond
                  0 // Enegry
    );

    CreateItem("laser2077 turret", // Name
     m_IDs, // ID
      ITYPE_TURRET, // ItemType
       0, // Damage
        LEVEL_ENEGRY, // Level
         TURRET_LASER_2077, // TurretType
          90, // Proba
           0, // Speed
            20, // Log
             0, // Coal
              1, // Copper
               0, // Iron
                0, // Gold
                 50, // Diamond
                  20 // Enegry
    );

}

bool CItemSystem::CheckItemName(const char* pItemName)
{  
    for(int i=0;i < m_IDs; i++)
    {
        if(m_ItemList[i])
        {
            if(str_comp(m_ItemList[i]->m_Name, pItemName) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool CItemSystem::CreateItem(const char* pItemName, int ID, int Type, int Damage, int Level, int TurretType, int Proba, 
    int Speed, int Log, int Coal, int Copper, int Iron, int Gold, int Diamond, int Enegry)
{
    m_ItemList[ID] = new CItem(ID, Log, Coal, Copper, Iron, Gold, Diamond, Enegry);
    m_ItemList[ID]->m_Name = pItemName;
    m_ItemList[ID]->m_Level = Level;
    m_ItemList[ID]->m_Type = Type;
    m_ItemList[ID]->m_Damage = Damage;
    m_ItemList[ID]->m_TurretType = TurretType;
    m_ItemList[ID]->m_Proba = Proba;
    m_ItemList[ID]->m_Speed = Speed;
    dbg_msg("s","%d %d", Speed, m_IDs);
    m_IDs++;

    return true;
}

int CItemSystem::GetItemId(const char* pItemName)
{
    for(int i = 0; i < m_IDs; i++)
    {
        if(str_comp(m_ItemList[i]->m_Name, pItemName) == 0)
        {
            dbg_msg("s", "%d", i);
            return m_ItemList[i]->m_ID;
        }
    }
    return -1;
}

void CItemSystem::SendCantMakeItemChat(int To, int* Resource)
{
    std::string Buffer;
    char aBuf[128];
    Buffer.append("You need at least ");
    if(Resource[RESOURCE_LOG] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%s %d log", aBuf, Resource[RESOURCE_LOG]);
    }
    if(Resource[RESOURCE_COAL] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%s %d coal", aBuf, Resource[RESOURCE_COAL]);
    }
    if(Resource[RESOURCE_COPPER] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%s %d copper", aBuf, Resource[RESOURCE_COPPER]);
    }
    if(Resource[RESOURCE_IRON] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%s %d iron", aBuf, Resource[RESOURCE_IRON]);
    }
    if(Resource[RESOURCE_DIAMOND] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%s %d diamond", aBuf, Resource[RESOURCE_DIAMOND]);
    }
    if(Resource[RESOURCE_ENEGRY] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%s %d enegry", aBuf, Resource[RESOURCE_ENEGRY]);
    }
    Buffer.append(aBuf);
    m_pGameServer->SendChatTarget(To, Buffer.c_str());
}

void CItemSystem::SendMakeItemChat(int To, CItem *Item)
{
    std::string Buffer;

    Buffer.append("You made a ");
    Buffer.append(Item->m_Name);
    Buffer.append("!Good Job!");

    m_pGameServer->SendChatTarget(To, Buffer.c_str());
}

void CItemSystem::SendMakeItemFailedChat(int To, int* Resource)
{
    m_pGameServer->SendChatTarget(To, _("Bad luck. The production failed..."));
    std::string Buffer;
    char aBuf[128];
    Buffer.append("You lost ");
    CPlayer *p = m_pGameServer->m_apPlayers[To];
    if(Resource[RESOURCE_LOG] > 0)
    {
        p->m_Knapsack.m_Resource[RESOURCE_LOG]-=Resource[RESOURCE_LOG];
        str_format(aBuf, sizeof(aBuf), "%d log.", Resource[RESOURCE_LOG]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_COAL] > 0)
    {
        p->m_Knapsack.m_Resource[RESOURCE_COAL]-=Resource[RESOURCE_COAL];
        str_format(aBuf, sizeof(aBuf), "%d coal.", Resource[RESOURCE_COAL]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_COPPER] > 0)
    {
        p->m_Knapsack.m_Resource[RESOURCE_COPPER]-=Resource[RESOURCE_COPPER];
        str_format(aBuf, sizeof(aBuf), "%d copper.", Resource[RESOURCE_COPPER]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_IRON] > 0)
    {
        p->m_Knapsack.m_Resource[RESOURCE_IRON]-=Resource[RESOURCE_IRON];
        str_format(aBuf, sizeof(aBuf), "%d iron.", Resource[RESOURCE_IRON]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_DIAMOND] > 0)
    {
        p->m_Knapsack.m_Resource[RESOURCE_DIAMOND]-=Resource[RESOURCE_DIAMOND];
        str_format(aBuf, sizeof(aBuf), "%d diamond.", Resource[RESOURCE_DIAMOND]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_ENEGRY] > 0)
    {
        p->m_Knapsack.m_Resource[RESOURCE_DIAMOND]-=Resource[RESOURCE_DIAMOND];
        str_format(aBuf, sizeof(aBuf), "%d enegry.", Resource[RESOURCE_ENEGRY]);
        Buffer.append(aBuf);
    }
    m_pGameServer->SendChatTarget(To, Buffer.c_str());
}

void CItemSystem::MakeItem(const char* pItemName, int ClientID)
{
    if(!CheckItemName(pItemName))
    {
        m_pGameServer->SendChatTarget(ClientID, _("No such item."), NULL);
        return;
    }

    CItem *MakeItem = GetItem(GetItemId(pItemName));

    for(int i = 0;i < NUM_RESOURCE;i++)
    {
        if(m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Resource[i] < MakeItem->m_NeedResource[i])
        {
            SendCantMakeItemChat(ClientID, MakeItem->m_NeedResource);
            return;
        }
    }

    if(random_int(0, 100) < MakeItem->m_Proba)
    {
		SendMakeItemChat(ClientID, MakeItem);
        int ItemLevel = MakeItem->m_Level;
        dbg_msg("Type, Level","%d %d", MakeItem->m_Type, ItemLevel);
        switch (MakeItem->m_Type)
        {
            case ITYPE_AXE: 
                if(m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Axe < ItemLevel)
                    m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Axe = ItemLevel;
                break;
            
            case ITYPE_PICKAXE:
                if(m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Pickaxe < ItemLevel)
                   m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Pickaxe = ItemLevel;
                break;
            
            case ITYPE_SWORD:
                if(m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Sword < ItemLevel)
                    m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Sword = ItemLevel;
                break;
            
            case ITYPE_TURRET:
            {
                int Lifes;
                switch (MakeItem->m_TurretType)
                {
                case TURRET_GUN:
                    Lifes = 400;
                    break;
                
                case TURRET_SHOTGUN:
                    Lifes = 64;
                    break;
                
                case TURRET_LASER:
                    Lifes = 200;
                    break;
                
                case TURRET_LASER_2077:
                    Lifes = 350;
                    break;

                default:
                    break;
                }
                new CTurret(&m_pGameServer->m_World, m_pGameServer->GetPlayerChar(ClientID)->m_Pos, ClientID, MakeItem->m_TurretType, 64, Lifes);
            }
        }
    }else
    {
        SendMakeItemFailedChat(ClientID, MakeItem->m_NeedResource);
    }
}