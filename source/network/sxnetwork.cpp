
/***********************************************************
Copyright � Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "sxnetwork.h"
#include "Server.h"
#include "Client.h"

#if !defined(DEF_STD_REPORT)
#define DEF_STD_REPORT
report_func g_fnReportf = DefReport;
#endif

CServer *g_pServer = NULL;
CClient *g_pClient = NULL;

//##########################################################################


//#define SN_PRECOND(ret) if(!g_pLevel){LibReport(REPORT_MSG_LEVEL_ERROR, "%s - sxlevel is not init", GEN_MSG_LOCATION);return ret;}

//##########################################################################

SX_LIB_API void SNetwork_Dbg_Set(report_func fnFunc)
{
	g_fnReportf = fnFunc;
}

SX_LIB_API void SNetwork_0Create()
{
	Core_SetOutPtr();
	Core_0RegisterConcmdArg("connect", [](int argc, const char ** argv){
		if(SNetwork_IsConnected())
		{
			SNetwork_Disconnect();
		}

		if(argc < 2)
		{
			printf("Usage: connect <ip:port>\n");
			return;
		}

		int len = strlen(argv[1]);
		char *szAddr = (char*)alloca(len + 1);
		memcpy(szAddr, argv[1], len + 1);
		char *szAddrParts[2];
		int iPartsCount = parse_str(szAddr, szAddrParts, 2, ':');
		if(iPartsCount != 2)
		{
			printf("Invalid address\n");
			return;
		}
		if(!strcmp(szAddrParts[0], "localhost"))
		{
			strcpy(szAddrParts[0], "127.0.0.1");
		}

		uint16_t usPort;
		if(!sscanf(szAddrParts[1], "%hu", &usPort))
		{
			printf("Invalid port\n");
			return;
		}

		SNetwork_Connect(szAddrParts[0], usPort);
	}, "Connect to game server");

	Core_0RegisterConcmd("disconnect", []()
	{
		if(SNetwork_IsConnected())
		{
			SNetwork_Disconnect();
		}
		else
		{
			printf("Not connected\n");
			return;
		}
	}, "Connect to game server");
}

SX_LIB_API void SNetwork_AKill()
{
	//SN_PRECOND(_VOID);

	//mem_delete(g_pNet);
}

SX_LIB_API void SNetwork_Update()
{
	if(g_pServer)
	{
		g_pServer->update();
	}
	if(g_pClient)
	{
		g_pClient->update();
	}
}

//##########################################################################

SX_LIB_API void SNetwork_InitServer(unsigned short usPort, const char *szIp)
{
	assert(!g_pServer);

	if(!szIp)
	{
		szIp = "0.0.0.0";
	}

	g_pServer = new CServer(szIp, usPort);
	g_pServer->registerMessage(CLC_DROP, [](INETbuff *pData, INetUser *pNetUser){
		// drop client
		printf("Client sent drop\n");
		pNetUser->kick("Client sent drop");
	});
}

SX_LIB_API void SNetwork_FinishServer()
{
	assert(g_pServer);

	mem_delete(g_pServer);
}


SX_LIB_API void SNetwork_Connect(const char *szIp, unsigned short usPort)
{
	assert(!g_pClient);

	g_pClient = new CClient();
	g_pClient->registerMessage(SVC_DISCONNECT, [](INETbuff *pData, INetUser *pNetUser){
		char buf[128];
		pData->readString(buf, sizeof(buf));
		printf("Kicked: %s\n", buf);
	});
	g_pClient->registerMessage(SVC_NEWLEVEL, [](INETbuff *pData, INetUser *pNetUser){
		char buf[128];
		pData->readString(buf, sizeof(buf));
		Core_0ConsoleExecCmd("map %s", buf);
	});
	g_pClient->registerMessage(SVC_ENDLEVEL, [](INETbuff *pData, INetUser *pNetUser){
		Core_0ConsoleExecCmd("endmap");
	});
	g_pClient->registerMessage(SVC_ENTCONFIG, [](INETbuff *pData, INetUser *pNetUser){
		printf("Received entconfig\n");
		char szClass[256], szKey[256], szValue[256];

		//@TODO: Use that data!

		// dynclasses
		uint32_t uEntCount = pData->readUInt32();
		for(uint32_t i = 0; i < uEntCount; ++i)
		{
			pData->readString(szClass, sizeof(szClass));
			uint16_t usKeyCount = pData->readUInt16();
			for(uint16_t j = 0; j < usKeyCount; ++j)
			{
				pData->readString(szKey, sizeof(szKey));
				pData->readString(szValue, sizeof(szValue));
			}
		}

		// defaults
		uEntCount = pData->readUInt32();
		for(uint32_t i = 0; i < uEntCount; ++i)
		{
			pData->readString(szClass, sizeof(szClass));
			uint16_t usKeyCount = pData->readUInt16();
			for(uint16_t j = 0; j < usKeyCount; ++j)
			{
				pData->readString(szKey, sizeof(szKey));
				pData->readString(szValue, sizeof(szValue));
			}
		}
	});
	g_pClient->connect(szIp, usPort);
}

SX_LIB_API void SNetwork_Disconnect()
{
	assert(g_pClient);

	g_pClient->disconnect();
	mem_delete(g_pClient);
}

SX_LIB_API bool SNetwork_IsConnected()
{
	return(g_pClient != NULL);
}

SX_LIB_API void SNetwork_BroadcastMessage(byte *pData, int iLength, bool isReliable)
{
	if(g_pServer)
	{
		g_pServer->sendMessage(pData, iLength, isReliable);
	}
}

SX_LIB_API void SNetwork_BroadcastMessageBuf(INETbuff *pNetBuff, bool isReliable)
{
	if(g_pServer)
	{
		g_pServer->sendMessage(pNetBuff, isReliable);
	}
}

SX_LIB_API INETbuff *SNetwork_CreateBuffer()
{
	return(new CNETbuff());
}

SX_LIB_API void SNetwork_FreeBuffer(INETbuff *pBuf)
{
	mem_delete(pBuf);
}

SX_LIB_API void SNetwork_OnClientConnected(PFNCLIENTHANDLER fnHandler)
{
	if(g_pServer)
	{
		g_pServer->onClientConnected(fnHandler);
	}
}

SX_LIB_API void SNetwork_OnClientDisconnected(PFNCLIENTHANDLER fnHandler)
{
	if(g_pServer)
	{
		g_pServer->onClientDisconnected(fnHandler);
	}
}
