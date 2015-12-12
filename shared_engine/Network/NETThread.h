//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine network thread
//				Provides event message sending
//////////////////////////////////////////////////////////////////////////////////

#ifndef NETTHREAD_H
#define NETTHREAD_H

#include "utils/eqthread.h"
#include "utils/DkList.h"
#include "utils/eqtimer.h"

#include "network/c_udp.h"
#include "network/NetInterfaces.h"
#include "network/NetMessageBuffer.h"
#include "network/NetEvent.h"

#ifndef NO_LUA
#include "luabinding/LuaBinding.h"
#endif // NO_LUA

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
typedef EventFilterResult_e (*pfnEventFilterCallback)( CNetworkThread* pNetThread, CNetMessageBuffer* pMsg, int nEventType );

// event filter callback definition
typedef void (*pfnUpdateCycleCallback)( CNetworkThread* pNetThread, double fDt);

struct rcvdmessage_t;
struct netfragmsg_t;

struct sendevent_t
{
	uint16				event_type;
	uint16				event_id;
	int					client_id;

	short				message_id;

	int					flags;

	CNetEvent*			pEvent;
	CNetMessageBuffer*	pStream;
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
	void						FreeMessage( rcvdmessage_t* pMessage );

	INetworkInterface*			GetNetworkInterface();
	void						SetNetworkInterface(INetworkInterface* pInterface);

	//-------------------------------------------------------------
	//< message processors and event generators and generate events
	//-------------------------------------------------------------

	void						StopWork();

	int							GetPooledEventCount() {return m_pSentEvents.numElem();}
	bool						IsMessageInPool(int nEventID);

	///< sets event filter callback, usually for servers
	void						SetEventFilterCallback(pfnEventFilterCallback fnCallback);

	///< sets cycle callback
	void						SetCycleCallback(pfnUpdateCycleCallback fnCallback);

	///< registers event factory
	void						RegisterEventList( const DkList<neteventfactory_t>& list, bool bUnregisterOld = false );

	void						RegisterEvent( pfnNetEventFactory fnFactoryFunc, int eventIdentifier );
	void						UnregisterEvent( int eventIdentifier );

	///< generates events from recieved messages
	int							UpdateDispatchEvents();

	//-----------------------------------------------------

	// send data
	///< nMsgEventID - data query caller
	bool						SendData(CNetMessageBuffer* pData, int nMsgEventID, int client_id = -1, int flags = 0);

	///< sends event, returns event ID
	int							SendEvent( CNetEvent* pEvent, int nEventType, int client_id = -1, int flags = NSFLAG_IMMEDIATE);

	///< sends event and waits for data (SendData)
	///< you should not call it inside another event.
	///< returns false if no data has been recieved (you will assume that event does not support output)
	///< this messages are always must be guaranteed
	bool						SendWaitDataEvent(CNetEvent* pEvent, int nEventType, CNetMessageBuffer* pOutputData, int client_id = -1);

#ifndef NO_LUA

	///< send event called in LUA
	int							SendLuaEvent(OOLUA::Table& luaevent, int nEventType, int client_id);

	///< send event with waiter called in LUA
	bool						SendLuaWaitDataEvent(OOLUA::Table& luaevent, int nEventType, CNetMessageBuffer* pOutputData, int client_id);

#endif // LUA_INTEGRATE
	//-----------------------------------------------------

	///< time
	double						GetCurTime() {return m_fCurTime;}

	///< previous time
	double						GetPrevTime() {return m_fPrevTime;}

protected:
	virtual int					Run();

private:
	int							DispatchEvents();
	EEventMessageStatus			DispatchEvent( int nIndex, DkList<msg_status_t>& msgStates );



	CNetEvent*					CreateEvent( CNetMessageBuffer *pMsg, int eventIdentifier );

	bool						ProcessMessage( rcvdmessage_t* pMsg, CNetMessageBuffer* pOutput = NULL, int nWaitEventID = -1 );

	///< simply adds message
	void						AddMessage(rcvdmessage_t* pMessage, rcvdmessage_t* pAddTo);

	///< adds fragmented message to waiter
	void						AddFragmentedMessage( netmessage_t* pMsg, int size, const sockaddr_in& addr );

	///< dispatches fragmented message to the queue
	void						DispatchFragmentedMessage( netfragmsg_t* pMsg );

protected:

	DkList<rcvdmessage_t*>		m_lagMessages;

	DkList<sendevent_t>			m_pSentEvents;

	DkList<neteventfactory_t>	m_netEventFactory;

	DkList<netfragmsg_t*>		m_fragmented_messages;

	Threading::CEqMutex			m_Mutex;
	bool						m_bLockUpdateDispatch;
	bool						m_bStopWork;

	pfnEventFilterCallback		m_fnEventFilter;

	pfnUpdateCycleCallback		m_fnCycleCallback;

	int							m_nLastClientID;

	rcvdmessage_t*				m_pFirstMessage;
	rcvdmessage_t*				m_pLastMessage;

	INetworkInterface*			m_pNetInterface;

	double						m_fCurTime;
	double						m_fPrevTime;

	CEqTimer					m_Timer;

	///< this was made for syncronized client queries
	int							m_nEventCounter;
};

#ifndef NO_LUA

// net event class for Lua
class CLuaNetEvent : public CNetEvent
{
public:
	CLuaNetEvent(lua_State* vm, OOLUA::Table& table);

	void Process( CNetworkThread* pNetThread );

	void Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pBuf );

	void Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pBuf );

	bool OnDeliveryFailed();

	int	GetEventType() const {return NETTHREAD_EVENTS_LUA_START;}

protected:
	lua_State*		m_state;
	OOLUA::Table	m_table;

	OOLUA::Lua_func_ref m_pack;
	OOLUA::Lua_func_ref m_unpack;
	OOLUA::Lua_func_ref m_process;
	OOLUA::Lua_func_ref m_deliveryfail;
};

#endif // LUA_INTEGRATE

}; // namespace Networking

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(Networking::CNetworkThread)

	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(SendData)

	OOLUA_MEM_FUNC_RENAME(SendEvent, int, SendLuaEvent, OOLUA::Table&, int, int)
	OOLUA_MEM_FUNC_RENAME(SendWaitDataEvent, bool, SendLuaWaitDataEvent, OOLUA::Table&, int, Networking::CNetMessageBuffer*, int)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // LUA_INTEGRATE


#endif // NETTHREAD_H
