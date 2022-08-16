
#ifndef GAME_SERVER_ENTITIES_GIGA_Qian
#define GAME_SERVER_ENTITIES_GIGA_Qian

#include <game/server/gamecontext.h>
#include <game/server/entity.h>
#include <vector>

#define SIDES 32
class CQian : public CEntity
{
public:
    CQian(CGameWorld *pGameWorld, int m_CoreID, int Owner, vec2 Pos);
    
    virtual void Tick();
    virtual void Reset();
    virtual void Snap(int SnappingClient);

    enum
    {
        SKILL_NONE,
        SKILL_LASEREYES,
        SKILL_SUPERBOOM,
        SKILL_PLASMA,
        NUM_SKILLS,
    };
    
    struct CLaserEyes : public CEntity
    {
        enum
        {
            TYPE_LEFT,
            TYPE_RIGHT,
            TYPE_UP,
            TYPE_DOWN,
            TYPE_FREEGO,
        };

        public:
            CLaserEyes(CGameWorld *pGameWorld, int Type, CQian *OwnerQian);
            vec2 GetPos() { return m_Pos; };

            virtual void Tick();
            virtual void Reset();
            virtual void Snap(int SnappingClient);

            int GetType() { return m_Type; };


        private:
            vec2 m_Pos;
            vec2 GetTeePos(int ClientID);
            CCharacter *FindTee();
            //bool SearchTee(vec2 Pos);

            void MoveTo(vec2 Pos);
            int m_Type;
            
            void Fire(CCharacter *Target);

            int m_IDs[16];
            CQian *pOwnerQian;
            int m_Degres;
    };

    public:
        CLaserEyes *GetEyes(int Type = -1);
        std::vector<CLaserEyes*>m_vEyes;
        void CreateEye();

    vec2 GetPos() { return m_Pos; };

    int GetOwner() { return m_Owner; };
private:
    int m_IDs[4];
    int m_aIDs[SIDES];

    int m_Owner;
    void Rotate(vec2* vertex, float x_orig, float y_orig, float angle);

    void PonytailSkill();
    int m_UsingSkill;
    int m_SkillTimer;

    float m_Angle;

    vec2 m_Vel;
};
#endif 