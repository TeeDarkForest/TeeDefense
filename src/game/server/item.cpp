/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/math.h>
#include <string.h>
#include "item.h"
#include "gamecontext.h"
#include "gamecontroller.h"

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
    CreateItem("logaxe", m_IDs, ITYPE_AXE, 0, LEVEL_LOG, -1, 20, 10, 10, 0, 0, 0, 0, 0, 0);
    CreateItem("logpickaxe", m_IDs, ITYPE_PICKAXE, 0, LEVEL_LOG, -1, 20, 500, 25, 0, 0, 0, 0, 0, 0);
}

bool CItemSystem::CheckItemName(const char* pItemName)
{
    for(int i=0;i < m_IDs; i++)
    {
        if(str_comp(CItemSystem::m_ItemList[i].m_Name, pItemName) == 0)
        {
            return true;
        }
    }
    return false;
}

bool CItemSystem::CreateItem(const char* pItemName, int ID, int Type, int Damage, int Level, int TurretType, int Proba, 
    int Speed, int Log, int Coal, int Copper, int Iron, int Gold, int Diamond, int Enegry)
{
    if(CheckItemName(pItemName))
    {
        m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ItemSystem", "Can't create item, because name is found.");
        return false;
    }
    CItem *pItem = new CItem();
    pItem->Load(ID, Log, Coal, Copper, Iron, Gold, Diamond, Enegry);
    pItem->m_Name = pItemName;
    pItem->m_Level = Level;
    pItem->m_Type = Type;
    pItem->m_Damage = Damage;
    pItem->m_TurretType = TurretType;
    pItem->m_Proba = Proba;
    pItem->m_Speed = Speed;
    m_ItemList[ID] = *pItem;
    m_IDs++;

    return true;
}

int CItemSystem::GetItemId(const char* pItemName)
{
    for(int i = 0; i < m_IDs; i++)
    {
        if(str_comp(m_ItemList[i].m_Name, pItemName))
        {
            return m_ItemList[i].m_ID;
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

void CItemSystem::SendMakeItemChat(int To, CItem Item)
{
    std::string Buffer;

    Buffer.append("You made a ");
    Buffer.append(Item.m_Name);
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

void CItemSystem::MakeItem(const char* pItemName, int ClientID)
{
    if(!CheckItemName(pItemName))
    {
        m_pGameServer->SendChatTarget(ClientID, _("No such item."), NULL);
    }

    CItem MakeItem = GetItem(GetItemId(pItemName));

    for(int i = 0;i < NUM_RESOURCE;i++)
    {
        if(m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Resource[i] < MakeItem.m_NeedResource[i])
        {
            SendCantMakeItemChat(ClientID, MakeItem.m_NeedResource);
            return;
        }
    }

    if(random_int(0, 100) < MakeItem.m_Proba)
    {
		SendMakeItemChat(ClientID, MakeItem);
        int ItemId = MakeItem.m_ID;
        switch (MakeItem.m_Type)
        {
            case ITYPE_AXE: m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Axe = ItemId;break;
            case ITYPE_PICKAXE: m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Pickaxe = ItemId;break;
            case ITYPE_SWORD: m_pGameServer->m_apPlayers[ClientID]->m_Knapsack.m_Sword = ItemId;break;
        }
    }else
    {
        SendMakeItemFailedChat(ClientID, MakeItem.m_NeedResource);
    }
}