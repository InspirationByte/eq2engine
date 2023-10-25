#include "core/core_common.h"
#include "WGPURenderAPI.h"

WGPURenderAPI WGPURenderAPI::Instance;

void WGPURenderAPI::Init(const ShaderAPIParams& params)
{
	memset(&m_caps, 0, sizeof(m_caps));
	ShaderAPI_Base::Init(params);
}
//void WGPURenderAPI::Shutdown() {}

void WGPURenderAPI::PrintAPIInfo() const
{
}

bool WGPURenderAPI::IsDeviceActive() const
{
	return true;
}

IVertexFormat* WGPURenderAPI::CreateVertexFormat(const char* name, ArrayCRef<VertexFormatDesc> formatDesc)
{
	IVertexFormat* pVF = PPNew CWGPUVertexFormat(name, formatDesc.ptr(), formatDesc.numElem());
	m_VFList.append(pVF);
	return pVF;
}

IVertexBuffer* WGPURenderAPI::CreateVertexBuffer(const BufferInfo& bufferInfo)
{
	CWGPUVertexBuffer* buffer = new CWGPUVertexBuffer(bufferInfo.elementSize);
	m_VBList.append(buffer);
	return buffer;
}

IIndexBuffer* WGPURenderAPI::CreateIndexBuffer(const BufferInfo& bufferInfo)
{
	CWGPUIndexBuffer* buffer = new CWGPUIndexBuffer(bufferInfo.elementSize);
	m_IBList.append(buffer);
	return buffer;
}

// Destroy vertex format
void WGPURenderAPI::DestroyVertexFormat(IVertexFormat* pFormat)
{
	if (m_VFList.fastRemove(pFormat))
		delete pFormat;
}

// Destroy vertex buffer
void WGPURenderAPI::DestroyVertexBuffer(IVertexBuffer* pVertexBuffer)
{
	if (m_VBList.fastRemove(pVertexBuffer))
		delete pVertexBuffer;
}

// Destroy index buffer
void WGPURenderAPI::DestroyIndexBuffer(IIndexBuffer* pIndexBuffer)
{
	if (m_IBList.fastRemove(pIndexBuffer))
		delete pIndexBuffer;
}

//-------------------------------------------------------------
// Shaders and it's operations

IShaderProgramPtr WGPURenderAPI::CreateNewShaderProgram(const char* pszName, const char* query)
{
	return nullptr;
}

void WGPURenderAPI::FreeShaderProgram(IShaderProgram* pShaderProgram)
{
}

bool WGPURenderAPI::CompileShadersFromStream(IShaderProgramPtr pShaderOutput, const ShaderProgCompileInfo& info, const char* extra)
{
	return true; 
}

//-------------------------------------------------------------
// Occlusion query

// creates occlusion query object
IOcclusionQuery* WGPURenderAPI::CreateOcclusionQuery()
{
	return nullptr;
}

// removal of occlusion query object
void WGPURenderAPI::DestroyOcclusionQuery(IOcclusionQuery* pQuery)
{
}

//-------------------------------------------------------------
// Textures

ITexturePtr WGPURenderAPI::CreateTextureResource(const char* pszName)
{
	CRefPtr<CWGPUTexture> texture = CRefPtr_new(CWGPUTexture);
	texture->SetName(pszName);

	m_TextureList.insert(texture->m_nameHash, texture);
	return ITexturePtr(texture);
}

// It will add new rendertarget
ITexturePtr	WGPURenderAPI::CreateRenderTarget(const char* pszName,int width, int height, ETextureFormat nRTFormat, ETexFilterMode textureFilterType, ETexAddressMode textureAddress, ECompareFunc comparison, int nFlags)
{
	CRefPtr<CWGPUTexture> pTexture = CRefPtr_new(CWGPUTexture);
	pTexture->SetName(pszName);

	CScopedMutex scoped(g_sapi_TextureMutex);
	CHECK_TEXTURE_ALREADY_ADDED(pTexture);
	m_TextureList.insert(pTexture->m_nameHash, pTexture);

	pTexture->SetDimensions(width, height);
	pTexture->SetFormat(nRTFormat);

	return ITexturePtr(pTexture);
}

//-------------------------------------------------------------
// Render states management

IRenderState* WGPURenderAPI::CreateBlendingState( const BlendStateParams &blendDesc )
{
	return nullptr;
}

IRenderState* WGPURenderAPI::CreateDepthStencilState( const DepthStencilStateParams &depthDesc )
{
	return nullptr;
}

IRenderState* WGPURenderAPI::CreateRasterizerState( const RasterizerStateParams &rasterDesc )
{
	return nullptr;
}

void WGPURenderAPI::DestroyRenderState( IRenderState* pShaderProgram, bool removeAllRefs)
{
}

//-------------------------------------------------------------
// Render target operations

void  WGPURenderAPI::Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const MColor& fillColor, float fDepth, int nStencil)
{
}

void WGPURenderAPI::CopyFramebufferToTexture(const ITexturePtr& pTargetTexture)
{
}

void WGPURenderAPI::CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect, IAARectangle* destRect)
{
}

void WGPURenderAPI::ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets, ArrayCRef<int> rtSlice, const ITexturePtr& depthTarget, int depthSlice)
{
}

void WGPURenderAPI::ChangeRenderTargetToBackBuffer()
{
}

void WGPURenderAPI::ResizeRenderTarget(const ITexturePtr& pRT, int newWide, int newTall)
{
}

//-------------------------------------------------------------
// Various setup functions for drawing

void WGPURenderAPI::ChangeVertexFormat(IVertexFormat* pVertexFormat)
{
}

void WGPURenderAPI::ChangeVertexBuffer(IVertexBuffer* pVertexBuffer,int nStream, const intptr offset)
{
}

void WGPURenderAPI::ChangeIndexBuffer(IIndexBuffer* pIndexBuffer)
{
}

//-------------------------------------------------------------
// State management

void  WGPURenderAPI::SetScissorRectangle(const IAARectangle& rect)
{
}

void WGPURenderAPI::SetDepthRange(float fZNear, float fZFar)
{
}

//-------------------------------------------------------------
// Renderer state managemet

void WGPURenderAPI::SetShader(IShaderProgramPtr pShader)
{
}

void WGPURenderAPI::SetTexture(int nameHash, const ITexturePtr& texture)
{
}

void WGPURenderAPI::SetShaderConstantRaw(int nameHash, const void* data, int nSize)
{
}

//-------------------------------------------------------------
// Primitive drawing

void WGPURenderAPI::DrawIndexedPrimitives(EPrimTopology nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex)
{
}

void WGPURenderAPI::DrawNonIndexedPrimitives(EPrimTopology nType, int nFirstVertex, int nVertices)
{
}