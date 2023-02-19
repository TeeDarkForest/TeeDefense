#ifndef GAME_SERVER_ENTITIES_DLASER_H
#define GAME_SERVER_ENTITIES_DLASER_H

#include <game/server/entity.h>


class CDangerLaser : public CEntity
{
public:
	CDangerLaser(CGameWorld *pGameWorld, vec2 Pos1, vec2 Pos2, int Victim, int Owner, int Damage = 5);

	virtual void Reset();
	virtual void Tick();
    virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_Pos1;
    vec2 m_Pos2;
    int m_Damage;
    int m_Life;
    int m_Owner;
    int m_Victim;
};

#endif 