#include "turret.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <new>
#include <engine/shared/config.h>
#include "lightning.h"
#define RAD 0.017453292519943295769236907684886f
CTurret::CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Type, int Radius, bool Follow, bool Lightning, bool Freeze)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_TURRET)
{
    m_Pos = Pos;
    m_Owner = Owner;
    m_Type = Type;
    m_Radius = Radius;
    m_Follow = Follow;
    m_Lightning = Lightning;
    m_Freeze = Freeze;
    m_FireDelay = 0;

    for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
        m_IDs[i] = Server()->SnapNewID();

    for (unsigned i = 0; i < sizeof(m_aIDs) / sizeof(int); i ++)
        m_aIDs[i] = Server()->SnapNewID();

    m_ID = Server()->SnapNewID();

    m_ProximityRadius = PickupPhysSizeS;

    GameWorld()->InsertEntity(this);
}

bool CTurret::GetFollow()
{
    return m_Follow;
}
bool CTurret::GetFreeze()
{
    return m_Freeze;
}
bool CTurret::GetLightning()
{
    return m_Lightning;
}
int CTurret::GetOwner()
{
    return m_Owner;
}
int CTurret::GetRadius()
{
    return m_Radius;
}
int CTurret::GetType()
{
    return m_Type;
}

int CTurret::GetSnapType()
{
    switch (GetType())
    {
    case TURRET_GUN:
        return WEAPON_GUN;
        break;
    
    case TURRET_SHOTGUN:
        return WEAPON_SHOTGUN;
        break;
    
    case TURRET_FOLLOW_GRENADE:
        return WEAPON_GRENADE;
        break;
    
    case TURRET_LASER:
        return WEAPON_RIFLE;
        break;
    
    default:
        break;
    }
}

void CTurret::Tick()
{
    for(CCharacter *pChr = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
    {
        if(!pChr->IsAlive()/* || !pChr->GetPlayer()->GetZomb()*/)
            return;

        vec2 TargetPos;
        float Len = distance(pChr->m_Pos, m_Pos);
        if(Len < pChr->m_ProximityRadius+GameServer()->Tuning()->m_LaserReach)
        {
            TargetPos = pChr->m_Pos;
            vec2 Direction = normalize(TargetPos - m_Pos);
            switch (GetType())
            {
            case TURRET_GUN:
                if(m_FireDelay <= 0)
                {
                    new CProjectile(GameWorld(), WEAPON_GUN, GetOwner(), m_Pos, Direction, 5000, 1, false, 10, SOUND_GRENADE_EXPLODE, WEAPON_GUN);
                    m_FireDelay = 50;
                }
                break;

            case TURRET_SHOTGUN:
                if(m_FireDelay <= 0)
                {
                    new CProjectile(GameWorld(), WEAPON_SHOTGUN, GetOwner(), m_Pos, Direction, 5000, 1, false, 10, SOUND_GRENADE_EXPLODE, WEAPON_SHOTGUN);
                    m_FireDelay = 75;
                }
                break;
            case TURRET_LASER:
                if(m_FireDelay <= 0)
                {
                    new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, 1);
                    m_FireDelay = 50;
                }
                break;
            
            default:
                break;

            }
        }
    }

    if(m_FireDelay >= 0)
        m_FireDelay--;
}

void CTurret::Reset()
{
    for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
	{
		if(m_IDs[i] >= 0){
			Server()->SnapFreeID(m_IDs[i]);
			m_IDs[i] = -1;
		}
	}
    Server()->SnapFreeID(m_ID);

    GameServer()->m_World.DestroyEntity(this);
}

void CTurret::Snap(int SnappingClient)
{
    if ( m_Degres + 1 < 360 )
		m_Degres += 1;
	else
		m_Degres = 0;

    int aIDSize = sizeof(m_aIDs) / sizeof(int);

    float AngleStep = 2.0f * M_PI / NumSide;

    for(int i=0; i<NumSide; i++)
	{
	    vec2 PartPosStart = m_Pos + vec2(GetRadius() * cos(AngleStep*i), GetRadius() * sin(AngleStep*i));
	    vec2 PartPosEnd = m_Pos + vec2(GetRadius() * cos(AngleStep*(i+1)), GetRadius() * sin(AngleStep*(i+1)));
	    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
	    if(!pObj)
	    	return;
	    pObj->m_X = (int)PartPosStart.x;
	    pObj->m_Y = (int)PartPosStart.y;
	    pObj->m_FromX = (int)PartPosEnd.x;
	    pObj->m_FromY = (int)PartPosEnd.y;
	    pObj->m_StartTick = Server()->Tick();
	}

    switch(GetType())
    {
    case TURRET_GUN:
    case TURRET_FOLLOW_GRENADE:
    case TURRET_LASER:
    {
        CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	    if(!pObj)
	    	return;
        pObj->m_X = m_Pos.x;
        pObj->m_Y = m_Pos.y;
        pObj->m_Type = GetSnapType();
        pObj->m_StartTick = Server()->Tick();
    }
    break;
    case TURRET_SHOTGUN:
    case TURRET_SHOTGUN_2077:
    {
        for(int i = 0; i < aIDSize; i++)
        {
            if(GetType() == TURRET_SHOTGUN_2077)
            {
                float a = frandom() * 360 * RAD;
                new CLightning(GameWorld(), m_Pos, vec2(cosf(a), sinf(a)), 1, 50, GetOwner(), 5, 9);
            }

            CNetObj_Projectile *pEff = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aIDs[i], sizeof(CNetObj_Projectile)));
	        if(!pEff)
	        	return;
            vec2 a = m_Pos + (GetDir((m_Degres-i*22.5)*M_PIl/180) * 32);
            pEff->m_X = a.x;
            pEff->m_Y = a.y;
            pEff->m_Type = WEAPON_SHOTGUN;
            pEff->m_StartTick = Server()->Tick();
        }
        break;
    }
    default:
        break;
    }
}