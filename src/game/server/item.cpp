/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/math.h>
#include <string.h>
#include "item.h"
#include "gamecontext.h"
#include "gamecontroller.h"
#include "entities/turret.h"

void CItem::Load(int ID, int Log, int Coal, int Copper, int Iron, int Gold, int Diamond, int Enegry)
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

CItemSystem::CItemSystem()
{
    m_IDs = 0;
    // unfinish. ED ni lai gao zhe ge.
    CreateItem("log_sword", m_IDs, ITYPE_SWORD, 0, LEVEL_LOG, -1, 20, 10, 10, 0, 0, 0, 0, 0, 0);
    CreateItem("log_axe", m_IDs, ITYPE_AXE, 0, LEVEL_LOG, -1, 20, 10, 10, 0, 0, 0, 0, 0, 0);
    CreateItem("log_pickaxe", m_IDs, ITYPE_PICKAXE, 0, LEVEL_LOG, -1, 20, 500, 25, 0, 0, 0, 0, 0, 0);
    CreateItem("copper_sword", m_IDs, ITYPE_SWORD, 0, LEVEL_COPPER, -1, 20, 10, 10, 0, 25, 0, 0, 0, 0);
    CreateItem("copper_axe", m_IDs, ITYPE_AXE, 0, LEVEL_COPPER, -1, 20, 10, 10, 0, 0, 0, 0, 0, 0);
    CreateItem("copper_pickaxe", m_IDs, ITYPE_PICKAXE, 0, LEVEL_COPPER, -1, 20, 500, 25, 0, 0, 0, 0, 0, 0);
    CreateItem("iron_sword", m_IDs, ITYPE_SWORD, 0, LEVEL_IRON, -1, 20, 10, 10, 0, 0, 0, 0, 0, 0);
    CreateItem("iron_axe", m_IDs, ITYPE_AXE, 0, LEVEL_IRON, -1, 20, 10, 10, 0, 0, 0, 0, 0, 0);
    CreateItem("iron_pickaxe", m_IDs, ITYPE_PICKAXE, 0, LEVEL_IRON, -1, 20, 500, 25, 0, 0, 0, 0, 0, 0);
    
    CreateItem("gun_turret", m_IDs, ITYPE_TURRET, 0, LEVEL_LOG, TURRET_GUN, 20, 500, 25, 0, 0, 0, 0, 0, 0);
    
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
    m_ItemList[ID] = new CItem();
    m_ItemList[ID]->Load(ID, Log, Coal, Copper, Iron, Gold, Diamond, Enegry);
    m_ItemList[ID]->m_Name = pItemName;
    m_ItemList[ID]->m_Level = Level;
    m_ItemList[ID]->m_Type = Type;
    m_ItemList[ID]->m_Damage = Damage;
    m_ItemList[ID]->m_TurretType = TurretType;
    m_ItemList[ID]->m_Proba = Proba;
    m_ItemList[ID]->m_Speed = Speed;
    m_IDs++;

    return true;
}

int CItemSystem::GetItemId(const char* pItemName)
{
    for(int i = 0; i < m_IDs; i++)
    {
        if(str_comp(m_ItemList[i]->m_Name, pItemName))
        {
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
    Buffer.append("Good Job!");

    m_pGameServer->SendChatTarget(To, Buffer.c_str());
}

void CItemSystem::SendMakeItemFailedChat(int To, int* Resource)
{
    m_pGameServer->SendChatTarget(To, _("Bad luck. The production failed..."));
    std::string Buffer;
    char aBuf[128];
    Buffer.append("You lost ");
    if(Resource[RESOURCE_LOG] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%d log.", Resource[RESOURCE_LOG]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_COAL] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%d coal.", Resource[RESOURCE_COAL]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_COPPER] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%d copper.", Resource[RESOURCE_COPPER]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_IRON] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%d iron.", Resource[RESOURCE_IRON]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_DIAMOND] > 0)
    {
        str_format(aBuf, sizeof(aBuf), "%d diamond.", Resource[RESOURCE_DIAMOND]);
        Buffer.append(aBuf);
    }
    if(Resource[RESOURCE_ENEGRY] > 0)
    {
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
    }

    CItem *MakeItem = GetItem(GetItemId(pItemName));

    for(int i = 0;i < NUM_RESOURCE;i++)
    {
        if(MakeItem->m_NeedResource[i] && m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Resource[i])
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