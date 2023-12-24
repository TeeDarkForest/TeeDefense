/* (c) FlowerFell-Sans. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                 */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "CKs.h"

#include <game/server/GameCore/Account/account.h>

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
	if (m_LockPlayer == -1)
		return;

	if (!GameServer()->GetPlayerChar(m_LockPlayer))
		return;

	if (!GameServer()->GetPlayerChar(m_LockPlayer)->IsAlive())
		return;

	else
	{
		GameServer()->GetPlayerChar(m_LockPlayer)->Teleport(m_Pos);
	}
}

void CKs::Tick()
{
	if (m_SpawnTick >= 0)
		m_SpawnTick--;
	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if (pChr && pChr->IsAlive() && !pChr->GetPlayer()->GetZomb())
	{
		// DUDE! Forget the old comment here
		int RespawnTime = -1;
		int PickSpeed = 1;

		CPlayer *pP = GameServer()->GetPlayer(pChr->GetCID(), false, true);
		if (GameServer()->GetPlayerChar(m_LockPlayer) && !GameServer()->GetPlayerChar(m_LockPlayer)->IsAlive())
		{
			pP->m_LockedCK = false;
			m_LockPlayer = 0;
		}
		if (pP->PressTab() && m_SpawnTick <= 0 && !pP->m_LockedCK)
		{
			pP->m_LockedCK = true;
			m_LockPlayer = pChr->GetCID();
			m_SpawnTick = 50;
		}

		if (m_LockPlayer >= 0 && GameServer()->GetPlayerChar(m_LockPlayer) && GameServer()->GetPlayerChar(m_LockPlayer)->m_LatestInput.m_Jump)
		{
			pP->m_LockedCK = false;
			GameServer()->GetPlayerChar(m_LockPlayer);
			m_LockPlayer = -1;
			m_SpawnTick = 50;
		}
		HandleLockPlayer();
		if (pChr->m_LatestInput.m_Fire & 1 && pChr->m_ActiveWeapon == WEAPON_HAMMER && pChr->GetPlayer()->m_MiningTick <= 0)
		{
			if (pChr->GetPlayer()->m_Holding[ITYPE_AXE] >= 0 && m_Type == ITEM_LOG)
			{

				pChr->m_InMining = true;
				GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
				Picking(GameServer()->GetSpeed(pChr->GetPlayer()->m_Holding[ITYPE_AXE], ITYPE_AXE), pChr->GetPlayer());
				return;
			}
			else if (m_Type == ITEM_LOG)
			{
				Picking(8, pChr->GetPlayer()); // 8 16 24 32 40 48 56
				return;
			}
			else if (pChr->GetPlayer()->m_Holding[ITYPE_PICKAXE] >= 0)
			{
				if (m_Type == ITEM_ENEGRY)
					PickSpeed = GameServer()->GetSpeed(pChr->GetPlayer()->m_Holding[ITYPE_PICKAXE], ITYPE_PICKAXE) / 2;
				else
					PickSpeed = GameServer()->GetSpeed(pChr->GetPlayer()->m_Holding[ITYPE_PICKAXE], ITYPE_PICKAXE);
			}

			pChr->GetPlayer()->m_MiningType = m_Type;
			
			pChr->m_InMining = true;
			GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
			Picking(PickSpeed, pChr->GetPlayer());
		}
	}
}

void CKs::Picking(int Time, CPlayer *Player)
{
	m_Health -= Time;
	int CID = Player->GetCID();
	dynamic_string buf;
	Server()->Localization()->Format_L(buf, Player->GetLanguage(), _(GameServer()->m_Items[m_Type].m_Name));
	if (m_Health <= 0)
	{
		Player->m_Items[m_Type]++;
		GameServer()->SendChatTarget(CID, _("You picked up a {str:Resource}"), "Resource", buf.buffer());
		m_Health = GetMaxHealth(m_Type);
		GameServer()->ClearVotes(CID);
	}
	dynamic_string buffer;
	dynamic_string buffre;
	Server()->Localization()->Format_L(buffer, Player->GetLanguage(), _("- Picking: {str:item} -\n"), "item", buf.buffer());
	Server()->Localization()->Format_L(buffre, Player->GetLanguage(), _("{int:Health} left. Keep hit!"), "Health", &m_Health);
	buffer.append(buffre);
	GameServer()->SendBroadcast(buffer.buffer(), CID);
	Player->m_MiningTick = 25;
}

void CKs::TickPaused()
{
}

void CKs::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if (!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	if (m_Type == ITEM_LOG)
		pP->m_Type = POWERUP_HEALTH;
	else
		pP->m_Type = POWERUP_ARMOR;
	pP->m_Subtype = 0;
}

int CKs::GetMaxHealth(int Type)
{
	switch (Type)
	{
	case ITEM_LOG:
		return 50;
		break;

	case ITEM_COAL:
		return 8000;
		break;

	case ITEM_COPPER:
		return 2000;
		break;

	case ITEM_IRON:
		return 8000;
		break;

	case ITEM_GOLDEN:
		return 30000;
		break;

	case ITEM_DIAMOND:
		return 1200000;
		break;

	case ITEM_ENEGRY:
		return 3000000;
		break;

	default:
		return 99999999999999;
		break;
	}
}