//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game object
//////////////////////////////////////////////////////////////////////////////////

#include "GameObject.h"
#include "world.h"

// networking questions
#include "game_multiplayer.h"
#include "replay.h"

ConVar r_enableObjectsInstancing("r_enableObjectsInstancing", "1", "Use instancing on game objects");

void CBaseNetworkedObject::OnNetworkStateChanged(void* ptr)
{
	uint propOfs = (char*)ptr - ((char*)this);

	// TODO: find a prop and write it's index instead

	NETWORK_CHANGELIST(NetGame).append(propOfs);
	NETWORK_CHANGELIST(Replay).append(propOfs);
}

void CBaseNetworkedObject::OnNetworkStateChanged()
{
	Msg("changed some unchangeable\n");
}

void CBaseNetworkedObject::OnPackMessage( CNetMessageBuffer* buffer, DkList<int>& changeList)
{
	// packs all data
	netvariablemap_t* map = GetNetworkTableMap();
	while(map)
	{
		PackNetworkVariables(map, buffer, changeList);
		map = map->m_baseMap;
	};
}

void CBaseNetworkedObject::OnUnpackMessage( CNetMessageBuffer* buffer)
{
	// unpacks all data
	netvariablemap_t* map = GetNetworkTableMap();
	while(map)
	{
		UnpackNetworkVariables(map, buffer);
		map = map->m_baseMap;
	};
}

inline void PackNetworkVariables(void* object, const netvariablemap_t* map, IVirtualStream* stream, DkList<int>& changeList)
{
	if (map->m_numProps == 1 && map->m_props[0].size == 0)
		return;

	int numWrittenProps = 0;	
	
	int startPos = stream->Tell();
	stream->Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	for (int i = 0; i < map->m_numProps; i++)
	{
		const netprop_t& prop = map->m_props[i];

		if (changeList.numElem())
		{
			bool found = false;

			// PARANOID
			for (int j = 0; j < changeList.numElem(); j++)
			{
				if (changeList[j] == prop.offset)
				{
					found = true;
					break;
				}
			}

			if (!found)
				continue;
		}

		// write offset
		stream->Write(&prop.nameHash, 1, sizeof(int));

		char* varData = ((char*)object) + prop.offset;

		if (prop.type == NETPROP_NETPROP)
		{
			static DkList<int> _empty;
			PackNetworkVariables(varData, prop.nestedMap, stream, _empty);

			numWrittenProps++;

			continue;
		}

		// write data
		stream->Write(varData, 1, prop.size);
		numWrittenProps++;
	}

	long lastPos = stream->Tell();
	stream->Seek(startPos, VS_SEEK_SET);
	stream->Write(&numWrittenProps, 1, sizeof(numWrittenProps));

	stream->Seek(lastPos, VS_SEEK_SET);
}

inline void UnpackNetworkVariables(void* object, const netvariablemap_t* map, Networking::CNetMessageBuffer* buffer)
{
	if (map->m_numProps == 1 && map->m_props[0].size == 0)
		return;

	int numWrittenProps = buffer->ReadInt();

	for (int i = 0; i < numWrittenProps; i++)
	{
		netprop_t* found = nullptr;

		int nameHash = buffer->ReadInt();

		for (int j = 0; j < map->m_numProps; j++)
		{
			if (map->m_props[j].nameHash == nameHash)
			{
				found = &map->m_props[j];
				break;
			}
		}

		if (!found)
		{
			MsgError("invalid prop!\n");
			continue;
		}

		char* varData = ((char*)object) + found->offset;

		if (found->type == NETPROP_NETPROP)
		{
			UnpackNetworkVariables(varData, found->nestedMap, buffer);
			continue;
		}

		// write vars
		buffer->ReadData(varData, found->size);
	}
}


void CBaseNetworkedObject::PackNetworkVariables(const netvariablemap_t* map, CNetMessageBuffer* buffer, DkList<int>& changeList)
{
	CMemoryStream varsData;
	varsData.Open(nullptr, VS_OPEN_WRITE | VS_OPEN_READ, 2048);

	::PackNetworkVariables(this, map, &varsData, changeList);

	// write to net buffer
	buffer->WriteData(varsData.GetBasePointer(), varsData.Tell());
}

void CBaseNetworkedObject::UnpackNetworkVariables(const netvariablemap_t* map, CNetMessageBuffer* buffer)
{
	::UnpackNetworkVariables(this, map, buffer);
}

//--------------------------------------------------------------------------------------------------------------------------

BEGIN_NETWORK_TABLE_NO_BASE( CGameObject )
END_NETWORK_TABLE()

CGameObject::CGameObject() : CBaseNetworkedObject(), 
	m_state( GO_STATE_NOTSPAWN ),
	m_scriptID( SCRIPT_ID_NOTSCRIPTED ),
	m_networkID( NETWORK_ID_OFFLINE ),
	m_replayID( REPLAY_NOT_TRACKED ),
	m_objId( -1 )
{
	m_vecOrigin = vec3_zero;
	m_vecAngles = vec3_zero;
	m_pModel = NULL;

	m_name = "";
	m_defname = "";
	m_userData = NULL;
	m_keyValues = NULL;

	m_drawFlags = 0;

	m_worldMatrix = identity4();
	m_bodyGroupFlags = (1 << 0);
}

void CGameObject::Ref_DeleteObject()
{
	delete this;
}

void CGameObject::Precache()
{

}

// fast lua helpers
void CGameObject::L_Activate()
{
	// - You can't do that
	if(m_scriptID != SCRIPT_ID_NOTSCRIPTED)
		return;

#ifndef NO_LUA
	m_scriptID = g_pGameSession->GenScriptID();
#endif // NO_LUA
	Spawn();
	g_pGameWorld->AddObject( this );
}

void  CGameObject::L_Remove()
{
#ifndef EDITOR
	if (m_replayID == REPLAY_NOT_TRACKED || 
		m_replayID != REPLAY_NOT_TRACKED && g_replayTracker->m_state != REPL_PLAYING)
#endif // EDITOR
		m_state = GO_STATE_REMOVE_BY_SCRIPT;
}

void CGameObject::Spawn()
{
	
#ifndef EDITOR
	// call NET_SPAWN
	if((g_pGameSession->GetSessionType() == SESSION_NETWORK) &&
		g_pGameSession->IsServer() &&
		s_networkedObjectTypes[ObjType()] == true)
	{
		CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

		// initiate spawn object on clients
		// NOTE: duplicate static objects doesn't spawn

		netSes->Net_SpawnObject( this );

		//Msg("[SPAWN] Network spawn id=%d type=%s name='%s'\n", m_networkID, s_objectTypeNames[ObjType()], m_name.c_str());
	}
#endif // EDITOR

	m_state = GO_STATE_IDLE;
}

void CGameObject::OnRemove()
{
#ifndef NO_LUA
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if( m_luaOnRemove.Push() )
	{
		if(!m_luaOnRemove.Call(0, 0, 0))
		{
			MsgError("CGameObject:OnRemove error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}
	

#endif // NO_LUA

#ifndef EDITOR
	// call NET_REMOVE
	if((g_pGameSession->GetSessionType() == SESSION_NETWORK) &&
		g_pGameSession->IsServer() &&
		(m_networkID != NETWORK_ID_OFFLINE))
	{
		CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

		//Msg("[REMOVE] Network object remove id=%d\n", m_networkID);

		netSes->Net_RemoveObject( this );
	}
#endif // EDITOR

	m_state = GO_STATE_REMOVED;
}

void CGameObject::UpdateTransform()
{
	// refresh it's matrix
	m_worldMatrix = translate(m_vecOrigin)*rotateXYZ4(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z));
}

void CGameObject::Simulate( float fDt )
{
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if (m_luaOnSimulate.Push())
	{
		OOLUA::push(state, fDt);
		if (!m_luaOnSimulate.Call(1, 0, 0))
		{
			MsgError("CGameObject:OnSimulate error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}

	UpdateTransform();
}

void CGameObject::SetOrigin( const Vector3D& origin )
{
	m_vecOrigin = origin;
}

void CGameObject::SetAngles( const Vector3D& angles )
{
	m_vecAngles = angles;
}

void CGameObject::SetVelocity(const Vector3D& vel)
{
}

const Vector3D& CGameObject::GetOrigin() const
{
	return m_vecOrigin;
}

const Vector3D& CGameObject::GetAngles() const
{
	return m_vecAngles;
}

const Vector3D& CGameObject::GetVelocity() const
{
	static Vector3D s_vel_zero(0);
	return s_vel_zero;
}

void CGameObject::SetModel(const char* pszModelName)
{
	int nModelCacheId = g_studioModelCache->PrecacheModel( pszModelName );

	SetModelPtr(g_studioModelCache->GetModel( nModelCacheId ));
}

void CGameObject::SetModelPtr(IEqModel* modelPtr)
{
	m_pModel = modelPtr;
	
	if(!m_pModel)
		return;

	//studiohdr_t* pHdr = m_pModel->GetHWData()->studio;

	m_bodyGroupFlags = (1 << 0);
}

bool CGameObject::CheckVisibility( const occludingFrustum_t& frustum ) const
{
	if( !m_pModel )
		return false;

	if(!frustum.IsSphereVisible(GetOrigin(), length(m_pModel->GetAABB().maxPoint)))
		return false;

	return true;
}

void CGameObject::Draw( int nRenderFlags )
{
	if( !m_pModel )
		return;

	// if model has any instance - just don't render it in that way
	if(!(nRenderFlags & RFLAG_NOINSTANCE) && g_pShaderAPI->GetCaps().isInstancingSupported && m_pModel->GetInstancer())
	{
		if( m_pModel->GetInstancer()->HasInstances() )
			return;
	}

	DrawEGF(nRenderFlags, nullptr);
}

void CGameObject::DrawEGF(int nRenderFlags, Matrix4x4* boneTransforms, int materialGroup)
{
	materials->SetMatrix(MATRIXMODE_WORLD, m_worldMatrix);
	materials->SetCullMode((nRenderFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

	float camDist = g_pGameWorld->m_view.GetLODScaledDistFrom(GetOrigin());
	int nStartLOD = m_pModel->SelectLod(camDist); // lod distance check

	studiohdr_t* pHdr = m_pModel->GetHWData()->studio;
	for (int i = 0; i < pHdr->numBodyGroups; i++)
	{
		// check bodygroups for rendering
		if (!(m_bodyGroupFlags & (1 << i)))
			continue;

		int bodyGroupLOD = nStartLOD;
		int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
		studiolodmodel_t* lodModel = pHdr->pLodModel(nLodModelIdx);

		int nModDescId = lodModel->modelsIndexes[bodyGroupLOD];

		// get the right LOD model number
		while (nModDescId == -1 && bodyGroupLOD > 0)
		{
			bodyGroupLOD--;
			nModDescId = lodModel->modelsIndexes[bodyGroupLOD];
		}

		if (nModDescId == -1)
			continue;

		studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

		// render model groups that in this body group
		for (int j = 0; j < modDesc->numGroups; j++)
		{
			if(boneTransforms)
				materials->SetSkinningEnabled(true);

			int materialIndex = modDesc->pGroup(j)->materialIndex;
			materials->BindMaterial(m_pModel->GetMaterial(materialIndex, materialGroup), 0);

			m_pModel->PrepareForSkinning( boneTransforms );
			m_pModel->DrawGroup(nModDescId, j);

			materials->SetSkinningEnabled(false);
		}
	}
}

CGameObject* CGameObject::GetChildShadowCaster(int idx) const
{
	return NULL;
}

int CGameObject::GetChildCasterCount() const
{
	return 0;
}

void CGameObject::ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const
{
	conf.dist = 3.5f;
	conf.height = 1.0f;
	conf.distInCar = 0.0f;
	conf.widthInCar = 0.0f;
	conf.heightInCar = 0.5f;
	conf.fov = 70.0f;
}

void CGameObject::SetDrawFlags(ubyte newFlags)
{
	m_drawFlags = newFlags;
}

ubyte CGameObject::GetDrawFlags() const 
{
	return m_drawFlags;
}

void CGameObject::SetBodyGroups(ubyte newBodyGroup)
{
	m_bodyGroupFlags = newBodyGroup;
}

ubyte CGameObject::GetBodyGroups() const
{ 
	return m_bodyGroupFlags; 
}

void CGameObject::SetUserData(void* dataPtr)
{
	m_userData = dataPtr;
}

void* CGameObject::GetUserData() const
{
	return m_userData;
}

void CGameObject::SetName(const char* pszName)
{
	m_name = pszName;
}

const char* CGameObject::GetName() const
{
	return m_name.c_str();
}

const char* CGameObject::GetDefName() const
{
	return m_defname.c_str();
}

void CGameObject::OnPackMessage( CNetMessageBuffer* buffer, DkList<int>& changeList)
{
	BaseClass::OnPackMessage(buffer, changeList);
	//buffer->WriteString(m_defname);

#ifndef NO_LUA
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if( m_luaOnPackMessage.Push() )
	{
		OOLUA::push(state, buffer);
	
		if(!m_luaOnPackMessage.Call(1, 0, 0))
		{
			MsgError("CGameObject:OnPackMessage error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}
#endif // NO_LUA
}

void CGameObject::OnUnpackMessage( CNetMessageBuffer* buffer )
{
	BaseClass::OnUnpackMessage(buffer);
	//m_defname = buffer->ReadString();

#ifndef NO_LUA
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if( m_luaOnUnpackMessage.Push() )
	{
		OOLUA::push(state, buffer);
	
		if(!m_luaOnUnpackMessage.Call(1, 0, 0))
		{
			MsgError("CGameObject:OnUnpackMessage error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}
#endif // NO_LUA
}

void CGameObject::OnPhysicsFrame(float fDt)
{
}

#ifndef NO_LUA
void CGameObject::L_RegisterEventHandler(const OOLUA::Table& tableRef)
{
	m_luaEvtHandler = tableRef;
	m_luaOnCarCollision.Get(m_luaEvtHandler, "OnCarCollision", true);
	m_luaOnRemove.Get(m_luaEvtHandler, "OnRemove", true);
	m_luaOnSimulate.Get(m_luaEvtHandler, "OnSimulate", true);
}

OOLUA::Table& CGameObject::L_GetEventHandler() const
{
	return (OOLUA::Table&)m_luaEvtHandler;
}

#endif // NO_LUA

void CGameObject::OnCarCollisionEvent( const CollisionPairData_t& pair, CGameObject* hitBy )
{
#ifndef NO_LUA
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if( m_luaOnCarCollision.Push() )
	{
		OOLUA::Table tab = OOLUA::new_table(state);

		// why i can't push them as vector3d?
		//tab.set("normal", (Vector3D)pair.normal);
		//tab.set("position", (Vector3D)pair.position);
		tab.set("impulse", pair.appliedImpulse);
		tab.set("velocity", pair.impactVelocity);
		tab.set("hitBy", hitBy);

		OOLUA::push(state, tab);
	
		if(!m_luaOnCarCollision.Call(1, 0, 0))
		{
			MsgError("CGameObject:OnCarCollision error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}
#endif // NO_LUA
}

#ifndef NO_LUA

OOLUA_EXPORT_FUNCTIONS(CGameObject,
	Spawn, Remove,
	SetModel,
	SetName,
	SetOrigin, SetAngles, SetVelocity,
	SetDrawFlags,
	SetBodyGroups,
	SetContents, SetCollideMask, 
	SetEventHandler)

OOLUA_EXPORT_FUNCTIONS_CONST(CGameObject, 
	GetName,
	GetOrigin, GetAngles, GetVelocity,
	GetDrawFlags,
	GetBodyGroups,
	GetScriptID, GetType, 
	GetContents, GetCollideMask,
	GetEventHandler)

#endif // NO_LUA