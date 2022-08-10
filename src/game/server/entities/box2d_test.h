/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_BOX2D_TEST_H
#define GAME_SERVER_ENTITIES_BOX2D_TEST_H

#include <box2d/box2d.h>
#include <game/server/entity.h>

class CBox2DTest : public CEntity
{
public:
	CBox2DTest(CGameWorld *pGameWorld, vec2 Pos, vec2 Size, b2World* World, b2BodyType, float dens);
	~CBox2DTest();

	virtual void Tick();
	virtual void Reset();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	b2Body* getBody() { return m_Body; }
	void Rotate(vec2* vertex, float x_orig, float y_orig, float angle);

	b2Body* m_car;
	b2Body* m_wheel1;
	b2Body* m_wheel2;

	float m_speed;
	b2WheelJoint* m_spring1;
	b2WheelJoint* m_spring2;

	void HandleLockPlayer();

private:
	b2World* m_World;
	b2Body* m_Body;
	vec2 m_Size;
	int m_IDs[6]; // for the other box corners
	int m_LockPlayer;
	int m_DTick;
	int m_VelY;
};

#endif
