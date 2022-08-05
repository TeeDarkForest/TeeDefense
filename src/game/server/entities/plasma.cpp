// Strongly modified version of ddnet Plasma. Source: Shereef Marzouk
#include <engine/server.h>
#include <engine/config.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "plasma.h"

CPlasma::CPlasma(CGameWorld *pGameWorld, vec2 Pos, int Owner, int TrackedPlayer,vec2 Direction, bool Freeze, bool Explosive, int Weapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PLASMA)
{
	m_Owner = Owner;
	m_Pos = Pos;
	m_Freeze = Freeze;
	m_TrackedPlayer = TrackedPlayer;
	m_Dir = Direction;
	m_Explosive = Explosive;
	m_StartTick = Server()->Tick();
	m_LifeSpan = Server()->TickSpeed()*60;
	m_InitialAmount = 1.0f;
	m_Weapon = Weapon;
	GameWorld()->InsertEntity(this);
}

int CPlasma::GetOwner() const
{
	return m_Owner;
}

void CPlasma::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CPlasma::Tick()
{
	
	//reduce lifespan
	if (m_LifeSpan < 0)
	{
		Reset();
		return;
	}
	m_LifeSpan--;
	
	// tracking, position and collision calculation
	CCharacter *pTarget = GameServer()->GetPlayerChar(m_TrackedPlayer);
	if(pTarget)
	{
		float Dist = distance(m_Pos, pTarget->m_Pos);
		if(Dist < 24.0f)
		{
			Explode();
		}
		else
		{
			
			m_Dir = normalize(pTarget->m_Pos - m_Pos);
			m_Speed = clamp(Dist, 0.0f, 16.0f) * (1.0f - m_InitialAmount);
			m_Pos += m_Dir*m_Speed;
			
			m_InitialAmount *= 0.98f;
			
			//collision detection
			if(GameServer()->Collision()->CheckPoint(m_Pos.x, m_Pos.y)) // this only works as long as the projectile is not moving too fast
			{
				Explode();
			}
		}
	} 
	else //Target died before impact -> explode
	{
		Explode();
	}
	
}

void CPlasma::Explode() 
{
	//GameServer()->CreateSound(CurPos, m_SoundImpact);
	if (m_Explosive) 
	{
		GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_GRENADE, false);
	}
	Reset();
}

void CPlasma::Snap(int SnappingClient)
{	
	CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(
		NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	
	if(!pObj)
		return;
	
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_VelX = (int)(m_Dir.x);
	pObj->m_VelY = (int)(m_Dir.y);
	pObj->m_StartTick = Server()->Tick();
	pObj->m_Type = m_Weapon;
}
