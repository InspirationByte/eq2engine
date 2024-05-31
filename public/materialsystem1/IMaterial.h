//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Material System Material
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/ITexture.h"
#include "renderers/IGPUBuffer.h"

class CTextureAtlas;

struct MatVarData
{
	float			vector[16]{ 0 }; // TODO: think about memory consumption
	int				intValue;
	EqString		strValue;
	ITexturePtr		texture;
	GPUBufferView	buffer;
};

struct MaterialVarBlock : public WeakRefObject<MaterialVarBlock>
{
	Map<int, int>		variableMap{ PP_SL };
	Array<MatVarData>	variables{ PP_SL };
};

struct EMPTY_BASES MV_VOID {};

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
	T*					GetArray() const;

	void				Purge();

	void				operator=(std::nullptr_t) { m_matVarIdx = -1; m_vars = nullptr; }

protected:
	MatVarProxy(int varIdx, MaterialVarBlock& owner); // only CMatVar can initialize us

	CWeakPtr<MaterialVarBlock>	m_vars;	// used only to track material existance
	int							m_matVarIdx{ -1 };
};

enum EMaterialFlags
{
	MATERIAL_FLAG_NO_CULL			= (1 << 0),		// no culling (two sided)
	MATERIAL_FLAG_WIREFRAME			= (1 << 1),
	MATERIAL_FLAG_NO_Z_TEST			= (1 << 2),
	MATERIAL_FLAG_NO_Z_WRITE		= (1 << 3),
	MATERIAL_FLAG_ONLY_Z			= (1 << 4),		// material is only used for Z test and write. No color map writes (and no fragment shader)

	MATERIAL_FLAG_RECEIVESHADOWS	= (1 << 5),		// this material receives shadows
	MATERIAL_FLAG_CASTSHADOWS		= (1 << 6),		// this material occludes light
	MATERIAL_FLAG_INVISIBLE			= (1 << 7),		// invisible in standart scene mode. Shadows can be casted if MATERIAL_FLAG_CASTSHADOWS set

	MATERIAL_FLAG_SKINNED			= (1 << 8),		// this material can be applied on skinned mesh, using vertex shaders
	MATERIAL_FLAG_VERTEXBLEND		= (1 << 9),		// this material is uses vertex blending

	MATERIAL_FLAG_ALPHATESTED		= (1 << 10),	// has alphatesting
	MATERIAL_FLAG_TRANSPARENT		= (1 << 11),	// has transparency

	MATERIAL_FLAG_SKY				= (1 << 12),	// used for skybox
	MATERIAL_FLAG_DECAL				= (1 << 13),	// is decal shader (also enables polygon offset feature)
	MATERIAL_FLAG_WATER				= (1 << 14),	// this is water material

	MATERIAL_FLAG_TEXTRANSITION		= (1 << 15),	// transits textures to create painting effect (vertex transition)
};

enum EMaterialLoadingState
{
	MATERIAL_LOAD_ERROR		= -1,	// has error
	MATERIAL_LOAD_NEED_LOAD	= 0,	// needs loading
	MATERIAL_LOAD_OK,				// loading ok
	MATERIAL_LOAD_INQUEUE,			// is loading now
};

//---------------------------------------------------------------------------------

class IMaterial 
	: public RefCountedObject<IMaterial>
	, public WeakRefObject<IMaterial>
{
public:
	// returns full material path
	virtual const char*				GetName() const = 0;
	virtual const char*				GetShaderName() const = 0;

	virtual CTextureAtlas*			GetAtlas() const = 0;		
	virtual int						GetFlags() const = 0;

	virtual int						GetState() const = 0;
	virtual bool					IsError() const = 0;

	// material var operations
	virtual MatVarProxyUnk			FindMaterialVar( const char* pszVarName ) const = 0;	// only searches for existing matvar
	virtual MatVarProxyUnk			GetMaterialVar( const char* pszVarName, const char* defaultValue = nullptr) = 0;
	virtual const MaterialVarBlock& GetMaterialVars() const = 0;

	virtual bool					LoadShaderAndTextures() = 0;
	virtual void					WaitForLoading() const = 0;

	virtual const ITexturePtr&		GetBaseTexture(int stage = 0) = 0;

private:
	virtual MatVarData&				VarAt(int idx) const = 0;
};

using IMaterialPtr = CRefPtr<IMaterial>;