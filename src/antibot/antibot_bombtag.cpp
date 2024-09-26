#include <antibot/antibot_data.h>
#include <base/system.h>
#include <engine/shared/protocol.h>
#include <game/gamecore.h>
#define ANTIBOTAPI DYNAMIC_EXPORT

#include "antibot_interface.h"

#include <cstring>

static CAntibotData *g_pData;
static int g_Clicks[ANTIBOT_MAX_CLIENTS][SERVER_TICK_SPEED];
static CAntibotRoundData *g_pRoundData;

extern "C" {

int AntibotAbiVersion()
{
	return ANTIBOT_ABI_VERSION;
}
void AntibotInit(CAntibotData *pCallbackData)
{
	g_pData = pCallbackData;
	g_pRoundData = 0;
	g_pData->m_pfnLog("bombtag antibot initialized", g_pData->m_pUser);
}

void AntibotRoundStart(CAntibotRoundData *pRoundData)
{
	g_pRoundData = pRoundData;
};

void AntibotRoundEnd(void){};
void AntibotUpdateData(void) {}
void AntibotDestroy(void) { g_pData = 0; }
void AntibotConsoleCommand(const char *pCommand)
{
	if(strcmp(pCommand, "dump") == 0)
	{
		g_pData->m_pfnLog("bombtag antibot", g_pData->m_pUser);
	}
	else
	{
		g_pData->m_pfnLog("unknown command", g_pData->m_pUser);
	}
}
void AntibotOnPlayerInit(int /*ClientId*/) {}
void AntibotOnPlayerDestroy(int ClientId)
{
	std::fill(std::begin(g_Clicks[ClientId]), std::end(g_Clicks[ClientId]), 0);
}
void AntibotOnSpawn(int /*ClientId*/) {}
void AntibotOnHammerFireReloading(int /*ClientId*/) {}
void AntibotOnHammerFire(int /*ClientId*/) {}
void AntibotOnHammerHit(int /*ClientId*/, int /*TargetId*/) {}

void AntibotOnDirectInput(int ClientId)
{
	if(!g_pData)
		return;

	CAntibotCharacterData Character = g_pRoundData->m_aCharacters[ClientId];

	// Auto clicker check
	if(CountInput(Character.m_aLatestInputs[1].m_Fire, Character.m_aLatestInputs[0].m_Fire).m_Presses)
		g_Clicks[ClientId][g_pRoundData->m_Tick % SERVER_TICK_SPEED] = 1;

	int ClicksPerSecond = 0;
	for(int Click : g_Clicks[ClientId])
	{
		if(Click == 1)
		{
			ClicksPerSecond++;
		}
	}

	if(ClicksPerSecond >= 13)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "High CPS detected for '%s' (%d) - %dcps", Character.m_aName, ClientId, ClicksPerSecond);
		g_pData->m_pfnLog(aBuf, g_pData->m_pUser);
		if(ClicksPerSecond >= 17)
		{
			str_format(aBuf, sizeof(aBuf), "Autoclicker detected.", ClicksPerSecond);
			g_pData->m_pfnKick(ClientId, aBuf, g_pData->m_pUser);
		}
	}
}

void AntibotOnCharacterTick(int ClientId)
{
	g_Clicks[ClientId][(g_pRoundData->m_Tick - 50) % SERVER_TICK_SPEED] = 0;
}

void AntibotOnHookAttach(int /*ClientId*/, bool /*Player*/) {}
void AntibotOnEngineTick(void) {}
void AntibotOnEngineClientJoin(int /*ClientId*/, bool /*Sixup*/) {}
void AntibotOnEngineClientDrop(int /*ClientId*/, const char * /*pReason*/) {}
bool AntibotOnEngineClientMessage(int /*ClientId*/, const void * /*pData*/, int /*Size*/, int /*Flags*/) { return false; }
bool AntibotOnEngineServerMessage(int /*ClientId*/, const void * /*pData*/, int /*Size*/, int /*Flags*/) { return false; }
bool AntibotOnEngineSimulateClientMessage(int * /*pClientId*/, void * /*pBuffer*/, int /*BufferSize*/, int * /*pOutSize*/, int * /*pFlags*/) { return false; }
}
