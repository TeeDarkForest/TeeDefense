/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H
#include <box2d/box2d.h>

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
#include "item.h"

#ifdef CONF_SQL
#include "OnTime/sql.h"
#endif

#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/

class CBox2DBox;
class CBox2DTest;
class CBox2DCrank;

class BodyRangeRay : public b2RayCastCallback
{
public:
	b2Body* m_body;
	b2Vec2 m_point;
	b2Vec2 m_normal;
	float m_fraction;

	float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction)
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
	b2Body* Body;
	b2Vec2 findPos;

	bool ReportFixture(b2Fixture* fixture)
	{
		b2Shape* shape = fixture->GetShape();
		bool inside = shape->TestPoint(fixture->GetBody()->GetTransform(), findPos);

		if (inside)
		{
			Body = fixture->GetBody();
			return false;
		}
		return true;
	}
};


class CGameContext : public IGameServer
{
	IServer *m_pServer;
	class IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;

	#ifdef CONF_SQL
	/* SQL */
	CSQL *m_Sql;
	CAccountData *m_AccountData;
	#endif

	static void ConsoleOutputCallback_Chat(const char *pLine, void *pUser);

	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConPause(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static void ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConLockTeams(IConsole::IResult *pResult, void *pUserData);
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static void ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

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

	static void ConB2CreateBox(IConsole::IResult *pResult, void *pUserData);
	static void ConB2CreateTest(IConsole::IResult *pResult, void *pUserData);
	static void ConB2CreateCrank(IConsole::IResult *pResult, void *pUserData);
	static void ConB2CreateGround(IConsole::IResult *pResult, void *pUserData);
	static void ConB2ClearWorld(IConsole::IResult *pResult, void *pUserData);
	

	CGameContext(int Resetting, bool ChangeMap);
	void Construct(int Resetting, bool ChangeMap);

	bool m_Resetting;

	//Zomb2
	int m_MessageReturn;
public:
	int m_ChatResponseTargetID;
	int m_ChatPrintCBIndex;

public:
	b2World *m_b2world;
	std::vector<CBox2DBox*> m_b2bodies;
	std::vector<CBox2DTest*> m_b2Test;
	std::vector<b2Body*> m_b2explosions;

	void CreateGround(vec2 Pos, int Type = 0);
	void HandleBox2D();

private:
	struct Actor
	{
		bool m_Dead;

		bool IsDead() {return m_Dead;}
	};

public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	virtual class CLayers *Layers() { return &m_Layers; }

	#ifdef CONF_SQL
	/* SQL */
	CSQL *Sql() const { return m_Sql; };
	CAccountData *AccountData() {return m_AccountData; };
	void LogoutAccount(int ClientID);
	#endif
	
	CGameContext();
	~CGameContext();

	void Clear(bool ChangeMap);

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	IGameController *m_pController;
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
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, int64_t Mask=-1LL);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, int64_t Mask=-1LL);
	void CreateHammerHit(vec2 Pos, int64_t Mask=-1LL);
	void CreatePlayerSpawn(vec2 Pos, int64_t Mask=-1LL);
	void CreateDeath(vec2 Pos, int Who, int64_t Mask=-1LL);
	void CreateSound(vec2 Pos, int Sound, int64_t Mask=-1LL);
	void CreateSoundGlobal(int Sound, int Target=-1);


	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	// network
	void SendChatTarget(int To, const char *pText, ...);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(const char *pText, int ClientID);
	void SendBroadcast_VL(const char *pText, int ClientID, ...);
	void SetClientLanguage(int ClientID, const char *pLanguage);



	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);

	//
	void SwapTeams();

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

	virtual void OnSetAuthed(int ClientID,int Level);
	
	virtual const char *GameType();
	virtual const char *Version();
	virtual const char *NetVersion();

	//Zomb2
	void OnZombie(int ClientID, int Zomb);
	void OnZombieKill(int ClientID);

	// Tee Defense
	bool m_NeedResetTower;
	bool GetPaused();
	int m_TowerHealth;

	int m_ItemID;
	void InitItems();
	void CreateItem(const char* pItemName, int ID, int Type, int Damage, int Level, int TurretType, int Proba, 
		        int Speed, int Log, int Coal, int Copper, int Iron, int Gold, int Diamond, int Enegry, int ZombieHeart = 0);
	struct CItem
	{
		const char* m_Name;
    	int m_Type;
    	int m_NeedResource[NUM_RESOURCE];
    	int m_Proba;
    	int m_Level;
    	int m_Damage;
    	int m_Speed;
    	int m_ID;
    	int m_TurretType;
	};
	
	int GetItemId(const char* pItemName);

	int GetSpeed(int Level, int Type);

    int GetDmg(int Level);

	void MakeItem(const char* pItemName, int ClientID);

    bool CheckItemName(const char* pItemName);

    void SendCantMakeItemChat(int To, int *Resource);

    void SendMakeItemChat(int To, CItem Item);

    void SendMakeItemFailedChat(int To, int* Resource);
		
	std::vector<CItem> m_vItem;
};

inline int64_t CmaskAll() { return -1LL; }
inline int64_t CmaskOne(int ClientID) { return 1LL<<ClientID; }
inline int64_t CmaskAllExceptOne(int ClientID) { return CmaskAll()^CmaskOne(ClientID); }
inline bool CmaskIsSet(int64_t Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }

#endif
