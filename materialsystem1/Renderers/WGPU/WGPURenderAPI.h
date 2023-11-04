//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>

#include "ShaderAPI_Base.h"
#include "WGPUTexture.h"
#include "WGPUBuffer.h"
#include "WGPUVertexFormat.h"

using namespace Threading;

extern CEqMutex	g_sapi_TextureMutex;
extern CEqMutex	g_sapi_ShaderMutex;
extern CEqMutex	g_sapi_VBMutex;
extern CEqMutex	g_sapi_IBMutex;
extern CEqMutex	g_sapi_Mutex;

class CWGPURenderAPI : public ShaderAPI_Base
{
	friend class CWGPURenderLib;
public:
	static CWGPURenderAPI Instance;

	CWGPURenderAPI() {}
	~CWGPURenderAPI() {}

	// Init + Shurdown
	void			Init(const ShaderAPIParams& params);
	//void			Shutdown() {}

//-------------------------------------------------------------
// Renderer information
	void			PrintAPIInfo() const;
	bool			IsDeviceActive() const;

	// shader API class type for shader developers.
	EShaderAPIType	GetShaderAPIClass()		{ return SHADERAPI_WEBGPU; }

	// Renderer string (ex: OpenGL, D3D9)
	const char*		GetRendererName() const { return "WebGPU"; }

//-------------------------------------------------------------
// MT Synchronization

	// Synchronization
	void			Flush() {}
	void			Finish() {}

//-------------------------------------------------------------
// Vertex buffer objects

	IVertexFormat*	CreateVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> formatDesc);
	IVertexBuffer*	CreateVertexBuffer(const BufferInfo& bufferInfo);
	IIndexBuffer*	CreateIndexBuffer(const BufferInfo& bufferInfo);
	void			DestroyVertexFormat(IVertexFormat* pFormat);
	void			DestroyVertexBuffer(IVertexBuffer* pVertexBuffer);
	void			DestroyIndexBuffer(IIndexBuffer* pIndexBuffer);

//-------------------------------------------------------------
// Shaders and it's operations

	IShaderProgramPtr	CreateNewShaderProgram(const char* pszName, const char* query = nullptr);
	void			FreeShaderProgram(IShaderProgram* pShaderProgram);
	bool			CompileShadersFromStream(IShaderProgramPtr pShaderOutput, const ShaderProgCompileInfo& info, const char* extra = nullptr);

//-------------------------------------------------------------
// Occlusion query

	// creates occlusion query object
	IOcclusionQuery*	CreateOcclusionQuery();

	// removal of occlusion query object
	void			DestroyOcclusionQuery(IOcclusionQuery* pQuery);

//-------------------------------------------------------------
// Textures

	ITexturePtr		CreateTextureResource(const char* pszName);
	ITexturePtr		CreateRenderTarget(const char* pszName, int width, int height, ETextureFormat nRTFormat, ETexFilterMode textureFilterType = TEXFILTER_LINEAR, ETexAddressMode textureAddress = TEXADDRESS_WRAP, ECompareFunc comparison = COMPFUNC_NEVER, int nFlags = 0);

//-------------------------------------------------------------
// Pipeline management

	IGPURenderPipelinePtr	CreateRenderPipeline(const IGPUPipelineLayoutPtr layoutDesc, const RenderPipelineDesc& pipelineDesc);
	IGPUPipelineLayoutPtr	CreatePipelineLayout(const RenderPipelineLayoutDesc& layoutDesc);
	IGPUBindGroupPtr			CreateBindGroup(const IGPUPipelineLayoutPtr layoutDesc, int layoutBindGroupIdx, const BindGroupDesc& bindGroupDesc);

//-------------------------------------------------------------
// Buffer management

	IGPUBufferPtr			CreateBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* name = nullptr);

//-------------------------------------------------------------
// DEPRECATED Render states management

	IRenderState*	CreateBlendingState(const BlendStateParams& blendDesc);
	IRenderState*	CreateDepthStencilState(const DepthStencilStateParams& depthDesc);
	IRenderState*	CreateRasterizerState(const RasterizerStateParams& rasterDesc);
	void			DestroyRenderState(IRenderState* pShaderProgram, bool removeAllRefs = false);

//-------------------------------------------------------------
// DEPRECATED Render target operations

	void			Clear(bool bClearColor, bool bClearDepth, bool bClearStencil, const MColor& fillColor, float fDepth, int nStencil);

	void			CopyFramebufferToTexture(const ITexturePtr& pTargetTexture);
	void			CopyRendertargetToTexture(const ITexturePtr& srcTarget, const ITexturePtr& destTex, IAARectangle* srcRect = nullptr, IAARectangle* destRect = nullptr);
	
	void			ChangeRenderTargets(ArrayCRef<ITexturePtr> renderTargets, ArrayCRef<int> rtSlice = nullptr, const ITexturePtr& depthTarget = nullptr, int depthSlice = 0);
	void			ChangeRenderTargetToBackBuffer();
	void			ResizeRenderTarget(const ITexturePtr& pRT, int newWide, int newTall);

//-------------------------------------------------------------
// DEPRECATED Various setup functions for drawing

	void			ChangeVertexFormat(IVertexFormat* pVertexFormat);
	void			ChangeVertexBuffer(IVertexBuffer* pVertexBuffer, int nStream, const intptr offset = 0);
	void			ChangeIndexBuffer(IIndexBuffer* pIndexBuffer);

//-------------------------------------------------------------
// DEPRECATED State management

	void			SetScissorRectangle(const IAARectangle& rect);
	void			SetDepthRange(float fZNear, float fZFar);

//-------------------------------------------------------------
// DEPRECATED Renderer state managemet

	void			SetShader(IShaderProgramPtr pShader);
	void			SetTexture(int nameHash, const ITexturePtr& texture);
	void			SetShaderConstantRaw(int nameHash, const void* data, int nSize);

	void			Reset(int nResetType = STATE_RESET_ALL) {}

	void			ApplyTextures() {}
	void			ApplySamplerState() {}
	void			ApplyBlendState() {}
	void			ApplyDepthState() {}
	void			ApplyRasterizerState() {}
	void			ApplyShaderProgram() {}
	void			ApplyConstants() {}

//-------------------------------------------------------------
// DEPRECATED Primitive drawing

	void			DrawIndexedPrimitives(EPrimTopology nType, int nFirstIndex, int nIndices, int nFirstVertex, int nVertices, int nBaseVertex = 0);
	void			DrawNonIndexedPrimitives(EPrimTopology nType, int nFirstVertex, int nVertices);

//-------------------------------------------------------------
// Private access

	WGPUDevice		GetWGPUDevice() const { return m_rhiDevice; }
	WGPUQueue		GetWGPUQueue() const { return m_rhiQueue; };

protected:

	WGPUDevice		m_rhiDevice{ nullptr };
	WGPUQueue		m_rhiQueue{ nullptr };
};
