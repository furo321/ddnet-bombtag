/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
/* Based on Race mod stuff and tweaked by GreYFoX@GTi and others to fit our DDRace needs. */
#include "Bomb.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/version.h>

#define GAME_TYPE_NAME "BOMB"

CGameControllerBomb::CGameControllerBomb(class CGameContext *pGameServer) :
	IGameController(pGameServer), m_Teams(pGameServer)
{
	m_pGameType = GAME_TYPE_NAME;
	m_Bomb.m_ClientID = -1;
	m_Bomb.m_Tick = -1;
	m_RoundActive = false;
	for(int i = 0; i < MAX_CLIENTS; i++)
		aPlayers[i].m_State = STATE_NONE;

	InitTeleporter();
}

CGameControllerBomb::~CGameControllerBomb() = default;

void CGameControllerBomb::OnCharacterSpawn(CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);
	pChr->SetTeams(&m_Teams);
	pChr->SetTeleports(&m_TeleOuts, &m_TeleCheckOuts);
	m_Teams.OnCharacterSpawn(pChr->GetPlayer()->GetCID());

	pChr->SetArmor(10);
	pChr->GiveWeapon(WEAPON_GUN, true);
	pChr->SetWeapon(WEAPON_HAMMER);
	SetSkin(pChr->GetPlayer());
}

int CGameControllerBomb::OnCharacterDeath(CCharacter *pChr, CPlayer *pKiller, int Weapon)
{
	if(aPlayers[pChr->GetPlayer()->GetCID()].m_State >= STATE_ACTIVE && m_RoundActive)
	{
		GameServer()->SendBroadcast("You will automatically rejoin the game when the round is over", pChr->GetPlayer()->GetCID());
		aPlayers[pChr->GetPlayer()->GetCID()].m_State = STATE_ACTIVE;
	}
	return 0;
}

void CGameControllerBomb::HandleCharacterTiles(CCharacter *pChr, int MapIndex)
{
}

void CGameControllerBomb::OnPlayerConnect(CPlayer *pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientID = pPlayer->GetCID();

	if(!Server()->ClientPrevIngame(ClientID))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientID), GetTeamName(pPlayer->GetTeam()));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, -1, CGameContext::CHAT_SIX);

		GameServer()->SendChatTarget(ClientID, "BOMB Mod. Version: " GAME_VERSION);
	}
	aPlayers[pPlayer->GetCID()].m_Score = 0;
	if(m_RoundActive)
	{
		GameServer()->SendBroadcast("There's currently a game in progress, you'll join once the round is over!", ClientID);
	}
}

void CGameControllerBomb::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	int ClientID = pPlayer->GetCID();
	bool WasModerator = pPlayer->m_Moderating && Server()->ClientIngame(ClientID);

	IGameController::OnPlayerDisconnect(pPlayer, pReason);

	if(!GameServer()->PlayerModerating() && WasModerator)
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Server kick/spec votes are no longer actively moderated.");

	aPlayers[pPlayer->GetCID()].m_State = STATE_NONE;
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
		int seq = m_Tick % (SERVER_TICK_SPEED * 3);
		if(seq == 50)
			GameServer()->SendBroadcast("Waiting for players.", -1);
		else if(seq == 100)
			GameServer()->SendBroadcast("Waiting for players..", -1);
		else if(seq == 0)
			GameServer()->SendBroadcast("Waiting for players...", -1);
	}
	if(!m_RoundActive && AmountOfPlayers(STATE_ACTIVE) + AmountOfPlayers(STATE_ALIVE) > 1 && !m_Warmup)
	{
		GameServer()->SendBroadcast("Game started", -1);
		StartBombRound();
	}
	DoWinCheck();
	m_Tick++;
}

void CGameControllerBomb::DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	Team = ClampTeam(Team);
	if(Team == pPlayer->GetTeam())
		return;

	IGameController::DoTeamChange(pPlayer, Team, DoChatMsg);
	SetSkin(pPlayer);
}

CClientMask CGameControllerBomb::GetMaskForPlayerWorldEvent(int Asker, int ExceptID)
{
	if(Asker == -1)
		return CClientMask().set().reset(ExceptID);

	return m_Teams.TeamMask(GetPlayerTeam(Asker), ExceptID, Asker);
}

void CGameControllerBomb::InitTeleporter()
{
	if(!GameServer()->Collision()->Layers()->TeleLayer())
		return;
	int Width = GameServer()->Collision()->Layers()->TeleLayer()->m_Width;
	int Height = GameServer()->Collision()->Layers()->TeleLayer()->m_Height;

	for(int i = 0; i < Width * Height; i++)
	{
		int Number = GameServer()->Collision()->TeleLayer()[i].m_Number;
		int Type = GameServer()->Collision()->TeleLayer()[i].m_Type;
		if(Number > 0)
		{
			if(Type == TILE_TELEOUT)
			{
				m_TeleOuts[Number - 1].push_back(
					vec2(i % Width * 32 + 16, i / Width * 32 + 16));
			}
			else if(Type == TILE_TELECHECKOUT)
			{
				m_TeleCheckOuts[Number - 1].push_back(
					vec2(i % Width * 32 + 16, i / Width * 32 + 16));
			}
		}
	}
}

int CGameControllerBomb::GetPlayerTeam(int ClientID) const
{
	return m_Teams.m_Core.Team(ClientID);
}

void CGameControllerBomb::SetSkins()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			SetSkin(GameServer()->m_apPlayers[i]);
}

void CGameControllerBomb::SetSkin(CPlayer *pPlayer)
{
	if(m_Bomb.m_ClientID == pPlayer->GetCID())
	{
		str_copy(pPlayer->m_TeeInfos.m_aSkinName, "bomb", sizeof(pPlayer->m_TeeInfos.m_aSkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 0;
	}
	else
	{
		str_copy(pPlayer->m_TeeInfos.m_aSkinName, "default", sizeof(pPlayer->m_TeeInfos.m_aSkinName));
		pPlayer->m_TeeInfos.m_UseCustomColor = 0;
	}
}

void CGameControllerBomb::MakeRandomBomb()
{
	int Playing[MAX_CLIENTS];
	int Players = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(aPlayers[i].m_State == STATE_ALIVE)
			Playing[Players++] = i;

	if(Players)
		MakeBomb(Playing[secure_rand() % Players]);

	m_Bomb.m_Tick = 20 * SERVER_TICK_SPEED;
}

void CGameControllerBomb::MakeBomb(int ClientID)
{
	GameServer()->SendBroadcast("", m_Bomb.m_ClientID); // clear previous broadcast
	m_Bomb.m_ClientID = ClientID;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' is the new bomb!", Server()->ClientName(m_Bomb.m_ClientID));
	GameServer()->SendBroadcast(aBuf, ClientID);

	GameServer()->SendBroadcast("You are the new bomb!\nHit another player before the time runs out!", ClientID);
	SetSkins();
}

bool CGameControllerBomb::CanJoinTeam(int Team, int NotThisID)
{
	if(!m_RoundActive && Team != TEAM_SPECTATORS)
	{
		aPlayers[NotThisID].m_State = STATE_ACTIVE;
		return true;
	}
	if(Team == TEAM_SPECTATORS)
	{
		aPlayers[NotThisID].m_State = STATE_SPECTATING;
		GameServer()->SendBroadcast("You are a spectator now\nYou won't join when a new round begins", NotThisID);
		return true;
	}
	else
	{
		aPlayers[NotThisID].m_State = STATE_ACTIVE;
		GameServer()->SendBroadcast("You will join the game when the round is over", NotThisID);
		return false;
	}
}

void CGameControllerBomb::OnHammerHit(int ClientID, int TargetID)
{
	if(ClientID == m_Bomb.m_ClientID)
		MakeBomb(TargetID);
	else if(TargetID == m_Bomb.m_ClientID)
	{
		m_Bomb.m_Tick -= SERVER_TICK_SPEED;
		UpdateTimer();
	}

	else
		GameServer()->m_apPlayers[TargetID]->GetCharacter()->Freeze(1);

	SetSkins();
}

void CGameControllerBomb::DoWinCheck()
{
	if(!m_RoundActive)
		return;

	// check if bomb as left


	if(AmountOfPlayers(STATE_ALIVE) <= 1)
	{
		m_RoundActive = false;
		GameServer()->m_World.m_Paused = true;
		m_GameOverTick = Server()->Tick();
		m_Bomb.m_ClientID = -1;
		DoWarmup(3);
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(aPlayers[i].m_State == STATE_ALIVE)
				aPlayers[i].m_State = STATE_ACTIVE;
	}

	if(m_Bomb.m_Tick <= 0 || aPlayers[m_Bomb.m_ClientID].m_State <= STATE_SPECTATING)
	{
		if(AmountOfPlayers(STATE_ALIVE) > 2)
			EndBombRound(false);
		else
			EndBombRound(true);
	}
	else
	{
		if(m_Bomb.m_Tick % SERVER_TICK_SPEED == 0)
			UpdateTimer();

		m_Bomb.m_Tick--;
	}

	// Move killed players to spectator
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			if(aPlayers[i].m_State == STATE_ACTIVE && !m_Warmup)
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
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(aPlayers[i].m_State == State)
			Amount++;
	return Amount;
}

void CGameControllerBomb::EndBombRound(bool RealEnd)
{
	if(!RealEnd)
	{
		GameServer()->SendBroadcast("BOOM!", -1);
		GameServer()->CreateExplosion(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, m_Bomb.m_ClientID, WEAPON_GAME, false, 0);
		GameServer()->CreateSound(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, SOUND_GRENADE_EXPLODE);
		GameServer()->m_apPlayers[m_Bomb.m_ClientID]->KillCharacter();

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "'%s' eliminated!", Server()->ClientName(m_Bomb.m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, -1, CGameContext::CHAT_SIX);

		MakeRandomBomb();
	}
	else
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(aPlayers[i].m_State == STATE_ALIVE && i != m_Bomb.m_ClientID)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "'%s' won the round!", Server()->ClientName(i));
				GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, -1, CGameContext::CHAT_SIX);
				aPlayers[i].m_Score++;
				GameServer()->m_apPlayers[i]->m_Score = aPlayers[i].m_Score;
				break;
			}
		}
		if(aPlayers[m_Bomb.m_ClientID].m_State <= STATE_SPECTATING && GameServer()->m_apPlayers[m_Bomb.m_ClientID])
		{
			GameServer()->SendBroadcast("BOOM!", -1);
			GameServer()->CreateExplosion(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, m_Bomb.m_ClientID, WEAPON_GAME, false, 0);
			GameServer()->CreateSound(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, SOUND_GRENADE_EXPLODE);
			GameServer()->m_apPlayers[m_Bomb.m_ClientID]->KillCharacter();
		}
		else
		{
			GameServer()->SendBroadcast("Bomb left the game!", -1);
		}
		EndRound();
		DoWarmup(3);
		m_RoundActive = false;
		m_Bomb.m_ClientID = -1;
		EndRound();
		DoWarmup(3);
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(aPlayers[i].m_State == STATE_ALIVE)
				aPlayers[i].m_State = STATE_ACTIVE;
	}
}

void CGameControllerBomb::StartBombRound()
{
	m_RoundActive = true;
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(aPlayers[i].m_State == STATE_ACTIVE)
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_FLOCK, true);
			aPlayers[i].m_State = STATE_ALIVE;
		}

	MakeRandomBomb();
}

void CGameControllerBomb::UpdateTimer()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
			GameServer()->m_apPlayers[i]->GetCharacter()->SetArmor(m_Bomb.m_Tick / SERVER_TICK_SPEED);
	GameServer()->CreateDamageInd(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, 0, m_Bomb.m_Tick / SERVER_TICK_SPEED);
}