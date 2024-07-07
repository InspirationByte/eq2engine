#pragma once
#include "renderers/IShaderAPI.h"

class CWGPURenderPassRecorder : public IGPURenderPassRecorder
{
public:
	~CWGPURenderPassRecorder();

	IVector2D				GetRenderTargetDimensions() const { return m_renderTargetDims; }
	ArrayCRef<ETextureFormat>	GetRenderTargetFormats() const { return ArrayCRef(m_renderTargetsFormat); }
	ETextureFormat			GetDepthTargetFormat() const { return m_depthTargetFormat; }

	bool					IsDepthReadOnly() const { return m_depthReadOnly; }
	bool					IsStencilReadOnly() const { return m_stencilReadOnly; }
	int						GetTargetMultiSamples() const { return m_renderTargetMSAASamples; }

	void					DbgPopGroup() const;
	void					DbgPushGroup(const char* groupLabel) const;
	void					DbgAddMarker(const char* label) const;

	void					AddBundle(IGPURenderBundleRecorder* bundle);

	void					SetPipeline(IGPURenderPipeline* pipeline);
	IGPURenderPipelinePtr	GetPipeline() const { return m_pipeline; }

	void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets);
	void					SetVertexBuffer(int slot, IGPUBuffer* vertexBuffer, int64 offset = 0, int64 size = -1);
	void					SetIndexBuffer(IGPUBuffer* indexBuf, EIndexFormat indexFormat, int64 offset = 0, int64 size = -1);

	void					SetViewport(const AARectangle& rectangle, float minDepth, float maxDepth);
	void					SetScissorRectangle(const IAARectangle& rectangle);

	void					Draw(int vertexCount, int firstVertex, int instanceCount, int firstInstance = 0);
	void					DrawIndexed(int indexCount, int firstIndex, int instanceCount, int baseVertex = 0, int firstInstance = 0);
	void					DrawIndexedIndirect(IGPUBuffer* indirectBuffer, int indirectOffset);
	void					DrawIndirect(IGPUBuffer* indirectBuffer, int indirectOffset);

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
	IVector2D				m_renderTargetDims{ 0 };
	int						m_renderTargetMSAASamples{ 1 };
	bool					m_depthReadOnly{ false };
	bool					m_stencilReadOnly{ false };

	IGPURenderPipelinePtr	m_pipeline;

	WGPUCommandEncoder		m_rhiCommandEncoder{ nullptr };
	WGPURenderPassEncoder	m_rhiRenderPassEncoder{ nullptr };
	void*					m_userData{ nullptr };
};