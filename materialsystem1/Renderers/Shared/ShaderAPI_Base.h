//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				For high level look for the material system (Engine)
//				Contains FFP functions and Shader (FFP overriding programs)
//
//				The renderer may do anti-wallhacking functions//////////////////////////////////////////////////////////////////////////////////

#ifndef SHADERAPI_BASE_H
#define SHADERAPI_BASE_H

#include "renderers/IShaderAPI.h"
#include "ds/Array.h"
#include "ds/Map.h"
#include "utils/eqthread.h"

using namespace Threading;

class ConCommandBase;

class ShaderAPI_Base : public IShaderAPI
{
public:
										ShaderAPI_Base();

	// Init + Shurdown
	virtual void						Init( shaderAPIParams_t &params );
	virtual void						Shutdown();

	void								ThreadLock();
	void								ThreadUnlock();

	virtual void						SetViewport(int x, int y, int w, int h);
	virtual void						GetViewport(int &x, int &y, int &w, int &h);

	ETextureFormat						GetScreenFormat();

	// default error texture pointer
	ITexture*							GetErrorTexture();

	static void							GetConsoleTextureList(const ConCommandBase* base, Array<EqString>&, const char* query);

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

	// Load texture from file (DDS or TGA only)
	ITexture*							LoadTexture(const char* pszFileName, ER_TextureFilterMode textureFilterType, ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP, int nFlags = 0);

	// creates texture from image array, used in LoadTexture, common use only
	ITexture*							CreateTexture(const Array<CImage*>& pImages, const SamplerStateParam_t& sampler, int nFlags = 0);

	// creates procedural (lockable) texture
	ITexture*							CreateProceduralTexture(const char* pszName,
																ETextureFormat nFormat,
																int width, int height,
																int depth = 1,
																int arraySize = 1,
																ER_TextureFilterMode texFilter = TEXFILTER_NEAREST,
																ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
																int nFlags = 0,
																int nDataSize = 0, const unsigned char* pData = NULL
																);

	// Finds texture by name
	ITexture*							FindTexture(const char* pszName);

	// Error texture generator
	ITexture*							GenerateErrorTexture(int nFlags = 0);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// Changes render target (single RT)
	void								ChangeRenderTarget(ITexture* pRenderTarget, int nCubemapFace = 0, ITexture* pDepthTarget = NULL, int nDepthSlice = 0);

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
	void								SetTextureOnIndex( ITexture* pTexture, int level = 0 );

	// returns the currently set textre at level
	ITexture*							GetTextureAt( int level ) const;

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
	bool								LoadShadersFromFile(IShaderProgram* pShaderOutput, const char* pszFilePrefix, const char *extra = NULL);
	IShaderProgram*						FindShaderProgram(const char* pszName, const char* query);

	// Shader constants setup
	int									SetShaderConstantInt(const char *pszName, const int constant, int const_id = -1);
	int									SetShaderConstantFloat(const char *pszName, const float constant, int const_id = -1);
	int									SetShaderConstantVector2D(const char *pszName, const Vector2D &constant, int const_id = -1);
	int									SetShaderConstantVector3D(const char *pszName, const Vector3D &constant, int const_id = -1);
	int									SetShaderConstantVector4D(const char *pszName, const Vector4D &constant, int const_id = -1);
	int									SetShaderConstantMatrix4(const char *pszName, const Matrix4x4 &constant, int const_id = -1);
	int									SetShaderConstantArrayFloat(const char *pszName, const float *constant, int count, int const_id = -1);
	int									SetShaderConstantArrayVector2D(const char *pszName, const Vector2D *constant, int count, int const_id = -1);
	int									SetShaderConstantArrayVector3D(const char *pszName, const Vector3D *constant, int count, int const_id = -1);
	int									SetShaderConstantArrayVector4D(const char *pszName, const Vector4D *constant, int count, int const_id = -1);
	int									SetShaderConstantArrayMatrix4(const char *pszName, const Matrix4x4 *constant, int count, int const_id = -1);

//-------------------------------------------------------------
// Other operations (Now it's unused)
//-------------------------------------------------------------

	// Find sampler state with adding new one (if not exist)
	SamplerStateParam_t					MakeSamplerState(ER_TextureFilterMode textureFilterType,ER_TextureAddressMode addressS, ER_TextureAddressMode addressT, ER_TextureAddressMode addressR);

	// Find Rasterizer State with adding new one (if not exist)
	RasterizerStateParams_t				MakeRasterizerState(ER_CullMode nCullMode, ER_FillMode nFillMode = FILL_SOLID, bool bMultiSample = true, bool bScissor = false);

	// Find Depth State with adding new one (if not exist)
	DepthStencilStateParams_t			MakeDepthState(bool bDoDepthTest, bool bDoDepthWrite, ER_CompareFunc depthCompFunc = COMP_LEQUAL);

protected:

	void								GetImagesForTextureName(Array<EqString>& textureNames, const char* pszFileName, int nFlags);
	
	bool								RestoreTextureInternal(ITexture* pTexture);

	virtual void						CreateTextureInternal(ITexture** pTex, const Array<CImage*>& pImages, const SamplerStateParam_t& sSamplingParams,int nFlags = 0) = 0;

//-------------------------------------------------------------
// Useful data
//-------------------------------------------------------------

	const shaderAPIParams_t*			m_params;
	ShaderAPICaps_t						m_caps;

	// Shader list
	Map<int, IShaderProgram*>			m_ShaderList;

	// Loaded textures list
	Map<int, ITexture*>					m_TextureList;

	// List of dynamically added sampler states
	Array<IRenderState*>				m_SamplerStates;

	// List of dynamically added blending states
	Array<IRenderState*>				m_BlendStates;

	// List of dynamically added depth states
	Array<IRenderState*>				m_DepthStates;

	// List of dynamically added rasterizer states
	Array<IRenderState*>				m_RasterizerStates;

	// occlusion queries
	Array<IOcclusionQuery*>				m_OcclusionQueryList;

	// Aviable vertex formats
	Array<IVertexFormat*>				m_VFList;

	// Aviable vertex buffers
	Array<IVertexBuffer*>				m_VBList;

	// Aviable index buffers
	Array<IIndexBuffer*>				m_IBList;

	// Current shader
	IShaderProgram*						m_pCurrentShader;

	// Current shader
	IShaderProgram*						m_pSelectedShader;

	// Selected textures
	ITexture*							m_pSelectedTextures[MAX_TEXTUREUNIT];
	// Current textures
	ITexture*							m_pCurrentTextures[MAX_TEXTUREUNIT];

	// Selected textures
	ITexture*							m_pSelectedVertexTextures[MAX_VERTEXTEXTURES];
	// Current textures
	ITexture*							m_pCurrentVertexTextures[MAX_VERTEXTEXTURES];

	// Selected render targets
	ITexture*							m_pSelectedRenderTargets[MAX_MRTS];

	// Current render targets
	ITexture*							m_pCurrentColorRenderTargets[MAX_MRTS];
	int									m_nCurrentCRTSlice[MAX_MRTS];

	// Current render targets
	ITexture*							m_pCurrentDepthRenderTarget;

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
	ITexture*							m_pErrorTexture;

	// Viewport
	int16								m_nViewportWidth;
	int16								m_nViewportHeight;

	// Perfomance counters
	int									m_nDrawCalls;
	int									m_nTrianglesCount;

	int									m_nDrawIndexedPrimitiveCalls;

	ETextureFormat						m_nScreenFormat;

	CEqMutex							m_Mutex;
};

#endif // SHADERAPI_BASE_H
