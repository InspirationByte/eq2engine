//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/ShaderAPI_Base.h"
#include "GLTexture.h"

enum EGraphicsVendor
{
	VENDOR_OTHER = -1,

	VENDOR_ATI,
	VENDOR_NV,
	VENDOR_INTEL,
};

class CGLRenderLib;
class CVertexFormatGL;
class CVertexBufferGL;
class CIndexBufferGL;
class CGLTexture;
class CGLShaderProgram;
class CGLOcclusionQuery;
class GLWorkerThread;

class ShaderAPIGL : public ShaderAPI_Base
{
public:
	friend class		CGLRenderLib_WGL;
	friend class		CGLRenderLib_EGL;
	friend class		CGLRenderLib_SDL;
	friend class		CGLRenderLib_GLX;

	friend class 		CVertexFormatGL;
	friend class 		CVertexBufferGL;
	friend class 		CIndexBufferGL;
	friend class		CGLTexture;
	friend class		CGLShaderProgram;
	friend class		CGLOcclusionQuery;
	friend class		GLWorkerThread;

	// Init + Shurdown
	void				Init( const shaderAPIParams_t &params);
	void				Shutdown();

	void				PrintAPIInfo() const;

	bool				IsDeviceActive() const;

//-------------------------------------------------------------
// Rendering's applies
//-------------------------------------------------------------
	void				Reset(int nResetType = STATE_RESET_ALL);

	void				ApplyTextures();
	void				ApplySamplerState();
	void				ApplyBlendState();
	void				ApplyDepthState();
	void				ApplyRasterizerState();
	void				ApplyShaderProgram();
	void				ApplyConstants();

	void				Clear(	bool bClearColor,
								bool bClearDepth = true,
								bool bClearStencil = true,
								const ColorRGBA &fillColor = ColorRGBA(0),
								float fDepth = 1.0f,
								int nStencil = 0
								);

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

	// Device vendor and version
	const char*			GetDeviceNameString() const;

	// Renderer string (ex: OpenGL, D3D9)
	const char*			GetRendererName() const;

	ER_ShaderAPIType	GetShaderAPIClass() const {return SHADERAPI_OPENGL;}

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// Synchronization
	void				Flush();
	void				Finish();

//-------------------------------------------------------------
// State manipulation
//-------------------------------------------------------------

	// creates blending state
	IRenderState*		CreateBlendingState( const BlendStateParam_t &blendDesc );

	// creates depth/stencil state
	IRenderState*		CreateDepthStencilState( const DepthStencilStateParams_t &depthDesc );

	// creates rasterizer state
	IRenderState*		CreateRasterizerState( const RasterizerStateParams_t &rasterDesc );

	// completely destroys shader
	void				DestroyRenderState( IRenderState* pState, bool removeAllRefs = false );

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------

	// creates occlusion query object
	IOcclusionQuery*	CreateOcclusionQuery();

	// removal of occlusion query object
	void				DestroyOcclusionQuery(IOcclusionQuery* pQuery);

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// Create procedural texture such as error texture, etc.
	ITexturePtr			CreateProceduralTexture(const char* pszName,
												int width, int height,
												const unsigned char* data, int nDataSize,
												ETextureFormat nFormat,
												ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
												int nFlags = 0);

	// It will add new rendertarget
	ITexturePtr			CreateRenderTarget(	int width, int height,
											ETextureFormat nRTFormat,
											ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR,
											ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
											ER_CompareFunc comparison = COMPFUNC_NEVER,
											int nFlags = 0);
	// It will add new rendertarget
	ITexturePtr			CreateNamedRenderTarget(const char* pszName,
												int width, int height,
												ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR,
												ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
												ER_CompareFunc comparison = COMPFUNC_NEVER,
												int nFlags = 0);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------


	// Copy render target to texture
	void				CopyFramebufferToTexture(const ITexturePtr& renderTarget);

	// Copy render target to texture
	void				CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect = nullptr, IAARectangle* destRect = nullptr);

	// Changes render target (MRT)
	void				ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets,
											ArrayCRef<int> rtSlice = nullptr,
											const ITexturePtr& depthTarget = nullptr,
											int depthSlice = 0);

	// Changes back to backbuffer
	void				ChangeRenderTargetToBackBuffer();

	// resizes render target
	void				ResizeRenderTarget(const ITexturePtr& renderTarget, int newWide, int newTall);

//-------------------------------------------------------------
// Matrix for rendering
//-------------------------------------------------------------

	// Matrix mode
	void				SetMatrixMode(ER_MatrixMode nMatrixMode);

	// Will save matrix
	void				PushMatrix();

	// Will reset matrix
	void				PopMatrix();

	// Load identity matrix
	void				LoadIdentityMatrix();

	// Load custom matrix
	void				LoadMatrix(const Matrix4x4 &matrix);

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

	// Set Depth range for next primitives
	void				SetDepthRange(float fZNear,float fZFar);

	// sets viewport
	void				SetViewport(int x, int y, int w, int h);

	// returns viewport
	void				GetViewport(int &x, int &y, int &w, int &h);

	// returns current size of viewport
	void				GetViewportDimensions(int &wide, int &tall);

	// sets scissor rectangle
	void				SetScissorRectangle( const IAARectangle &rect );

	// Set the texture. Animation is set from ITexture every frame (no affection on speed) before you do 'ApplyTextures'
	// Also you need to specify texture name. If you don't, use registers (not fine with DX10, 11)
	void				SetTexture(int nameHash, const ITexturePtr& texture);

	int					GetSamplerUnit(CGLShaderProgram* prog, const char* samplerName);

//-------------------------------------------------------------
// Vertex buffer object handling
//-------------------------------------------------------------

	// Changes the vertex format
	void				ChangeVertexFormat(IVertexFormat* pVertexFormat);

	// Changes the vertex buffer
	void				ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset = 0);

	// Changes the index buffer
	void				ChangeIndexBuffer(IIndexBuffer* pIndexBuffer);

	// Destroy vertex format
	void				DestroyVertexFormat(IVertexFormat* pFormat);

	// Destroy vertex buffer
	void				DestroyVertexBuffer(IVertexBuffer* pVertexBuffer);

	// Destroy index buffer
	void				DestroyIndexBuffer(IIndexBuffer* pIndexBuffer);

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// Creates shader class for needed ShaderAPI
	IShaderProgram*		CreateNewShaderProgram(const char* pszName, const char* query = nullptr);

	// Destroy all shader
	void				DestroyShaderProgram(IShaderProgram* pShaderProgram);

	// Load any shader from stream
	bool				CompileShadersFromStream(	IShaderProgram* pShaderOutput,
													const shaderProgramCompileInfo_t& info,
													const char* extra = nullptr
													);

	// Set current shader for rendering
	void				SetShader(IShaderProgram* pShader);

	// RAW Constant (Used for structure types, etc.)
	void				SetShaderConstantRaw(int nameHash, const void *data, int nSize);

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

	IVertexFormat*		CreateVertexFormat(const char* name, ArrayCRef<VertexFormatDesc> formatDesc);
	IVertexBuffer*		CreateVertexBuffer(ER_BufferAccess nBufAccess, int nNumVerts, int strideSize, void *pData = nullptr);
	IIndexBuffer*		CreateIndexBuffer(int nIndices, int nIndexSize, ER_BufferAccess nBufAccess, void *pData = nullptr);

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

	// Indexed primitive drawer
	void				DrawIndexedPrimitives(ER_PrimitiveType nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex = 0);

	// Draw elements
	void				DrawNonIndexedPrimitives(ER_PrimitiveType nType, int nFirstVertex, int nVertices);

protected:

	ITexturePtr			CreateTextureResource(const char* pszName);
private:
	void				ApplyBuffers();

	void				StepProgressiveLodTextures();

	void				PreloadShadersFromCache();
	bool				InitShaderFromCache(IShaderProgram* pShaderOutput, IVirtualStream* pStream, uint32 checksum = 0);

	//OpenGL - Specific
	void				InternalChangeFrontFace(int nCullFaceMode);

	Set<CGLTexture*>	m_progressiveTextures{ PP_SL };

	GLuint				m_frameBuffer{ 0 };
	GLuint				m_depthBuffer{ 0 };

	GLenum				m_drawBuffers[MAX_MRTS]{ 0 };
	GLuint				m_currentGLDepth{ GL_NONE };

	uint				m_currentGLVB[MAX_VERTEXSTREAM];
	uint				m_currentGLIB{ 0 };
	GLuint				m_drawVAO{ 0 };
	int					m_boundInstanceStream{ -1 };
	
	GLTextureRef_t		m_currentGLTextures[MAX_TEXTUREUNIT];
	int					m_nCurrentRenderTargets{ 0 };

	int					m_nCurrentFrontFace{ 0 };

	int					m_nCurrentSrcFactor{ BLENDFACTOR_ONE };
	int					m_nCurrentDstFactor{ BLENDFACTOR_ZERO };
	int					m_nCurrentBlendFunc{ BLENDFUNC_ADD };

	float				m_fCurrentDepthBias{ 0.0f };
	float				m_fCurrentSlopeDepthBias{ 0.0f };

	bool				m_bCurrentMultiSampleEnable{ false };
	bool				m_bCurrentScissorEnable{ false };
	int					m_nCurrentCullMode{ CULL_BACK };
	int					m_nCurrentFillMode{ FILL_SOLID };

	int					m_nCurrentMask{ COLORMASK_ALL };
	bool				m_bCurrentBlendEnable{ false };

	IAARectangle			m_viewPort;
	EGraphicsVendor		m_vendor{ VENDOR_OTHER };
	bool				m_deviceLost{ false };
};

extern ShaderAPIGL g_shaderApi;

bool GLCheckError(const char* op, ...);
void PrintGLExtensions();
void InitGLHardwareCapabilities(ShaderAPICaps_t& caps);
