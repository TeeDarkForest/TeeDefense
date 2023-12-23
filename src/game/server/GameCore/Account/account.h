#ifndef GAME_SERVER_GAMECORE_ACCOUNT_H
#define GAME_SERVER_GAMECORE_ACCOUNT_H

#include <game/server/GameCore/TWorldComponent.h>
#include <game/server/gamecontext.h>

#include <game/server/GameCore/Database/DB.h>

class CAccount : public TWorldComponent
{
public:
    ~CAccount(){};

    virtual void OnInitWorld();
    bool Register(int ClientID, const char *Username, const char *Password);
    bool Login(int ClientID, const char *Username, const char *Password);
    void SyncAccountData(int ClientID, int Table);
    void SaveAccountData(int ClientID, int Table);
};

struct FaBao
{
    CGameContext *m_pGameServer;
    int m_ClientID;
    char m_Username[64];
    char m_Password[64];
};

struct SFaBao : FaBao
{
    int m_Table;
};

#endif