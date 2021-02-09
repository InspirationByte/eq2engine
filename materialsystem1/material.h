//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium material
//////////////////////////////////////////////////////////////////////////////////

#ifndef CMATERIAL_H
#define CMATERIAL_H

//#include "DebugInterface.h"
#include "IMaterial.h"
#include "materialvar.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"
#include "utils/eqthread.h"

struct kvkeybase_t;
class IMaterialProxy;

class CMaterial : public IMaterial
{
public:
	friend class			CBaseShader;
	friend class			CMaterialSystem;

							// constructor, destructor
							CMaterial(Threading::CEqMutex& mutex);
							~CMaterial();

	const char*				GetName() const {return m_szMaterialName.GetData();}
	const char*				GetShaderName() const;
	CTextureAtlas*			GetAtlas() const;

	int						GetState() const {return m_state;}
	bool					IsError() const {return (m_state == MATERIAL_LOAD_ERROR);}
	int						GetFlags() const;

// init + shutdown

	// initializes material from file
	void					Init(const char* matFileName);
	
	// initializes material from keyvalues
	void					Init(const char* materialName, kvkeybase_t* shader_root);

	void					Cleanup(bool dropVars = true, bool dropShader = true);

	bool					LoadShaderAndTextures();
	void					WaitForLoading() const;

// material var operations
	IMatVar*				FindMaterialVar(const char* pszVarName) const;
	IMatVar*				GetMaterialVar(const char* pszVarName, const char* defaultParam);
	IMatVar*				CreateMaterialVar(const char* pszVarName, const char* defaultParam);
	void					RemoveMaterialVar(IMatVar* pVar);

// render-time operations
	void					UpdateProxy(float fDt);					
	ITexture*				GetBaseTexture(int stage = 0);

	void					Setup(uint paramMask);
private:

	void					InitVars(kvkeybase_t* kvs);

	void					InitShader();
	void					InitMaterialVars(kvkeybase_t* kvs);
	void					InitMaterialProxy(kvkeybase_t* kvs);

	virtual void			Ref_DeleteObject(); // empty for now

protected:
	IMaterialSystemShader*	m_shader;
	int						m_state;	// FIXME: may be interlocked?

	uint					m_frameBound;
	bool					m_proxyIsDirty;
	bool					m_loadFromDisk;

	DkList<CMatVar*>		m_variables;
	DkList<IMaterialProxy*>	m_proxies;
	CTextureAtlas*			m_atlas;

	EqString				m_szMaterialName;
	EqString				m_szShaderName;

	Threading::CEqMutex&	m_Mutex;
};


#endif //CMATERIAL_H
