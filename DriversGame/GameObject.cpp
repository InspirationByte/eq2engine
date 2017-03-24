//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
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
	m_isNetworkStateChanged = true;
}

void CBaseNetworkedObject::OnPackMessage( CNetMessageBuffer* buffer )
{
	// packs all data
	netvariablemap_t* map = GetNetworkTableMap();
	while(map)
	{
		PackNetworkVariables(map, buffer);
		map = map->m_baseMap;
	};
}

void CBaseNetworkedObject::OnUnpackMessage( CNetMessageBuffer* buffer )
{
	// unpacks all data
	netvariablemap_t* map = GetNetworkTableMap();
	while(map)
	{
		UnpackNetworkVariables(map, buffer);
		map = map->m_baseMap;
	};
}

void CBaseNetworkedObject::PackNetworkVariables(const netvariablemap_t* map, CNetMessageBuffer* buffer)
{
	if(map->m_numProps == 1 && map->m_props[0].size == 0)
		return;

	// debug only
	//buffer->WriteString(map->m_mapName);

	for(int i = 0; i < map->m_numProps; i++)
	{
		const netprop_t& prop = map->m_props[i];
		if(prop.size == 0)
			continue;
	
		// write vars
		char* varData = ((char*)this)+prop.offset;

		buffer->WriteData( varData, prop.size ); // TODO: special rules for strings
	}
}

void CBaseNetworkedObject::UnpackNetworkVariables(const netvariablemap_t* map, CNetMessageBuffer* buffer)
{
	// first, write the map name (DEBUG)
	if(map->m_numProps == 1 && map->m_props[0].size == 0)
		return;

	//EqString mapName = buffer->ReadString();

	for(int i = 0; i < map->m_numProps; i++)
	{
		const netprop_t& prop = map->m_props[i];
		if(prop.size == 0)
			continue;
	
		// write vars
		char* varData = ((char*)this)+prop.offset;
		buffer->ReadData( varData, prop.size );
	}
}

//--------------------------------------------------------------------------------------------------------------------------

BEGIN_NETWORK_TABLE_NO_BASE( CGameObject )

END_NETWORK_TABLE()

CGameObject::CGameObject() : CBaseNetworkedObject(), 
	m_state( GO_STATE_NOTSPAWN ),
	m_scriptID( SCRIPT_ID_NOTSCRIPTED ),
	m_networkID( NETWORK_ID_OFFLINE ),
	m_replayID( REPLAY_NOT_TRACKED )
{
	m_vecOrigin = vec3_zero;
	m_vecAngles = vec3_zero;
	m_pModel = NULL;

	m_name = "";
	m_defname = "";
	m_userData = NULL;
	m_keyValues = NULL;

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
	g_pGameWorld->AddObject( this, true );
}

void  CGameObject::Remove()
{
	m_state = GO_STATE_REMOVE;
}

void  CGameObject::L_Remove()
{
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
		if(!m_luaOnRemove.Call(1, 0, 0))
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

void CGameObject::Simulate( float fDt )
{
	// refresh it's matrix
	m_worldMatrix = translate(m_vecOrigin)*rotateXYZ4(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z));
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
	int nModelCacheId = g_pModelCache->PrecacheModel( pszModelName );

	SetModelPtr(g_pModelCache->GetModel( nModelCacheId ));
}

void CGameObject::SetModelPtr(IEqModel* modelPtr)
{
	m_pModel = modelPtr;
	
	if(!m_pModel)
		return;

	//studiohdr_t* pHdr = m_pModel->GetHWData()->pStudioHdr;

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

	materials->SetMatrix(MATRIXMODE_WORLD, m_worldMatrix);
	materials->SetCullMode((nRenderFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

	float camDist = g_pGameWorld->m_view.GetLODScaledDistFrom( GetOrigin() );
	int nStartLOD = m_pModel->SelectLod( camDist ); // lod distance check

	studiohdr_t* pHdr = m_pModel->GetHWData()->pStudioHdr;
	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		// check bodygroups for rendering
		if(!(m_bodyGroupFlags & (1 << i)))
			continue;

		int bodyGroupLOD = nStartLOD;
		int nLodModelIdx = pHdr->pBodyGroups(i)->lodmodel_index;
		studiolodmodel_t* lodModel = pHdr->pLodModel(nLodModelIdx);

		int nModDescId = lodModel->lodmodels[ bodyGroupLOD ];

		// get the right LOD model number
		while(nModDescId == -1 && bodyGroupLOD > 0)
		{
			bodyGroupLOD--;
			nModDescId = lodModel->lodmodels[ bodyGroupLOD ];
		}

		if(nModDescId == -1)
			continue;
	
		studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

		// render model groups that in this body group
		for(int j = 0; j < modDesc->numgroups; j++)
		{
			//materials->SetSkinningEnabled(true);

			int materialIndex = modDesc->pGroup(j)->materialIndex;
			materials->BindMaterial( m_pModel->GetMaterial(materialIndex) , false);

			//m_pModel->PrepareForSkinning( m_BoneMatrixList );
			m_pModel->DrawGroup( nModDescId, j );

			//materials->SetSkinningEnabled(false);
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

void CGameObject::OnPackMessage( CNetMessageBuffer* buffer )
{
	BaseClass::OnPackMessage(buffer);
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
}

OOLUA::Table& CGameObject::L_GetEventHandler()
{
	return m_luaEvtHandler;
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

OOLUA_EXPORT_FUNCTIONS(CGameObject,SetName, Spawn, Remove, SetModel, SetOrigin, SetAngles, SetVelocity, SetContents, SetCollideMask, SetEventHandler, GetEventHandler)
OOLUA_EXPORT_FUNCTIONS_CONST(CGameObject, GetName, GetOrigin, GetAngles, GetVelocity, GetScriptID, GetType, GetContents, GetCollideMask)

#endif // NO_LUA