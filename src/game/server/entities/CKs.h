#ifndef GAME_SERVER_ENTITIES_CKS
#define GAME_SERVER_ENTITIES_CKS

#include <game/server/entity.h>

const int PhysSize = 14;

enum
{
    CK_WOOD,
    CK_COAL,
    CK_COPPER,
    CK_IRON,
    CK_GOLD,
    CK_DIAMONAD,
};

class CKs : public CEntity
{
public:
	CKs(CGameWorld *pGameWorld, int Type, vec2 Pos, int SubType = 0);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	int m_Type;
	int m_Subtype;
	int m_SpawnTick;
};

#endif