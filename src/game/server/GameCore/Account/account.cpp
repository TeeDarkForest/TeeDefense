#include "account.h"

void CAccount::OnInitWorld()
{
}

bool CAccount::Register(int ClientID, const char *Username, const char *Password)
{
    lock_wait(DB()->SQL_Lock);
    char aBuf[512];
    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s';", Username);
    sql::ResultSet *Result;
    if (DB()->Connect())
    {
        try
        {
            Result = DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                GameServer()->SendChatTarget(ClientID, _("This username is already in use."));
                DB()->FreeData(Result);
                lock_unlock(DB()->SQL_Lock);
                return false;
            }
            else
            {
                str_format(aBuf, sizeof(aBuf), "INSERT INTO tw_Accounts(Username, Password) VALUES ('%s', '%s');", Username, Password);
                DB()->Execute(aBuf);
                GameServer()->SendChatTarget(ClientID, _("Account was created successfully."));
                Login(ClientID, Username, Password);
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    DB()->FreeData(Result);

    lock_unlock(DB()->SQL_Lock);
    return true;
}

bool CAccount::Login(int ClientID, const char *Username, const char *Password)
{
    lock_wait(DB()->SQL_Lock);
    CPlayer *P = GameServer()->GetPlayer(ClientID);
    char aBuf[512];
    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s';", Username);
    sql::ResultSet *Result;
    if (DB()->Connect())
    {
        try
        {
            Result = DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s' AND Password = '%s';", Username, Password);
                Result = DB()->ExecuteQuery(aBuf);
                if (Result->next())
                {
                    P->m_AccData.m_UserID = Result->getInt("UserID");
                    str_copy(P->m_AccData.m_aUsername, Result->getString("Username").c_str(), sizeof(P->m_AccData.m_aUsername));
                    str_copy(P->m_AccData.m_aPassword, Result->getString("Password").c_str(), sizeof(P->m_AccData.m_aPassword));

                    GameServer()->SendChatTarget(ClientID, _("You are now logged in."));
                    GameServer()->SendBroadcast_VL(_("Welcome {str:Name}!"), ClientID, "Name", GameServer()->Server()->ClientName(ClientID));
                }
                else
                {
                    GameServer()->SendChatTarget(ClientID, _("The password you entered is wrong."));
                    DB()->FreeData(Result);
                    lock_unlock(DB()->SQL_Lock);
                    return false;
                }
            }
            else
            {
                GameServer()->SendChatTarget(ClientID, _("This Account does not exists."));
                GameServer()->SendChatTarget(ClientID, _("Please register first. (/register <user> <pass>)"));
                DB()->FreeData(Result);
                lock_unlock(DB()->SQL_Lock);
                return false;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    DB()->FreeData(Result);

    lock_unlock(DB()->SQL_Lock);
    return true;
}

void CAccount::SyncAccountData(int ClientID, int Table)
{
    lock_wait(DB()->SQL_Lock);
    sql::ResultSet *Result;
    if (DB()->Connect())
    {
        try
        {
            CPlayer *P = GameServer()->GetPlayer(ClientID);
            int UserID = P->m_AccData.m_UserID;
            char aBuf[512];
            str_format(aBuf, sizeof(aBuf), "SELECT Username from tw_Accounts WHERE UserID = %d;", UserID);
            Result = DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                switch (Table)
                {
                case CGameContext::TABLE_ACCOUNT:
                {
                    P->m_AccData.m_UserID = Result->getInt("UserID");
                    str_copy(P->m_AccData.m_aUsername, Result->getString("Username").c_str(), sizeof(P->m_AccData.m_aUsername));
                    str_copy(P->m_AccData.m_aPassword, Result->getString("Password").c_str(), sizeof(P->m_AccData.m_aPassword));
                }
                break;

                case CGameContext::TABLE_ITEM:
                {
                    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Items WHERE UserID = %d;", UserID);
                    sql::ResultSet *res = DB()->ExecuteQuery(aBuf);
                    while (res->next())
                    {
                        int ItemID = res->getInt("ItemID");
                        int Num = res->getInt("Num");

                        GameServer()->GetPlayer(ClientID)->m_Items[ItemID] = Num;
                    }
                    GameServer()->ClearVotes(ClientID);
                }
                break;

                default:
                    break;
                }
            }
            else
            {
                dbg_msg("SyncAccountData", "Error when saving account data.");
                DB()->FreeData(Result);
                lock_unlock(DB()->SQL_Lock);
                return;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    DB()->FreeData(Result);

    lock_unlock(DB()->SQL_Lock);
}

void CAccount::SaveAccountData(int ClientID, int Table)
{
    lock_wait(DB()->SQL_Lock);
    sql::ResultSet *Result;
    if (DB()->Connect())
    {
        try
        {
            CPlayer *P = GameServer()->GetPlayer(ClientID);
            int UserID = P->m_AccData.m_UserID;
            char aBuf[512];
            str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE UserID = %d;", UserID);
            Result = DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                switch (Table)
                {
                case CGameContext::TABLE_ACCOUNT:
                {
                    str_format(aBuf, sizeof(aBuf), "UPDATE tw_Accounts SET Username='%s',Password='%s' WHERE UserID=%d;", P->m_AccData.m_aUsername, P->m_AccData.m_aPassword, UserID);
                    DB()->Execute(aBuf);
                }
                break;

                case CGameContext::TABLE_ITEM:
                {
                    for (int i = 0; i < NUM_ITEM; i++)
                    {
                        str_format(aBuf, sizeof(aBuf), "SELECT * FROM tw_Items WHERE UserID=%d AND ItemID=%d;", UserID, i);
                        sql::ResultSet *res = DB()->ExecuteQuery(aBuf);

                        if (res->next())
                        {
                            if (P->m_Items[i])
                                str_format(aBuf, sizeof(aBuf), "UPDATE tw_Items SET Num=%d WHERE UserID=%d AND ItemID=%d;", P->m_Items[i], UserID, i); // if yes, update it.
                            else
                                str_format(aBuf, sizeof(aBuf), "DELETE FROM tw_Items WHERE UserID=%d AND ItemID=%d;", UserID, i); // So delete it
                            DB()->Execute(aBuf);                                                                                  // Execute
                        }
                        else if (P->m_Items[i])
                        {
                            str_format(aBuf, sizeof(aBuf), "INSERT INTO tw_Items(UserID, ItemID, Num) VALUES (%d, %d, %d)", UserID, i, P->m_Items[i]); // if not, insert it.
                            DB()->Execute(aBuf);
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
                DB()->FreeData(Result);
                lock_unlock(DB()->SQL_Lock);
                return;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Saving Account Data (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    DB()->FreeData(Result);

    lock_unlock(DB()->SQL_Lock);
}