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
		m_Health = 8000;
		break;
	
	case CK_COPPER:
		m_Health = 16000;
		break;

	case CK_IRON:
		m_Health = 40000;
		break;

	case CK_GOLD:
		m_Health = 60000;
		break;

	case CK_DIAMONAD:
		m_Health = 500000;
		break;

	case CK_ENEGRY:
		m_Health = 3000000;
		break;
	
	default:
		m_Health = 99999999;
		break;
	}

	m_LockPlayer = -1;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CKs::Reset()
{
	m_LockPlayer = -1;
}

void CKs::HandleLockPlayer()
{
	if(m_LockPlayer == -1)
		return;

	if(!GameServer()->GetPlayerChar(m_LockPlayer))
		return;

	if(!GameServer()->GetPlayerChar(m_LockPlayer)->IsAlive())
		return;

	else
	{
		GameServer()->GetPlayerChar(m_LockPlayer)->Teleport(m_Pos);
	}
}

void CKs::Tick()
{
	if(m_SpawnTick >= 0)
		m_SpawnTick--;
	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive() && !pChr->GetPlayer()->GetZomb())
	{
		/* NOW U CHANCE TO BE A [[BIG SHOT]]!         */
		/*           --------  Spamton G. Spamton   */
		
		
		/*If u have wooden pickaxe, u can mine all 'CKs' low then Iron(not include logs)*/
		/*          copper                                        Gold                  */
		/*          iron                                          Diamond               */
		/*          gold                                          NULL                  */

		int RespawnTime = -1;
		int PickSpeed = 1;
		
		if(GameServer()->GetPlayerChar(m_LockPlayer) && !GameServer()->GetPlayerChar(m_LockPlayer)->IsAlive())
			m_LockPlayer = 0;
		
		if(pChr->GetPlayer()->PressTab() && m_SpawnTick <= 0)
		{
			m_LockPlayer = pChr->GetCID();
			m_SpawnTick = 50;
		}

		if(m_LockPlayer >= 0 &&GameServer()->GetPlayerChar(m_LockPlayer) && GameServer()->GetPlayerChar(m_LockPlayer)->m_LatestInput.m_Jump)
		{
			m_LockPlayer = -1;
			m_SpawnTick = 50;
		}
		HandleLockPlayer();
		if(pChr->m_LatestInput.m_Fire&1&& pChr->m_ActiveWeapon == WEAPON_HAMMER && pChr->GetPlayer()->m_MiningTick <= 0)
		{
			if(pChr->GetPlayer()->m_Knapsack.m_Axe >= 0 && m_Type == CK_WOOD)
			{

				pChr->m_InMining = true;
				GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
				Picking(GameServer()->ItemSystem()->GetSpeed(pChr->GetPlayer()->m_Knapsack.m_Axe,ITYPE_AXE), pChr->GetPlayer());
				return;
			}
			else if(m_Type == CK_WOOD) 
			{
				Picking(8, pChr->GetPlayer()); // 8 16 24 32 40 48 56
				return;
			}
			else if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe >= 0)
			{
				if(m_Type == CK_ENEGRY)
					PickSpeed = GameServer()->ItemSystem()->GetSpeed(pChr->GetPlayer()->m_Knapsack.m_Pickaxe,ITYPE_PICKAXE) / 2;
				else
					PickSpeed = GameServer()->ItemSystem()->GetSpeed(pChr->GetPlayer()->m_Knapsack.m_Pickaxe,ITYPE_PICKAXE);
			}

			pChr->GetPlayer()->m_MiningType = m_Type;
			int CID = pChr->GetCID();
			switch (m_Type)
			{
				case CK_COAL:
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(PickSpeed, pChr->GetPlayer());
					break;
				case CK_IRON:
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(PickSpeed, pChr->GetPlayer());
					break;
				case CK_COPPER:
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(PickSpeed, pChr->GetPlayer());
					break;
				case CK_GOLD:
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(PickSpeed, pChr->GetPlayer());
					break;
				case CK_DIAMONAD:
					if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe < LEVEL_GOLD)
					{
						if(Server()->Tick()%100 == 0)
						{
							GameServer()->SendChatTarget(CID, _("You don't have a good pickaxe for Diamond"));
							GameServer()->SendChatTarget(CID, _("Make a Gold pickaxe first."));
						}
						return;
					}
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(PickSpeed, pChr->GetPlayer());
					break;
				case CK_ENEGRY:
					if(pChr->GetPlayer()->m_Knapsack.m_Pickaxe < LEVEL_DIAMOND)
					{
						if(Server()->Tick()%50 == 0)
						{
							GameServer()->SendChatTarget(CID, _("You need Diamond pickaxe for pick Enegry!"));
							GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
						}
						return;
					}
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(PickSpeed, pChr->GetPlayer());
					break;
				

				default:
					break;
			}
			
		}
	}
}

void CKs::Picking(int Time, CPlayer *Player)
{
	m_Health -= Time;
	int CID = Player->GetCID();
	if(m_Health <= 0)
	{
		switch (m_Type)
		{
		case CK_WOOD:
			Player->m_Knapsack.m_Resource[RESOURCE_LOG]++;
			GameServer()->SendChatTarget(CID, _("You picked up a Log"));
			m_Health = 50;
			break;
		case CK_COAL:
			Player->m_Knapsack.m_Resource[RESOURCE_COAL]++;
			GameServer()->SendChatTarget(CID, _("You picked up a Coal"));
			m_Health = 800;
			break;
		case CK_COPPER:
			Player->m_Knapsack.m_Resource[RESOURCE_COPPER]++;
			GameServer()->SendChatTarget(CID, _("You picked up a Copper"));
			m_Health = 1600;
			break;
		case CK_IRON:
			Player->m_Knapsack.m_Resource[RESOURCE_IRON]++;
			GameServer()->SendChatTarget(CID, _("You picked up a Iron"));
			m_Health = 4000;
			break;
		case CK_GOLD:
			Player->m_Knapsack.m_Resource[RESOURCE_GOLD]++;
			GameServer()->SendChatTarget(CID, _("You picked up a Gold"));
			m_Health = 6000;
			break;
		case CK_DIAMONAD:
			Player->m_Knapsack.m_Resource[RESOURCE_DIAMOND]++;
			GameServer()->SendChatTarget(CID, _("You picked up a Diamond"));
			m_Health = 10000;
			break;
		case CK_ENEGRY:
			Player->m_Knapsack.m_Resource[RESOURCE_ENEGRY]++;
			GameServer()->SendChatTarget(CID, _("You picked up a Enegry"));
			m_Health = 15000;
			break;
		
		default:
			break;
		}
		GameServer()->Sql()->update(CID);
	}
	GameServer()->SendBroadcast_VL(_("{int:Health} left. Keep hit!"), CID, "Health", &m_Health);
	Player->m_MiningTick = 25;
}

void CKs::TickPaused()
{
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
