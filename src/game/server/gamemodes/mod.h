#ifndef GAME_SERVER_GAMEMODES_ZOD_H
#define GAME_SERVER_GAMEMODES_ZOD_H
#include <game/server/gamecontroller.h>

class CGameControllerMOD : public IGameController
{
public:
	vec2 *m_pTeleporter;

	CGameControllerMOD(class CGameContext *pGameServer);
	
	virtual void Snap(int SnappingClient);
	virtual void Tick();
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
};

#endif
