//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Material System Material
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/ITexture.h"

struct MatVarData
{
	float			vector[16]{ 0 }; // TODO: think about memory consumption
	int				intValue;
	EqString		strValue;
	ITexturePtr		texture;
};

struct MaterialVarBlock : public WeakRefObject<MaterialVarBlock>
{
	Map<int, int>		variableMap{ PP_SL };
	Array<MatVarData>	variables{ PP_SL };
};

struct MV_VOID {};

template<typename T>
class MatVarProxy;

using MatVarProxyUnk = MatVarProxy<MV_VOID>;

template<typename T>
class MatVarProxy
{
	template<typename U>
	friend class MatVarProxy;

	friend class CMaterial;
	friend class CMaterialSystem;
public:
	MatVarProxy() = default;

	template<typename U>
	MatVarProxy(const MatVarProxy<U>& op) { m_matVarIdx = op.m_matVarIdx; m_vars = op.m_vars; }

	template<typename U>
	MatVarProxy(MatVarProxy<U>&& op) : m_matVarIdx(op.m_matVarIdx), m_vars(std::move(op.m_vars)) { }

	bool				IsValid() const { return m_vars; }

	void				Set(const T& value);
	const T&			Get() const;

	T*					GetArray();

	void				Purge();

	void				operator=(std::nullptr_t) { m_matVarIdx = -1; m_vars = nullptr; }

protected:
	MatVarProxy(int varIdx, MaterialVarBlock& owner); // only CMatVar can initialize us

	CWeakPtr<MaterialVarBlock>	m_vars;	// used only to track material existance
	int							m_matVarIdx{ -1 };
};

using MatTextureProxy = MatVarProxy<ITexturePtr>;
using MatStringProxy = MatVarProxy<EqString>;
using MatIntProxy = MatVarProxy<int>;
using MatFloatProxy = MatVarProxy<float>;
using MatVec2Proxy = MatVarProxy<Vector2D>;
using MatVec3Proxy = MatVarProxy<Vector3D>;
using MatVec4Proxy = MatVarProxy<Vector4D>;
using MatM3x3Proxy = MatVarProxy<Matrix3x3>;
using MatM4x4Proxy = MatVarProxy<Matrix4x4>;

class CTextureAtlas;

enum EMaterialFlags
{
	MATERIAL_FLAG_RECEIVESHADOWS	= (1 << 0),		// this material receives shadows
	MATERIAL_FLAG_CASTSHADOWS		= (1 << 1),		// this material occludes light
	MATERIAL_FLAG_INVISIBLE			= (1 << 2),		// invisible in standart scene mode. Shadows can be casted if MATERIAL_FLAG_CASTSHADOWS set
	MATERIAL_FLAG_NOCULL			= (1 << 3),		// no culling (two sided)

	MATERIAL_FLAG_SKINNED			= (1 << 4),		// this material can be applied on skinned mesh, using vertex shaders
	MATERIAL_FLAG_VERTEXBLEND		= (1 << 5),		// this material is uses vertex blending

	MATERIAL_FLAG_ALPHATESTED		= (1 << 6),		// has alphatesting
	MATERIAL_FLAG_TRANSPARENT		= (1 << 7),		// has transparency

	MATERIAL_FLAG_SKY				= (1 << 8),	// used for skybox
	MATERIAL_FLAG_DECAL				= (1 << 9),	// is decal shader (also enables polygon offset feature)
	MATERIAL_FLAG_WATER				= (1 << 10),	// this is water material

	MATERIAL_FLAG_TEXTRANSITION		= (1 << 11),	// transits textures to create painting effect (vertex transition)
};

enum EMaterialLoadingState
{
	MATERIAL_LOAD_ERROR		= -1,	// has error
	MATERIAL_LOAD_NEED_LOAD	= 0,	// needs loading
	MATERIAL_LOAD_OK,				// loading ok
	MATERIAL_LOAD_INQUEUE,			// is loading now
};

//---------------------------------------------------------------------------------

class IMaterial : public RefCountedObject<IMaterial, RefCountedKeepPolicy>, public WeakRefObject<IMaterial>
{
public:
	// returns full material path
	virtual const char*				GetName() const = 0;
	virtual const char*				GetShaderName() const = 0;

	// returns the atlas
	virtual CTextureAtlas*			GetAtlas() const = 0;
	
	// returns shader flags in short			
	virtual int						GetFlags() const = 0;

	// returns material loading state
	virtual int						GetState() const = 0;

	// loading error indicator
	virtual bool					IsError() const = 0;

// material var operations
	virtual MatVarProxyUnk			FindMaterialVar( const char* pszVarName ) const = 0;	// only searches for existing matvar

	// finds or creates material var
	virtual MatVarProxyUnk			GetMaterialVar( const char* pszVarName, const char* defaultValue = nullptr) = 0;

// render-time operations

	virtual bool					LoadShaderAndTextures() = 0;

	// waits for material loading
	virtual void					WaitForLoading() const = 0;

	// updates material proxies
	virtual void					UpdateProxy(float fDt) = 0;

	// retrieves the base texture on specified stage
	virtual const ITexturePtr&		GetBaseTexture(int stage = 0) = 0;

private:
	virtual MatVarData&				VarAt(int idx) const = 0;
};

using IMaterialPtr = CRefPtr<IMaterial>;