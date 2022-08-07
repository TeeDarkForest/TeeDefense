/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include "player.h"


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team, int Zomb)
{
	m_pGameServer = pGameServer;
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_Team = Team;
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();
	m_TeamChangeTick = Server()->Tick();
	SetLanguage(Server()->GetClientLanguage(ClientID));

	m_Authed = IServer::AUTHED_NO;

	m_PrevTuningParams = *pGameServer->Tuning();
	m_NextTuningParams = m_PrevTuningParams;

	int* idMap = Server()->GetIdMap(ClientID);
	for (int i = 1;i < VANILLA_MAX_CLIENTS;i++)
	{
	    idMap[i] = -1;
	}
	idMap[0] = ClientID;

	m_AccData.m_UserID = 0;
	LoggedIn = false;
	if(!Zomb)
		ResetKnapsack();
	//Zomb2
	m_Zomb = Zomb;
	mem_zero(m_SubZomb, sizeof(m_SubZomb));
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
}

void CPlayer::HandleTuningParams()
{
	if(!(m_PrevTuningParams == m_NextTuningParams))
	{
		if(m_IsReady)
		{
			CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
			int *pParams = (int *)&m_NextTuningParams;
			for(unsigned i = 0; i < sizeof(m_NextTuningParams)/sizeof(int); i++)
			Msg.AddInt(pParams[i]);
			Server()->SendMsg(&Msg, MSGFLAG_VITAL, GetCID());
		}

		m_PrevTuningParams = m_NextTuningParams;
	}

	m_NextTuningParams = *GameServer()->Tuning();
}

void CPlayer::Tick()
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	Server()->SetClientScore(m_ClientID, m_Score);
	Server()->SetClientLanguage(m_ClientID, m_aLanguage);

	// do latency stuff
	if(!m_Zomb)
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(!GameServer()->m_World.m_Paused)
	{
		if(!m_pCharacter && m_Team == TEAM_SPECTATORS && m_SpectatorID == SPEC_FREEVIEW)
			m_ViewPos -= vec2(clamp(m_ViewPos.x-m_LatestActivity.m_TargetX, -500.0f, 500.0f), clamp(m_ViewPos.y-m_LatestActivity.m_TargetY, -400.0f, 400.0f));

		if(!m_pCharacter && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
			m_Spawning = true;

		if(m_pCharacter)
		{
			if(m_pCharacter->IsAlive())
			{
				m_ViewPos = m_pCharacter->m_Pos;
			}
			else
			{
				delete m_pCharacter;
				m_pCharacter = 0;
			}
		}
		else if(m_Spawning && m_RespawnTick <= Server()->Tick())
			TryRespawn();
	}
	else
	{
		++m_RespawnTick;
		++m_DieTick;
		++m_ScoreStartTick;
		++m_LastActionTick;
		++m_TeamChangeTick;
 	}

	/*char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Log: %d", m_Knapsack.m_Log);
	GameServer()->SendBroadcast(aBuf, m_ClientID);*/

	if(m_MiningTick > -1)
		m_MiningTick--;

	HandleTuningParams();
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS && m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[m_SpectatorID])
		m_ViewPos = GameServer()->m_apPlayers[m_SpectatorID]->m_ViewPos;
}

void CPlayer::Snap(int SnappingClient)
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID) && !m_Zomb)
		return;

	int id = m_ClientID;
	if (!Server()->Translate(id, SnappingClient)) return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

	if(m_Zomb)
	{
		pClientInfo->m_UseCustomColor = 0;
		StrToInts(&pClientInfo->m_Clan0, 3, "Zombie");
		pClientInfo->m_Country = 1000;
		pClientInfo->m_ColorBody = 16776960;
		pClientInfo->m_ColorFeet = 16776960;
		m_Team = 10; // Don't snap Zombies at Scorebroad.
		if(m_Zomb == 1)//Zaby
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zaby");
			StrToInts(&pClientInfo->m_Skin0, 6, "zaby");
		}
		else if(m_Zomb == 2)//Zoomer
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zoomer");
			StrToInts(&pClientInfo->m_Skin0, 6, "redstripe");
		}
		else if(m_Zomb == 3)//Zooker
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zooker");
			StrToInts(&pClientInfo->m_Skin0, 6, "bluekitty");
		}
		else if(m_Zomb == 4)//Zamer
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zamer");
			StrToInts(&pClientInfo->m_Skin0, 6, "twinbop");
		}
		else if(m_Zomb == 5)//Zunner
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zunner");
			StrToInts(&pClientInfo->m_Skin0, 6, "cammostripes");
		}
		else if(m_Zomb == 6)//Zaster
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zaster");
			StrToInts(&pClientInfo->m_Skin0, 6, "coala");
		}
		else if(m_Zomb == 7)//Zotter
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zotter");
			StrToInts(&pClientInfo->m_Skin0, 6, "cammo");
		}
		else if(m_Zomb == 8)//Zenade
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zenade");
			StrToInts(&pClientInfo->m_Skin0, 6, "twintri");
		}
		else if(m_Zomb == 9)//Fombie
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Flombie");
			StrToInts(&pClientInfo->m_Skin0, 6, "toptri");
		}
		else if(m_Zomb == 10)//Zinja
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zinja");
			StrToInts(&pClientInfo->m_Skin0, 6, "default");
		}
		else if(m_Zomb == 11)//Zele
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zele");
			StrToInts(&pClientInfo->m_Skin0, 6, "redbopp");
		}
		else if(m_Zomb == 12)//Zinvis
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zinvis");
			StrToInts(&pClientInfo->m_Skin0, 6, "zaby");
		}
		else if(m_Zomb == 13)//Zeater
		{
			StrToInts(&pClientInfo->m_Name0, 4, "Zeater");
			StrToInts(&pClientInfo->m_Skin0, 6, "warpaint");
		}
	}
	else
	{
		StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(m_ClientID));
		StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
		pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);
		pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
		pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
		pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;
		StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	}

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, id, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	if(m_Zomb)
		pPlayerInfo->m_Latency = 0;
	else
		pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = id;
	pPlayerInfo->m_Score = m_Score;
	pPlayerInfo->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;

	if(m_ClientID == SnappingClient && m_Team == TEAM_SPECTATORS)
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}
}

void CPlayer::FakeSnap(int SnappingClient)
{
	IServer::CClientInfo info;
	Server()->GetClientInfo(SnappingClient, &info);

	int id;

	if (info.m_CustClt)
		int id = MAX_CHARACTERS-1;
	else
		int id = VANILLA_MAX_CLIENTS - 1;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));

	if(!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, " ");
	StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
}

void CPlayer::OnDisconnect(const char *pReason)
{
	#ifdef CONF_SQL
	if(!GetZomb() && LoggedIn)
	{
		dbg_msg("saa","LO");
		GameServer()->LogoutAccount(m_ClientID);
	}
	#endif

	if(Server()->ClientIngame(m_ClientID) && !m_Zomb)
	{
		char aBuf[512];
		if(pReason && *pReason)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(m_ClientID), pReason);
			GameServer()->SendChatTarget(-1, _("'{str:PlayerName}' has left the game ({str:Reason})"), "PlayerName", Server()->ClientName(m_ClientID), "Reason", pReason);
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(m_ClientID));
			GameServer()->SendChatTarget(-1, _("'{str:PlayerName}' has left the game"), "PlayerName", Server()->ClientName(m_ClientID));
		}

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}

	ResetKnapsack();
	KillCharacter();
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING)
			return;

		// reset input
		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
 		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		m_pCharacter->Die(m_ClientID, Weapon);
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::Respawn()
{
	if(m_Team != TEAM_SPECTATORS)
		m_Spawning = true;
}

void CPlayer::SetTeam(int Team, bool DoChatMsg)
{
	return;
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	if(!GameServer()->m_pController->CanSpawn(m_Team, &SpawnPos))
		return;

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(SpawnPos);
}

const char* CPlayer::GetLanguage()
{
	return m_aLanguage;
}

void CPlayer::SetLanguage(const char* pLanguage)
{
	str_copy(m_aLanguage, pLanguage, sizeof(m_aLanguage));
}

void CPlayer::DeleteCharacter()
{
	if(m_pCharacter)
	{
		m_Spawning = false;
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

bool CPlayer::GetZomb(int Type)
{
	if(m_Zomb == Type)
		return true;
	for(int i = 0; i < (int)(sizeof(m_SubZomb)/sizeof(m_SubZomb[0])); i++)
	{
		if(m_SubZomb[i] == Type)
			return true;
	}
	return false;
}

void CPlayer::ResetKnapsack()
{
	for(int i = 0; i < NUM_RESOURCE; i++)
		m_Knapsack.m_Resource[i] = 0;
	m_Knapsack.m_Axe = -1;
	m_Knapsack.m_Pickaxe = -1;
	m_Knapsack.m_Sword = -1;
}

int CPlayer::GetTeam()
{
	if(GetZomb())
		return TEAM_ZOMBIE;
	else
		return TEAM_HUMAN;
}

bool CPlayer::PressTab()
{
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
		return true;
	return false;
}
#ifdef CONF_SQL
void CPlayer::Logout()
{
	m_AccData.m_UserID = 0;
	LoggedIn = false;
}
#endif