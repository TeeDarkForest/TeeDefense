/* (c) FlowerFell-Sans. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                 */
#ifndef GAME_SERVER_ENTITIES_CKS
#define GAME_SERVER_ENTITIES_CKS

#include <game/server/entity.h>
#include <game/server/player.h>
const int PhysSize = 14;

enum
{
    CK_WOOD,
    CK_COAL,
    CK_COPPER,
    CK_IRON,
    CK_GOLD,
    CK_DIAMONAD,
	CK_ENEGRY,
	CK_Abyss_LumSub,
	CK_Abyss_Agar,
	CK_Abyss_ScrapMetal,
	CK_Abyss_ScrapMatal_S,
	CK_Abyss_NuclearWaste_S,
	CK_Abyss_Remnant,
};

class CKs : public CEntity
{
public:
	CKs(CGameWorld *pGameWorld, int Type, vec2 Pos, int ID, int SubType = 0);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	virtual void Picking(int Time, CPlayer *Player);
	
	void HandleLockPlayer();
	int m_Health;
	int m_LockPlayer;
private:
	int m_CKID;
	int m_Type;
	int m_Subtype;
	int m_SpawnTick;
};

#endif