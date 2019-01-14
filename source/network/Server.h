
/***********************************************************
Copyright � Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __SERVER_H
#define __SERVER_H

#include "sxnetwork.h"
#include "NetChannel.h"

#define NET_CHALLENGE_LIFETIME 3 /* seconds */

struct CChallengePair
{
	uint32_t m_uIP;
	uint32_t m_uChallenge;
	time_t m_time;
	bool m_isUsed;
};

class CServer
{
public:
	CServer(const char *szIp, unsigned short usPort);
	~CServer();

	void update();

	void registerMessage(CLIENT_COMMAND msgid, PFNMESSAGEHANDLER fnHandler);

	void sendMessage(byte *pData, int iLength, bool isReliable = false);
	void sendMessage(INETbuff *pNetBuff, bool isReliable = false);

	void onClientConnected(PFNCLIENTHANDLER fnHandler);
	void onClientDisconnected(PFNCLIENTHANDLER fnHandler);

protected:
	int m_iListener;
	CNetChannel *m_pNetChannel;
	Array<CChallengePair> m_vChallenges;

	uint32_t getChallenge(uint32_t uIP);
	bool isChallengeValid(uint32_t uIP, uint32_t uChallenge);

	void sendQueryInfo(CNetPeer *pPeer);

	bool validateTicket(CNetPeer *pPeer, byte *pData, int iLen);

	PFNMESSAGEHANDLER m_vMsgHandlers[CLC_LAST];

	PFNCLIENTHANDLER m_fnOnClientConnected = NULL;
};

#endif