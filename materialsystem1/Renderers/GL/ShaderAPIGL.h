//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef SHADERAPIGL_H
#define SHADERAPIGL_H

#include "../Shared/ShaderAPI_Base.h"

#ifdef USE_GLES2

#include <glad_es3.h>
#ifdef PLAT_ANDROID // direct link
#include <EGL/egl.h>
#else
#include <glad_egl.h>
#endif // PLAT_ANDROID

#else
#include <glad.h>
#endif


enum EGraphicsVendor
{
	VENDOR_ATI,
	VENDOR_NV,
	VENDOR_INTEL,
	VENDOR_OTHER,
};

enum EGLTextureType
{
	GLTEX_TYPE_ERROR = -1,

	GLTEX_TYPE_TEXTURE = 0,
	GLTEX_TYPE_CUBETEXTURE,
	GLTEX_TYPE_VOLUMETEXTURE,
};

struct GLTextureRef_t
{
	GLuint glTexID;
	EGLTextureType type;
};

class 		CVertexFormatGL;
class 		CVertexBufferGL;
class 		CIndexBufferGL;
class		CGLTexture;
class		CGLRenderLib;
class		CGLShaderProgram;

class ShaderAPIGL : public ShaderAPI_Base
{
public:

	friend class 		CVertexFormatGL;
	friend class 		CVertexBufferGL;
	friend class 		CIndexBufferGL;
	friend class		CGLTexture;
	friend class		CGLRenderLib;
	friend class		CGLShaderProgram;
	friend class		CGLOcclusionQuery;

	friend class		GLWorkerThread;

						~ShaderAPIGL();
						ShaderAPIGL();

	// Init + Shurdown
	void				Init( shaderAPIParams_t &params);
	void				Shutdown();

	void				PrintAPIInfo();

	bool				IsDeviceActive();

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

	// Unload the texture and free the memory
	void				FreeTexture(ITexture* pTexture);

	// Create procedural texture such as error texture, etc.
	ITexture*			CreateProceduralTexture(const char* pszName,
												int width, int height,
												const unsigned char* data, int nDataSize,
												ETextureFormat nFormat,
												ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
												int nFlags = 0);

	// It will add new rendertarget
	ITexture*			CreateRenderTarget(	int width, int height,
											ETextureFormat nRTFormat,
											ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR,
											ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
											ER_CompareFunc comparison = COMP_NEVER,
											int nFlags = 0);
	// It will add new rendertarget
	ITexture*			CreateNamedRenderTarget(const char* pszName,
												int width, int height,
												ETextureFormat nRTFormat, ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR,
												ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
												ER_CompareFunc comparison = COMP_NEVER,
												int nFlags = 0);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// saves rendertarget to texture, you can also save screenshots
	void				SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName);

	// Copy render target to texture
	void				CopyFramebufferToTexture(ITexture* pTargetTexture);

	// Copy render target to texture
	void				CopyRendertargetToTexture(ITexture* srcTarget, ITexture* destTex, IRectangle* srcRect = NULL, IRectangle* destRect = NULL);

	// Changes render target (MRT)
	void				ChangeRenderTargets(ITexture** pRenderTargets, int nNumRTs, int* nCubemapFaces = NULL, ITexture* pDepthTarget = NULL, int nDepthSlice = 0);

	// fills the current rendertarget buffers
	void				GetCurrentRenderTargets(ITexture* pRenderTargets[MAX_MRTS], int *nNumRTs, ITexture** pDepthTarget, int cubeNumbers[MAX_MRTS]);

	// Changes back to backbuffer
	void				ChangeRenderTargetToBackBuffer();

	// resizes render target
	void				ResizeRenderTarget(ITexture* pRT, int newWide, int newTall);

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
	void				SetScissorRectangle( const IRectangle &rect );

	// Set the texture. Animation is set from ITexture every frame (no affection on speed) before you do 'ApplyTextures'
	// Also you need to specify texture name. If you don't, use registers (not fine with DX10, 11)
	void				SetTexture( ITexture* pTexture, const char* pszName, int index = 0 );

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
	IShaderProgram*		CreateNewShaderProgram(const char* pszName, const char* query = NULL);

	// Destroy all shader
	void				DestroyShaderProgram(IShaderProgram* pShaderProgram);

	// Load any shader from stream
	bool				CompileShadersFromStream(	IShaderProgram* pShaderOutput,
													const shaderProgramCompileInfo_t& info,
													const char* extra = NULL
													);

	// Set current shader for rendering
	void				SetShader(IShaderProgram* pShader);

	// RAW Constant (Used for structure types, etc.)
	int					SetShaderConstantRaw(const char *pszName, const void *data, int nSize, int nConstId);

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

	IVertexFormat*		CreateVertexFormat(const char* name, VertexFormatDesc_s *formatDesc, int nAttribs);
	IVertexBuffer*		CreateVertexBuffer(ER_BufferAccess nBufAccess, int nNumVerts, int strideSize, void *pData = NULL);
	IIndexBuffer*		CreateIndexBuffer(int nIndices, int nIndexSize, ER_BufferAccess nBufAccess, void *pData = NULL);

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

	// Indexed primitive drawer
	void				DrawIndexedPrimitives(ER_PrimitiveType nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex = 0);

	// Draw elements
	void				DrawNonIndexedPrimitives(ER_PrimitiveType nType, int nFirstVertex, int nVertices);

protected:

	void				CreateTextureInternal(ITexture** pTex, const Array<CImage*>& pImages, const SamplerStateParam_t& sampler,int nFlags = 0);
	GLTextureRef_t		CreateGLTextureFromImage(CImage* pSrc, const SamplerStateParam_t& sampler, int& wide, int& tall, int nFlags);

private:
	//OpenGL - Specific
	void					SetupGLSamplerState(uint texTarget, const SamplerStateParam_t& sSamplingParams, int mipMapCount = 1);
	void					InternalChangeFrontFace(int nCullFaceMode);

	GLuint					m_frameBuffer;
	GLuint					m_depthBuffer;

	GLuint					m_currentGLDepth;
	
	GLenum					m_drawBuffers[MAX_MRTS];

	int						m_boundInstanceStream;
	uint					m_currentGLVB[MAX_VERTEXSTREAM];
	uint					m_currentGLIB;

	GLTextureRef_t			m_currentGLTextures[MAX_TEXTUREUNIT];

	int						m_nCurrentRenderTargets;

	int						m_nCurrentFrontFace;

	int						m_nCurrentSrcFactor;
	int						m_nCurrentDstFactor;
	int						m_nCurrentBlendFunc;

	float					m_fCurrentDepthBias;
	float					m_fCurrentSlopeDepthBias;

	int						m_nCurrentDepthFunc;
	bool					m_bCurrentDepthTestEnable;
	bool					m_bCurrentDepthWriteEnable;

	bool					m_bCurrentMultiSampleEnable;
	bool					m_bCurrentScissorEnable;
	int						m_nCurrentCullMode;
	int						m_nCurrentFillMode;

	int						m_nCurrentMask;
	bool					m_bCurrentBlendEnable;

	uint					m_nCurrentVBO;

	IRectangle				m_viewPort;

	int						m_nCurrentMatrixMode;
	Matrix4x4				m_matrices[4];
	EGraphicsVendor			m_vendor;
};

#endif // SHADERAPIGL_H
