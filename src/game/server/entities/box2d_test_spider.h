/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_BOX2D_SPIDER_H
#define GAME_SERVER_ENTITIES_BOX2D_SPIDER_H

#include <box2d/box2d.h>
#include <game/server/entity.h>

class CBox2DTestSpider : public CEntity
{
public:
	CBox2DTestSpider(CGameWorld *pGameWorld, vec2 Pos, vec2 Size, b2World* World, b2BodyType bodytype, float dens);
	~CBox2DTestSpider();

	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	b2Body* getBody() { if(m_Body)
                            return m_Body; }

    b2World* getWorld() { if(m_World)
                            return m_World; }

    b2RevoluteJoint* getMotorJoint() { if(m_motorJoint)
                            return m_motorJoint; }
	void Rotate(vec2* vertex, float x_orig, float y_orig, float angle);

private:
	b2World* m_World;
	b2Body* m_Body;
	b2Body* m_wheel;
    b2CircleShape m_circle;
	b2RevoluteJoint* m_motorJoint;
	b2Vec2 m_offset;
	bool m_motorOn;
	float m_motorSpeed;
	vec2 m_Size;

	int m_IDs[4];
	int m_aIDs[4];
	bool m_IsCollision;
	float m_Angle;
};

#endif
