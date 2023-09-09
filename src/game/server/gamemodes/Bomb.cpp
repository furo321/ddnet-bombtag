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
	m_Score[ClientID] = 0;
}

void CGameControllerBomb::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	int ClientID = pPlayer->GetCID();
	bool WasModerator = pPlayer->m_Moderating && Server()->ClientIngame(ClientID);

	IGameController::OnPlayerDisconnect(pPlayer, pReason);

	if(!GameServer()->PlayerModerating() && WasModerator)
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Server kick/spec votes are no longer actively moderated.");
}

void CGameControllerBomb::OnReset()
{
	IGameController::OnReset();
	m_Teams.Reset();
}

void CGameControllerBomb::Tick()
{
	IGameController::Tick();

	if(AmountOfPlayers() > 1)
	{
		if(m_Bomb.m_ClientID != -1 && (!GameServer()->m_apPlayers[m_Bomb.m_ClientID] || GameServer()->m_apPlayers[m_Bomb.m_ClientID]->GetTeam() == TEAM_SPECTATORS))
			m_Bomb.m_ClientID = -1;

		if(!m_Warmup && m_Bomb.m_ClientID == -1)
			MakeRandomBomb();

		if(m_Bomb.m_Tick % SERVER_TICK_SPEED == 0)
		{
			CCharacter* Bomb = GameServer()->m_apPlayers[m_Bomb.m_ClientID]->GetCharacter();

			if(m_Bomb.m_Tick / SERVER_TICK_SPEED <= 10)
			{
				if(Bomb->IsAlive())
					Bomb->SetArmor(m_Bomb.m_Tick / SERVER_TICK_SPEED);
				GameServer()->CreateDamageInd(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, 0, m_Bomb.m_Tick / SERVER_TICK_SPEED);
			}
		}
		

		m_Bomb.m_Tick--;
		DoWinCheck();
	}
	else
	{
		if(AmountOfPlayers() == 1)
			GameServer()->SendBroadcast("Waiting for players...", -1);
	}
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
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			Playing[Players++] = i;

	if(Players)
		MakeBomb(Playing[secure_rand() % Players]);

	m_Bomb.m_Tick = 20 * SERVER_TICK_SPEED;
}

void CGameControllerBomb::MakeBomb(int ClientID)
{
	GameServer()->SendBroadcast("", m_Bomb.m_ClientID); // clear previous broadcast
	m_Bomb.m_ClientID = ClientID;
	GameServer()->SendBroadcast("You are the new bomb!\nHit another player before the time runs out!", ClientID);
	SetSkins();
}

void CGameControllerBomb::OnHammerHit(int ClientID, int TargetID)
{
	if(ClientID == m_Bomb.m_ClientID)
		MakeBomb(TargetID);
	else if(TargetID == m_Bomb.m_ClientID)
		m_Bomb.m_Tick -= SERVER_TICK_SPEED;
	else
		GameServer()->m_apPlayers[TargetID]->GetCharacter()->Freeze(1);

	SetSkins();
}

void CGameControllerBomb::DoWinCheck()
{
	if(m_Bomb.m_Tick <= 0)
	{
		GameServer()->SendBroadcast("BOOM!", -1);
		GameServer()->CreateExplosion(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, m_Bomb.m_ClientID, WEAPON_GAME, false, 0);
		GameServer()->CreateSound(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, SOUND_GRENADE_EXPLODE);
		GameServer()->m_apPlayers[m_Bomb.m_ClientID]->KillCharacter();
		MakeRandomBomb();

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s won the round!", Server()->ClientName(m_Bomb.m_ClientID));
		GameServer()->SendBroadcast(aBuf, -1);
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, -1, CGameContext::CHAT_SIX);
		m_Score[m_Bomb.m_ClientID]++;
		GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_Score = m_Score[m_Bomb.m_ClientID];
		EndRound();

		DoWarmup(5);
		GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
	}
}

int CGameControllerBomb::AmountOfPlayers()
{
	int Sum = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			Sum++;
	
	return Sum;
}