/* (c) FlowerFell-Sans. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                 */

#define SCALE 30
#include <game/generated/protocol.h>

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include "giga-Qian.h"
#include "growingexplosion.h"
#include "plasma.h"
#include "laser.h"

#include <engine/shared/config.h>

void CQian::Rotate(vec2* vertex, float x_orig, float y_orig, float angle)
{
	// FUCK THIS MATH
	float s = sin(angle);
	float c = cos(angle);

	vertex->x -= x_orig;
	vertex->y -= y_orig;

	float xnew = vertex->x * c - vertex->y * s;
	float ynew = vertex->x * s + vertex->y * c;

	vertex->x = xnew + x_orig;
	vertex->y = ynew + y_orig;
}

CQian::CQian(CGameWorld *pGameWorld, int m_CoreID, int Owner, vec2 Pos) :
    CEntity(pGameWorld, CGameWorld::ENTTYPE_QIAN)
{
    dbg_msg("CREATE!","GOOD");
    m_Pos = Pos;
    m_Owner = Owner;

    new CLaserEyes(pGameWorld, CLaserEyes::TYPE_LEFT, this);
    new CLaserEyes(pGameWorld, CLaserEyes::TYPE_RIGHT, this);
    new CLaserEyes(pGameWorld, CLaserEyes::TYPE_UP, this);
    new CLaserEyes(pGameWorld, CLaserEyes::TYPE_DOWN, this);
    new CLaserEyes(pGameWorld, CLaserEyes::TYPE_FREEGO, this);

    for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
        m_IDs[i] = Server()->SnapNewID();
    for (unsigned i = 0; i < sizeof(m_aIDs) / sizeof(int); i ++)
        m_aIDs[i] = Server()->SnapNewID();

    m_UsingSkill = SKILL_NONE;
    GameServer()->m_World.InsertEntity(this);
}

void CQian::Tick()
{
    if(!GameServer()->GetPlayerChar(m_Owner) || GameServer()->m_EventDuration <= 1)
    {
        Reset();
    }
    int NumHumbie = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(!GameServer()->m_apPlayers[i])
            continue;

        if(!GameServer()->GetPlayerChar(i))
            continue;
        
        if(GameServer()->m_apPlayers[i]->GetZomb(15))
            NumHumbie++;
    }

    if(NumHumbie >= GameServer()->NumHumanAlive())
        Reset();

    
	if (GameLayerClipped(m_Pos))
	{
		m_MarkedForDestroy = true;
	}

    m_Vel.y+=GameServer()->Tuning()->m_Gravity;
	GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(32.0f*6.0f, 32.0f*6.0f), 0);
    
    if(GameServer()->GetPlayerChar(m_Owner))
        m_Vel = GameServer()->GetPlayerChar(m_Owner)->m_Core.m_Vel*4.0f;
 //   m_Vel.x = GameServer()->GetPlayerChar(m_Owner)->m_Core.m_Direction*32.0f;
    
    if(GameServer()->GetPlayerChar(m_Owner))
        GameServer()->GetPlayerChar(m_Owner)->Teleport(m_Pos);
    PonytailSkill();
}

void CQian::Snap(int SnappingClient)
{
	vec2 vertices[4] = {
		vec2(m_Pos.x - (64.0f/2), m_Pos.y - (64.0f/2)),
		vec2(m_Pos.x + (64.0f/2), m_Pos.y - (64.0f/2)),
		vec2(m_Pos.x + (64.0f/2), m_Pos.y + (64.0f/2)),
		vec2(m_Pos.x - (64.0f/2), m_Pos.y + (64.0f/2))
	};

	CNetObj_Laser *pObj1 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[0], sizeof(CNetObj_Laser)));
	CNetObj_Laser *pObj2 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[1], sizeof(CNetObj_Laser)));
	CNetObj_Laser *pObj3 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[2], sizeof(CNetObj_Laser)));
	CNetObj_Laser *pObj4 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[3], sizeof(CNetObj_Laser)));
	if(!pObj1 or !pObj2 or !pObj3 or !pObj4)
		return;

	if(m_Angle >= 720)
        m_Angle = 0;
    m_Angle++;

	// FUCK THIS MATH
	for (int i=0; i<4; i++)
	{
		Rotate(&vertices[i], m_Pos.x, m_Pos.y, m_Angle);
	}

	pObj1->m_X = (int)vertices[1].x;
	pObj1->m_Y = (int)vertices[1].y;
	pObj1->m_FromX = (int)vertices[0].x;
	pObj1->m_FromY = (int)vertices[0].y;
	pObj2->m_X = (int)vertices[2].x;
	pObj2->m_Y = (int)vertices[2].y;
	pObj2->m_FromX = (int)vertices[1].x;
	pObj2->m_FromY = (int)vertices[1].y;
	pObj3->m_X = (int)vertices[3].x;
	pObj3->m_Y = (int)vertices[3].y;
	pObj3->m_FromX = (int)vertices[2].x;
	pObj3->m_FromY = (int)vertices[2].y;
	pObj4->m_X = (int)vertices[0].x;
	pObj4->m_Y = (int)vertices[0].y;
	pObj4->m_FromX = (int)vertices[3].x;
	pObj4->m_FromY = (int)vertices[3].y;

	pObj1->m_StartTick = Server()->Tick();
	pObj2->m_StartTick = Server()->Tick();
	pObj3->m_StartTick = Server()->Tick();
	pObj4->m_StartTick = Server()->Tick();


    float AngleStep = 2.0f * pi / SIDES;

    float Radius = 13.0f*32.0f;

    for(int i=0; i<SIDES; i++)
	{
	    vec2 PartPosStart = m_Pos + vec2(Radius * cos(AngleStep*i), Radius * sin(AngleStep*i));
	    vec2 PartPosEnd = m_Pos + vec2(Radius * cos(AngleStep*(i+1)), Radius * sin(AngleStep*(i+1)));
	    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_aIDs[i], sizeof(CNetObj_Laser)));
	    if(!pObj)
	    	return;
	    pObj->m_X = (int)PartPosStart.x;
	    pObj->m_Y = (int)PartPosStart.y;
	    pObj->m_FromX = (int)PartPosEnd.x;
	    pObj->m_FromY = (int)PartPosEnd.y;
	    pObj->m_StartTick = Server()->Tick();
	}
}

void CQian::Reset()
{
    if(GameServer()->m_apPlayers[m_Owner])
        GameServer()->m_apPlayers[m_Owner]->KillCharacter();

    for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
	{
		if(m_IDs[i] >= 0){
			Server()->SnapFreeID(m_IDs[i]);
			m_IDs[i] = -1;
        }
	}
    for (unsigned i = 0; i < sizeof(m_aIDs) / sizeof(int); i ++)
	{
		if(m_aIDs[i] >= 0){
			Server()->SnapFreeID(m_aIDs[i]);
			m_aIDs[i] = -1;
        }
	}

    GetEyes()->Reset();
    
    str_copy(g_Config.m_SvMap, g_Config.m_SvAbyssMap, sizeof(g_Config.m_SvMap));

    GameServer()->m_World.DestroyEntity(this);
}

void CQian::PonytailSkill()
{
    if(Server()->Tick()%Server()->TickSpeed()*30 && m_UsingSkill == SKILL_NONE)
    {
        m_UsingSkill = rand()%NUM_SKILLS+1;
        m_SkillTimer = 5*50;
    }
    if(m_SkillTimer == 0)
        m_UsingSkill = 0;

    if(m_SkillTimer >= 0)
        m_SkillTimer--;
    
    switch (m_UsingSkill)
    {
    case SKILL_SUPERBOOM:
        if(Server()->Tick()%50 == 0)
            new CGrowingExplosion(GameWorld(), GetPos(), vec2(0,0), GetOwner(), 15, GROWINGEXPLOSIONEFFECT_ELECTRIC);
        break;

    case SKILL_PLASMA:
    if(Server()->Tick()%25 == 0)
    {
        CCharacter *pTarget = GameServer()->m_World.ClosestCharacter(GetPos(), 13*32, NULL);
        if(pTarget && pTarget->GetCID() != m_Owner)
            new CPlasma(GameWorld(), GetPos(), GetOwner(), pTarget->GetCID(), vec2(0,0), false, true, WEAPON_GRENADE);
    }
    break;
    
    default:
        break;
    }
}

CQian::CLaserEyes::CLaserEyes(CGameWorld *pGameWorld, int Type, CQian *OwnerQian) :
    CEntity(pGameWorld, CGameWorld::ENTTYPE_EYE)
{
    m_Pos = OwnerQian->GetPos();
    m_Type = Type;
    pOwnerQian = OwnerQian;
    m_Degres = 0;
    for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
        m_IDs[i] = Server()->SnapNewID();

    GameServer()->m_World.InsertEntity(this);
}

CQian::CLaserEyes *CQian::GetEyes(int Type)
{
    CLaserEyes *FoundEye;
    for(CLaserEyes *pEye = (CLaserEyes*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_EYE); pEye; pEye = (CLaserEyes *)pEye->TypeNext())
    {
        if(pEye->GetType() != Type && Type != -1)
            continue;

        return pEye;
    }

    return 0;
}

class CCharacter *CQian::CLaserEyes::FindTee()
{
    CCharacter *FoundTee = GameWorld()->ClosestCharacter(this->GetPos(), 32*13, NULL, true);
    if(FoundTee && FoundTee->GetCID() != pOwnerQian->GetOwner())
        return FoundTee;
    return 0;
}

void CQian::CLaserEyes::MoveTo(vec2 Pos)
{
    m_Pos = Pos;
    //m_Pos = vec2((m_Pos.x-Pos.x)/2.0f, (m_Pos.y-Pos.y)/2.0f);
}

void CQian::CLaserEyes::Tick()
{
    if ( m_Degres + 1 < 360 )
		m_Degres += 1;
	else
		m_Degres = 0;

    /*if(m_State == STATE_ATTACK)
    {
        {
            for(CCharacter *pChr = (CCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
            {
                if(pChr->GetPlayer()->GetZomb())
                    continue;
                MoveTo(pChr->m_Pos);
                Fire(pChr);
            }
        }
    }*/
/*    vec2 TargetPos;
    float xyPos = 13.0f*32.0f ;

    switch (m_Type)
    {
    case TYPE_LEFT:
        TargetPos = vec2(pOwnerQian->m_Pos.x - xyPos, pOwnerQian->m_Pos.y);
        break;
    
    case TYPE_RIGHT:
        TargetPos = vec2(pOwnerQian->m_Pos.x + xyPos, pOwnerQian->m_Pos.y);
        break;
    
    case TYPE_UP:   
        TargetPos = vec2(pOwnerQian->m_Pos.x, pOwnerQian->m_Pos.y - xyPos);
        break;
    
    case TYPE_DOWN:
        TargetPos = vec2(pOwnerQian->m_Pos.x, pOwnerQian->m_Pos.y + xyPos);
        break;
    
    default:
        TargetPos = m_Pos;
        break;
    }*/

    if(m_Type < TYPE_FREEGO)
    {
        float AngleStep = 2.0f * pi / 4;
        float Radius = 3.0f*32.0f;


        MoveTo(pOwnerQian->m_Pos + (GetDir((m_Degres-m_Type*90.0f)*pi/180) * 320));
        CCharacter *pTarget = FindTee();
        if(pTarget && Server()->Tick()%(2*50) == 0)
        {
            Fire(pTarget);
        }
    }

    if(m_Type == TYPE_FREEGO)
    {
        CCharacter *pTarget = FindTee();
        if(pTarget && pTarget->IsAlive())
        {
            m_Pos = m_Pos - pTarget->m_Pos/2.0f;
        }
        else
            MoveTo(pOwnerQian->m_Pos);
    }
}

void CQian::CLaserEyes::Reset()
{
    for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
	{
		if(m_IDs[i] >= 0){
			Server()->SnapFreeID(m_IDs[i]);
			m_IDs[i] = -1;
        }
	}

    GameServer()->m_World.DestroyEntity(this);
}

void CQian::CLaserEyes::Snap(int SnappingClient)
{
    int aIDSize = sizeof(m_IDs) / sizeof(int);

    float AngleStep = 2.0f * pi / aIDSize;

    float Radius = 3.0f*32.0f;

    for(int i=0; i<aIDSize; i++)
	{
	    vec2 PartPosStart = m_Pos + vec2(Radius * cos(AngleStep*i), Radius * sin(AngleStep*i));
	    vec2 PartPosEnd = m_Pos + vec2(Radius * cos(AngleStep*(i+1)), Radius * sin(AngleStep*(i+1)));
	    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
	    if(!pObj)
	    	return;
	    pObj->m_X = (int)PartPosStart.x;
	    pObj->m_Y = (int)PartPosStart.y;
	    pObj->m_FromX = (int)PartPosEnd.x;
	    pObj->m_FromY = (int)PartPosEnd.y;
	    pObj->m_StartTick = Server()->Tick();
	}
}

void CQian::CLaserEyes::Fire(CCharacter *pTarget)
{
    if(distance(this->GetPos(), pTarget->m_Pos) >= 13*32)
        return;
    vec2 Direction = normalize(vec2(pTarget->m_Pos.x+(float)(rand()%100-51), pTarget->m_Pos.y+(float)(rand()%100-51)) - m_Pos);
    new CLaser(GameWorld(), this->GetPos(), Direction, 13*32, pOwnerQian->GetOwner(), 2);
}

vec2 CQian::CLaserEyes::GetTeePos(int ClientID)
{
    if(GameServer()->m_apPlayers[ClientID] && GameServer()->m_apPlayers[ClientID]->GetCharacter())
        return GameServer()->m_apPlayers[ClientID]->GetCharacter()->m_Pos;
    return GetPos();
}