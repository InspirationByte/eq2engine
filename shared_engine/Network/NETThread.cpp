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
struct rcvdMessage_t
{
	rcvdMessage_t()
	{
		pNext = NULL;
		pPrev = NULL;
		recvTime = 0.0;
		clientid = -1;
		messageid = -1;
		flags = 0;
		recvTime = 0.0;
	}

	ERecvMessageKind	type;

	short				clientid;
	short				messageid;

	int					flags;

	sockaddr_in			addr;		// paired with client id now, unnecessary to check

	rcvdMessage_t*		pNext;
	rcvdMessage_t*		pPrev;

	double				recvTime;

	CNetMessageBuffer*	data;
};

struct netFragMsg_t
{
	short			client_id;
	sockaddr_in		addr;		// paired with client id now, unnecessary to check

	short			message_id;
	uint8			nfragments;

	DkList< netMessage_t* > parts;
};

//--------------------------------------------------------------------

CNetworkThread::CNetworkThread( INetworkInterface* pInterface ) 
	: Threading::CEqThread(), m_mutex(GetGlobalMutex(MUTEXPURPOSE_NET_THREAD)),
	m_firstMessage(NULL), m_lastMessage(NULL), m_netInterface(pInterface),
	m_prevTime(0.0), m_lastClientID(-1), m_stopWork(false), m_lockUpdateDispatch(false),
	m_fnCycleCallback(NULL), m_fnEventFilter(NULL), m_eventCounter(0)
{

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
	return m_netInterface;
}

void CNetworkThread::SetNetworkInterface(INetworkInterface* pInterface)
{
	// TODO: reset undispatched events
	m_netInterface = pInterface;
}

void CNetworkThread::StopWork()
{
	if(IsTerminating() || !IsRunning())
		return;

	m_mutex.Lock();
	m_stopWork = true;
	m_mutex.Unlock();

	// move all pooled events to send buffer
	DispatchEvents();

	// wait and stop
	StopThread();
}

void CNetworkThread::OnUCDPRecievedStatic(void* thisptr, ubyte* data, int size, const sockaddr_in& from, short msgId, ERecvMessageKind type)
{
	CNetworkThread* thisThread = (CNetworkThread*)thisptr;

	thisThread->OnUCDPRecieved( data, size, from, msgId, type );
}

void CNetworkThread::OnUCDPRecieved(ubyte* data, int size, const sockaddr_in& from, short msgId, ERecvMessageKind type)
{
	if(type == RECV_MSG_STATUS)
	{
		// TODO: find sent event with msgId and signal about it's success
		int status = size;

		CScopedMutex m(m_mutex);

		for(int i = 0; i < m_queuedEvents.numElem(); i++)
			m_queuedEvents[i].pStream->SetMessageStatus(msgId, status);

		return;
	}

	netMessage_t::hdr_t* msgHdr = (netMessage_t::hdr_t*)data;

	if( msgHdr->nfragments > 1 )
	{
		netMessage_t* rcvdMsg = new netMessage_t;

		if(!m_netInterface->PreProcessRecievedMessage( data, size, *rcvdMsg))
		{
			// TODO: if it had fragments, remove them // if(rcvdMsg->header.nfragments > 0)
			delete rcvdMsg;
			return;
		}

		// add fragment to waiter
		// this function also will try to dispatch message that got fully received
		m_mutex.Lock();
		AddFragmentedMessage( rcvdMsg, rcvdMsg->header.message_size, from );
		m_mutex.Unlock();
	}
	else
	{
		netMessage_t rcvdMsg; // fixme: allocate in temp memory?

		if(!m_netInterface->PreProcessRecievedMessage( data, size, rcvdMsg))
			return;

		// add as usual
		rcvdMessage_t* pMsg = new rcvdMessage_t;
		pMsg->type = type;

		pMsg->data = new CNetMessageBuffer( m_netInterface );
		pMsg->data->SetClientInfo( from, rcvdMsg.header.clientid );
		pMsg->addr = from;
		pMsg->messageid = rcvdMsg.header.messageid;

		// write contents
		pMsg->data->WriteData( rcvdMsg.data, rcvdMsg.header.message_size);

		pMsg->flags = 0;

		pMsg->pNext = NULL;
		pMsg->pPrev = NULL;
		pMsg->clientid = rcvdMsg.header.clientid;

		// queue our message
		m_mutex.Lock();
		AddMessage(pMsg, m_lastMessage);
		m_mutex.Unlock();
	}
}

int CNetworkThread::Run()
{
	if(!m_netInterface)
		return -1;

	double curTime = m_Timer.GetTime();
	m_prevTime = m_Timer.GetTime();

	// completely stop work after all messages are sent
	bool bDoStopWork = false;

	while(!bDoStopWork)
	{
		// wait for send if shutting down
		bDoStopWork = m_stopWork && (m_netInterface->GetSocket()->GetSendPoolCount() == 0);

		curTime = m_Timer.GetTime();

		// do custom cycles
		OnCycle();

		// TEST:	checking for lag packets
		//			and add them
		for(int i = 0; i < m_lateMessages.numElem(); i++)
		{
			if(curTime >= m_lateMessages[i]->recvTime )
			{
				AddMessage(m_lateMessages[i], m_lastMessage);

				m_lateMessages.removeIndex(i);
				i--;
			}
		}

		double timeSinceLastUpdate = curTime - m_prevTime;

		// call our cycle callback
		if(m_fnCycleCallback)
			(m_fnCycleCallback)(this, timeSinceLastUpdate);

		m_netInterface->Update( (int)(timeSinceLastUpdate * 1000.0), OnUCDPRecievedStatic, this );

		m_prevTime = curTime;

		// this system is lagging very hard if you use 1
		Platform_Sleep( 1 );
	}

	// reset
	m_stopWork = false;

	return 0;
}


void CNetworkThread::FreeMessage(rcvdMessage_t* pMsg)
{
	if(pMsg->pPrev)
		pMsg->pPrev->pNext = pMsg->pNext;
	else
		m_firstMessage = pMsg->pNext;

	if(pMsg->pNext)
		pMsg->pNext->pPrev = pMsg->pPrev;
	else
		m_lastMessage = pMsg->pPrev;

	delete pMsg->data;
	delete pMsg;
}

void CNetworkThread::AddMessage(rcvdMessage_t* pMsg, rcvdMessage_t* pAddTo)
{
	pMsg->recvTime = GetCurTime();

	// LAG PACKETS
	if( !(pMsg->flags & MSGFLAG_TESTLAGGING) && net_fakelatency.GetFloat() > 0 )
	{
		// put messages into temporary buffer

		pMsg->flags |= MSGFLAG_TESTLAGGING;

		pMsg->recvTime += RandomFloat(net_fakelatency.GetFloat() * net_fakelatency_randthresh.GetFloat(), net_fakelatency.GetFloat()) * 0.001f;

		m_lateMessages.append( pMsg );

		return;
	}

	pMsg->data->ResetPos();

	// filter event receiving, at 'add' level
	int8 type = pMsg->data->ReadByte();

	if( type == MSGTYPE_EVENT )
	{
		int nEventType = pMsg->data->ReadInt();

		// filter events
		if( m_fnEventFilter )
		{
			// assume the eventfilter can decode our messages
			int result = (m_fnEventFilter)( this, pMsg->data, nEventType );

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
	pMsg->data->ResetPos();

	if(!m_firstMessage || !pAddTo)
	{
		m_firstMessage = pMsg;
		m_lastMessage = pMsg;
		return;
	}

	pMsg->pPrev = pAddTo;
	pMsg->pNext = pAddTo->pNext;

	if(!pAddTo->pNext)
		m_lastMessage = pMsg;
	else
		pAddTo->pNext->pPrev = pMsg;

	pAddTo->pNext = pMsg;
}

// adds fragmented message to waiter
void CNetworkThread::AddFragmentedMessage( netMessage_t* pMsg, int size, const sockaddr_in& addr )
{
	netFragMsg_t* fragMsg = NULL;
	int idx = -1;

	// find waiting message
	for(int i = 0; i < m_fragmented_messages.numElem(); i++)
	{
		netFragMsg_t* msg = m_fragmented_messages[i];

		if(	msg->client_id == pMsg->header.clientid &&
			msg->message_id == pMsg->header.messageid &&
			msg->nfragments == pMsg->header.nfragments &&
			msg->parts.numElem() < pMsg->header.nfragments)
		{
			fragMsg = msg;
			idx = i;
			break;
		}
	}

	// create if not found
	if(!fragMsg)
	{
		fragMsg = new netFragMsg_t;
		fragMsg->client_id = pMsg->header.clientid;
		fragMsg->addr = addr;
		fragMsg->message_id = pMsg->header.messageid;
		fragMsg->nfragments = pMsg->header.nfragments;

		idx = m_fragmented_messages.append(fragMsg);
	}

	// add message
	fragMsg->parts.append( pMsg );

	if( fragMsg->parts.numElem() == fragMsg->nfragments )
	{
		DispatchFragmentedMessage( fragMsg );

		// delete this fragment collection
		delete fragMsg;

		m_fragmented_messages.fastRemoveIndex( idx );
	}
}

int sort_cmp_msgParts( netMessage_t* const &a, netMessage_t* const &b)
{
	return a->header.fragmentid - b->header.fragmentid;
}

// dispatches fragmented message to the queue
void CNetworkThread::DispatchFragmentedMessage( netFragMsg_t* pMsg )
{
	// sort messages by fragment id and join em all to the single message
	// then we have to add it to the main processing queue
	pMsg->parts.sort( sort_cmp_msgParts );

	// copy message
	rcvdMessage_t* rcvdMsg = new rcvdMessage_t;

	CNetMessageBuffer* finalBuffer = new CNetMessageBuffer( m_netInterface );
	finalBuffer->SetClientInfo(pMsg->addr, pMsg->client_id);

	rcvdMsg->data = finalBuffer;

	// write fragments to received message and reset pointers
	for(int i = 0; i < pMsg->parts.numElem(); i++)
	{
		finalBuffer->WriteData( pMsg->parts[i]->data, pMsg->parts[i]->header.message_size);

		delete pMsg->parts[i];
	}

	rcvdMsg->flags = MSGFLAG_FRAGMENTED;

	rcvdMsg->pNext = NULL;
	rcvdMsg->pPrev = NULL;
	rcvdMsg->clientid = pMsg->client_id;
	rcvdMsg->messageid = pMsg->message_id;
	rcvdMsg->addr = pMsg->addr;

	// queue message
	AddMessage( rcvdMsg, m_lastMessage );
}

void CNetworkThread::FreeMessages()
{
	m_mutex.Lock();

	rcvdMessage_t* pMsg = m_firstMessage;

	while( pMsg )
	{
		rcvdMessage_t* toRem = pMsg;

		pMsg = pMsg->pNext;

		delete toRem->data;
		delete toRem;
	}

	m_firstMessage = NULL;
	m_lastMessage = NULL;

	m_mutex.Unlock();
}

void DestroySendEvent(const sendEvent_t& evt)
{
	delete evt.pEvent;
	delete evt.pStream;
}

// generates events from recieved messages
int CNetworkThread::UpdateDispatchEvents()
{
	if( !m_netInterface )
		return 0;

	rcvdMessage_t* pMsg = m_firstMessage;

	while(pMsg)
	{
		rcvdMessage_t* procMsg = pMsg;

		ProcessMessage( procMsg );

		pMsg = pMsg->pNext;

		m_mutex.Lock();
		FreeMessage( procMsg );
		m_mutex.Unlock();
	}

	// dispatch events
	return DispatchEvents();
}

int CNetworkThread::DispatchEvents()
{
	int numPendingMessages = 0;

	// Try to send our events/datas
	for(int i = 0; i < m_queuedEvents.numElem(); i++)
	{
		EEventMessageStatus status = DispatchEvent( m_queuedEvents[i] );

		// if it's no longer pending and sent or have error
		if( status != EVTMSGSTATUS_PENDING )
		{
			DestroySendEvent( m_queuedEvents[i] );
			m_queuedEvents.fastRemoveIndex(i);
			i--;
		}
		else
			numPendingMessages++;
	}

	return numPendingMessages;
}

EEventMessageStatus CNetworkThread::DispatchEvent( sendEvent_t& evt )
{
	// don't send event if we have no peers
	if(evt.client_id == NM_SENDTOALL && !m_netInterface->HasPeersToSend())
		return EVTMSGSTATUS_ERROR;

	if( evt.flags & CDPSEND_IMMEDIATE )
	{
		// if interface fails, run down immediately!
		if( !evt.pStream->Send( evt.flags ) )
			return EVTMSGSTATUS_ERROR;

		return EVTMSGSTATUS_OK;
	}

	// send non-immediate messages
	if( !evt.pStream->Send( evt.flags ) )
	{
		MsgError("DispatchEvent error: can't send eventtype=%d to client=%d\n", evt.event_type, evt.client_id);
		return EVTMSGSTATUS_ERROR;
	}

	int sendStatus = evt.pStream->GetOverallStatus();

	if(sendStatus == DELIVERY_IN_PROGRESS)
		return EVTMSGSTATUS_PENDING;

	return sendStatus == DELIVERY_SUCCESS ? EVTMSGSTATUS_OK : EVTMSGSTATUS_ERROR;
}

bool CNetworkThread::ProcessMessage( rcvdMessage_t* pMsg, CNetMessageBuffer* pOutput, int nWaitEventID)
{
	// already processed message? (NEVER HAPPENS)
	if( pMsg->flags & MSGFLAG_PROCESSED )
		return true;

	CNetMessageBuffer* pMessage = pMsg->data;

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

		return (type == MSGTYPE_DATA);
	}

	if( type == MSGTYPE_EVENT )
	{
		int nEventType = pMessage->ReadUInt16();
		int nEventID = pMessage->ReadUInt16();

		// dispatch event
		CNetEvent* pEvent = CreateEvent( pMessage, nEventType );

		if( !pEvent )
		{
			MsgError("Invalid event id='%d' from client %d (thread: %s)\n", nEventType, pMessage->GetClientID(), GetName());
			pMsg->flags |= MSGFLAG_PROCESSED;

			return true;
		}

		pMessage->SetClientInfo( pMsg->addr, pMsg->clientid );
		pEvent->SetID(nEventID);

		pEvent->Unpack( this, pMessage );

		m_lastClientID = pMessage->GetClientID();

		// now process it
		pEvent->Process( this );

		delete pEvent;
	}
	else
	{
		int nMsgEventID = pMessage->ReadUInt16();
		int nMsgDataSize = pMessage->ReadInt();

		// this is not our event
		if(nWaitEventID != nMsgEventID)
			return false;

		pOutput->WriteNetBuffer(pMessage, true, nMsgDataSize);
		pOutput->ResetPos();
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

	if(nEventType != -1)
	{
		ASSERTMSG(nEventType == pEvent->GetEventType(), varargs("bad event type (%d vs %d)", nEventType, pEvent->GetEventType()));
	}
	else
		nEventType = pEvent->GetEventType();

	if(!m_netInterface)
		ASSERT(!"m_pNetInterface=NULL when SendEvent called. Please fix your algorhithms");

	sendEvent_t evt;

	evt.event_type = nEventType;
	evt.client_id = client_id;
	evt.flags = nFlags;

#ifndef NO_LUA
	int nLuaEventID = -1;

	// Lua events starts from NETTHREAD_EVENTS_LUA_START
	if(evt.event_type >= NETTHREAD_EVENTS_LUA_START)
	{
		nLuaEventID = evt.event_type - NETTHREAD_EVENTS_LUA_START;

		evt.event_type = NETTHREAD_EVENTS_LUA_START;
	}
#endif // NO_LUA

	evt.pEvent = pEvent;
	evt.pStream = new CNetMessageBuffer(m_netInterface);

	if(evt.client_id == NM_SENDTOLAST)
		evt.client_id = m_lastClientID; // doesn't matter for server

	sockaddr_in emptyAddr;
	memset(&emptyAddr, 0, sizeof(emptyAddr));

	evt.pStream->SetClientInfo( emptyAddr, evt.client_id );

	int8 msg_type = MSGTYPE_EVENT;

	if(m_eventCounter >= 65535)
		m_eventCounter = 0;

	evt.event_id = m_eventCounter++;

	// write message header
	evt.pStream->WriteByte( msg_type );

	evt.pStream->WriteUInt16( evt.event_type );
	evt.pStream->WriteUInt16( evt.event_id ); // write increment event counter

#ifndef NO_LUA
	// handle Lua-handled network events
	if(nLuaEventID >= 0)
		evt.pStream->WriteUInt16( nLuaEventID );
#endif // NO_LUA

	pEvent->Pack( this, evt.pStream );

	// to send pipe
	m_mutex.Lock();
	m_queuedEvents.append( evt );
	m_mutex.Unlock();

	return evt.event_id;
}

// sends event and waits for it
bool CNetworkThread::SendWaitDataEvent(CNetEvent* pEvent, int nEventType, CNetMessageBuffer* pOutputData, int client_id)
{
	ASSERT(pOutputData);

	// push event to queue
	int nEventID = SendEvent(pEvent, nEventType, client_id, CDPSEND_GUARANTEED);

	const double fTimeOut = 2.5; // 2.5 seconds to reach timeout
	const double fPendingTimeOut = 5.0; // 5 seconds to reach timeout

	double fStartTime = m_Timer.GetTime();
	double fPendingLastTime = m_Timer.GetTime();

	bool bMessageIsUp = false;
	bool bErrorMessage = false;

	while(true)
	{
		// dispatch events
		// but check our local event for status
		for(int i = 0; i < m_queuedEvents.numElem(); i++)
		{
			EEventMessageStatus status = DispatchEvent( m_queuedEvents[i] );

			if( m_queuedEvents[i].event_id == nEventID )
			{
				if( status == EVTMSGSTATUS_ERROR)
				{
					bErrorMessage = true;
				}
				else if( status == EVTMSGSTATUS_OK )
				{
					bMessageIsUp = true;
				}
				else
				{
					// if message is in pool, reset timeout
					fPendingLastTime = m_Timer.GetTime();
					bMessageIsUp = true;
				}
			}

			// if it's no longer pending and sent or have error
			if( status != EVTMSGSTATUS_PENDING )
			{
				DestroySendEvent( m_queuedEvents[i] );
				m_queuedEvents.fastRemoveIndex(i);
				i--;
			}

			if(bErrorMessage)
				break;
		}

		if(!bMessageIsUp ||
			((m_Timer.GetTime() - fPendingLastTime > fTimeOut) ||	// did not recieved any confirmation
			(m_Timer.GetTime() - fStartTime > fPendingTimeOut)))
		{
			//Msg("message timed out (up=%d) (stimeout=%g, ptimeout=%g)\n", 
			//	bMessageIsUp ? 1 : 0, m_Timer.GetTime() - fStartTime, m_Timer.GetTime() - fPendingLastTime);

			break;
		}

		rcvdMessage_t* pMsg = m_firstMessage;

		while(pMsg)
		{
			rcvdMessage_t* procMsg = pMsg;

			pMsg = pMsg->pNext;

			bool result = ProcessMessage( procMsg, pOutputData, nEventID );

			if( result )
			{
				m_mutex.Lock();
				FreeMessage( procMsg );
				m_mutex.Unlock();

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

	return (pOutputData->GetMessageLength() > 0);
}

bool CNetworkThread::IsMessageInPool(int nEventID)
{
	for(int i = 0; i < m_queuedEvents.numElem(); i++)
	{
		if(m_queuedEvents[i].event_id == nEventID)
		{
			return true;
		}
	}

	return false;
}

bool CNetworkThread::SendData(CNetMessageBuffer* pData, int nMsgEventID, int client_id, int nFlags)
{
	ASSERT(pData);

	if(!m_netInterface)
		ASSERT(!"m_pNetInterface=NULL when SendData called. Did you forgot 'SetNetworkInterface'? ");

	CScopedMutex m(m_mutex);

	sendEvent_t evt;

	evt.event_type = -1;
	evt.client_id = client_id;
	evt.flags = nFlags;

	evt.pEvent = NULL;		// no event, data only
	evt.pStream = new CNetMessageBuffer(m_netInterface);

	if(evt.client_id == NM_SENDTOLAST)
		evt.client_id = m_lastClientID; // doesn't matter for server

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
	m_queuedEvents.append( evt );

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
	m_table = table;

	EqLua::LuaStackGuard g(m_state);

	m_table.push_on_stack( m_state );

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

CNetEvent* LUANetEventCallbackFactory(CNetworkThread* thread, CNetMessageBuffer* msg)
{
	OOLUA::Script& state = GetLuaState();

	EqLua::LuaStackGuard s(state);

	int nLuaEventID = msg->ReadInt();

	if(!state.call("CreateNetEvent", nLuaEventID))
	{
		Msg("LUANetEventCallbackFactory error:\n %s\n", OOLUA::get_last_error(state).c_str());
		//Msg("	-- this error could be if no CreateNetEvent function found, check 'netevents.lua'\n");
		return NULL;
	}

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

			return (CNetEvent*)new CLuaNetEvent(state, table);
		}
	}

	return NULL;
}


// send event called in LUA
int CNetworkThread::SendLuaEvent(OOLUA::Table& luaevent, int nEventType, int client_id)
{
	CLuaNetEvent* pEvent = new CLuaNetEvent(luaevent.state(), luaevent);

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
