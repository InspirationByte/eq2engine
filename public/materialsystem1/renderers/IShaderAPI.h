//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				For high level look for the material system (Engine)
//				Contains FFP functions and Shader (FFP overriding programs)
//////////////////////////////////////////////////////////////////////////////////


#ifndef SHADERAPI_INTERFACE_H
#define SHADERAPI_INTERFACE_H

// Platform definitions
#include "core/dktypes.h"
#include "ds/Array.h"
#include <stdint.h>

// Various declarations
#include "ShaderAPICaps.h"

// Shader
#include "IShaderProgram.h"

// VBO
#include "IVertexBuffer.h"
#include "IVertexFormat.h"
#include "IIndexBuffer.h"

// Textures
#include "ITexture.h"

// occ query objects
#include "IOcclusionQuery.h"

// render states
#include "IRenderState.h"

struct kvkeybase_t;

// designed to be sent as windowHandle param
struct externalWindowDisplayParams_t
{
	void*			window;
	void**			paramArray;
	int				numParams;
};

// shader api initializer
struct shaderAPIParams_t
{
	shaderAPIParams_t()
	{
		windowHandle			= NULL;
		screenFormat			= FORMAT_RGB8;
		screenRefreshRateHZ		= 60;
		multiSamplingMode		= 0;
		depthBits				= 24;
		verticalSyncEnabled		= false;

		memset(texturePath, 0, sizeof(texturePath));
	}

	// basic parameters for shader API initialization
	void*			windowHandle;			// OS window handle or externalWindowDisplayParams_t
	ETextureFormat	screenFormat;			// screen back buffer format

	int				screenRefreshRateHZ;	// refresh rate in HZ
	int				multiSamplingMode;		// multisampling

	// extended parameters
	int				depthBits;				// bit depth for depth/stencil

	bool			verticalSyncEnabled;	// vertical syncronization

	char			texturePath[64];		// texture path
};

class CImage;

//
// ShaderAPI interface
//
class IShaderAPI
{
public:
	virtual ~IShaderAPI() {}

	// initializes shader api.
	// Don't use this, this already called by materials->Init()
	virtual void				Init( shaderAPIParams_t &params ) = 0;

	// shutdowns shader api. Don't use this, this already called by materials->Shutdown()
	virtual void				Shutdown() = 0;

	// returns current screen format. Useful for screen render targets, for 16-bit screen
	virtual ETextureFormat		GetScreenFormat() = 0;

	// prints shader api information to console output
	virtual void				PrintAPIInfo() = 0;

	// returns device activation state
	virtual bool				IsDeviceActive() = 0;

//-------------------------------------------------------------
// Rendering's applies
//-------------------------------------------------------------

	// reset states. Use RESET_TYPE flags. By default all states reset
	virtual void				Reset(int nResetTypeFlags = STATE_RESET_ALL) = 0;

	// applies currently set up parameters
	virtual void				Apply() = 0;

	// applies only textures
	virtual void				ApplyTextures() = 0;

	// applies only sampler states
	virtual void				ApplySamplerState() = 0;

	// applies blending states
	virtual void				ApplyBlendState() = 0;

	// applies depth states
	virtual void				ApplyDepthState() = 0;

	// applies rasterizer states
	virtual void				ApplyRasterizerState() = 0;

	// applies vertex buffer objects
	virtual void				ApplyBuffers() = 0;

	// applies shader programs
	virtual void				ApplyShaderProgram() = 0;

	// applies shader constants TODO: remove due no usage
	virtual void				ApplyConstants() = 0;

//-------------------------------------------------------------
// Misc
//-------------------------------------------------------------

	// clears backbuffer. Don't use bClearColor with bClearStencil
	virtual void				Clear(	bool bClearColor,
										bool bClearDepth = true,
										bool bClearStencil = true,
										const ColorRGBA &fillColor = ColorRGBA(0),
										float fDepth = 1.0f,
										int nStencil = 0
										) = 0;

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

	// shader API class type for shader developers.
	virtual const ShaderAPICaps_t&	GetCaps() const = 0;

	virtual ER_ShaderAPIType		GetShaderAPIClass() const = 0;

	// Renderer string (ex: OpenGL, D3D9)
	virtual const char*				GetRendererName() const = 0;

	// Device vendor and version
	virtual const char*				GetDeviceNameString() const = 0;

//-------------------------------------------------------------
// Render statistics
//-------------------------------------------------------------

	// Draw call counter
	virtual int						GetDrawCallsCount() const = 0;

	// draw idexed calls
	virtual int						GetDrawIndexedPrimitiveCallsCount() const = 0;

	// triangles per scene drawing
	virtual int						GetTrianglesCount() const = 0;

	// Resetting the counters
	virtual void					ResetCounters() = 0;

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// flushes
	virtual void					Flush() = 0;

	// finishes
	virtual void					Finish() = 0;

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------

	// creates occlusion query object
	virtual IOcclusionQuery*	CreateOcclusionQuery() = 0;

	// removal of occlusion query object
	virtual void				DestroyOcclusionQuery(IOcclusionQuery* pQuery) = 0;


//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// default error texture pointer
	virtual ITexture*			GetErrorTexture() = 0;

	// Searches for texture by name, can be used for shared RT's
	virtual ITexture*			FindTexture( const char* pszName ) = 0;

	// unloads the texture and frees the memory
	virtual void				FreeTexture(ITexture* pTexture) = 0;

	// creates texture from image array, used in LoadTexture, common use only
	virtual	ITexture*			CreateTexture(const Array<CImage*>& pImages, const SamplerStateParam_t& sampler, int nFlags = 0) = 0;


	// BEGIN CUT HERE

	// loads texture
	virtual ITexture*			LoadTexture(	const char* pszFileName,
												ER_TextureFilterMode textureFilterType,
												ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
												int nFlags = 0
												) = 0;

	// creates procedural (lockable) texture
	virtual ITexture*			CreateProceduralTexture(const char* pszName,
														ETextureFormat nFormat,
														int width, int height,
														int depth = 1,
														int arraySize = 1,
														ER_TextureFilterMode texFilter = TEXFILTER_NEAREST,
														ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
														int nFlags = 0,
														int nDataSize = 0, const unsigned char* pData = NULL
														) = 0;

	// creates render target texture
	virtual ITexture*			CreateRenderTarget(	int width, int height,
													ETextureFormat nRTFormat,
													ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR,
													ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
													ER_CompareFunc comparison = COMP_NEVER,
													int nFlags = 0
													) = 0;

	// creates named render target texture, useful for material system shader texture searching
	virtual ITexture*			CreateNamedRenderTarget(	const char* pszName,
															int width, int height,
															ETextureFormat nRTFormat,
															ER_TextureFilterMode textureFilterType = TEXFILTER_LINEAR,
															ER_TextureAddressMode textureAddress = TEXADDRESS_WRAP,
															ER_CompareFunc comparison = COMP_NEVER,
															int nFlags = 0
															) = 0;

	// generates a new error texture
	virtual ITexture*			GenerateErrorTexture(int nFlags = 0) = 0;

	// END CUT HERE

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// saves rendertarget to texture, you can also save screenshots
	virtual void				SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName) = 0;

	// blits the current framebuffer to texture TODO: need flags such as depth copy, etc
	virtual void				CopyFramebufferToTexture(ITexture* pTargetTexture) = 0;

	// Copy render target to texture
	virtual void				CopyRendertargetToTexture(ITexture* srcTarget, ITexture* destTex, IRectangle* srcRect = NULL, IRectangle* destRect = NULL) = 0;

	// changes the rendertarget
	virtual void				ChangeRenderTarget( ITexture* pRenderTarget, int nCubemapFace = 0, ITexture* pDepthTarget = NULL, int nDepthSlice = 0 ) = 0;

	// changes render targets (also known as multiple-render-targetting)
	virtual void				ChangeRenderTargets(	ITexture** pRenderTargets, int nNumRTs,
														int* nCubemapFaces = NULL,
														ITexture* pDepthTarget = NULL,
														int nDepthSlice = 0) = 0;

	// changes render target back to backbuffer
	virtual void				ChangeRenderTargetToBackBuffer() = 0;

	// resizes render target
	virtual void				ResizeRenderTarget( ITexture* pRT, int newWide, int newTall ) = 0;

	// returns the current set rendertargets
	virtual void				GetCurrentRenderTargets(	ITexture* pRenderTargets[MAX_MRTS], int *nNumRTs,
															ITexture** pDepthTarget,
															int cubeNumbers[MAX_MRTS]) = 0;

//-------------------------------------------------------------
// Matrix for rendering - FFP-likeness
//-------------------------------------------------------------

	// sets matrix mode
	virtual void				SetMatrixMode( ER_MatrixMode nMatrixMode ) = 0;

	// loads identity matrix
	virtual void				LoadIdentityMatrix() = 0;

	// Load custom matrix
	virtual void				LoadMatrix( const Matrix4x4 &matrix ) = 0;

	// Will save matrix
	virtual void				PushMatrix() = 0;

	// Will reset matrix
	virtual void				PopMatrix() = 0;

//-------------------------------------------------------------
// State manipulation
//-------------------------------------------------------------

	// Find sampler state with adding new one (if not exist)
	virtual SamplerStateParam_t	MakeSamplerState(ER_TextureFilterMode textureFilterType,ER_TextureAddressMode addressS, ER_TextureAddressMode addressT, ER_TextureAddressMode addressR) = 0;

	// creates blending state
	virtual IRenderState*		CreateBlendingState( const BlendStateParam_t &blendDesc ) = 0;

	// creates depth/stencil state
	virtual IRenderState*		CreateDepthStencilState( const DepthStencilStateParams_t &depthDesc ) = 0;

	// creates rasterizer state
	virtual IRenderState*		CreateRasterizerState( const RasterizerStateParams_t &rasterDesc ) = 0;

	// completely destroys shader
	virtual void				DestroyRenderState( IRenderState* pShaderProgram, bool removeAllRefs = false) = 0;

//-------------------------------------------------------------
// State setup functions for drawing
//-------------------------------------------------------------

	// sets the near/far depth range
	virtual void				SetDepthRange( float fZNear,float fZFar ) = 0;

	// sets rendering viewport
	virtual void				SetViewport( int x, int y, int w, int h ) = 0;

	// retunrs current rendering viewport.
	virtual void				GetViewport( int &x, int &y, int &w, int &h ) = 0;

	// retunrs current rendering viewport size only
	virtual void				GetViewportDimensions( int &wide, int &tall ) = 0;

	// sets scissor rectangle
	virtual void				SetScissorRectangle( const IRectangle &rect ) = 0;

	// sets render state
	//virtual void				SetRenderState( IRenderState* pState ) = 0;

	// TODO: REMOVE HERE, INTERNAL, DON'T USE: Sets the reserved blending state (from pre-defined preset)
	virtual void				SetBlendingState( IRenderState* pBlending ) = 0;

	// TODO: REMOVE HERE, INTERNAL, DON'T USE: Set depth and stencil state
	virtual void				SetDepthStencilState( IRenderState *pDepthStencilState ) = 0;

	// TODO: REMOVE HERE, INTERNAL, DON'T USE: sets reserved rasterizer mode
	virtual void				SetRasterizerState( IRenderState* pState ) = 0;

	// Set the texture. Animation is set from ITexture every frame (no affection on speed) before you do 'ApplyTextures'
	// Also you need to specify texture name. If you don't, use registers (not fine with DX10, 11)
	virtual void				SetTexture( ITexture* pTexture, const char* pszName = NULL, int index = 0) = 0;

	// returns the currently set textre at level
	virtual ITexture*			GetTextureAt( int level ) const = 0;

//-------------------------------------------------------------
// Vertex buffer object handling
//-------------------------------------------------------------

	// sets the vertex format
	virtual void				SetVertexFormat( IVertexFormat* pVertexFormat ) = 0;

	// sets the vertex buffer
	virtual void				SetVertexBuffer( IVertexBuffer* pVertexBuffer, int nStream, const intptr offset = 0 ) = 0;

	// sets the vertex buffer
	virtual void				SetVertexBuffer( int nStream, const void* base ) = 0;

	// changes the index buffer
	virtual void				SetIndexBuffer( IIndexBuffer *pIndexBuffer ) = 0;

	// Change* procedures is direct, so they doesn't needs ApplyBuffers().

	// changes the vertex format
	virtual void				ChangeVertexFormat( IVertexFormat* pVertexFormat ) = 0;

	// changes the vertex buffer
	virtual void				ChangeVertexBuffer( IVertexBuffer* pVertexBuffer,int nStream, const intptr offset = 0 ) = 0;

	// changes the index buffer
	virtual void				ChangeIndexBuffer( IIndexBuffer *pIndexBuffer ) = 0;

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// search for existing shader program
	virtual IShaderProgram*		FindShaderProgram( const char* pszName, const char* query = NULL ) = 0;

	// creates empty shader program
	virtual IShaderProgram*		CreateNewShaderProgram( const char* pszName, const char* query = NULL ) = 0;

	// completely destroy shader
	virtual void				DestroyShaderProgram( IShaderProgram* pShaderProgram ) = 0;

	// Loads and compiles shaders from files
	virtual bool				LoadShadersFromFile(	IShaderProgram* pShaderOutput,
														const char* pszFileNamePrefix,
														const char* extra = NULL
														) = 0;

	// Load any shader from stream
	virtual bool				CompileShadersFromStream(	IShaderProgram* pShaderOutput,
															const shaderProgramCompileInfo_t& info,
															const char* extra = NULL
															) = 0;

	// Sets current shader for rendering
	virtual void				SetShader(IShaderProgram* pShader) = 0;

	// Shader constants setup
	virtual int					SetShaderConstantInt(const char *pszName, const int constant, int const_id = -1) = 0;
	virtual int					SetShaderConstantFloat(const char *pszName, const float constant, int const_id = -1) = 0;
	virtual int					SetShaderConstantVector2D(const char *pszName, const Vector2D &constant, int const_id = -1) = 0;
	virtual int					SetShaderConstantVector3D(const char *pszName, const Vector3D &constant, int const_id = -1) = 0;
	virtual int					SetShaderConstantVector4D(const char *pszName, const Vector4D &constant, int const_id = -1) = 0;
	virtual int					SetShaderConstantMatrix4(const char *pszName, const Matrix4x4 &constant, int const_id = -1) = 0;
	virtual int					SetShaderConstantArrayFloat(const char *pszName, const float *constant, int count, int const_id = -1) = 0;
	virtual int					SetShaderConstantArrayVector2D(const char *pszName, const Vector2D *constant, int count, int const_id = -1) = 0;
	virtual int					SetShaderConstantArrayVector3D(const char *pszName, const Vector3D *constant, int count, int const_id = -1) = 0;
	virtual int					SetShaderConstantArrayVector4D(const char *pszName, const Vector4D *constant, int count, int const_id = -1) = 0;
	virtual int					SetShaderConstantArrayMatrix4(const char *pszName, const Matrix4x4 *constant, int count, int const_id = -1) = 0;

	// Shader constants setup (RAW mode, use for different structures)
    virtual int					SetShaderConstantRaw(const char *pszName, const void *data, int nSize, int const_id = -1) = 0;


//-------------------------------------------------------------
// Vertex buffer objects creation/destroying
//-------------------------------------------------------------

	// create new vertex declaration
	virtual IVertexFormat*		CreateVertexFormat( const char* name, VertexFormatDesc_s *formatDesc, int nAttribs ) = 0;
	virtual IVertexFormat*		FindVertexFormat( const char* name ) const = 0;

	// NOTENOTE: when you set nSize you must add the vertex structure size!
	virtual IVertexBuffer*		CreateVertexBuffer(	ER_BufferAccess nBufAccess,
															int nNumVerts,
															int strideSize,
															void *pData = NULL
															) = 0;

	// create new index buffer
	virtual IIndexBuffer*		CreateIndexBuffer(	int nIndices,
															int nIndexSize,
															ER_BufferAccess nBufAccess,
															void *pData = NULL
															) = 0;

	// Destroy
	virtual void				DestroyVertexFormat(IVertexFormat* pFormat) = 0;
	virtual void				DestroyVertexBuffer(IVertexBuffer* pVertexBuffer) = 0;
	virtual void				DestroyIndexBuffer(IIndexBuffer* pIndexBuffer) = 0;

//-------------------------------------------------------------
// Primitive drawing
//-------------------------------------------------------------

	// Indexed primitive drawer
	virtual void				DrawIndexedPrimitives(	ER_PrimitiveType nType,
														int nFirstIndex,
														int nIndices,
														int nFirstVertex,
														int nVertices,
														int nBaseVertex = 0
														) = 0;

	// Non-Indexed primitive drawer
	virtual void				DrawNonIndexedPrimitives(ER_PrimitiveType nType, int nFirstVertex, int nVertices) = 0;

};

// it always external, declare new one in your app...
extern IShaderAPI*				g_pShaderAPI;

#endif // SHADERAPI_INTERFACE_H
