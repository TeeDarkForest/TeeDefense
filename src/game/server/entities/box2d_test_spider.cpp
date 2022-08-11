/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#define SCALE 30

#include "box2d_test_spider.h"
#include <math.h>

#include <game/generated/protocol.h>

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

void CBox2DTestSpider::Rotate(vec2* vertex, float x_orig, float y_orig, float angle)
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

CBox2DTestSpider::CBox2DTestSpider(CGameWorld *pGameWorld, vec2 Pos, vec2 Size, b2World* World, b2BodyType bodytype, float dens)
:   CEntity(pGameWorld, CGameWorld::ENTTYPE_BOX2D)
{
    m_Pos = Pos;
	m_Size = Size;
	m_World = World;
	
	m_offset.Set(10.0f, 100.0f);
	m_motorSpeed = 3000.0f;
	m_motorOn = true;
	b2Vec2 pivot(0.0f, 16.0f);

	{
 	    b2BodyDef bd;
		bd.position.Set(m_Pos.x / SCALE, m_Pos.y / SCALE);
		bd.type = b2_dynamicBody;
		bd.bullet = true;
		bd.angle = 0.0f;
		m_Body = m_World->CreateBody(&bd);

		b2PolygonShape shape;
		shape.SetAsBox(48.0f / 2 / SCALE, 48.0f / 2 / SCALE);
		b2FixtureDef fd;
		fd.shape = &shape;
		fd.density = 1.0f;
		m_Body->CreateFixture(&fd);
	}

	{
		b2CircleShape shape;
		shape.m_radius = 1.6f;
		b2FixtureDef sd;
		sd.density = 1.0f;
		sd.shape = &shape;
		sd.filter.groupIndex = -1;
		b2BodyDef bd;
		bd.type = b2_dynamicBody;
		bd.position = b2Vec2(m_Pos.x / SCALE, m_Pos.y / SCALE) + pivot;
		m_wheel = m_World->CreateBody(&bd);
		m_wheel->CreateFixture(&sd);
	}
    

    for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
        m_IDs[i] = Server()->SnapNewID();
	for (unsigned i = 0; i < sizeof(m_aIDs) / sizeof(int); i ++)
        m_aIDs[i] = Server()->SnapNewID();
    
    GameServer()->m_World.InsertEntity(this);
}

CBox2DTestSpider::~CBox2DTestSpider()
{
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

	if (GameServer()->m_b2world)
	{
		m_World->DestroyBody(m_Body);
		m_World->DestroyBody(m_wheel);
	}

	for (unsigned i=0; i<GameServer()->m_b2TestSpider.size(); i++)
	{
		if (GameServer()->m_b2TestSpider[i] == this)
		{
			dbg_msg("s","Erase");
			GameServer()->m_b2TestSpider.erase(GameServer()->m_b2TestSpider.begin() + i);
			break;
		}
	}
}

void CBox2DTestSpider::Tick()
{
    m_Pos = vec2(m_Body->GetPosition().x * SCALE, m_Body->GetPosition().y * SCALE);
	if (GameLayerClipped(m_Pos))
	{
		m_MarkedForDestroy = true;
	}

}

void CBox2DTestSpider::TickPaused()
{

}

void CBox2DTestSpider::Snap(int SnappingClient)
{
	{ // Chair.
		vec2 pos(m_Body->GetPosition().x * SCALE, (m_Body->GetPosition().y) * SCALE);
		vec2 vertices[4] = {
			vec2(pos.x - (48.0f / 2.0f), pos.y - (48.0f / 2.0f)),
			vec2(pos.x + (48.0f / 2.0f), pos.y - (48.0f / 2.0f)),
			vec2(pos.x + (48.0f / 2.0f), pos.y + (48.0f / 2.0f)),
			vec2(pos.x - (48.0f / 2.0f), pos.y + (48.0f / 2.0f))
		};

		CNetObj_Laser *pObj1 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[0], sizeof(CNetObj_Laser)));
		CNetObj_Laser *pObj2 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[1], sizeof(CNetObj_Laser)));
		CNetObj_Laser *pObj3 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[2], sizeof(CNetObj_Laser)));
		CNetObj_Laser *pObj4 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[3], sizeof(CNetObj_Laser)));

		if(!pObj1 or !pObj2 or !pObj3 or !pObj4)
			return;

		float angle = m_Body->GetAngle(); // radians
		
		for (int i=0; i<4; i++)
		{
			Rotate(&vertices[i], pos.x, pos.y, angle);
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
	}

	{
		vec2 pos(m_wheel->GetPosition().x * SCALE, (m_wheel->GetPosition().y) * SCALE);
		vec2 vertices[4] = {
			vec2(pos.x - (24.0f / 2.0f), pos.y - (48.0f / 2.0f)),
			vec2(pos.x + (24.0f / 2.0f), pos.y - (48.0f / 2.0f)),
			vec2(pos.x + (24.0f / 2.0f), pos.y + (48.0f / 2.0f)),
			vec2(pos.x - (24.0f / 2.0f), pos.y + (48.0f / 2.0f))
		};

		CNetObj_Laser *pObj1 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_aIDs[0], sizeof(CNetObj_Laser)));
		CNetObj_Laser *pObj2 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_aIDs[1], sizeof(CNetObj_Laser)));
		CNetObj_Laser *pObj3 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_aIDs[2], sizeof(CNetObj_Laser)));
		CNetObj_Laser *pObj4 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_aIDs[3], sizeof(CNetObj_Laser)));

		if(!pObj1 or !pObj2 or !pObj3 or !pObj4)
			return;

		float angle = m_wheel->GetAngle(); // radians
		
		for (int i=0; i<4; i++)
		{
			Rotate(&vertices[i], pos.x, pos.y, angle);
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
	}

    /*float m_Radius = 30.0f;
    float AngleStep = 2.0f * pi / 4;
    for(int i=0; i<4; i++)
	{
	    vec2 PartPosStart = pos + vec2(m_Radius * cos(AngleStep*i), m_Radius * sin(AngleStep*i));
	    vec2 PartPosEnd = pos + vec2(m_Radius * cos(AngleStep*(i+1)), m_Radius * sin(AngleStep*(i+1)));
	    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
	    if(!pObj)
	    	return;
	    pObj->m_X = (int)PartPosStart.x-15;
	    pObj->m_Y = (int)PartPosStart.y-15;
	    pObj->m_FromX = (int)PartPosEnd.x-15;
	    pObj->m_FromY = (int)PartPosEnd.y-15;
	    pObj->m_StartTick = Server()->Tick();
    }*/
}