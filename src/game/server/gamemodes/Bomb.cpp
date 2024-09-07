#include "Bomb.h"
#include "base/color.h"
#include "engine/shared/protocol.h"
#include "game/gamecore.h"
#include "game/generated/protocol.h"
#include "game/server/entities/character.h"
#include "game/server/player.h"
#include <random>

// Exchange this to a string that identifies your game mode.
// DM, TDM and CTF are reserved for teeworlds original modes.
// DDraceNetwork and TestDDraceNetwork are used by DDNet.
#define GAME_TYPE_NAME "BOMB"

CGameControllerBomb::CGameControllerBomb(class CGameContext *pGameServer) :
	IGameController(pGameServer), m_Teams(pGameServer)
{
	m_pGameType = GAME_TYPE_NAME;

	m_RoundActive = false;
	for(auto &aPlayer : m_aPlayers)
	{
		aPlayer.m_State = STATE_NONE;
		aPlayer.m_Bomb = false;
	}
}

CGameControllerBomb::~CGameControllerBomb() = default;

void CGameControllerBomb::OnCharacterSpawn(CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);
	pChr->SetTeams(&m_Teams);
	//pChr->SetTeleports(&m_TeleOuts, &m_TeleCheckOuts);
	m_Teams.OnCharacterSpawn(pChr->GetPlayer()->GetCid());

	pChr->SetArmor(10);
	pChr->GiveWeapon(WEAPON_GUN, true);
	pChr->SetWeapon(WEAPON_HAMMER);
	if(m_RoundActive)
		SetSkin(pChr->GetPlayer());
}

int CGameControllerBomb::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon)
{
	int ClientId = pVictim->GetPlayer()->GetCid();
	if(m_aPlayers[ClientId].m_State == STATE_ALIVE)
	{
		EliminatePlayer(ClientId);
	}

	if(m_aPlayers[ClientId].m_State > STATE_ACTIVE && m_RoundActive)
	{
		GameServer()->SendBroadcast("You will automatically rejoin the game when the round is over", ClientId);
		m_aPlayers[ClientId].m_State = STATE_ACTIVE;
	}
	return 0;
}

void CGameControllerBomb::OnPlayerConnect(CPlayer *pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientId = pPlayer->GetCid();

	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientId), GetTeamName(pPlayer->GetTeam()));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
		GameServer()->SendChatTarget(ClientId, "BOMB Mod. Source code: https://github.com/furo321/ddnet-bombtag");
	}
	m_aPlayers[pPlayer->GetCid()].m_Score = 0;
	if(m_RoundActive)
	{
		GameServer()->SendBroadcast("There's currently a game in progress, you'll join once the round is over!", ClientId);
	}
	SetSkin(pPlayer);
}

void CGameControllerBomb::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	IGameController::OnPlayerDisconnect(pPlayer, pReason);
	m_aPlayers[pPlayer->GetCid()].m_State = STATE_NONE;
}

void CGameControllerBomb::OnReset()
{
	IGameController::OnReset();
	m_Teams.Reset();
}

void CGameControllerBomb::Tick()
{
	IGameController::Tick();
	if(AmountOfPlayers(STATE_ACTIVE) == 1 && !m_RoundActive)
	{
		int Sequence = Server()->Tick() % (SERVER_TICK_SPEED * 3);
		if(Sequence == 50)
			GameServer()->SendBroadcast("Waiting for players.", -1);
		else if(Sequence == 100)
			GameServer()->SendBroadcast("Waiting for players..", -1);
		else if(Sequence == 0)
			GameServer()->SendBroadcast("Waiting for players...", -1);
	}
	if(!m_RoundActive && AmountOfPlayers(STATE_ACTIVE) + AmountOfPlayers(STATE_ALIVE) > 1 && !m_Warmup)
	{
		GameServer()->SendBroadcast("Game started", -1);
		StartBombRound();
	}
	DoWinCheck();
	if(m_RoundActive)
		SetSkins();
}

void CGameControllerBomb::DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	Team = ClampTeam(Team);
	if(Team == pPlayer->GetTeam())
		return;

	IGameController::DoTeamChange(pPlayer, Team, DoChatMsg);
	if(m_RoundActive)
		SetSkin(pPlayer);
}

int CGameControllerBomb::GetPlayerTeam(int ClientId) const
{
	return m_Teams.m_Core.Team(ClientId);
}

void CGameControllerBomb::SetSkins()
{
	for(auto &pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS)
			SetSkin(pPlayer);
}

void CGameControllerBomb::SetSkin(CPlayer *pPlayer)
{
	if(m_aPlayers[pPlayer->GetCid()].m_Bomb)
	{
		str_copy(pPlayer->m_TeeInfos.m_aSkinName, "bomb", sizeof(pPlayer->m_TeeInfos.m_aSkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 0;

		if(m_aPlayers[pPlayer->GetCid()].m_Tick < (3 * SERVER_TICK_SPEED))
		{
			pPlayer->m_TeeInfos.m_ColorFeet = 16777215; // white
			pPlayer->m_TeeInfos.m_UseCustomColor = 1;
			ColorRGBA Color = ColorRGBA(255 - m_aPlayers[pPlayer->GetCid()].m_Tick, 0, 0);
			pPlayer->m_TeeInfos.m_ColorBody = color_cast<ColorHSLA>(Color).PackAlphaLast(false);
		}
	}
	else
	{
		CSkinInfo *pRealSkin = &m_aPlayers[pPlayer->GetCid()].m_RealSkin;
		str_copy(pPlayer->m_TeeInfos.m_aSkinName, pRealSkin->m_aSkinName, sizeof(pPlayer->m_TeeInfos.m_aSkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = pRealSkin->m_UseCustomColor;
		pPlayer->m_TeeInfos.m_ColorBody = pRealSkin->m_aSkinBodyColor;
		pPlayer->m_TeeInfos.m_ColorFeet = pRealSkin->m_aSkinFeetColor;
	}
}

void CGameControllerBomb::MakeRandomBomb(int Count)
{
	int Playing[MAX_CLIENTS];
	int Players = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_aPlayers[i].m_State == STATE_ALIVE)
			Playing[Players++] = i;

	std::random_device RandomDevice;
	std::mt19937 g(RandomDevice());
	std::shuffle(Playing, Playing + Players, g);

	for(int i = 0; i < Count; i++)
	{
		MakeBomb(Playing[i], g_Config.m_BombtagSecondsToExplosion * SERVER_TICK_SPEED);
	}
}

void CGameControllerBomb::MakeBomb(int ClientId, int Ticks)
{
	GameServer()->SendBroadcast("", m_aPlayers[ClientId].m_Bomb); // clear previous broadcast
	m_aPlayers[ClientId].m_Bomb = true;
	m_aPlayers[ClientId].m_Tick = Ticks;

	GameServer()->SendBroadcast("You are the new bomb!\nHit another player before the time runs out!", ClientId);
}

int CGameControllerBomb::GetAutoTeam(int NotThisId)
{
	const int Team = TEAM_RED;
	m_aPlayers[NotThisId].m_State = STATE_ACTIVE;
	char aTeamJoinError[512];
	CanJoinTeam(Team, NotThisId, aTeamJoinError, sizeof(aTeamJoinError));
	return Team;
}

bool CGameControllerBomb::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
	if(!m_RoundActive && Team != TEAM_SPECTATORS)
	{
		str_copy(pErrorReason, "", ErrorReasonSize);
		m_aPlayers[NotThisId].m_State = STATE_ACTIVE;
		return true;
	}
	if(Team == TEAM_SPECTATORS)
	{
		m_aPlayers[NotThisId].m_State = STATE_SPECTATING;
		str_copy(pErrorReason, "You are a spectator now\nYou won't join when a new round begins", ErrorReasonSize);
		return true;
	}
	else
	{
		m_aPlayers[NotThisId].m_State = STATE_ACTIVE;
		str_copy(pErrorReason, "You will join the game when the round is over", ErrorReasonSize);
		return false;
	}
}

void CGameControllerBomb::OnHammerHit(int ClientId, int TargetId)
{
	if(m_aPlayers[ClientId].m_Bomb && !m_aPlayers[TargetId].m_Bomb)
	{
		m_aPlayers[ClientId].m_Bomb = false;
		MakeBomb(TargetId, m_aPlayers[ClientId].m_Tick);
	}
	else if(!m_aPlayers[ClientId].m_Bomb && m_aPlayers[TargetId].m_Bomb)
	{
		m_aPlayers[TargetId].m_Tick -= g_Config.m_BombtagBombDamage * SERVER_TICK_SPEED;
		UpdateTimer();
	}
	else if(!m_aPlayers[ClientId].m_Bomb && !m_aPlayers[TargetId].m_Bomb && g_Config.m_BombtagHammerFreeze)
	{
		CCharacterCore NewCore = GameServer()->m_apPlayers[TargetId]->GetCharacter()->GetCore();
		NewCore.m_FreezeEnd = Server()->Tick() + g_Config.m_BombtagHammerFreeze;
		NewCore.m_FreezeStart = Server()->Tick();
		GameServer()->m_apPlayers[TargetId]->GetCharacter()->m_FreezeTime = g_Config.m_BombtagHammerFreeze;
		GameServer()->m_apPlayers[TargetId]->GetCharacter()->SetCore(NewCore);
	}
}

void CGameControllerBomb::DoWinCheck()
{
	if(!m_RoundActive)
		return;

	if(AmountOfPlayers(STATE_ALIVE) <= 1)
	{
		EndBombRound(true);
		m_RoundActive = false;
		GameServer()->m_World.m_Paused = true;
		m_GameOverTick = Server()->Tick();
		DoWarmup(3);
		for(auto &aPlayer : m_aPlayers)
		{
			if(aPlayer.m_State == STATE_ALIVE)
			{
				aPlayer.m_State = STATE_ACTIVE;
				aPlayer.m_Bomb = false;
			}
		}
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(AmountOfBombs() == 0)
		{
			if(AmountOfPlayers(STATE_ALIVE) >= 2)
				EndBombRound(false);
			else
				EndBombRound(true);
		}
		if(m_aPlayers[i].m_Bomb)
		{
			if(m_aPlayers[i].m_Tick % SERVER_TICK_SPEED == 0)
				UpdateTimer();
			if(m_aPlayers[i].m_Tick <= 0)
			{
				ExplodeBomb(i);
			}
			m_aPlayers[i].m_Tick--;
		}
	}
	// Move killed players to spectator
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			if(m_aPlayers[i].m_State == STATE_ACTIVE && !m_Warmup)
			{
				if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				{
					GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS, true);
				}
			}
		}
	}
}

int CGameControllerBomb::AmountOfPlayers(int State = STATE_ACTIVE)
{
	int Amount = 0;
	for(auto &aPlayer : m_aPlayers)
		if(aPlayer.m_State == State)
			Amount++;
	return Amount;
}

int CGameControllerBomb::AmountOfBombs()
{
	int Amount = 0;
	for(auto &aPlayer : m_aPlayers)
		if(aPlayer.m_Bomb)
			Amount++;
	return Amount;
}

void CGameControllerBomb::EndBombRound(bool RealEnd)
{
	int Alive = 0;
	for(auto &aPlayer : m_aPlayers)
		if(aPlayer.m_State == STATE_ALIVE && !aPlayer.m_Bomb)
			Alive++;

	if(!RealEnd)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aPlayers[i].m_State == STATE_ALIVE)
			{
				m_aPlayers[i].m_Score += g_Config.m_BombtagScoreForSuriving;
				GameServer()->m_apPlayers[i]->m_Score = m_aPlayers[i].m_Score;
			}
		}
		MakeRandomBomb(std::ceil(Alive / (float)g_Config.m_BombtagBombsPerPlayer));
	}
	else
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aPlayers[i].m_State == STATE_ALIVE)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "'%s' won the round!", Server()->ClientName(i));
				GameServer()->SendChat(-1, TEAM_ALL, aBuf);
				m_aPlayers[i].m_Score += g_Config.m_BombtagScoreForWinning;
				GameServer()->m_apPlayers[i]->m_Score = m_aPlayers[i].m_Score;
				break;
			}
		}

		EndRound();
		DoWarmup(3);
		m_RoundActive = false;
		EndRound();
		DoWarmup(3);
		for(auto &aPlayer : m_aPlayers)
		{
			if(aPlayer.m_State == STATE_ALIVE)
			{
				aPlayer.m_State = STATE_ACTIVE;
				aPlayer.m_Bomb = false;
			}
		}
	}
}

void CGameControllerBomb::ExplodeBomb(int ClientId)
{
	GameServer()->CreateExplosion(GameServer()->m_apPlayers[ClientId]->m_ViewPos, ClientId, WEAPON_GAME, false, 0);
	GameServer()->CreateSound(GameServer()->m_apPlayers[ClientId]->m_ViewPos, SOUND_GRENADE_EXPLODE);
	m_aPlayers[ClientId].m_State = STATE_ACTIVE;
	GameServer()->m_apPlayers[ClientId]->KillCharacter();

	EliminatePlayer(ClientId);
}

void CGameControllerBomb::EliminatePlayer(int ClientId)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' eliminated!", Server()->ClientName(ClientId));
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);

	m_aPlayers[ClientId].m_Bomb = false;
	m_aPlayers[ClientId].m_State = STATE_ACTIVE;
}

void CGameControllerBomb::StartBombRound()
{
	int players = 0;
	m_RoundActive = true;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aPlayers[i].m_State == STATE_ACTIVE)
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_FLOCK, true);
			m_aPlayers[i].m_State = STATE_ALIVE;
			players++;
		}
	}
	MakeRandomBomb(std::ceil(players / (float)g_Config.m_BombtagBombsPerPlayer));
}

void CGameControllerBomb::UpdateTimer()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
		{
			if(m_aPlayers[i].m_Bomb)
			{
				GameServer()->CreateDamageInd(GameServer()->m_apPlayers[i]->m_ViewPos, 0, m_aPlayers[i].m_Tick / SERVER_TICK_SPEED);
				GameServer()->m_apPlayers[i]->GetCharacter()->SetArmor(m_aPlayers[i].m_Tick / SERVER_TICK_SPEED);
			}
		}
	}
}

void CGameControllerBomb::OnSkinChange(const char *pSkin, bool UseCustomColor, int ColorBody, int ColorFeet, int ClientId)
{
	CSkinInfo *pRealSkin = &m_aPlayers[ClientId].m_RealSkin;

	if(!str_find_nocase(pSkin, "bomb"))
		str_copy(pRealSkin->m_aSkinName, pSkin);
	else
		str_copy(pRealSkin->m_aSkinName, "default");

	pRealSkin->m_UseCustomColor = UseCustomColor;
	pRealSkin->m_aSkinBodyColor = ColorBody;
	pRealSkin->m_aSkinFeetColor = ColorFeet;
}
