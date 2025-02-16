#include "Bomb.h"

#include <base/color.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/shared/protocol.h>
#include <game/gamecore.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/score.h>
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
	int ClientId = pChr->GetPlayer()->GetCid();
	IGameController::OnCharacterSpawn(pChr);
	pChr->SetTeams(&m_Teams);
	//pChr->SetTeleports(&m_TeleOuts, &m_TeleCheckOuts);
	m_Teams.OnCharacterSpawn(ClientId);

	pChr->SetArmor(10);
	pChr->GiveWeapon(WEAPON_GUN, true);
	pChr->SetWeapon(WEAPON_HAMMER);

	if(m_aPlayers[ClientId].m_Bomb && m_RoundActive)
		MakeBomb(ClientId, m_aPlayers[ClientId].m_Tick);

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
	GameServer()->Score()->LoadPlayerRoundsWon(pPlayer->GetCid(), Server()->ClientName(pPlayer->GetCid()));
	int ClientId = pPlayer->GetCid();

	if(pPlayer->GetTeam() == TEAM_SPECTATORS && m_aPlayers[ClientId].m_State != STATE_ACTIVE)
		m_aPlayers[ClientId].m_State = STATE_SPECTATING;

	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientId), GetTeamName(pPlayer->GetTeam()));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
		GameServer()->SendChatTarget(ClientId, "BOMB Mod. Source code: https://github.com/furo321/ddnet-bombtag");
	}
	if(m_RoundActive && m_aPlayers[ClientId].m_State != STATE_SPECTATING)
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

	// Bombtag reset
	for(auto &aPlayer : m_aPlayers)
	{
		if(aPlayer.m_State >= STATE_ACTIVE)
		{
			aPlayer.m_State = STATE_ACTIVE;
			aPlayer.m_Bomb = false;
		}
		m_RoundActive = false;
	}
}

void CGameControllerBomb::DoAfkLogic()
{
	if(!m_RoundActive)
		return;

	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		if(m_aPlayers[pPlayer->GetCid()].m_State == STATE_SPECTATING)
			continue;

		int Angle = (std::atan2(pChr->Core()->m_Input.m_TargetX, pChr->Core()->m_Input.m_TargetY) * 100.0f);
		int AfkHash = pChr->Core()->m_Input.m_PlayerFlags + Angle + pChr->Core()->m_Direction + pChr->Core()->m_Jumps;

		if(AfkHash == m_aPlayers[pPlayer->GetCid()].m_AfkHash)
		{
			m_aPlayers[pPlayer->GetCid()].m_TicksAfk++;
		}
		else
		{
			m_aPlayers[pPlayer->GetCid()].m_TicksAfk = 0;
		}

		m_aPlayers[pPlayer->GetCid()].m_AfkHash = AfkHash;
		if(m_aPlayers[pPlayer->GetCid()].m_TicksAfk > 20 * SERVER_TICK_SPEED)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "'%s' has been moved to spectators due to inactivity.", Server()->ClientName(pPlayer->GetCid()));
			GameServer()->SendChat(-1, TEAM_ALL, aBuf);
			GameServer()->m_apPlayers[pPlayer->GetCid()]->SetTeam(TEAM_SPECTATORS, false);
			m_aPlayers[pPlayer->GetCid()].m_TicksAfk = 0;
			m_aPlayers[pPlayer->GetCid()].m_State = STATE_SPECTATING;
		}
	}
}

void CGameControllerBomb::Tick()
{
	IGameController::Tick();
	DoAfkLogic();

	// Change to enqueued map
	if(!m_RoundActive && !m_Warmup && str_length(m_aEnqueuedMap))
	{
		Server()->ChangeMap(m_aEnqueuedMap);
		str_copy(m_aEnqueuedMap, "");
		return;
	}

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

	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(m_aPlayers[pPlayer->GetCid()].m_Bomb)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		pChr->GiveWeapon(WEAPON_HAMMER);
		pChr->GiveWeapon(WEAPON_GUN, true);
	}
}

void CGameControllerBomb::DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	Team = ClampTeam(Team);
	if(Team == pPlayer->GetTeam())
		return;

	if(Team == TEAM_SPECTATORS)
	{
		if(m_aPlayers[pPlayer->GetCid()].m_State == STATE_ALIVE)
			EliminatePlayer(pPlayer->GetCid());

		m_aPlayers[pPlayer->GetCid()].m_State = STATE_SPECTATING;
	}

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
		if(pPlayer)
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

	CCharacter *pChr = GameServer()->m_apPlayers[ClientId]->GetCharacter();
	if(pChr)
	{
		if(g_Config.m_BombtagBombWeapon != WEAPON_HAMMER)
			pChr->GiveWeapon(WEAPON_HAMMER, true);

		pChr->GiveWeapon(g_Config.m_BombtagBombWeapon);
		pChr->SetWeapon(g_Config.m_BombtagBombWeapon);
	}

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
		m_aPlayers[NotThisId].m_Bomb = false;
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

void CGameControllerBomb::OnTakeDamage(int Dmg, int From, int To, int Weapon)
{
	if(From == To || From == -1 || To == -1)
		return;

	if(Weapon < 0)
		return;

	if(Weapon != WEAPON_HAMMER && !m_aPlayers[From].m_Bomb)
		return;

	if(m_aPlayers[From].m_Bomb && Weapon != g_Config.m_BombtagBombWeapon)
		return;

	if(m_aPlayers[From].m_Bomb && !m_aPlayers[To].m_Bomb)
	{
		// new bomb
		GameServer()->SendBroadcast("", From);
		m_aPlayers[From].m_Bomb = false;
		MakeBomb(To, m_aPlayers[From].m_Tick);

		CCharacter *pOldBombChr = GameServer()->m_apPlayers[From]->GetCharacter();
		CCharacter *pNewBombChr = GameServer()->m_apPlayers[To]->GetCharacter();

		if(pOldBombChr && pNewBombChr)
		{
			pOldBombChr->GiveWeapon(g_Config.m_BombtagBombWeapon, true);
			pOldBombChr->GiveWeapon(WEAPON_HAMMER, true);
			pOldBombChr->SetWeapon(WEAPON_HAMMER);

			pNewBombChr->GiveWeapon(g_Config.m_BombtagBombWeapon, false);
			if(m_aPlayers[To].m_Tick < g_Config.m_BombtagMinSecondsToExplosion * SERVER_TICK_SPEED && g_Config.m_BombtagMinSecondsToExplosion)
				m_aPlayers[To].m_Tick = g_Config.m_BombtagMinSecondsToExplosion * SERVER_TICK_SPEED;
			pNewBombChr->SetWeapon(g_Config.m_BombtagBombWeapon);
		}
	}
	else if(!m_aPlayers[From].m_Bomb && m_aPlayers[To].m_Bomb)
	{
		// damage to bomb
		m_aPlayers[To].m_Tick -= g_Config.m_BombtagBombDamage * SERVER_TICK_SPEED;
		UpdateTimer();
	}
	else if(!m_aPlayers[From].m_Bomb && !m_aPlayers[To].m_Bomb && g_Config.m_BombtagHammerFreeze)
	{
		CCharacter *pChr = GameServer()->m_apPlayers[To]->GetCharacter();
		if(!pChr)
			return;

		CCharacterCore NewCore = pChr->GetCore();
		NewCore.m_FreezeEnd = Server()->Tick() + g_Config.m_BombtagHammerFreeze;
		NewCore.m_FreezeStart = Server()->Tick();
		pChr->m_FreezeTime = g_Config.m_BombtagHammerFreeze;
		pChr->SetCore(NewCore);
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
	if(!m_RoundActive)
		return;

	int Alive = 0;
	for(auto &aPlayer : m_aPlayers)
		if(aPlayer.m_State == STATE_ALIVE && !aPlayer.m_Bomb)
			Alive++;

	if(!RealEnd)
	{
		// for(int i = 0; i < MAX_CLIENTS; i++)
		// {
		// 	if(m_aPlayers[i].m_State == STATE_ALIVE)
		// 	{
		// 		GameServer()->m_apPlayers[i]->m_Score = GameServer()->m_apPlayers[i]->m_Score.value_or(0) + 1;
		// 	}
		// }
		const int BombsPerPlayer = g_Config.m_BombtagBombsPerPlayer;
		MakeRandomBomb(std::ceil((Alive / (float)BombsPerPlayer) - (BombsPerPlayer == 1 ? 1 : 0)));
	}
	else
	{
		bool WinnerAnnounced = false;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aPlayers[i].m_State == STATE_ALIVE)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "'%s' won the round!", Server()->ClientName(i));
				GameServer()->SendChat(-1, TEAM_ALL, aBuf);
				GameServer()->m_apPlayers[i]->m_Score = GameServer()->m_apPlayers[i]->m_Score.value_or(0) + 1;
				GameServer()->Score()->SaveStats(Server()->ClientName(i), true);
				WinnerAnnounced = true;
				break;
			}
		}
		if(!WinnerAnnounced)
		{
			GameServer()->SendChat(-1, TEAM_ALL, "Noone won the round!");
		}

		if(g_Config.m_BombtagMysteryChance && rand() % 101 <= g_Config.m_BombtagMysteryChance)
		{
			const char *pLine = GameServer()->Server()->GetMysteryRoundLine();
			if(pLine)
			{
				GameServer()->SendChat(-1, TEAM_ALL, "MYSTERY ROUND!");
				GameServer()->Console()->ExecuteFile(g_Config.m_SvMysteryRoundsResetFileName);
				GameServer()->Console()->ExecuteLine(pLine);
				m_WasMysteryRound = true;
			}
		}
		else if(m_WasMysteryRound)
		{
			GameServer()->Console()->ExecuteFile(g_Config.m_SvMysteryRoundsResetFileName);
			m_WasMysteryRound = false;
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
	if(!GameServer()->m_apPlayers[ClientId])
		return;

	GameServer()->CreateExplosion(GameServer()->m_apPlayers[ClientId]->m_ViewPos, ClientId, WEAPON_GAME, true, 0);
	GameServer()->CreateSound(GameServer()->m_apPlayers[ClientId]->m_ViewPos, SOUND_GRENADE_EXPLODE);
	m_aPlayers[ClientId].m_State = STATE_ACTIVE;

	// Collateral damage
	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!g_Config.m_BombtagCollateralDamage)
			break;

		if(!pPlayer)
			continue;

		CCharacter *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		if(ClientId == pPlayer->GetCid())
			continue;

		if(distance(pPlayer->m_ViewPos, GameServer()->m_apPlayers[ClientId]->m_ViewPos) <= 96)
		{
			GameServer()->m_apPlayers[ClientId]->KillCharacter();
			EliminatePlayer(pPlayer->GetCid(), true);
		}
	}
	GameServer()->m_apPlayers[ClientId]->KillCharacter();
	EliminatePlayer(ClientId);
}

void CGameControllerBomb::EliminatePlayer(int ClientId, bool Collateral)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' eliminated%s!", Server()->ClientName(ClientId), Collateral ? " by collateral damage" : "");
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);
	GameServer()->Score()->SaveStats(Server()->ClientName(ClientId), false);

	m_aPlayers[ClientId].m_Bomb = false;
	m_aPlayers[ClientId].m_State = STATE_ACTIVE;
}

void CGameControllerBomb::StartBombRound()
{
	int Players = 0;
	m_RoundActive = true;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aPlayers[i].m_State == STATE_ACTIVE)
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_FLOCK, true);
			m_aPlayers[i].m_State = STATE_ALIVE;
			Players++;
		}
	}
	const int BombsPerPlayer = g_Config.m_BombtagBombsPerPlayer;
	MakeRandomBomb(std::ceil((Players / (float)BombsPerPlayer) - (BombsPerPlayer == 1 ? 1 : 0)));
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
