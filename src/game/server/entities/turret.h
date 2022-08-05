#ifndef GAME_SERVER_ENTITIES_TURRET
#define GAME_SERVER_ENTITIES_TURRET

#include <game/server/entity.h>
#include "projectile.h"
#include "laser.h"
#include <game/server/gamecontext.h>

#define NumSide 6
#define PickupPhysSizeS 14

enum
{
    TURRET_GUN, // Normal pistol
    TURRET_SHOTGUN, // Normal shotgun
    TURRET_FOLLOW_GRENADE, // Follow grenade
    TURRET_LASER, // Fast laser gun
    TURRET_NO_GRAVITY_GUN, // Fire fireball
    TURRET_FREEZE_GUN, // Freeze zombie
    TURRET_GUN_2077, // Follow, fast pistol
    TURRET_SHOTGUN_2077, // Follow, spread, shotgun
    TURRET_LASER_2077, // Lightning!!
};

class CTurret : public CEntity
{
public:
    CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Type, int Radius = 64, int Lifes = 120);
    virtual ~CTurret();

    virtual void Tick();
    virtual void Reset();
	virtual void Snap(int SnappingClient);

    int GetOwner();
    int GetRadius();
    int GetType();
    int GetSnapType();
private:
    int m_Type;
    int m_Radius;
    int m_Owner;
    int m_IDs[NumSide];
    int m_aIDs[NumSide];

    int m_FireDelay;
    int m_Degres;

    int m_Lifes;

    vec2 TargetPos;
};
#endif