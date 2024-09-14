#ifndef GAME_SERVER_GAMEMODES_BOMB_H
#define GAME_SERVER_GAMEMODES_BOMB_H

#include <game/server/gamecontroller.h>

class CGameControllerBomb : public IGameController
{
public:
	CGameControllerBomb(class CGameContext *pGameServer);
	~CGameControllerBomb();

	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;

	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	int GetAutoTeam(int NotThisId) override;

	void OnPlayerConnect(class CPlayer *pPlayer) override;
	void OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason) override;

	void OnReset() override;

	void Tick() override;

	void DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg = true) override;

	int GetPlayerTeam(int ClientId) const;

	CGameTeams m_Teams;

	// BOMB
	bool m_RoundActive = false;

	enum
	{
		STATE_NONE = -2,
		STATE_SPECTATING,
		STATE_ACTIVE,
		STATE_ALIVE,
	};
	class CSkinInfo
	{
	public:
		char m_aSkinName[24];
		int m_aSkinFeetColor;
		int m_aSkinBodyColor;
		bool m_UseCustomColor;
	};
	struct
	{
		int m_Score = 0;
		int m_State = STATE_NONE;
		int m_Tick = 0;
		bool m_Bomb = false;
		CSkinInfo m_RealSkin;
	} m_aPlayers[MAX_CLIENTS];

	void SetSkins();
	void SetSkin(class CPlayer *pPlayer);
	void MakeRandomBomb(int Count);
	void MakeBomb(int ClientId, int Ticks);
	void DoWinCheck();
	int AmountOfPlayers(int State);
	int AmountOfBombs();
	void StartBombRound();
	void EndBombRound(bool RealEnd);
	void ExplodeBomb(int ClientId);
	void EliminatePlayer(int ClientId);
	void UpdateTimer();

	void OnTakeDamage(int Dmg, int From, int To, int Weapon) override;
	void OnSkinChange(const char *pSkin, bool UseCustomColor, int ColorBody, int ColorFeet, int ClientId) override;
};
#endif // GAME_SERVER_GAMEMODES_BOMB_H
