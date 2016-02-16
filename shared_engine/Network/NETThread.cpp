//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine network thread
//////////////////////////////////////////////////////////////////////////////////

#include "NETThread.h"
#include "eqGlobalMutex.h"
#include "ConVar.h"
#include "utils/strtools.h"

ConVar net_fakelatency("net_fakelatency", "0", "Simulate latency (value is in ms). Operating on recieved only\n", CV_CHEAT);
ConVar net_fakelatency_randthresh("net_fakelatency_randthresh", "1.0f", "Fake latency randomization threshold\n", CV_CHEAT);

using namespace Threading;

extern OOLUA::Script& GetLuaState();

namespace Networking
{

#define NET_MAX_SIZE

enum MessageFlags_e
{
	MSGFLAG_PROCESSED = (1 << 0),		// processed and can be removed
	MSGFLAG_FRAGMENTED = (1 << 1),		// fragmented
	MSGFLAG_TESTLAGGING = (1 << 2),		// lag test
};

enum EMsgDataType
{
	MSGTYPE_EVENT		= (1 << 0),
	MSGTYPE_DATA		= (1 << 1),	// is a ping message
};

//----------------------------------
// received message linked list
//----------------------------------
struct rcvdmessage_t
{
	rcvdmessage_t()
	{
		pNext = NULL;
		pPrev = NULL;
		recvTime = 0.0;
		clientid = -1;
		messageid = -1;
		flags = 0;
		recvTime = 0.0;
	}

	short				clientid;
	short				messageid;

	sockaddr_in			addr;		// paired with client id now, unnecessary to check

	int					flags;

	CNetMessageBuffer*	pMessage;

	rcvdmessage_t*		pNext;
	rcvdmessage_t*		pPrev;

	double				recvTime;
};

struct netmsg_fragment_t
{
	netmessage_t* pMsg;
	int	size;
};

struct netfragmsg_t
{
	short			client_id;
	sockaddr_in		addr;		// paired with client id now, unnecessary to check

	short			message_id;
	uint8			nfragments;

	DkList< netmsg_fragment_t > frags;
};

CNetworkThread::CNetworkThread( INetworkInterface* pInterface ) : Threading::CEqThread(), m_Mutex(GetGlobalMutex(MUTEXPURPOSE_NET_THREAD))
{
	m_pFirstMessage = NULL;
	m_pLastMessage = NULL;

	m_pNetInterface = pInterface;

	m_fCurTime = 0.0;
	m_fPrevTime = 0.0;

	m_nLastClientID = 0;
	m_bStopWork = false;

	m_fnCycleCallback = NULL;
	m_fnEventFilter = NULL;

	m_nEventCounter = 0;

	m_bLockUpdateDispatch = false;
}

CNetworkThread::~CNetworkThread()
{
	StopWork();
}

CNetEvent* LUANetEventCallbackFactory(CNetworkThread* thread, CNetMessageBuffer* msg);

#ifdef NO_LUA
CNetEvent*	LUANetEventCallbackFactoryEmpty(CNetworkThread* thread, CNetMessageBuffer* msg)
{
	int luaEventId = msg->ReadInt();

	MsgError("LUANetEventCallbackFactoryEmpty: attempt to read Lua event '%d',\nbut in this version Lua is unsupported\n");

	return NULL;
}
#endif // NO_LUA

void CNetworkThread::Init()
{
#ifndef NO_LUA
	// register event class
	RegisterEvent(LUANetEventCallbackFactory, NETTHREAD_EVENTS_LUA_START);
#else
	//RegisterEvent(LUANetEventCallbackFactoryEmpty, NETTHREAD_EVENTS_LUA_START);
#endif // NO_LUA
}

INetworkInterface* CNetworkThread::GetNetworkInterface()
{
	return m_pNetInterface;
}

void CNetworkThread::SetNetworkInterface(INetworkInterface* pInterface)
{
	m_pNetInterface = pInterface;
}

void CNetworkThread::StopWork()
{
	if(IsTerminating() || !IsRunning())
		return;

	m_Mutex.Lock();
	m_bStopWork = true;
	m_Mutex.Unlock();

	// move all pooled events to send buffer
	DispatchEvents();

	// wait and stop
	StopThread();
}

int CNetworkThread::Run()
{
	if(!m_pNetInterface)
		return -1;

	m_fCurTime = m_Timer.GetTime();
	m_fPrevTime = m_Timer.GetTime();

	// completely stop work after all messages are sent
	bool bDoStopWork = false;

	while(!bDoStopWork)
	{
		bDoStopWork = m_bStopWork && (m_pNetInterface->GetSocket()->GetSendPoolCount() == 0);

		// wait for other operations

		m_fCurTime = m_Timer.GetTime();

		// do custom cycles
		OnCycle();

		// TEST:	checking for lag packets
		//			and add them
		for(int i = 0; i < m_lagMessages.numElem(); i++)
		{
			if(m_fCurTime >= m_lagMessages[i]->recvTime )
			{
				AddMessage(m_lagMessages[i], m_pLastMessage);

				m_lagMessages.removeIndex(i);
				i--;
			}
		}

		double timeSinceLastUpdate = m_fCurTime - m_fPrevTime;

		// call our cycle callback
		if(m_fnCycleCallback)
			(m_fnCycleCallback)(this, timeSinceLastUpdate);

		m_pNetInterface->Update( (int)(timeSinceLastUpdate * 1000.0) );

		int ready = m_pNetInterface->GetIncommingMessages();

		if( ready < 0 )
			Msg("error %d\n", ready);

		if( ready )
		{
			// UDP subfragment method
			for(int i = 0; i < ready; i++)
			{
				netmessage_t* pRcvdMsg = new netmessage_t;
				int msg_size = 0;

				sockaddr_in from;

				if( m_pNetInterface->Receive( pRcvdMsg, msg_size, &from ) )
				{
					if( pRcvdMsg->nfragments > 1 )
					{
						// add fragment to waiter
						// this function also will try to dispatch message that got fully received
						m_Mutex.Lock();
						AddFragmentedMessage( pRcvdMsg, msg_size, from );
						m_Mutex.Unlock();
					}
					else
					{
						// add as usual
						rcvdmessage_t* pMsg = new rcvdmessage_t;

						pMsg->pMessage = new CNetMessageBuffer( m_pNetInterface );
						pMsg->pMessage->SetClientInfo( from, pRcvdMsg->clientid );
						pMsg->addr = from;
						pMsg->messageid = pRcvdMsg->messageid;

						// write contents
						pMsg->pMessage->WriteData( pRcvdMsg->message_bytes, pRcvdMsg->message_size);

						pMsg->flags = 0;

						pMsg->pNext = NULL;
						pMsg->pPrev = NULL;
						pMsg->clientid = pRcvdMsg->clientid;

						// queue message
						m_Mutex.Lock();
						AddMessage(pMsg, m_pLastMessage);
						m_Mutex.Unlock();

						// delete netmessage
						delete pRcvdMsg;
					}
				}
				else
				{
					delete pRcvdMsg;
				}
			}
		}

		m_fPrevTime = m_fCurTime;

		// this system is lagging very hard if you use 1
		Platform_Sleep( 1 );
	}

	// reset
	m_bStopWork = false;

	return 0;
}


void CNetworkThread::FreeMessage(rcvdmessage_t* pMsg)
{
	if(pMsg->pPrev)
		pMsg->pPrev->pNext = pMsg->pNext;
	else
		m_pFirstMessage = pMsg->pNext;

	if(pMsg->pNext)
		pMsg->pNext->pPrev = pMsg->pPrev;
	else
		m_pLastMessage = pMsg->pPrev;

	delete pMsg->pMessage;
	delete pMsg;
}

int g_msg = 0;

void CNetworkThread::AddMessage(rcvdmessage_t* pMsg, rcvdmessage_t* pAddTo)
{
	pMsg->recvTime = m_fCurTime;

	// LAG PACKETS
	if( !(pMsg->flags & MSGFLAG_TESTLAGGING) && net_fakelatency.GetFloat() > 0 )
	{
		// put messages into temporary buffer

		pMsg->flags |= MSGFLAG_TESTLAGGING;

		pMsg->recvTime += RandomFloat(net_fakelatency.GetFloat() * net_fakelatency_randthresh.GetFloat(), net_fakelatency.GetFloat()) * 0.001f;

		m_lagMessages.append( pMsg );

		return;
	}

	pMsg->pMessage->ResetPos();

	// filter event receiving, at 'add' level
	int8 type = pMsg->pMessage->ReadByte();

	if( type == MSGTYPE_EVENT )
	{
		int nEventType = pMsg->pMessage->ReadInt();

		// filter events
		if( m_fnEventFilter )
		{
			// assume the eventfilter can decode our messages
			int result = (m_fnEventFilter)( this, pMsg->pMessage, nEventType );

			if( result == EVENTFILTER_ERROR_NOTALLOWED )
			{
				delete pMsg;
				return;
			}
			else if( result != EVENTFILTER_OK )
			{
				delete pMsg;
				return;
			}
		}
	}

	// reset
	pMsg->pMessage->ResetPos();

	if(!m_pFirstMessage || !pAddTo)
	{
		m_pFirstMessage = pMsg;
		m_pLastMessage = pMsg;
		return;
	}

	pMsg->pPrev = pAddTo;
	pMsg->pNext = pAddTo->pNext;

	if(!pAddTo->pNext)
		m_pLastMessage = pMsg;
	else
		pAddTo->pNext->pPrev = pMsg;

	pAddTo->pNext = pMsg;
}

// adds fragmented message to waiter
void CNetworkThread::AddFragmentedMessage( netmessage_t* pMsg, int size, const sockaddr_in& addr )
{
	netfragmsg_t* pFragMsgs = NULL;
	int idx = -1;

	// find waiting message
	for(int i = 0; i < m_fragmented_messages.numElem(); i++)
	{
		netfragmsg_t* pFragMsg = m_fragmented_messages[i];

		if(	pFragMsg->client_id == pMsg->clientid &&
			pFragMsg->message_id == pMsg->messageid &&
			pFragMsg->nfragments == pMsg->nfragments &&
			pFragMsg->frags.numElem() < pMsg->nfragments)
		{
			pFragMsgs = pFragMsg;
			idx = i;
			break;
		}
	}

	// create if not found
	if(!pFragMsgs)
	{
		pFragMsgs = new netfragmsg_t;
		pFragMsgs->client_id = pMsg->clientid;
		pFragMsgs->addr = addr;
		pFragMsgs->message_id = pMsg->messageid;
		pFragMsgs->nfragments = pMsg->nfragments;

		idx = m_fragmented_messages.append(pFragMsgs);
	}

	netmsg_fragment_t frag;

	frag.pMsg = pMsg;
	frag.size = size;

	// add message
	pFragMsgs->frags.append( frag );

	if( pFragMsgs->frags.numElem() == pFragMsgs->nfragments )
	{
		DispatchFragmentedMessage( pFragMsgs );

		// delete this fragment collection
		delete pFragMsgs;
		m_fragmented_messages.fastRemoveIndex( idx );
	}
}

int compare_msg_frags( const netmsg_fragment_t &a, const netmsg_fragment_t &b)
{
	return a.pMsg->fragmentid - b.pMsg->fragmentid;
}

// dispatches fragmented message to the queue
void CNetworkThread::DispatchFragmentedMessage( netfragmsg_t* pMsg )
{
	// sort messages by fragment id and join em all to the single message
	// then we have to add it to the main processing queue
	pMsg->frags.sort( compare_msg_frags );

	// copy message
	rcvdmessage_t* pRcvdMsg = new rcvdmessage_t;

	CNetMessageBuffer* pMessage = new CNetMessageBuffer( m_pNetInterface );
	pMessage->SetClientInfo(pMsg->addr, pMsg->client_id);
	pRcvdMsg->pMessage = pMessage;

	// write fragments to received message and reset pointers
	for(int i = 0; i < pMsg->frags.numElem(); i++)
	{
		pMessage->WriteData( pMsg->frags[i].pMsg->message_bytes, pMsg->frags[i].size - NETMESSAGE_HDR);

		delete pMsg->frags[i].pMsg;
	}

	pRcvdMsg->flags = MSGFLAG_FRAGMENTED;

	pRcvdMsg->pNext = NULL;
	pRcvdMsg->pPrev = NULL;
	pRcvdMsg->clientid = pMsg->client_id;
	pRcvdMsg->messageid = pMsg->message_id;
	pRcvdMsg->addr = pMsg->addr;

	// queue message
	AddMessage( pRcvdMsg, m_pLastMessage );
}

void CNetworkThread::FreeMessages()
{
	m_Mutex.Lock();

	rcvdmessage_t* pMsg = m_pFirstMessage;

	while( pMsg )
	{
		rcvdmessage_t* toRem = pMsg;

		pMsg = pMsg->pNext;

		delete toRem->pMessage;
		delete toRem;
	}

	m_pFirstMessage = NULL;
	m_pLastMessage = NULL;

	m_Mutex.Unlock();
}

void DestroySendEvent(const sendevent_t& evt)
{
	delete evt.pEvent;
	delete evt.pStream;
}

// generates events from recieved messages
int CNetworkThread::UpdateDispatchEvents()
{
	if( !m_pNetInterface )
		return 0;

	// wait for signal (this really shouldn't happen)
	//while(m_bLockUpdateDispatch){ Platform_Sleep( 1 ); }

	rcvdmessage_t* pMsg = m_pFirstMessage;

	while(pMsg)
	{
		rcvdmessage_t* procMsg = pMsg;

		ProcessMessage( procMsg );

		pMsg = pMsg->pNext;

		m_Mutex.Lock();

		//if(procMsg->flags & MSGFLAG_PROCESSED)
		FreeMessage( procMsg );

		m_Mutex.Unlock();
	}

	// dispatch events
	return DispatchEvents();
}

int CNetworkThread::DispatchEvents()
{
	CUDPSocket* pSock = m_pNetInterface->GetSocket();

	DkList<msg_status_t> msgStates;
	pSock->GetMessageStatusList( msgStates );

	int numPendingMessages = 0;

	// Try to send our events/datas
	for(int i = 0; i < m_pSentEvents.numElem(); i++)
	{
		EEventMessageStatus status = DispatchEvent(i, msgStates);

		// if it's no longer pending and sent or have error
		if( status != EVTMSGSTATUS_PENDING )
		{
			DestroySendEvent( m_pSentEvents[i] );
			m_pSentEvents.removeIndex(i);
			i--;
		}
		else
			numPendingMessages++;
	}

	return numPendingMessages;
}

EEventMessageStatus CNetworkThread::DispatchEvent( int nIndex, DkList<msg_status_t>& msgStates )
{
	sendevent_t& eventdata = m_pSentEvents[nIndex];

	if(eventdata.flags & NSFLAG_IMMEDIATE)
	{
		// if interface fails, run down immediately!
		if( !eventdata.pStream->Send( eventdata.message_id, eventdata.flags) )
			return EVTMSGSTATUS_ERROR;

		return EVTMSGSTATUS_OK;
	}

	// send non-immediate messages

	int msgId = eventdata.message_id;

	if( msgId >= 0 )
	{
		msg_status_t state = {-1, DELIVERY_INVALID};

		// search for this event/data
		for(int j = 0; j < msgStates.numElem(); j++)
		{
			if(msgStates[j].message_id == msgId)
			{
				state = msgStates[j];
				break;
			}
		}

		// not invalid?
		if( state.status != DELIVERY_INVALID )
		{
			if(state.status != DELIVERY_SUCCESS)
			{
				// if there is no event, or this is a event and it handles removal
				if( (!eventdata.pEvent) ||
					(eventdata.pEvent && !eventdata.pEvent->OnDeliveryFailed()) )
				{
					return EVTMSGSTATUS_ERROR;
				}
				else
				{
					m_pSentEvents[nIndex].message_id = -1; // reset message
				}
			}
			else
			{
				return EVTMSGSTATUS_OK;
			}
		}
	}
	else
	{
		// send and assign message id
		if(msgId != CUDP_MESSAGE_ERROR)
		{
			// if interface fails, run down immediately!
			if( !eventdata.pStream->Send( eventdata.message_id, eventdata.flags) )
			{
				MsgError("DispatchEvent error: can't send eventtype=%d to client=%d\n", m_pSentEvents[nIndex].event_type, m_pSentEvents[nIndex].client_id);
				return EVTMSGSTATUS_ERROR;
			}

			if(	eventdata.message_id == CUDP_MESSAGE_IMMEDIATE )
				return EVTMSGSTATUS_OK;
		}

		/*
		if(msgId  -1)
		{
			return EVTMSGSTATUS_ERROR;
		}*/
	}

	return EVTMSGSTATUS_PENDING;
}

bool CNetworkThread::ProcessMessage( rcvdmessage_t* pMsg, CNetMessageBuffer* pOutput, int nWaitEventID)
{
	// already processed message? (NEVER HAPPENS)
	if( pMsg->flags & MSGFLAG_PROCESSED )
		return true;

	CNetMessageBuffer* pMessage = pMsg->pMessage;

	// always do it
	pMessage->ResetPos();

	int8 type = pMessage->ReadByte();

	int8 check_type = pOutput ? MSGTYPE_DATA : MSGTYPE_EVENT;

	// skip
	if(type != check_type)
	{
		if(type == MSGTYPE_DATA && nWaitEventID == -1 && !pOutput)
		{
			Msg("Got dull MSGTYPE_DATA while pOutput=NULL!!!\n");
		}

		// MSGTYPE_DATA ����� ����� ����������, ���� ����� �� ������� ���
		return (type == MSGTYPE_DATA);
	}

	if( type == MSGTYPE_EVENT )
	{
		int nEventType = pMsg->pMessage->ReadUInt16();
		int nEventID = pMsg->pMessage->ReadUInt16();

		// dispatch event
		CNetEvent* pEvent = CreateEvent( pMsg->pMessage, nEventType );

		if( !pEvent )
		{
			MsgError("Invalid event id='%d' from client %d (thread: %s)\n", nEventType, pMessage->GetClientID(), GetName());
			pMsg->flags |= MSGFLAG_PROCESSED;

			return true;
		}

		// ��������� ���������� �������
		pMessage->SetClientInfo( pMsg->addr, pMsg->clientid );

		// ����������� ������������� ������������
		pEvent->SetID(nEventID);

		// ������ ������ ���������
		pEvent->Unpack( this, pMessage );

		m_nLastClientID = pMessage->GetClientID();

		// now process it
		pEvent->Process( this );

		delete pEvent;
	}
	else
	{
		int nMsgEventID = pMsg->pMessage->ReadUInt16();
		int nMsgDataSize = pMsg->pMessage->ReadInt();

		// this is not our event
		if(nWaitEventID != nMsgEventID)
			return false;

		pOutput->WriteNetBuffer(pMsg->pMessage, true, nMsgDataSize);

		pOutput->ResetPos();

		// ��������� ���������� �������
		pOutput->SetClientInfo( pMsg->addr, pMsg->clientid );
	}

	pMsg->flags |= MSGFLAG_PROCESSED;

	return true;
}

// sets event filter callback, usually for servers
void CNetworkThread::SetEventFilterCallback(pfnEventFilterCallback fnCallback)
{
	m_fnEventFilter = fnCallback;
}

// sets event filter callback, usually for servers
void CNetworkThread::SetCycleCallback(pfnUpdateCycleCallback fnCallback)
{
	m_fnCycleCallback = fnCallback;
}

// sends event
int CNetworkThread::SendEvent( CNetEvent* pEvent, int nEventType, int client_id /* = -1*/, int nFlags/* = NSFLAG_IMMEDIATE*/ )
{
	ASSERT(pEvent);

	if(nEventType != -1) // �������� �� ������ ������������
	{
		ASSERTMSG(nEventType == pEvent->GetEventType(), varargs("bad event type (%d vs %d)", nEventType, pEvent->GetEventType()));
	}
	else
		nEventType = pEvent->GetEventType();

	if(!m_pNetInterface)
		ASSERT(!"m_pNetInterface=NULL when SendEvent called. Please fix your algorhithms");

	//CScopedMutex m(m_Mutex);

	sendevent_t evt;

	evt.event_type = nEventType;
	evt.client_id = client_id;
	evt.flags = nFlags;

#ifndef NO_LUA
	int nLuaEventID = -1;

	// ������� ��� Lua ���������� � NETTHREAD_EVENTS_LUA_START
	if(evt.event_type >= NETTHREAD_EVENTS_LUA_START)
	{
		nLuaEventID = evt.event_type - NETTHREAD_EVENTS_LUA_START;

		evt.event_type = NETTHREAD_EVENTS_LUA_START;
	}
#endif // NO_LUA

	evt.message_id = -1; // to be assigned

	evt.pEvent = pEvent;
	evt.pStream = new CNetMessageBuffer(m_pNetInterface);

	if(evt.client_id == NM_SENDTOLAST)
		evt.client_id = m_nLastClientID; // doesn't matter for server

	sockaddr_in emptyAddr;
	memset(&emptyAddr, 0, sizeof(emptyAddr));

	evt.pStream->SetClientInfo( emptyAddr, evt.client_id );

	int8 msg_type = MSGTYPE_EVENT;

	// ���� � ��� Int ��� ������� ����, ��... �����
	if(m_nEventCounter >= 65535)
		m_nEventCounter = 0;

	evt.event_id = m_nEventCounter++;

	// write message header
	evt.pStream->WriteByte( msg_type );

	evt.pStream->WriteUInt16( evt.event_type );
	evt.pStream->WriteUInt16( evt.event_id ); // write increment event counter

#ifndef NO_LUA
	// ID Lua �������.
	// �������� � LUANetEventCallbackFactory, ���� �� ���� ������ ���� ������ Pack/Unpack
	if(nLuaEventID >= 0)
		evt.pStream->WriteUInt16( nLuaEventID );
#endif // NO_LUA

	// ���������� ���������
	pEvent->Pack( this, evt.pStream );

	// �������� ��� � ���
	m_pSentEvents.append( evt );

	return evt.event_id;
}

// sends event and waits for it
bool CNetworkThread::SendWaitDataEvent(CNetEvent* pEvent, int nEventType, CNetMessageBuffer* pOutputData, int client_id)
{
	ASSERT(pOutputData);

	// block next UpdateDispatchEvents routine for a while
	//m_bLockUpdateDispatch = true;

	// push event to queue
	int nEventID = SendEvent(pEvent, nEventType, client_id, NSFLAG_GUARANTEED);

#pragma todo("non-fixed timeout for SendWaitDataEvent")

	const double fTimeOut = 2.5; // 5 seconds to reach timeout
	const double fPendingTimeOut = 5.0; // 5 seconds to reach timeout

	double fStartTime = m_Timer.GetTime();
	double fPendingLastTime = m_Timer.GetTime();

	CUDPSocket* pSock = m_pNetInterface->GetSocket();

	bool bMessageIsUp = false;
	bool bErrorMessage = false;

	while(true)
	{
		DkList<msg_status_t> msgStates;
		pSock->GetMessageStatusList( msgStates );

		// dispatch events
		// but check our local event for status
		for(int i = 0; i < m_pSentEvents.numElem(); i++)
		{
			EEventMessageStatus status = DispatchEvent(i, msgStates);

			if(m_pSentEvents[i].event_id == nEventID)
			{
				if( status == EVTMSGSTATUS_ERROR)
				{
					bErrorMessage = true;
				}
				else
				{
					// if message is in pool, reset timeout
					fPendingLastTime = m_Timer.GetTime();
					bMessageIsUp = true;
				}
			}

			// if it's no longer pending and sent or have error
			if(status != EVTMSGSTATUS_PENDING)
			{
				if(status == EVTMSGSTATUS_OK)
				{
					bMessageIsUp = true;
				}

				DestroySendEvent( m_pSentEvents[i] );
				m_pSentEvents.fastRemoveIndex(i);
				i--;
			}

			if(bErrorMessage)
				break;
		}

		if(bErrorMessage)
			break;

		if(!bMessageIsUp ||
			((m_Timer.GetTime() - fPendingLastTime > fTimeOut) ||	// ������� ������� ����� (����� �� ����� ������� ACK)
			(m_Timer.GetTime() - fStartTime > fPendingTimeOut)))	// ����-��� �� ������� ���������
		{
			//Msg("message timed out (up=%d) (stimeout=%g, ptimeout=%g)\n", bMessageIsUp ? 1 : 0, m_Timer.GetTime() - fStartTime, m_Timer.GetTime() - fPendingLastTime);
			break;
		}

		rcvdmessage_t* pMsg = m_pFirstMessage;

		while(pMsg)
		{
			rcvdmessage_t* procMsg = pMsg;

            pMsg = pMsg->pNext;

			bool result = ProcessMessage( procMsg, pOutputData, nEventID );

			if( result )
			{
				m_Mutex.Lock();
				FreeMessage( procMsg );
				m_Mutex.Unlock();

				// if message was read
				if(pOutputData->GetMessageLength() > 0)
					break;
			}
		}

		// if message was read
		if(pOutputData->GetMessageLength() > 0)
			break;

		Platform_Sleep(1);
	}

	// clear
	m_bLockUpdateDispatch = false;

	return (pOutputData->GetMessageLength() > 0);
}

bool CNetworkThread::IsMessageInPool(int nEventID)
{
	for(int i = 0; i < m_pSentEvents.numElem(); i++)
	{
		if(m_pSentEvents[i].event_id == nEventID)
		{
			return true;
		}
	}

	return false;
}

bool CNetworkThread::SendData(CNetMessageBuffer* pData, int nMsgEventID, int client_id, int nFlags)
{
	ASSERT(pData);

	if(!m_pNetInterface)
		ASSERT(!"m_pNetInterface=NULL when SendData called. Did you forgot 'SetNetworkInterface'? ");

	CScopedMutex m(m_Mutex);

	sendevent_t evt;

	evt.event_type = -1;
	evt.client_id = client_id;
	evt.flags = nFlags;

	evt.message_id = -1;	// to be assigned

	evt.pEvent = NULL;		// no event, data only
	evt.pStream = new CNetMessageBuffer(m_pNetInterface);

	if(evt.client_id == NM_SENDTOLAST)
		evt.client_id = m_nLastClientID; // doesn't matter for server

	sockaddr_in emptyAddr;
	memset(&emptyAddr, 0, sizeof(emptyAddr));

	evt.pStream->SetClientInfo( emptyAddr, evt.client_id );

	int8 msg_type = MSGTYPE_DATA;

	// write message header
	evt.pStream->WriteByte( msg_type );
	evt.pStream->WriteUInt16( nMsgEventID );
	evt.pStream->WriteInt( pData->GetMessageLength() );

	evt.pStream->WriteNetBuffer(pData);

	evt.event_id = nMsgEventID;

	// add to pool
	m_pSentEvents.append( evt );

	return true;
}

// registers event factory
void CNetworkThread::RegisterEventList( const DkList<neteventfactory_t>& list, bool bUnregisterOld )
{
	if(bUnregisterOld)
		m_netEventFactory.clear();

	// check for the already registered event
	for(int i = 0; i < m_netEventFactory.numElem(); i++)
	{
		for(int j = 0; j < list.numElem(); j++)
		{
			if(m_netEventFactory[i].eventIdent == list[j].eventIdent)
				ASSERT( !"Event was already registered" );
		}

		//UnregisterEvent( eventIdentifier );
	}

	m_netEventFactory.append(list);
}

void CNetworkThread::RegisterEvent( pfnNetEventFactory fnFactoryFunc, int eventIdentifier )
{
	// check for the already registered event
	for(int i = 0; i < m_netEventFactory.numElem(); i++)
	{
		if(m_netEventFactory[i].eventIdent == eventIdentifier)
			ASSERT( !"Event was already registered" );

		//UnregisterEvent( eventIdentifier );
	}

	neteventfactory_t evt;
	evt.eventIdent = eventIdentifier;
	evt.factory = fnFactoryFunc;

	m_netEventFactory.append(evt);
}

void CNetworkThread::UnregisterEvent( int eventIdentifier )
{
	for(int i = 0; i < m_netEventFactory.numElem(); i++)
	{
		if(m_netEventFactory[i].eventIdent == eventIdentifier)
		{
			m_netEventFactory.fastRemoveIndex(i);
			i--;
		}
	}
}

CNetEvent* CNetworkThread::CreateEvent( CNetMessageBuffer *pMsg, int eventIdentifier )
{
	ASSERT(m_netEventFactory.numElem() > 0);

	for(int i = 0; i < m_netEventFactory.numElem(); i++)
	{
		if(m_netEventFactory[i].eventIdent == eventIdentifier)
		{
			return (m_netEventFactory[i].factory)(this, pMsg);
		}
	}

	return NULL;
}

#ifndef NO_LUA



CLuaNetEvent::CLuaNetEvent(lua_State* vm, OOLUA::Table& table)
{
	m_state = vm;
	m_table = table;		// ���������� �������, ��� ��� ����� ���

	EqLua::LuaStackGuard g(m_state);

	// ����� ��������� �������, ����� ����� ���� �������� �������
	m_table.push_on_stack( m_state );

	// �������� ������ �� �������
	lua_getfield(m_state, -1, "Pack");
	m_pack.set_ref(m_state, luaL_ref(m_state, LUA_REGISTRYINDEX));

	lua_getfield(m_state, -1, "Unpack");
	m_unpack.set_ref(m_state, luaL_ref(m_state, LUA_REGISTRYINDEX));

	lua_getfield(m_state, -1, "Process");
	m_process.set_ref(m_state, luaL_ref(m_state, LUA_REGISTRYINDEX));

	lua_getfield(m_state, -1, "OnDeliveryFailed");
	m_deliveryfail.set_ref(m_state, luaL_ref(m_state, LUA_REGISTRYINDEX));
}

void CLuaNetEvent::Process( CNetworkThread* pNetThread )
{
	int n = lua_gettop(m_state);
	lua_pop(m_state, n);

	EqLua::LuaStackGuard s(m_state);

	// call table function, passing parameters
	if(!m_process.push(m_state))
	{
		MsgError("CLuaNetEvent::Unpack push failed\n");
		return;
	}

	m_table.set( "eventId", m_nEventID );

	// first we place a parent table of 'Pack' function it as this\self
	m_table.push_on_stack(m_state);

	// next are to place arguemnts
	OOLUA::push(m_state, pNetThread);

	int res = lua_pcall(m_state, 2, 0, 0);

	// if error
	if( res != 0 )
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(m_state);

#ifdef NETTHREAD_MESSAGEBOX_ERRORS
		ErrorMsg(varargs("CLuaNetEvent::Process error:\n %s\n", OOLUA::get_last_error(m_state).c_str()));
#else
		MsgError("CLuaNetEvent::Process error:\n %s\n", OOLUA::get_last_error(m_state).c_str());
#endif // NETTHREAD_MESSAGEBOX_ERRORS
	}
}

void CLuaNetEvent::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	int n = lua_gettop(m_state);
	lua_pop(m_state, n);

	EqLua::LuaStackGuard s(m_state);

	// call table function, passing parameters
	if(!m_unpack.push(m_state))
	{
		MsgError("CLuaNetEvent::Unpack push failed\n");
		return;
	}

	m_table.set( "eventId", m_nEventID );

	// first we place a parent table of 'Pack' function it as this\self
	m_table.push_on_stack(m_state);

	// next are to place arguemnts
	OOLUA::push(m_state, pNetThread);
	OOLUA::push(m_state, pStream);

	int res = lua_pcall(m_state, 3, 0, 0);

	// if error
	if( res != 0 )
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(m_state);
#ifdef NETTHREAD_MESSAGEBOX_ERRORS
		ErrorMsg(varargs("CLuaNetEvent::Unpack error:\n %s\n", OOLUA::get_last_error(m_state).c_str()));
#else
		MsgError("CLuaNetEvent::Unpack error:\n %s\n", OOLUA::get_last_error(m_state).c_str());
#endif // NETTHREAD_MESSAGEBOX_ERRORS
	}
}

void CLuaNetEvent::Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	int n = lua_gettop(m_state);
	lua_pop(m_state, n);

	EqLua::LuaStackGuard s(m_state);

	// call table function, passing parameters
	if(!m_pack.push(m_state))
	{
		MsgError("CLuaNetEvent::Unpack push failed\n");
		return;
	}

	// first we place a parent table of 'Pack' function it as this\self
	m_table.push_on_stack(m_state);

	// next are to place arguemnts
	OOLUA::push(m_state, pNetThread);
	OOLUA::push(m_state, pStream);

	int res = lua_pcall(m_state, 3, 0, 0);

	// if error
	if( res != 0 )
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(m_state);

#ifdef NETTHREAD_MESSAGEBOX_ERRORS
		ErrorMsg(varargs("CLuaNetEvent::Pack error:\n %s\n", OOLUA::get_last_error(m_script->state()).c_str()));
#else
		MsgError("CLuaNetEvent::Pack error:\n %s\n", OOLUA::get_last_error(m_state).c_str());
#endif // NETTHREAD_MESSAGEBOX_ERRORS
	}
}

// standard message handlers

// delivery completely failed after retries: if returns false, this removes message
bool CLuaNetEvent::OnDeliveryFailed()
{
	EqLua::LuaStackGuard s(m_state);

	// call table function and return it's result
	if(!m_deliveryfail.push(m_state))
		return false;

	// first we place a parent table of 'Pack' function it as this\self
	m_table.push_on_stack(m_state);

	int res = lua_pcall(m_state, 1, 0, 0);

	bool result = false;

	// if error
	if( res != 0 )
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(m_state);

#ifdef NETTHREAD_MESSAGEBOX_ERRORS
		ErrorMsg(varargs("CLuaNetEvent::OnDeliveryFailed error:\n %s\n", OOLUA::get_last_error(m_state).c_str()));
#else
		MsgError("CLuaNetEvent::OnDeliveryFailed error:\n %s\n", OOLUA::get_last_error(m_state).c_str());
#endif // NETTHREAD_MESSAGEBOX_ERRORS
	}
	else
	{
		 OOLUA::pull(m_state, result);
	}

	return result;
}

// ������ ������� ������������� ��� LUA. ��������� ���
CNetEvent* LUANetEventCallbackFactory(CNetworkThread* thread, CNetMessageBuffer* msg)
{
	// ������� ������� � LUA
	OOLUA::Script& state = GetLuaState();

	EqLua::LuaStackGuard s(state);

	int nLuaEventID = msg->ReadInt();

	if(!state.call("CreateNetEvent", nLuaEventID))
	{
		Msg("LUANetEventCallbackFactory error:\n %s\n", OOLUA::get_last_error(state).c_str());
		//Msg("	-- this error could be if no CreateNetEvent function found, check 'netevents.lua'\n");
		return NULL;
	}

	// �������� �������
	if (lua_isnil(state, -1))
	{
		MsgError("LuaNetEvent ID '%d' is not registered, maybe you forgot to AddNetEvent?\n", nLuaEventID);
	}
	else
	{
		if(!lua_istable(state, -1))
		{
			MsgError("LuaNetEvent ID '%d' has invalid factory type\n", nLuaEventID);
			Msg("	-- follow the documentation in 'netevents.lua'\n");
		}
		else
		{
			OOLUA::Table table;

			table.lua_get(state, -1);

			// ������ �� ����� ������������������
			return (CNetEvent*)new CLuaNetEvent(state, table);
		}
	}

	return NULL;
}


// send event called in LUA
int CNetworkThread::SendLuaEvent(OOLUA::Table& luaevent, int nEventType, int client_id)
{
	CLuaNetEvent* pEvent = new CLuaNetEvent(luaevent.state(), luaevent);
	//pEvent->m_nEventID = NETTHREAD_EVENTS_LUA_START;

	// event type �������� ���������.
	return SendEvent(pEvent, NETTHREAD_EVENTS_LUA_START + nEventType, client_id);
}

// send event with waiter called in LUA
bool CNetworkThread::SendLuaWaitDataEvent(OOLUA::Table& luaevent, int nEventType, CNetMessageBuffer* pOutputData, int client_id)
{
	CLuaNetEvent* pEvent = new CLuaNetEvent(luaevent.state(), luaevent);

	return SendWaitDataEvent(pEvent, NETTHREAD_EVENTS_LUA_START + nEventType, pOutputData, client_id);
}

#endif // NO_LUA

}; // namespace Networking
