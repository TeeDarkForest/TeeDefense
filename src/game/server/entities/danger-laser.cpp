#include <game/server/gamecontext.h>
#include "danger-laser.h"

CDangerLaser::CDangerLaser(CGameWorld *pGameWorld, vec2 Pos1, vec2 Pos2, int Victim, int Owner, int Damage) : CEntity(pGameWorld, CGameWorld::ENTTYPE_DLASER)
{
	m_Pos1 = Pos1;
	m_Pos2 = Pos2;

	m_Damage = Damage;
	m_Victim = Victim;

	m_Life = 0;
	m_Owner = Owner;
	GameWorld()->InsertEntity(this);
}

void CDangerLaser::Tick()
{
	m_Life++;

	if(GameServer()->GetPlayerChar(m_Victim))
		GameServer()->GetPlayerChar(m_Victim)->TakeDamage(vec2(0.0, 0.2), 1, m_Owner, WEAPON_RIFLE);
	else
		Reset();

	if(m_Life >= 2)
		Reset();
}

void CDangerLaser::TickPaused()
{

}

void CDangerLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CDangerLaser::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if (!pObj)
		return;

	pObj->m_X = (int)m_Pos1.x;
	pObj->m_Y = (int)m_Pos1.y;
	pObj->m_FromX = (int)m_Pos2.x;
	pObj->m_FromY = (int)m_Pos2.y;
	pObj->m_StartTick = Server()->Tick();
}