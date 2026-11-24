//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: RPC protocols: named methods, remote calls and notifies
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "RPCEndpoint.h"
#include "RPCProtocol.h"

const KVSection& RPCResponse::GetResult() const
{
	static const KVSection s_emptySection;
	if (!message)
		return s_emptySection;

	const KVSection* result = message->root.FindSection("result");
	return result ? *result : s_emptySection;
}

//-------------------------------------------------------------------------------------------------

bool RPCDeferredReply::IsValid() const
{
	const RPCEndpoint* endpoint = m_endpoint.Ptr();
	if (!endpoint || !m_callId)
		return false;

	const RPCEndpoint::Connection* conn = endpoint->FindConnection(m_conn);
	return conn && conn->deferredCalls.contains(m_callId);
}

void RPCDeferredReply::Reply(const KVSection& result, IFileStreamPtr payload) const
{
	if (!IsValid())
		return;

	RPCEndpoint* endpoint = m_endpoint.Ptr();
	endpoint->FindConnection(m_conn)->deferredCalls.remove(m_callId);
	endpoint->SendReply(m_conn, m_callId, &result, 0, nullptr, payload);
}

void RPCDeferredReply::ReplyError(int code, const char* message) const
{
	if (!IsValid())
		return;

	RPCEndpoint* endpoint = m_endpoint.Ptr();
	endpoint->FindConnection(m_conn)->deferredCalls.remove(m_callId);
	endpoint->SendReply(m_conn, m_callId, nullptr, code, message, nullptr);
}

//-------------------------------------------------------------------------------------------------

RPCCallContext::RPCCallContext(RPCEndpoint& endpoint, NetConnectionId conn, uint callId)
	: m_endpoint(endpoint)
	, m_conn(conn)
	, m_callId(callId)
{
}

void RPCCallContext::Reply(const KVSection& result, IFileStreamPtr payload)
{
	if (m_answered || IsNotify())
		return;

	m_answered = true;
	m_endpoint.SendReply(m_conn, m_callId, &result, 0, nullptr, payload);
}

void RPCCallContext::ReplyError(int code, const char* message)
{
	if (m_answered || IsNotify())
		return;

	m_answered = true;
	m_endpoint.SendReply(m_conn, m_callId, nullptr, code, message, nullptr);
}

RPCDeferredReply RPCCallContext::Defer()
{
	RPCDeferredReply reply;
	if (m_answered || IsNotify())
		return reply;

	RPCEndpoint::Connection* conn = m_endpoint.FindConnection(m_conn);
	if (!conn)
		return reply;

	m_answered = true;
	conn->deferredCalls.insert(m_callId, true);

	reply.m_endpoint.Assign(&m_endpoint);
	reply.m_conn = m_conn;
	reply.m_callId = m_callId;
	return reply;
}

//-------------------------------------------------------------------------------------------------

RPCProtocol::RPCProtocol(const char* name, int version)
	: m_name(name)
	, m_version(version)
{
}

RPCProtocol::~RPCProtocol()
{
	if (m_endpoint)
		m_endpoint->RemoveProtocol(*this);
}

RPCProtocol& RPCProtocol::Method(const char* name, RPCMethodFunc func)
{
	const int nameHash = StringId(name);
	ASSERT_MSG(!m_methods.contains(nameHash), "RPCProtocol '%s' already has method '%s'", m_name.ToCString(), name);

	m_methods.insert(nameHash, func);
	return *this;
}

const RPCMethodFunc* RPCProtocol::FindMethod(const char* name) const
{
	auto it = m_methods.find(StringId(name));
	if (it.atEnd())
		return nullptr;
	return &it.value();
}

Future<RPCResponse> RPCProtocol::RemoteCall(const char* method, const KVSection& params, IFileStreamPtr payload)
{
	return RemoteCall(NET_CONNECTION_NONE, method, params, payload);
}

Future<RPCResponse> RPCProtocol::RemoteCall(NetConnectionId conn, const char* method, const KVSection& params, IFileStreamPtr payload)
{
	if (!m_endpoint)
		return Future<RPCResponse>::Failure(RPC_ERROR_NOT_CONNECTED, "protocol is not added to an endpoint");

	return m_endpoint->Call(*this, conn, method, params, payload);
}

void RPCProtocol::RemoteNotify(const char* method, const KVSection& params, IFileStreamPtr payload)
{
	RemoteNotify(NET_CONNECTION_NONE, method, params, payload);
}

void RPCProtocol::RemoteNotify(NetConnectionId conn, const char* method, const KVSection& params, IFileStreamPtr payload)
{
	if (!m_endpoint)
		return;

	m_endpoint->Notify(*this, conn, method, params, payload);
}
