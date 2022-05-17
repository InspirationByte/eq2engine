//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Controlled reilable datagramm protocol
//
//				TODO:	buffer overflow protection
//////////////////////////////////////////////////////////////////////////////////

#ifndef C_UDP_H
#define C_UDP_H

#include "ds/Array.h"
#include "ds/VirtualStream.h"

#include "utils/eqthread.h"

#include "core/platform/Platform.h"
#include "net_defs.h"

#define CUDP_MESSAGE_ID_IMMEDIATE			(-3)
#define CUDP_MESSAGE_ID_ERROR				(-2)
#define UDP_CDP_MAX_MESSAGEPAYLOAD			MAX_MESSAGE_LENGTH		// maximum payload for this protocol type; Tweak this if you have issues

using namespace Threading;

namespace Networking
{

enum ECDPSendFlags
{
	CDPSEND_GUARANTEED		= (1 << 0),		// just simply send UDP message
	CDPSEND_IMMEDIATE		= (1 << 1),		// send message immediately as it comes to buffer. It kills order preserving
	CDPSEND_IS_RESPONSE		= (1 << 2),		// contains response header
};

enum EDeliveryStatus
{
	DELIVERY_IN_PROGRESS = -1,
	DELIVERY_SUCCESS = 0,
	DELIVERY_FAILED,
};

enum ERecvMessageKind
{
	RECV_MSG_DATA = 0,
	RECV_MSG_RESPONSE_DATA,
	RECV_MSG_STATUS,
};

struct cdp_queued_message_t;

// recv callback
// when it's @type is RECV_MSG_STATUS, @size is EDeliveryStatus
typedef void (*CDPRecvPipe_fn)(void* thisptr, ubyte* data, int size, const sockaddr_in& from, short msgId, ERecvMessageKind type);

class CEqRDPSocket
{
public:
									CEqRDPSocket();
									~CEqRDPSocket();

	bool							Init(int port);
	void							Close();

	// puts message to send queue, or sends it immediately if flags used
	int								Send( char* data, int size, const sockaddr_in* to, short& msgId, short flags = 0 );

	// this really updates socket
	void							UpdateRecieve( int dtMs, CDPRecvPipe_fn recvFunc, void* recvObj );
	void							UpdateSendQueue( int dtMs, CDPRecvPipe_fn recvFunc, void* recvObj );

	int								GetSendPoolCount() const;

	void							PrintStats() const;

	sockaddr_in						GetAddress() const { return m_addr; }

protected:
	cdp_queued_message_t*			GetFreeBuffer( int freeSpaceRequired, const sockaddr_in* to, short nFlags );

	bool							HasAnyMessageWithId( short message_id, const sockaddr_in& addr );

	void							RemoveMessageFromSendPool( short message_id );
	void							CheckMessageForResend( short message_id );

	void							SendMessageStatus( const sockaddr_in* to, short message_id, bool isOk );

	int								GetMessageUniqueID();

private:
	bool							m_init;

	SOCKET 							m_sock;

	sockaddr_in						m_addr;

	struct cdp_receive_msgid_t
	{
		sockaddr_in		addr;
		short			msgid;
		int				timeMs;
	};

	// messages are not limited, but they are splitted
	Array<cdp_queued_message_t*> 	m_pMessageQueue{ PP_SL };
	Array<cdp_receive_msgid_t>		m_receivedIds{ PP_SL };

	int								m_nMessageIDInc;

	int								m_nSendTimeout;
	int								m_nUnconfirmedRemoveTimeout;
	int								m_nRecvTimeout;

	uint32							m_time;

	CEqMutex						m_Mutex;
	CEqSignal						m_SendSignal;
};

}; // namespace CUDP

#endif // C_UDP_H
