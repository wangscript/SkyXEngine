
/***********************************************************
Copyright � Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "INETbuff.h"

enum SERVER_COMMAND
{
	SVC_NOP = 0, // Do nothing
	SVC_DISCONNECT, // [string: message]
	SVC_NEWLEVEL, // [string: level]
	SVC_ENDLEVEL,
	SVC_ENTCONFIG, // default entity fields
	SVC_NEWBASELINE,
	SVC_SPAWNBASELINE, // [byte: id]

	SVC_DRAWPHYSICS,



	SVC_USER_FIRST = 128, // First user message
	SVC_USER_LAST = 254, // Last user message
	SVC_CUSTOM = 255, // Two-bytes message
	SVC_LAST
};

enum CLIENT_COMMAND
{
	CLC_NOP = 0, // Do nothing
	CLC_DROP, // client disconnected
	CLC_MOVE, // client movement

	CLC_LAST
};

class INetUser
{
public:
	virtual ~INetUser()
	{
	};

	virtual void sendMessage(byte *pData, int iLength, bool isReliable = false) = 0;
	virtual void sendMessage(INETbuff *pNetBuff, bool isReliable = false) = 0;

	virtual void kick(const char *szReason) = 0;
	
	virtual int getAverageLoss() = 0;
	virtual int getLastFrameLoss() = 0;

	virtual int getLatency() = 0;

	virtual ID getID() = 0;

	virtual void setUserPointer(void *ptr) = 0;
	virtual void *getUserPointer() = 0;
};

typedef void(*PFNMESSAGEHANDLER)(INETbuff *pData, INetUser *pNetUser);
typedef void(*PFNCLIENTHANDLER)(INetUser *pNetUser);

#define MAX_BACKUP_COMMANDS 16

#pragma pack(push, 1)
struct CUserCmd
{
	float m_fMoveForward;
	float m_fMoveSide;
	float m_fAngleY;
	float m_fAngleUpDown;
	float m_fLerpTime;
	bool m_inJump;
	bool m_inCrouch;
};
#pragma pack(pop)

typedef void(*PFNCLIENTMOVE)(INetUser *pNetUser, CUserCmd *pUserCmd);


#endif