#pragma once
#include "renderers/IShaderAPI.h"

class CWGPURenderPassRecorder : public IGPURenderPassRecorder
{
public:
	~CWGPURenderPassRecorder();

	IVector2D		GetRenderTargetDimensions() const { return m_renderTargetDims; }
	ETextureFormat	GetRenderTargetFormat(int idx) const { return m_renderTargetsFormat[idx]; }
	ETextureFormat	GetDepthTargetFormat() const { return m_depthTargetFormat; }

	void SetPipeline(IGPURenderPipeline* pipeline);

	void SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets);
	void SetVertexBuffer(int slot, IGPUBuffer* vertexBuffer, int64 offset = 0, int64 size = -1);
	void SetIndexBuffer(IGPUBuffer* indexBuf, EIndexFormat indexFormat, int64 offset = 0, int64 size = -1);

	void SetViewport(const AARectangle& rectangle, float minDepth, float maxDepth);
	void SetScissorRectangle(const IAARectangle& rectangle);

	void Draw(int vertexCount, int firstVertex, int instanceCount, int firstInstance = 0);
	void DrawIndexed(int indexCount, int firstIndex, int instanceCount, int baseVertex = 0, int firstInstance = 0);
	void DrawIndexedIndirect(IGPUBuffer* indirectBuffer, int indirectOffset);
	void DrawIndirect(IGPUBuffer* indirectBuffer, int indirectOffset);

	// TODO:

	// BeginOcclusionQuery(uint32_t queryIndex);
	// EndOcclusionQuery();

	// ExecuteBundles(size_t bundleCount, WGPURenderBundle const* bundles);
	// PixelLocalStorageBarrier();

	// InsertDebugMarker(char const* markerLabel);
	// PopDebugGroup();
	// PushDebugGroup(char const* groupLabel);

	// SetBlendConstant(WGPUColor const* color);
	// SetStencilReference(uint32_t reference);

	void					Complete();

	IGPUCommandBufferPtr	End();

	void*					GetUserData() const { return m_userData; }

	ETextureFormat			m_renderTargetsFormat[MAX_RENDERTARGETS]{ FORMAT_NONE };
	ETextureFormat			m_depthTargetFormat{ FORMAT_NONE };
	WGPUCommandEncoder		m_rhiCommandEncoder{ nullptr };
	WGPURenderPassEncoder	m_rhiRenderPassEncoder{ nullptr };
	IVector2D				m_renderTargetDims{ 0 };
	void*					m_userData{ nullptr };
};