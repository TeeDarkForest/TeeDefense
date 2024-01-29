#include "item.h"
#include <game/server/gamecontext.h>
#include <engine/external/json-parser/json.h>

CItem_F::CItem_F(CGameContext *pGameServer)
{
    m_pGameServer = pGameServer;
}

void CItem_F::LoadIndex()
{
    const char *pIndex = "./server_items/index.json";
    IOHANDLE File = GameServer()->Storage()->OpenFile(pIndex, IOFLAG_READ, CStorage::TYPE_ALL);
    dbg_assert(!(!File), "Can't open 'server_items/index.json'");
    int FileSize = (int)io_length(File);
    char *pFileData = new char[FileSize + 1];
    io_read(File, pFileData, FileSize);
    pFileData[FileSize] = 0;
    io_close(File);

    // parse json data
    json_settings JsonSettings;
    mem_zero(&JsonSettings, sizeof(JsonSettings));
    char aError[256];
    json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
    dbg_assert(!(!pJsonData), "Can't open 'server_items/index.json'");

    const json_value &rStart = (*pJsonData)["item indices"];
    if (rStart.type == json_array)
    {
        for (unsigned i = 0; i < rStart.u.array.length; ++i)
        {
            if (rStart[i])
                LoadItem((const char *)rStart[i]);
        }
        for (unsigned i = 0; i < rStart.u.array.length; ++i)
        {
            if (rStart[i])
                LoadFormula((const char *)rStart[i]);
        }
    }

    // clean up
    json_value_free(pJsonData);
    delete[] pFileData;
}

void CItem_F::LoadItem(const char *FileName)
{
    IOHANDLE File = GameServer()->Storage()->OpenFile(FileName, IOFLAG_READ, CStorage::TYPE_ALL);
    if (!File)
    {
        dbg_msg("Item Loader", "Can't open '%s'", FileName);
        return;
    }
    int FileSize = (int)io_length(File);
    char *pFileData = new char[FileSize + 1];
    io_read(File, pFileData, FileSize);
    pFileData[FileSize] = 0;
    io_close(File);

    // parse json data
    json_settings JsonSettings;
    mem_zero(&JsonSettings, sizeof(JsonSettings));
    char aError[256];
    json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
    if (!pJsonData)
    {
        dbg_msg("Item Loader", "Can't open '%s'", FileName);
        return;
    }

    const json_value &rStart = (*pJsonData)["item"];
    if (rStart)
    {
        int Type = rStart["type"].u.integer;
        if (rStart["multiple"])
        {
            for (unsigned i = 0; i < rStart["multiple"].u.object.length; i++)
            {
                const json_value &rMultiple = rStart["multiple"][i];
                int ID = rMultiple["id"].u.integer;
                m_Items[ID].m_Type = rMultiple["type"].u.integer;
                m_Items[ID].m_ID = ID;
                str_copy(m_Items[ID].m_ItemName, rMultiple["name"], sizeof(m_Items[ID].m_ItemName));
                m_Items[ID].m_Proba = rMultiple["proba"].u.integer;
                m_Items[ID].m_Damage = rMultiple["damage"].u.integer;
                m_Items[ID].m_Capacity = rMultiple["capacity"].u.integer;
                m_Items[ID].m_Unplaceable = rMultiple["unplaceable"].u.integer;

                dbg_msg("Test", "ID: %d, Type: %d, Damage: %d, Proba: %d, Capacity: %d, ItemName: %s", ID, m_Items[ID].m_Type,
                        m_Items[ID].m_Damage, m_Items[ID].m_Proba, m_Items[ID].m_Capacity, m_Items[ID].m_ItemName);
            }
        }
        else
        {
            int ID = rStart["id"].u.integer;
            m_Items[ID].m_Type = rStart["type"].u.integer;
            m_Items[ID].m_ID = ID;
            str_copy(m_Items[ID].m_ItemName, rStart["name"], sizeof(m_Items[ID].m_ItemName));
            m_Items[ID].m_Proba = rStart["proba"].u.integer;
            m_Items[ID].m_Damage = rStart["damage"].u.integer;
            dbg_msg("Test", "ID: %d, Type: %d, Damage: %d, Proba: %d, Capacity: %d, ItemName: %s", ID, m_Items[ID].m_Type,
                    m_Items[ID].m_Damage, m_Items[ID].m_Proba, m_Items[ID].m_Capacity, m_Items[ID].m_ItemName);
        }
    }
    const json_value &rCK = (*pJsonData)["ck"];
}

void CItem_F::LoadFormula(const char *FileName)
{
    IOHANDLE File = GameServer()->Storage()->OpenFile(FileName, IOFLAG_READ, CStorage::TYPE_ALL);
    if (!File)
    {
        dbg_msg("Item Loader", "Can't open '%s'", FileName);
        return;
    }
    int FileSize = (int)io_length(File);
    char *pFileData = new char[FileSize + 1];
    io_read(File, pFileData, FileSize);
    pFileData[FileSize] = 0;
    io_close(File);

    // parse json data
    json_settings JsonSettings;
    mem_zero(&JsonSettings, sizeof(JsonSettings));
    char aError[256];
    json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, aError);
    if (!pJsonData)
    {
        dbg_msg("Item Loader", "Can't open '%s'", FileName);
        return;
    }

    const json_value &rStart = (*pJsonData)["item"];
    if (rStart)
    {
        if (rStart["multiple"])
        {
            for (int i = 0; i < rStart["multiple"].u.object.length; i++)
            {
                int ID = rStart["multiple"][i]["id"];
                const json_value &rFormula = rStart["multiple"][i]["formula"];
                for (int j = 0; j < rFormula.u.object.length; j++)
                    m_Items[ID].m_Formula[FindItem(rFormula.u.object.values[j].name)] = rFormula.u.object.values[j].value->u.integer;
            }
        }
        else if (rStart["formula"])
        {
            int ID = rStart["id"];
            const json_value &rFormula = rStart["formula"];
            for (int j = 0; j < rFormula.u.object.length; j++)
                m_Items[ID].m_Formula[FindItem(rFormula.u.object.values[j].name)] = rFormula.u.object.values[j].value->u.integer;
        }
    }
}

int CItem_F::FindItem(const char *ItemName)
{
    for (int i = 0; i < NUM_ITEM; i++)
    {
        // dbg_msg("HSAD", "%s %s", m_Items[i].m_ItemName, ItemName);
        if (str_comp(m_Items[i].m_ItemName, ItemName) == 0)
            return m_Items[i].m_ID;
    }
    return 0;
}