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

//------------------------------------------------------------

#define SHADERCACHE_IDENT		MCHAR4('C','A','C','H')

struct shaderCacheHdr_t
{
	int			ident;
	uint32		checksum;		// file crc32

	uint		psSize;
	uint		vsSize;

	ushort		numConstants;
	ushort		numSamplers;

	int			nameLen;
	int			extraLen;
};

//------------------------------------------------------------

class ConCommandBase;

class ShaderAPI_Base : public IShaderAPI
{
public:
	ShaderAPI_Base();

	// Init + Shurdown
	virtual void				Init( const ShaderAPIParams &params );
	virtual void				Shutdown();

	const ShaderAPIParams&		GetParams() const { return m_params; }

	virtual void				SetViewport(const IAARectangle& rect);
	static void					GetConsoleTextureList(const ConCommandBase* base, Array<EqString>&, const char* query);

	// texture uploading frequency
	void						SetProgressiveTextureFrequency(int frames);
	int							GetProgressiveTextureFrequency() const;

//-------------------------------------------------------------
// Apply/Reset functions
//-------------------------------------------------------------

	virtual void				Reset(int nResetTypeFlags = STATE_RESET_ALL);

	void						Apply();

	virtual void				ApplyBuffers();

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

	const ShaderAPICaps&		GetCaps() const {return m_caps;}

	virtual EShaderAPIType		GetShaderAPIClass() const {return SHADERAPI_EMPTY;}

	// Draw call counter
	int							GetDrawCallsCount() const;

	int							GetDrawIndexedPrimitiveCallsCount() const;

	// triangles per scene drawing
	int							GetTrianglesCount() const;

	// Resetting the counters
	void						ResetCounters();

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// prepares for async operation (required to be called in main thread)
	virtual void				BeginAsyncOperation( uintptr_t threadId ) {}	// obsolete for D3D

	// completes for async operation (must be called in worker thread)
	virtual void				EndAsyncOperation() {}							// obsolete for D3D

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// creates texture from image array, used in LoadTexture, common use only
	ITexturePtr					CreateTexture(const ArrayCRef<CImagePtr>& pImages, const SamplerStateParams& sampler, int nFlags = 0);

	// creates procedural (lockable) texture
	ITexturePtr					CreateProceduralTexture(const char* pszName,
														ETextureFormat nFormat,
														int width, int height,
														int depth = 1,
														int arraySize = 1,
														ETexFilterMode texFilter = TEXFILTER_NEAREST,
														ETexAddressMode textureAddress = TEXADDRESS_WRAP,
														int nFlags = 0,
														int nDataSize = 0, const unsigned char* pData = nullptr
														);

	// Finds texture by name
	ITexturePtr					FindTexture(const char* pszName);

	// Searches for existing texture or creates new one. Use this for resource loading
	ITexturePtr					FindOrCreateTexture( const char* pszName, bool& justCreated);

	// Unload the texture and free the memory
	void						FreeTexture(ITexture* pTexture);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// Changes render target (single RT)
	void						ChangeRenderTarget(const ITexturePtr& renderTarget, int rtSlice = 0, const ITexturePtr& depthTarget = nullptr, int depthSlice = 0);

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

	// INTERNAL: Sets the reserved blending state (from pre-defined preset)
	void						SetBlendingState( IRenderState* pBlending );

	// INTERNAL: Set depth and stencil state
	void						SetDepthStencilState( IRenderState *pDepthStencilState );

	// INTERNAL: sets reserved rasterizer mode
	void						SetRasterizerState( IRenderState* pState );

	// internal texture setup
	void						SetTextureAtIndex(const ITexturePtr& texture, int level);

	// returns the currently set textre at level
	const ITexturePtr&			GetTextureAt( int level ) const;

//-------------------------------------------------------------
// Vertex buffer object handling
//-------------------------------------------------------------

	// Sets the vertex format
	void						SetVertexFormat(IVertexFormat* pVertexFormat);

	// Sets the vertex buffer
	void						SetVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset = 0);

	// Changes the index buffer
	void						SetIndexBuffer(IIndexBuffer *pIndexBuffer);

	// returns vertex format
	IVertexFormat*				FindVertexFormat(const char* name) const;

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// Loads and compiles shaders from files
	bool						LoadShadersFromFile(IShaderProgramPtr pShaderOutput, const char* pszFilePrefix, const char *extra = nullptr);
	IShaderProgramPtr			FindShaderProgram(const char* pszName, const char* query);

protected:

	virtual ITexturePtr			CreateTextureResource(const char* pszName) = 0;

	// NOTE: not thread-safe, used for error checks
	ITexture*					FindTexturePtr(int nameHash) const;

//-------------------------------------------------------------
// Useful data
//-------------------------------------------------------------

	ShaderAPIParams				m_params;
	ShaderAPICaps				m_caps;

	Map<int, IShaderProgram*>	m_ShaderList{ PP_SL };
	Map<int, ITexture*>			m_TextureList{ PP_SL };

	Array<IRenderState*>		m_SamplerStates{ PP_SL };
	Array<IRenderState*>		m_BlendStates{ PP_SL };
	Array<IRenderState*>		m_DepthStates{ PP_SL };
	Array<IRenderState*>		m_RasterizerStates{ PP_SL };
	Array<IOcclusionQuery*>		m_OcclusionQueryList{ PP_SL };

	Array<IVertexFormat*>		m_VFList{ PP_SL };
	Array<IVertexBuffer*>		m_VBList{ PP_SL };
	Array<IIndexBuffer*>		m_IBList{ PP_SL };

	IShaderProgramPtr			m_pCurrentShader{ nullptr };
	IShaderProgramPtr			m_pSelectedShader{ nullptr };
	ITexturePtr					m_pSelectedTextures[MAX_TEXTUREUNIT];
	ITexturePtr					m_pCurrentTextures[MAX_TEXTUREUNIT];
	ITexturePtr					m_pSelectedVertexTextures[MAX_VERTEXTEXTURES];
	ITexturePtr					m_pCurrentVertexTextures[MAX_VERTEXTEXTURES];
	ITexturePtr					m_pCurrentColorRenderTargets[MAX_RENDERTARGETS];
	int							m_nCurrentCRTSlice[MAX_RENDERTARGETS]{ 0 };

	ITexturePtr					m_pCurrentDepthRenderTarget;

// ----------------Selected-----------------

	IRenderState*				m_pSelectedBlendstate{ nullptr };
	IRenderState*				m_pSelectedDepthState{ nullptr };
	IRenderState*				m_pSelectedRasterizerState{ nullptr };

// ----------------Current-----------------

	IRenderState*				m_pCurrentBlendstate{ nullptr };
	IRenderState*				m_pCurrentDepthState{ nullptr };
	IRenderState*				m_pCurrentRasterizerState{ nullptr };

	IVertexFormat*				m_pSelectedVertexFormat{ nullptr };
	IVertexFormat*				m_pCurrentVertexFormat{ nullptr };

	IVertexFormat*				m_pActiveVertexFormat[MAX_VERTEXSTREAM]{ nullptr };
	IVertexBuffer*				m_pSelectedVertexBuffers[MAX_VERTEXSTREAM]{ nullptr };
	IVertexBuffer*				m_pCurrentVertexBuffers[MAX_VERTEXSTREAM]{ nullptr };

	IIndexBuffer*				m_pSelectedIndexBuffer{ nullptr };
	IIndexBuffer*				m_pCurrentIndexBuffer{ nullptr };

	intptr						m_nSelectedOffsets[MAX_VERTEXSTREAM]{ 0 };
	intptr						m_nCurrentOffsets[MAX_VERTEXSTREAM]{ 0 };
	IAARectangle				m_viewPort{ 0, 0, 800, 600 };

	int							m_progressiveTextureFrequency{ 0 };

	// Perfomance counters
	int							m_nDrawCalls{ 0 };
	int							m_nTrianglesCount{ 0 };
	int							m_nDrawIndexedPrimitiveCalls{ 0 };
};

#ifdef _RETAIL

#define CHECK_TEXTURE_ALREADY_ADDED(texture) 

#else

#define CHECK_TEXTURE_ALREADY_ADDED(texture) { \
		ITexture* dupTex = FindTexturePtr(texture->m_nameHash); \
		if(dupTex) ASSERT_FAIL("Texture %s was already added, refcount %d", dupTex->GetName(), dupTex->Ref_Count()); \
	}

#endif // _RETAIL