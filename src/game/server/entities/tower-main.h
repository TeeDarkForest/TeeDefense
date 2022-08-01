#ifndef GAME_SERVER_ENTITIES_TOWER_MAIN
#define GAME_SERVER_ENTITIES_TOWER_MAIN

#include <game/server/gamecontext.h>
#include <game/server/entity.h>

#define NumSide 32
#define Radius 240
class CTowerMain : public CEntity
{
public:
    CTowerMain(CGameWorld *pGameWorld, vec2 m_StandPos);
    int m_Health;
    int m_Energy;

    void TakeDamage(int Dmg);

    virtual void Tick();
    virtual void Reset();
	virtual void Snap(int SnappingClient);

private:

    int m_FlagID;
    int m_aIDs[9];
    int m_alIDs[NumSide];
};

#endif