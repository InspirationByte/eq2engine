//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine network thread
//				Provides event message sending
//////////////////////////////////////////////////////////////////////////////////

#ifndef NETTHREAD_H
#define NETTHREAD_H

#ifndef NO_LUA
#include "luabinding/LuaBinding.h"
#include "luabinding/LuaBinding_Engine.h"
#endif // NO_LUA

#include "utils/eqthread.h"
#include "utils/DkList.h"
#include "utils/eqtimer.h"

#include "network/c_udp.h"
#include "network/NetInterfaces.h"
#include "network/NetMessageBuffer.h"
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
typedef EventFilterResult_e (*pfnEventFilterCallback)( CNetworkThread* pNetThread, CNetMessageBuffer* pMsg, int nEventType );

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
	int							SendEvent( CNetEvent* pEvent, int nEventType, int client_id = -1, int flags = CDPSEND_IMMEDIATE);

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

#endif // NO_LUA
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

	CNetEvent*					CreateEvent( CNetMessageBuffer *pMsg, int eventIdentifier );

	bool						ProcessMessage( rcvdMessage_t* pMsg, CNetMessageBuffer* pOutput = NULL, int nWaitEventID = -1 );

	///< simply adds message
	void						AddMessage(rcvdMessage_t* pMessage, rcvdMessage_t* pAddTo);

	///< adds fragmented message to waiter
	void						AddFragmentedMessage( netMessage_t* pMsg, int size, const sockaddr_in& addr );

	///< dispatches fragmented message to the queue
	void						DispatchFragmentedMessage( netFragMsg_t* pMsg );

protected:

	// event factory
	DkList<neteventfactory_t>	m_netEventFactory;

	// network interface this thread using
	INetworkInterface*			m_netInterface;

	// delayed messages, for testing network only
	DkList<rcvdMessage_t*>		m_lateMessages;

	// all undispatched fragmented messages
	DkList<netFragMsg_t*>		m_fragmented_messages;

	// message queue
	rcvdMessage_t*				m_firstMessage;
	rcvdMessage_t*				m_lastMessage;

	// events
	DkList<sendEvent_t>			m_queuedEvents;

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

#endif // NO_LUA

}; // namespace Networking

#ifndef NO_LUA
#ifndef __INTELLISENSE__

OOLUA_PROXY( Networking::CNetMessageBuffer )

	OOLUA_MFUNC( WriteByte )
	OOLUA_MFUNC( WriteUByte )
	OOLUA_MFUNC( WriteInt16 )
	OOLUA_MFUNC( WriteUInt16 )
	OOLUA_MFUNC( WriteInt )
	OOLUA_MFUNC( WriteUInt )
	OOLUA_MFUNC( WriteBool )
	OOLUA_MFUNC( WriteFloat )

	OOLUA_MFUNC( WriteVector2D )
	OOLUA_MFUNC( WriteVector3D )
	OOLUA_MFUNC( WriteVector4D )

	OOLUA_MFUNC( ReadByte )
	OOLUA_MFUNC( ReadUByte )
	OOLUA_MFUNC( ReadInt16 )
	OOLUA_MFUNC( ReadUInt16 )
	OOLUA_MFUNC( ReadInt )
	OOLUA_MFUNC( ReadUInt )
	OOLUA_MFUNC( ReadBool )
	OOLUA_MFUNC( ReadFloat )

	OOLUA_MFUNC( ReadVector2D )
	OOLUA_MFUNC( ReadVector3D )
	OOLUA_MFUNC( ReadVector4D )

	OOLUA_MEM_FUNC( void, WriteString, const char* )
	OOLUA_MEM_FUNC( char*, ReadString, int& )

	OOLUA_MFUNC( WriteNetBuffer )

	OOLUA_MFUNC( WriteKeyValues )
	OOLUA_MFUNC( ReadKeyValues )

	OOLUA_MFUNC_CONST( GetMessageLength )
	OOLUA_MFUNC_CONST( GetClientID )

OOLUA_PROXY_END

OOLUA_PROXY(Networking::CNetworkThread)

	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(SendData)

	OOLUA_MEM_FUNC_RENAME(SendEvent, int, SendLuaEvent, OOLUA::Table&, int, int)
	OOLUA_MEM_FUNC_RENAME(SendWaitDataEvent, bool, SendLuaWaitDataEvent, OOLUA::Table&, int, Networking::CNetMessageBuffer*, int)
OOLUA_PROXY_END

#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // NETTHREAD_H
