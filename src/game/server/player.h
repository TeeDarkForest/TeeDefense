/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.h"
#include "gamecontext.h"
#include "teedefense/Item/item.h"

// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

public:
	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Init(int CID);

	void TryRespawn();
	void Respawn();
	void SetTeam(int Team, bool DoChatMsg = true);
	int GetTeam();
	int GetCID() const { return m_ClientID; };

	void Tick();
	void PostTick();
	void Snap(int SnappingClient);

	void FakeSnap(int SnappingClient);

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void KillCharacter(int Weapon = WEAPON_GAME);
	CCharacter *GetCharacter();

	const char *GetLanguage();
	void SetLanguage(const char *pLanguage);

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;

	bool m_IsReady;

	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;

	// TODO: clean this up
	struct CTeeInfo
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int m_RespawnTick;
	int m_DieTick;
	int m_Score;
	int m_ScoreStartTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	int m_TeamChangeTick;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;

	int m_Authed;

	bool IsBot() { return false; }

private:
	CCharacter *m_pCharacter;
	CGameContext *m_pGameServer;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	//
	bool m_Spawning;
	int m_ClientID;
	int m_Team;

	char m_aLanguage[16];

public:
	int m_SnappingNum;

private:
	void HandleTuningParams(); // This function will send the new parameters if needed

public:
	CTuningParams *GetNextTuningParams() { return &m_NextTuningParams; };
	CTuningParams m_PrevTuningParams;
	CTuningParams m_NextTuningParams;

	//
public:
	struct // knapsack
	{
		// Developers (Special things. XD)
		int m_Shengyan; // This is special, he is SLUG lmao! hahahahahahah xaxaxaxaxaxaxaxaxa
		int m_Ninecloud;
		int m_EDreemurr;
		int m_XyCloud;
		int m_HGDio;   // This is special one too, he is OOOOOOOOOOOOOOOO.
		int m_Jiuyuan; // half developer (?)
		int m_FFS;	   // it me, FlowerFell-Sans. lmao(why my name is soo long...)
	} m_Knapsack;

	int m_Holding[NUM_ITYPE];

	struct SAccData
	{
		int m_UserID;
		char m_aUsername[64];
		char m_aPassword[64];
	};

	SAccData m_AccData;
	SPlayerItemData m_Items[NUM_ITEM];

	// Mine
	int m_MiningType;
	int m_MiningTick;

	bool m_LockedCK;

	// Check
	bool PressTab();

	bool LoggedIn()
	{
		return (m_AccData.m_UserID > 0);
	}

	void ResetKnapsack();

	bool m_InitAcc;
};

#endif
