//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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
	virtual void						Init( const shaderAPIParams_t &params );
	virtual void						Shutdown();

	const shaderAPIParams_t&			GetParams() const { return m_params; }

	virtual void						SetViewport(int x, int y, int w, int h);
	virtual void						GetViewport(int &x, int &y, int &w, int &h);

	ETextureFormat						GetScreenFormat();

	// default error texture pointer
	const ITexturePtr&					GetErrorTexture() const;

	static void							GetConsoleTextureList(const ConCommandBase* base, Array<EqString>&, const char* query);

	// texture uploading frequency
	void								SetProgressiveTextureFrequency(int frames);
	int									GetProgressiveTextureFrequency() const;

//-------------------------------------------------------------
// Apply/Reset functions
//-------------------------------------------------------------

	virtual void						Reset(int nResetTypeFlags = STATE_RESET_ALL);

	void								Apply();

	virtual void						ApplyBuffers();

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

	const ShaderAPICaps_t&				GetCaps() const {return m_caps;}

	virtual ER_ShaderAPIType			GetShaderAPIClass() const {return SHADERAPI_EMPTY;}

	// Draw call counter
	int									GetDrawCallsCount() const;

	int									GetDrawIndexedPrimitiveCallsCount() const;

	// triangles per scene drawing
	int									GetTrianglesCount() const;

	// Resetting the counters
	void								ResetCounters();

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// prepares for async operation (required to be called in main thread)
	virtual void						BeginAsyncOperation( uintptr_t threadId ) {}	// obsolete for D3D

	// completes for async operation (must be called in worker thread)
	virtual void						EndAsyncOperation() {}							// obsolete for D3D

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// creates texture from image array, used in LoadTexture, common use only
	ITexturePtr							CreateTexture(const ArrayCRef<CRefPtr<CImage>>& pImages, const SamplerStateParam_t& sampler, int nFlags = 0);

	// creates procedural (lockable) texture
	ITexturePtr							CreateProceduralTexture(const char* pszName,
																ETextureFormat nFormat,
																int width, int height,
																int depth = 1,
																int arraySize = 1,
																ER_TextureFilterMode texFilter = TEXFILTER_NEAREST,
																ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
																int nFlags = 0,
																int nDataSize = 0, const unsigned char* pData = nullptr
																);

	// Finds texture by name
	ITexturePtr							FindTexture(const char* pszName);

	// Searches for existing texture or creates new one. Use this for resource loading
	ITexturePtr							FindOrCreateTexture( const char* pszName, bool& justCreated);

	// Unload the texture and free the memory
	void								FreeTexture(ITexture* pTexture);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// Changes render target (single RT)
	void								ChangeRenderTarget(const ITexturePtr& renderTarget, int rtSlice = 0, const ITexturePtr& depthTarget = nullptr, int depthSlice = 0);

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

	// INTERNAL: Sets the reserved blending state (from pre-defined preset)
	void								SetBlendingState( IRenderState* pBlending );

	// INTERNAL: Set depth and stencil state
	void								SetDepthStencilState( IRenderState *pDepthStencilState );

	// INTERNAL: sets reserved rasterizer mode
	void								SetRasterizerState( IRenderState* pState );

	// internal texture setup
	void								SetTextureAtIndex(const ITexturePtr& texture, int level);

	// returns the currently set textre at level
	const ITexturePtr&					GetTextureAt( int level ) const;

//-------------------------------------------------------------
// Vertex buffer object handling
//-------------------------------------------------------------

	// Sets the vertex format
	void								SetVertexFormat(IVertexFormat* pVertexFormat);

	// Sets the vertex buffer
	void								SetVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset = 0);

	// Sets the vertex buffer
	void								SetVertexBuffer(int nStream, const void* base);

	// Changes the index buffer
	void								SetIndexBuffer(IIndexBuffer *pIndexBuffer);

	// returns vertex format
	IVertexFormat*						FindVertexFormat(const char* name) const;

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// Loads and compiles shaders from files
	bool								LoadShadersFromFile(IShaderProgram* pShaderOutput, const char* pszFilePrefix, const char *extra = nullptr);
	IShaderProgram*						FindShaderProgram(const char* pszName, const char* query);

	// Shader constants setup
	void								SetShaderConstantInt(const char *pszName, const int constant);
	void								SetShaderConstantFloat(const char *pszName, const float constant);
	void								SetShaderConstantVector2D(const char *pszName, const Vector2D &constant);
	void								SetShaderConstantVector3D(const char *pszName, const Vector3D &constant);
	void								SetShaderConstantVector4D(const char *pszName, const Vector4D &constant);
	void								SetShaderConstantMatrix4(const char *pszName, const Matrix4x4 &constant);
	void								SetShaderConstantArrayFloat(const char *pszName, const float *constant, int count);
	void								SetShaderConstantArrayVector2D(const char *pszName, const Vector2D *constant, int count);
	void								SetShaderConstantArrayVector3D(const char *pszName, const Vector3D *constant, int count);
	void								SetShaderConstantArrayVector4D(const char *pszName, const Vector4D *constant, int count);
	void								SetShaderConstantArrayMatrix4(const char *pszName, const Matrix4x4 *constant, int count);

protected:

	virtual ITexturePtr					CreateTextureResource(const char* pszName) = 0;

//-------------------------------------------------------------
// Useful data
//-------------------------------------------------------------

	shaderAPIParams_t					m_params;
	ShaderAPICaps_t						m_caps;

	// Shader list
	Map<int, IShaderProgram*>			m_ShaderList{ PP_SL };

	// Loaded textures list
	Map<int, ITexture*>					m_TextureList{ PP_SL };

	// List of dynamically added sampler states
	Array<IRenderState*>				m_SamplerStates{ PP_SL };

	// List of dynamically added blending states
	Array<IRenderState*>				m_BlendStates{ PP_SL };

	// List of dynamically added depth states
	Array<IRenderState*>				m_DepthStates{ PP_SL };

	// List of dynamically added rasterizer states
	Array<IRenderState*>				m_RasterizerStates{ PP_SL };

	// occlusion queries
	Array<IOcclusionQuery*>				m_OcclusionQueryList{ PP_SL };

	// Aviable vertex formats
	Array<IVertexFormat*>				m_VFList{ PP_SL };

	// Aviable vertex buffers
	Array<IVertexBuffer*>				m_VBList{ PP_SL };

	// Aviable index buffers
	Array<IIndexBuffer*>				m_IBList{ PP_SL };

	// Current shader
	IShaderProgram*						m_pCurrentShader;

	// Current shader
	IShaderProgram*						m_pSelectedShader;

	// Selected textures
	ITexturePtr							m_pSelectedTextures[MAX_TEXTUREUNIT];
	// Current textures
	ITexturePtr							m_pCurrentTextures[MAX_TEXTUREUNIT];

	// Selected textures
	ITexturePtr							m_pSelectedVertexTextures[MAX_VERTEXTEXTURES];
	// Current textures
	ITexturePtr							m_pCurrentVertexTextures[MAX_VERTEXTEXTURES];

	// Current render targets
	ITexturePtr							m_pCurrentColorRenderTargets[MAX_MRTS];
	int									m_nCurrentCRTSlice[MAX_MRTS];

	// Current render targets
	ITexturePtr							m_pCurrentDepthRenderTarget;

// ----------------Selected-----------------

	// Selected blend state (must call ApplyBlendState after setup)
	IRenderState*						m_pSelectedBlendstate;

	// Selected blend state (must call ApplyDepthState after setup)
	IRenderState*						m_pSelectedDepthState;

	// Selected blend state (must call ApplyRasterizerState after setup)
	IRenderState*						m_pSelectedRasterizerState;

// ----------------Current-----------------

	// Current blend state (must call ApplyBlendState after setup)
	IRenderState*						m_pCurrentBlendstate;

	// Current blend state (must call ApplyDepthState after setup)
	IRenderState*						m_pCurrentDepthState;

	// Current blend state (must call ApplyRasterizerState after setup)
	IRenderState*						m_pCurrentRasterizerState;

	// VF selectoin
	IVertexFormat*						m_pSelectedVertexFormat;
	IVertexFormat*						m_pCurrentVertexFormat;

	IVertexFormat*						m_pActiveVertexFormat[MAX_VERTEXSTREAM];

	// Vertex buffer
	IVertexBuffer*						m_pSelectedVertexBuffers[MAX_VERTEXSTREAM];
	IVertexBuffer*						m_pCurrentVertexBuffers[MAX_VERTEXSTREAM];

	// Index buffer
	IIndexBuffer*						m_pSelectedIndexBuffer;
	IIndexBuffer*						m_pCurrentIndexBuffer;

	// Buffer offset
	intptr								m_nSelectedOffsets[MAX_VERTEXSTREAM];
	intptr								m_nCurrentOffsets[MAX_VERTEXSTREAM];

	// special error texture
	ITexturePtr							m_pErrorTexture;

	// Viewport
	int16								m_nViewportWidth;
	int16								m_nViewportHeight;

	int									m_progressiveTextureFrequency{ 0 };

	// Perfomance counters
	int									m_nDrawCalls;
	int									m_nTrianglesCount;

	int									m_nDrawIndexedPrimitiveCalls;

	ETextureFormat						m_nScreenFormat;
};
