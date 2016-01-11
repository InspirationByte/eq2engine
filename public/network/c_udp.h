//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Controlled (un)reilable datagramm protocol
//
//				TODO:	buffer overflow protection
//						raw socket support
//////////////////////////////////////////////////////////////////////////////////

#ifndef C_UDP_H
#define C_UDP_H

#include "utils/DkList.h"
#include "utils/eqthread.h"

#include "Platform.h"
#include "net_defs.h"
#include "VirtualStream.h"

#define CUDP_MESSAGE_IMMEDIATE	(-3)
#define CUDP_MESSAGE_ERROR		(-2)

namespace Networking
{

using namespace Threading;

#define UDP_CDP_MAX_MESSAGEPAYLOAD		(16*1024)		// maximum payload for this protocol type; Tweak this if you have issues

enum CDPSendFlags_e
{
	CDPSEND_PRESERVE_ORDER	= (1 << 0),		// preserves ordering (not 100%)
	CDPSEND_GUARANTEED		= (1 << 1),		// just simply send UDP message
	CDPSEND_IMMEDIATE		= (1 << 2),		// send message immediately as it comes to buffer. It kills order preserving
};

struct cdp_message_t;

struct cdp_receive_info_t
{
	short			message_id;
	short			flags;

	cdp_message_t*	data;
};

struct cdp_receive_msgid_t
{
	sockaddr_in		addr;
	short			msgid;
	int				timeMs;
};

// recieve callback
typedef bool (*CDPRecvCallback_fn)( const cdp_receive_info_t& info );

// send callback
typedef bool (*CDPSendCallback_fn)( cdp_message_t* message );

//------------------------------------------------------------------------------
// message buffer
//------------------------------------------------------------------------------
struct cdp_message_t
{
	cdp_message_t()
	{
		bytestream = NULL;
		flags = 0;
		sendTimes.SetValue(0);
		sentTimeout.SetValue(0);
		removeTimeout.SetValue(0);
		sendTime = 0;
	}

	CEqInterlockedInteger	sendTimes;		// send times
	CEqInterlockedInteger	sentTimeout;	// timeout to send
	CEqInterlockedInteger	removeTimeout;	// timeout to remove

	uint32					sendTime;

	short					flags;

	sockaddr_in				addr;			// address of sender or receiver

	CMemoryStream*			bytestream;

	bool Write( void* pData, int nSize );

	void WriteReset();

	void ReadReset();
};

enum DeliveryStatus_e
{
	DELIVERY_INVALID = -1,
	DELIVERY_SUCCESS = 0,
	DELIVERY_FAILED,
};

struct msg_status_t
{
	short				message_id;
	DeliveryStatus_e	status;
};

class CUDPSocket
{
public:
								CUDPSocket();
								~CUDPSocket();

	bool						Init(int port);

	// sends message
	int							Send( char* data, int size, const sockaddr_in* to, short& msgId, short flags = CDPSEND_PRESERVE_ORDER );

	// receives message
	int							Recv( char* data, int size, sockaddr_in* from, short flags = 0 );

	// this really updates socket
	void						UpdateRecv( int timeMs );
	void						UpdateSend( int timeMs );

	int							GetReceivedMessageCount();
	int							GetSendPoolCount();

	// sets the recieved callback NOTE: this is called from another thread
	void						SetRecvCallback( const CDPRecvCallback_fn cb );

	void						GetMessageStatusList( DkList<msg_status_t>& msgStatusList );

	void						PrintStats();

	sockaddr_in					GetAddress() { return m_addr; }

protected:
	cdp_message_t*				GetFreeBuffer( int freeSpaceRequired, const sockaddr_in* to, short nFlags );

	void						PutMessageOrdered( const cdp_receive_info_t& info );

	bool						HasAnyMessageWithId( short message_id, const sockaddr_in& addr );

	void						RemoveMessageFromSendPool( short message_id );
	void						CheckMessageForResend( short message_id );

	void						SendMessageStatus( const sockaddr_in* to, short message_id, bool isOk );

	int							GetMessageUniqueID();

	void						Lock();
	void						Unlock();

	// sets the delay times of protocol
	void						SetDelayTimes(	int nSendBufferTimeout,			// wait time for full filling up buffer in milliseconds. Less value - higher send rate.
												int nUnconfirmedRemoveTimeout,	// remove unconfirmed (but sent) messages
												int nReceivedIdsTimeout			// time to flush confirmed message ids
												);

private:
	bool						m_init;

	SOCKET 						m_sock;

	sockaddr_in					m_addr;

	// messages are not limited, but they are splitted
	DkList<cdp_message_t*> 		m_pMessageQueue;

	DkList<cdp_receive_info_t>	m_ReceivedMessageQueue;
	DkList<cdp_receive_msgid_t>	m_receivedIds;

	DkList<msg_status_t>		m_messageSendStatus;

	int							m_nMessageIDInc;

	int							m_nSendTimeout;
	int							m_nUnconfirmedRemoveTimeout;
	int							m_nRecvTimeout;

	uint32						m_time;

	CDPRecvCallback_fn			m_nRecvCallback;

	CEqMutex					m_Mutex;
	CEqSignal					m_SendSignal;
};

}; // namespace CUDP

#endif // C_UDP_H
