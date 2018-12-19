//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Direct3D 10 ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef ShaderAPID3DX10_H
#define ShaderAPID3DX10_H

#include "../Shared/ShaderAPI_Base.h"

#include <d3d10.h>
#include <d3dx10.h>
#include "D3D10MeshBuilder.h"

class ShaderAPID3DX10 : public ShaderAPI_Base
{
public:

	friend class 				CVertexFormatD3D10;
	friend class 				CVertexBufferD3D10;
	friend class 				CIndexBufferD3D10;
	friend class				CD3D10ShaderProgram;
	friend class				CD3DRenderLib;
	friend class				CD3D10Texture;
	friend class				CD3D10SwapChain;

								~ShaderAPID3DX10();
								ShaderAPID3DX10();

	void						SetD3DDevice(ID3D10Device* d3ddev);

	bool						CreateBackbufferDepth(int wide, int tall, DXGI_FORMAT depthFormat, int nMSAASamples);
	void						ReleaseBackbufferDepth();


	// Init + Shurdown
	void						Init(const shaderAPIParams_t &params);
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

	void						Clear(bool bClearColor,bool bClearDepth, bool bClearStencil, const ColorRGBA &fillColor,float fDepth, int nStencil);

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

	// shader API class type for shader developers.
	// DON'T USE TYPES IN DYNAMIC SHADER CODE! USE MATSYSTEM MAT-FILE DEFS!
	ER_ShaderAPIType			GetShaderAPIClass() {return SHADERAPI_DIRECT3D10;}

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

	// The hardware (for engine) is supports full dynamic lighting
	//bool						IsSupportsFullLighting() const;

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// Synchronization
	void						Flush();
	void						Finish();

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------
	/*
	void						OcclusionQuery_Begin();
	int							OcclusionQuery_Result();
	*/

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// Unload the texture and free the memory
	void						FreeTexture(ITexture* pTexture);
	
	// Create procedural texture such as error texture, etc.
	ITexture*					CreateProceduralTexture(const char* pszName,int width, int height, const unsigned char* data, int nDataSize, ETextureFormat nFormat, ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP, int nFlags = 0);

	// It will add new rendertarget
	ITexture*					CreateRenderTarget(int width, int height,ETextureFormat nRTFormat,ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR, ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP, ER_CompareFunc comparison = COMP_NEVER, int nFlags = 0);

	// It will add new rendertarget
	ITexture*					CreateNamedRenderTarget(const char* pszName,int width, int height, ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR, ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP, ER_CompareFunc comparison = COMP_NEVER, int nFlags = 0);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// saves rendertarget to texture, you can also save screenshots
	void						SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName);

	// Copy render target to texture
	void						CopyFramebufferToTexture(ITexture* pTargetTexture);

	// Copy render target to texture with resizing
	//void						CopyFramebufferToTextureEx(ITexture* pTargetTexture,int srcX0 = -1, int srcY0 = -1,int srcX1 = -1, int srcY1 = -1,int destX0 = -1, int destY0 = -1,int destX1 = -1, int destY1 = -1);

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

	// Creates new mesh builder
	IMeshBuilder*				CreateMeshBuilder();

	void						DestroyMeshBuilder(IMeshBuilder* pBuilder);

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// search for existing shader program
	IShaderProgram*				FindShaderProgram(const char* pszName, const char* query = NULL);

	// Creates shader class for needed ShaderAPI
	IShaderProgram*				CreateNewShaderProgram(const char* pszName, const char* query = NULL);

	// Destroy all shader
	void						DestroyShaderProgram(IShaderProgram* pShaderProgram);

	// Load any shader from stream
	bool						CompileShadersFromStream(	IShaderProgram* pShaderOutput,
															const shaderProgramCompileInfo_t& info,
															const char* extra = NULL);

	// Set current shader for rendering
	void						SetShader(IShaderProgram* pShader);

	// Set Texture for shader
	void						SetShaderTexture(const char* pszName, ITexture* pTexture);

	// RAW Constant (Used for structure types, etc.)
	int							SetShaderConstantRaw(const char *pszName, const void *data, int nSize, int nConstID);


	//-----------------------------------------------------
	// Advanced shader programming
	//-----------------------------------------------------
	/*
	// Pixel Shader constants setup (by register number)
	void						SetPixelShaderConstantInt(int reg, const int constant);
	void						SetPixelShaderConstantFloat(int reg, const float constant);
	void						SetPixelShaderConstantVector2D(int reg, const Vector2D &constant);
	void						SetPixelShaderConstantVector3D(int reg, const Vector3D &constant);
	void						SetPixelShaderConstantVector4D(int reg, const Vector4D &constant);
	void						SetPixelShaderConstantMatrix4(int reg, const Matrix4x4 &constant);
	void						SetPixelShaderConstantFloatArray(int reg, const float *constant, int count);
	void						SetPixelShaderConstantVector2DArray(int reg, const Vector2D *constant, int count);
	void						SetPixelShaderConstantVector3DArray(int reg, const Vector3D *constant, int count);
	void						SetPixelShaderConstantVector4DArray(int reg, const Vector4D *constant, int count);
	void						SetPixelShaderConstantMatrix4Array(int reg, const Matrix4x4 *constant, int count);

	// Vertex Shader constants setup (by register number)
	void						SetVertexShaderConstantInt(int reg, const int constant);
	void						SetVertexShaderConstantFloat(int reg, const float constant);
	void						SetVertexShaderConstantVector2D(int reg, const Vector2D &constant);
	void						SetVertexShaderConstantVector3D(int reg, const Vector3D &constant);
	void						SetVertexShaderConstantVector4D(int reg, const Vector4D &constant);
	void						SetVertexShaderConstantMatrix4(int reg, const Matrix4x4 &constant);
	void						SetVertexShaderConstantFloatArray(int reg, const float *constant, int count);
	void						SetVertexShaderConstantVector2DArray(int reg, const Vector2D *constant, int count);
	void						SetVertexShaderConstantVector3DArray(int reg, const Vector3D *constant, int count);
	void						SetVertexShaderConstantVector4DArray(int reg, const Vector4D *constant, int count);
	void						SetVertexShaderConstantMatrix4Array(int reg, const Matrix4x4 *constant, int count);
	
	// RAW Constant
	void						SetPixelShaderConstantRaw(int reg, const void *data, int nVectors = 1);
	void						SetVertexShaderConstantRaw(int reg, const void *data, int nVectors = 1);
	*/
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
	IRenderState*				CreateSamplerState( const SamplerStateParam_t &samplerDesc );

	// completely destroys shader
	void						DestroyRenderState( IRenderState* pShaderProgram, bool removeAllRefs = false );

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

	IVertexFormat*				CreateVertexFormat(VertexFormatDesc_s *formatDesc, int nAttribs);
	IVertexBuffer*				CreateVertexBuffer(ER_BufferAccess nBufAccess, int nNumVerts, int strideSize, void *pData = NULL);
	IIndexBuffer*				CreateIndexBuffer(int nIndices, int nIndexSize, ER_BufferAccess nBufAccess, void *pData = NULL);

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

	// Indexed primitive drawer
	void						DrawIndexedPrimitives(ER_PrimitiveType nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex = 0);

	// Draw elements
	void						DrawNonIndexedPrimitives(ER_PrimitiveType nType, int nFirstVertex, int nVertices);

	// mesh buffer FFP emulation
	void						DrawMeshBufferPrimitives(ER_PrimitiveType nType, int nVertices, int nIndices);

//-------------------------------------------------------------
// Internal
//-------------------------------------------------------------

	ID3D10ShaderResourceView*	TexResource_CreateShaderResourceView(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, int firstSlice = -1, int sliceCount = -1);
	ID3D10RenderTargetView*		TexResource_CreateShaderRenderTargetView(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, int firstSlice = -1, int sliceCount = -1);
	ID3D10DepthStencilView*		TexResource_CreateShaderDepthStencilView(ID3D10Resource *resource, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN, int firstSlice = -1, int sliceCount = -1);

	void						CreateTextureInternal(ITexture** pTex, const DkList<CImage*>& pImages, const SamplerStateParam_t& sSamplingParams,int nFlags = 0);

protected:

	ID3D10Resource*				CreateD3DTextureFromImage(CImage* pSrc, int& wide, int& tall, int nFlags = 0);

	//void						AddTextureInternal(ITexture** pTex, CImage *texImage,SamplerStateParam_t& sSamplingParams,int nFlags = 0);
	//void						AddAnimatedTextureInternal(ITexture** pTex, CImage **texImage, int numTextures, SamplerStateParam_t& sSamplingParams,int nFlags = 0);

	int							GetSamplerUnit(IShaderProgram* pProgram,const char* pszSamplerName);

private:
	//OpenGL - Specific
	//void						InternalSetupSampler(uint texTarget,SamplerStateParam_t *sSamplingParams);
	//void						InternalChangeFrontFace(int nCullFaceMode);

	//ID3D10RenderTargetView*		m_pBackBufferRTV;
	ID3D10DepthStencilView*		m_pDepthBufferDSV;
	//ID3D10Texture2D*			m_pBackBuffer;
	ID3D10Texture2D*			m_pDepthBuffer;

	ITexture*					m_pBackBufferTexture;
	ITexture*					m_pDepthBufferTexture;

	ITexture*					m_pCurrentTexturesVS[MAX_TEXTUREUNIT];
	ITexture*					m_pCurrentTexturesGS[MAX_TEXTUREUNIT];
	ITexture*					m_pCurrentTexturesPS[MAX_TEXTUREUNIT];

	ITexture*					m_pSelectedTexturesVS[MAX_TEXTUREUNIT];
	ITexture*					m_pSelectedTexturesGS[MAX_TEXTUREUNIT];
	ITexture*					m_pSelectedTexturesPS[MAX_TEXTUREUNIT];

	int							m_pCurrentTextureSlicesVS[MAX_TEXTUREUNIT];
	int							m_pCurrentTextureSlicesGS[MAX_TEXTUREUNIT];
	int							m_pCurrentTextureSlicesPS[MAX_TEXTUREUNIT];

	int							m_pSelectedTextureSlicesVS[MAX_TEXTUREUNIT];
	int							m_pSelectedTextureSlicesGS[MAX_TEXTUREUNIT];
	int							m_pSelectedTextureSlicesPS[MAX_TEXTUREUNIT];

	// Sampler states is not binding same as OpenGL
	SamplerStateParam_t*		m_pSelectedSamplerStatesVS[MAX_SAMPLERSTATE];
	SamplerStateParam_t*		m_pSelectedSamplerStatesGS[MAX_SAMPLERSTATE];
	SamplerStateParam_t*		m_pSelectedSamplerStatesPS[MAX_SAMPLERSTATE];

	SamplerStateParam_t*		m_pCurrentSamplerStatesVS[MAX_SAMPLERSTATE];
	SamplerStateParam_t*		m_pCurrentSamplerStatesGS[MAX_SAMPLERSTATE];
	SamplerStateParam_t*		m_pCurrentSamplerStatesPS[MAX_SAMPLERSTATE];

	int							m_nCurrentDepthSlice;

	// Custom blend state
	SamplerStateParam_t*		m_pCustomSamplerState;

	int							m_nCurrentFrontFace;

	IShaderProgram*				m_pMeshBufferNoTextureShader;
	IShaderProgram*				m_pMeshBufferTexturedShader;

	ID3D10Device*				m_pD3DDevice;
	ID3D10Query*				m_pEventQuery;

	Vector4D					m_vScaleBias2D;

	Vector4D					m_vCurColor;

	int							m_nCurrentSampleMask;
	int							m_nSelectedSampleMask;

	ER_MatrixMode				m_nCurrentMatrixMode;
	Matrix4x4					m_matrices[4];

	bool						m_bDeviceIsLost;
};

#endif // ShaderAPID3DX10_H
