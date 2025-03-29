#include <antibot/antibot_data.h>
#include <base/system.h>
#include <engine/shared/packer.h>
#include <game/gamecore.h>
#define ANTIBOTAPI DYNAMIC_EXPORT

#include "antibot_interface.h"

#include <cstring>

static CAntibotData *g_pData;
static int g_Clicks[ANTIBOT_MAX_CLIENTS][SERVER_TICK_SPEED];
static CAntibotRoundData *g_pRoundData;
static int g_KickId;

extern "C" {

int AntibotAbiVersion()
{
	return ANTIBOT_ABI_VERSION;
}
void AntibotInit(CAntibotData *pCallbackData)
{
	g_pData = pCallbackData;
	g_pRoundData = 0;
	g_KickId = -1;
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
	if(g_KickId == ClientId)
		g_KickId = -1;
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
			// Mark the player as someone who should be kicked
			// Kicking here leads to weird crashes in "OnPredictedEarlyInput".
			g_KickId = ClientId;
		}
	}
}

void AntibotOnCharacterTick(int ClientId)
{
	if(g_pRoundData->m_Tick > SERVER_TICK_SPEED)
		g_Clicks[ClientId][(g_pRoundData->m_Tick - SERVER_TICK_SPEED) % SERVER_TICK_SPEED] = 0;
}

void AntibotOnHookAttach(int /*ClientId*/, bool /*Player*/) {}
void AntibotOnEngineTick(void)
{
	if(!g_pData)
		return;

	if(g_KickId != -1)
	{
		g_pData->m_pfnKick(g_KickId, "Autoclicker detected.", g_pData->m_pUser);
		g_KickId = -1;
	}
}

void AntibotOnEngineClientJoin(int /*ClientId*/, bool /*Sixup*/) {}
void AntibotOnEngineClientDrop(int /*ClientId*/, const char * /*pReason*/) {}

bool AntibotOnEngineClientMessage(int ClientId, const void *pData, int Size, int Flags)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pData, Size);
	CMsgPacker Packer(NETMSG_EX, true);

	// unpack msgid and system flag
	int Msg;
	bool Sys;
	CUuid Uuid;

	int Result = UnpackMessageId(&Msg, &Sys, &Uuid, &Unpacker, &Packer);
	if(Result == UNPACKMESSAGE_ERROR)
	{
		return false;
	}

	if(Msg == NETMSG_CLIENTVER)
	{
		CUuid *pConnectionId = (CUuid *)Unpacker.GetRaw(sizeof(*pConnectionId));
		int DDNetVersion = Unpacker.GetInt();
		const char *pDDNetVersionStr = Unpacker.GetString(CUnpacker::SANITIZE_CC);
		if(Unpacker.Error() || DDNetVersion < 0)
		{
			return false;
		}
		
		if(str_find_nocase(pDDNetVersionStr, "npmjs"))
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "Kicked ID %d due to client lib", ClientId);
			g_pData->m_pfnLog(aBuf, g_pData->m_pUser);
			g_pData->m_pfnKick(ClientId, "Unsupported client (0)", g_pData->m_pUser);
		}
	}

	if(Msg == NETMSGTYPE_CL_SAY)
	{
		CNetObjHandler NetObjHandler;
		void *pRawMsg = NetObjHandler.SecureUnpackMsg(Msg, &Unpacker);
		if(!pRawMsg)
			return false;

		CNetMsg_Cl_Say *pMsg = (CNetMsg_Cl_Say *)pRawMsg;
		char aTrimmed[1024];
		str_copy(aTrimmed, str_utf8_skip_whitespaces(pMsg->m_pMessage));
		str_utf8_trim_right(aTrimmed);

		int aSkeleton[1024];
		int SkeletonLength = str_utf8_to_skeleton(aTrimmed, aSkeleton, std::size(aSkeleton));

		char aSkeletonString[1024];
		for(int i = 0; i < 1024; i++)
		{
			str_utf8_encode(&aSkeletonString[i], aSkeleton[i]);
		}
		aSkeletonString[SkeletonLength] = '\0';

		char aNonAsciiStripped[1024];
		int Len = 0;

		for(char i : aSkeletonString)
		{
			if(i == '\0')
				break;

			if((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z'))
			{
				aNonAsciiStripped[Len] = i;
				Len++;
			}
		}

		if(str_find_nocase(aNonAsciiStripped, "krxciientxyz") || str_find_nocase(aNonAsciiStripped, "krxclientxyz"))
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "Autobanned '%s' with message '%s'", g_pRoundData->m_aCharacters[ClientId].m_aName, pMsg->m_pMessage);
			g_pData->m_pfnLog(aBuf, g_pData->m_pUser);

			g_pData->m_pfnBan(ClientId, 0, "Bot detected", g_pData->m_pUser);
			//g_pData->m_pfnKick(ClientId, "Bot detected", g_pData->m_pUser);
			return true;
		}
	}

	return false;
}

bool AntibotOnEngineServerMessage(int /*ClientId*/, const void * /*pData*/, int /*Size*/, int /*Flags*/) { return false; }
bool AntibotOnEngineSimulateClientMessage(int * /*pClientId*/, void * /*pBuffer*/, int /*BufferSize*/, int * /*pOutSize*/, int * /*pFlags*/) { return false; }
}
