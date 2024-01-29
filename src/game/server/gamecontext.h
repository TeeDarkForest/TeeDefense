/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#ifdef CONF_BOX2D
#include <box2d/box2d.h>
#endif

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <teeuniverses/components/localization.h>

#include <game/layers.h>
#include <game/voting.h>

#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"
#include <vector>
#include "teedefense/Item/item.h"

#include "GameCore/TWorldController.h"
#include "GameCore/Database/DB.h"

#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#include "mask128.h"

#include "botengine.h"

#include <engine/storage.h>

/*
	Tick
		Game Context (CGameContext::Tick)
			Game World (CGameWorld::Tick)
				Reset world if requested (CGameWorld::Reset)
				All entities in the world (CEntity::Tick)
				All entities in the world (CEntity::TickDefered)
				Remove entities marked for deletion (CGameWorld::RemoveEntity)
			Game Controller (CGameController::Tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::Snap)
			Game World (CGameWorld::Snap)
				All Entities in the world (CEntity::Snap)
			Game Controller (CGameController::Snap)
			Events handler (CEventHandler::Snap)
			All players (CPlayer::Snap)

*/

#ifdef CONF_BOX2D

class CBox2DBox;
class CBox2DTest;
class CBox2DTestSpider;

class BodyRangeRay : public b2RayCastCallback
{
public:
	b2Body *m_body;
	b2Vec2 m_point;
	b2Vec2 m_normal;
	float m_fraction;

	float ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal, float fraction)
	{
		m_body = fixture->GetBody();
		m_point = point;
		m_normal = normal;
		m_fraction = fraction;
		return 0;
	}
};

class BodyCollideQuery : public b2QueryCallback
{
public:
	b2Body *Body;
	b2Vec2 findPos;

	bool ReportFixture(b2Fixture *fixture)
	{
		b2Shape *shape = fixture->GetShape();
		bool inside = shape->TestPoint(fixture->GetBody()->GetTransform(), findPos);

		if (inside)
		{
			Body = fixture->GetBody();
			return false;
		}
		return true;
	}
};
#endif

class CGameContext : public IGameServer
{
	IServer *m_pServer;
	IStorage *m_pStorage;
	class IConsole *m_pConsole;
	class TWorldController *m_pTWorldController;
	CLayers m_Layers;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;

	CDB *m_pDB;

	static void ConsoleOutputCallback_Chat(const char *pLine, void *pUser);

	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConPause(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static void ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	static void ConSetEventTimer(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConStatusDB(IConsole::IResult *pResult, void *pUser);

	static void ConLanguage(IConsole::IResult *pResult, void *pUserData);
	static void ConAbout(IConsole::IResult *pResult, void *pUserData);

	static void ConClassPassword(IConsole::IResult *pResult, void *pUserData);
	static void ConCraft(IConsole::IResult *pResult, void *pUserData);
	static void ConHelp(IConsole::IResult *pResult, void *pUserData);
	static void ConAddCommandVote(IConsole::IResult *pResult, void *pUserData);
	static void ConSkipWarmup(IConsole::IResult *pResult, void *pUserData);
	static void ConMe(IConsole::IResult *pResult, void *pUserData);
	static void ConLogout(IConsole::IResult *pResult, void *pUserData);
	static void ConRegister(IConsole::IResult *pResult, void *pUserData);
	static void ConLogin(IConsole::IResult *pResult, void *pUserData);
	static void ConCheckEvent(IConsole::IResult *pResult, void *pUserData);

	// Vote Command
	static void ConMake(IConsole::IResult *pResult, void *pUserData);
	static void ConUse(IConsole::IResult *pResult, void *pUserData);
	static void ConSync(IConsole::IResult *pResult, void *pUserData);

#ifdef CONF_BOX2D
	static void ConB2CreateBox(IConsole::IResult *pResult, void *pUserData);
	static void ConB2CreateTest(IConsole::IResult *pResult, void *pUserData);
	static void ConB2CreateTestSpider(IConsole::IResult *pResult, void *pUserData);
	static void ConB2CreateCrank(IConsole::IResult *pResult, void *pUserData);
	static void ConB2CreateGround(IConsole::IResult *pResult, void *pUserData);
	static void ConB2ClearWorld(IConsole::IResult *pResult, void *pUserData);
#endif

	CGameContext(int Resetting, bool ChangeMap);
	void Construct(int Resetting, bool ChangeMap);

	bool m_Resetting;

	// Zomb2
	int m_MessageReturn;

public:
	int m_ChatResponseTargetID;
	int m_ChatPrintCBIndex;

#ifdef CONF_BOX2D
public:
	b2World *m_b2world;
	std::vector<CBox2DBox *> m_b2bodies;
	std::vector<CBox2DTest *> m_b2Test;
	std::vector<CBox2DTestSpider *> m_b2TestSpider;
	std::vector<b2Body *> m_b2explosions;

	vec2 B2Vec2ToVec2(b2Vec2 b2vec2, float Scale)
	{
		return vec2(b2vec2.x * Scale, b2vec2.y * Scale);
	}

	void CreateGround(vec2 Pos, int Type = 0);
	void HandleBox2D();

private:
	struct Actor
	{
		bool m_Dead;

		bool IsDead() { return m_Dead; }
	};
#endif

public:
	IServer *Server() const { return m_pServer; }
	IStorage *Storage() const { return m_pStorage; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	virtual class CLayers *Layers() { return &m_Layers; }

	/* SQL */
	CDB *DB() { return m_pDB; }
	TWorldController *TW() const { return m_pTWorldController; };
	void LogoutAccount(int ClientID);

	CGameContext();
	~CGameContext();

	void Clear(bool ChangeMap);

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	CGameController *m_pController;
	CGameWorld m_World;

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);

	int m_LockTeams;

	// voting
	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientID);

	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_NumVoteOptions;
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN = 0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	class CBotEngine *m_pBotEngine;

	// MMOTee
	struct CVoteOptions
	{
		char m_aDescription[VOTE_DESC_LENGTH] = {0};
		char m_aCommand[VOTE_CMD_LENGTH] = {0};
		void *data = {0};
	};
	array<CVoteOptions> m_PlayerVotes[MAX_CLIENTS];

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, Mask128 Mask = Mask128());
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, Mask128 Mask = Mask128());
	void CreateHammerHit(vec2 Pos, Mask128 Mask = Mask128());
	void CreatePlayerSpawn(vec2 Pos, Mask128 Mask = Mask128());
	void CreateDeath(vec2 Pos, int Who, Mask128 Mask = Mask128());
	void CreateSound(vec2 Pos, int Sound, Mask128 Mask = Mask128());
	void CreateSoundGlobal(int Sound, int Target = -1);

	int NumZombiesAlive();
	int NumHumanAlive();

	enum
	{
		CHAT_ALL = -2,
		CHAT_SPEC = -1,
		CHAT_RED = 0,
		CHAT_BLUE = 1
	};

	// network
	void SendChatTarget(int To, const char *pText, ...);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(const char *pText, int ClientID);
	void SendBroadcast_VL(int ClientID, const char *pText, ...);
	void SetClientLanguage(int ClientID, const char *pLanguage);
	// MMOTee
	void AddVote_VL(int To, const char *aCmd, const char *pText, ...);
	void AddVote(const char *Desc, const char *Cmd, int ClientID = -1);

	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);

	// engine events
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnShutdown(bool ChangeMap);

	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientID);
	virtual void OnPostSnap();

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	virtual void OnClientConnected(int ClientID);
	virtual void OnClientEnter(int ClientID);
	virtual void OnClientDrop(int ClientID, const char *pReason);
	virtual void OnClientDirectInput(int ClientID, void *pInput);
	virtual void OnClientPredictedInput(int ClientID, void *pInput);

	virtual bool IsClientReady(int ClientID);
	virtual bool IsClientPlayer(int ClientID);

	virtual void OnSetAuthed(int ClientID, int Level);

	virtual const char *GameType();
	virtual const char *Version();
	virtual const char *NetVersion();

	// Tee Defense
	bool m_NeedResetTower;
	bool GetPaused();
	int m_TowerHealth;

	void InitCrafts();
	void CreateItem(const char *pItemName, int ID, int Type, int Damage, int Level, int TurretType, int Proba,
					int Speed, int NeedItems[NUM_ITEM], int Life = -1);
	
	const char *GetItemNameByID(int Type);

	void InitVotes(int ClientID);
	void ClearVotes(int ClientID);

	struct CItem
	{
		const char *m_Name;
		int m_Type;
		int m_NeedResource[NUM_ITEM];
		int m_Proba;
		int m_Level;
		int m_Damage;
		int m_Speed;
		int m_ID;
		int m_TurretType;
		int m_Life;
	};

	int GetItemId(const char *pItemName);

	int GetSpeed(int Level, int Type);

	int GetDmg(int Level);

	void MakeItem(int ItemID, int ClientID);

	bool CheckItemName(const char *pItemName);

	void SendCantMakeItemChat(int To, int *Resource);

	void SendMakeItemChat(int To, CItem Item);

	void SendMakeItemFailedChat(int To, int *Resource);

	CItem m_Items[NUM_ITEM];

	int m_EventTimer;
	int m_EventType;
	int m_EventDuration;

	void OnUpdatePlayerServerInfo(char *aBuf, int BufSize, int ID) override;

public:
	CPlayer *GetPlayer(int ClientID, bool CheckAuthed = false, bool CheckCharacter = false);
	enum
	{
		TABLE_ACCOUNT = 0,
		TABLE_ITEM,
	};

	void PutTurret(int Type, int Owner, int Life, int Radius);
	void AddVote_Make(int ClientID, int Type);

public:
	int NumPlayers(bool CheckCharacter = false);

	// Bot slots
	bool AddBot(int i);
	void OnZombieKill(int ClientID);

	void MakeBotSleep(int i); // Bot-only
	void WakeBotUp(int i); // Bot-only

	void InitBots();

	CItem_F *m_pItemF;
	CItem_F *ItemF() { return m_pItemF; }
};

inline Mask128 CmaskAll() { return Mask128(); }
inline Mask128 CmaskOne(int ClientID) { return Mask128(ClientID); }
inline Mask128 CmaskUnset(Mask128 Mask, int ClientID) { return Mask ^ CmaskOne(ClientID); }
inline Mask128 CmaskAllExceptOne(int ClientID) { return CmaskUnset(CmaskAll(), ClientID); }
inline bool CmaskIsSet(Mask128 Mask, int ClientID) { return (Mask & CmaskOne(ClientID)) != 0; }

#endif
