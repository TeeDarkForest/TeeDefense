/* (c) FlowerFell-Sans. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                 */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "CKs.h"

CKs::CKs(CGameWorld *pGameWorld, int Type, vec2 Pos, int SubType)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = Type;
	m_Subtype = SubType;
	m_ProximityRadius = PhysSize;
	m_Pos = Pos;

	switch (m_Type)
	{
	case CK_WOOD:
		m_Health = 50;
		break;
	
	case CK_COAL:
		m_Health = 70;
		break;
	
	default:
		m_Health = 10;
		break;
	}

	Reset();

	GameWorld()->InsertEntity(this);
}

void CKs::Reset()
{
	if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else
		m_SpawnTick = -1;
}

void CKs::Tick()
{
	// wait for respawn
	if(m_SpawnTick > 0)
	{
		if(Server()->Tick() > m_SpawnTick)
		{
			// respawn
			m_SpawnTick = -1;

			if(m_Type == POWERUP_WEAPON)
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN);
		}
		else
			return;
	}
	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive() && !pChr->GetPlayer()->GetZomb())
	{
		/* NOW U CHANCE TO BE [[BIG SHOT]]!         */
		/*           --------  Spamton G. Spamton   */
		int RespawnTime = -1;
		int BIGSHOT = 1;
		if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe[CK_WOOD] && m_Type < CK_IRON)
			BIGSHOT = 10;
		if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe[CK_COPPER] && m_Type < CK_GOLD)
			BIGSHOT = 20;
		if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe[CK_IRON] && m_Type < CK_DIAMONAD)
			BIGSHOT = 20;
		if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe[CK_GOLD])
		{
			if(m_Type < CK_DIAMONAD)
				BIGSHOT = 10;
			else
				BIGSHOT = 30;
		}
		if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe[CK_DIAMONAD])
			BIGSHOT = 50;

		if(pChr->m_LatestInput.m_Fire&1 && pChr->m_ActiveWeapon == WEAPON_HAMMER && pChr->GetPlayer()->m_MiningTick <= 0)
		{
			pChr->GetPlayer()->m_MiningType = m_Type;
			int CID = pChr->GetCID();
			switch (m_Type)
			{
				case CK_WOOD:
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					if(pChr->GetPlayer()->m_Knapsack.m_Axe[CK_DIAMONAD])
						Picking(50, pChr->GetPlayer());
					else if(pChr->GetPlayer()->m_Knapsack.m_Axe[CK_GOLD])
						Picking(30, pChr->GetPlayer());
					else if(pChr->GetPlayer()->m_Knapsack.m_Axe[CK_IRON])
						Picking(30, pChr->GetPlayer()); // 30 60
					else if(pChr->GetPlayer()->m_Knapsack.m_Axe[CK_COPPER])
						Picking(17, pChr->GetPlayer()); // 17 34 51
					else if(pChr->GetPlayer()->m_Knapsack.m_Axe[CK_WOOD])
						Picking(10, pChr->GetPlayer()); // 15 30 45 55
					else
						Picking(8, pChr->GetPlayer()); // 8 16 24 32 40 48 56
					break;

				case CK_COAL:
				case CK_IRON:
				case CK_COPPER:
				case CK_GOLD:
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(BIGSHOT, pChr->GetPlayer());
					break;

				case CK_DIAMONAD:
					if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe[4] && !pChr->GetPlayer()->m_Knapsack.m_Pickaxe[5])
					{
						GameServer()->SendChatTarget(CID, _("You don't have a good pickaxe for Diamond"));
						GameServer()->SendChatTarget(CID, _("Make a Iron pickaxe or Diamond pickaxe first."));
						return;
					}
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(BIGSHOT, pChr->GetPlayer());
					break;
				

				default:
					break;
			};
		}
	}
}

void CKs::Picking(int Time, CPlayer *Player)
{
	m_Health -= Time;
	int CID = Player->GetCID();
	if(m_Health <= 0)
	{
		switch (Player->m_MiningType)
		{
		case CK_WOOD:
			Player->m_Knapsack.m_Log++;
			GameServer()->SendChatTarget(CID, _("You picked up a Log"));
			m_Health = 50;
			break;
		case CK_COAL:
			Player->m_Knapsack.m_Coal++;
			GameServer()->SendChatTarget(CID, _("You picked up a Coal"));
			m_Health = 70;
			break;
		case CK_COPPER:
			Player->m_Knapsack.m_Copper++;
			GameServer()->SendChatTarget(CID, _("You picked up a Copper"));
			break;
		case CK_IRON:
			Player->m_Knapsack.m_Iron++;
			GameServer()->SendChatTarget(CID, _("You picked up a Iron"));
			break;
		case CK_GOLD:
			Player->m_Knapsack.m_Gold++;
			GameServer()->SendChatTarget(CID, _("You picked up a Gold (damn why when I created this mode just make gold important, in Minecraft it just a piece of sXXt!!!!!!!!!)"));
			break;
		case CK_DIAMONAD:
			Player->m_Knapsack.m_Diamond++;
			GameServer()->SendChatTarget(CID, _("OMG DIAMONAD! U PICKED UP A DIAMONAD!!!!!"));
			break;
		
		default:
			break;
		}
	}
	Player->m_MiningTick = 25;
}

void CKs::TickPaused()
{
	if(m_SpawnTick != -1)
		++m_SpawnTick;
}

void CKs::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	if(m_Type == CK_WOOD)
		pP->m_Type = POWERUP_HEALTH;
	else
		pP->m_Type = POWERUP_ARMOR;
	pP->m_Subtype = 0;
}
