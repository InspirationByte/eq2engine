//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Direct 3D 9.0 ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/ShaderAPI_Base.h"

enum EGraphicsVendor
{
	VENDOR_ATI,
	VENDOR_NV,
	VENDOR_INTEL,
	VENDOR_OTHER,
};

struct DX9Sampler_t;

class ShaderAPID3DX9 : public ShaderAPI_Base
{
public:

	friend class 				CVertexFormatD3DX9;
	friend class 				CVertexBufferD3DX9;
	friend class 				CIndexBufferD3DX9;
	friend class				CD3D9ShaderProgram;
	friend class				CD3D9Texture;
	friend class				CD3DRenderLib;
	friend class				CD3D9OcclusionQuery;
	
	friend class				CD3D9DepthStencilState;
	friend class				CD3D9RasterizerState;
	friend class				CD3D9BlendingState;
	
								~ShaderAPID3DX9();
								ShaderAPID3DX9();

	// Only in D3DX9 Renderer
#ifdef USE_D3DEX
	void						SetD3DDevice(LPDIRECT3DDEVICE9EX d3ddev, D3DCAPS9 &d3dcaps);
#else
	void						SetD3DDevice(LPDIRECT3DDEVICE9 d3ddev, D3DCAPS9 &d3dcaps);
#endif

	void						CheckDeviceResetOrLost(HRESULT hr);
	bool						ResetDevice(D3DPRESENT_PARAMETERS &d3dpp);

	bool						CreateD3DFrameBufferSurfaces();
	void						ReleaseD3DFrameBufferSurfaces();


	// Init + Shurdown
	void						Init( shaderAPIParams_t &params );
	void						Shutdown();

	void						PrintAPIInfo();

	bool						IsDeviceActive();

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
	// DON'T USE TYPES IN DYNAMIC SHADER CODE! USE MATSYSTEM MAT-FILE DEFS!
	ER_ShaderAPIType			GetShaderAPIClass() const {return SHADERAPI_DIRECT3D9;}

	// Device vendor and version
	const char*					GetDeviceNameString() const;

	// Renderer string (ex: OpenGL, D3D9)
	const char*					GetRendererName() const;

	// Render targetting support
	bool						IsSupportsRendertargetting() const;

	// Render targetting support for Multiple RTs
	bool						IsSupportsMRT() const;

	// Supports multitexturing???
	bool						IsSupportsMultitexturing() const;

	// The driver/hardware is supports Pixel shaders?
	bool						IsSupportsPixelShaders() const;

	// The driver/hardware is supports Vertex shaders?
	bool						IsSupportsVertexShaders() const;

	// The driver/hardware is supports Geometry shaders?
	bool						IsSupportsGeometryShaders() const;

	// The driver/hardware is supports Domain shaders?
	bool						IsSupportsDomainShaders() const;

	// The driver/hardware is supports Hull (tessellator) shaders?
	bool						IsSupportsHullShaders() const;

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

	// Unload the texture and free the memory
	void						FreeTexture(ITexture* pTexture);

	// It will add new rendertarget
	ITexture*					CreateRenderTarget(	int width, int height,
													ETextureFormat nRTFormat,
													ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR, 
													ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP, 
													ER_CompareFunc comparison = COMP_NEVER, 
													int nFlags = 0);

	// It will add new rendertarget
	ITexture*					CreateNamedRenderTarget(const char* pszName,
														int width, int height, 
														ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR,
														ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
														ER_CompareFunc comparison = COMP_NEVER,
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

	IVertexFormat*				CreateVertexFormat(const char* name, const VertexFormatDesc_t *formatDesc, int nAttribs);
	IVertexBuffer*				CreateVertexBuffer(ER_BufferAccess nBufAccess, int nNumVerts, int strideSize, void *pData = NULL);
	IIndexBuffer*				CreateIndexBuffer(int nIndices, int nIndexSize, ER_BufferAccess nBufAccess, void *pData = NULL);

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

	// saves rendertarget to texture, you can also save screenshots
	void						SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName);

	// Copy render target to texture
	void						CopyFramebufferToTexture(ITexture* pTargetTexture);

	// Copy render target to texture
	void						CopyRendertargetToTexture(ITexture* srcTarget, ITexture* destTex, IRectangle* srcRect = NULL, IRectangle* destRect = NULL);

	// Changes render target (MRT)
	void						ChangeRenderTargets(ITexture** pRenderTargets, int nNumRTs, int* nCubemapFaces = NULL, ITexture* pDepthTarget = NULL, int nDepthSlice = 0);

	// fills the current rendertarget buffers
	void						GetCurrentRenderTargets(ITexture* pRenderTargets[MAX_MRTS], int *nNumRTs, ITexture** pDepthTarget, int cubeNumbers[MAX_MRTS]);

	// Changes back to backbuffer
	void						ChangeRenderTargetToBackBuffer();

	// resizes render target
	void						ResizeRenderTarget(ITexture* pRT, int newWide, int newTall);

//-------------------------------------------------------------
// Matrix for rendering
//-------------------------------------------------------------

	// Matrix mode
	void						SetMatrixMode(ER_MatrixMode nMatrixMode);

	// Will save matrix
	void						PushMatrix();

	// Will reset matrix
	void						PopMatrix();

	// Load identity matrix
	void						LoadIdentityMatrix();

	// Load custom matrix
	void						LoadMatrix(const Matrix4x4 &matrix);

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
	void						SetScissorRectangle( const IRectangle &rect );

	// Set the texture. Animation is set from ITexture every frame (no affection on speed) before you do 'ApplyTextures'
	// Also you need to specify texture name. If you don't, use registers (not fine with DX10, 11)
	void						SetTexture( ITexture* pTexture, const char* pszName, int index = 0 );

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
	IShaderProgram*				CreateNewShaderProgram(const char* pszName, const char* query = NULL);

	// Destroy all shader
	void						DestroyShaderProgram(IShaderProgram* pShaderProgram);

	// Load any shader from stream
	bool						CompileShadersFromStream(	IShaderProgram* pShaderOutput,
															const shaderProgramCompileInfo_t& info,
															const char* extra = NULL
															);

	// Set current shader for rendering
	void						SetShader(IShaderProgram* pShader);

	// RAW Constant (Used for structure types, etc.)
	void						SetShaderConstantRaw(const char *pszName, const void *data, int nSize);

	void						CreateTextureInternal(ITexture** pTex, const Array<CImage*>& pImages, const SamplerStateParam_t& sSamplingParams,int nFlags = 0);
protected:

	IDirect3DBaseTexture9*		CreateD3DTextureFromImage(CImage* pSrc, int& wide, int& tall, int nFlags = 0);

	bool						GetSamplerUnit(CD3D9ShaderProgram* pProgram, const char* pszSamplerName, const DX9Sampler_t** sampler);

private:
	static bool					InternalCreateRenderTarget(LPDIRECT3DDEVICE9 dev, CD3D9Texture* tex, int nFlags, const ShaderAPICaps_t& caps);
	
	CD3D9Texture*				m_fbColorTexture;
	CD3D9Texture*				m_fbDepthTexture;

	// Sampler states is not binding same as OpenGL
	SamplerStateParam_t*		m_pSelectedSamplerStates[MAX_SAMPLERSTATE];
	SamplerStateParam_t			m_pCurrentSamplerStates[MAX_SAMPLERSTATE];
	int							m_nCurrentSamplerStateDirty;

	// Sampler states is not binding same as OpenGL
	SamplerStateParam_t*		m_pSelectedVertexSamplerStates[MAX_SAMPLERSTATE];
	SamplerStateParam_t			m_pCurrentVertexSamplerStates[MAX_SAMPLERSTATE];
	int							m_nCurrentVertexSamplerStateDirty;

	UINT						m_nSelectedStreamParam[MAX_VERTEXSTREAM];

	// Custom blend state
	
	SamplerStateParam_t			m_defaultSamplerState;

	int							m_nCurrentFrontFace;

	int							m_nCurrentSrcFactor;
	int							m_nCurrentDstFactor;
	int							m_nCurrentBlendMode;

	int							m_nCurrentDepthFunc;
	bool						m_bCurrentDepthTestEnable;
	bool						m_bCurrentDepthWriteEnable;
	LPDIRECT3DSURFACE9			m_pCurrentDepthSurface;

	bool						m_bDoStencilTest;

	uint8						m_nStencilMask;
	uint8						m_nStencilWriteMask;
	uint8						m_nStencilRef;
	int							m_nStencilFunc;
	int							m_nStencilFail;
	int							m_nDepthFail;
	int							m_nStencilPass;

	float						m_fCurrentDepthBias;
	float						m_fCurrentSlopeDepthBias;

	bool						m_bCurrentMultiSampleEnable;
	bool						m_bCurrentScissorEnable;
	int							m_nCurrentCullMode;
	int							m_nCurrentFillMode;

	int							m_nCurrentMask;
	bool						m_bCurrentBlendEnable;
	bool						m_bCurrentAlphaTestEnabled;
	float						m_fCurrentAlphaTestRef;

	Vector4D					m_vsRegs[256];
	Vector4D					m_psRegs[224];

	int							m_nMinVSDirty;
	int							m_nMaxVSDirty;

	int							m_nMinPSDirty;
	int							m_nMaxPSDirty;

	D3DTRANSFORMSTATETYPE		m_nCurrentMatrixMode;
	D3DCAPS9					m_hCaps;
	LPDIRECT3DQUERY9			m_pEventQuery;

#ifdef USE_D3DEX
	LPDIRECT3DDEVICE9EX			m_pD3DDevice;
#else
	LPDIRECT3DDEVICE9			m_pD3DDevice;
#endif

	// the main renderer
	//ITexture*					m_pBackBuffer;
	//ITexture*					m_pDepthBuffer;

	//int							m_nCurrentSampleMask;
	int							m_nSelectedSampleMask;

	//uint8						m_nMRTs;

	bool						m_bDeviceIsLost;
	bool						m_bDeviceAtReset;

	EGraphicsVendor				m_vendor;
};
