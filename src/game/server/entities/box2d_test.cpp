#ifdef CONF_BOX2D
/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#define SCALE 30

#include "box2d_test.h"
#include <math.h>

#include <game/generated/protocol.h>

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

void CBox2DTest::Rotate(vec2* vertex, float x_orig, float y_orig, float angle)
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


CBox2DTest::CBox2DTest(CGameWorld *pGameWorld, vec2 Pos, vec2 Size, b2World* World, b2BodyType bodytype, float dens) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_Size = Size;
	m_World = World;
	m_LockPlayer = -1;
	m_VelY = 0;

	m_speed = 70.0f;

	b2Body* ground = NULL;
	{
		b2BodyDef bd;
		ground = m_World->CreateBody(&bd);
		b2EdgeShape shape;
		b2FixtureDef fd;
		fd.shape = &shape;
		fd.density = 0.0f;
		fd.friction = 0.6f;
		shape.SetTwoSided(b2Vec2(-20.0f, 0.0f), b2Vec2(20.0f, 0.0f));
		ground->CreateFixture(&fd);
		float hs[10] = {0.25f, 1.0f, 4.0f, 0.0f, 0.0f, -1.0f, -2.0f, -2.0f, -1.25f, 0.0f};
		float x = 20.0f, y1 = 0.0f, dx = 5.0f;
		for (int32 i = 0; i < 10; ++i)
		{
			float y2 = hs[i];
			shape.SetTwoSided(b2Vec2(x, y1), b2Vec2(x + dx, y2));
			ground->CreateFixture(&fd);
			y1 = y2;
			x += dx;
		}
		for (int32 i = 0; i < 10; ++i)
		{
			float y2 = hs[i];
			shape.SetTwoSided(b2Vec2(x, y1), b2Vec2(x + dx, y2));
			ground->CreateFixture(&fd);
			y1 = y2;
			x += dx;
		}
		shape.SetTwoSided(b2Vec2(x, 0.0f), b2Vec2(x + 40.0f, 0.0f));
		ground->CreateFixture(&fd);
		x += 80.0f;
		shape.SetTwoSided(b2Vec2(x, 0.0f), b2Vec2(x + 40.0f, 0.0f));
		ground->CreateFixture(&fd);
		x += 40.0f;
		shape.SetTwoSided(b2Vec2(x, 0.0f), b2Vec2(x + 10.0f, 5.0f));
		ground->CreateFixture(&fd);
		x += 20.0f;
		shape.SetTwoSided(b2Vec2(x, 0.0f), b2Vec2(x + 40.0f, 0.0f));
		ground->CreateFixture(&fd);
		x += 40.0f;
		shape.SetTwoSided(b2Vec2(x, 0.0f), b2Vec2(x, 20.0f));
		ground->CreateFixture(&fd);
	}

    // Car
	{
		b2PolygonShape chassis;
		b2Vec2 vertices[8];
		vertices[0].Set(-1.5f, -0.5f);
		vertices[1].Set(1.5f, -0.5f);
		vertices[2].Set(1.5f, 0.0f);
		vertices[3].Set(0.0f, 0.9f);
		vertices[4].Set(-1.15f, 0.9f);
		vertices[5].Set(-1.5f, 0.2f);
		chassis.Set(vertices, 6);
		b2CircleShape circle;
		circle.m_radius = 0.4f;
		b2BodyDef bd;
		bd.type = b2_dynamicBody;
		bd.position = b2Vec2(Pos.x / SCALE, Pos.y / SCALE);
		m_car = m_World->CreateBody(&bd);
		m_car->CreateFixture(&chassis, 1.0f);
		b2FixtureDef fd;
		fd.shape = &circle;
		fd.density = 1.0f;
		fd.friction = 0.9f;
		bd.position = b2Vec2((Pos.x / SCALE) -1.0f, (Pos.y / SCALE) - 0.35f);
		m_wheel1 = m_World->CreateBody(&bd);
		m_wheel1->CreateFixture(&fd);
		bd.position = b2Vec2((Pos.x / SCALE) + 1.0f, (Pos.y / SCALE) - 0.4f);
		m_wheel2 = m_World->CreateBody(&bd);
		m_wheel2->CreateFixture(&fd);
		b2WheelJointDef jd;
		b2Vec2 axis(0.0f, 1.0f);
		float mass1 = m_wheel1->GetMass();
		float mass2 = m_wheel2->GetMass();
		float hertz = 4.0f;
		float dampingRatio = 0.7f;
		float omega = 2.0f * b2_pi * hertz;
		jd.Initialize(m_car, m_wheel1, m_wheel1->GetPosition(), axis);
		jd.motorSpeed = 0.0f;
		jd.maxMotorTorque = 1000.0f;
		jd.enableMotor = true;
		jd.stiffness = mass1 * omega * omega;
		jd.damping = 2.0f * mass1 * dampingRatio * omega;
		jd.lowerTranslation = -0.25f;
		jd.upperTranslation = 0.25f;
		jd.enableLimit = true;
		m_spring1 = (b2WheelJoint*)m_World->CreateJoint(&jd);
		jd.Initialize(m_car, m_wheel2, m_wheel2->GetPosition(), axis);
		jd.motorSpeed = 0.0f;
		jd.maxMotorTorque = 10.0f;
		jd.enableMotor = true;
		jd.stiffness = mass2 * omega * omega;
		jd.damping = 2.0f * mass2 * dampingRatio * omega;
		jd.lowerTranslation = -0.25f;
		jd.upperTranslation = 0.25f;
		jd.enableLimit = true;
		m_spring2 = (b2WheelJoint*)m_World->CreateJoint(&jd);
	}

    for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
        m_IDs[i] = Server()->SnapNewID();

	GameWorld()->InsertEntity(this);
}

CBox2DTest::~CBox2DTest()
{
	for (unsigned i = 0; i < sizeof(m_IDs) / sizeof(int); i ++)
	{
		if(m_IDs[i] >= 0){
			Server()->SnapFreeID(m_IDs[i]);
			m_IDs[i] = -1;
        }
	}

	if (GameServer()->m_b2world)
	{
		dbg_msg("s","Destory");
		m_World->DestroyBody(m_car);
	}
}

void CBox2DTest::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CBox2DTest::HandleLockPlayer()
{
	if(m_LockPlayer < 0)
		return;

	if(!GameServer()->m_apPlayers[m_LockPlayer])
		return;

	if(!GameServer()->GetPlayerChar(m_LockPlayer))
		return;

	GameServer()->GetPlayerChar(0)->m_TeeJoint->SetMaxForce(0.0f);

	if(GameServer()->GetPlayerChar(m_LockPlayer)->m_Core.m_Vel.x > 0)
	{
	    m_spring1->SetMotorSpeed(m_speed);
	}
	else if(GameServer()->GetPlayerChar(m_LockPlayer)->m_Core.m_Vel.x < 0)
	    m_spring1->SetMotorSpeed(-m_speed);
	else
	    m_spring1->SetMotorSpeed(0.0f);

	GameServer()->GetPlayerChar(0)->Teleport(m_Pos);
}

void CBox2DTest::Tick()
{
	m_Pos = vec2(m_car->GetPosition().x * SCALE, m_car->GetPosition().y * SCALE);
	vec2 m_WheelPos = vec2(m_wheel1->GetPosition().x * SCALE, m_wheel1->GetPosition().y * SCALE);
	if (GameLayerClipped(m_Pos))
	{
		m_MarkedForDestroy = true;
	}

	if(m_DTick > 0)
		m_DTick--;

	if(m_LockPlayer > -1 && !GameServer()->m_apPlayers[m_LockPlayer])
	{
		m_LockPlayer = -1;
		return;
	}
	if(m_LockPlayer > -1 && !GameServer()->GetPlayerChar(m_LockPlayer))
	{
		m_LockPlayer = -1;
		return;
	}
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 30, NULL);
	if(pChr)
	{
		if(pChr->GetPlayer()->PressTab())
		{
			if(pChr->GetCID() == m_LockPlayer && !m_DTick && pChr->m_InVehicle)
			{
				m_LockPlayer = -1;
				pChr->m_InVehicle = false;
			}
			else if(!pChr->m_InVehicle && !m_DTick)
			{
				dbg_msg("GJ","%d",pChr->GetCID());
				m_DTick = 50;
				m_LockPlayer = pChr->GetCID();
				pChr->m_InVehicle = true;
			}
		}
	}

	HandleLockPlayer();
}

void CBox2DTest::TickPaused()
{

}

void CBox2DTest::Snap(int SnappingClient)
{
	vec2 pos(m_car->GetPosition().x * SCALE, m_car->GetPosition().y * SCALE);
	vec2 vertices[6] = {
		vec2(pos.x - 30, pos.y - 10),
		vec2(pos.x + 30, pos.y - 10),
		vec2(pos.x + 30, pos.y + 0),
		vec2(pos.x - 0, pos.y + 18),
        vec2(pos.x - 23, pos.y + 18),
        vec2(pos.x - 30, pos.y + 4)
	};

	if (NetworkClipped(SnappingClient) and NetworkClipped(SnappingClient, vertices[0]) and NetworkClipped(SnappingClient, vertices[1]) and NetworkClipped(SnappingClient, vertices[2]) and NetworkClipped(SnappingClient, vertices[3]))
		return;

	CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
    CNetObj_Laser *pObj1 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[0], sizeof(CNetObj_Laser)));
	CNetObj_Laser *pObj2 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[1], sizeof(CNetObj_Laser)));
	CNetObj_Laser *pObj3 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[2], sizeof(CNetObj_Laser)));
    CNetObj_Laser *pObj4 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[3], sizeof(CNetObj_Laser)));
    CNetObj_Laser *pObj5 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[4], sizeof(CNetObj_Laser)));
    CNetObj_Laser *pObj6 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[5], sizeof(CNetObj_Laser)));

	if(!pObj or !pObj1 or !pObj2 or !pObj3 or !pObj4 or !pObj5 or !pObj6)
		return;

	float angle = m_car->GetAngle(); // radians

	// headshot2017 was FUCK THIS MATH
	for (int i=0; i<6; i++)
	{
		Rotate(&vertices[i], pos.x, pos.y, angle);
	}

    //pObj->m_X = (int)m_wheel1->GetPosition().x;
    //pObj->m_Y = (int)m_wheel1->GetPosition().y;
    pObj->m_X = (int)(m_wheel1->GetPosition().x  * SCALE);
    pObj->m_Y = (int)(m_wheel1->GetPosition().y  * SCALE);
    pObj->m_Type = WEAPON_GRENADE;
    pObj->m_StartTick = Server()->Tick();

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

    pObj4->m_X = (int)vertices[4].x;
	pObj4->m_Y = (int)vertices[4].y;
	pObj4->m_FromX = (int)vertices[3].x;
	pObj4->m_FromY = (int)vertices[3].y;

    pObj5->m_X = (int)vertices[5].x;
	pObj5->m_Y = (int)vertices[5].y;
	pObj5->m_FromX = (int)vertices[4].x;
	pObj5->m_FromY = (int)vertices[4].y;

    pObj6->m_X = (int)vertices[0].x;
	pObj6->m_Y = (int)vertices[0].y;
	pObj6->m_FromX = (int)vertices[5].x;
	pObj6->m_FromY = (int)vertices[5].y;

	pObj1->m_StartTick = Server()->Tick();
	pObj2->m_StartTick = Server()->Tick();
	pObj3->m_StartTick = Server()->Tick();
	pObj4->m_StartTick = Server()->Tick();
    pObj5->m_StartTick = Server()->Tick();
    pObj6->m_StartTick = Server()->Tick();
}
#endif