//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine network thread
//				Provides event message sending
//////////////////////////////////////////////////////////////////////////////////

#ifndef NETTHREAD_H
#define NETTHREAD_H

#include "utils/eqthread.h"
#include "ds/Array.h"
#include "utils/eqtimer.h"

#include "network/c_udp.h"
#include "network/NetInterfaces.h"
#include "network/Buffer.h"
#include "network/NetEvent.h"

namespace Networking
{

#ifdef _WIN32
#undef CreateEvent
#endif

// event filtering
enum EventFilterResult_e
{
	EVENTFILTER_OK = 0,
	EVENTFILTER_ERROR_NOTALLOWED,
};

#define NETTHREAD_EVENTS_LUA_START		100

// event filter callback definition
typedef EventFilterResult_e (*pfnEventFilterCallback)( CNetworkThread* pNetThread, Buffer* pMsg, int nEventType );

// event filter callback definition
typedef void (*pfnUpdateCycleCallback)( CNetworkThread* pNetThread, double fDt);

struct rcvdMessage_t;
struct netFragMsg_t;

struct sendEvent_t
{

	uint16				event_type;
	uint16				event_id;
	int					client_id;

	int					flags;

	CNetEvent*			pEvent;
	Buffer*	pStream;
};

enum EEventMessageStatus
{
	EVTMSGSTATUS_ERROR = -1,
	EVTMSGSTATUS_PENDING = 0,
	EVTMSGSTATUS_OK,
};

//-----------------------------------------------------
// network event generator/processor thread
//-----------------------------------------------------
class CNetworkThread : public Threading::CEqThread
{
public:
	CNetworkThread( INetworkInterface* pInterface );
	~CNetworkThread();

	void						Init();

	virtual void				OnCycle() {}

	void						FreeMessages();
	void						FreeMessage( rcvdMessage_t* pMessage );

	INetworkInterface*			GetNetworkInterface();
	void						SetNetworkInterface(INetworkInterface* pInterface);

	//-------------------------------------------------------------
	//< message processors and event generators and generate events
	//-------------------------------------------------------------

	void						StopWork();

	int							GetPooledEventCount() {return m_queuedEvents.numElem();}
	bool						IsMessageInPool(int nEventID);

	///< sets event filter callback, usually for servers
	void						SetEventFilterCallback(pfnEventFilterCallback fnCallback);

	///< sets cycle callback
	void						SetCycleCallback(pfnUpdateCycleCallback fnCallback);

	///< registers event factory
	void						RegisterEventList( const Array<neteventfactory_t>& list, bool bUnregisterOld = false );

	void						RegisterEvent( pfnNetEventFactory fnFactoryFunc, int eventIdentifier );
	void						UnregisterEvent( int eventIdentifier );

	///< generates events from recieved messages
	int							UpdateDispatchEvents();

	//-----------------------------------------------------

	// send data
	///< nMsgEventID - data query caller
	bool						SendData(Buffer* pData, int nMsgEventID, int client_id = -1, int flags = 0);

	///< sends event, returns event ID
	int							SendEvent( CNetEvent* pEvent, int nEventType, int client_id = -1, int flags = CDPSEND_IMMEDIATE);

	///< sends event and waits for data (SendData)
	///< you should not call it inside another event.
	///< returns false if no data has been recieved (you will assume that event does not support output)
	///< this messages are always must be guaranteed
	bool						SendWaitDataEvent(CNetEvent* pEvent, int nEventType, Buffer* pOutputData, int client_id = -1);

	//-----------------------------------------------------

	///< time
	double						GetCurTime() {return m_Timer.GetTime();}

	///< previous time
	double						GetPrevTime() {return m_prevTime;}

protected:
	virtual int					Run();

private:

	static void					OnUCDPRecievedStatic(void* thisptr, ubyte* data, int size, const sockaddr_in& from, short msgId, ERecvMessageKind type);
	void						OnUCDPRecieved(ubyte* data, int size, const sockaddr_in& from, short msgId, ERecvMessageKind type);

	int							DispatchEvents();
	EEventMessageStatus			DispatchEvent( sendEvent_t& evt );

	CNetEvent*					CreateEvent( Buffer *pMsg, int eventIdentifier );

	bool						ProcessMessage( rcvdMessage_t* pMsg, Buffer* pOutput = NULL, int nWaitEventID = -1 );

	///< simply adds message
	void						AddMessage(rcvdMessage_t* pMessage, rcvdMessage_t* pAddTo);

	///< adds fragmented message to waiter
	void						AddFragmentedMessage( netMessage_t* pMsg, int size, const sockaddr_in& addr );

	///< dispatches fragmented message to the queue
	void						DispatchFragmentedMessage( netFragMsg_t* pMsg );

protected:

	// event factory
	Array<neteventfactory_t>	m_netEventFactory;

	// network interface this thread using
	INetworkInterface*			m_netInterface;

	// delayed messages, for testing network only
	Array<rcvdMessage_t*>		m_lateMessages;

	// all undispatched fragmented messages
	Array<netFragMsg_t*>		m_fragmented_messages;

	// message queue
	rcvdMessage_t*				m_firstMessage;
	rcvdMessage_t*				m_lastMessage;

	// events
	Array<sendEvent_t>			m_queuedEvents;

	// callbacks
	pfnEventFilterCallback		m_fnEventFilter;		// event filter callback
	pfnUpdateCycleCallback		m_fnCycleCallback;		// thread cycle callback, used for heartbeat or smth

	// last client ID, used for synchronized messages which returns DATA
	int							m_lastClientID;

	Threading::CEqMutex&		m_mutex;
	bool						m_lockUpdateDispatch;
	bool						m_stopWork;

	double						m_prevTime;

	CEqTimer					m_Timer;

	///< this was made for syncronized client queries
	int							m_eventCounter;
};



}; // namespace Networking

#endif // NETTHREAD_H
