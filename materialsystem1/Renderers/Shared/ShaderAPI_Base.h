//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				For high level look for the material system (Engine)
//				Contains FFP functions and Shader (FFP overriding programs)
//
//				The renderer may do anti-wallhacking functions
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IShaderAPI.h"

class ConCommandBase;

class ShaderAPI_Base : public IShaderAPI
{
public:
	ShaderAPI_Base();

	// Init + Shurdown
	virtual void			Init( const ShaderAPIParams &params );
	virtual void			Shutdown();

	const ShaderAPIParams&	GetParams() const { return m_params; }
	const ShaderAPICapabilities&	GetCaps() const {return m_caps;}
	virtual EShaderAPIType	GetShaderAPIClass() const {return SHADERAPI_EMPTY;}

	static void				GetConsoleTextureList(const ConCommandBase* base, Array<EqString>&, const char* query);

	// texture uploading frequency
	void					SetProgressiveTextureFrequency(int frames);
	int						GetProgressiveTextureFrequency() const;

	void					SubmitCommandBuffer(const IGPUCommandBuffer* cmdBuffer) const;
	Future<bool>			SubmitCommandBufferAwaitable(const IGPUCommandBuffer* cmdBuffer) const;

//-------------------------------------------------------------
// Renderer statistics
//-------------------------------------------------------------

	int						GetDrawCallsCount() const;
	int						GetDrawIndexedPrimitiveCallsCount() const;
	int						GetTrianglesCount() const;
	void					ResetCounters();

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// prepares for async operation (required to be called in main thread)
	virtual void			BeginAsyncOperation( uintptr_t threadId ) {}	// obsolete for D3D

	// completes for async operation (must be called in worker thread)
	virtual void			EndAsyncOperation() {}							// obsolete for D3D

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// creates texture from image array, used in LoadTexture, common use only
	ITexturePtr				CreateTexture(const ArrayCRef<CImagePtr>& pImages, const SamplerStateParams& sampler, int nFlags = 0);

	// creates procedural (lockable) texture
	ITexturePtr				CreateProceduralTexture(const char* pszName,
													ETextureFormat nFormat,
													int width, int height,
													int arraySize = 1,
													const SamplerStateParams& sampler = {},
													int flags = 0,
													int dataSize = 0, const ubyte* data = nullptr
													);

	// Finds texture by name
	ITexturePtr				FindTexture(const char* pszName);

	// Searches for existing texture or creates new one. Use this for resource loading
	ITexturePtr				FindOrCreateTexture( const char* pszName, bool& justCreated);

	// Unload the texture and free the memory
	void					FreeTexture(ITexture* pTexture);

//-------------------------------------------------------------
// Vertex format
//-------------------------------------------------------------

	IVertexFormat*			FindVertexFormat(const char* name) const;
	IVertexFormat*			FindVertexFormatById(int nameHash) const;

protected:

	virtual ITexturePtr		CreateTextureResource(const char* pszName) = 0;

	// NOTE: not thread-safe, used for error checks
	ITexture*				FindTexturePtr(int nameHash) const;

//-------------------------------------------------------------
// Useful data
//-------------------------------------------------------------

	ShaderAPIParams			m_params;
	ShaderAPICapabilities			m_caps;

	Map<int, ITexture*>		m_TextureList{ PP_SL };
	Array<IVertexFormat*>	m_VFList{ PP_SL };

	int						m_progressiveTextureFrequency{ 0 };

	int						m_statDrawIndexedCount{ 0 };
	int						m_statDrawCount{ 0 };
	int						m_statPrimitiveCount{ 0 };
};

#ifdef _RETAIL

#define CHECK_TEXTURE_ALREADY_ADDED(texture) 

#else

#define CHECK_TEXTURE_ALREADY_ADDED(texture) { \
		ITexture* dupTex = FindTexturePtr(texture->m_nameHash); \
		if(dupTex) ASSERT_FAIL("Texture %s was already added, refcount %d", dupTex->GetName(), dupTex->Ref_Count()); \
	}

#endif // _RETAIL