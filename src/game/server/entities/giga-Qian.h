
#ifndef GAME_SERVER_ENTITIES_GIGA_Qian
#define GAME_SERVER_ENTITIES_GIGA_Qian

#include <box2d/box2d.h>
#include <game/server/gamecontext.h>
#include <game/server/entity.h>
#include <vector>

class CQian
{
public:
    CQian(CGameWorld *pGameWorld, int m_CoreID, int Health, b2World* World, b2BodyType bodytype);
    
    virtual void Tick();
    virtual void Reset();
    virtual void Snap(int SnappingClient);
    
    struct CLaserEyes
    {
        CLaserEyes(CGameWorld *pGameWorld);
        vec2 m_Pos;
        vec2 m_EyeEndPos;
        vec2 GetTeePos();

        void MoveTo(vec2 Pos);
    };

    std::vector<CLaserEyes*>m_vEyes;

    b2Body* getBody() { return m_Body; }
private:
    b2World* m_World;
	b2Body* m_Body;
};
#endif 