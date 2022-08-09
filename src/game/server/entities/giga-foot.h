
#ifndef GAME_SERVER_ENTITIES_GIGA_MECHA_FOOT
#define GAME_SERVER_ENTITIES_GIGA_MECHA_FOOT

#include <box2d/box2d.h>
#include <game/server/gamecontext.h>
#include <game/server/entity.h>
#include <vector>

class CGigaMechaFoot
{
public:
    CGigaMechaFoot(CGameWorld *pGameWorld, int m_CoreID, int Health, b2World* World, b2BodyType bodytype);
    
    virtual void Tick();
    virtual void Reset();
    virtual void Snap(int SnappingClient);
    
    struct Foot
    {
        vec2 m_FootRootPos;
        vec2 m_FootEndPos;
        vec2 GetTeePos_StepIt();
    } m_Foot;

    b2Body* getBody() { return m_Body; }
private:
    b2World* m_World;
	b2Body* m_Body;
};
#endif 