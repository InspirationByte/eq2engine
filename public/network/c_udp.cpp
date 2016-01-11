//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Controlled (un)reilable datagramm protocol
//
//				TODO:	buffer overflow protection
//						raw socket support
//////////////////////////////////////////////////////////////////////////////////

#include "c_udp.h"
#include "net_defs.h"

#include "DebugInterface.h"

#include "utils/strtools.h"
#include "utils/CRC32.h"
#include "utils/KeyValues.h"

#include "ConCommand.h"

ConVar net_fakelag("net_fakelag", "0", "Simulate lagging packets\n", CV_CHEAT);

namespace Networking
{

//
// protocol definitions
//

#define UDP_CDP_IDENT						MCHAR4('E','Q','D','P')		// signature: EqDatagramPacket

#define UDP_CDP_PROTOCOL_VERSION			7			// protocol version

#define UDP_CDP_MIN_SEND_BUFFER				2048		// minimal send buffer; tweak this if you have bandwith issues
#define UDP_CDP_MIN_MESSAGESIZE				512

#define UDP_CDP_MAX_QUEUE_BUFFERS			16			// maximum amount of queue buffers

#define UDP_CDP_FORCESEND_FILLPERCENTAGE	(0.7)

#define CDP_MAX_MESSAGE_ID					32760

#define CDP_DATA_PACKETDATA					0xda1a
#define CDP_DATA_PACKETSTATE				0x51a1

// default settings: tweak this if you have bandwith issues in cdp_config.cfg
#define CDP_SEND_TIMEOUT_MS					10			// 40 ms delay to send
#define CDP_SEND_REMOVE_TIMEOUT_MS			1000		// Guaranteed messages only - 5 sec delay to send unacknowledged messages
#define CDP_RECV_TIMEOUT_MS					1500		// Guaranteed messages only - 5 sec delay to receive message and then remove it from received list

//ConVar net_cudp_sendtime("net_cudp_sendtime", "100", 1.0, 200.0, "Delay to send message buffer", CV_CHEAT);
//ConVar net_cudp_removetime("net_cudp_removetime", "500", 1.0, 80.0, "Delay to send message buffer", CV_CHEAT);
//ConVar net_cudp_recvtimeout("net_cudp_recvtimeout", "1500", 1.0, 80.0, "Delay to send message buffer", CV_CHEAT);

// big message header
struct udp_cdp_hdr_s
{
	int				ident;					// UDP_CDP_IDENT
	ubyte			protocol_version;		// UDP_RCP_PROTOCOL_VERSION
	ubyte			flags;					// message flags ( CDPSendFlags_e )

	//unsigned long	crc32;					// message CRC32
	short			message_id;				// less than CDP_MAX_MESSAGE_ID
};

ALIGNED_TYPE(udp_cdp_hdr_s,1) udp_cdp_hdr_t;

// submessage/data
struct udp_cdp_submsg_s
{
	ushort	data_type;				// message data type (state of packet or packet data)
	ushort	size;					// message size including header
};

ALIGNED_TYPE(udp_cdp_submsg_s,2) udp_cdp_submsg_t;

// packet state message data
struct udp_cdp_packetstate_s
{
	short	message_id;
	short	status;
};

ALIGNED_TYPE(udp_cdp_packetstate_s,2) udp_cdp_packetstate_t;

void FreeMessage(cdp_message_t* pMessage)
{
	delete pMessage->bytestream;
	delete pMessage;
}

bool cdp_message_t::Write( void* pData, int nSize )
{
	ASSERT( bytestream->Tell() + nSize < UDP_CDP_MAX_MESSAGEPAYLOAD );

	return bytestream->Write( pData, 1, nSize ) > 0;
}

void cdp_message_t::WriteReset()
{
	delete bytestream;
	bytestream = NULL;
}

void cdp_message_t::ReadReset()
{
	bytestream->Seek(0, VS_SEEK_SET);
}

// allocates message
cdp_message_t* AllocMessage()
{
	cdp_message_t* pMessage = new cdp_message_t;

	memset(pMessage, 0, sizeof(cdp_message_t));

	if(!pMessage->bytestream)
	{
		pMessage->bytestream = new CMemoryStream();

		if( !pMessage->bytestream->Open( NULL, VS_OPEN_WRITE, UDP_CDP_MIN_MESSAGESIZE ))
		{
			FreeMessage(pMessage);

			return NULL;
		}
	}

	return pMessage;
}

int cdp_get_incomming_messages( SOCKET sock )
{
	struct timeval stTimeOut;

	fd_set stReadFDS;

	FD_ZERO(&stReadFDS);

    // Timeout of one second
#ifdef _WIN32
	stTimeOut.tv_sec = 0;
	stTimeOut.tv_usec = 20 * 1000;
#else
	stTimeOut.tv_sec = 0;
	stTimeOut.tv_usec = 0;
#endif // _WIN32
	FD_SET(sock, &stReadFDS);

#ifdef _WIN32
	return select( sock+1, &stReadFDS, NULL, NULL, &stTimeOut);
#else
    int cnt = select( sock+1, &stReadFDS, NULL, NULL, &stTimeOut);

    if (FD_ISSET(sock, &stReadFDS))
    {
        // Есть данные для чтения
        return cnt;
    }

    return -1;
#endif
}

CUDPSocket::CUDPSocket()
{
	m_sock = -1;

	memset(&m_addr, 0, sizeof(sockaddr_in));

	m_init = false;

	m_nMessageIDInc = 0;

	m_nRecvCallback = NULL;
}

CUDPSocket::~CUDPSocket()
{
	if(m_init)
	{
		closesocket( m_sock );

		m_sock = -1;
		m_init = false;

		memset(&m_addr, 0, sizeof(sockaddr_in));

		// cleanup lists
		for(int i = 0; i < m_pMessageQueue.numElem(); i++)
		{
			FreeMessage(m_pMessageQueue[i]);
		}

		m_pMessageQueue.clear();
	}
}

bool CUDPSocket::Init( int port )
{
	SOCKET sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	if (sock == INVALID_SOCKET)
	{
		MsgError("Could not create socket.\n");
		return false;
	}

	unsigned long _true = 1;
	int	i = 1;

#ifdef _WIN32

	// create non-blocking socket
	if(ioctlsocket (sock, FIONBIO, &_true) == -1)
	{
		closesocket(sock);

		Msg("cannot make socket non-locking!\n");
		return false;
	}
#else
    fcntl( sock, F_SETFL, O_NONBLOCK );
#endif

	sockaddr_in addr;

	memset((void *)&addr, 0, sizeof(sockaddr_in));

	// Set family and port
	addr.sin_family = AF_INET;
	addr.sin_port = htons( port );

	hostent* hostinfo;

	// Set address automatically
	// Get host name of this computer
	char host_name[256];
	gethostname(host_name, sizeof(host_name));
	hostinfo = gethostbyname(host_name);

	if (hostinfo == NULL)
	{
		closesocket( sock );

		Msg("Unable to get hostinfo!\n");
		return false;
	}

	in_addr hostaddress;

	hostaddress = *((in_addr *)( hostinfo->h_addr ));

#ifdef LINUX
	addr.sin_addr.s_addr = INADDR_ANY;
#else
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
#endif // LINUX

	// Bind address to socket
	if( bind(sock, (struct sockaddr *)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR )
	{
		closesocket(sock);

		int nErrorMsg = sock_errno;
		MsgError( "Failed to bind socket to port %d (%s)!!!\n", port, NETErrorString( nErrorMsg ) );

		return false;
	}

#ifdef LINUX
	uint8* host_ip = (uint8*)&hostaddress.s_addr;
	Msg("\nAddress: %u.%u.%u.%u:%d\n\n",	(unsigned char)host_ip[0],
											(unsigned char)host_ip[1],
											(unsigned char)host_ip[2],
											(unsigned char)host_ip[3],
											port);
#else
	Msg("\nAddress: %u.%u.%u.%u:%d\n\n",	(unsigned char)hostaddress.S_un.S_un_b.s_b1,
											(unsigned char)hostaddress.S_un.S_un_b.s_b2,
											(unsigned char)hostaddress.S_un.S_un_b.s_b3,
											(unsigned char)hostaddress.S_un.S_un_b.s_b4,
											port);
#endif // LINUX
	m_sock = sock;

	m_init = true;

	// setup defaults
	m_nSendTimeout					= CDP_SEND_TIMEOUT_MS;
	m_nUnconfirmedRemoveTimeout		= CDP_SEND_REMOVE_TIMEOUT_MS;
	m_nRecvTimeout					= CDP_RECV_TIMEOUT_MS;

	/*
	TKeyValues kvs;
	if( kvs.LoadFromFile( "cdp_config.cfg", SP_ROOT ) )
	{
		m_nSendTimeout = kint(kvs.GetRoot().FindKey("send_timeout"), CDP_SEND_TIMEOUT_MS);
		m_nUnconfirmedRemoveTimeout = kint(kvs.GetRoot().FindKey("sendremove_timeout"), CDP_SEND_REMOVE_TIMEOUT_MS);
		m_nRecvTimeout = kint(kvs.GetRoot().FindKey("recv_timeout"), CDP_RECV_TIMEOUT_MS);
	}
	*/

	//m_receivedIds.Resize( CDP_MAX_MESSAGE_ID );

	return true;
}

void CUDPSocket::SendMessageStatus(  const sockaddr_in* to, short message_id, bool isOk  )
{
	// alloc new message
	cdp_message_t* buffer = AllocMessage();
	buffer->addr = *to;

	udp_cdp_hdr_t hdr;
	hdr.ident = UDP_CDP_IDENT;
	hdr.protocol_version = UDP_CDP_PROTOCOL_VERSION;
	hdr.message_id = -1;
	hdr.flags = 0; // mark it as unguaranteed, but not necessary for sender, may be reciever
	//hdr.crc32 = 0;

	// write header before return
	buffer->Write( &hdr, sizeof( udp_cdp_hdr_t ) );

	// generate message subheader
	udp_cdp_submsg_t subhdr;
	subhdr.data_type = CDP_DATA_PACKETSTATE;
	subhdr.size = sizeof( udp_cdp_packetstate_t );

	// write header
	buffer->Write( &subhdr, sizeof( udp_cdp_submsg_t ));

	udp_cdp_packetstate_t statehdr;
	statehdr.message_id = message_id;
	statehdr.status = isOk ? 0x1 : 0x0;

	// write message size
	buffer->Write( &statehdr, sizeof( udp_cdp_packetstate_t ) );

	// status messages also with CRC
	//udp_cdp_hdr_t* pModHDR = (udp_cdp_hdr_t*)buffer->bytestream->GetBasePointer();
	//pModHDR->crc32 = buffer->bytestream->GetCRC32();

	// sent packet state now
	sendto( m_sock, (char*)buffer->bytestream->GetBasePointer(), buffer->bytestream->Tell(), 0, (sockaddr*)&buffer->addr, sizeof(sockaddr_in) );

	// free buffer since we just allocated it
	FreeMessage( buffer );
}

// sends message
int CUDPSocket::Send( char* data, int size, const sockaddr_in* to, short& msgId, short flags/* = 0 */)
{
	// don't write zero-length messages
	if(size <= 0)
	{
		msgId = CUDP_MESSAGE_ERROR;
		return -1;
	}

	cdp_message_t* buffer = NULL;

	bool continiousMessage = (flags & CDPSEND_GUARANTEED) > 0;

	if(continiousMessage)
	{
		buffer = GetFreeBuffer( size + sizeof(udp_cdp_submsg_t) + 16, to, flags );
	}
	else
	{
		buffer = AllocMessage();

		buffer->addr = *to;

		udp_cdp_hdr_t hdr;
		hdr.ident = UDP_CDP_IDENT;
		hdr.protocol_version = UDP_CDP_PROTOCOL_VERSION;
		hdr.message_id = GetMessageUniqueID();
		hdr.flags = 0; // mark it as unguaranteed, but not necessary for sender, may be reciever
		//hdr.crc32 = 0;

		// write header before return
		buffer->Write( &hdr, sizeof( udp_cdp_hdr_t ) );
	}

	if(!buffer)
	{
		msgId = CUDP_MESSAGE_ERROR;
		return -1;
	}

	if(continiousMessage)
		m_Mutex.Lock();

	buffer->addr = *to;

	// generate message subheader
	udp_cdp_submsg_t subhdr;
	subhdr.data_type = CDP_DATA_PACKETDATA;
	subhdr.size = size+sizeof(udp_cdp_submsg_t);

	// write subheader
	buffer->Write( &subhdr, sizeof(udp_cdp_submsg_t));

	// write message bytes
	buffer->Write( data, size );

	// if buffer now have filled for over 80 percent, we force to send message fast
	if( float(buffer->bytestream->Tell()) / float(UDP_CDP_MAX_MESSAGEPAYLOAD) >= UDP_CDP_FORCESEND_FILLPERCENTAGE || (flags & CDPSEND_IMMEDIATE))
	{
		buffer->sentTimeout.SetValue( m_nSendTimeout );
	}

	if( continiousMessage )
	{
		udp_cdp_hdr_t* pHdr = (udp_cdp_hdr_t*)buffer->bytestream->GetBasePointer();

		// set message id to return
		msgId = pHdr->message_id;

		m_Mutex.Unlock();
	}
	else
	{
		// FAKE LAG
		if(net_fakelag.GetInt() && RandomInt(0, net_fakelag.GetInt()) == 0)
		{
			FreeMessage( buffer );
			msgId = CUDP_MESSAGE_IMMEDIATE;
			return size;
		}

		// sent packet state now
		sendto( m_sock, (char*)buffer->bytestream->GetBasePointer(), buffer->bytestream->Tell(), 0, (sockaddr*)&buffer->addr, sizeof(sockaddr_in) );

		// free buffer since we just allocated it
		FreeMessage( buffer );

		msgId = CUDP_MESSAGE_IMMEDIATE;
	}

	/*
	// DEBUG
	char filename[256];

	udp_cdp_hdr_t* hdr = (udp_cdp_hdr_t*)buffer->bytestream->GetBasePointer();

	strcpy( filename, varargs("msg_debug/cdpprepsendmsg_%d", hdr->message_id) );

	FILE* pFile = fopen(filename, "wb");
	if(pFile)
	{
		fwrite(buffer->bytestream->GetBasePointer(), buffer->bytestream->Tell(), 1, pFile);
		fclose(pFile);
	}
	*/

	return size;
}

// receives message
int CUDPSocket::Recv( char* data, int size, sockaddr_in* from, short flags/* = 0 */)
{
	if(m_ReceivedMessageQueue.numElem() == 0)
		return 0;

	m_Mutex.Lock();

	// return first message from receive buffer
	int copy_size = m_ReceivedMessageQueue[0].data->bytestream->Tell();

	// safety first
	if(copy_size > size)
		copy_size = size;

	memcpy( data, m_ReceivedMessageQueue[0].data->bytestream->GetBasePointer(), copy_size );

	// copy address if we want
	if(from)
		*from = m_ReceivedMessageQueue[0].data->addr;

	/*
	char filename[256];

	strcpy( filename, varargs("msg_debug/cdp_recv_msg_%d", m_ReceivedMessageQueue[i].message_id) );

	FILE* pFile = fopen(filename, "wb");
	if(pFile)
	{
		fwrite(data, copy_size, 1, pFile);
		fclose(pFile);
	}
	*/


	FreeMessage( m_ReceivedMessageQueue[0].data );
	m_ReceivedMessageQueue.fastRemoveIndex( 0 );

	m_Mutex.Unlock();

	return copy_size;
}

void CUDPSocket::PutMessageOrdered( const cdp_receive_info_t& info )
{
	for(int i = 0; i < m_ReceivedMessageQueue.numElem(); i++)
	{
		if(m_ReceivedMessageQueue[i].message_id > info.message_id)
			continue;

		m_ReceivedMessageQueue.insert( info, i+1 );
		return;
	}

	m_ReceivedMessageQueue.append( info );
}

// this really updates socket
void CUDPSocket::UpdateRecv( int timeMs )
{
	if(!m_init)
		return;

	// get incomming messages from socket
	int count = cdp_get_incomming_messages( m_sock );

	for( int i = 0; i < count; i++ )
	{
		sockaddr_in fromaddr;
		socklen_t	fromlen = sizeof(sockaddr_in);

		// zero it
		static char message_buffer[UDP_CDP_MAX_MESSAGEPAYLOAD] = {0};

		// receive messages
		int r_size = recvfrom( m_sock, message_buffer, UDP_CDP_MAX_MESSAGEPAYLOAD, 0, (sockaddr*)&fromaddr, &fromlen );

		if( r_size < 0 )
		{
			int nErrorMsg = sock_errno;
#ifdef _WIN32
			// skip WSAEWOULDBLOCK, WSAENOTCONN and WSAECONNRESET to avoid receiving ICMP error
			if( nErrorMsg != WSAEWOULDBLOCK && nErrorMsg != WSAENOTCONN && nErrorMsg != WSAECONNRESET )
			{
				Msg("receive error (%s)\n", NETErrorString(nErrorMsg));
				continue;
			}
#else
			// skip WSAEWOULDBLOCK, WSAENOTCONN and WSAECONNRESET to avoid receiving ICMP error
			if( nErrorMsg != EWOULDBLOCK && nErrorMsg != ENOTCONN && nErrorMsg != ECONNRESET && nErrorMsg != EAGAIN )
			{
				Msg("receive error (%s)\n", NETErrorString(nErrorMsg));
				continue;
			}
#endif // _WIN32
		}

		// FAKE LAG - do not accept recieved message
		if(net_fakelag.GetInt() && RandomInt(0, net_fakelag.GetInt()) == 0)
			continue;

		// don't receive zero packets
		if(r_size <= 0)
			continue;

		// don't receive from itself !!!
		if( NETCompareAdr( fromaddr, m_addr ) )
			continue;

		// get message header
		udp_cdp_hdr_t* hdr = (udp_cdp_hdr_t*)message_buffer;

		if( hdr->ident != UDP_CDP_IDENT)
			continue; // unk kind of message

		if( hdr->protocol_version != UDP_CDP_PROTOCOL_VERSION )
			continue; // wrong version

		// skip this message if it was already received
		if( HasAnyMessageWithId( hdr->message_id, fromaddr ) )
		{
			// send status
			if(hdr->flags & CDPSEND_GUARANTEED)
			{
                SendMessageStatus( &fromaddr, hdr->message_id, true );
			}


			continue;
		}

		/*
		// now message recieved, check it's CRC
		unsigned long crc32_check = hdr->crc32;
		hdr->crc32 = 0;

		hdr->crc32 = CRC32_BlockChecksum( message_buffer, r_size );

		if( hdr->crc32 != crc32_check )
		{
			SendMessageStatus( &fromaddr, hdr->message_id, false );

			// discard, crc error
			continue;
		}
		*/

		if( hdr->message_id >= (CDP_MAX_MESSAGE_ID - 1) )
			m_receivedIds.clear( false );	// SetNum because needs less reallocations

		// add to received IDs
		if(hdr->message_id != -1)
		{
			cdp_receive_msgid_t		mid;

			mid.addr	= fromaddr;
			mid.msgid	= hdr->message_id;
			mid.timeMs	= 0;				// sometime i've forgotten to zero it.

			Threading::CScopedMutex m( m_Mutex );

			m_receivedIds.append( mid );
		}

		int message_offset = sizeof(udp_cdp_hdr_t);
		int msg_idx = 0;

		// cyclic reading
		while( message_offset < r_size )
		{
			// get message header
			udp_cdp_submsg_t* subhdr = (udp_cdp_submsg_t*)(message_buffer + message_offset);

			// prevent zero messages
			if( subhdr->size <= 0 )
				break;

			message_offset += subhdr->size;

			// then add them to received_messages buffer list
			// or check for status message

			// is a packet
			if( subhdr->data_type == CDP_DATA_PACKETDATA )
			{
				//Threading::CScopedMutex m( m_Mutex );

				// if message was guaranteed
				if(hdr->flags & CDPSEND_GUARANTEED)
					SendMessageStatus( &fromaddr, hdr->message_id, true );

				// get message size
				int msg_size = subhdr->size-sizeof(udp_cdp_submsg_t);
				char* rbuf = ((char*)subhdr) + sizeof(udp_cdp_submsg_t);

				cdp_message_t* message = AllocMessage();

				message->Write( rbuf, msg_size );
				message->addr = fromaddr;

				cdp_receive_info_t info;
				info.message_id = hdr->message_id;
				info.data = message;
				info.flags = hdr->flags;

				bool shouldAdd = true;

				// make call.
				if( m_nRecvCallback != NULL )
					shouldAdd = (m_nRecvCallback)( info );

				if(shouldAdd)
				{
                    m_Mutex.Lock();

					// put ordered or in recieve order
					if( hdr->flags & CDPSEND_PRESERVE_ORDER )
						PutMessageOrdered( info );
					else
						m_ReceivedMessageQueue.append( info );

                    m_Mutex.Unlock();
				}
				else
					FreeMessage(message);


				/*
				// DEBUG

				char filename[256];

				strcpy( filename, varargs("msg_debug/cdpmsg_%d_%d", hdr->message_id, msg_idx) );

				FILE* pFile = fopen(filename, "wb");
				if(pFile)
				{
					fwrite(rbuf, msg_size, 1, pFile);
					fclose(pFile);
				}
				*/
			}
			else if( subhdr->data_type == CDP_DATA_PACKETSTATE )
			{
				char* rbuf = ((char*)subhdr) + sizeof(udp_cdp_submsg_t);

				// get a state and message index, then check remove message from send list
				udp_cdp_packetstate_t* statehdr = (udp_cdp_packetstate_t*)rbuf;

				// got state
				if( statehdr->status == 0x1 )
				{
					// add status to list
					msg_status_t msgStat;
					msgStat.message_id = statehdr->message_id;
					msgStat.status = DELIVERY_SUCCESS;

					Lock();
					m_messageSendStatus.append( msgStat );
					Unlock();

					// finally remove
					RemoveMessageFromSendPool( statehdr->message_id );
				}
				else
					CheckMessageForResend( statehdr->message_id );
			}

			msg_idx++;
		}
	}

	for(int i = 0; i < m_receivedIds.numElem(); i++)
	{
		m_receivedIds[i].timeMs += timeMs;

		if( m_receivedIds[i].timeMs > m_nRecvTimeout )
		{
			// remove it
			m_receivedIds.fastRemoveIndex(i);
			i--;
		}
	}
}

void CUDPSocket::UpdateSend( int timeMs )
{
	if(!m_init)
		return;

	for(int i = 0; i < m_pMessageQueue.numElem(); i++)
	{
		cdp_message_t* buffer = m_pMessageQueue[i];

		// after 5 retries of sending that should be removed
		// TODO: tweak
		if( buffer->sendTimes.GetValue() > 5 )
		{
			buffer->removeTimeout.Add(timeMs);

			// remove if timed out
			if( buffer->removeTimeout.GetValue() >= m_nUnconfirmedRemoveTimeout )
			{
				// add pending message ids
				udp_cdp_hdr_t* pMsgHdr = (udp_cdp_hdr_t*)buffer->bytestream->GetBasePointer();

				msg_status_t msgStat;
				msgStat.message_id = pMsgHdr->message_id;
				msgStat.status = DELIVERY_FAILED;

				m_Mutex.Lock();

				m_messageSendStatus.append( msgStat );

				// remove
				FreeMessage( buffer );

				m_pMessageQueue.fastRemoveIndex( i );
				i--;

				if(m_pMessageQueue.numElem() < UDP_CDP_MAX_QUEUE_BUFFERS)
					m_SendSignal.Raise();

				m_Mutex.Unlock();

				continue;
			}
		}

		buffer->sentTimeout.Add(timeMs);

		int nSendTimeoutQueueMod = m_nSendTimeout;

		if(buffer->sentTimeout.GetValue() >= nSendTimeoutQueueMod)
		{
			buffer->sendTimes.Increment();

			// set the send time
			buffer->sendTime = m_time;

			/*
			// set crc
			udp_cdp_hdr_t* pData = (udp_cdp_hdr_t*)buffer->bytestream->GetBasePointer();
			pData->crc32 = 0;
			pData->crc32 = buffer->bytestream->GetCRC32(); // before CRC32 being computed it should be always zero
			*/

			bool bSend = true;

			// FAKE LAG
			if(net_fakelag.GetInt() && RandomInt(0, net_fakelag.GetInt()) == 0)
				bSend = false;

			if(bSend)
			{
				int s_size = sendto( m_sock, (char*)buffer->bytestream->GetBasePointer(), buffer->bytestream->Tell(), 0, (sockaddr*)&buffer->addr, sizeof(sockaddr_in) );

                udp_cdp_hdr_t* hdr = (udp_cdp_hdr_t*)buffer->bytestream->GetBasePointer();

				if( s_size < 0 )
				{
					int nErrorMsg = sock_errno;

					// skip WSAEWOULDBLOCK and WSAENOTCONN to avoid freeze
#ifdef _WIN32
					if( nErrorMsg != WSAEWOULDBLOCK && nErrorMsg != WSAENOTCONN )
#else
					if( nErrorMsg != EWOULDBLOCK && nErrorMsg != ENOTCONN )
#endif // _WIN32
					{
						continue;
					}
				}
			}


			// if this message is unguaranteed, remove now
			if( !(buffer->flags & CDPSEND_GUARANTEED) )
			{
				Threading::CScopedMutex m( m_Mutex );

				FreeMessage( buffer );

				m_pMessageQueue.fastRemoveIndex( i );
				i--;

				if(m_pMessageQueue.numElem() < UDP_CDP_MAX_QUEUE_BUFFERS)
				{
					m_SendSignal.Raise();
				}

				continue;
			}

			buffer->sentTimeout.SetValue(0);

			// DEBUG
			/*
			char filename[256];

			udp_cdp_hdr_t* hdr = (udp_cdp_hdr_t*)buffer->data;

			strcpy( filename, varargs("msg_debug/cdpsendmsg_%d", hdr->message_id) );

			FILE* pFile = fopen(filename, "wb");
			if(pFile)
			{
				fwrite(buffer->data, buffer->size, 1, pFile);
				fclose(pFile);
			}
			*/
		}
	}

	// that's all
	if(m_time > UINT_MAX - timeMs)
		m_time = 0;

	// increment the time
	m_time += timeMs;
}

bool CUDPSocket::HasAnyMessageWithId( short message_id, const sockaddr_in& addr  )
{
	CScopedMutex m(m_Mutex);

	for(int i = 0; i < m_receivedIds.numElem(); i++)
	{
		if((m_receivedIds[i].msgid == message_id) && NETCompareAdr(m_receivedIds[i].addr, addr))
			return true;
	}

	return false;
}

// returns a free message (it could create new buffer or return one freed)
cdp_message_t* CUDPSocket::GetFreeBuffer( int freeSpaceRequired, const sockaddr_in* to, short nFlags )
{
	ASSERT(freeSpaceRequired < UDP_CDP_MAX_MESSAGEPAYLOAD);

	{
		Threading::CScopedMutex m(m_Mutex);

		// find and return existing buffer
		for( int i = 0; i < m_pMessageQueue.numElem(); i++ )
		{
			if(!m_pMessageQueue[i]->bytestream)
			{
				FreeMessage(m_pMessageQueue[i]);
				m_pMessageQueue.fastRemoveIndex(i);
				i--;
				continue;
			}

			int nCurPos = m_pMessageQueue[i]->bytestream->Tell();

			int nFreeSpace = ( UDP_CDP_MAX_MESSAGEPAYLOAD - m_pMessageQueue[i]->bytestream->Tell() );

			if( nCurPos < UDP_CDP_MIN_SEND_BUFFER &&						// check for reaching minimal send size (THIS IS UGLY)
				nFreeSpace > freeSpaceRequired &&							// check for overflow
				(m_pMessageQueue[i]->sendTimes.GetValue() == 0) &&			// don't use already sent buffers
				(m_pMessageQueue[i]->flags == nFlags) &&					// check it's usage
				NETCompareAdr( m_pMessageQueue[i]->addr, *to ) )			// and only deliver to needed address
			{
				return m_pMessageQueue[i];
			}

		}
	}

	// otherwise, allocate new

	// if we ran out of buffers, force all to send immediately
	// and wait thread
	if( m_pMessageQueue.numElem() >= UDP_CDP_MAX_QUEUE_BUFFERS )
	{
		Msg("GetFreeBuffer: exceeded UDP_CDP_MAX_QUEUE_BUFFERS (%d > %d)\n", m_pMessageQueue.numElem(), UDP_CDP_MAX_QUEUE_BUFFERS);

		m_Mutex.Lock();

		for(int i = 0; i < m_pMessageQueue.numElem(); i++)
		{
			m_pMessageQueue[i]->sentTimeout.SetValue(m_nSendTimeout);
		}

		m_Mutex.Unlock();

		m_SendSignal.Wait( m_nSendTimeout*m_pMessageQueue.numElem() );

		m_SendSignal.Clear();
	}

	cdp_message_t* buffer = AllocMessage();

	buffer->flags = nFlags;

	udp_cdp_hdr_t hdr;
	hdr.ident = UDP_CDP_IDENT;
	hdr.protocol_version = UDP_CDP_PROTOCOL_VERSION;
	hdr.message_id = GetMessageUniqueID();
	hdr.flags = nFlags;
	//hdr.crc32 = 0;

	// write header before return
	buffer->Write(&hdr, sizeof( udp_cdp_hdr_t ));

	m_Mutex.Lock();
	m_pMessageQueue.append(buffer);
	m_Mutex.Unlock();

	return buffer;
}

void CUDPSocket::RemoveMessageFromSendPool( short message_id )
{
	for( int i = 0; i < m_pMessageQueue.numElem(); i++ )
	{
		cdp_message_t* pMsg = m_pMessageQueue[i];

		// this could not be happened, but i'll keep it there
		//if( pMsg->sendTimes == 0 )
		//	continue;

		// get message header
		udp_cdp_hdr_t* hdr = (udp_cdp_hdr_t*)m_pMessageQueue[i]->bytestream->GetBasePointer();

		if( hdr->message_id == message_id )
		{
			Threading::CScopedMutex m( m_Mutex );

			FreeMessage( m_pMessageQueue[i] );
			m_pMessageQueue.fastRemoveIndex( i );
			i--;
		}
	}
}

void CUDPSocket::CheckMessageForResend( short message_id )
{
	for( int i = 0; i < m_pMessageQueue.numElem(); i++ )
	{
		cdp_message_t* pMsg = m_pMessageQueue[i];

		// this could not be happened, but i'll keep it there
		if( pMsg->sendTimes.GetValue() == 0 )
			continue;

		// get message header
		udp_cdp_hdr_t* hdr = (udp_cdp_hdr_t*)m_pMessageQueue[i]->bytestream->GetBasePointer();

		if( hdr->message_id == message_id )
		{
			Threading::CScopedMutex m( m_Mutex );

			// repeat
			m_pMessageQueue[i]->sendTimes.SetValue(0);
			m_pMessageQueue[i]->sentTimeout.SetValue(0);

			return;
		}
	}
}

int CUDPSocket::GetMessageUniqueID()
{
	// reset
	if( m_nMessageIDInc >= CDP_MAX_MESSAGE_ID )
		m_nMessageIDInc = 0;

	return m_nMessageIDInc++;
}

// select function for receiving messages
int	CUDPSocket::GetReceivedMessageCount()
{
	return m_ReceivedMessageQueue.numElem();
}

int CUDPSocket::GetSendPoolCount()
{
	return m_pMessageQueue.numElem();
}

void CUDPSocket::GetMessageStatusList( DkList<msg_status_t>& msgStatusList )
{
	Threading::CScopedMutex m( m_Mutex );

	msgStatusList.append( m_messageSendStatus );
	m_messageSendStatus.clear();
}

void CUDPSocket::SetRecvCallback(const CDPRecvCallback_fn cb)
{
	m_nRecvCallback = cb;
}

// sets the delay times of protocol
void CUDPSocket::SetDelayTimes( int nSendBufferTimeout, int nUnconfirmedRemoveTimeout, int nReceivedIdsTimeout )
{
	m_nSendTimeout = nSendBufferTimeout;
	m_nUnconfirmedRemoveTimeout = nUnconfirmedRemoveTimeout;
	m_nRecvTimeout = nReceivedIdsTimeout;
}

void CUDPSocket::Lock()
{
	m_Mutex.Lock();
}

void CUDPSocket::Unlock()
{
	m_Mutex.Unlock();
}

void CUDPSocket::PrintStats()
{

	Msg("m_pMessageQueue = %d\n", m_pMessageQueue.numElem());
	Msg("m_ReceivedMessageQueue = %d\n", m_ReceivedMessageQueue.numElem());
	Msg("m_receivedIds = %d\n", m_receivedIds.numElem());
	Msg("m_messageSendStatus = %d\n", m_messageSendStatus.numElem());

}

}; // namespace CUDP
