// Strongly modified version of ddnet Plasma. Source: Shereef Marzouk
#ifndef GAME_SERVER_ENTITIES_FOLLOW_GUN_H
#define GAME_SERVER_ENTITIES_FOLLOW_GUN_H

#include <game/server/entity.h>

class CFGun: public CEntity
{
	
public:
	CFGun(CGameWorld *pGameWorld, vec2 Pos, int Owner, vec2 Direction, int Weapon, int Damage);
	
	int GetOwner() const;
	
	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
    void FillInfo(CNetObj_Projectile *pProj);
	
public:
	int m_Owner;

private:
	int m_StartTick;
	int m_LifeSpan;
	vec2 m_Dir;
	float m_Speed;
	int m_TrackedPlayer;
    int m_Damage;
	int m_SoundImpact;
	int m_Weapon;
    vec2 m_Direction;
};

#endif
