//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Empty ShaderAPI for some applications using matsystem
//////////////////////////////////////////////////////////////////////////////////

#ifndef SHADERAPIEMPTY_H
#define SHADERAPIEMPTY_H

#include "../ShaderAPI_Base.h"
#include "IMeshBuilder.h"

#include "CEmptyTexture.h"
#include "imaging/ImageLoader.h"

class CEmptyMeshBuilder : public IMeshBuilder
{
	void		Begin(PrimitiveType_e type) {}
	void		End() {}

	// color setup
	void		Color3f( float r, float g, float b ){}
	void		Color3fv( float const *rgb ){}
	void		Color4f( float r, float g, float b, float a ){}
	void		Color4fv( float const *rgba ){}

	// position setup
	void		Position3f	(float x, float y, float z){}
	void		Position3fv	(const float *v) {}

	// normal setup
	void		Normal3f	(float nx, float ny, float nz){}
	void		Normal3fv	(const float *v){}

	void		TexCoord2f	(float s, float t){}
	void		TexCoord2fv	(const float *v){}
	void		TexCoord3f	(float s, float t, float r){}
	void		TexCoord3fv	(const float *v){}

	void		AdvanceVertex() {}	// advances vertex
};

static CEmptyMeshBuilder g_emptyMeshBuilder;

class CEmptyVertexBuffer : public IVertexBuffer
{
public:
				CEmptyVertexBuffer(int stride) : m_lockData(NULL), m_stride(stride) {}

	// returns size in bytes
	long		GetSizeInBytes() {return 0;}

	// returns vertex count
	int			GetVertexCount() {return 0;}

	// retuns stride size
	int			GetStrideSize() {return m_stride;}

	// updates buffer without map/unmap operations which are slower
	void		Update(void* data, int size, int offset, bool discard = true)
	{
	}

	// locks vertex buffer and gives to programmer buffer data
	bool		Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
	{
		*outdata = malloc(sizeToLock*m_stride);
		return true;
	}

	// unlocks buffer
	void		Unlock() {free(m_lockData); m_lockData = NULL;}

	// sets vertex buffer flags
	void		SetFlags( int flags ) {}
	int			GetFlags() {return 0;}

	void*		m_lockData;
	int			m_stride;
};

class CEmptyIndexBuffer : public IIndexBuffer
{
public:
				CEmptyIndexBuffer(int stride) : m_lockData(NULL), m_stride(stride) {}

	// returns index size
	int8		GetIndexSize() {return m_stride;}

	// returns index count
	int			GetIndicesCount() {return 0;}

	// updates buffer without map/unmap operations which are slower
	void		Update(void* data, int size, int offset, bool discard = true)
	{
	}

	// locks vertex buffer and gives to programmer buffer data
	bool		Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
	{
		*outdata = malloc(sizeToLock*m_stride);
		return true;
	}

	// unlocks buffer
	void		Unlock() {free(m_lockData); m_lockData = NULL;}

	// sets vertex buffer flags
	void		SetFlags( int flags ) {}
	int			GetFlags() {return 0;}

	void*		m_lockData;
	int			m_stride;
};


class ShaderAPIEmpty : public ShaderAPI_Base
{
public:

	friend class				CEmptyTexture;

								ShaderAPIEmpty() {}
								~ShaderAPIEmpty() {}
								

	// Init + Shurdown
	void						Init(shaderapiinitparams_t &params) { memset(&m_caps, 0, sizeof(m_caps)); ShaderAPI_Base::Init(params);}
	//void						Shutdown() {}

	void						PrintAPIInfo() {}

	bool						IsDeviceActive() {return true;}

//-------------------------------------------------------------
// Rendering's applies
//-------------------------------------------------------------
	void						Reset(int nResetType = STATE_RESET_ALL) {}

	void						ApplyTextures() {}
	void						ApplySamplerState() {}
	void						ApplyBlendState() {}
	void						ApplyDepthState() {}
	void						ApplyRasterizerState() {}
	void						ApplyShaderProgram() {}
	void						ApplyConstants() {}

	void						Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const ColorRGBA &fillColor,float fDepth, int nStencil) {}

//-------------------------------------------------------------
// Renderer information
//-------------------------------------------------------------

	// shader API class type for shader developers.
	// DON'T USE TYPES IN DYNAMIC SHADER CODE! USE MATSYSTEM MAT-FILE DEFS!
	ShaderAPIClass_e			GetShaderAPIClass() {return SHADERAPI_EMPTY;}

	// Device vendor and version
	const char*					GetDeviceNameString() const {return "NULL device";}

	// Renderer string (ex: OpenGL, D3D9)
	const char*					GetRendererName() const {return "Empty";}

	// Render targetting support
	bool						IsSupportsRendertargetting() const {return false;}

	// Render targetting support for Multiple RTs
	bool						IsSupportsMRT() const {return false;}

	// Supports multitexturing???
	bool						IsSupportsMultitexturing() const {return false;}

	// The driver/hardware is supports Pixel shaders?
	bool						IsSupportsPixelShaders() const {return false;}

	// The driver/hardware is supports Vertex shaders?
	bool						IsSupportsVertexShaders() const {return false;}

	// The driver/hardware is supports Geometry shaders?
	bool						IsSupportsGeometryShaders() const {return false;}

	// The driver/hardware is supports Domain shaders?
	bool						IsSupportsDomainShaders() const {return false;}

	// The driver/hardware is supports Hull (tessellator) shaders?
	bool						IsSupportsHullShaders() const {return false;}

	// The hardware (for engine) is supports full dynamic lighting
	//bool						IsSupportsFullLighting() const {return false;}

//-------------------------------------------------------------
// MT Synchronization
//-------------------------------------------------------------

	// Synchronization
	void						Flush(){}
	void						Finish(){}

//-------------------------------------------------------------
// Occlusion query
//-------------------------------------------------------------

	// creates occlusion query object
	IOcclusionQuery*	CreateOcclusionQuery() {return NULL;}

	// removal of occlusion query object
	void				DestroyOcclusionQuery(IOcclusionQuery* pQuery) {}

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// Unload the texture and free the memory
	void						FreeTexture(ITexture* pTexture)
	{
		CEmptyTexture* pTex = (CEmptyTexture*)(pTexture);

		if(pTex == NULL)
			return;

		pTex->Ref_Drop();

		if(pTex->Ref_Count() <= 0)
		{
			DevMsg(DEVMSG_SHADERAPI,"Texture unloaded: %s\n",pTexture->GetName());

			m_TextureList.remove(pTexture);
			delete pTex;
		}
	}

	// It will add new rendertarget
	ITexture*					CreateRenderTarget(int width, int height,ETextureFormat nRTFormat,Filter_e textureFilterType = TEXFILTER_LINEAR, AddressMode_e textureAddress = ADDRESSMODE_WRAP, CompareFunc_e comparison = COMP_NEVER, int nFlags = 0)
	{
		CEmptyTexture* pTex = new CEmptyTexture();
		pTex->SetName("_rt_001");

		pTex->SetDimensions(width, height);
		pTex->SetFormat(nRTFormat);

		m_TextureList.append(pTex);

		return pTex;
	}

	// It will add new rendertarget
	ITexture*					CreateNamedRenderTarget(const char* pszName,int width, int height, ETextureFormat nRTFormat, Filter_e textureFilterType = TEXFILTER_LINEAR, AddressMode_e textureAddress = ADDRESSMODE_WRAP, CompareFunc_e comparison = COMP_NEVER, int nFlags = 0)
	{
		CEmptyTexture* pTex = new CEmptyTexture();
		pTex->SetName(pszName);

		m_TextureList.append(pTex);

		pTex->SetDimensions(width, height);
		pTex->SetFormat(nRTFormat);

		return pTex;
	}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// saves rendertarget to texture, you can also save screenshots
	void						SaveRenderTarget(ITexture* pTargetTexture, const char* pFileName) {}

	// Copy render target to texture
	void						CopyFramebufferToTexture(ITexture* pTargetTexture){}

	// Copy render target to texture
	void						CopyRendertargetToTexture(ITexture* srcTarget, ITexture* destTex, IRectangle* srcRect = NULL, IRectangle* destRect = NULL) {}

	// Changes render target (MRT)
	void						ChangeRenderTargets(ITexture** pRenderTargets, int nNumRTs, int* nCubemapFaces = NULL, ITexture* pDepthTarget = NULL, int nDepthSlice = 0){}

	// fills the current rendertarget buffers
	void						GetCurrentRenderTargets(ITexture* pRenderTargets[MAX_MRTS], int *nNumRTs, ITexture** pDepthTarget, int cubeNumbers[MAX_MRTS]){}

	// Changes back to backbuffer
	void						ChangeRenderTargetToBackBuffer(){}

	// resizes render target
	void						ResizeRenderTarget(ITexture* pRT, int newWide, int newTall){}

	// returns current size of backbuffer surface
	void						GetViewportDimensions(int &wide, int &tall){}

	// sets scissor rectangle
	void						SetScissorRectangle( const IRectangle &rect ) {}

//-------------------------------------------------------------
// Matrix for rendering
//-------------------------------------------------------------

	// Matrix mode
	void						SetMatrixMode(MatrixMode_e nMatrixMode){}

	// Will save matrix
	void						PushMatrix(){}

	// Will reset matrix
	void						PopMatrix(){}

	// Load identity matrix
	void						LoadIdentityMatrix(){}

	// Load custom matrix
	void						LoadMatrix(const Matrix4x4 &matrix){}

	// Setup 2D mode
	void						SetupMatrixFor2DMode(float fLeft, float fRight, float fTop, float fBottom){}

//-------------------------------------------------------------
// Various setup functions for drawing
//-------------------------------------------------------------

	// Set Depth range for next primitives
	void						SetDepthRange(float fZNear,float fZFar){}

	// Changes the vertex format
	void						ChangeVertexFormat(IVertexFormat* pVertexFormat){}

	// Changes the vertex buffer
	void						ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset = 0){}

	// Changes the index buffer
	void						ChangeIndexBuffer(IIndexBuffer* pIndexBuffer){}

	// Destroy vertex format
	void						DestroyVertexFormat(IVertexFormat* pFormat){}

	// Destroy vertex buffer
	void						DestroyVertexBuffer(IVertexBuffer* pVertexBuffer) {if(pVertexBuffer) delete pVertexBuffer;}

	// Destroy index buffer
	void						DestroyIndexBuffer(IIndexBuffer* pIndexBuffer) {if(pIndexBuffer) delete pIndexBuffer;}

//-------------------------------------------------------------
// State manipulation
//-------------------------------------------------------------

	// creates blending state
	IRenderState*				CreateBlendingState( const BlendStateParam_t &blendDesc ) {return NULL;}

	// creates depth/stencil state
	IRenderState*				CreateDepthStencilState( const DepthStencilStateParams_t &depthDesc ) {return NULL;}

	// creates rasterizer state
	IRenderState*				CreateRasterizerState( const RasterizerStateParams_t &rasterDesc ) {return NULL;}

	// completely destroys shader
	void						DestroyRenderState( IRenderState* pShaderProgram, bool removeAllRefs = false) {}

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// search for existing shader program
	IShaderProgram*				FindShaderProgram(const char* pszName, const char* query = NULL){return NULL;}

	// Creates shader class for needed ShaderAPI
	IShaderProgram*				CreateNewShaderProgram(const char* pszName, const char* query = NULL){return NULL;}

	// Destroy all shader
	void						DestroyShaderProgram(IShaderProgram* pShaderProgram){}

	// Load any shader from stream
	bool						CompileShadersFromStream(	IShaderProgram* pShaderOutput,
															const shaderProgramCompileInfo_t& info,
															const char* extra = NULL){return true;}

	// Set current shader for rendering
	void						SetShader(IShaderProgram* pShader){}

	// Set Texture for shader
	void						SetTexture(ITexture* pTexture, const char* pszName, int index){}

	// RAW Constant (Used for structure types, etc.)
	int							SetShaderConstantRaw(const char *pszName, const void *data, int nSize, int nConstID){ return nConstID;}


	//-----------------------------------------------------
	// Advanced shader programming
	//-----------------------------------------------------

	// Pixel Shader constants setup (by register number)
	void						SetPixelShaderConstantInt(int reg, const int constant){}
	void						SetPixelShaderConstantFloat(int reg, const float constant){}
	void						SetPixelShaderConstantVector2D(int reg, const Vector2D &constant){}
	void						SetPixelShaderConstantVector3D(int reg, const Vector3D &constant){}
	void						SetPixelShaderConstantVector4D(int reg, const Vector4D &constant){}
	void						SetPixelShaderConstantMatrix4(int reg, const Matrix4x4 &constant){}
	void						SetPixelShaderConstantFloatArray(int reg, const float *constant, int count){}
	void						SetPixelShaderConstantVector2DArray(int reg, const Vector2D *constant, int count){}
	void						SetPixelShaderConstantVector3DArray(int reg, const Vector3D *constant, int count){}
	void						SetPixelShaderConstantVector4DArray(int reg, const Vector4D *constant, int count){}
	void						SetPixelShaderConstantMatrix4Array(int reg, const Matrix4x4 *constant, int count){}

	// Vertex Shader constants setup (by register number)
	void						SetVertexShaderConstantInt(int reg, const int constant){}
	void						SetVertexShaderConstantFloat(int reg, const float constant){}
	void						SetVertexShaderConstantVector2D(int reg, const Vector2D &constant){}
	void						SetVertexShaderConstantVector3D(int reg, const Vector3D &constant){}
	void						SetVertexShaderConstantVector4D(int reg, const Vector4D &constant){}
	void						SetVertexShaderConstantMatrix4(int reg, const Matrix4x4 &constant){}
	void						SetVertexShaderConstantFloatArray(int reg, const float *constant, int count){}
	void						SetVertexShaderConstantVector2DArray(int reg, const Vector2D *constant, int count){}
	void						SetVertexShaderConstantVector3DArray(int reg, const Vector3D *constant, int count){}
	void						SetVertexShaderConstantVector4DArray(int reg, const Vector4D *constant, int count){}
	void						SetVertexShaderConstantMatrix4Array(int reg, const Matrix4x4 *constant, int count){}

	// RAW Constant
	void						SetPixelShaderConstantRaw(int reg, const void *data, int nVectors = 1){}
	void						SetVertexShaderConstantRaw(int reg, const void *data, int nVectors = 1){}

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

	IVertexFormat*				CreateVertexFormat(VertexFormatDesc_s *formatDesc, int nAttribs){return NULL;}
	IVertexBuffer*				CreateVertexBuffer(BufferAccessType_e nBufAccess, int nNumVerts, int strideSize, void *pData = NULL){return new CEmptyVertexBuffer(strideSize);}
	IIndexBuffer*				CreateIndexBuffer(int nIndices, int nIndexSize, BufferAccessType_e nBufAccess, void *pData = NULL){return new CEmptyIndexBuffer(nIndexSize);}

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

	// Indexed primitive drawer
	void						DrawIndexedPrimitives(PrimitiveType_e nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex = 0){}

	// Draw elements
	void						DrawNonIndexedPrimitives(PrimitiveType_e nType, int nFirstVertex, int nVertices){}

protected:

	void						CreateTextureInternal(ITexture** pTex, const DkList<CImage*>& pImages, const SamplerStateParam_t& sampler,int nFlags)
	{
		if(!pImages.numElem())
			return;

		CEmptyTexture* pTexture = NULL;

		// get or create
		if(*pTex)
			pTexture = (CEmptyTexture*)*pTex;
		else
			pTexture = new CEmptyTexture();

		// FIXME: hold images?

		CScopedMutex m( m_Mutex );

		// Bind this sampler state to texture
		pTexture->SetSamplerState(sampler);
		pTexture->SetDimensions(pImages[0]->GetWidth(), pImages[0]->GetHeight());
		pTexture->SetFormat(pImages[0]->GetFormat());
		pTexture->SetFlags(nFlags | TEXFLAG_MANAGED);
		pTexture->SetName( pImages[0]->GetName() );

		// if this is a new texture, add
		if(!(*pTex))
			m_TextureList.append(pTexture);

		// set for output
		*pTex = pTexture;
	}

	int							GetSamplerUnit(IShaderProgram* pProgram,const char* pszSamplerName) {return 0;}
};

#endif // SHADERAPIEMPTY_H
