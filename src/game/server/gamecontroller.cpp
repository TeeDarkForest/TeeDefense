/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/mapitems.h>

#include <game/generated/protocol.h>

#include "entities/pickup.h"
#include "entities/CKs.h"
#include <game/server/entities/tower-main.h>
#include "gamecontroller.h"
#include "gamecontext.h"
#include <base/math.h>
#include "entities/turret.h"

#include "GameCore/Account/account.h"

CGameController::CGameController(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
	m_pGameType = "TeeDefense Time";

	//
	DoWarmup(g_Config.m_SvWarmup);
	m_GameOverTick = -1;
	m_SuddenDeath = 0;
	m_RoundStartTick = Server()->Tick();
	m_RoundCount = 0;
	m_GameFlags = 0;
	m_aTeamscore[TEAM_HUMAN] = 0;
	m_aTeamscore[TEAM_ZOMBIE] = g_Config.m_SvLives;
	m_aMapWish[0] = 0;

	m_UnbalancedTick = -1;
	m_ForceBalanced = false;

	m_aNumSpawnPoints[0] = 0;
	m_aNumSpawnPoints[1] = 0;
	m_aNumSpawnPoints[2] = 0;

	m_CKsID = 0;
	// Zomb2
	//	m_pTop = new CTop(m_pGameServer);
	mem_zero(m_Zombie, sizeof(m_Zombie));

	m_ZombLeft = 0;
	StartWave();
}

CGameController::~CGameController()
{
}

float CGameController::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos)
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for (; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// team mates are not as dangerous as enemies
		float Scoremod = 1.0f;
		if (pEval->m_FriendlyTeam != -1 && pC->GetPlayer()->GetTeam() == pEval->m_FriendlyTeam)
			Scoremod = 0.5f;

		float d = distance(Pos, pC->m_Pos);
		Score += Scoremod * (d == 0 ? 1000000000.0f : 1.0f / d);
	}

	return Score;
}

void CGameController::EvaluateSpawnType(CSpawnEval *pEval, int Type)
{
	// get spawn point
	for (int i = 0; i < m_aNumSpawnPoints[Type]; i++)
	{
		// check if the position is occupado
		CCharacter *aEnts[MAX_CLIENTS];
		int Num = GameServer()->m_World.FindEntities(m_aaSpawnPoints[Type][i], 64, (CEntity **)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		vec2 Positions[5] = {vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f)}; // start, left, up, right, down
		int Result = -1;
		for (int Index = 0; Index < 5 && Result == -1; ++Index)
		{
			Result = Index;
			for (int c = 0; c < Num; ++c)
				if (GameServer()->Collision()->CheckPoint(m_aaSpawnPoints[Type][i] + Positions[Index]) ||
					distance(aEnts[c]->m_Pos, m_aaSpawnPoints[Type][i] + Positions[Index]) <= aEnts[c]->m_ProximityRadius)
				{
					Result = -1;
					break;
				}
		}
		if (Result == -1)
			continue; // try next spawn point

		vec2 P = m_aaSpawnPoints[Type][i] + Positions[Result];
		float S = EvaluateSpawnPos(pEval, P);
		if (!pEval->m_Got || pEval->m_Score > S)
		{
			pEval->m_Got = true;
			pEval->m_Score = S;
			pEval->m_Pos = P;
		}
	}
}

bool CGameController::CanSpawn(int Team, vec2 *pOutPos, bool Boss)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if (Team == TEAM_SPECTATORS)
		return false;

	if (Team == TEAM_HUMAN)
		EvaluateSpawnType(&Eval, 1);
	else if (Boss)
		EvaluateSpawnType(&Eval, 0);
	else
		EvaluateSpawnType(&Eval, 2);

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

bool CGameController::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;

	switch (Index)
	{
	case ENTITY_SPAWN_ANY:
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
		break;
	case ENTITY_SPAWN_HUMAN:
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
		break;
	case ENTITY_SPAWN_ZOMBIE:
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
		break;
	case ENTITY_ARMOR_1:
		Type = POWERUP_ARMOR;
		break;
	case ENTITY_HEALTH_1:
		Type = POWERUP_HEALTH;
		break;
	case ENTITY_WEAPON_SHOTGUN:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
		break;
	case ENTITY_WEAPON_GRENADE:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
		break;
	case ENTITY_WEAPON_RIFLE:
		Type = POWERUP_WEAPON;
		SubType = WEAPON_RIFLE;
		break;
	case ENTITY_POWERUP_NINJA:
		if (g_Config.m_SvPowerups)
		{
			Type = POWERUP_NINJA;
			SubType = WEAPON_NINJA;
		}
		break;
	case ENTITY_LOG:
		new CKs(&GameServer()->m_World, ITEM_LOG, Pos, m_CKsID);
		m_CKsID++;
		break;
	case ENTITY_COAL:
		new CKs(&GameServer()->m_World, ITEM_COAL, Pos, m_CKsID);
		m_CKsID++;
		break;
	case ENTITY_COPPER:
		new CKs(&GameServer()->m_World, ITEM_COPPER, Pos, m_CKsID);
		m_CKsID++;
		break;
	case ENTITY_IRON:
		new CKs(&GameServer()->m_World, ITEM_IRON, Pos, m_CKsID);
		m_CKsID++;
		break;
	case ENTITY_GOLD:
		new CKs(&GameServer()->m_World, ITEM_GOLDEN, Pos, m_CKsID);
		m_CKsID++;
		break;
	case ENTITY_DIAMOND:
		new CKs(&GameServer()->m_World, ITEM_DIAMOND, Pos, m_CKsID);
		m_CKsID++;
		break;
	case ENTITY_ENERGY:
		new CKs(&GameServer()->m_World, ITEM_ENEGRY, Pos, m_CKsID);
		m_CKsID++;
		break;
	case ENTITY_MAIN_TOWER:
		new CTowerMain(&GameServer()->m_World, Pos);
		break;

	default:
		break;
	}
	if (Type != -1)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, SubType);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

void CGameController::EndRound()
{
	if (m_Warmup) // game can't end when we are running warmup
		return;

	HandleTop();
	GameServer()->m_World.m_Paused = true;
	m_GameOverTick = Server()->Tick();
	m_SuddenDeath = 0;
	ResetGame();
	GameServer()->m_NeedResetTowers = true;
	mem_zero(m_Zombie, sizeof(m_Zombie));
}

void CGameController::ResetGame()
{
	GameServer()->m_World.m_ResetRequested = true;
}

const char *CGameController::GetTeamName(int Team)
{
	if (Team == TEAM_HUMAN)
		return "humans";
	else if (Team == TEAM_ZOMBIE)
		return "zombies";
	return "spectators";
}

static bool IsSeparator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void CGameController::StartRound()
{
	// ResetGame();

	m_RoundStartTick = Server()->Tick();
	m_SuddenDeath = 0;
	m_GameOverTick = -1;
	m_ForceBalanced = false;
	Server()->DemoRecorder_HandleAutoStart();
}

void CGameController::ChangeMap(const char *pToMap)
{
	str_copy(m_aMapWish, pToMap, sizeof(m_aMapWish));
	EndRound();
}

void CGameController::CycleMap()
{
	if (m_aMapWish[0] != 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "rotating map to %s", m_aMapWish);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		str_copy(g_Config.m_SvMap, m_aMapWish, sizeof(g_Config.m_SvMap));
		m_aMapWish[0] = 0;
		m_RoundCount = 0;
		return;
	}
	if (!str_length(g_Config.m_SvMaprotation))
		return;

	// handle maprotation
	const char *pMapRotation = g_Config.m_SvMaprotation;
	const char *pCurrentMap = g_Config.m_SvMap;

	int CurrentMapLen = str_length(pCurrentMap);
	const char *pNextMap = pMapRotation;
	while (*pNextMap)
	{
		int WordLen = 0;
		while (pNextMap[WordLen] && !IsSeparator(pNextMap[WordLen]))
			WordLen++;

		if (WordLen == CurrentMapLen && str_comp_num(pNextMap, pCurrentMap, CurrentMapLen) == 0)
		{
			// map found
			pNextMap += CurrentMapLen;
			while (*pNextMap && IsSeparator(*pNextMap))
				pNextMap++;

			break;
		}

		pNextMap++;
	}

	// restart rotation
	if (pNextMap[0] == 0)
		pNextMap = pMapRotation;

	// cut out the next map
	char aBuf[512] = {0};
	for (int i = 0; i < 511; i++)
	{
		aBuf[i] = pNextMap[i];
		if (IsSeparator(pNextMap[i]) || pNextMap[i] == 0)
		{
			aBuf[i] = 0;
			break;
		}
	}

	// skip spaces
	int i = 0;
	while (IsSeparator(aBuf[i]))
		i++;

	m_RoundCount = 0;

	char aBufMsg[256];
	str_format(aBufMsg, sizeof(aBufMsg), "rotating map to %s", &aBuf[i]);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	str_copy(g_Config.m_SvMap, &aBuf[i], sizeof(g_Config.m_SvMap));
}

void CGameController::PostReset()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			GameServer()->m_apPlayers[i]->m_Score = 0;
		}
	}
}

void CGameController::OnPlayerInfoChange(class CPlayer *pP)
{
	const int aTeamColors[2] = {65387, 10223467};
	if (IsTeamplay())
	{
		pP->m_TeeInfos.m_UseCustomColor = 1;
		if (pP->GetTeam() >= TEAM_HUMAN && pP->GetTeam() <= TEAM_ZOMBIE)
		{
			pP->m_TeeInfos.m_ColorBody = aTeamColors[pP->GetTeam()];
			pP->m_TeeInfos.m_ColorFeet = aTeamColors[pP->GetTeam()];
		}
		else
		{
			pP->m_TeeInfos.m_ColorBody = 12895054;
			pP->m_TeeInfos.m_ColorFeet = 12895054;
		}
	}
}

int CGameController::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	// do scoreing
	if (Weapon == WEAPON_GAME ||
		Weapon == WEAPON_WORLD ||
		Weapon == WEAPON_SELF)
		return 0;

	if (!pKiller)
		return 0;

	if (pVictim->GetPlayer()->IsBot())
	{
		int rando = rand() % 100 + 1;
		int num = 1 + rand() % 4;
		if (rando <= 50)
		{
			pKiller->m_Items[ITEM_LOG] += num;
			GameServer()->SendChatTarget(pKiller->GetCID(), _("You got {int:num} Log from the Zombie"), "num", &num);
		}
		else if (rando >= 51 && rando <= 75)
		{
			pKiller->m_Items[ITEM_COPPER] += num;
			GameServer()->SendChatTarget(pKiller->GetCID(), _("You got {int:num} Copper from the Zombie"), "num", &num);
		}
		else if (rando <= 99)
		{
			pKiller->m_Items[ITEM_GOLDEN] += num;
			GameServer()->SendChatTarget(pKiller->GetCID(), _("You got {int:num} Gold from the Zombie"), "num", &num);
		}
		else
		{
			short r = rand() % 3;
			if (r == 0)
			{
				pKiller->m_Items[ITEM_ENEGRY]++;
				GameServer()->SendChatTarget(pKiller->GetCID(), _("===-=== You got a Enegry from the Zombie ----"));
			}
			else if (r == 1)
			{
				pKiller->m_Items[ITEM_DIAMOND]++;
				GameServer()->SendChatTarget(pKiller->GetCID(), _("===-=== You got a Enegry from the Zombie ----"));
			}
		}

		pKiller->m_Items[ITEM_ZOMBIEHEART]++;
		GameServer()->SendChatTarget(pKiller->GetCID(), _("You picked up a Zombie's Heart"));
		pKiller->m_Score++;
		DoZombMessage(m_ZombLeft--);
		GameServer()->TW()->Account()->SaveAccountData(pKiller->GetCID(), CGameContext::TABLE_ITEM, pKiller->m_AccData);
		GameServer()->ClearVotes(pKiller->GetCID());
	}
	if (pKiller && pKiller == pVictim->GetPlayer())
		return 0; // suicide
	else
	{
		if (IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam())
		{
			if (g_Config.m_SvTeamdamage)
				pKiller->m_Score--; // teamkill
		}
		else
			pKiller->m_Score++; // normal kill
	}
	pVictim->GetPlayer()->m_RespawnTick = 5;

	Server()->ExpireServerInfo();
	return 0;
}

void CGameController::OnCharacterSpawn(class CCharacter *pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	if (pChr->GetPlayer()->GetTeam() == TEAM_HUMAN)
	{
		pChr->IncreaseHealth(90);
		pChr->GiveWeapon(WEAPON_HAMMER, -1);
		pChr->GiveWeapon(WEAPON_GUN, 10);
	}
}

void CGameController::DoWarmup(int Seconds)
{
	if (Seconds < 0)
		m_Warmup = 0;
	else
		m_Warmup = Seconds * Server()->TickSpeed();
}

void CGameController::TogglePause()
{
	if (IsGameOver())
		return;

	if (GameServer()->m_World.m_Paused)
	{
		// unpause
		if (g_Config.m_SvUnpauseTimer > 0)
			m_UnpauseTimer = g_Config.m_SvUnpauseTimer * Server()->TickSpeed();
		else
		{
			GameServer()->m_World.m_Paused = false;
			m_UnpauseTimer = 0;
		}
	}
	else
	{
		// pause
		GameServer()->m_World.m_Paused = true;
		m_UnpauseTimer = 0;
	}
}

bool CGameController::IsFriendlyFire(int ClientID1, int ClientID2)
{
	if (ClientID1 == -1 || ClientID2 == -1)
		return false;
	if (ClientID1 == ClientID2)
		return false;

	if (IsTeamplay())
	{
		if (!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
			return false;

		if (GameServer()->m_apPlayers[ClientID1]->GetTeam() == GameServer()->m_apPlayers[ClientID2]->GetTeam())
			return true;
	}

	return false;
}

bool CGameController::IsForceBalanced()
{
	if (m_ForceBalanced)
	{
		m_ForceBalanced = false;
		return true;
	}
	else
		return false;
}

bool CGameController::CanBeMovedOnBalance(int ClientID)
{
	return true;
}

void CGameController::Tick()
{
	// do warmup
	if (!GameServer()->m_World.m_Paused && m_Warmup)
	{
		m_Warmup--;
		if (!m_Warmup)
		{
			StartWave();
			GameServer()->m_World.m_Paused = false;
		}
	}

	if (m_GameOverTick != -1)
	{
		// game over.. wait for restart
		if (Server()->Tick() > m_GameOverTick + Server()->TickSpeed() * 10)
		{
			CycleMap();
			PostReset();

			// Zomb2: Do this ONLY when the Game ended, that must be BEFORE the round restarts
			m_aTeamscore[TEAM_HUMAN] = 0;
			m_aTeamscore[TEAM_ZOMBIE] = g_Config.m_SvLives;
			DoWarmup(g_Config.m_SvWarmup);
			m_GameOverTick = -1;
			m_RoundStartTick = Server()->Tick();
			GameServer()->m_World.m_Paused = false;

			// StartRound(); Not needed anymore, do warmup instead (with warmup config)
			m_RoundCount++;
		}
	}
	else if (GameServer()->m_World.m_Paused && m_UnpauseTimer)
	{
		--m_UnpauseTimer;
		if (!m_UnpauseTimer)
			GameServer()->m_World.m_Paused = false;
	}


	// game is Paused
	if (GameServer()->m_World.m_Paused)
		++m_RoundStartTick;

	// do team-balancing
	if (IsTeamplay() && m_UnbalancedTick != -1 && Server()->Tick() > m_UnbalancedTick + g_Config.m_SvTeambalanceTime * Server()->TickSpeed() * 60)
	{
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "Balancing teams");

		int aT[2] = {0, 0};
		float aTScore[2] = {0, 0};
		float aPScore[MAX_CLIENTS] = {0.0f};
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			{
				aT[GameServer()->m_apPlayers[i]->GetTeam()]++;
				aPScore[i] = GameServer()->m_apPlayers[i]->m_Score * Server()->TickSpeed() * 60.0f /
							 (Server()->Tick() - GameServer()->m_apPlayers[i]->m_ScoreStartTick);
				aTScore[GameServer()->m_apPlayers[i]->GetTeam()] += aPScore[i];
			}
		}

		// are teams unbalanced?
		if (absolute(aT[0] - aT[1]) >= 2)
		{
			int M = (aT[0] > aT[1]) ? 0 : 1;
			int NumBalance = absolute(aT[0] - aT[1]) / 2;

			do
			{
				CPlayer *pP = 0;
				float PD = aTScore[M];
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (!GameServer()->m_apPlayers[i] || !CanBeMovedOnBalance(i))
						continue;
					// remember the player who would cause lowest score-difference
					if (GameServer()->m_apPlayers[i]->GetTeam() == M && (!pP || absolute((aTScore[M ^ 1] + aPScore[i]) - (aTScore[M] - aPScore[i])) < PD))
					{
						pP = GameServer()->m_apPlayers[i];
						PD = absolute((aTScore[M ^ 1] + aPScore[i]) - (aTScore[M] - aPScore[i]));
					}
				}

				// move the player to the other team
				int Temp = pP->m_LastActionTick;
				pP->SetTeam(M ^ 1);
				pP->m_LastActionTick = Temp;

				pP->Respawn();
				pP->m_ForceBalanced = true;
			} while (--NumBalance);

			m_ForceBalanced = true;
		}
		m_UnbalancedTick = -1;
	}

	// check for inactive players
	if (g_Config.m_SvInactiveKickTime > 0)
	{
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
#ifdef CONF_DEBUG
			if (g_Config.m_DbgDummies)
			{
				if (i >= MAX_CLIENTS - g_Config.m_DbgDummies)
					break;
			}
#endif
		}
	}

	DoWincheck();
}

bool CGameController::IsTeamplay() const
{
	return m_GameFlags & GAMEFLAG_TEAMS;
}

void CGameController::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = (CNetObj_GameInfo *)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if (!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if (m_GameOverTick != -1)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
	if (m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;
	if (GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
	pGameInfoObj->m_RoundStartTick = 0;
	pGameInfoObj->m_WarmupTimer = GameServer()->m_World.m_Paused ? m_UnpauseTimer : m_Warmup;

	pGameInfoObj->m_ScoreLimit = g_Config.m_SvScorelimit;
	pGameInfoObj->m_TimeLimit = g_Config.m_SvTimelimit;

	pGameInfoObj->m_RoundNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvRoundsPerMap) ? g_Config.m_SvRoundsPerMap : 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount + 1;

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if (!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_HUMAN];
	pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_ZOMBIE];

	pGameDataObj->m_FlagCarrierRed = 0;
	pGameDataObj->m_FlagCarrierBlue = 0;
}

int CGameController::GetAutoTeam(int NotThisID)
{
	// this will force the auto balancer to work overtime aswell
	if (g_Config.m_DbgStress)
		return 0;

	int aNumplayers[2] = {0, 0};
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if (GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_HUMAN && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_ZOMBIE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	int Team = 0;
	if (IsTeamplay())
		Team = aNumplayers[TEAM_HUMAN] > aNumplayers[TEAM_ZOMBIE] ? TEAM_ZOMBIE : TEAM_HUMAN;

	if (CanJoinTeam(Team, NotThisID))
		return Team;
	return -1;
}

bool CGameController::CanJoinTeam(int Team, int NotThisID)
{
	if (Team == TEAM_SPECTATORS || (GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS))
		return true;

	int aNumplayers[2] = {0, 0};
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if (GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_HUMAN && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_ZOMBIE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	return (aNumplayers[0] + aNumplayers[1]) < Server()->MaxClients() - g_Config.m_SvSpectatorSlots;
}

bool CGameController::CheckTeamBalance()
{
	// Zomb2
	return true;
}

bool CGameController::CanChangeTeam(CPlayer *pPlayer, int JoinTeam)
{
	int aT[2] = {0, 0};

	if (!IsTeamplay() || JoinTeam == TEAM_SPECTATORS || !g_Config.m_SvTeambalanceTime)
		return true;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if (pP && pP->GetTeam() != TEAM_SPECTATORS)
			aT[pP->GetTeam()]++;
	}

	// simulate what would happen if changed team
	aT[JoinTeam]++;
	if (pPlayer->GetTeam() != TEAM_SPECTATORS)
		aT[JoinTeam ^ 1]--;

	// there is a player-difference of at least 2
	if (absolute(aT[0] - aT[1]) >= 2)
	{
		// player wants to join team with less players
		if ((aT[0] < aT[1] && JoinTeam == TEAM_HUMAN) || (aT[0] > aT[1] && JoinTeam == TEAM_ZOMBIE))
			return true;
		else
			return false;
	}
	else
		return true;
}

void CGameController::DoWincheck()
{
	if (m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested)
	{
		if (GameServer()->m_TowerHealth <= 0)
		{
			EndRound();
		}
	}
}

int CGameController::ClampTeam(int Team)
{
	if (Team < 0)
		return TEAM_SPECTATORS;
	if (IsTeamplay())
		return Team & 1;
	return 0;
}

void CGameController::StartWave()
{
	if (GameServer()->NumPlayers(true))
	{
		// Zaby, Zaby has no alround wave
		if (GameServer()->NumPlayers() == 1)
			m_Zombie[0] += 10;
		else if (GameServer()->NumPlayers() == 2)
			m_Zombie[0] += 20;
		else
			SetWaveAlg(GameServer()->NumPlayers() % 3, GameServer()->NumPlayers() / 3);
	}
	int BeforeZombLeft = m_ZombLeft;
	for (int i = 0; i < (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0])); i++)
		m_ZombLeft += m_Zombie[i];

	DoZombMessage(0);
	// DoWarmup(BeforeZombLeft + GameServer()->NumPlayers() * 2 + 10);
	DoWarmup(4);
	CheckZomb();
}

int CGameController::RandZomb()
{
	int size = (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0]));
	int Rand = rand() % size;
	int WTF = g_Config.m_SvMaxZombieSpawn; // dont make it to high, can cause bad cpu
	while (!m_Zombie[Rand])
	{
		Rand = rand() % size;
		WTF--;
		if (!WTF) // Anti 100% CPU :D (Very crappy, but it's a fix :P)
			return -1;
	}
	return Rand;
}

void CGameController::DoZombMessage(int Which)
{
	g_Config.m_SvScorelimit = m_ZombLeft;
}

void CGameController::DoLifeMessage(int Life)
{
	GameServer()->SendChatTarget(-1, _("The main tower is under attacked!"));
	if (Life > 1)
	{
		if (Life <= 30)
			GameServer()->SendChatTarget(-1, _("Only {int:Health} lifes left!"), "Health", &Life);
		else if (Life == 50)
			GameServer()->SendChatTarget(-1, _("Only HALF health left!!"));
		else
			GameServer()->SendChatTarget(-1, _("{int:Health} health left!"), "Health", &Life);
	}
	else if (Life == 1)
		GameServer()->SendChatTarget(-1, _("!!!Only 1 health left!!!"));
}

void CGameController::HandleTop()
{
	/*	//char aBuf[16];
		if(m_pTop->GetInfo())//File exist
		{
			if(m_pTop->m_TopFiveVars.m_aTeams[4].m_TeamScore < m_aTeamscore[TEAM_HUMAN])
			{
				m_pTop->m_TopFiveVars.m_aTeams[4].m_NumPlayers = 0;
				for(int i = 0; i < 16; i++)
				{
					if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
					{
						//str_format(aBuf, sizeof(aBuf), "%s", Server()->ClientName(i));
						m_pTop->m_TopFiveVars.m_aTeams[4].m_NumPlayers++;
						str_format(m_pTop->m_TopFiveVars.m_aTeams[4].m_aaName[i], sizeof(m_pTop->m_TopFiveVars.m_aTeams[4].m_aaName[i]), "%s", Server()->ClientName(i));
						GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "DEBUGGING", m_pTop->m_TopFiveVars.m_aTeams[4].m_aaName[i]);
						m_pTop->m_TopFiveVars.m_aTeams[4].m_aKills[i] = GameServer()->m_apPlayers[i]->m_Score;
					}
					else
					{
						str_format(m_pTop->m_TopFiveVars.m_aTeams[4].m_aaName[i], sizeof(m_pTop->m_TopFiveVars.m_aTeams[4].m_aaName[i]), "null");
						m_pTop->m_TopFiveVars.m_aTeams[4].m_aKills[i] = 0;
					}
				}
				m_pTop->m_TopFiveVars.m_aTeams[4].m_TeamScore = m_aTeamscore[TEAM_HUMAN];
				m_pTop->m_TopFiveVars.m_aTeams[4].m_Waves = m_Wave;
				m_pTop->SortArray(4);
			}
			else
				return;//File esistiert und Spieler schlechter als 5. platz -> bb
		}
		else//File doesn't exist-> Platz 1
		{
			m_pTop->m_TopFiveVars.m_aTeams[0].m_NumPlayers = 0;//iniatialisiere!!
			for(int i = 0; i < 16; i++)
			{
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				{
					m_pTop->m_TopFiveVars.m_aTeams[0].m_NumPlayers++;
					str_format(m_pTop->m_TopFiveVars.m_aTeams[0].m_aaName[i], sizeof(m_pTop->m_TopFiveVars.m_aTeams[0].m_aaName[i]), "%s", Server()->ClientName(i));
					m_pTop->m_TopFiveVars.m_aTeams[0].m_aKills[i] = GameServer()->m_apPlayers[i]->m_Score;
				}
				else
				{
					str_format(m_pTop->m_TopFiveVars.m_aTeams[0].m_aaName[i], sizeof(m_pTop->m_TopFiveVars.m_aTeams[0].m_aaName[i]), "null");
					m_pTop->m_TopFiveVars.m_aTeams[0].m_aKills[i] = 0;
				}
			}
			m_pTop->m_TopFiveVars.m_aTeams[0].m_TeamScore = m_aTeamscore[TEAM_HUMAN];
			m_pTop->m_TopFiveVars.m_aTeams[0].m_Waves = m_Wave;

			for(int k = 1; k < 16; k++)//Speicher den rest als Null
			{
				for(int i = 0; i < 16; i++)
				{
					str_format(m_pTop->m_TopFiveVars.m_aTeams[k].m_aaName[i], sizeof(m_pTop->m_TopFiveVars.m_aTeams[k].m_aaName[i]), "null");
					m_pTop->m_TopFiveVars.m_aTeams[k].m_aKills[i] = 0;
				}
				m_pTop->m_TopFiveVars.m_aTeams[k].m_NumPlayers = 0;
				m_pTop->m_TopFiveVars.m_aTeams[k].m_TeamScore = 0;
				m_pTop->m_TopFiveVars.m_aTeams[k].m_Waves = 0;
			}
			m_pTop->SortArray(0);
		}
		m_pTop->Write(m_pTop->m_TopFiveVars);*/
}

void CGameController::SetWaveAlg(int modulus, int wavedrittel)
{
	if (GameServer()->NumPlayers() > 16) // endless Waves, but exponentiell Zombie code
	{
		for (int i = 0; i < (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0])); i++)
			m_Zombie[i] += GameServer()->NumPlayers(); // 3 mal wavedrittel + modulus 2
		return;
	}
	else
	{
		m_Zombie[0] += 10 + GameServer()->NumPlayers();
	}
}

int CGameController::GetZombieReihenfolge(int wavedrittel) // Was hei�t Riehenfolge auf englisch ...
{
	// sehr unsch�n, man m�sste die Zombies neu sortieren was ein haufen arbeit ist
	if (!wavedrittel)
		return 0;
	else if (wavedrittel == 1)
		return 2;
	else if (wavedrittel == 2)
		return 3;
	else if (wavedrittel == 3)
		return 4;
	else if (wavedrittel == 4)
		return 6;
	else if (wavedrittel == 5)
		return 5;
	else if (wavedrittel == 6)
		return 7;
	else if (wavedrittel == 7)
		return 8;
	else if (wavedrittel == 8)
		return 9;
	else if (wavedrittel == 9)
		return 10;
	else if (wavedrittel == 10)
		return 11;
	else if (wavedrittel == 11)
		return 1;
	else // shouldnt be needed
		return 0;
}

void CGameController::CheckZomb()
{
	for (int j = 0; j < (int)(sizeof(m_Zombie) / sizeof(m_Zombie[0])); j++)
	{
		if (!m_Zombie[j])
			continue;

		for (int i = ZOMBIE_START; i < 64; i++) //...
		{
			if (m_Zombie[j] <= 0)
				break;

			CPlayer *pP = GameServer()->m_apPlayers[i];
			if (!pP)
				continue;

			if (!pP->m_BotSleep)
				continue;

			GameServer()->WakeBotUp(i);
			m_Zombie[j]--;
		}
	}
}
