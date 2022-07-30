#include "tower-main.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <new>

#define PickupPhysSizeS 14
#define M_PI 3.14159265358979323846
CTowerMain::CTowerMain(CGameWorld *pGameWorld, vec2 StandPos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_TOWERMAIN)
{
    m_Pos = StandPos;
    
    for (unsigned i = 0; i < sizeof(m_aIDs) / sizeof(int); i ++)
        m_aIDs[i] = Server()->SnapNewID();

    m_ProximityRadius = PickupPhysSizeS;

    GameWorld()->InsertEntity(this);

    m_Health = 1;
}

void CTowerMain::Tick()
{
    for(CCharacter *pChr = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
    {
        int ClientID = pChr->GetCID();
        if(Server()->Tick()%Server()->TickSpeed() == 0)
            pChr->IncreaseHealth(1);
    }
}
void CTowerMain::Reset() 
{
	for (unsigned i = 0; i < sizeof(m_aIDs) / sizeof(int); i ++)
	{
		if(m_aIDs[i] >= 0){
			Server()->SnapFreeID(m_aIDs[i]);
			m_aIDs[i] = -1;
		}
	}
    GameServer()->m_World.DestroyEntity(this);
}

void CTowerMain::Snap(int SnappingCLient)
{
    int aSize = sizeof(m_aIDs) / sizeof(int);

    for (int i = 0; i < aSize; i ++) 
	{
        CNetObj_Projectile *pEff = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aIDs[i], sizeof(CNetObj_Projectile)));
		if (!pEff)
            continue;

		pEff->m_X = (int)(cos(M_PI/9*i*4)*(16.0+5/110)+m_Pos.x);
		pEff->m_Y = (int)(sin(M_PI/9*i*4)*(16.0+5/110)+m_Pos.y);

		pEff->m_StartTick = Server()->Tick()-2;
		pEff->m_Type = WEAPON_SHOTGUN;
    }

    CNetObj_Flag *pFlag = static_cast<CNetObj_Flag *>(Server()->SnapNewItem(NETOBJTYPE_FLAG, m_FlagID, sizeof(CNetObj_Flag)));
    if(!pFlag)
        return;

    pFlag->m_Team = TEAM_RED;    
    pFlag->m_X = m_Pos.x;
    pFlag->m_Y = m_Pos.y;
}

void CTowerMain::TakeDamage(int Dmg)
{
    if(m_Health <= 0)
        return;
    m_Health -= Dmg;
}