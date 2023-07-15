/* (c) FlowerFell-Sans. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                 */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "CKs.h"

CKs::CKs(CGameWorld *pGameWorld, int Type, vec2 Pos, int ID, int SubType)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = Type;
	m_Subtype = SubType;
	m_ProximityRadius = PhysSize;
	m_Pos = Pos;

	m_Health = GetMaxHealth(m_Type);

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
		/*           --------  Spamton G. Spamton     */
		// BE A BIG  BE A BIG  BE A [[BIGEST SHOT]]   //
		int RespawnTime = -1;
		int PickSpeed = 1;
		
		if(GameServer()->GetPlayerChar(m_LockPlayer) && !GameServer()->GetPlayerChar(m_LockPlayer)->IsAlive())
			m_LockPlayer = 0;
		
		if(pChr->GetPlayer()->PressTab() && m_SpawnTick <= 0)
		{
			m_LockPlayer = pChr->GetCID();
			m_SpawnTick = 50;
		}

		if(m_LockPlayer >= 0 && GameServer()->GetPlayerChar(m_LockPlayer) && GameServer()->GetPlayerChar(m_LockPlayer)->m_LatestInput.m_Jump)
		{
			GameServer()->GetPlayerChar(m_LockPlayer);
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
				Picking(GameServer()->GetSpeed(pChr->GetPlayer()->m_Knapsack.m_Axe,ITYPE_AXE), pChr->GetPlayer());
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
					PickSpeed = GameServer()->GetSpeed(pChr->GetPlayer()->m_Knapsack.m_Pickaxe,ITYPE_PICKAXE) / 2;
				else
					PickSpeed = GameServer()->GetSpeed(pChr->GetPlayer()->m_Knapsack.m_Pickaxe,ITYPE_PICKAXE);
			}

			pChr->GetPlayer()->m_MiningType = m_Type;
			int CID = pChr->GetCID();
			switch (m_Type)
			{
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
					pChr->m_InMining = true;
					GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
					Picking(PickSpeed, pChr->GetPlayer());
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
		if(Player->LoggedIn)
		{
			GameServer()->Sql()->UpdateCK(Player->GetCID(), GetRealNameByType(m_Type), "+1");
		}
		else
			Player->m_Knapsack.m_Resource[GetResourceID(m_Type)]++;
		dynamic_string buf;
		Server()->Localization()->Format_L(buf, Player->GetLanguage(), _(GetNameByType(m_Type)));
		GameServer()->SendChatTarget(CID, _("You picked up a {str:Resource}"), "Resource", buf.buffer());
		m_Health = GetMaxHealth(m_Type);
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



int CKs::GetMaxHealth(int Type)
{
	switch (Type)
	{
	case CK_WOOD:
		return 50;
		break;
	
	case CK_COAL:
		return 8000;
		break;

	case CK_COPPER:
		return 1600;
		break;
	
	case CK_IRON:
		return 4000;
		break;
	
	case CK_GOLD:
		return 6000;
		break;
	
	case CK_DIAMONAD:
		return 60000;
		break;
	
	case CK_ENEGRY:
		return 3000000;
		break;
	
	case CK_Abyss_Agar:
		return 500000;
		break;
	
	case CK_Abyss_LEnegry:
		return 500000;
		break;
	
	case CK_Abyss_ScrapMetal:
		return 3000000;
		break;
	
	case CK_Abyss_ScrapMetal_S:
		return 4500000;
		break;
	
	case CK_Abyss_NuclearWaste_S:
		return 4250000;
		break;

	case CK_Abyss_Remnant:
		return 5000000;
		break;
	
	case CK_Abyss_ConstantFragment:
		return 25;
		break;

	case CK_Abyss_DeathAgglomerate:
		return 70000000;
		break;
	
	case CK_Abyss_DeepPrism:
		return 65000000;
		break;

	case CK_Abyss_PlatinumWildColor:
		return 10000000;
		break;
	
	default:
		return 99999999999999;
		break;
	}
}

int CKs::GetResourceID(int Type)
{
	switch (Type)
	{
	case CK_WOOD:
		return RESOURCE_LOG;
		break;
	
	case CK_COAL:
		return RESOURCE_COAL;
		break;

	case CK_COPPER:
		return RESOURCE_COPPER;
		break;
	
	case CK_IRON:
		return RESOURCE_IRON;
		break;
	
	case CK_GOLD:
		return RESOURCE_GOLD;
		break;
	
	case CK_DIAMONAD:
		return RESOURCE_DIAMOND;
		break;
	
	case CK_ENEGRY:
		return RESOURCE_ENEGRY;
		break;
	
	case CK_Abyss_Agar:
		return Abyss_Agar;
		break;
	
	case CK_Abyss_LEnegry:
		return Abyss_LEnegry;
		break;
	
	case CK_Abyss_ScrapMetal:
		return Abyss_ScrapMetal;
		break;
	
	case CK_Abyss_ScrapMetal_S:
		return Abyss_ScrapMetal_S;
		break;
	
	case CK_Abyss_NuclearWaste_S:
		return Abyss_NuclearWaste_S;
		break;

	case CK_Abyss_Remnant:
		return Abyss_Remnant;
		break;

	case CK_Abyss_ConstantFragment:
		return Abyss_ConstantFragment;
		break;

	case CK_Abyss_DeathAgglomerate:
		return Abyss_DeathAgglomerate;
		break;
	
	case CK_Abyss_DeepPrism:
		return Abyss_Prism;
		break;

	case CK_Abyss_PlatinumWildColor:
		return Abyss_PlatinumWildColor;
		break;
	
	default:
		return RESOURCE_LOG;
		break;
	}
}

const char* CKs::GetNameByType(int Type)
{
	switch (Type)
	{
	case CK_WOOD:
		return "Log";
		break;
	
	case CK_COAL:
		return "Coal";
		break;

	case CK_COPPER:
		return "Copper";
		break;
	
	case CK_IRON:
		return "Iron";
		break;
	
	case CK_GOLD:
		return "Gold";
		break;
	
	case CK_DIAMONAD:
		return "Diamond";
		break;
	
	case CK_ENEGRY:
		return "Enegry";
		break;
	
	case CK_Abyss_Agar:
		return "Agar";
		break;
	
	case CK_Abyss_LEnegry:
		return "Light Enegry";
		break;
	
	case CK_Abyss_ScrapMetal:
		return "Scrap Metal";
		break;
	
	case CK_Abyss_ScrapMetal_S:
		return "Slag Scrap Matal";
		break;
	
	case CK_Abyss_NuclearWaste_S:
		return "Slag Nuclear Waste";
		break;

	case CK_Abyss_Remnant:
		return "Remnant";
		break;
	
	case CK_Abyss_ConstantFragment:
		return "Contant Fragment";
		break;

	case CK_Abyss_DeathAgglomerate:
		return "Death Agglomerate";
		break;
	
	case CK_Abyss_DeepPrism:
		return "Abyss Prism";
		break;

	case CK_Abyss_PlatinumWildColor:
		return "Platinum Wild Color";
		break;

	default:
		return "SB";
		break;
	}
}

const char* CKs::GetRealNameByType(int Type)
{
	switch (Type)
	{
	case CK_WOOD:
		return "Log";
		break;
	
	case CK_COAL:
		return "Coal";
		break;

	case CK_COPPER:
		return "Copper";
		break;
	
	case CK_IRON:
		return "Iron";
		break;
	
	case CK_GOLD:
		return "Gold";
		break;
	
	case CK_DIAMONAD:
		return "Diamond";
		break;
	
	case CK_ENEGRY:
		return "Enegry";
		break;
	
	case CK_Abyss_Agar:
		return "Agar";
		break;
	
	case CK_Abyss_LEnegry:
		return "LightEnegry";
		break;
	
	case CK_Abyss_ScrapMetal:
		return "ScrapMetal";
		break;
	
	case CK_Abyss_ScrapMetal_S:
		return "SlagScrapMatal";
		break;
	
	case CK_Abyss_NuclearWaste_S:
		return "SlagNuclearWaste";
		break;

	case CK_Abyss_Remnant:
		return "Remnant";
		break;

	case CK_Abyss_ConstantFragment:
		return "ContantFragment";
		break;

	case CK_Abyss_DeathAgglomerate:
		return "DeathAgglomerate";
		break;
	
	case CK_Abyss_DeepPrism:
		return "Prism";
		break;

	case CK_Abyss_PlatinumWildColor:
		return "PlatinumWildColor";
		break;
	
	default:
		return "SB";
		break;
	}
}