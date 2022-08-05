/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "follow-gun.h"

CFGun::CFGun(CGameWorld *pGameWorld, vec2 Pos, int Owner, vec2 Direction, int Weapon, int Damage)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_Direction = Direction;
	m_LifeSpan = GameServer()->Tuning()->m_GunLifetime*3;
	m_Owner = Owner;
	m_Damage = Damage;
	m_SoundImpact = SOUND_GRENADE_FIRE;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();

	GameWorld()->InsertEntity(this);
}

void CFGun::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CFGun::Tick()
{
	int Collide;
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->GetPlayerChar(m_Owner);

    if(!TargetChr)
    {
		m_Pos += m_Dir * m_Speed;

		float MinDistance = 2400.0f, MinDistancePlayer = -1;
		for(CCharacter *p = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CCharacter *)p->TypeNext())
		{
			//if(p->GetPlayer()->GetZomb())continue;

			float Len = distance(p->m_Pos, m_Pos);
			if(Len < p->m_ProximityRadius + 360)
			{
				if(MinDistance > Len)
				{
					MinDistance = Len;
					MinDistancePlayer = p->GetPlayer()->GetCID();
				}
			}
		}
		if(MinDistancePlayer > -1)
		{
			m_TrackedPlayer = MinDistancePlayer;
		}
	}
    else
    {
        float Dist = distance(m_Pos, TargetChr->m_Pos);
		if(Dist < 24.0f)
		{	
			Collide = GameServer()->Collision()->CheckPoint(m_Pos.x, m_Pos.y);
            dbg_msg("qqqqqqqqqqakldlkwahe114514","");
		}
		else
		{
			
			m_Dir = normalize(TargetChr->m_Pos - m_Pos);
			m_Pos += m_Dir*m_Speed;
		}
    }
    
    m_LifeSpan--;

	if(GameServer()->Collision()->CheckPoint(m_Pos.x, m_Pos.y))
	{
		if(TargetChr && TargetChr->GetPlayer()->GetZomb())
			TargetChr->TakeDamage(vec2(0,0), m_Damage, m_Owner, m_Weapon);

		GameServer()->m_World.DestroyEntity(this);
	}
}

void CFGun::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = m_Direction.x;
	pProj->m_VelY = m_Direction.y;
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Weapon;
}

void CFGun::Snap(int SnappingClient)
{
	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
}
