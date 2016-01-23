//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech material
//////////////////////////////////////////////////////////////////////////////////

#ifndef CMATERIAL_H
#define CMATERIAL_H

//#include "DebugInterface.h"
#include "materialsystem/IMaterial.h"
#include "materialvar.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"

class CMaterial : public IMaterial
{
public:
	friend class			CBaseShader;
	friend class			CMaterialSystem;

							// constructor, destructor
							CMaterial();
							~CMaterial();

	const char*				GetName() const {return m_szMaterialName.GetData();}
	const char*				GetShaderName() const;
	int						GetState() const {return m_state;}
	bool					IsError() const {return (m_state == MATERIAL_LOAD_ERROR);}
	int						GetFlags() const;

// init + shutdown
	void					Init(const char* szFileName_noExt, bool flushMatVarsOnly = false);
	void					Cleanup(bool bUnloadShaders = true, bool bUnloadTextures = true, bool keepMaterialVars = false);

	bool					LoadShaderAndTextures();
	void					WaitForLoading() const;

	void					InitVars(bool flushOnly = false);
	void					InitMaterialProxy(kvkeybase_t* pProxyData);

// material var operations
	IMatVar*				FindMaterialVar(const char* pszVarName) const;
	IMatVar*				GetMaterialVar(const char* pszVarName, const char* defaultParam);
	IMatVar*				CreateMaterialVar(const char* pszVarName);
	void					RemoveMaterialVar(IMatVar* pVar);

// render-time operations
	void					UpdateProxy(float fDt);					
	ITexture*				GetBaseTexture(int stage = 0);

	void					Setup();
private:

	virtual void			Ref_DeleteObject(); // empty for now

protected:
	IMaterialSystemShader*	m_pShader;
	int						m_state;	// FIXME: may be interlocked?

	uint					m_frameBound;
	bool					m_proxyIsDirty;

	DkList<CMatVar*>		m_hMatVars;
	DkList<IMaterialProxy*>	m_hMatProxies;

	EqString				m_szMaterialName;
	EqString				m_szShaderName;
};


#endif //CMATERIAL_H
