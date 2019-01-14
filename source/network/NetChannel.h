#ifndef NETCHANNEL_H__
#define NETCHANNEL_H__

#include "sxnetwork.h"
#include "NETbuff.h"
#include "unet.h"

typedef std::chrono::system_clock::time_point time_point;

struct CNetPeer
{
	uint32_t m_uIP;
	uint16_t m_uPort;
};

struct CSentPacket
{
	time_point m_time;

};

#define NET_LOSS_COUNT_FRAMES 16
#define NET_PACKET_BUFFER_SIZE 64

class CNetUser: public INetUser
{
	friend class CNetChannel;

public:
	CNetUser(CNetPeer *pNetPeer):
		m_netPeer(*pNetPeer)
	{
		memset(m_pbLoss, 0, sizeof(m_pbLoss[0]) * NET_LOSS_COUNT_FRAMES);
	}

	void sendMessage(byte *pData, int iLength, bool isReliable = false);
	void sendMessage(INETbuff *pNetBuff, bool isReliable = false);

	void kick(const char *szReason);

	int getAverageLoss();
	int getLastFrameLoss();

	int getLatency();

	ID getID();

	void setUserPointer(void *ptr);
	void *getUserPointer();

protected:
	CNetPeer m_netPeer;
	ID m_id = -1;
	uint8_t m_u8SourcePort;
	bool m_bDoDisconnect = false;
	bool m_bDoSendData = true;

	uint16_t m_uSeq = 0; //!< Last outgoing sequence
	uint16_t m_uAck = 0; //!< Last received ack (will be sent back to peer)

	uint16_t m_uSeqRel = 0; //!< Last outgoing sequence
	uint16_t m_uAckRel = 0; //!< Last received ack (will be sent back to peer)
	uint8_t m_u8RelSeqNum = 0;
	UINT m_uReliableCount = 0;
	bool m_isAllReliableSent = true;
	Array<Array<byte>, 4> m_aabOutgoindRels;

	UINT m_uReliableExpected = 0;
	CNETbuff m_bufInRel;
	//UINT m_uReliableGot = 0;

	byte m_pbLoss[NET_LOSS_COUNT_FRAMES];
	byte m_u8LossIdx = 0;
	byte m_iLoss = 0;
	byte m_iLastFrameLoss = 0;
	int m_iLatency = 0; // ms

	CNETbuff m_bufOut;
	CNETbuff m_bufOutRel;

	CSentPacket m_pSentPackets[NET_PACKET_BUFFER_SIZE];

	void *m_pUser = NULL;
};


class CNetChannel
{
public:
	CNetChannel(int iSocket);

	void update();

	void sendRaw(CNetPeer *pNetPeer, byte *pData, int iSize);
	//void sendMessage(CNetUser *pNetUser, INETbuff *pBuf, bool isReliable = false);

	bool readPacket(INETbuff *pBuf, CNetUser **ppFrom, CNetPeer *pNetPeer, bool *pIsRaw);

	ID acceptClient(CNetPeer *pNetPeer, CNetUser **ppNetUser = NULL, int iForceSourcePort=-1);

	const Array<CNetUser*> &getClients();

	void onClientDisconnected(PFNCLIENTHANDLER fnHandler);

protected:
	int m_iSocket;

	Array<CNetUser*> m_apUsers;

	void sendMessages();
	CNetUser *findUser(CNetPeer *pNetPeer, uint8_t u8SourcePort);
	bool isSequenceGreater(UINT uFirst, UINT uSecond);
	UINT getSequenceDelta(UINT uFirst, UINT uSecond);

	PFNCLIENTHANDLER m_fnOnClientDisconnected = NULL;
};

#endif
