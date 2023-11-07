/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_GAMEMODES_BOMB_H
#define GAME_SERVER_GAMEMODES_BOMB_H

#include <game/server/gamecontroller.h>
#include <game/server/teams.h>

#include <map>
#include <vector>

struct CScoreLoadBestTimeResult;
class CGameControllerBomb : public IGameController
{
public:
	CGameControllerBomb(class CGameContext *pGameServer);
	~CGameControllerBomb();

	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void HandleCharacterTiles(class CCharacter *pChr, int MapIndex) override;

	bool CanJoinTeam(int Team, int NotThisID) override;

	void OnPlayerConnect(class CPlayer *pPlayer) override;
	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason) override;

	void OnReset() override;

	void Tick() override;

	void DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg = true) override;

	CClientMask GetMaskForPlayerWorldEvent(int Asker, int ExceptID = -1) override;

	void InitTeleporter();

	int GetPlayerTeam(int ClientID) const;

	CGameTeams m_Teams;

	std::map<int, std::vector<vec2>> m_TeleOuts;
	std::map<int, std::vector<vec2>> m_TeleCheckOuts;

	std::shared_ptr<CScoreLoadBestTimeResult> m_pLoadBestTimeResult;

	// BOMB
	bool m_RoundActive = false;
	int64_t m_Tick = 0;

	enum
	{
		STATE_NONE = -2,
		STATE_SPECTATING,
		STATE_ACTIVE,
		STATE_ALIVE,
	};
	struct
	{
		int m_Score = 0;
		int m_State = STATE_NONE;
		int m_Tick = 0;
		bool m_Bomb = false;
	} aPlayers[MAX_CLIENTS];

	void SetSkins();
	void SetSkin(class CPlayer *pPlayer);
	void MakeRandomBomb(int Count);
	void MakeBomb(int ClientID, int Ticks);
	void DoWinCheck();
	int AmountOfPlayers(int State);
	int AmountOfBombs();
	void StartBombRound();
	void EndBombRound(bool RealEnd);
	void ExplodeBomb(int ClientID);
	void UpdateTimer();

	void OnHammerHit(int ClientID, int TargetID) override;
};
#endif // GAME_SERVER_GAMEMODES_BOMB_H
