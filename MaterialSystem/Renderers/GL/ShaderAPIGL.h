//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef SHADERAPIGL_H
#define SHADERAPIGL_H

#include "../ShaderAPI_Base.h"

#ifdef USE_GLES2
#include "glad_es3.h"
#include <EGL/egl.h>
#else
#include "glad.h"
#endif

#ifdef LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <X11/extensions/xf86vmode.h>

typedef XID GLXContextID;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef XID GLXPbuffer;
typedef XID GLXWindow;
typedef XID GLXFBConfigID;
typedef struct __GLXcontextRec *GLXContext;
typedef struct __GLXFBConfigRec *GLXFBConfig;
#endif

#include "VertexFormatGL.h"

//#define USE_OPENGL_ES

enum EGraphicsVendor
{
	VENDOR_ATI,
	VENDOR_NV,
	VENDOR_INTEL,
	VENDOR_OTHER,
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

						~ShaderAPIGL();
						ShaderAPIGL();

	// Init + Shurdown
	void				Init(const shaderapiinitparams_t &params);
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

	// Render targetting support
	bool				IsSupportsRendertargetting() const;

	// Render targetting support for Multiple RTs
	bool				IsSupportsMRT() const;

	// Supports multitexturing???
	bool				IsSupportsMultitexturing() const;

	// The driver/hardware is supports Pixel shaders?
	bool				IsSupportsPixelShaders() const;

	// The driver/hardware is supports Vertex shaders?
	bool				IsSupportsVertexShaders() const;

	// The driver/hardware is supports Geometry shaders?
	bool				IsSupportsGeometryShaders() const;

	// The driver/hardware is supports Domain shaders?
	bool				IsSupportsDomainShaders() const;

	// The driver/hardware is supports Hull (tessellator) shaders?
	bool				IsSupportsHullShaders() const;


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
												AddressMode_e textureAddress = ADDRESSMODE_WRAP,
												int nFlags = 0);

	// It will add new rendertarget
	ITexture*			CreateRenderTarget(	int width, int height,
											ETextureFormat nRTFormat,
											Filter_e textureFilterType = TEXFILTER_LINEAR,
											AddressMode_e textureAddress = ADDRESSMODE_WRAP,
											CompareFunc_e comparison = COMP_NEVER,
											int nFlags = 0);
	// It will add new rendertarget
	ITexture*			CreateNamedRenderTarget(const char* pszName,
												int width, int height,
												ETextureFormat nRTFormat, Filter_e textureFilterType = TEXFILTER_LINEAR,
												AddressMode_e textureAddress = ADDRESSMODE_WRAP,
												CompareFunc_e comparison = COMP_NEVER,
												int nFlags = 0);

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// saves rendertarget to texture, you can also save screenshots
	void				SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName);

	// Copy render target to texture
	void				CopyFramebufferToTexture(ITexture* pTargetTexture);

	// Copy render target to texture with resizing
	//void				CopyFramebufferToTextureEx(ITexture* pTargetTexture,int srcX0 = -1, int srcY0 = -1,int srcX1 = -1, int srcY1 = -1,int destX0 = -1, int destY0 = -1,int destX1 = -1, int destY1 = -1);

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
	void				SetMatrixMode(MatrixMode_e nMatrixMode);

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

	// Creates new mesh builder
	IMeshBuilder*		CreateMeshBuilder();

	// destroys mesh builder
	void				DestroyMeshBuilder(IMeshBuilder* pBuilder);

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// search for existing shader program
	IShaderProgram*		FindShaderProgram(const char* pszName, const char* query = NULL);

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

	IVertexFormat*		CreateVertexFormat(VertexFormatDesc_s *formatDesc, int nAttribs);
	IVertexBuffer*		CreateVertexBuffer(BufferAccessType_e nBufAccess, int nNumVerts, int strideSize, void *pData = NULL);
	IIndexBuffer*		CreateIndexBuffer(int nIndices, int nIndexSize, BufferAccessType_e nBufAccess, void *pData = NULL);

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

	// Indexed primitive drawer
	void				DrawIndexedPrimitives(PrimitiveType_e nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex = 0);

	// Draw elements
	void				DrawNonIndexedPrimitives(PrimitiveType_e nType, int nFirstVertex, int nVertices);

	// mesh buffer FFP emulation
	void				DrawMeshBufferPrimitives(PrimitiveType_e nType, int nVertices, int nIndices);

protected:

	void				ThreadingSharingRequest();
	void				ThreadingSharingRelease();
	bool				GL_CRITICAL();
	void				GL_END_CRITICAL();

	void				CreateTextureInternal(ITexture** pTex, const DkList<CImage*>& pImages, const SamplerStateParam_t& sampler,int nFlags = 0);
	GLuint				CreateGLTextureFromImage(CImage* pSrc, GLuint gltarget, const SamplerStateParam_t& sampler, int& wide, int& tall, int nFlags);

	//void				AddTextureInternal(ITexture** pTex, CImage *texImage,SamplerStateParam_t& sSamplingParams,int nFlags = 0);
	//void				AddAnimatedTextureInternal(ITexture** pTex, CImage **texImage, int numTextures, SamplerStateParam_t& sSamplingParams,int nFlags = 0);

private:
	//OpenGL - Specific
	void				InternalSetupSampler(uint texTarget, const SamplerStateParam_t& sSamplingParams);
	void				InternalChangeFrontFace(int nCullFaceMode);

	GLuint				m_frameBuffer;
	GLuint				m_depthBuffer;

	GLenum				m_drawBuffers[MAX_MRTS];

	int					m_boundInstanceStream;

	int					m_nCurrentRenderTargets;

	int					m_nCurrentFrontFace;

	int					m_nCurrentSrcFactor;
	int					m_nCurrentDstFactor;
	int					m_nCurrentBlendFunc;

	int					m_nCurrentDepthFunc;
	bool				m_bCurrentDepthTestEnable;
	bool				m_bCurrentDepthWriteEnable;

	bool				m_bCurrentMultiSampleEnable;
	bool				m_bCurrentScissorEnable;
	int					m_nCurrentCullMode;
	int					m_nCurrentFillMode;

	int					m_nCurrentMask;
	bool				m_bCurrentBlendEnable;

	uint				m_nCurrentVBO;

	IRectangle			m_viewPort;

	int					m_nCurrentMatrixMode;
	Matrix4x4			m_matrices[4];

	IShaderProgram*		m_pMeshBufferNoTextureShader;
	IShaderProgram*		m_pMeshBufferTexturedShader;

#ifdef USE_GLES2
    EGLNativeDisplayType	m_hdc;
    EGLDisplay				m_display;
    EGLSurface				m_eglSurface;
    EGLContext				m_glContext;
	EGLContext				m_glContext2;
#elif _WIN32
	HDC					m_hdc;
	HGLRC				m_glContext;
	HGLRC				m_glContext2;
#elif defined(LINUX)
	GLXContext			m_glContext;
	GLXContext			m_glContext2;
	Display*            m_display;
#elif defined(__APPLE__)
	AGLContext			m_glContext;
	GLXContext			m_glContext2;
#endif // _WIN32

	uintptr_t			m_mainThreadId;
	uintptr_t			m_currThreadId;
	bool				m_isSharing;
	bool				m_isMainAtCritical;

	CEqSignal			m_busySignal;

	EGraphicsVendor		m_vendor;
};

#endif // SHADERAPIGL_H
