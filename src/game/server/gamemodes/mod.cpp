// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "mod.h"

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "TeeDefense Time";
	m_GameFlags = GAMEFLAG_TEAMS;
	m_LastActivePlayers = 0;
}



int CGameControllerMOD::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);
	pVictim->GetPlayer()->m_RespawnTick = max(pVictim->GetPlayer()->m_RespawnTick, Server()->Tick()+Server()->TickSpeed()*g_Config.m_SvRespawnDelayTDM);

	return 0;
}

void CGameControllerMOD::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
	pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];

	pGameDataObj->m_FlagCarrierRed = 0;
	pGameDataObj->m_FlagCarrierBlue = 0;
}

void CGameControllerMOD::Tick()
{
	int Players = 0;
	for(int i = 0;i < VANILLA_MAX_CLIENTS - 1; i++)
	{
		if(GameServer()->m_apPlayers[Players])
		{
			if(GameServer()->m_apPlayers[Players]->GetTeam() == TEAM_RED)
			{
				Players++;
			}
		}
	}

	if(Players && (m_LastActivePlayers == 0))
	{
		StartRound();
	}

	m_LastActivePlayers = Players;

	IGameController::Tick();
}