//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D10 renderer impl for Eq
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ShaderAPI_Base.h"

class ShaderAPID3DX10 : public ShaderAPI_Base
{
	friend class 				CVertexFormatD3D10;
	friend class 				CVertexBufferD3D10;
	friend class 				CIndexBufferD3D10;
	friend class				CD3D10ShaderProgram;
	friend class				CD3D11RenderLib;
	friend class				CD3D10Texture;
	friend class				CD3D10SwapChain;
public:
								~ShaderAPID3DX10();
								ShaderAPID3DX10();

	void						SetD3DDevice(ID3D10Device* d3ddev);

	bool						CreateBackbufferDepth(int wide, int tall, DXGI_FORMAT depthFormat, int nMSAASamples);
	void						ReleaseBackbufferDepth();


	// Init + Shurdown
	void						Init(const shaderAPIParams_t &params);
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

	void						Clear(bool bClearColor,bool bClearDepth, bool bClearStencil, const ColorRGBA &fillColor,float fDepth, int nStencil);

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

	// shader API class type for shader developers.
	ER_ShaderAPIType			GetShaderAPIClass() {return SHADERAPI_DIRECT3D10;}

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
	
	// Create procedural texture such as error texture, etc.
	ITexturePtr					CreateProceduralTexture(const char* pszName,int width, int height, const unsigned char* data, int nDataSize, ETextureFormat nFormat, ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP, int nFlags = 0);

	// It will add new rendertarget
	ITexturePtr					CreateRenderTarget(int width, int height,ETextureFormat nRTFormat,ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR, ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP, ER_CompareFunc comparison = COMPFUNC_NEVER, int nFlags = 0);

	// It will add new rendertarget
	ITexturePtr					CreateNamedRenderTarget(const char* pszName,int width, int height, ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR, ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP, ER_CompareFunc comparison = COMPFUNC_NEVER, int nFlags = 0);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// Copy render target to texture
	void						CopyFramebufferToTexture(const ITexturePtr& renderTarget);

	// Copy render target to texture
	void						CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect = nullptr, IAARectangle* destRect = nullptr);

	// Changes render target (MRT)
	void						ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets, ArrayCRef<int> rtSlice, const ITexturePtr& depthTarget, int depthSlice);

	// Changes back to backbuffer
	void						ChangeRenderTargetToBackBuffer();

	// resizes render target
	void						ResizeRenderTarget(const ITexturePtr& renderTarget, int newWide, int newTall);

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
	void						SetScissorRectangle( const IAARectangle &rect );

	// Set the texture. Animation is set from ITexture every frame (no affection on speed) before you do 'ApplyTextures'
	// Also you need to specify texture name. If you don't, use registers (not fine with DX10, 11)
	void						SetTexture(int nameHash, const ITexturePtr& pTexture);

//-------------------------------------------------------------
// Vertex and index buffers
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
															const char* extra = nullptr);

	// Set current shader for rendering
	void						SetShader(IShaderProgram* pShader);

	// RAW Constant (Used for structure types, etc.)
	void						SetShaderConstantRaw(int nameHash, const void *data, int nSize);

//-------------------------------------------------------------
// State manipulation 
//-------------------------------------------------------------

	// creates blending state
	IRenderState*				CreateBlendingState( const BlendStateParam_t &blendDesc );
	
	// creates depth/stencil state
	IRenderState*				CreateDepthStencilState( const DepthStencilStateParams_t &depthDesc );

	// creates rasterizer state
	IRenderState*				CreateRasterizerState( const RasterizerStateParams_t &rasterDesc );

	// creates sampler state
	IRenderState*				CreateSamplerState( const SamplerStateParams &samplerDesc );

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
// Internal
//-------------------------------------------------------------

	ID3D10ShaderResourceView*	TexResource_CreateShaderResourceView(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, int firstSlice = -1, int sliceCount = -1);
	ID3D10RenderTargetView*		TexResource_CreateShaderRenderTargetView(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, int firstSlice = -1, int sliceCount = -1);
	ID3D10DepthStencilView*		TexResource_CreateShaderDepthStencilView(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, int firstSlice = -1, int sliceCount = -1);


protected:
	static void					InternalCreateDepthTarget(CD3D10Texture* pTexture, ID3D10Device* pDevice);
	static void					InternalCreateRenderTarget(CD3D10Texture* pTexture, ID3D10Device* pDevice);

	void						CreateTextureInternal(ITexture** pTex, const Array<CImage*>& pImages, const SamplerStateParams& sSamplingParams,int nFlags = 0);
	ID3D10Resource*				CreateD3DTextureFromImage(CImage* pSrc, int& wide, int& tall, int nFlags = 0);
	
	// Creates empty texture resource.
	ITexturePtr					CreateTextureResource(const char* pszName);

	static bool					FillShaderResourceView(ID3D10SamplerState** samplers, ID3D10ShaderResourceView** dest, int& min, int& max, ITexturePtr* selectedTextures, ITexturePtr* currentTextures, const int selectedTextureSlices[], int currentTextureSlices[]);
	static bool					InternalFillSamplerState(ID3D10SamplerState** dest, int& min, int& max, ITexturePtr* selectedTextures, ITexturePtr* currentTextures);

	//void						InternalSetupSampler(uint texTarget,SamplerStateParams *sSamplingParams);
	//void						InternalChangeFrontFace(int nCullFaceMode);

	//ID3D10RenderTargetView*		m_pBackBufferRTV{ nullptr };
	ID3D10DepthStencilView*		m_pDepthBufferDSV{ nullptr };
	//ID3D10Texture2D*			m_pBackBuffer{ nullptr };
	ID3D10Texture2D*			m_pDepthBuffer{ nullptr };

	ITexturePtr					m_pBackBufferTexture;
	ITexturePtr					m_pDepthBufferTexture;

	ITexturePtr					m_pCurrentTexturesVS[MAX_TEXTUREUNIT];
	ITexturePtr					m_pCurrentTexturesGS[MAX_TEXTUREUNIT];
	ITexturePtr					m_pCurrentTexturesPS[MAX_TEXTUREUNIT];
	ITexturePtr					m_pSelectedTexturesVS[MAX_TEXTUREUNIT];
	ITexturePtr					m_pSelectedTexturesGS[MAX_TEXTUREUNIT];
	ITexturePtr					m_pSelectedTexturesPS[MAX_TEXTUREUNIT];

	int							m_pCurrentTextureSlicesVS[MAX_TEXTUREUNIT];
	int							m_pCurrentTextureSlicesGS[MAX_TEXTUREUNIT];
	int							m_pCurrentTextureSlicesPS[MAX_TEXTUREUNIT];

	int							m_pSelectedTextureSlicesVS[MAX_TEXTUREUNIT];
	int							m_pSelectedTextureSlicesGS[MAX_TEXTUREUNIT];
	int							m_pSelectedTextureSlicesPS[MAX_TEXTUREUNIT];

	// Sampler states is not binding same as OpenGL
	SamplerStateParams*		m_pSelectedSamplerStatesVS[MAX_SAMPLERSTATE];
	SamplerStateParams*		m_pSelectedSamplerStatesGS[MAX_SAMPLERSTATE];
	SamplerStateParams*		m_pSelectedSamplerStatesPS[MAX_SAMPLERSTATE];

	SamplerStateParams*		m_pCurrentSamplerStatesVS[MAX_SAMPLERSTATE];
	SamplerStateParams*		m_pCurrentSamplerStatesGS[MAX_SAMPLERSTATE];
	SamplerStateParams*		m_pCurrentSamplerStatesPS[MAX_SAMPLERSTATE];

	int							m_nCurrentDepthSlice{ 0 };

	// Custom blend state
	SamplerStateParams*		m_pCustomSamplerState{ nullptr };
	int							m_nCurrentFrontFace;

	ID3D10Device*				m_pD3DDevice{ nullptr };
	ID3D10Query*				m_pEventQuery{ nullptr };

	int							m_nCurrentSampleMask{ -1 };
	int							m_nSelectedSampleMask{ -1 };
	bool						m_deviceIsLost{ false };
};
