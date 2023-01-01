//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Material System Material
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class ITexture;
class CTextureAtlas;

struct MatVarData
{
	EqString		pszValue;
	Vector4D		vector{ 0 };
	int				intValue{ 0 };
	ITexture*		texture{ nullptr };
};

// WARNING: modifying this you must recompile all engine!
enum MaterialFlags_e
{
	MATERIAL_FLAG_BASETEXTURE_CUR	= (1 << 0),		// use the currently set up texture in ShaderAPI as baseTexture

	MATERIAL_FLAG_ISSKY				= (1 << 1),		// used for skybox
	MATERIAL_FLAG_INVISIBLE			= (1 << 2),		// invisible in standart scene mode. Shadows can be casted if MATERIAL_FLAG_CASTSHADOWS set

	MATERIAL_FLAG_ALPHATESTED		= (1 << 3),		// has alphatesting along with other transparency modes

	MATERIAL_FLAG_TRANSPARENT		= (1 << 4),		// has transparency
	MATERIAL_FLAG_ADDITIVE			= (1 << 5),		// additive transparency
	MATERIAL_FLAG_MODULATE			= (1 << 6),		// additive transparency

	MATERIAL_FLAG_NOCULL			= (1 << 7),		// no culling (two sided)

	MATERIAL_FLAG_DECAL				= (1 << 8),		// is decal shader (also enables polygon offset feature)

	MATERIAL_FLAG_SKINNED			= (1 << 9),		// this material can be applied on skinned mesh, using vertex shaders
	MATERIAL_FLAG_VERTEXBLEND		= (1 << 10),	// this material is uses vertex blending

	MATERIAL_FLAG_CASTSHADOWS		= (1 << 11),	// this material occludes light
	MATERIAL_FLAG_RECEIVESHADOWS	= (1 << 12),	// this material receives shadows

	MATERIAL_FLAG_ISTEXTRANSITION	= (1 << 13),	// transits textures to create painting effect (vertex transition)
	MATERIAL_FLAG_HASBUMPTEXTURE	= (1 << 14),	// has bumpmap texture
	MATERIAL_FLAG_HASCUBEMAP		= (1 << 15),	// has cubemap texture

	MATERIAL_FLAG_USE_ENVCUBEMAP	= (1 << 16),	// cubemap is $env_cubemap

	MATERIAL_FLAG_WATER				= (1 << 17),	// this is water material
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
	friend class MatVarProxy;

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
	virtual MatVarProxy				FindMaterialVar( const char* pszVarName ) const = 0;	// only searches for existing matvar

	// finds or creates material var
	virtual MatVarProxy				GetMaterialVar( const char* pszVarName, const char* defaultValue) = 0;

// render-time operations

	virtual bool					LoadShaderAndTextures() = 0;

	// waits for material loading
	virtual void					WaitForLoading() const = 0;

	// updates material proxies
	virtual void					UpdateProxy(float fDt) = 0;

	// retrieves the base texture on specified stage
	virtual ITexture*				GetBaseTexture(int stage = 0) = 0;

private:
	virtual MatVarData&				VarAt(int idx) const = 0;
};

using IMaterialPtr = CRefPtr<IMaterial>;