#include "account.h"

void CAccount::OnInitWorld()
{
}

static void register_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    int ClientID = Data->m_ClientID;
    lock_wait(Data->m_pGameServer->DB()->SQL_Lock);
    char aBuf[512];
    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s';", Data->m_Username);
    sql::ResultSet *Result;
    if (Data->m_pGameServer->DB()->Connect())
    {
        try
        {
            Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                Data->m_pGameServer->SendChatTarget(ClientID, _("This username is already in use."));
                Data->m_pGameServer->DB()->FreeData(Result);
                lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                return;
            }
            else
            {
                str_format(aBuf, sizeof(aBuf), "INSERT INTO tw_Accounts(Username, Password) VALUES ('%s', '%s');", Data->m_Username, Data->m_Password);
                Data->m_pGameServer->DB()->Execute(aBuf);
                Data->m_pGameServer->SendChatTarget(ClientID, _("Account was created successfully."));
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    Data->m_pGameServer->DB()->FreeData(Result);

    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
}

bool CAccount::Register(int ClientID, const char *Username, const char *Password)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    str_copy(data->m_Username, Username, sizeof data->m_Username);
    str_copy(data->m_Password, Password, sizeof data->m_Password);

    void *register_account_thread = thread_init(register_thread, data);
#if defined(CONF_FAMILY_UNIX)
    pthread_detach((pthread_t)register_account_thread);
#endif
    return true;
}

static void login_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    lock_wait(Data->m_pGameServer->DB()->SQL_Lock);
    int ClientID = Data->m_ClientID;
    CPlayer *P = Data->m_pGameServer->GetPlayer(ClientID);
    char aBuf[512];
    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s';", Data->m_Username);
    sql::ResultSet *Result;
    if (Data->m_pGameServer->DB()->Connect())
    {
        try
        {
            Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s' AND Password = '%s';", Data->m_Username, Data->m_Password);
                Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
                if (Result->next())
                {
                    P->m_AccData.m_UserID = Result->getInt("UserID");
                    P->m_Holding[ITYPE_SWORD] = Result->getInt("Sword");
                    P->m_Holding[ITYPE_AXE] = Result->getInt("Axe");
                    P->m_Holding[ITYPE_PICKAXE] = Result->getInt("Pickaxe");

                    str_copy(P->m_AccData.m_aUsername, Result->getString("Username").c_str(), sizeof(P->m_AccData.m_aUsername));
                    str_copy(P->m_AccData.m_aPassword, Result->getString("Password").c_str(), sizeof(P->m_AccData.m_aPassword));
                    P->SetLanguage(Result->getString("Language").c_str());

                    Data->m_pGameServer->SendChatTarget(ClientID, _("You are now logged in."));
                    Data->m_pGameServer->SendBroadcast_VL(_("Welcome {str:Name}!"), ClientID, "Name", Data->m_pGameServer->Server()->ClientName(ClientID));
                    P->m_InitAcc = true;
                }
                else
                {
                    Data->m_pGameServer->SendChatTarget(ClientID, _("The password you entered is wrong."));
                    Data->m_pGameServer->DB()->FreeData(Result);
                    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                    return;
                }
            }
            else
            {
                Data->m_pGameServer->SendChatTarget(ClientID, _("This Account does not exists."));
                Data->m_pGameServer->SendChatTarget(ClientID, _("Please register first. (/register <user> <pass>)"));
                Data->m_pGameServer->DB()->FreeData(Result);
                lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                return;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    Data->m_pGameServer->DB()->FreeData(Result);

    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
}
bool CAccount::Login(int ClientID, const char *Username, const char *Password)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    str_copy(data->m_Username, Username, sizeof data->m_Username);
    str_copy(data->m_Password, Password, sizeof data->m_Password);

    void *login_account_thread = thread_init(login_thread, data);
#if defined(CONF_FAMILY_UNIX)
    pthread_detach((pthread_t)login_account_thread);
#endif
    return true;
}

static void sync_accdata_thread(void *user)
{
    SFaBao *Data = (SFaBao *)user;
    int ClientID = Data->m_ClientID;
    CPlayer *P = Data->m_pGameServer->GetPlayer(ClientID);
    int UserID = P->m_AccData.m_UserID;
    if (!UserID)
        return;

    lock_wait(Data->m_pGameServer->DB()->SQL_Lock);
    sql::ResultSet *Result;

    if (Data->m_pGameServer->DB()->Connect())
    {
        try
        {
            char aBuf[512];
            str_format(aBuf, sizeof(aBuf), "SELECT Username from tw_Accounts WHERE UserID = %d;", UserID);
            Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                switch (Data->m_Table)
                {
                case CGameContext::TABLE_ACCOUNT:
                {
                    P->m_AccData.m_UserID = Result->getInt("UserID");
                    P->m_Holding[ITYPE_SWORD] = Result->getInt("Sword");
                    P->m_Holding[ITYPE_AXE] = Result->getInt("Axe");
                    P->m_Holding[ITYPE_PICKAXE] = Result->getInt("Pickaxe");

                    str_copy(P->m_AccData.m_aUsername, Result->getString("Username").c_str(), sizeof(P->m_AccData.m_aUsername));
                    str_copy(P->m_AccData.m_aPassword, Result->getString("Password").c_str(), sizeof(P->m_AccData.m_aPassword));
                    P->SetLanguage(Result->getString("Language").c_str());
                }
                break;

                case CGameContext::TABLE_ITEM:
                {
                    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Items WHERE UserID = %d;", UserID);
                    sql::ResultSet *res = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
                    while (res->next())
                    {
                        int ItemID = res->getInt("ItemID");
                        int Num = res->getInt("Num");

                        Data->m_pGameServer->GetPlayer(ClientID)->m_Items[ItemID] = Num;
                    }
                    Data->m_pGameServer->ClearVotes(ClientID);
                }
                break;

                default:
                    break;
                }
            }
            else
            {
                dbg_msg("SyncAccountData", "Error when saving account data.");
                Data->m_pGameServer->DB()->FreeData(Result);
                lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                return;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    Data->m_pGameServer->DB()->FreeData(Result);

    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
}

void CAccount::SyncAccountData(int ClientID, int Table)
{
    SFaBao *data = new SFaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    data->m_Table = Table;
    void *sync_thread = thread_init(sync_accdata_thread, data);
#if defined(CONF_FAMILY_UNIX)
    pthread_detach((pthread_t)sync_thread);
#endif
}

static void save_accdata_thread(void *user)
{
    SFaBao *Data = (SFaBao *)user;
    int ClientID = Data->m_ClientID;
    CPlayer *P = Data->m_pGameServer->GetPlayer(ClientID);
    int UserID = P->m_AccData.m_UserID;
    if (!UserID)
        return;

    lock_wait(Data->m_pGameServer->DB()->SQL_Lock);
    sql::ResultSet *Result;
    if (Data->m_pGameServer->DB()->Connect())
    {
        try
        {
            char aBuf[512];
            str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE UserID = %d;", UserID);
            Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                switch (Data->m_Table)
                {
                case CGameContext::TABLE_ACCOUNT:
                {
                    str_format(aBuf, sizeof(aBuf), "UPDATE tw_Accounts SET "
                                                   "Username='%s',Password='%s',Language='%s',Sword=%d,Axe=%d,Pickaxe=%d "
                                                   "WHERE UserID=%d;",
                               P->m_AccData.m_aUsername, P->m_AccData.m_aPassword, P->GetLanguage(), P->m_Holding[ITYPE_SWORD], P->m_Holding[ITYPE_AXE], P->m_Holding[ITYPE_PICKAXE], UserID);
                    Data->m_pGameServer->DB()->Execute(aBuf);
                }
                break;

                case CGameContext::TABLE_ITEM:
                {
                    for (int i = 0; i < NUM_ITEM; i++)
                    {
                        str_format(aBuf, sizeof(aBuf), "SELECT * FROM tw_Items WHERE UserID=%d AND ItemID=%d;", UserID, i);
                        sql::ResultSet *res = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);

                        if (res->next())
                        {
                            if (P->m_Items[i])
                                str_format(aBuf, sizeof(aBuf), "UPDATE tw_Items SET Num=%d WHERE UserID=%d AND ItemID=%d;", P->m_Items[i], UserID, i); // if yes, update it.
                            else
                                str_format(aBuf, sizeof(aBuf), "DELETE FROM tw_Items WHERE UserID=%d AND ItemID=%d;", UserID, i); // So delete it
                            Data->m_pGameServer->DB()->Execute(aBuf);                                                             // Execute
                        }
                        else if (P->m_Items[i])
                        {
                            str_format(aBuf, sizeof(aBuf), "INSERT INTO tw_Items(UserID, ItemID, Num) VALUES (%d, %d, %d)", UserID, i, P->m_Items[i]); // if not, insert it.
                            Data->m_pGameServer->DB()->Execute(aBuf);
                        }
                    }
                }
                break;

                default:
                    break;
                }
            }
            else
            {
                dbg_msg("SyncAccountData", "Error when saving account data.");
                Data->m_pGameServer->DB()->FreeData(Result);
                lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                return;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Saving Account Data (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    Data->m_pGameServer->DB()->FreeData(Result);

    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
}

void CAccount::SaveAccountData(int ClientID, int Table)
{
    SFaBao *data = new SFaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    data->m_Table = Table;
    void *save_thread = thread_init(save_accdata_thread, data);
#if defined(CONF_FAMILY_UNIX)
    pthread_detach((pthread_t)save_thread);
#endif
}