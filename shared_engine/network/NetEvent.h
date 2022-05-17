#ifndef NETEVENT_H
#define NETEVENT_H

namespace Networking
{

class CNetworkThread;
class Buffer;

//-----------------------------------------------------
// Network events
//-----------------------------------------------------

// The network event.
class CNetEvent
{
public:
	virtual ~CNetEvent() {}

	virtual void	Process( CNetworkThread* pNetThread ) = 0;

	virtual void	Unpack( CNetworkThread* pNetThread, Buffer* pStream ) = 0;
	virtual void	Pack( CNetworkThread* pNetThread, Buffer* pStream ) = 0;

	// standard message handlers

	// delivery completely failed after retries: if returns false, this removes message
	virtual bool	OnDeliveryFailed() {return false;}

	// this was made for send safety check
	virtual int		GetEventType() const = 0;

	void			SetID(int nId) {m_nEventID = nId;}
	int				GetID() const {return m_nEventID;}

protected:
	int				m_nEventID;
};

// event factory
// 22-04-14: added eventID to pass on a lua function
typedef CNetEvent* (*pfnNetEventFactory)(CNetworkThread* pNetThread, Buffer* msg);

struct neteventfactory_t
{
	int					eventIdent;
	pfnNetEventFactory	factory;
};

//-------------------------------------------------------------------------------------

#define NETEVENT_LIST(name) g_netevents_##name##list

#define DECLARE_NETEVENT_LIST(name) Array<neteventfactory_t> NETEVENT_LIST(name){ PP_SL }
#define EXTERN_NETEVENT_LIST(name) extern DECLARE_NETEVENT_LIST(name)

//-------------------------------------------------------------------------------------

// event registrator
#define BEGIN_DECLARE_NETEVENT( eventtype, classname )							\
	class C##classname##_Registrator											\
	{																			\
	public:																		\
		typedef C##classname##_Registrator	this_class;							\
		C##classname##_Registrator() {m_eventType = eventtype; Register();}		\
		static CNetEvent* Factory(CNetworkThread*, Buffer*)			\
		{ return (CNetEvent*)new classname(); }									\
		void Register();														\
		int m_eventType;														\
	};																			\
	inline void C##classname##_Registrator::Register() {						\

// for multiple use
#define REGISTER_THREAD_NETEVENT( evlist ) {neteventfactory_t fac = {m_eventType, &this_class::Factory}; NETEVENT_LIST(evlist).append(fac);}

#define END_NETEVENT( classname ) } static C##classname##_Registrator g_##classname##_Registrator;

// single event registrator
#define DECLARE_NETEVENT( eventtype, classname, evlist)	\
	BEGIN_DECLARE_NETEVENT(eventtype, classname)		\
	REGISTER_THREAD_NETEVENT(evlist)					\
	END_NETEVENT(classname)

}; // namespace Networking

#endif // NETEVENT_H
