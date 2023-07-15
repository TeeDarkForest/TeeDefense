/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/math.h>
#include <string.h>
#include <engine/shared/config.h>
#include <engine/map.h>
#include <engine/console.h>
#include "gamecontext.h"
#include <game/version.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include "entities/turret.h"

#include "item.h"
#ifdef CONF_BOX2D
#include "entities/box2d_box.h"
#include "entities/box2d_test.h"
#include "entities/box2d_test_spider.h"
#endif

#include "entities/giga-Qian.h"

#include <teeuniverses/components/localization.h>
#include <engine/server/crypt.h>


#include <engine/shared/json.h>

// Test Msg.
#define D(MSG) (dbg_msg("Test", MSG))

enum
{
	RESET,
	NO_RESET
};

#ifdef CONF_BOX2D
class MyQueryCallback : public b2QueryCallback
{
public:
	bool ReportFixture(b2Fixture *fixture)
	{
		b2Body *body = fixture->GetBody();
		body->SetAwake(true);

		// Return true to continue the query.
		return true;
	}
};
#endif

void CGameContext::Construct(int Resetting, bool ChangeMap)
{
	m_Resetting = 0;
	m_pServer = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
		m_apPlayers[i] = 0;

	m_pController = 0;
	m_VoteCloseTime = 0;
	m_pVoteOptionFirst = 0;
	m_pVoteOptionLast = 0;
	m_NumVoteOptions = 0;
	m_LockTeams = 0;
	m_ChatResponseTargetID = -1;

	if (Resetting == NO_RESET)
		m_pVoteOptionHeap = new CHeap();

	/* SQL */
	m_AccountData = new CAccountData;
	m_Sql = new CSQL(this);

	InitItems();
	InitCrafts();

	Qian = false;

#ifdef CONF_BOX2D
	b2AABB worldAABB;
	worldAABB.lowerBound.Set(0, 0);
	worldAABB.upperBound.Set(Collision()->GetHeight(), Collision()->GetWidth());
	b2Vec2 gravity(0.f, 0.5f * 30.0f);

	MyQueryCallback callback;

	m_b2world = new b2World(gravity);
	m_b2world->QueryAABB(&callback, worldAABB);
#endif
}

CGameContext::CGameContext(int Resetting, bool ChangeMap)
{
	Construct(Resetting, ChangeMap);
}

CGameContext::CGameContext()
{
	Construct(NO_RESET, false);
}

CGameContext::~CGameContext()
{
	for (auto &pPlayer : m_apPlayers)
		delete pPlayer;
	if (!m_Resetting)
		delete m_pVoteOptionHeap;
}

void CGameContext::OnSetAuthed(int ClientID, int Level)
{
	if (m_apPlayers[ClientID])
		m_apPlayers[ClientID]->m_Authed = Level;
}

void CGameContext::Clear(bool ChangeMap)
{
	CHeap *pVoteOptionHeap = m_pVoteOptionHeap;
	CVoteOptionServer *pVoteOptionFirst = m_pVoteOptionFirst;
	CVoteOptionServer *pVoteOptionLast = m_pVoteOptionLast;
	int NumVoteOptions = m_NumVoteOptions;
	CTuningParams Tuning = m_Tuning;

	delete m_Sql;
	delete m_AccountData;

#ifdef CONF_BOX2D
	if (m_b2world)
	{
		delete m_b2world;
		m_b2world = NULL;
	}
#endif

	m_Resetting = true;
	this->~CGameContext();
	mem_zero(this, sizeof(*this));
	new (this) CGameContext(RESET, ChangeMap);

	m_pVoteOptionHeap = pVoteOptionHeap;
	m_pVoteOptionFirst = pVoteOptionFirst;
	m_pVoteOptionLast = pVoteOptionLast;
	m_NumVoteOptions = NumVoteOptions;
	m_Tuning = Tuning;
}

class CCharacter *CGameContext::GetPlayerChar(int ClientID)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || !m_apPlayers[ClientID])
		return 0;
	return m_apPlayers[ClientID]->GetCharacter();
}

void CGameContext::CreateDamageInd(vec2 Pos, float Angle, int Amount, Mask128 Mask)
{
	float a = 3 * 3.14159f / 2 + Angle;
	// float a = get_angle(dir);
	float s = a - pi / 3;
	float e = a + pi / 3;
	for (int i = 0; i < Amount; i++)
	{
		float f = mix(s, e, float(i + 1) / float(Amount + 2));
		CNetEvent_DamageInd *pEvent = (CNetEvent_DamageInd *)m_Events.Create(NETEVENTTYPE_DAMAGEIND, sizeof(CNetEvent_DamageInd), Mask);
		if (pEvent)
		{
			pEvent->m_X = (int)Pos.x;
			pEvent->m_Y = (int)Pos.y;
			pEvent->m_Angle = (int)(f * 256.0f);
		}
	}
}

void CGameContext::CreateHammerHit(vec2 Pos, Mask128 Mask)
{
	// create the event
	CNetEvent_HammerHit *pEvent = (CNetEvent_HammerHit *)m_Events.Create(NETEVENTTYPE_HAMMERHIT, sizeof(CNetEvent_HammerHit), Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, Mask128 Mask)
{
	// create the event
	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion), Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}

	// deal damage
	CCharacter *apEnts[MAX_CLIENTS];
	float Radius = 135.0f;
	float InnerRadius = 48.0f;
	int Num = m_World.FindEntities(Pos, Radius, (CEntity **)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

	if (!NoDamage)
	{
		for (int i = 0; i < Num; i++)
		{
			vec2 Diff = apEnts[i]->m_Pos - Pos;
			vec2 ForceDir(0, 1);
			float l = length(Diff);
			if (l)
				ForceDir = normalize(Diff);
			l = 1 - clamp((l - InnerRadius) / (Radius - InnerRadius), 0.0f, 1.0f);
			float Dmg = 6 * l;
			if ((int)Dmg)
				apEnts[i]->TakeDamage(ForceDir * Dmg * 2, (int)Dmg, Owner, Weapon);
		}
	}

#ifdef CONF_BOX2D
	// apply force to box2d objects
	b2Vec2 b2Pos(Pos.x / 30.f, Pos.y / 30.f);
	b2Vec2 b2Rad((Radius) / 30.f, (Radius) / 30.f);
	float Strength = 4;

	int numRays = 32;
	for (int i = 0; i < numRays; i++)
	{
		float angle = ((i / (float)numRays) * 360) / 180.f * b2_pi;
		b2Vec2 rayDir(sinf(angle), cosf(angle));

		// use a "particle" system, essentially very tiny circles, that hits objects at high speeds
		b2BodyDef bd;
		bd.type = b2_dynamicBody;
		bd.fixedRotation = true; // don't rotate
		bd.bullet = true;		 // avoid tunneling at high speed
		bd.linearDamping = 10;	 // slow down
		bd.gravityScale = 0;	 // don't be affected by gravity
		bd.position = b2Pos;
		bd.linearVelocity = (Strength * 30.f) * rayDir;
		b2Body *body = m_b2world->CreateBody(&bd);

		b2CircleShape circle;
		circle.m_radius = 0.05; // ok so basically i'm very... you get the drill

		b2FixtureDef fd;
		fd.shape = &circle;
		fd.density = 60 / (float)numRays;
		fd.friction = 0;
		fd.restitution = 0.99f;
		fd.filter.groupIndex = -1;
		body->CreateFixture(&fd);

		m_b2explosions.push_back(body);
	}
#endif
}

/*
void create_smoke(vec2 Pos)
{
	// create the event
	EV_EXPLOSION *pEvent = (EV_EXPLOSION *)events.create(EVENT_SMOKE, sizeof(EV_EXPLOSION));
	if(pEvent)
	{
		pEvent->x = (int)Pos.x;
		pEvent->y = (int)Pos.y;
	}
}*/

void CGameContext::CreatePlayerSpawn(vec2 Pos, Mask128 Mask)
{
	// create the event
	CNetEvent_Spawn *ev = (CNetEvent_Spawn *)m_Events.Create(NETEVENTTYPE_SPAWN, sizeof(CNetEvent_Spawn), Mask);
	if (ev)
	{
		ev->m_X = (int)Pos.x;
		ev->m_Y = (int)Pos.y;
	}
}

void CGameContext::CreateDeath(vec2 Pos, int ClientID, Mask128 Mask)
{
	// create the event
	CNetEvent_Death *pEvent = (CNetEvent_Death *)m_Events.Create(NETEVENTTYPE_DEATH, sizeof(CNetEvent_Death), Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_ClientID = ClientID;
	}
}

void CGameContext::CreateSound(vec2 Pos, int Sound, Mask128 Mask)
{
	if (Sound < 0)
		return;

	// create a sound
	CNetEvent_SoundWorld *pEvent = (CNetEvent_SoundWorld *)m_Events.Create(NETEVENTTYPE_SOUNDWORLD, sizeof(CNetEvent_SoundWorld), Mask);
	if (pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
		pEvent->m_SoundID = Sound;
	}
}

void CGameContext::CreateSoundGlobal(int Sound, int Target)
{
	if (Sound < 0)
		return;

	CNetMsg_Sv_SoundGlobal Msg;
	Msg.m_SoundID = Sound;
	if (Target == -2)
		Server()->SendPackMsg(&Msg, MSGFLAG_NOSEND, -1);
	else
	{
		int Flag = MSGFLAG_VITAL;
		if (Target != -1)
			Flag |= MSGFLAG_NORECORD;
		Server()->SendPackMsg(&Msg, Flag, Target);
	}
}

void CGameContext::SendChatTarget(int To, const char *pText, ...)
{
	int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_CLIENTS : To + 1);

	CNetMsg_Sv_Chat Msg;
	Msg.m_Team = 0;
	Msg.m_ClientID = -1;

	dynamic_string Buffer;

	va_list VarArgs;
	va_start(VarArgs, pText);

	for (int i = Start; i < End; i++)
	{
		if (m_apPlayers[i])
		{
			Buffer.clear();
			Server()->Localization()->Format_VL(Buffer, m_apPlayers[i]->GetLanguage(), pText, VarArgs);

			Msg.m_pMessage = Buffer.buffer();
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
		}
	}

	va_end(VarArgs);
}

void CGameContext::SendChat(int ChatterClientID, int Team, const char *pText)
{
	char aBuf[256];
	if (ChatterClientID >= 0 && ChatterClientID < MAX_CLIENTS)
		str_format(aBuf, sizeof(aBuf), "%d:%d:%s: %s", ChatterClientID, Team, Server()->ClientName(ChatterClientID), pText);
	else
		str_format(aBuf, sizeof(aBuf), "*** %s", pText);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, Team != CHAT_ALL ? "teamchat" : "chat", aBuf);

	if (Team == CHAT_ALL)
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 0;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	else
	{
		CNetMsg_Sv_Chat Msg;
		Msg.m_Team = 1;
		Msg.m_ClientID = ChatterClientID;
		Msg.m_pMessage = pText;

		// pack one for the recording only
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NOSEND, -1);

		// send to the clients
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() == Team)
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, i);
		}
	}
}

void CGameContext::SendEmoticon(int ClientID, int Emoticon)
{
	CNetMsg_Sv_Emoticon Msg;
	Msg.m_ClientID = ClientID;
	Msg.m_Emoticon = Emoticon;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
}

void CGameContext::SendWeaponPickup(int ClientID, int Weapon)
{
	if (!m_apPlayers[ClientID]->GetZomb()) // Zombies don't pickup weapons
	{
		CNetMsg_Sv_WeaponPickup Msg;
		Msg.m_Weapon = Weapon;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
	}
}

void CGameContext::SendBroadcast(const char *pText, int ClientID)
{
	CNetMsg_Sv_Broadcast Msg;
	Msg.m_pMessage = pText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SendBroadcast_VL(const char *pText, int ClientID, ...)
{
	CNetMsg_Sv_Broadcast Msg;
	int Start = (ClientID < 0 ? 0 : ClientID);
	int End = (ClientID < 0 ? MAX_CLIENTS : ClientID + 1);

	dynamic_string Buffer;

	va_list VarArgs;
	va_start(VarArgs, pText);

	// only for server demo record
	if (ClientID < 0)
	{
		Server()->Localization()->Format_VL(Buffer, "en", _(pText), VarArgs);
		Msg.m_pMessage = Buffer.buffer();
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NOSEND, -1);
	}

	for (int i = Start; i < End; i++)
	{
		if (m_apPlayers[i])
		{
			Buffer.clear();
			Server()->Localization()->Format_VL(Buffer, m_apPlayers[i]->GetLanguage(), _(pText), VarArgs);

			Msg.m_pMessage = Buffer.buffer();
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
		}
	}

	va_end(VarArgs);
}

//
void CGameContext::StartVote(const char *pDesc, const char *pCommand, const char *pReason)
{
	// check if a vote is already running
	if (m_VoteCloseTime)
		return;

	// reset votes
	m_VoteEnforce = VOTE_ENFORCE_UNKNOWN;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
		{
			m_apPlayers[i]->m_Vote = 0;
			m_apPlayers[i]->m_VotePos = 0;
		}
	}

	// start vote
	m_VoteCloseTime = time_get() + time_freq() * 25;
	str_copy(m_aVoteDescription, pDesc, sizeof(m_aVoteDescription));
	str_copy(m_aVoteCommand, pCommand, sizeof(m_aVoteCommand));
	str_copy(m_aVoteReason, pReason, sizeof(m_aVoteReason));
	SendVoteSet(-1);
	m_VoteUpdate = true;
}

void CGameContext::EndVote()
{
	m_VoteCloseTime = 0;
	SendVoteSet(-1);
}

void CGameContext::SendVoteSet(int ClientID)
{
	CNetMsg_Sv_VoteSet Msg;
	if (m_VoteCloseTime)
	{
		Msg.m_Timeout = (m_VoteCloseTime - time_get()) / time_freq();
		Msg.m_pDescription = m_aVoteDescription;
		Msg.m_pReason = m_aVoteReason;
	}
	else
	{
		Msg.m_Timeout = 0;
		Msg.m_pDescription = "";
		Msg.m_pReason = "";
	}
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SendVoteStatus(int ClientID, int Total, int Yes, int No)
{
	CNetMsg_Sv_VoteStatus Msg = {0};
	Msg.m_Total = Total;
	Msg.m_Yes = Yes;
	Msg.m_No = No;
	Msg.m_Pass = Total - (Yes + No);

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::AbortVoteKickOnDisconnect(int ClientID)
{
	if (m_VoteCloseTime && ((!str_comp_num(m_aVoteCommand, "kick ", 5) && str_toint(&m_aVoteCommand[5]) == ClientID) ||
							(!str_comp_num(m_aVoteCommand, "set_team ", 9) && str_toint(&m_aVoteCommand[9]) == ClientID)))
		m_VoteCloseTime = -1;
}

void CGameContext::CheckPureTuning()
{
	// might not be created yet during start up
	if (!m_pController)
		return;
}

void CGameContext::SendTuningParams(int ClientID)
{
	CheckPureTuning();

	CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
	int *pParams = (int *)&m_Tuning;
	for (unsigned i = 0; i < sizeof(m_Tuning) / sizeof(int); i++)
		Msg.AddInt(pParams[i]);
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::SwapTeams()
{
	if (!m_pController->IsTeamplay())
		return;

	SendChatTarget(-1, _("Teams were swapped"));

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			m_apPlayers[i]->SetTeam(m_apPlayers[i]->GetTeam() ^ 1, false);
	}

	(void)m_pController->CheckTeamBalance();
}

void CGameContext::OnTick()
{
	// check tuning
	CheckPureTuning();

	// copy tuning
	m_World.m_Core.m_Tuning = m_Tuning;
	m_World.Tick();

	// if(world.paused) // make sure that the game object always updates
	m_pController->Tick();

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
		{
			m_apPlayers[i]->Tick();
			m_apPlayers[i]->PostTick();
		}
	}
	// Zomb2 - fixing a very little bug
	if (m_MessageReturn)
		m_MessageReturn--;
	if (m_MessageReturn == 1)
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		Server()->SendPackMsg(&Msg, MSGFLAG_FLUSH, -1);
	}
	// update voting
	if (m_VoteCloseTime)
	{
		// abort the kick-vote on player-leave
		if (m_VoteCloseTime == -1)
		{
			SendChatTarget(-1, _("Vote aborted"));
			EndVote();
		}
		else
		{
			int Total = 0, Yes = 0, No = 0;
			if (m_VoteUpdate)
			{
				// count votes
				char aaBuf[MAX_CLIENTS][NETADDR_MAXSTRSIZE] = {{0}};
				for (int i = 0; i < MAX_CLIENTS; i++)
					if (m_apPlayers[i])
						Server()->GetClientAddr(i, aaBuf[i], NETADDR_MAXSTRSIZE);
				bool aVoteChecked[MAX_CLIENTS] = {0};
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (!m_apPlayers[i] || m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS || aVoteChecked[i]) // don't count in votes by spectators
						continue;

					int ActVote = m_apPlayers[i]->m_Vote;
					int ActVotePos = m_apPlayers[i]->m_VotePos;

					// check for more players with the same ip (only use the vote of the one who voted first)
					for (int j = i + 1; j < MAX_CLIENTS; ++j)
					{
						if (!m_apPlayers[j] || aVoteChecked[j] || str_comp(aaBuf[j], aaBuf[i]))
							continue;

						aVoteChecked[j] = true;
						if (m_apPlayers[j]->m_Vote && (!ActVote || ActVotePos > m_apPlayers[j]->m_VotePos))
						{
							ActVote = m_apPlayers[j]->m_Vote;
							ActVotePos = m_apPlayers[j]->m_VotePos;
						}
					}

					Total++;
					if (ActVote > 0)
						Yes++;
					else if (ActVote < 0)
						No++;
				}

				if (Yes >= Total / 2 + 1)
					m_VoteEnforce = VOTE_ENFORCE_YES;
				else if (No >= (Total + 1) / 2)
					m_VoteEnforce = VOTE_ENFORCE_NO;
			}

			if (m_VoteEnforce == VOTE_ENFORCE_YES)
			{
				Server()->SetRconCID(IServer::RCON_CID_VOTE);
				Console()->ExecuteLine(m_aVoteCommand, -1);
				Server()->SetRconCID(IServer::RCON_CID_SERV);
				EndVote();
				SendChatTarget(-1, _("Vote passed"));

				if (m_apPlayers[m_VoteCreator])
					m_apPlayers[m_VoteCreator]->m_LastVoteCall = 0;
			}
			else if (m_VoteEnforce == VOTE_ENFORCE_NO || time_get() > m_VoteCloseTime)
			{
				EndVote();
				SendChatTarget(-1, _("Vote failed"));
			}
			else if (m_VoteUpdate)
			{
				m_VoteUpdate = false;
				SendVoteStatus(-1, Total, Yes, No);
			}
		}
	}

#ifdef CONF_BOX2D
	HandleBox2D();
#endif

	Qian = QianIsAlive();

	if (!m_EventDuration && !IsAbyss())
		m_EventTimer = rand() % 30 * 60 * Server()->TickSpeed() + 15 * 60 * Server()->TickSpeed();
	if (m_EventTimer > -1 && !IsAbyss())
		m_EventTimer--;
	if (m_EventDuration >= 0 && !IsAbyss())
		m_EventDuration--;
	if (m_EventTimer == 0 && !IsAbyss())
	{
		m_EventTimer = -1;
		m_EventDuration = 15 * 30 * Server()->TickSpeed();
		m_EventType = 1;
	}
	else
		m_EventType = 0;

	switch (m_EventType)
	{
	case 1:
		UnsealQianFromAbyss(0);
		break;

	default:
		break;
	}

#ifdef CONF_DEBUG
	if (g_Config.m_DbgDummies)
	{
		for (int i = 0; i < g_Config.m_DbgDummies; i++)
		{
			CNetObj_PlayerInput Input = {0};
			Input.m_Direction = (i & 1) ? -1 : 1;
			m_apPlayers[MAX_CLIENTS - i - 1]->OnPredictedInput(&Input);
		}
	}
#endif
}

bool CGameContext::QianIsAlive()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!m_apPlayers[i])
			continue;

		if (m_apPlayers[i]->GetZomb(14))
			return true;
	}
	return false;
}

#ifdef CONF_BOX2D
void CGameContext::HandleBox2D()
{
	if (m_b2world)
	{
		m_b2world->Step(1. / g_Config.m_B2WorldFps, 8, 3);
		for (unsigned i = 0; i < m_b2explosions.size(); i++)
		{
			if (m_b2explosions[i]->GetLinearVelocity().x < 5.f and m_b2explosions[i]->GetLinearVelocity().y < 5.f) // delete
			{
				m_b2world->DestroyBody(m_b2explosions[i]);
				m_b2explosions.erase(m_b2explosions.begin() + i);
				--i;
			}
		}
	}
}
#endif

// Server hooks
void CGameContext::OnClientDirectInput(int ClientID, void *pInput)
{
	if (!m_World.m_Paused)
		m_apPlayers[ClientID]->OnDirectInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientPredictedInput(int ClientID, void *pInput)
{
	if (!m_World.m_Paused)
		m_apPlayers[ClientID]->OnPredictedInput((CNetObj_PlayerInput *)pInput);
}

void CGameContext::OnClientEnter(int ClientID)
{
	// world.insert_entity(&players[client_id]);
	m_apPlayers[ClientID]->Respawn();
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientID), m_pController->GetTeamName(m_apPlayers[ClientID]->GetTeam()));
	SendChatTarget(-1, _("'{str:PlayerName}' entered and joined the {str:Team}"), "PlayerName", Server()->ClientName(ClientID), "Team", m_pController->GetTeamName(m_apPlayers[ClientID]->GetTeam()));

	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), m_apPlayers[ClientID]->GetTeam());
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	if (str_comp(Server()->ClientName(ClientID), "FlowerFell-Sans") == 0 || str_comp(Server()->ClientName(ClientID), "Flower") == 0)
	{
		SendChatTarget(-1, _("欢迎模式作者FlowerFell-Sans | Flower 进入服务器!"));
	}
	if (str_comp(Server()->ClientName(ClientID), "ResetPower") == 0 || str_comp(Server()->ClientName(ClientID), "RemakePower") == 0)
	{
		SendChatTarget(-1, _("欢迎模式作者ResetPower | RemakePower进入服务器!"));
	}

	m_VoteUpdate = true;

	ClearVotes(ClientID);

	Server()->ExpireServerInfo();
}

void CGameContext::OnClientConnected(int ClientID)
{
	if (m_apPlayers[ClientID])
	{
		OnZombieKill(ClientID);
	}
	m_apPlayers[ClientID] = new (ClientID) CPlayer(this, ClientID, 0, 0);
	// players[client_id].init(client_id);
	// players[client_id].client_id = client_id;

	(void)m_pController->CheckTeamBalance();

#ifdef CONF_DEBUG
	if (g_Config.m_DbgDummies)
	{
		if (ClientID >= MAX_CLIENTS - g_Config.m_DbgDummies)
			return;
	}
#endif

	// send active vote
	if (m_VoteCloseTime)
		SendVoteSet(ClientID);

	SetClientLanguage(ClientID, "zh-cn");
	SendChatTarget(ClientID, _("Use command '/language en' to change language English"));
	SendChatTarget(ClientID, _("上面那消息是给外国人看的，咱中国人不用管！awa"));
	// send motd
	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = g_Config.m_SvMotd;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);

	Server()->ExpireServerInfo();
}

void CGameContext::OnClientDrop(int ClientID, const char *pReason)
{
	AbortVoteKickOnDisconnect(ClientID);
	m_apPlayers[ClientID]->OnDisconnect(pReason);
	delete m_apPlayers[ClientID];
	m_apPlayers[ClientID] = 0;

	(void)m_pController->CheckTeamBalance();
	m_VoteUpdate = true;

	// update spectator modes
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_apPlayers[i] && m_apPlayers[i]->m_SpectatorID == ClientID)
			m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
	}

	Server()->ExpireServerInfo();
}

void CGameContext::OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgID, pUnpacker);
	CPlayer *pPlayer = m_apPlayers[ClientID];

	if (!pRawMsg)
	{
		if (g_Config.m_Debug)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgID), MsgID, m_NetObjHandler.FailedMsgOn());
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
		}
		return;
	}

	if (Server()->ClientIngame(ClientID))
	{
		if (MsgID == NETMSGTYPE_CL_SAY)
		{
			if (g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat + Server()->TickSpeed() > Server()->Tick())
				return;

			CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
			if (!str_utf8_check(pMsg->m_pMessage))
			{
				return;
			}
			int Team = pMsg->m_Team ? pPlayer->GetTeam() : CGameContext::CHAT_ALL;

			// trim right and set maximum length to 128 utf8-characters
			int Length = 0;
			const char *p = pMsg->m_pMessage;
			const char *pEnd = 0;
			while (*p)
			{
				const char *pStrOld = p;
				int Code = str_utf8_decode(&p);

				// check if unicode is not empty
				if (Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
					(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
					Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
				{
					pEnd = 0;
				}
				else if (pEnd == 0)
					pEnd = pStrOld;

				if (++Length >= 127)
				{
					*(const_cast<char *>(p)) = 0;
					break;
				}
			}
			if (pEnd != 0)
				*(const_cast<char *>(pEnd)) = 0;

			// drop empty and autocreated spam messages (more than 16 characters per second)
			if (Length == 0 || (pMsg->m_pMessage[0] != '/' && g_Config.m_SvSpamprotection && pPlayer->m_LastChat && pPlayer->m_LastChat + Server()->TickSpeed() * ((15 + Length) / 16) > Server()->Tick()))
				return;

			pPlayer->m_LastChat = Server()->Tick();

			if (pMsg->m_pMessage[0] == '/' || pMsg->m_pMessage[0] == '\\')
			{
				switch (m_apPlayers[ClientID]->m_Authed)
				{
				case IServer::AUTHED_ADMIN:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
					break;
				case IServer::AUTHED_MOD:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_MOD);
					break;
				default:
					Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_USER);
				}
				m_ChatResponseTargetID = ClientID;

				Console()->ExecuteLineFlag(pMsg->m_pMessage + 1, ClientID, CFGFLAG_CHAT);

				m_ChatResponseTargetID = -1;
				Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
			}
			else
			{
				SendChat(ClientID, Team, pMsg->m_pMessage);
			}
		}
		else if (MsgID == NETMSGTYPE_CL_CALLVOTE)
		{
			if (g_Config.m_SvSpamprotection && pPlayer->m_LastVoteTry && pPlayer->m_LastVoteTry + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			int64 Now = Server()->Tick();
			pPlayer->m_LastVoteTry = Now;
			if (pPlayer->GetTeam() == TEAM_SPECTATORS)
			{
				SendChatTarget(ClientID, _("Spectators aren't allowed to start a vote."));
				return;
			}

			char aChatmsg[512] = {0};
			char aDesc[VOTE_DESC_LENGTH] = {0};
			char aCmd[VOTE_CMD_LENGTH] = {0};
			bool m_ChatTarget = false;
			CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;
			const char *pReason = pMsg->m_pReason[0] ? pMsg->m_pReason : "No reason given";
			if (str_comp_nocase(pMsg->m_pType, "option") == 0)
			{
				for (int i = 0; i < m_PlayerVotes[ClientID].size(); ++i)
				{
					if (str_comp_nocase(pMsg->m_pValue, m_PlayerVotes[ClientID][i].m_aDescription) == 0)
					{
						str_format(aDesc, sizeof(aDesc), "%s", m_PlayerVotes[ClientID][i].m_aDescription);
						str_format(aCmd, sizeof(aCmd), "%s", m_PlayerVotes[ClientID][i].m_aCommand);

						if (m_VoteCloseTime && !str_startswith(aCmd, "ccv_") && !str_startswith(aCmd, "ccv_abyss"))
						{
							SendChatTarget(ClientID, _("Wait for current vote to end before calling a new one."));
							return;
						}

						if (str_startswith(aCmd, "ccv_abyss") && !IsAbyss())
							return;

						if (!str_startswith(aCmd, "ccv_") && !str_startswith(aCmd, "ccv_abyss"))
							SendChatTarget(-1, _("'{str:PlayerName}' called vote to change server option '{str:Option}' ({str:Reason})"), "PlayerName",
										   Server()->ClientName(ClientID), "Option", m_PlayerVotes[ClientID][i].m_aDescription,
										   "Reason", pReason);
						m_ChatTarget = true;
						m_PlayerVotes[ClientID][i].data;
						break;
					}
				}
			}
			else if (str_comp_nocase(pMsg->m_pType, "kick") == 0)
			{
				if (!g_Config.m_SvVoteKick)
				{
					SendChatTarget(ClientID, _("Server does not allow voting to kick players"));
					return;
				}

				if (g_Config.m_SvVoteKickMin)
				{
					int PlayerNum = 0;
					for (int i = 0; i < MAX_CLIENTS; ++i)
						if (m_apPlayers[i] && m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
							++PlayerNum;

					if (PlayerNum < g_Config.m_SvVoteKickMin)
					{
						str_format(aChatmsg, sizeof(aChatmsg), "Kick voting requires %d players on the server", g_Config.m_SvVoteKickMin);
						SendChatTarget(ClientID, _("Kick voting requires {str:Num} players on the server"), g_Config.m_SvVoteKickMin);
						return;
					}
				}

				int KickID = str_toint(pMsg->m_pValue);
				if (KickID < 0 || KickID >= MAX_CLIENTS || !m_apPlayers[KickID])
				{
					SendChatTarget(ClientID, _("Invalid client id to kick"));
					return;
				}
				if (KickID == ClientID)
				{
					SendChatTarget(ClientID, _("You can't kick yourself"));
					return;
				}

				if (!Server()->ReverseTranslate(KickID, ClientID))
					return;

				if (Server()->IsAuthed(KickID))
				{
					SendChatTarget(ClientID, _("You can't kick admins"));
					char aBufKick[128];
					str_format(aBufKick, sizeof(aBufKick), "'%s' called for vote to kick you", Server()->ClientName(ClientID));
					SendChatTarget(KickID, _("'{str:Player}' called for vote to kick you"), "Player", Server()->ClientName(ClientID));
					return;
				}

				SendChatTarget(-1, "'{str:Player1}' called for vote to kick '{str:Player2}' ({str:Reason})", "Player1", Server()->ClientName(ClientID), "Player2", Server()->ClientName(KickID), "Reason", pReason);
				str_format(aDesc, sizeof(aDesc), "Kick '%s'", Server()->ClientName(KickID));
				if (!g_Config.m_SvVoteKickBantime)
					str_format(aCmd, sizeof(aCmd), "kick %d Kicked by vote", KickID);
				else
				{
					char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
					Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
					str_format(aCmd, sizeof(aCmd), "ban %s %d Banned by vote", aAddrStr, g_Config.m_SvVoteKickBantime);
				}
			}
			else if (str_comp_nocase(pMsg->m_pType, "spectate") == 0)
			{
				if (!g_Config.m_SvVoteSpectate)
				{
					SendChatTarget(ClientID, _("Server does not allow voting to move players to spectators"));
					return;
				}

				int SpectateID = str_toint(pMsg->m_pValue);
				if (SpectateID < 0 || SpectateID >= MAX_CLIENTS || !m_apPlayers[SpectateID] || m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
				{
					SendChatTarget(ClientID, _("Invalid client id to move"));
					return;
				}
				if (SpectateID == ClientID)
				{
					SendChatTarget(ClientID, _("You can't move yourself"));
					return;
				}
				if (!Server()->ReverseTranslate(SpectateID, ClientID))
					return;

				SendChatTarget(-1, "'{str:Player1}' called for vote to move '{str:Player2}' to spectators ({str:Reason})", "Player1", Server()->ClientName(ClientID), "Player2", Server()->ClientName(SpectateID), "Reason", pReason);
				str_format(aDesc, sizeof(aDesc), "move '%s' to spectators", Server()->ClientName(SpectateID));
				str_format(aCmd, sizeof(aCmd), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
			}
			std::string Command(aCmd);
			if (str_comp(aCmd, "ccv_null") == 0)
			{
				return;
			}
			else if (Command.find("ccv_make ") == 0)
			{
				char ItemName[128];
				mem_copy(ItemName, aCmd + 9, sizeof(ItemName));
				if (m_apPlayers[ClientID] && GetPlayerChar(ClientID) && GetPlayerChar(ClientID)->IsAlive())
					MakeItem(ItemName, ClientID);
				return;
			}
			else if (Command.find("ccv_sync") == 0)
			{
				if (m_apPlayers[ClientID] && GetPlayerChar(ClientID) && GetPlayerChar(ClientID)->IsAlive() && m_apPlayers[ClientID]->LoggedIn)
					Sql()->SyncAccountData((ClientID));
				return;
			}

			if (aCmd[0])
			{
				if (!m_ChatTarget)
					SendChat(-1, CGameContext::CHAT_ALL, aChatmsg);
				StartVote(aDesc, aCmd, pReason);
				pPlayer->m_Vote = 1;
				pPlayer->m_VotePos = m_VotePos = 1;
				m_VoteCreator = ClientID;
				pPlayer->m_LastVoteCall = Now;
			}
		}
		else if (MsgID == NETMSGTYPE_CL_VOTE)
		{
			if (!m_VoteCloseTime)
				return;

			if (pPlayer->m_Vote == 0)
			{
				CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
				if (!pMsg->m_Vote)
					return;

				pPlayer->m_Vote = pMsg->m_Vote;
				pPlayer->m_VotePos = ++m_VotePos;
				m_VoteUpdate = true;
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETTEAM && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetTeam *pMsg = (CNetMsg_Cl_SetTeam *)pRawMsg;

			if (pPlayer->GetTeam() == pMsg->m_Team || (g_Config.m_SvSpamprotection && pPlayer->m_LastSetTeam && pPlayer->m_LastSetTeam + Server()->TickSpeed() * 3 > Server()->Tick()))
				return;

			if (pMsg->m_Team != TEAM_SPECTATORS && m_LockTeams)
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				SendBroadcast_VL(_("Teams are locked"), ClientID);
				return;
			}

			if (pPlayer->m_TeamChangeTick > Server()->Tick())
			{
				pPlayer->m_LastSetTeam = Server()->Tick();
				int TimeLeft = (pPlayer->m_TeamChangeTick - Server()->Tick()) / Server()->TickSpeed();
				char aBuf[128];
				char aTime[128];
				str_format(aTime, sizeof(aTime), "%02d:%02d", TimeLeft / 60, TimeLeft % 60);
				str_format(aBuf, sizeof(aBuf), "Time to wait before changing team: %s", aTime);
				SendBroadcast_VL(_("Time to wait before changing team: {str:Time}"), ClientID, "Time", aTime);
				return;
			}

			// Switch team on given client and kill/respawn him
			if (m_pController->CanJoinTeam(pMsg->m_Team, ClientID))
			{
				if (m_pController->CanChangeTeam(pPlayer, pMsg->m_Team))
				{
					pPlayer->m_LastSetTeam = Server()->Tick();
					if (pPlayer->GetTeam() == TEAM_SPECTATORS || pMsg->m_Team == TEAM_SPECTATORS)
						m_VoteUpdate = true;
					pPlayer->SetTeam(pMsg->m_Team);
					(void)m_pController->CheckTeamBalance();
					pPlayer->m_TeamChangeTick = Server()->Tick();
				}
				else
					SendBroadcast_VL(_("Teams must be balanced, please join other team"), ClientID);
			}
			else
			{
				SendBroadcast_VL(_("Only {str:ID} active players are allowed"), ClientID, "ID", ClientID);
			}
		}
		else if (MsgID == NETMSGTYPE_CL_SETSPECTATORMODE && !m_World.m_Paused)
		{
			CNetMsg_Cl_SetSpectatorMode *pMsg = (CNetMsg_Cl_SetSpectatorMode *)pRawMsg;

			if (g_Config.m_SvSpamprotection && pPlayer->m_LastSetSpectatorMode && pPlayer->m_LastSetSpectatorMode + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			if (pMsg->m_SpectatorID != SPEC_FREEVIEW)
				if (!Server()->ReverseTranslate(pMsg->m_SpectatorID, ClientID))
					return;

			if (pPlayer->GetTeam() != TEAM_SPECTATORS || pPlayer->m_SpectatorID == pMsg->m_SpectatorID || ClientID == pMsg->m_SpectatorID)
				return;

			pPlayer->m_LastSetSpectatorMode = Server()->Tick();
			if (pMsg->m_SpectatorID != SPEC_FREEVIEW && (!m_apPlayers[pMsg->m_SpectatorID] || m_apPlayers[pMsg->m_SpectatorID]->GetTeam() == TEAM_SPECTATORS))
				SendChatTarget(ClientID, _("Invalid spectator id used"));
			else
				pPlayer->m_SpectatorID = pMsg->m_SpectatorID;
		}
		else if (MsgID == NETMSGTYPE_CL_CHANGEINFO)
		{
			if (g_Config.m_SvSpamprotection && pPlayer->m_LastChangeInfo && pPlayer->m_LastChangeInfo + Server()->TickSpeed() * 5 > Server()->Tick())
				return;

			CNetMsg_Cl_ChangeInfo *pMsg = (CNetMsg_Cl_ChangeInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set infos
			char aOldName[MAX_NAME_LENGTH];
			str_copy(aOldName, Server()->ClientName(ClientID), sizeof(aOldName));
			Server()->SetClientName(ClientID, pMsg->m_pName);
			if (str_comp(aOldName, Server()->ClientName(ClientID)) != 0)
			{
				SendChatTarget(-1, _("'{str:Old}' changed name to '{str:New}'"), "Old", aOldName, "New", Server()->ClientName(ClientID));
			}
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);

			Server()->ExpireServerInfo();
		}
		else if (MsgID == NETMSGTYPE_CL_EMOTICON && !m_World.m_Paused)
		{
			CNetMsg_Cl_Emoticon *pMsg = (CNetMsg_Cl_Emoticon *)pRawMsg;

			if (g_Config.m_SvSpamprotection && pPlayer->m_LastEmote && pPlayer->m_LastEmote + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			pPlayer->m_LastEmote = Server()->Tick();

			SendEmoticon(ClientID, pMsg->m_Emoticon);
		}
		else if (MsgID == NETMSGTYPE_CL_KILL && !m_World.m_Paused)
		{
			if (pPlayer->m_LastKill && pPlayer->m_LastKill + Server()->TickSpeed() * 3 > Server()->Tick())
				return;

			pPlayer->m_LastKill = Server()->Tick();
			pPlayer->KillCharacter(WEAPON_SELF);
		}
	}
	else
	{
		if (MsgID == NETMSGTYPE_CL_STARTINFO)
		{
			if (pPlayer->m_IsReady)
				return;

			CNetMsg_Cl_StartInfo *pMsg = (CNetMsg_Cl_StartInfo *)pRawMsg;
			pPlayer->m_LastChangeInfo = Server()->Tick();

			// set start infos
			Server()->SetClientName(ClientID, pMsg->m_pName);
			Server()->SetClientClan(ClientID, pMsg->m_pClan);
			Server()->SetClientCountry(ClientID, pMsg->m_Country);
			str_copy(pPlayer->m_TeeInfos.m_SkinName, pMsg->m_pSkin, sizeof(pPlayer->m_TeeInfos.m_SkinName));
			pPlayer->m_TeeInfos.m_UseCustomColor = pMsg->m_UseCustomColor;
			pPlayer->m_TeeInfos.m_ColorBody = pMsg->m_ColorBody;
			pPlayer->m_TeeInfos.m_ColorFeet = pMsg->m_ColorFeet;
			m_pController->OnPlayerInfoChange(pPlayer);

			// send vote options
			CNetMsg_Sv_VoteClearOptions ClearMsg;
			Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);

			CNetMsg_Sv_VoteOptionListAdd OptionMsg;
			int NumOptions = 0;
			OptionMsg.m_pDescription0 = "";
			OptionMsg.m_pDescription1 = "";
			OptionMsg.m_pDescription2 = "";
			OptionMsg.m_pDescription3 = "";
			OptionMsg.m_pDescription4 = "";
			OptionMsg.m_pDescription5 = "";
			OptionMsg.m_pDescription6 = "";
			OptionMsg.m_pDescription7 = "";
			OptionMsg.m_pDescription8 = "";
			OptionMsg.m_pDescription9 = "";
			OptionMsg.m_pDescription10 = "";
			OptionMsg.m_pDescription11 = "";
			OptionMsg.m_pDescription12 = "";
			OptionMsg.m_pDescription13 = "";
			OptionMsg.m_pDescription14 = "";
			CVoteOptionServer *pCurrent = m_pVoteOptionFirst;
			while (pCurrent)
			{
				switch (NumOptions++)
				{
				case 0:
					OptionMsg.m_pDescription0 = pCurrent->m_aDescription;
					break;
				case 1:
					OptionMsg.m_pDescription1 = pCurrent->m_aDescription;
					break;
				case 2:
					OptionMsg.m_pDescription2 = pCurrent->m_aDescription;
					break;
				case 3:
					OptionMsg.m_pDescription3 = pCurrent->m_aDescription;
					break;
				case 4:
					OptionMsg.m_pDescription4 = pCurrent->m_aDescription;
					break;
				case 5:
					OptionMsg.m_pDescription5 = pCurrent->m_aDescription;
					break;
				case 6:
					OptionMsg.m_pDescription6 = pCurrent->m_aDescription;
					break;
				case 7:
					OptionMsg.m_pDescription7 = pCurrent->m_aDescription;
					break;
				case 8:
					OptionMsg.m_pDescription8 = pCurrent->m_aDescription;
					break;
				case 9:
					OptionMsg.m_pDescription9 = pCurrent->m_aDescription;
					break;
				case 10:
					OptionMsg.m_pDescription10 = pCurrent->m_aDescription;
					break;
				case 11:
					OptionMsg.m_pDescription11 = pCurrent->m_aDescription;
					break;
				case 12:
					OptionMsg.m_pDescription12 = pCurrent->m_aDescription;
					break;
				case 13:
					OptionMsg.m_pDescription13 = pCurrent->m_aDescription;
					break;
				case 14:
				{
					OptionMsg.m_pDescription14 = pCurrent->m_aDescription;
					OptionMsg.m_NumOptions = NumOptions;
					Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
					OptionMsg = CNetMsg_Sv_VoteOptionListAdd();
					NumOptions = 0;
					OptionMsg.m_pDescription1 = "";
					OptionMsg.m_pDescription2 = "";
					OptionMsg.m_pDescription3 = "";
					OptionMsg.m_pDescription4 = "";
					OptionMsg.m_pDescription5 = "";
					OptionMsg.m_pDescription6 = "";
					OptionMsg.m_pDescription7 = "";
					OptionMsg.m_pDescription8 = "";
					OptionMsg.m_pDescription9 = "";
					OptionMsg.m_pDescription10 = "";
					OptionMsg.m_pDescription11 = "";
					OptionMsg.m_pDescription12 = "";
					OptionMsg.m_pDescription13 = "";
					OptionMsg.m_pDescription14 = "";
				}
				}
				pCurrent = pCurrent->m_pNext;
			}
			if (NumOptions > 0)
			{
				OptionMsg.m_NumOptions = NumOptions;
				Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
			}

			// send tuning parameters to client
			SendTuningParams(ClientID);

			// client is ready to enter
			pPlayer->m_IsReady = true;
			CNetMsg_Sv_ReadyToEnter m;
			Server()->SendPackMsg(&m, MSGFLAG_VITAL | MSGFLAG_FLUSH, ClientID);

			Server()->ExpireServerInfo();
		}
	}
}

void CGameContext::ConTuneParam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pParamName = pResult->GetString(0);
	float NewValue = pResult->GetFloat(1);

	if (pSelf->Tuning()->Set(pParamName, NewValue))
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%s changed to %.2f", pParamName, NewValue);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
		pSelf->SendTuningParams(-1);
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "No such tuning parameter");
}

void CGameContext::ConTuneReset(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CTuningParams TuningParams;
	*pSelf->Tuning() = TuningParams;
	pSelf->SendTuningParams(-1);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", "Tuning reset");
}

void CGameContext::ConTuneDump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[256];
	for (int i = 0; i < pSelf->Tuning()->Num(); i++)
	{
		float v;
		pSelf->Tuning()->Get(i, &v);
		str_format(aBuf, sizeof(aBuf), "%s %.2f", pSelf->Tuning()->m_apNames[i], v);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tuning", aBuf);
	}
}

void CGameContext::ConPause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->TogglePause();
}

void CGameContext::ConChangeMap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->ChangeMap(pResult->NumArguments() ? pResult->GetString(0) : "");
}

void CGameContext::ConRestart(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (pResult->NumArguments())
		pSelf->m_pController->DoWarmup(pResult->GetInteger(0));
	else
		pSelf->m_pController->StartRound();
}

void CGameContext::ConBroadcast(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendBroadcast(pResult->GetString(0), -1);
}

void CGameContext::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, pResult->GetString(0));
}

void CGameContext::ConSetTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = clamp(pResult->GetInteger(0), 0, (int)MAX_CLIENTS - 1);
	int Team = clamp(pResult->GetInteger(1), -1, 1);
	int Delay = pResult->NumArguments() > 2 ? pResult->GetInteger(2) : 0;
	if (!pSelf->m_apPlayers[ClientID])
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "moved client %d to team %d", ClientID, Team);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	pSelf->m_apPlayers[ClientID]->m_TeamChangeTick = pSelf->Server()->Tick() + pSelf->Server()->TickSpeed() * Delay * 60;
	pSelf->m_apPlayers[ClientID]->SetTeam(Team);
	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConSetTeamAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Team = clamp(pResult->GetInteger(0), -1, 1);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "All players were moved to the %s", pSelf->m_pController->GetTeamName(Team));
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	for (int i = 0; i < MAX_CLIENTS; ++i)
		if (pSelf->m_apPlayers[i])
			pSelf->m_apPlayers[i]->SetTeam(Team, false);

	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConSwapTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SwapTeams();
}

void CGameContext::ConShuffleTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!pSelf->m_pController->IsTeamplay())
		return;

	int CounterRed = 0;
	int CounterBlue = 0;
	int PlayerTeam = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i)
		if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			++PlayerTeam;
	PlayerTeam = (PlayerTeam + 1) / 2;

	pSelf->SendChat(-1, CGameContext::CHAT_ALL, "Teams were shuffled");

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (pSelf->m_apPlayers[i] && pSelf->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			if (CounterRed == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_ZOMBIE, false);
			else if (CounterBlue == PlayerTeam)
				pSelf->m_apPlayers[i]->SetTeam(TEAM_HUMAN, false);
			else
			{
				if (rand() % 2)
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_ZOMBIE, false);
					++CounterBlue;
				}
				else
				{
					pSelf->m_apPlayers[i]->SetTeam(TEAM_HUMAN, false);
					++CounterRed;
				}
			}
		}
	}

	(void)pSelf->m_pController->CheckTeamBalance();
}

void CGameContext::ConLockTeams(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_LockTeams ^= 1;
	if (pSelf->m_LockTeams)
		pSelf->SendChatTarget(-1, _("Teams were locked"));
	else
		pSelf->SendChatTarget(-1, _("Teams were unlocked"));
}

void CGameContext::ConSkipWarmup(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (pSelf->m_pController->m_Warmup)
		pSelf->m_pController->m_Warmup = 1;
}

void CGameContext::ConAddCommandVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);
	char pCommand[256];
	str_format(pCommand, sizeof(pCommand), "ccv_%s", pResult->GetString(1));

	if (pSelf->m_NumVoteOptions == MAX_VOTE_OPTIONS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "maximum number of vote options reached");
		return;
	}

	// check for valid option
	if (str_length(pCommand) >= VOTE_CMD_LENGTH)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid command '%s'", pCommand);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}
	while (*pDescription && *pDescription == ' ')
		pDescription++;
	if (str_length(pDescription) >= VOTE_DESC_LENGTH || *pDescription == 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid option '%s'", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// check for duplicate entry
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while (pOption)
	{
		if (str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "option '%s' already exists", pDescription);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
		pOption = pOption->m_pNext;
	}

	// add the option
	++pSelf->m_NumVoteOptions;
	int Len = str_length(pCommand);

	pOption = (CVoteOptionServer *)pSelf->m_pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
	pOption->m_pNext = 0;
	pOption->m_pPrev = pSelf->m_pVoteOptionLast;
	if (pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	pSelf->m_pVoteOptionLast = pOption;
	if (!pSelf->m_pVoteOptionFirst)
		pSelf->m_pVoteOptionFirst = pOption;

	str_copy(pOption->m_aDescription, pDescription, sizeof(pOption->m_aDescription));
	mem_copy(pOption->m_aCommand, pCommand, Len + 1);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "added option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConAddVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);
	const char *pCommand = pResult->GetString(1);

	if (pSelf->m_NumVoteOptions == MAX_VOTE_OPTIONS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "maximum number of vote options reached");
		return;
	}

	// check for valid option
	if (!pSelf->Console()->LineIsValid(pCommand) || str_length(pCommand) >= VOTE_CMD_LENGTH)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid command '%s'", pCommand);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}
	while (*pDescription && *pDescription == ' ')
		pDescription++;
	if (str_length(pDescription) >= VOTE_DESC_LENGTH || *pDescription == 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "skipped invalid option '%s'", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// check for duplicate entry
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while (pOption)
	{
		if (str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "option '%s' already exists", pDescription);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
		pOption = pOption->m_pNext;
	}

	// add the option
	++pSelf->m_NumVoteOptions;
	int Len = str_length(pCommand);

	pOption = (CVoteOptionServer *)pSelf->m_pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
	pOption->m_pNext = 0;
	pOption->m_pPrev = pSelf->m_pVoteOptionLast;
	if (pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	pSelf->m_pVoteOptionLast = pOption;
	if (!pSelf->m_pVoteOptionFirst)
		pSelf->m_pVoteOptionFirst = pOption;

	str_copy(pOption->m_aDescription, pDescription, sizeof(pOption->m_aDescription));
	mem_copy(pOption->m_aCommand, pCommand, Len + 1);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "added option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);
}

void CGameContext::ConRemoveVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pDescription = pResult->GetString(0);

	// check for valid option
	CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
	while (pOption)
	{
		if (str_comp_nocase(pDescription, pOption->m_aDescription) == 0)
			break;
		pOption = pOption->m_pNext;
	}
	if (!pOption)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "option '%s' does not exist", pDescription);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
		return;
	}

	// inform clients about removed option
	CNetMsg_Sv_VoteOptionRemove OptionMsg;
	OptionMsg.m_pDescription = pOption->m_aDescription;
	pSelf->Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, -1);

	// TODO: improve this
	// remove the option
	--pSelf->m_NumVoteOptions;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "removed option '%s' '%s'", pOption->m_aDescription, pOption->m_aCommand);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	CHeap *pVoteOptionHeap = new CHeap();
	CVoteOptionServer *pVoteOptionFirst = 0;
	CVoteOptionServer *pVoteOptionLast = 0;
	int NumVoteOptions = pSelf->m_NumVoteOptions;
	for (CVoteOptionServer *pSrc = pSelf->m_pVoteOptionFirst; pSrc; pSrc = pSrc->m_pNext)
	{
		if (pSrc == pOption)
			continue;

		// copy option
		int Len = str_length(pSrc->m_aCommand);
		CVoteOptionServer *pDst = (CVoteOptionServer *)pVoteOptionHeap->Allocate(sizeof(CVoteOptionServer) + Len);
		pDst->m_pNext = 0;
		pDst->m_pPrev = pVoteOptionLast;
		if (pDst->m_pPrev)
			pDst->m_pPrev->m_pNext = pDst;
		pVoteOptionLast = pDst;
		if (!pVoteOptionFirst)
			pVoteOptionFirst = pDst;

		str_copy(pDst->m_aDescription, pSrc->m_aDescription, sizeof(pDst->m_aDescription));
		mem_copy(pDst->m_aCommand, pSrc->m_aCommand, Len + 1);
	}

	// clean up
	delete pSelf->m_pVoteOptionHeap;
	pSelf->m_pVoteOptionHeap = pVoteOptionHeap;
	pSelf->m_pVoteOptionFirst = pVoteOptionFirst;
	pSelf->m_pVoteOptionLast = pVoteOptionLast;
	pSelf->m_NumVoteOptions = NumVoteOptions;
}

void CGameContext::ConForceVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	const char *pType = pResult->GetString(0);
	const char *pValue = pResult->GetString(1);
	const char *pReason = pResult->NumArguments() > 2 && pResult->GetString(2)[0] ? pResult->GetString(2) : "No reason given";
	char aBuf[128] = {0};

	if (str_comp_nocase(pType, "option") == 0)
	{
		CVoteOptionServer *pOption = pSelf->m_pVoteOptionFirst;
		while (pOption)
		{
			if (str_comp_nocase(pValue, pOption->m_aDescription) == 0)
			{
				pSelf->SendChatTarget(-1, _("admin forced server option '{str:Option}' ({str:Reason})"), "Option", pValue, "Reason", pReason);
				pSelf->Console()->ExecuteLine(pOption->m_aCommand, -1);
				break;
			}

			pOption = pOption->m_pNext;
		}

		if (!pOption)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' isn't an option on this server", pValue);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
			return;
		}
	}
	else if (str_comp_nocase(pType, "kick") == 0)
	{
		int KickID = str_toint(pValue);
		if (KickID < 0 || KickID >= MAX_CLIENTS || !pSelf->m_apPlayers[KickID])
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to kick");
			return;
		}

		if (!g_Config.m_SvVoteKickBantime)
		{
			str_format(aBuf, sizeof(aBuf), "kick %d %s", KickID, pReason);
			pSelf->Console()->ExecuteLine(aBuf, -1);
		}
		else
		{
			char aAddrStr[NETADDR_MAXSTRSIZE] = {0};
			pSelf->Server()->GetClientAddr(KickID, aAddrStr, sizeof(aAddrStr));
			str_format(aBuf, sizeof(aBuf), "ban %s %d %s", aAddrStr, g_Config.m_SvVoteKickBantime, pReason);
			pSelf->Console()->ExecuteLine(aBuf, -1);
		}
	}
	else if (str_comp_nocase(pType, "spectate") == 0)
	{
		int SpectateID = str_toint(pValue);
		if (SpectateID < 0 || SpectateID >= MAX_CLIENTS || !pSelf->m_apPlayers[SpectateID] || pSelf->m_apPlayers[SpectateID]->GetTeam() == TEAM_SPECTATORS)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "Invalid client id to move");
			return;
		}
		pSelf->SendChatTarget(-1, _("admin moved '{str:Name}' to spectator ({str:Reason})"), "Name", pSelf->Server()->ClientName(SpectateID), "Reason", pReason);
		str_format(aBuf, sizeof(aBuf), "set_team %d -1 %d", SpectateID, g_Config.m_SvVoteSpectateRejoindelay);
		pSelf->Console()->ExecuteLine(aBuf, -1);
	}
}

void CGameContext::ConClearVotes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", "cleared votes");
	CNetMsg_Sv_VoteClearOptions VoteClearOptionsMsg;
	pSelf->Server()->SendPackMsg(&VoteClearOptionsMsg, MSGFLAG_VITAL, -1);
	pSelf->m_pVoteOptionHeap->Reset();
	pSelf->m_pVoteOptionFirst = 0;
	pSelf->m_pVoteOptionLast = 0;
	pSelf->m_NumVoteOptions = 0;
}

void CGameContext::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	// check if there is a vote running
	if (!pSelf->m_VoteCloseTime)
		return;

	if (str_comp_nocase(pResult->GetString(0), "yes") == 0)
	{
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_YES;
		pSelf->SendChatTarget(-1, _("admin forced vote yes"));
	}
	else if (str_comp_nocase(pResult->GetString(0), "no") == 0)
	{
		pSelf->m_VoteEnforce = CGameContext::VOTE_ENFORCE_NO;
		pSelf->SendChatTarget(-1, _("admin forced vote no"));
	}
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "forcing vote %s", pResult->GetString(0));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
}

void CGameContext::ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if (pResult->NumArguments())
	{
		CNetMsg_Sv_Motd Msg;
		Msg.m_pMessage = g_Config.m_SvMotd;
		CGameContext *pSelf = (CGameContext *)pUserData;
		for (int i = 0; i < MAX_CLIENTS; ++i)
			if (pSelf->m_apPlayers[i])
				pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}
}

void CGameContext::ConHelp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;
	int CID = pResult->GetClientID();
	pThis->SendChatTarget(CID, _("Defense Zombies, dont let them touch you base."));
	pThis->SendChatTarget(CID, _("If you team's Tower no health left, you lose."));
	pThis->SendChatTarget(CID, _("Don't try complete all wave, be cause it's Infinite!."));
	pThis->SendChatTarget(CID, _("Zombies made by AssassinTee."));
	pThis->SendChatTarget(CID, _("-------"));
	pThis->SendChatTarget(CID, _("In the map have many 'CKs'."));
	pThis->SendChatTarget(CID, _("The 'heart' is 'CK' log"));
	pThis->SendChatTarget(CID, _("Others I have point at map."));
	pThis->SendChatTarget(CID, _("Just find them then check!"));
	pThis->SendChatTarget(CID, _("You can make tools in Vote, check them."));
}

void CGameContext::ConAbout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;
	pThis->SendChatTarget(pResult->GetClientID(), _("{str:modname} {str:version} by {str:author}"), "modname", MOD_NAME, "version", MOD_VERSION, "author", MOD_AUTHORS);
	if (MOD_CREDITS[0])
		pThis->SendChatTarget(pResult->GetClientID(), _("Credits: {str:c}"), "c", MOD_CREDITS);
	if (MOD_THANKS[0])
		pThis->SendChatTarget(pResult->GetClientID(), _("Thanks to: {str:t}"), "t", MOD_THANKS);
	if (MOD_SOURCES[0])
		pThis->SendChatTarget(pResult->GetClientID(), _("Sources: {str:s}"), "s", MOD_SOURCES);
}

void CGameContext::ConMe(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;
	CPlayer *Player = pThis->m_apPlayers[pResult->GetClientID()];

	if (!Player->LoggedIn)
		pThis->SendChatTarget(pResult->GetClientID(), _("[Warning]: If you dont login or register a account, then when you left. you will lose EVERYTHING in this mod!"));
	int ShowResource;
	dynamic_string buffer;
	for (int i = 0; i < NUM_RESOURCE; i++)
	{
		dynamic_string buf;
		dynamic_string name;
		buf.clear();
		name.clear();
		ShowResource = Player->m_Knapsack.m_Resource[i];
		// dbg_msg);
		const char *IName = pThis->GetItemNameByID(i);
		pThis->Server()->Localization()->Format_L(name, Player->GetLanguage(), _(IName));
		pThis->Server()->Localization()->Format_L(buf, Player->GetLanguage(), _("{str:name}: {int:num}"), "name", name.buffer(), "num", &ShowResource);
		if (i != NUM_RESOURCE - 1)
		{
			buf.append(",");
		}
		buffer.append(buf);

		if (i % 2 == 0)
		{
			pThis->SendChatTarget(pResult->GetClientID(), _(buffer.buffer()));
			buffer.clear();
		}
	}

	pThis->SendChatTarget(pResult->GetClientID(), _(buffer.buffer()));
}

void CGameContext::ConCheckEvent(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;
	CPlayer *Player = pThis->m_apPlayers[pResult->GetClientID()];
	int Timer = pThis->m_EventTimer / pThis->Server()->TickSpeed();
	int End = pThis->m_EventDuration / pThis->Server()->TickSpeed();
	if (Timer)
		pThis->SendChatTarget(pResult->GetClientID(), _("Event will Start at {sec:Timer}"), "Timer", &Timer, NULL);
	else
		pThis->SendChatTarget(pResult->GetClientID(), _("Event will End at {sec:Timer}"), "Timer", &End, NULL);
}

void CGameContext::ConSetEventTimer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;
	CPlayer *Player = pThis->m_apPlayers[pResult->GetClientID()];
	if (pResult->NumArguments())
		pThis->m_EventTimer = pResult->GetInteger(0);
}

void CGameContext::ConLanguage(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int ClientID = pResult->GetClientID();

	const char *pLanguageCode = (pResult->NumArguments() > 0) ? pResult->GetString(0) : 0x0;
	char aFinalLanguageCode[8];
	aFinalLanguageCode[0] = 0;

	if (pLanguageCode)
	{
		if (str_comp_nocase(pLanguageCode, "ua") == 0)
			str_copy(aFinalLanguageCode, "uk", sizeof(aFinalLanguageCode));
		else
		{
			for (int i = 0; i < pSelf->Server()->Localization()->m_pLanguages.size(); i++)
			{
				if (str_comp_nocase(pLanguageCode, pSelf->Server()->Localization()->m_pLanguages[i]->GetFilename()) == 0)
					str_copy(aFinalLanguageCode, pLanguageCode, sizeof(aFinalLanguageCode));
			}
		}
	}

	if (aFinalLanguageCode[0])
	{
		pSelf->SetClientLanguage(ClientID, aFinalLanguageCode);
		pSelf->SendChatTarget(ClientID, _("Language successfully switched to English"));
	}
	else
	{
		const char *pLanguage = pSelf->m_apPlayers[ClientID]->GetLanguage();
		const char *pTxtUnknownLanguage = pSelf->Server()->Localization()->Localize(pLanguage, _("Unknown language or no input language code"));
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_CHAT, "language", pTxtUnknownLanguage);

		pSelf->SendChatTarget(ClientID, pTxtUnknownLanguage);

		dynamic_string BufferList;
		int BufferIter = 0;
		for (int i = 0; i < pSelf->Server()->Localization()->m_pLanguages.size(); i++)
		{
			if (i > 0)
				BufferIter = BufferList.append_at(BufferIter, ", ");
			BufferIter = BufferList.append_at(BufferIter, pSelf->Server()->Localization()->m_pLanguages[i]->GetFilename());
		}

		dynamic_string Buffer;
		pSelf->Server()->Localization()->Format_L(Buffer, pLanguage, _("Available languages: {str:ListOfLanguage}"), "ListOfLanguage", BufferList.buffer(), NULL);
		pSelf->SendChatTarget(ClientID, Buffer.buffer());
	}

	return;
}

void CGameContext::ConClassPassword(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pThis = (CGameContext *)pUserData;
	int ClientID = pResult->GetClientID();
	const char *pMessage = pResult->GetString(0);
	CPlayer *Player = pThis->m_apPlayers[ClientID];

	bool YES;
	if (str_comp(pMessage, g_Config.m_FFS) == 0)
	{
		Player->m_Knapsack.m_FFS = 1;
		YES = true;
	}
	else if (str_comp(pMessage, g_Config.m_Ninecloud) == 0)
	{
		Player->m_Knapsack.m_Ninecloud = 1;
		YES = true;
	}
	else if (str_comp(pMessage, g_Config.m_EDreemurr) == 0)
	{
		Player->m_Knapsack.m_EDreemurr = 1;
		YES = true;
	}
	else if (str_comp(pMessage, g_Config.m_Shengyan) == 0)
	{
		Player->m_Knapsack.m_Shengyan = 1;
		YES = true;
	}
	else if (str_comp(pMessage, g_Config.m_XyCloud) == 0)
	{
		Player->m_Knapsack.m_XyCloud = 1;
		YES = true;
	}
	else if (str_comp(pMessage, g_Config.m_HGDio) == 0)
	{
		Player->m_Knapsack.m_HGDio = 1;
		YES = true;
	}

	if (YES)
		pThis->SendChatTarget(ClientID, _("Nice! U entered to DEVELOPER MODE"));
	else
	{
		pThis->SendChatTarget(ClientID, _("Wrong password!"));
		pThis->SendChatTarget(ClientID, _("If you want get the password, ASK Server hoster"));
	}
}

void CGameContext::SetClientLanguage(int ClientID, const char *pLanguage)
{
	Server()->SetClientLanguage(ClientID, pLanguage);
	if (m_apPlayers[ClientID])
	{
		m_apPlayers[ClientID]->SetLanguage(pLanguage);
		ClearVotes(ClientID);
	}
}

void CGameContext::ConsoleOutputCallback_Chat(const char *pLine, void *pUser)
{
	CGameContext *pSelf = (CGameContext *)pUser;
	int ClientID = pSelf->m_ChatResponseTargetID;

	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;

	const char *pLineOrig = pLine;

	static volatile int ReentryGuard = 0;

	if (ReentryGuard)
		return;
	ReentryGuard++;

	if (*pLine == '[')
		do
			pLine++;
		while ((pLine - 2 < pLineOrig || *(pLine - 2) != ':') && *pLine != 0); // remove the category (e.g. [Console]: No Such Command)

	pSelf->SendChatTarget(ClientID, pLine);

	ReentryGuard--;
}

#ifdef CONF_BOX2D
void CGameContext::CreateGround(vec2 Pos, int Type)
{
	vec2 size(32, 32);
	float angle = 180 * b2_pi;
	b2BodyDef groundBodyDef;
	groundBodyDef.angle = angle;
	groundBodyDef.position = b2Vec2(Pos.x / 30, Pos.y / 30);
	groundBodyDef.type = b2_kinematicBody;
	b2Body *ground = m_b2world->CreateBody(&groundBodyDef);

	b2PolygonShape Shape;
	Shape.SetAsBox(size.x / 2 / 30, size.y / 2 / 30);
	b2FixtureDef FixtureDef;
	FixtureDef.density = 0;
	FixtureDef.shape = &Shape;
	ground->CreateFixture(&FixtureDef);
}

void CGameContext::ConB2CreateBox(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CCharacter *Char = pSelf->GetPlayerChar(pResult->GetClientID());
	if (not Char)
		return;

	vec2 size(pResult->GetInteger(0), pResult->GetInteger(1));
	pSelf->m_b2bodies.push_back(new CBox2DBox(&pSelf->m_World, vec2(Char->m_Pos.x, Char->m_Pos.y - 128), size, 0, pSelf->m_b2world, b2_dynamicBody, 1.f));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Box2D", "Created box above you");
}

void CGameContext::ConB2CreateTest(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CCharacter *Char = pSelf->GetPlayerChar(pResult->GetClientID());
	if (not Char)
		return;

	vec2 size(pResult->GetInteger(0), pResult->GetInteger(1));
	pSelf->m_b2Test.push_back(new CBox2DTest(&pSelf->m_World, vec2(Char->m_Pos.x, Char->m_Pos.y - 128), size, pSelf->m_b2world, b2_dynamicBody, 1.f));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Box2D", "Created box above you");
}

void CGameContext::ConB2CreateTestSpider(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CCharacter *Char = pSelf->GetPlayerChar(pResult->GetClientID());
	if (not Char)
		return;

	vec2 size(pResult->GetInteger(0), pResult->GetInteger(1));
	pSelf->m_b2TestSpider.push_back(new CBox2DTestSpider(&pSelf->m_World, vec2(Char->m_Pos.x, Char->m_Pos.y - 128), size, pSelf->m_b2world, b2_dynamicBody, 1.f));
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Box2D", "Created box above you");
}

void CGameContext::ConB2CreateGround(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CCharacter *Char = pSelf->GetPlayerChar(pResult->GetClientID());
	if (not Char)
		return;

	vec2 size(pResult->GetInteger(0), pResult->GetInteger(1));
	float angle = pResult->GetFloat(2) / 180.0f * b2_pi;
	pSelf->m_b2bodies.push_back(new CBox2DBox(&pSelf->m_World, vec2(Char->m_Pos.x, Char->m_Pos.y + 28), size, angle, pSelf->m_b2world, b2_kinematicBody, 0.f));

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Box2D", "Created ground");
}

void CGameContext::ConB2ClearWorld(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	for (unsigned i = 0; i < pSelf->m_b2bodies.size(); i++)
	{
		if (pSelf->m_b2bodies[i])
			delete pSelf->m_b2bodies[i];
	}
	pSelf->m_b2bodies.clear();

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Box2D", "Cleared world");
}

#endif

void CGameContext::OnConsoleInit()
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	m_ChatPrintCBIndex = Console()->RegisterPrintCallback(IConsole::OUTPUT_LEVEL_CHAT, ConsoleOutputCallback_Chat, this);

	Console()->Register("tune", "si", CFGFLAG_SERVER, ConTuneParam, this, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConTuneReset, this, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConTuneDump, this, "Dump tuning");

	Console()->Register("pause", "", CFGFLAG_SERVER, ConPause, this, "Pause/unpause game");
	Console()->Register("change_map", "?r", CFGFLAG_SERVER | CFGFLAG_STORE, ConChangeMap, this, "Change map");
	Console()->Register("restart", "?i", CFGFLAG_SERVER | CFGFLAG_STORE, ConRestart, this, "Restart in x seconds (0 = abort)");
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConBroadcast, this, "Broadcast message");
	Console()->Register("say", "r", CFGFLAG_SERVER, ConSay, this, "Say in chat");
	Console()->Register("set_team", "ii?i", CFGFLAG_SERVER, ConSetTeam, this, "Set team of player to team");
	Console()->Register("set_team_all", "i", CFGFLAG_SERVER, ConSetTeamAll, this, "Set team of all players to team");
	Console()->Register("swap_teams", "", CFGFLAG_SERVER, ConSwapTeams, this, "Swap the current teams");
	Console()->Register("shuffle_teams", "", CFGFLAG_SERVER, ConShuffleTeams, this, "Shuffle the current teams");
	Console()->Register("lock_teams", "", CFGFLAG_SERVER, ConLockTeams, this, "Lock/unlock teams");

	Console()->Register("add_vote", "sr", CFGFLAG_SERVER, ConAddVote, this, "Add a voting option");
	Console()->Register("add_command_vote", "sr", CFGFLAG_SERVER, ConAddCommandVote, this, "Add a voting option");
	Console()->Register("remove_vote", "s", CFGFLAG_SERVER, ConRemoveVote, this, "remove a voting option");
	Console()->Register("force_vote", "ss?r", CFGFLAG_SERVER, ConForceVote, this, "Force a voting option");
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, ConClearVotes, this, "Clears the voting options");
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConVote, this, "Force a vote to yes/no");
	Console()->Register("set_event_timer", "i", CFGFLAG_SERVER, ConSetEventTimer, this, "Set Abyss Event timer.(50 = 1s)");

#ifdef CONF_BOX2D
	Console()->Register("b2_create_box", "ii", CFGFLAG_SERVER, ConB2CreateBox, this, "create a box in the Box2D world using your current position");
	Console()->Register("b2_create_test", "", CFGFLAG_SERVER, ConB2CreateTest, this, "create a box in the Box2D world using your current position");
	Console()->Register("b2_create_test_spider", "", CFGFLAG_SERVER, ConB2CreateTestSpider, this, "create a box in the Box2D world using your current position");
	Console()->Register("b2_create_ground", "ii?i", CFGFLAG_SERVER, ConB2CreateGround, this, "create ground in the Box2D world using your current position");
	Console()->Register("b2_clear_world", "", CFGFLAG_SERVER, ConB2ClearWorld, this, "clear all bodies (except tee bodies) in the Box2D world");
#endif

	Console()->Register("about", "", CFGFLAG_CHAT, ConAbout, this, "Show information about the mod");
	Console()->Register("help", "", CFGFLAG_CHAT, ConHelp, this, "Show information about the mod");
	Console()->Register("language", "?s", CFGFLAG_CHAT, ConLanguage, this, "change language");
	Console()->Register("classpassword", "?s", CFGFLAG_CHAT, ConClassPassword, this, "Show information about the mod");
	Console()->Register("skip_warmup", "", CFGFLAG_SERVER, ConSkipWarmup, this, "Show information about the mod");
	Console()->Register("me", "", CFGFLAG_CHAT, ConMe, this, "Show information about the mod");
	Console()->Register("event", "", CFGFLAG_CHAT, ConCheckEvent, this, "Show information about the mod");


	Console()->Register("register", "?s?s", CFGFLAG_CHAT, ConRegister, this, "Show information about the mod");
	Console()->Register("login", "?s?s", CFGFLAG_CHAT, ConLogin, this, "Show information about the mod");
	Console()->Register("logout", "", CFGFLAG_CHAT, ConLogout, this, "Show information about the mod");

	Console()->Chain("sv_motd", ConchainSpecialMotdupdate, this);
}

void CGameContext::OnInit(/*class IKernel *pKernel*/)
{
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_World.SetGameServer(this);
	m_Events.SetGameServer(this);

	// if(!data) // only load once
	// data = load_data_from_memory(internal_data);

	for (int i = 0; i < NUM_NETOBJTYPES; i++)
		Server()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	m_Layers.Init(Kernel());
	m_Collision.Init(&m_Layers);

	m_pController = new CGameController(this);

	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = m_Layers.GameLayer();
	CTile *pTiles = (CTile *)Kernel()->RequestInterface<IMap>()->GetData(pTileMap->m_Data);

	for (int y = 0; y < pTileMap->m_Height; y++)
	{
		for (int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y * pTileMap->m_Width + x].m_Index;
			vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

#ifdef CONF_BOX2D
			switch (Collision()->GetRealTile(Pos))
			{
			case TILE_SOLID:
			case TILE_NOHOOK:
			{
				if (!Collision()->CheckPoint(Pos.x, Pos.y + 32) || !Collision()->CheckPoint(Pos.x, Pos.y - 32) || !Collision()->CheckPoint(Pos.x + 32, Pos.y) || !Collision()->CheckPoint(Pos.x - 32, Pos.y))
					CreateGround(Pos);
			}
			}
#endif

			if (Index >= ENTITY_OFFSET)
			{
				m_pController->OnEntity(Index - ENTITY_OFFSET, Pos);
			}
		}
	}

	// game.world.insert_entity(game.Controller);

#ifdef CONF_DEBUG
	if (g_Config.m_DbgDummies)
	{
		for (int i = 0; i < g_Config.m_DbgDummies; i++)
		{
			OnClientConnected(MAX_CLIENTS - i - 1);
		}
	}
#endif
}

void CGameContext::OnShutdown(bool ChangeMap)
{
#ifdef CONF_BOX2D
	b2Body *node = m_b2world->GetBodyList();
	while (node)
	{
		b2Body *b = node;
		node = node->GetNext();

		m_b2world->DestroyBody(b);
	}
#endif

	delete m_pController;
	m_pController = 0;
	Clear(ChangeMap);
}

void CGameContext::OnSnap(int ClientID)
{
	// add tuning to demo
	CTuningParams StandardTuning;
	if (ClientID == -1 && Server()->DemoRecorder_IsRecording() && mem_comp(&StandardTuning, &m_Tuning, sizeof(CTuningParams)) != 0)
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&m_Tuning;
		for (unsigned i = 0; i < sizeof(m_Tuning) / sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Server()->SendMsg(&Msg, MSGFLAG_RECORD | MSGFLAG_NOSEND, ClientID);
	}

	m_World.Snap(ClientID);
	m_pController->Snap(ClientID);
	m_Events.Snap(ClientID);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
			m_apPlayers[i]->Snap(ClientID);
	}
}
void CGameContext::OnPreSnap() {}
void CGameContext::OnPostSnap()
{
	m_Events.Clear();
}

bool CGameContext::IsClientReady(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->m_IsReady ? true : false;
}

bool CGameContext::IsClientPlayer(int ClientID)
{
	return m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS ? false : true;
}

const char *CGameContext::GameType() { return m_pController && m_pController->m_pGameType ? m_pController->m_pGameType : ""; }
const char *CGameContext::Version() { return GAME_VERSION; }
const char *CGameContext::NetVersion() { return GAME_NETVERSION; }

IGameServer *CreateGameServer() { return new CGameContext; }

void CGameContext::OnZombie(int ClientID, int Zomb)
{
	if (ClientID >= MAX_CLIENTS || m_apPlayers[ClientID])
		return;

	m_apPlayers[ClientID] = new (ClientID) CPlayer(this, ClientID, 1, Zomb);

	Server()->InitZomb(ClientID);
	m_apPlayers[ClientID]->TryRespawn();
}

void CGameContext::UnsealQianFromAbyss(int ClientID)
{
	if (Qian)
	{
		return;
	}
	for (int i = 0; i < MAX_CLIENTS; i++) //...
	{
		if (!m_apPlayers[i]) // Check if the CID is free
		{
			if (Qian)
				return;
			SendChatTarget(-1, _("You feeling the air around you getting closer to her temperature.."));
			OnZombie(i, 14);
			Qian = true;
			return;
		}
	}
}

void CGameContext::OnZombieKill(int ClientID)
{
	if (!m_apPlayers[ClientID]->GetZomb(15))
	{
		if (m_apPlayers[ClientID] && m_apPlayers[ClientID]->GetCharacter())
			m_apPlayers[ClientID]->DeleteCharacter();
		if (m_apPlayers[ClientID])
			delete m_apPlayers[ClientID];
		m_apPlayers[ClientID] = 0;
	}
	// update spectator modes
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (m_apPlayers[i] && m_apPlayers[i]->m_SpectatorID == ClientID)
			m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
	}
}

int CGameContext::NumZombiesAlive()
{
	int NumZombies;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
			if (m_apPlayers[i]->GetCharacter())
				if (m_apPlayers[i]->GetCharacter()->IsAlive())
					if (m_apPlayers[i]->GetZomb())
						NumZombies++;
	}
	return NumZombies;
}

int CGameContext::NumHumanAlive()
{
	int NumHuman = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (m_apPlayers[i])
			if (m_apPlayers[i]->GetCharacter())
				if (!m_apPlayers[i]->GetZomb())
					NumHuman++;
	}
	return NumHuman;
}

bool CGameContext::GetPaused()
{
	return m_World.m_Paused;
}

void CGameContext::ConRegister(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if (pResult->NumArguments() != 2)
	{
		pSelf->SendChatTarget(pResult->GetClientID(), _("Usage: /register <username> <password>"));
		return;
	}

	char Username[512];
	char Password[512];
	str_copy(Username, pResult->GetString(0), sizeof(Username));
	str_copy(Password, pResult->GetString(1), sizeof(Password));

	char aHash[64];
	Crypt(Password, (const unsigned char *)"d9", 1, 14, aHash);

	pSelf->Sql()->create_account(Username, aHash, pResult->GetClientID());
}

void CGameContext::ConLogin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (pResult->NumArguments() != 2)
	{
		pSelf->SendChatTarget(pResult->GetClientID(), _("usage: /login <username> <password>"));
		return;
	}

	char Username[512];
	char Password[512];
	str_copy(Username, pResult->GetString(0), sizeof(Username));
	str_copy(Password, pResult->GetString(1), sizeof(Password));

	char aHash[64]; // Result
	mem_zero(aHash, sizeof(aHash));
	Crypt(Password, (const unsigned char *)"d9", 1, 14, aHash);

	pSelf->Sql()->login(Username, aHash, pResult->GetClientID());
}

void CGameContext::ConLogout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->LogoutAccount(pResult->GetClientID());
}

void CGameContext::LogoutAccount(int ClientID)
{
	CPlayer *pP = m_apPlayers[ClientID];
	CCharacter *pChr = pP->GetCharacter();
	pP->Logout();
	SendChatTarget(pP->GetCID(), _("Logout succesful"));
}

void CGameContext::InitItems()
{
	// Dont move this item!!!
	CreateItem("checkpoint", // Name
			   m_ItemID,	 // ID
			   ITYPE_SWORD,	 // ItemType
			   2,			 // Damage
			   LEVEL_LOG,	 // Level
			   -1,			 // TurretType
			   90,			 // Proba
			   -1,			 // Speed
			   25,			 // Log
			   0,			 // Coal
			   0,			 // Copper
			   0,			 // Iron
			   0,			 // Gold
			   0,			 // Diamond
			   0			 // Enegry
	);
	// Register Items.
	CreateItem("wooden sword", // Name
			   m_ItemID,	   // ID
			   ITYPE_SWORD,	   // ItemType
			   2,			   // Damage
			   LEVEL_LOG,	   // Level
			   -1,			   // TurretType
			   90,			   // Proba
			   -1,			   // Speed
			   25,			   // Log
			   0,			   // Coal
			   0,			   // Copper
			   0,			   // Iron
			   0,			   // Gold
			   0,			   // Diamond
			   0			   // Enegry
	);
	CreateItem("wooden axe", // Name
			   m_ItemID,	 // ID
			   ITYPE_AXE,	 // ItemType
			   0,			 // Damage
			   LEVEL_LOG,	 // Level
			   -1,			 // TurretType
			   90,			 // Proba
			   10,			 // Speed
			   10,			 // Log
			   0,			 // Coal
			   0,			 // Copper
			   0,			 // Iron
			   0,			 // Gold
			   0,			 // Diamond
			   0			 // Enegry
	);
	CreateItem("wooden pickaxe", // Name
			   m_ItemID,		 // ID
			   ITYPE_PICKAXE,	 // ItemType
			   0,				 // Damage
			   LEVEL_LOG,		 // Level
			   -1,				 // TurretType
			   90,				 // Proba
			   200,				 // Speed
			   25,				 // Log
			   0,				 // Coal
			   0,				 // Copper
			   0,				 // Iron
			   0,				 // Gold
			   0,				 // Diamond
			   0				 // Enegry
	);
	CreateItem("copper axe", // Name
			   m_ItemID,	 // ID
			   ITYPE_AXE,	 // ItemType
			   0,			 // Damage
			   LEVEL_COPPER, // Level
			   -1,			 // TurretType
			   90,			 // Proba
			   17,			 // Speed
			   10,			 // Log
			   0,			 // Coal
			   25,			 // Copper
			   0,			 // Iron
			   0,			 // Gold
			   0,			 // Diamond
			   0			 // Enegry
	);
	CreateItem("copper pickaxe", // Name
			   m_ItemID,		 // ID
			   ITYPE_PICKAXE,	 // ItemType
			   0,				 // Damage
			   LEVEL_COPPER,	 // Level
			   -1,				 // TurretType
			   90,				 // Proba
			   500,				 // Speed
			   10,				 // Log
			   0,				 // Coal
			   25,				 // Copper
			   0,				 // Iron
			   0,				 // Gold
			   0,				 // Diamond
			   0				 // Enegry
	);
	CreateItem("iron sword", // Name
			   m_ItemID,	 // ID
			   ITYPE_SWORD,	 // ItemType
			   7,			 // Damage
			   LEVEL_IRON,	 // Level
			   -1,			 // TurretType
			   90,			 // Proba
			   0,			 // Speed
			   10,			 // Log
			   0,			 // Coal
			   0,			 // Copper
			   25,			 // Iron
			   0,			 // Gold
			   0,			 // Diamond
			   0			 // Enegry
	);
	CreateItem("iron axe", // Name
			   m_ItemID,   // ID
			   ITYPE_AXE,  // ItemType
			   0,		   // Damage
			   LEVEL_IRON, // Level
			   -1,		   // TurretType
			   70,		   // Proba
			   24,		   // Speed
			   10,		   // Log
			   0,		   // Coal
			   0,		   // Copper
			   25,		   // Iron
			   0,		   // Gold
			   0,		   // Diamond
			   0		   // Enegry
	);
	CreateItem("iron pickaxe", // Name
			   m_ItemID,	   // ID
			   ITYPE_PICKAXE,  // ItemType
			   0,			   // Damage
			   LEVEL_IRON,	   // Level
			   -1,			   // TurretType
			   90,			   // Proba
			   500,			   // Speed
			   10,			   // Log
			   0,			   // Coal
			   0,			   // Copper
			   25,			   // Iron
			   0,			   // Gold
			   0,			   // Diamond
			   0			   // Enegry
	);
	CreateItem("golden sword", // Name
			   m_ItemID,	   // ID
			   ITYPE_SWORD,	   // ItemType
			   10,			   // Damage
			   LEVEL_GOLD,	   // Level
			   -1,			   // TurretType
			   80,			   // Proba
			   1500,		   // Speed
			   10,			   // Log
			   0,			   // Coal
			   0,			   // Copper
			   0,			   // Iron
			   25,			   // Gold
			   0,			   // Diamond
			   0			   // Enegry
	);
	CreateItem("golden axe", // Name
			   m_ItemID,	 // ID
			   ITYPE_AXE,	 // ItemType
			   0,			 // Damage
			   LEVEL_GOLD,	 // Level
			   -1,			 // TurretType
			   80,			 // Proba
			   30,			 // Speed
			   10,			 // Log
			   0,			 // Coal
			   0,			 // Copper
			   0,			 // Iron
			   10,			 // Gold
			   0,			 // Diamond
			   0			 // Enegry
	);
	CreateItem("golden pickaxe", // Name
			   m_ItemID,		 // ID
			   ITYPE_PICKAXE,	 // ItemType
			   0,				 // Damage
			   LEVEL_GOLD,		 // Level
			   -1,				 // TurretType
			   90,				 // Proba
			   1500,			 // Speed
			   10,				 // Log
			   0,				 // Coal
			   0,				 // Copper
			   0,				 // Iron
			   25,				 // Gold
			   0,				 // Diamond
			   0				 // Enegry
	);
	CreateItem("diamond sword", // Name
			   m_ItemID,		// ID
			   ITYPE_SWORD,		// ItemType
			   20,				// Damage
			   LEVEL_DIAMOND,	// Level
			   -1,				// TurretType
			   90,				// Proba
			   0,				// Speed
			   10,				// Log
			   0,				// Coal
			   0,				// Copper
			   0,				// Iron
			   0,				// Gold
			   25,				// Diamond
			   0				// Enegry
	);
	CreateItem("diamond axe", // Name
			   m_ItemID,	  // ID
			   ITYPE_AXE,	  // ItemType
			   0,			  // Damage
			   LEVEL_DIAMOND, // Level
			   -1,			  // TurretType
			   90,			  // Proba
			   50,			  // Speed
			   10,			  // Log
			   0,			  // Coal
			   0,			  // Copper
			   0,			  // Iron
			   0,			  // Gold
			   25,			  // Diamond
			   0			  // Enegry
	);
	CreateItem("diamond pickaxe", // Name
			   m_ItemID,		  // ID
			   ITYPE_PICKAXE,	  // ItemType
			   0,				  // Damage
			   LEVEL_DIAMOND,	  // Level
			   -1,				  // TurretType
			   80,				  // Proba
			   3500,			  // Speed
			   10,				  // Log
			   0,				  // Coal
			   0,				  // Copper
			   0,				  // Iron
			   0,				  // Gold
			   25,				  // Diamond
			   0				  // Enegry
	);
	CreateItem("enegry sword", // Name
			   m_ItemID,	   // ID
			   ITYPE_SWORD,	   // ItemType
			   38,			   // Damage
			   LEVEL_ENEGRY,   // Level
			   -1,			   // TurretType
			   90,			   // Proba
			   0,			   // Speed
			   10,			   // Log
			   0,			   // Coal
			   0,			   // Copper
			   0,			   // Iron
			   0,			   // Gold
			   50,			   // Diamond
			   100			   // Enegry
	);
	CreateItem("enegry pickaxe", // Name
			   m_ItemID,		 // ID
			   ITYPE_PICKAXE,	 // ItemType
			   0,				 // Damage
			   LEVEL_ENEGRY,	 // Level
			   -1,				 // TurretType
			   100,				 // Proba
			   10000,			 // Speed
			   5,				 // Log
			   0,				 // Coal
			   0,				 // Copper
			   0,				 // Iron
			   0,				 // Gold
			   35,				 // Diamond
			   20				 // Enegry
	);

	CreateItem("gun turret", // Name
			   m_ItemID,	 // ID
			   ITYPE_TURRET, // ItemType
			   0,			 // Damage
			   LEVEL_LOG,	 // Level
			   TURRET_GUN,	 // TurretType
			   90,			 // Proba
			   0,			 // Speed
			   50,			 // Log
			   0,			 // Coal
			   10,			 // Copper
			   0,			 // Iron
			   0,			 // Gold
			   0,			 // Diamond
			   0			 // Enegry
	);

	CreateItem("shotgun turret", // Name
			   m_ItemID,		 // ID
			   ITYPE_TURRET,	 // ItemType
			   0,				 // Damage
			   LEVEL_LOG,		 // Level
			   TURRET_SHOTGUN,	 // TurretType
			   90,				 // Proba
			   0,				 // Speed
			   75,				 // Log
			   0,				 // Coal
			   20,				 // Copper
			   0,				 // Iron
			   0,				 // Gold
			   0,				 // Diamond
			   0				 // Enegry
	);

	CreateItem("laser turret", // Name
			   m_ItemID,	   // ID
			   ITYPE_TURRET,   // ItemType
			   0,			   // Damage
			   LEVEL_DIAMOND,  // Level
			   TURRET_LASER,   // TurretType
			   90,			   // Proba
			   0,			   // Speed
			   0,			   // Log
			   0,			   // Coal
			   0,			   // Copper
			   0,			   // Iron
			   20,			   // Gold
			   1,			   // Diamond
			   0			   // Enegry
	);

	CreateItem("laser2077 turret", // Name
			   m_ItemID,		   // ID
			   ITYPE_TURRET,	   // ItemType
			   0,				   // Damage
			   LEVEL_ENEGRY,	   // Level
			   TURRET_LASER_2077,  // TurretType
			   90,				   // Proba
			   0,				   // Speed
			   20,				   // Log
			   0,				   // Coal
			   1,				   // Copper
			   0,				   // Iron
			   0,				   // Gold
			   100,				   // Diamond
			   250				   // Enegry
	);

	CreateItem("follow grenade turret", // Name
			   m_ItemID,				// ID
			   ITYPE_TURRET,			// ItemType
			   0,						// Damage
			   LEVEL_IRON,				// Level
			   TURRET_FOLLOW_GRENADE,	// TurretType
			   90,						// Proba
			   0,						// Speed
			   10,						// Log
			   500,						// Coal
			   50,						// Copper
			   0,						// Iron
			   40,						// Gold
			   25,						// Diamond
			   1						// Enegry
	);

	CreateItem("freeze gun turret", // Name
			   m_ItemID,			// ID
			   ITYPE_TURRET,		// ItemType
			   0,					// Damage
			   LEVEL_ENEGRY,		// Level
			   TURRET_LASER_2077,	// TurretType
			   90,					// Proba
			   0,					// Speed
			   10,					// Log
			   10,					// Coal
			   10,					// Copper
			   10,					// Iron
			   20,					// Gold
			   1,					// Diamond
			   1					// Enegry
	);

	CreateItem("shotgun2077 turret", // Name
			   m_ItemID,			 // ID
			   ITYPE_TURRET,		 // ItemType
			   0,					 // Damage
			   LEVEL_GOLD,			 // Level
			   TURRET_SHOTGUN_2077,	 // TurretType
			   90,					 // Proba
			   0,					 // Speed
			   10,					 // Log
			   10,					 // Coal
			   10,					 // Copper
			   10,					 // Iron
			   30,					 // Gold
			   10,					 // Diamond
			   1					 // Enegry
	);
}

void ResetResource(int *Resource)
{
	for (int i = 0; i < NUM_RESOURCE; i++)
		Resource[i] = 0;
}

void CGameContext::InitCrafts()
{
	int Resource[NUM_RESOURCE];

	ResetResource(Resource);
	Resource[Abyss_Agar] = 1;
	Resource[Abyss_LEnegry] = 8;
	Resource[RESOURCE_ENEGRY] = 1;
	CreateCraft("moonlight ingot", m_ItemID, ITYPE_MATERIAL, Abyss_MoonlightIngot, 99, Resource);

	ResetResource(Resource);
	Resource[RESOURCE_IRON] = 5;
	Resource[RESOURCE_GOLD] = 5;
	Resource[RESOURCE_DIAMOND] = 5;
	Resource[Abyss_Agar] = 5;
	Resource[RESOURCE_ZOMBIEHEART] = 5;
	CreateCraft("alloy", m_ItemID, ITYPE_MATERIAL, Abyss_MoonlightIngot, 99, Resource);

	ResetResource(Resource);
	Resource[Abyss_Alloy] = 5;
	Resource[Abyss_MoonlightIngot] = 3;
	Resource[Abyss_Agar] = 10;
	Resource[RESOURCE_ENEGRY] = 1;
	CreateCraft("yuerks", m_ItemID, ITYPE_MATERIAL, Abyss_MoonlightIngot, 99, Resource);

	ResetResource(Resource);
	Resource[Abyss_Yuerks] = 3;
	Resource[Abyss_LEnegry] = 5;
	Resource[Abyss_Agar] = 30;
	CreateCraft("starlight ingot", m_ItemID, ITYPE_MATERIAL, Abyss_MoonlightIngot, 99, Resource);

	ResetResource(Resource);
	Resource[Abyss_ScrapMetal_S] = 5;
	Resource[Abyss_Remnant] = 25;
	Resource[Abyss_MoonlightIngot] = 8;
	Resource[Abyss_Alloy] = 2;
	Resource[Abyss_Yuerks] = 5;
	Resource[Abyss_StarLightIngot] = 5;
	Resource[Abyss_LEnegry] = 2;
	CreateCraft("core energy", m_ItemID, ITYPE_MATERIAL, Abyss_MoonlightIngot, 99, Resource);

	ResetResource(Resource);
	Resource[Abyss_ScrapMetal_S] = 10;
	Resource[Abyss_NuclearWaste_S] = 10;
	Resource[Abyss_Alloy] = 1;
	CreateCraft("core nuclear waste", m_ItemID, ITYPE_MATERIAL, Abyss_MoonlightIngot, 99, Resource);

	ResetResource(Resource);
	Resource[Abyss_Agar] = 1;
	Resource[Abyss_ScrapMetal_S] = 25;
	Resource[Abyss_MoonlightIngot] = 12;
	Resource[Abyss_Alloy] = 25;
	CreateCraft("alloy pickaxe", m_ItemID, ITYPE_PICKAXE, LEVEL_ALLOY, 99, Resource, 17000);

	ResetResource(Resource);
	Resource[Abyss_Enegry_CORE] = 1;
	Resource[Abyss_NuclearWaste_CORE] = 5;
	Resource[Abyss_ScrapMetal] = 5;
	Resource[Abyss_Alloy] = 5;
	Resource[Abyss_Agar] = 20;
	Resource[Abyss_StarLightIngot] = 5;
	CreateCraft("core enegry pickaxe", m_ItemID, ITYPE_PICKAXE, LEVEL_ENEGRY_CORE, 99, Resource, 250000);

	ResetResource(Resource);
}

void CGameContext::CreateItem(const char *pItemName, int ID, int Type, int Damage, int Level, int TurretType, int Proba,
							  int Speed, int Log, int Coal, int Copper, int Iron, int Gold, int Diamond, int Enegry, int ZombieHeart)
{
	CItem data;
	m_vItem.push_back(data);
	m_ItemID = m_vItem.size() - 1;
	m_vItem[m_ItemID].m_Damage = Damage;
	m_vItem[m_ItemID].m_Level = Level;
	m_vItem[m_ItemID].m_Name = pItemName;
	ResetResource(m_vItem[m_ItemID].m_NeedResource);
	m_vItem[m_ItemID].m_NeedResource[RESOURCE_LOG] = Log;
	m_vItem[m_ItemID].m_NeedResource[RESOURCE_COAL] = Coal;
	m_vItem[m_ItemID].m_NeedResource[RESOURCE_COPPER] = Copper;
	m_vItem[m_ItemID].m_NeedResource[RESOURCE_IRON] = Iron;
	m_vItem[m_ItemID].m_NeedResource[RESOURCE_GOLD] = Gold;
	m_vItem[m_ItemID].m_NeedResource[RESOURCE_DIAMOND] = Diamond;
	m_vItem[m_ItemID].m_NeedResource[RESOURCE_ENEGRY] = Enegry;
	m_vItem[m_ItemID].m_NeedResource[RESOURCE_ZOMBIEHEART] = ZombieHeart;
	m_vItem[m_ItemID].m_Proba = Proba;
	m_vItem[m_ItemID].m_Speed = Speed;
	m_vItem[m_ItemID].m_ID = m_ItemID;
	m_vItem[m_ItemID].m_TurretType = TurretType;
	m_vItem[m_ItemID].m_Type = Type;
}

void CGameContext::CreateCraft(const char *pName, int ID, int Type, int Level, int Proba, int *Resource, int Speed)
{
	CItem data;
	m_vItem.push_back(data);
	m_ItemID = m_vItem.size() - 1;
	m_vItem[m_ItemID].m_Damage = -1;
	m_vItem[m_ItemID].m_Level = Level;
	m_vItem[m_ItemID].m_Name = pName;
	for (int i = 0; i < NUM_RESOURCE; i++)
		m_vItem[m_ItemID].m_NeedResource[i] = Resource[i];
	m_vItem[m_ItemID].m_Proba = Proba;
	m_vItem[m_ItemID].m_Speed = Speed;
	m_vItem[m_ItemID].m_ID = m_ItemID;
	m_vItem[m_ItemID].m_TurretType = -1;
	m_vItem[m_ItemID].m_Type = Type;
}

int CGameContext::GetItemId(const char *pItemName)
{
	for (int i = 0; i < m_vItem.size(); i++)
	{
		if (str_comp(m_vItem[i].m_Name, pItemName) == 0)
		{
			return m_vItem[i].m_ID;
		}
	}
	return -1;
}

void CGameContext::SendCantMakeItemChat(int To, int *Resource)
{
	std::string Buffer;
	dynamic_string Buffre;
	CPlayer *p = m_apPlayers[To];
	const char *Lang = p->GetLanguage();
	Buffre.clear();
	Server()->Localization()->Format_L(Buffre, Lang, _("You need at least "), NULL);

	Buffer.append(Buffre.buffer());
	for (int i = 0; i < NUM_RESOURCE; i++)
	{
		if (Resource[i] > 0)
		{
			dynamic_string iname;
			Buffre.clear();
			p->m_Knapsack.m_Resource[i] -= Resource[i];
			Server()->Localization()->Format_L(iname, Lang, _(GetItemNameByID(i)));
			Server()->Localization()->Format_L(Buffre, Lang, _("{int:num} {str:name}, "), "num", &Resource[i], "name", iname.buffer());
			Buffer.append(Buffre.buffer());
		}
	}
	Buffre.clear();
	Server()->Localization()->Format_L(Buffre, Lang, _("But you don't have them."), NULL);
	Buffer.append(Buffre.buffer());
	SendChatTarget(To, Buffer.c_str());
}

void CGameContext::SendMakeItemChat(int To, CItem Item)
{
	dynamic_string Buffer;
	dynamic_string IName;
	CPlayer *p = m_apPlayers[To];
	const char *Lang = p->GetLanguage();

	Server()->Localization()->Format_L(IName, Lang, _(Item.m_Name));
	Server()->Localization()->Format_L(Buffer, Lang, _("You made a {str:ItemName}! Good Job!"), "ItemName", IName.buffer());
	SendChatTarget(To, Buffer.buffer());
}

void CGameContext::SendMakeItemFailedChat(int To, int *Resource)
{
	SendChatTarget(To, _("Bad luck. The production failed..."));
	std::string Buffer;
	dynamic_string Buffre;
	CPlayer *p = m_apPlayers[To];
	const char *Lang = p->GetLanguage();
	Buffre.clear();
	Server()->Localization()->Format_L(Buffre, Lang, _("You lost "), NULL);
	Buffer.append(Buffre.buffer());
	for (int i = 0; i < NUM_RESOURCE; i++)
	{
		if (Resource[i] > 0)
		{
			dynamic_string iname;
			Buffre.clear();
			p->m_Knapsack.m_Resource[i] -= Resource[i];
			Server()->Localization()->Format_L(iname, Lang, _(GetItemNameByID(i)));
			Server()->Localization()->Format_L(Buffre, Lang, _("{int:num} {str:name}, "), "num", &Resource[i], "name", iname.buffer());
			Buffer.append(Buffre.buffer());
		}
	}
	Buffre.clear();
	Server()->Localization()->Format_L(Buffre, Lang, _("Bad luck."), NULL);
	Buffer.append(Buffre.buffer());
	SendChatTarget(To, Buffer.c_str());
}

void CGameContext::MakeItem(const char *pItemName, int ClientID)
{
	if (!m_apPlayers[ClientID])
		return;

	if (!m_apPlayers[ClientID]->GetCharacter())
		return;

	if (!m_apPlayers[ClientID]->GetCharacter()->IsAlive())
		return;

	if (!CheckItemName(pItemName))
	{
		SendChatTarget(ClientID, _("No such item."), NULL);
		return;
	}

	CItem MakeItem = m_vItem[GetItemId(pItemName)];

	for (int i = 0; i < NUM_RESOURCE; i++)
	{
		if (m_apPlayers[ClientID]->m_Knapsack.m_Resource[i] < MakeItem.m_NeedResource[i])
		{
			SendCantMakeItemChat(ClientID, MakeItem.m_NeedResource);
			return;
		}
	}


	Sql()->SyncAccountData(ClientID);

	if (random_int(0, 100) < MakeItem.m_Proba)
	{
		SendMakeItemChat(ClientID, MakeItem);
		int ItemLevel = MakeItem.m_Level;

		for (int i = 0; i < NUM_RESOURCE; i++)
		{
			m_apPlayers[ClientID]->m_Knapsack.m_Resource[i] -= MakeItem.m_NeedResource[i];
		}
		switch (MakeItem.m_Type)
		{
		case ITYPE_AXE:
			if (m_apPlayers[ClientID]->m_Knapsack.m_Axe < ItemLevel)
				m_apPlayers[ClientID]->m_Knapsack.m_Axe = ItemLevel;
			break;

		case ITYPE_PICKAXE:
			if (m_apPlayers[ClientID]->m_Knapsack.m_Pickaxe < ItemLevel)
				m_apPlayers[ClientID]->m_Knapsack.m_Pickaxe = ItemLevel;
			break;

		case ITYPE_SWORD:
			if (m_apPlayers[ClientID]->m_Knapsack.m_Sword < ItemLevel)
				m_apPlayers[ClientID]->m_Knapsack.m_Sword = ItemLevel;
			break;

		case ITYPE_MATERIAL:
		{
			m_apPlayers[ClientID]->m_Knapsack.m_Resource[MakeItem.m_Level] += 1;
			break;
		}

		case ITYPE_TURRET:
		{
			int Lifes;
			switch (MakeItem.m_TurretType)
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
			new CTurret(&m_World, GetPlayerChar(ClientID)->m_Pos, ClientID, MakeItem.m_TurretType, 64, Lifes);
		}
		}
	}
	else
	{
		SendMakeItemFailedChat(ClientID, MakeItem.m_NeedResource);
		return;
	}

	if (m_apPlayers[ClientID]->LoggedIn)
		Sql()->update(ClientID);
}

int CGameContext::GetDmg(int Level)
{
	if (Level <= 0)
		return 0;
	for (int i = 0; m_vItem.size(); i++)
	{
		if (m_vItem[i].m_Level == Level && m_vItem[i].m_Type == ITYPE_SWORD)
		{
			return m_vItem[i].m_Damage;
		}
	}
}

int CGameContext::GetSpeed(int Level, int Type)
{
	if (Level <= 0)
		return 8;
	for (int i = 0; m_vItem.size(); i++)
	{
		if (m_vItem[i].m_Level == Level && m_vItem[i].m_Type == Type)
		{
			return m_vItem[i].m_Speed;
		}
	}
}

bool CGameContext::CheckItemName(const char *pItemName)
{
	for (int i = 0; i < m_vItem.size(); i++)
	{
		if (str_comp(m_vItem[i].m_Name, pItemName) == 0)
		{
			return true;
		}
	}
	return false;
}

const char *CGameContext::GetItemSQLNameByID(int Type)
{
	switch (Type)
	{
	case RESOURCE_LOG:
		return "Log";
		break;

	case RESOURCE_COAL:
		return "Coal";
		break;

	case RESOURCE_COPPER:
		return "Copper";
		break;

	case RESOURCE_IRON:
		return "Iron";
		break;

	case RESOURCE_GOLD:
		return "Gold";
		break;

	case RESOURCE_DIAMOND:
		return "Diamond";
		break;

	case RESOURCE_ENEGRY:
		return "Enegry";
		break;

	case RESOURCE_ZOMBIEHEART:
		return "ZombieHeart";
		break;

	case Abyss_LEnegry:
		return "LightEnegry";
		break;

	case Abyss_Agar:
		return "Agar";
		break;

	case Abyss_ScrapMetal:
		return "ScrapMetal";
		break;

	case Abyss_ScrapMetal_S:
		return "SlagScrapMetal";
		break;

	case Abyss_NuclearWaste_S:
		return "SlagNuclearWaste";
		break;

	case Abyss_Remnant:
		return "Remnant";
		break;

	case Abyss_MoonlightIngot:
		return "MoonlightIngot";
		break;

	case Abyss_Alloy:
		return "Alloy";
		break;

	case Abyss_Yuerks:
		return "Yuerks";
		break;

	case Abyss_StarLightIngot:
		return "StarLightIngot";
		break;

	case Abyss_Enegry_CORE:
		return "CoreEnegry";
		break;

	case Abyss_NuclearWaste_CORE:
		return "CoreNuclearWaste";
		break;

	case Abyss_ConstantFragment:
		return "ConstantFragment";
		break;

	case Abyss_ConstantIngot:
		return "ConstantIngot";
		break;

	case Abyss_DeathAgglomerate:
		return "DeathAgglomerate";
		break;

	case Abyss_Prism:
		return "Prism";
		break;

	case Abyss_PlatinumWildColor:
		return "PlatinumWildColor";
		break;

	case Abyss_Star:
		return "Star";
		break;

	default:
		return "Log";
		break;
	}
}

const char *CGameContext::GetItemNameByID(int Type)
{
	switch (Type)
	{
	case RESOURCE_LOG:
		return "Log";
		break;

	case RESOURCE_COAL:
		return "Coal";
		break;

	case RESOURCE_COPPER:
		return "Copper";
		break;

	case RESOURCE_IRON:
		return "Iron";
		break;

	case RESOURCE_GOLD:
		return "Gold";
		break;

	case RESOURCE_DIAMOND:
		return "Diamond";
		break;

	case RESOURCE_ENEGRY:
		return "Enegry";
		break;

	case RESOURCE_ZOMBIEHEART:
		return "Zombie Heart";
		break;

	case Abyss_LEnegry:
		return "Light Enegry";
		break;

	case Abyss_Agar:
		return "Agar";
		break;

	case Abyss_ScrapMetal:
		return "Scrap Metal";
		break;

	case Abyss_ScrapMetal_S:
		return "Slag Scrap Metal";
		break;

	case Abyss_NuclearWaste_S:
		return "Slag Nuclear Waste";
		break;

	case Abyss_Remnant:
		return "Remnant";
		break;

	case Abyss_MoonlightIngot:
		return "Moonlight Ingot";
		break;

	case Abyss_Alloy:
		return "Alloy";
		break;

	case Abyss_Yuerks:
		return "Yuerks";
		break;

	case Abyss_StarLightIngot:
		return "StarLight Ingot";
		break;

	case Abyss_Enegry_CORE:
		return "Core Enegry";
		break;

	case Abyss_NuclearWaste_CORE:
		return "Core Nuclear Waste";
		break;

	case Abyss_ConstantFragment:
		return "Constant Fragment";
		break;

	case Abyss_ConstantIngot:
		return "Constant Ingot";
		break;

	case Abyss_DeathAgglomerate:
		return "Death Agglomerate";
		break;

	case Abyss_Prism:
		return "Prism";
		break;

	case Abyss_PlatinumWildColor:
		return "Platinum Wild Color";
		break;

	case Abyss_Star:
		return "Star";
		break;

	default:
		return "Log";
		break;
	}
}

bool CGameContext::IsAbyss()
{
	return str_comp(g_Config.m_SvMap, g_Config.m_SvAbyssMap) == 0;
}

// MMOTee
void CGameContext::AddVote(const char *Desc, const char *Cmd, int ClientID)
{
	while (*Desc && *Desc == ' ')
		Desc++;

	if (ClientID == -2)
		return;

	CVoteOptions Vote;
	str_copy(Vote.m_aDescription, Desc, sizeof(Vote.m_aDescription));
	str_copy(Vote.m_aCommand, Cmd, sizeof(Vote.m_aCommand));
	m_PlayerVotes[ClientID].add(Vote);

	// inform clients about added option
	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = Vote.m_aDescription;
	Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
}

void CGameContext::AddVote_VL(int To, const char *aCmd, const char *pText, ...)
{
	int Start = (To < 0 ? 0 : To);
	int End = (To < 0 ? MAX_CLIENTS : To + 1);

	dynamic_string Buffer;

	va_list VarArgs;
	va_start(VarArgs, pText);

	for (int i = Start; i < End; i++)
	{
		if (m_apPlayers[i])
		{
			Buffer.clear();
			Server()->Localization()->Format_VL(Buffer, m_apPlayers[i]->GetLanguage(), pText, VarArgs);
			AddVote(Buffer.buffer(), aCmd, i);
		}
	}

	Buffer.clear();
	va_end(VarArgs);
}

void CGameContext::InitVotes(int ClientID)
{
	AddVote_VL(ClientID, "ccv_null", _("解锁完整功能请加服务器官方群"));
	AddVote_VL(ClientID, "ccv_null", _("Q群群号:895105949"));
	AddVote_VL(ClientID, "ccv_null", _("==================="));
	AddVote_VL(ClientID, "skip_warmup", _("跳过热身,开始召唤僵尸（谨慎选择）"));
	AddVote_VL(ClientID, "ccv_null", _("==================="));
	AddVote_VL(ClientID, "ccv_sync", _("点我同步账号数据！（用于被赠送物品/QQ群内签到后使用）"));
	AddVote_VL(ClientID, "sv_map TDef-Shwar", _("地图：深层矿井"));
	AddVote_VL(ClientID, "sv_map TDef-Zero", _("地图：0号地区"));
	AddVote_VL(ClientID, "sv_map TDef-Deeply", _("地图：继续深入"));
	AddVote_VL(ClientID, "sv_map Mine-Craft", _("地图：我的世界"));
	AddVote_VL(ClientID, "sv_map yours_minecrafts", _("地图：你的《Minecraft》"));
	AddVote_VL(ClientID, "sv_abyss_map TDef-Abyss;sv_map TDef-Abyss", _("地图：深渊-永无止境"));
	AddVote_VL(ClientID, "sv_abyss_map TDef-City;sv_map TDef-City", _("地图：深渊-古代城市"));
	AddVote_VL(ClientID, "ccv_null", _("======= 材料 ======="));
	AddVote_VL(ClientID, "ccv_make moonlight ingot", _("合成月光锭"));
	AddVote_VL(ClientID, "ccv_make alloy", _("合成合金锭"));
	AddVote_VL(ClientID, "ccv_make yuerks", _("合成跃尔克斯"));
	AddVote_VL(ClientID, "ccv_make starlight ingot", _("合成星光锭"));
	AddVote_VL(ClientID, "ccv_make core energy", _("合成能量核心"));
	AddVote_VL(ClientID, "ccv_make core nuclear waste", _("合成核废料融体"));
	AddVote_VL(ClientID, "ccv_null", _("======= 工具 ======="));
	AddVote_VL(ClientID, "ccv_null", _("------ 斧头 ------"));
	AddVote_VL(ClientID, "ccv_make wooden axe", _("制作木斧"));
	AddVote_VL(ClientID, "ccv_make copper axe", _("制作铜斧"));
	AddVote_VL(ClientID, "ccv_make iron axe", _("制作铁斧"));
	AddVote_VL(ClientID, "ccv_make golden axe", _("制作金斧"));
	AddVote_VL(ClientID, "ccv_make diamond axe", _("制作钻石斧"));
	AddVote_VL(ClientID, "ccv_null", _("------ 镐子 ------"));
	AddVote_VL(ClientID, "ccv_make wooden pickaxe", _("制作木镐"));
	AddVote_VL(ClientID, "ccv_make copper pickaxe", _("制作铜镐"));
	AddVote_VL(ClientID, "ccv_make iron pickaxe", _("制作铁镐"));
	AddVote_VL(ClientID, "ccv_make golden pickaxe", _("制作金镐"));
	AddVote_VL(ClientID, "ccv_make diamond pickaxe", _("制作钻石镐"));
	AddVote_VL(ClientID, "ccv_make enegry pickaxe", _("制作宇宙无敌终极能量镐"));
	AddVote_VL(ClientID, "ccv_make alloy pickaxe", _("制作合金镐"));
	AddVote_VL(ClientID, "ccv_make core enegry pickaxe", _("制作宇宙无敌超级终极全世界第一核心·能量镐"));
	AddVote_VL(ClientID, "ccv_null", _("------ 短剑 ------"));
	AddVote_VL(ClientID, "ccv_make wooden sword", _("制作木剑"));
	AddVote_VL(ClientID, "ccv_make iron sword", _("制作铁剑"));
	AddVote_VL(ClientID, "ccv_make golden sword", _("制作金剑"));
	AddVote_VL(ClientID, "ccv_make diamond sword", _("制作钻石剑"));
	AddVote_VL(ClientID, "ccv_make enegry sword", _("制作宇宙无敌终极能量剑"));
	AddVote_VL(ClientID, "ccv_null", _("======= 炮塔 ======="));
	AddVote_VL(ClientID, "ccv_null", _("= 会在你所在的位置生成炮塔"));
	AddVote_VL(ClientID, "ccv_make gun turret", _("制作普通手枪炮塔"));
	AddVote_VL(ClientID, "ccv_make shotgun turret", _("制作普通散弹枪炮塔"));
	AddVote_VL(ClientID, "ccv_make follow grenade turret", _("制作跟踪榴弹炮塔"));
	AddVote_VL(ClientID, "ccv_make laser turret", _("制作激光枪炮塔"));
	AddVote_VL(ClientID, "ccv_make shotgun2077 turret", _("制作<[xX__=散弹:2077=__Xx]>炮塔"));
	AddVote_VL(ClientID, "ccv_make freeze gun turret", _("制作冰冻枪炮塔"));
	AddVote_VL(ClientID, "ccv_make laser2077 turret", _("制作<[xX__=激光炮塔:2077=__Xx]>"));
}

void CGameContext::ClearVotes(int ClientID)
{
	m_PlayerVotes[ClientID].clear();

	// send vote options
	CNetMsg_Sv_VoteClearOptions ClearMsg;
	Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);

	InitVotes(ClientID);
}

void CGameContext::GoToAbyss(CQian *Victim)
{
	m_World.DestroyEntity(Victim);
	str_copy(g_Config.m_SvMap, g_Config.m_SvAbyssMap, sizeof(g_Config.m_SvMap));
}

void CGameContext::OnUpdatePlayerServerInfo(char *aBuf, int BufSize, int ID)
{
	if (!m_apPlayers[ID])
		return;

	char aCSkinName[64];

	CPlayer::CTeeInfo &TeeInfo = m_apPlayers[ID]->m_TeeInfos;

	char aJsonSkin[400];
	aJsonSkin[0] = '\0';

	// 0.6
	if (TeeInfo.m_UseCustomColor)
	{
		str_format(aJsonSkin, sizeof(aJsonSkin),
				   "\"name\":\"%s\","
				   "\"color_body\":%d,"
				   "\"color_feet\":%d",
				   EscapeJson(aCSkinName, sizeof(aCSkinName), TeeInfo.m_SkinName),
				   TeeInfo.m_ColorBody,
				   TeeInfo.m_ColorFeet);
	}
	else
	{
		str_format(aJsonSkin, sizeof(aJsonSkin),
				   "\"name\":\"%s\"",
				   EscapeJson(aCSkinName, sizeof(aCSkinName), TeeInfo.m_SkinName));
	}

	str_format(aBuf, BufSize,
			   ",\"skin\":{"
			   "%s"
			   "},"
			   "\"afk\":%s,"
			   "\"team\":%d",
			   aJsonSkin,
			   JsonBool(false),
			   m_apPlayers[ID]->GetTeam());
}