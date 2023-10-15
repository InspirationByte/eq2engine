//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Direct 3D 9.0 ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/ShaderAPI_Base.h"

enum EGraphicsVendor
{
	VENDOR_OTHER = -1,
	VENDOR_ATI,
	VENDOR_NV,
	VENDOR_INTEL,
};

struct DX9Sampler_t;

class ShaderAPID3D9 : public ShaderAPI_Base
{
public:

	friend class 				CD3D9VertexFormat;
	friend class 				CD3D9VertexBuffer;
	friend class 				CD3D9IndexBuffer;
	friend class				CD3D9ShaderProgram;
	friend class				CD3D9Texture;
	friend class				CD3D9RenderLib;
	friend class				CD3D9OcclusionQuery;
	
	friend class				CD3D9DepthStencilState;
	friend class				CD3D9RasterizerState;
	friend class				CD3D9BlendingState;
	
								~ShaderAPID3D9();
								ShaderAPID3D9();	

	void						SetD3DDevice(LPDIRECT3DDEVICE9 d3ddev, D3DCAPS9 &d3dcaps);
	LPDIRECT3DDEVICE9			GetD3DDevice() { return m_pD3DDevice; }

	void						OnDeviceLost();
	bool						ResetDevice(D3DPRESENT_PARAMETERS &d3dpp);
	bool						RestoreDevice();

	bool						CreateD3DFrameBufferSurfaces();
	void						ReleaseD3DFrameBufferSurfaces();


	// Init + Shurdown
	void						Init( const shaderAPIParams_t &params );
	void						Shutdown();

	void						PrintAPIInfo() const;

	bool						IsDeviceActive() const;

//-------------------------------------------------------------
// Rendering's applies
//-------------------------------------------------------------
	void						Reset(int nResetType = STATE_RESET_ALL);

	void						ApplyTextures();
	void						ApplySamplerState();
	void						ApplyBlendState();
	void						ApplyDepthState();
	void						ApplyRasterizerState();
	void						ApplyShaderProgram();
	void						ApplyConstants();

	void						Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const ColorRGBA &fillColor,float fDepth, int nStencil);

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

	// shader API class type for shader developers.
	ER_ShaderAPIType			GetShaderAPIClass() const {return SHADERAPI_DIRECT3D9;}

	// Device vendor and version
	const char*					GetDeviceNameString() const;

	// Renderer string (ex: OpenGL, D3D9)
	const char*					GetRendererName() const;

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// Synchronization
	void						Flush();

	void						Finish();

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------

	// creates occlusion query object
	IOcclusionQuery*			CreateOcclusionQuery();

	// removal of occlusion query object
	void						DestroyOcclusionQuery(IOcclusionQuery* pQuery);

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// It will add new rendertarget
	ITexturePtr					CreateRenderTarget(const char* pszName,
														int width, int height, 
														ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR,
														ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
														ER_CompareFunc comparison = COMPFUNC_NEVER,
														int nFlags = 0);

//-------------------------------------------------------------
// State manipulation 
//-------------------------------------------------------------

	// creates blending state
	IRenderState*				CreateBlendingState( const BlendStateParam_t &blendDesc );
	
	// creates depth/stencil state
	IRenderState*				CreateDepthStencilState( const DepthStencilStateParams_t &depthDesc );

	// creates rasterizer state
	IRenderState*				CreateRasterizerState( const RasterizerStateParams_t &rasterDesc );

	// completely destroys shader
	void						DestroyRenderState( IRenderState* pShaderProgram, bool removeAllRefs = false );

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

	IVertexFormat*				CreateVertexFormat(const char* name, ArrayCRef<VertexFormatDesc> formatDesc);
	IVertexBuffer*				CreateVertexBuffer(ER_BufferAccess nBufAccess, int nNumVerts, int strideSize, void *pData = nullptr);
	IIndexBuffer*				CreateIndexBuffer(int nIndices, int nIndexSize, ER_BufferAccess nBufAccess, void *pData = nullptr);

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

	// Indexed primitive drawer
	void						DrawIndexedPrimitives(ER_PrimitiveType nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex = 0);

	// Draw elements
	void						DrawNonIndexedPrimitives(ER_PrimitiveType nType, int nFirstVertex, int nVertices);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// Copy render target to texture
	void						CopyFramebufferToTexture(const ITexturePtr& renderTarget);

	// Copy render target to texture
	void						CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect = nullptr, IAARectangle* destRect = nullptr);

	// Changes render target (MRT)
	void						ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets,
													ArrayCRef<int> rtSlice = nullptr,
													const ITexturePtr& depthTarget = nullptr,
													int depthSlice = 0);

	// Changes back to backbuffer
	void						ChangeRenderTargetToBackBuffer();

	// resizes render target
	void						ResizeRenderTarget(const ITexturePtr& renderTarget, int newWide, int newTall);

//-------------------------------------------------------------
// State setup functions for drawing
//-------------------------------------------------------------

	// sets viewport
	void						SetViewport(int x, int y, int w, int h);

	// returns viewport
	void						GetViewport(int &x, int &y, int &w, int &h);

	// returns current size of viewport
	void						GetViewportDimensions(int &wide, int &tall);

	// Set Depth range for next primitives
	void						SetDepthRange(float fZNear,float fZFar);

	// sets scissor rectangle
	void						SetScissorRectangle( const IAARectangle &rect );

	// Set the texture. Animation is set from ITexture every frame (no affection on speed) before you do 'ApplyTextures'
	// Also you need to specify texture name. If you don't, use registers (not fine with DX10, 11)
	void						SetTexture(int nameHash, const ITexturePtr& texture);

//-------------------------------------------------------------
// Vertex buffer object handling
//-------------------------------------------------------------

	// Changes the vertex format
	void						ChangeVertexFormat(IVertexFormat* pVertexFormat);

	// Changes the vertex buffer
	void						ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset = 0);

	// Changes the index buffer
	void						ChangeIndexBuffer(IIndexBuffer* pIndexBuffer);

	// Destroy vertex format
	void						DestroyVertexFormat(IVertexFormat* pFormat);

	// Destroy vertex buffer
	void						DestroyVertexBuffer(IVertexBuffer* pVertexBuffer);

	// Destroy index buffer
	void						DestroyIndexBuffer(IIndexBuffer* pIndexBuffer);

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// Creates shader class for needed ShaderAPI
	IShaderProgram*				CreateNewShaderProgram(const char* pszName, const char* query = nullptr);

	// Destroy all shader
	void						DestroyShaderProgram(IShaderProgram* pShaderProgram);

	// Load any shader from stream
	bool						CompileShadersFromStream(	IShaderProgram* pShaderOutput,
															const shaderProgramCompileInfo_t& info,
															const char* extra = nullptr
															);

	// Set current shader for rendering
	void						SetShader(IShaderProgram* pShader);

	// RAW Constant (Used for structure types, etc.)
	void						SetShaderConstantRaw(int nameHash, const void *data, int nSize);

protected:

	// Creates empty texture resource.
	ITexturePtr					CreateTextureResource(const char* pszName);

	void						StepProgressiveLodTextures();

	void						PreloadShadersFromCache();
	bool						InitShaderFromCache(IShaderProgram* pShaderOutput, IVirtualStream* pStream, uint32 checksum = 0);

private:
	static bool					InternalCreateRenderTarget(LPDIRECT3DDEVICE9 dev, CD3D9Texture* tex, int nFlags, const ShaderAPICaps_t& caps);
	
	D3DCAPS9					m_hCaps;

	Set<CD3D9Texture*>			m_progressiveTextures{ PP_SL };

	CD3D9Texture* m_fbColorTexture{ nullptr };
	CD3D9Texture* m_fbDepthTexture{ nullptr };

	// Sampler states is not binding same as OpenGL
	SamplerStateParams*		m_pSelectedSamplerStates[MAX_SAMPLERSTATE]{ nullptr };
	SamplerStateParams			m_pCurrentSamplerStates[MAX_SAMPLERSTATE];
	int							m_nCurrentSamplerStateDirty{ -1 };

	// Sampler states is not binding same as OpenGL
	SamplerStateParams*		m_pSelectedVertexSamplerStates[MAX_SAMPLERSTATE]{ nullptr };
	SamplerStateParams			m_pCurrentVertexSamplerStates[MAX_SAMPLERSTATE];
	int							m_nCurrentVertexSamplerStateDirty{ -1 };

	UINT						m_nSelectedStreamParam[MAX_VERTEXSTREAM]{ 0 };

	// Custom blend state
	SamplerStateParams			m_defaultSamplerState;

	int							m_nCurrentSrcFactor{ BLENDFACTOR_ONE };
	int							m_nCurrentDstFactor{ BLENDFACTOR_ZERO };
	int							m_nCurrentBlendMode{ BLENDFUNC_ADD };

	int							m_nCurrentDepthFunc{ COMPFUNC_LEQUAL };
	bool						m_bCurrentDepthTestEnable{ false };
	bool						m_bCurrentDepthWriteEnable{ false };
	LPDIRECT3DSURFACE9			m_pCurrentDepthSurface{ nullptr };

	bool						m_bDoStencilTest{ false };

	uint8						m_nStencilMask{ 0xff };
	uint8						m_nStencilWriteMask{ 0xff };
	uint8						m_nStencilRef{ 0xff };
	int							m_nStencilFunc{ COMPFUNC_ALWAYS };
	int							m_nStencilFail{ STENCILFUNC_KEEP };
	int							m_nDepthFail{ STENCILFUNC_KEEP };
	int							m_nStencilPass{ STENCILFUNC_KEEP };

	float						m_fCurrentDepthBias{ 0.0f };
	float						m_fCurrentSlopeDepthBias{ 0.0f };

	bool						m_bCurrentMultiSampleEnable{ false };
	bool						m_bCurrentScissorEnable{ false };
	int							m_nCurrentCullMode{ CULL_BACK };
	int							m_nCurrentFillMode{ FILL_SOLID };

	int							m_nCurrentMask{ COLORMASK_ALL };
	bool						m_bCurrentBlendEnable{ false };

	Vector4D					m_vsRegs[256]{ 0 };
	Vector4D					m_psRegs[224]{ 0 };

	int							m_nMinVSDirty{ 224 };
	int							m_nMaxVSDirty{ -1 };

	int							m_nMinPSDirty{ 224 };
	int							m_nMaxPSDirty{ -1 };

	D3DTRANSFORMSTATETYPE		m_nCurrentMatrixMode{ D3DTS_VIEW };
	LPDIRECT3DQUERY9			m_pEventQuery{ nullptr };
	LPDIRECT3DDEVICE9			m_pD3DDevice{ nullptr };

	bool						m_deviceIsLost{ false };
	bool						m_deviceAtReset{ false };

	EGraphicsVendor				m_vendor{ VENDOR_OTHER };
};
