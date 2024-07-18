//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium material
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/IMaterial.h"

class IMaterialProxy;
class IShaderAPI;
struct KVSection;

class CMaterial : public IMaterial
{
public:
	friend class			CBaseShader;
	friend class			CMaterialSystem;
	friend class			CEqMatSystemThreadedLoader;

							// constructor, destructor
							CMaterial(const char* materialName, int instanceFormatId, bool loadFromDisk);
							~CMaterial();

	void					Ref_DeleteObject();

	const char*				GetName() const { return m_szMaterialName.GetData(); }
	const char*				GetShaderName() const;
	CTextureAtlas*			GetAtlas() const;

	int						GetState() const {return m_state;}
	bool					IsError() const {return (m_state == MATERIAL_LOAD_ERROR);}
	int						GetFlags() const;

// init + shutdown

	// initializes material from file
	void					Init(IShaderAPI* renderAPI);
	
	// initializes material from keyvalues
	void					Init(IShaderAPI* renderAPI, const KVSection* shaderRoot);

	void					Cleanup(bool dropVars = true, bool dropShader = true);

	bool					LoadShaderAndTextures();
	void					WaitForLoading() const;

// material var operations
	MatVarProxyUnk			FindMaterialVar(const char* pszVarName) const;
	MatVarProxyUnk			GetMaterialVar(const char* pszVarName, const char* defaultValue);
	const MaterialVarBlock&	GetMaterialVars() const { return m_vars; }

// render-time operations
	void					UpdateProxy(float fDt, IGPUCommandRecorder* cmdRecorder, bool force = false);
	const ITexturePtr&		GetBaseTexture(int stage = 0);
private:

	void					InitVars(const KVSection* kvs, const char* renderAPIName);
	MatVarData&				VarAt(int idx) const;

	void					InitShader(IShaderAPI* renderAPI);
	void					InitMaterialVars(const KVSection* kvs, const char* prefix = nullptr);
	void					InitMaterialProxy(const KVSection* kvs);

protected:
	bool					DoLoadShaderAndTextures();
	void					OnVarUpdated();
	static void 			OnMatVarChanged(int varIdx, void* userData);

	EqString				m_szMaterialName;
	EqString				m_szShaderName;

	MaterialVarBlock		m_vars;
	Array<IMaterialProxy*>	m_proxies{ PP_SL };

	CTextureAtlas*			m_atlas{ nullptr };
	IMatSystemShader*		m_shader{ nullptr };

	int						m_state{ MATERIAL_LOAD_ERROR };	// FIXME: may be interlocked?
	int						m_nameHash{ 0 };
	int						m_instanceFormatId{ 0 };

	uint					m_frameBound{ 0 };
	bool					m_loadFromDisk{ false };
	bool					m_varsUpdated{ true };
};