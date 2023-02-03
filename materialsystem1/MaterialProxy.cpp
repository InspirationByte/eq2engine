//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Standard proxy arithmetics - add, multiply, subtract, divide
// Standard functions - sin, cos
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "materialsystem1/IMaterialSystem.h"
#include "MaterialProxy.h"

//---------------------------------------------------------------------
// ADD proxy
//---------------------------------------------------------------------
class CAddProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, KVSection* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;

		if(pKeyBase->values.numElem() < 3)
		{
			MsgError("'add' proxy in '%s' error: invalid argument count\n Usage: add v1 v2 out [options]\n", pAssignedMaterial->GetName());
			return;
		}

		ParseVariable(in1, KV_GetValueString(pKeyBase, 0) );
		ParseVariable(in2, KV_GetValueString(pKeyBase, 1) );
		ParseVariable(out, KV_GetValueString(pKeyBase, 2) );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(KV_GetValueString(pKeyBase, i), "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in1, dt);
		UpdateVar(in2, dt);

		if(useFrameTime)
			mvSetValue(out, in1.value + in2.value*dt);
		else
			mvSetValue(out, in1.value + in2.value);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};

DECLARE_PROXY(add, CAddProxy)

//---------------------------------------------------------------------
// SUBTRACT proxy
//---------------------------------------------------------------------
class CSubProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, KVSection* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;

		if(pKeyBase->values.numElem() < 3)
		{
			MsgError("'sub' proxy in '%s' error: invalid argument count\n Usage: sub v1 v2 out [options]\n", pAssignedMaterial->GetName());
			return;
		}

		ParseVariable(in1, KV_GetValueString(pKeyBase, 0) );
		ParseVariable(in2, KV_GetValueString(pKeyBase, 1) );
		ParseVariable(out, KV_GetValueString(pKeyBase, 2) );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(KV_GetValueString(pKeyBase, i), "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in1, dt);
		UpdateVar(in2, dt);

		if(useFrameTime)
			mvSetValue(out, in1.value - in2.value*dt);
		else
			mvSetValue(out, in1.value - in2.value);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};

DECLARE_PROXY(subtract, CSubProxy)

//---------------------------------------------------------------------
// MULTIPLY proxy
//---------------------------------------------------------------------
class CMulProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, KVSection* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;

		if(pKeyBase->values.numElem() < 3)
		{
			MsgError("'mul' proxy in '%s' error: invalid argument count\n Usage: mul v1 v2 out [options]\n", pAssignedMaterial->GetName());
			return;
		}

		ParseVariable(in1, KV_GetValueString(pKeyBase, 0) );
		ParseVariable(in2, KV_GetValueString(pKeyBase, 1) );
		ParseVariable(out, KV_GetValueString(pKeyBase, 2) );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(KV_GetValueString(pKeyBase, i), "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in1, dt);
		UpdateVar(in2, dt);

		if(useFrameTime)
			mvSetValue(out, in1.value * in2.value*dt);
		else
			mvSetValue(out, in1.value * in2.value);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};

DECLARE_PROXY(multiply, CMulProxy)


//---------------------------------------------------------------------
// DIVIDE proxy
//---------------------------------------------------------------------
class CDivProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, KVSection* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;

		if(pKeyBase->values.numElem() < 3)
		{
			MsgError("'div' proxy in '%s' error: invalid argument count\n Usage: div v1 v2 out [options]\n", pAssignedMaterial->GetName());
			return;
		}

		ParseVariable(in1, KV_GetValueString(pKeyBase, 0) );
		ParseVariable(in2, KV_GetValueString(pKeyBase, 1) );
		ParseVariable(out, KV_GetValueString(pKeyBase, 2) );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(KV_GetValueString(pKeyBase, i), "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in1, dt);
		UpdateVar(in2, dt);

		if(useFrameTime)
			mvSetValue(out, in1.value / in2.value*dt);
		else
			mvSetValue(out, in1.value / in2.value);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};
DECLARE_PROXY(divide, CDivProxy)


//---------------------------------------------------------------------
// SINE proxy
//---------------------------------------------------------------------
class CSinProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, KVSection* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;
		if(pKeyBase->values.numElem() < 2)
		{
			MsgError("'sin' proxy in '%s' error: invalid argument count\n Usage: sin v1 out [options]\n", pAssignedMaterial->GetName());
			return;
		}

		ParseVariable(in, KV_GetValueString(pKeyBase, 0) );
		ParseVariable(out, KV_GetValueString(pKeyBase, 1) );
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in, dt);

		mvSetValue(out, sin(in.value));
	}
private:
	proxyvar_t in;

	proxyvar_t out;
};

DECLARE_PROXY(sin, CSinProxy)

//---------------------------------------------------------------------
// ABS proxy
//---------------------------------------------------------------------
class CAbsProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, KVSection* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;
		if(pKeyBase->values.numElem() < 2)
		{
			MsgError("'abs' proxy in '%s' error: invalid argument count\n Usage: abs v1 out [options]\n", pAssignedMaterial->GetName());
			return;
		}

		ParseVariable(in, KV_GetValueString(pKeyBase, 0) );
		ParseVariable(out, KV_GetValueString(pKeyBase, 1) );
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in, dt);

		mvSetValue(out, fabs(in.value));
	}
private:
	proxyvar_t in;

	proxyvar_t out;
};

DECLARE_PROXY(abs, CAbsProxy)

//---------------------------------------------------------------------
// ANIMATEDTEXTURE proxy
//---------------------------------------------------------------------
class CAnimatedTextureProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, KVSection* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;
		KVSection* pair = nullptr;

		// frame count is the only static variable, frame rate is dynamic
		frameCount = KV_GetValueInt(pKeyBase);

		pair = pKeyBase->FindSection("framerate");
		ParseVariable(frameRate, (char*)KV_GetValueString(pair));

		pair = pKeyBase->FindSection("frameVar");
		ParseVariable(out,(char*)KV_GetValueString(pair));

		time = 0.0f;
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(frameRate, dt);
		UpdateVar(out, dt);

		time = fmodf(time + dt * frameRate.value,  frameCount);

		mvSetValueInt( out, time );
	}
private:
	float		time;

	proxyvar_t	frameRate;
	int			frameCount;

	proxyvar_t	out;
};

DECLARE_PROXY(animatedtexture, CAnimatedTextureProxy)

//---------------------------------------------------------------------
// standard proxy initialization
//---------------------------------------------------------------------
void InitStandardMaterialProxies()
{
	REGISTER_PROXY(add, CAddProxy);
	REGISTER_PROXY(subtract, CSubProxy);
	REGISTER_PROXY(multiply, CMulProxy);
	REGISTER_PROXY(divide, CDivProxy);
	REGISTER_PROXY(sin, CSinProxy);
	REGISTER_PROXY(abs, CAbsProxy);
	REGISTER_PROXY(animatedtexture, CAnimatedTextureProxy);
}