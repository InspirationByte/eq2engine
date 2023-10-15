//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Empty ShaderAPI for some applications using matsystem
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ShaderAPI_Base.h"
#include "EmptyTexture.h"
#include "imaging/ImageLoader.h"

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;
extern CEqMutex	g_sapi_ShaderMutex;
extern CEqMutex	g_sapi_VBMutex;
extern CEqMutex	g_sapi_IBMutex;
extern CEqMutex	g_sapi_Mutex;

class CEmptyVertexBuffer : public IVertexBuffer
{
public:
				CEmptyVertexBuffer(int stride) : m_lockData(nullptr), m_stride(stride) {}

	// returns size in bytes
	int			GetSizeInBytes() const { return 0; }

	// returns vertex count
	int			GetVertexCount() const { return 0; }

	// retuns stride size
	int			GetStrideSize() const { return m_stride; }

	// updates buffer without map/unmap operations which are slower
	void		Update(void* data, int size, int offset, bool discard = true)
	{
	}

	// locks vertex buffer and gives to programmer buffer data
	bool		Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly)
	{
		*outdata = PPAlloc(sizeToLock*m_stride);
		return true;
	}

	// unlocks buffer
	void		Unlock() {PPFree(m_lockData); m_lockData = nullptr;}

	// sets vertex buffer flags
	void		SetFlags( int flags ) {}
	int			GetFlags() const {return 0;}

	void*		m_lockData;
	int			m_stride;
};

class CEmptyIndexBuffer : public IIndexBuffer
{
public:
				CEmptyIndexBuffer(int stride) : m_lockData(nullptr), m_stride(stride) {}

	int			GetSizeInBytes() const { return 0; }

	// returns index size
	int			GetIndexSize() const {return m_stride;}

	// returns index count
	int			GetIndicesCount()  const {return 0;}

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
	void		Unlock() {free(m_lockData); m_lockData = nullptr;}

	// sets vertex buffer flags
	void		SetFlags( int flags ) {}
	int			GetFlags() const {return 0;}

	void*		m_lockData;
	int			m_stride;
};

class CEmptyVertexFormat : public IVertexFormat
{
public:
	CEmptyVertexFormat(const char* name, const VertexFormatDesc *desc, int nAttribs)
	{
		m_name = name;
		memset(m_streamStride, 0, sizeof(m_streamStride));

		m_vertexDesc.setNum(nAttribs);
		for(int i = 0; i < nAttribs; i++)
			m_vertexDesc[i] = desc[i];
	}

	const char* GetName() const {return m_name.ToCString();}

	int	GetVertexSize(int stream) const
	{
		return 0;
	}

	ArrayCRef<VertexFormatDesc> GetFormatDesc() const
	{
		return m_vertexDesc;
	}

protected:
	int							m_streamStride[MAX_VERTEXSTREAM];
	EqString					m_name;
	Array<VertexFormatDesc>	m_vertexDesc{ PP_SL };
};

class ShaderAPIEmpty : public ShaderAPI_Base
{
	friend class CEmptyRenderLib;
	friend class CEmptyTexture;
public:

								ShaderAPIEmpty() {}
								~ShaderAPIEmpty() {}

	// Init + Shurdown
	void						Init(const ShaderAPIParams &params) 
	{
		memset(&m_caps, 0, sizeof(m_caps));
		ShaderAPI_Base::Init(params);
	}
	//void						Shutdown() {}

	void						PrintAPIInfo() const {}

	bool						IsDeviceActive() const {return true;}

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
	EShaderAPIType			GetShaderAPIClass() {return SHADERAPI_EMPTY;}

	// Device vendor and version
	const char*					GetDeviceNameString() const {return "nullptr device";}

	// Renderer string (ex: OpenGL, D3D9)
	const char*					GetRendererName() const {return "Empty";}

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
	IOcclusionQuery*	CreateOcclusionQuery() {return nullptr;}

	// removal of occlusion query object
	void				DestroyOcclusionQuery(IOcclusionQuery* pQuery) {}

//-------------------------------------------------------------
// Textures
//-------------------------------------------------------------

	// It will add new rendertarget
	ITexturePtr					CreateRenderTarget(const char* pszName,int width, int height, ETextureFormat nRTFormat, ETexFilterMode textureFilterType = TEXFILTER_LINEAR, ETexAddressMode textureAddress = TEXADDRESS_WRAP, ECompareFunc comparison = COMPFUNC_NEVER, int nFlags = 0)
	{
		CRefPtr<CEmptyTexture> pTexture = CRefPtr_new(CEmptyTexture);
		pTexture->SetName(pszName);

		CScopedMutex scoped(g_sapi_TextureMutex);
		CHECK_TEXTURE_ALREADY_ADDED(pTexture);
		m_TextureList.insert(pTexture->m_nameHash, pTexture);

		pTexture->SetDimensions(width, height);
		pTexture->SetFormat(nRTFormat);

		return ITexturePtr(pTexture);
	}

//-------------------------------------------------------------
// Texture operations
//-------------------------------------------------------------

	// Copy render target to texture
	void						CopyFramebufferToTexture(const ITexturePtr& pTargetTexture) {}

	// Copy render target to texture
	void						CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect = nullptr, IAARectangle* destRect = nullptr) {}

	// Changes render target (MRT)
	void						ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets,
											ArrayCRef<int> rtSlice = nullptr,
											const ITexturePtr& depthTarget = nullptr,
											int depthSlice = 0) {}

	// Changes back to backbuffer
	void						ChangeRenderTargetToBackBuffer(){}

	// resizes render target
	void						ResizeRenderTarget(const ITexturePtr& pRT, int newWide, int newTall){}

	// sets scissor rectangle
	void						SetScissorRectangle( const IAARectangle &rect ) {}

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
	void						DestroyVertexFormat(IVertexFormat* pFormat){ if(pFormat) delete pFormat;}

	// Destroy vertex buffer
	void						DestroyVertexBuffer(IVertexBuffer* pVertexBuffer) {if(pVertexBuffer) delete pVertexBuffer;}

	// Destroy index buffer
	void						DestroyIndexBuffer(IIndexBuffer* pIndexBuffer) {if(pIndexBuffer) delete pIndexBuffer;}

//-------------------------------------------------------------
// State manipulation
//-------------------------------------------------------------

	// creates blending state
	IRenderState*				CreateBlendingState( const BlendStateParams &blendDesc ) {return nullptr;}

	// creates depth/stencil state
	IRenderState*				CreateDepthStencilState( const DepthStencilStateParams &depthDesc ) {return nullptr;}

	// creates rasterizer state
	IRenderState*				CreateRasterizerState( const RasterizerStateParams &rasterDesc ) {return nullptr;}

	// completely destroys shader
	void						DestroyRenderState( IRenderState* pShaderProgram, bool removeAllRefs = false) {}

//-------------------------------------------------------------
// Shaders and it's operations
//-------------------------------------------------------------

	// Creates shader class for needed ShaderAPI
	IShaderProgram*				CreateNewShaderProgram(const char* pszName, const char* query = nullptr){return nullptr;}

	// Destroy all shader
	void						DestroyShaderProgram(IShaderProgram* pShaderProgram){}

	// Load any shader from stream
	bool						CompileShadersFromStream(	IShaderProgram* pShaderOutput,
															const ShaderProgCompileInfo& info,
															const char* extra = nullptr){return true;}

	// Set current shader for rendering
	void						SetShader(IShaderProgram* pShader){}

	// Set Texture for shader
	void						SetTexture(int nameHash, const ITexturePtr& texture) {}

	// RAW Constant (Used for structure types, etc.)
	void						SetShaderConstantRaw(int nameHash, const void *data, int nSize){}

//-------------------------------------------------------------
// Vertex buffer objects
//-------------------------------------------------------------

	IVertexFormat*				CreateVertexFormat(const char* name, ArrayCRef<VertexFormatDesc> formatDesc)
	{
		IVertexFormat* pVF = PPNew CEmptyVertexFormat(name, formatDesc.ptr(), formatDesc.numElem());
		m_VFList.append(pVF);
		return pVF;
	}
	IVertexBuffer*				CreateVertexBuffer(EBufferAccessType nBufAccess, int nNumVerts, int strideSize, void *pData = nullptr){return new CEmptyVertexBuffer(strideSize);}
	IIndexBuffer*				CreateIndexBuffer(int nIndices, int nIndexSize, EBufferAccessType nBufAccess, void *pData = nullptr){return new CEmptyIndexBuffer(nIndexSize);}

//-------------------------------------------------------------
// Primitive drawing (lower level than DrawPrimitives2D)
//-------------------------------------------------------------

	// Indexed primitive drawer
	void						DrawIndexedPrimitives(EPrimTopology nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex = 0){}

	// Draw elements
	void						DrawNonIndexedPrimitives(EPrimTopology nType, int nFirstVertex, int nVertices){}

protected:

	ITexturePtr					CreateTextureResource(const char* pszName)
	{
		CRefPtr<CEmptyTexture> texture = CRefPtr_new(CEmptyTexture);
		texture->SetName(pszName);

		m_TextureList.insert(texture->m_nameHash, texture);
		return ITexturePtr(texture);
	}

	int							GetSamplerUnit(IShaderProgram* pProgram,const char* pszSamplerName) {return 0;}
};
