//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF model instancer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/IMaterialSystem.h"
#include "StudioGeomInstancer.h"

EGFInstBuffer::~EGFInstBuffer()
{
	PPFree(instances);
	g_renderAPI->DestroyVertexBuffer(instanceVB);
}

void EGFInstBuffer::Init(int sizeOfInstance)
{
	instanceVB = g_renderAPI->CreateVertexBuffer(BufferInfo(sizeOfInstance, EGF_INST_POOL_MAX_INSTANCES, BUFFER_DYNAMIC));
	instanceVB->SetFlags(VERTBUFFER_FLAG_INSTANCEDATA);

	instances = PPAlloc(sizeOfInstance * EGF_INST_POOL_MAX_INSTANCES);
}

CBaseEqGeomInstancer::CBaseEqGeomInstancer()
{
}

CBaseEqGeomInstancer::~CBaseEqGeomInstancer()
{
	Cleanup();
}

void CBaseEqGeomInstancer::ValidateAssert()
{
	ASSERT_MSG(m_vertFormat != nullptr, "Instancer is not valid - did you forgot to initialize it???");
}

void CBaseEqGeomInstancer::InitEx(ArrayCRef<VertexFormatDesc> instVertexFormat, ArrayCRef<EGFHwVertex::VertexStream> instVertStreamMapping, int sizeOfInstance)
{
	Cleanup();
	m_ownsVertexFormat = true;
	m_vertexStreamMapping = instVertStreamMapping;
	m_vertFormat = g_renderAPI->CreateVertexFormat("instancerFmt", instVertexFormat);
}

void CBaseEqGeomInstancer::Init( IVertexFormat* instVertexFormat, ArrayCRef<EGFHwVertex::VertexStream> instVertStreamMapping, int sizeOfInstance)
{
	Cleanup();
	m_instanceSize = sizeOfInstance;
	m_vertFormat = instVertexFormat;
	m_vertexStreamMapping = instVertStreamMapping;
	m_ownsVertexFormat = false;

	m_bodyGroupBounds[0] = 255;
	m_bodyGroupBounds[1] = 0;
	m_lodBounds[0] = 255;
	m_lodBounds[1] = 0;
	m_matGroupBounds[0] = 255;
	m_matGroupBounds[1] = 0;
}

void CBaseEqGeomInstancer::Cleanup()
{
	if(m_vertFormat && m_ownsVertexFormat)
	{
		g_renderAPI->Reset(STATE_RESET_VBO);
		g_renderAPI->ApplyBuffers();
		g_renderAPI->DestroyVertexFormat(m_vertFormat);
	}
	m_data.clear(true);
	m_vertFormat = nullptr;

	m_ownsVertexFormat = false;
}

bool CBaseEqGeomInstancer::HasInstances() const
{
	return m_bodyGroupBounds[0] != 255;
}

void* CBaseEqGeomInstancer::AllocInstance(int bodyGroup, int lod, int materialGroup)
{
	ushort bufId = MakeInstId(bodyGroup, lod, materialGroup);

	auto it = m_data.find(bufId);
	if (it.atEnd())
	{
		it = m_data.insert(bufId);
		EGFInstBuffer& newBuffer = *it;
		newBuffer.Init(m_instanceSize);
	}

	EGFInstBuffer& buffer = *it;
	if (buffer.numInstances >= EGF_INST_POOL_MAX_INSTANCES)
	{
		// TODO: create new pool
		return nullptr;
	}

	m_bodyGroupBounds[0] = min(m_bodyGroupBounds[0], bodyGroup);
	m_bodyGroupBounds[1] = max(m_bodyGroupBounds[1], bodyGroup);
	m_lodBounds[0] = min(m_lodBounds[0], lod);
	m_lodBounds[1] = max(m_lodBounds[1], lod);
	m_matGroupBounds[0] = min(m_matGroupBounds[0], materialGroup);
	m_matGroupBounds[1] = max(m_matGroupBounds[1], materialGroup);

	const int instanceId = buffer.numInstances++;
	return (ubyte*)buffer.instances + instanceId * m_instanceSize;
}

void CBaseEqGeomInstancer::Upload()
{
	// update all HW instance buffers
	for (auto it = m_data.begin(); !it.atEnd(); ++it)
	{
		EGFInstBuffer& buffer = *it;
		if (buffer.upToDateInstanes == buffer.numInstances)
			continue;

		const int updateStart = buffer.upToDateInstanes;
		const int updateCount = buffer.numInstances - buffer.upToDateInstanes;
		void* dataStart = (ubyte*)buffer.instances + buffer.upToDateInstanes * m_instanceSize;
		const bool discard = (updateStart == 0);

		buffer.instanceVB->Update(dataStart, updateCount, updateStart, discard);
		buffer.upToDateInstanes = buffer.numInstances;
	}
}

void CBaseEqGeomInstancer::Invalidate()
{
	for (auto it = m_data.begin(); !it.atEnd(); ++it)
	{
		EGFInstBuffer& buffer = *it;
		buffer.upToDateInstanes = buffer.numInstances = 0;
	}

	m_bodyGroupBounds[0] = 255;
	m_bodyGroupBounds[1] = 0;
	m_lodBounds[0] = 255;
	m_lodBounds[1] = 0;
	m_matGroupBounds[0] = 255;
	m_matGroupBounds[1] = 0;
}

void CBaseEqGeomInstancer::Draw( CEqStudioGeom* model )
{
	if(!model)
		return;

	ASSERT(model->GetInstancer() == this);

	Upload();

	// proceed to render
	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

	const int maxVertexCount = model->m_vertexBuffers[EGFHwVertex::VERT_POS_UV]->GetVertexCount();
	int instanceStreamId = -1;

	RenderDrawCmd drawCmd;
	drawCmd.vertexLayout = m_vertFormat;

	// setup vertex buffers
	{
		ArrayCRef<VertexFormatDesc> fmtDesc = m_vertFormat->GetFormatDesc();

		int setVertStreams = 0;
		for (int i = 0; i < fmtDesc.numElem(); ++i)
		{
			const VertexFormatDesc& desc = fmtDesc[i];
			const EGFHwVertex::VertexStream vertStreamId = m_vertexStreamMapping[desc.streamId];

			if (setVertStreams & (1 << int(vertStreamId)))
				continue;

			setVertStreams |= (1 << int(vertStreamId));
			if (instanceStreamId != desc.streamId && (desc.attribType & VERTEXATTRIB_FLAG_INSTANCE))
			{
				ASSERT_MSG(instanceStreamId == -1, "Multiple instance streams not yet supported");
				instanceStreamId = desc.streamId;
			}
			else
			{
				drawCmd.vertexBuffers[desc.streamId] = model->m_vertexBuffers[vertStreamId];
			}
		}
	}

	drawCmd.indexBuffer = model->m_indexBuffer;

	const studioHdr_t& studio = model->GetStudioHdr();
	for(int lod = m_lodBounds[0]; lod <= m_lodBounds[1]; lod++)
	{
		for(int bodyGrp = m_bodyGroupBounds[0]; bodyGrp <= m_bodyGroupBounds[1]; bodyGrp++)
		{
			const int bodyGroupLodIndex = studio.pBodyGroups(bodyGrp)->lodModelIndex;
			const studioLodModel_t* lodModel = studio.pLodModel(bodyGroupLodIndex);

			// get the right LOD model number
			int bodyGroupLOD = lod;
			uint8 modelDescId = EGF_INVALID_IDX;
			do
			{
				modelDescId = lodModel->modelsIndexes[bodyGroupLOD];
				bodyGroupLOD--;
			} while (modelDescId == EGF_INVALID_IDX && bodyGroupLOD >= 0);

			if (modelDescId == EGF_INVALID_IDX)
				continue;
	
			const studioMeshGroupDesc_t* modDesc = studio.pMeshGroupDesc(modelDescId);

			for (int mGrp = m_matGroupBounds[0]; mGrp <= m_matGroupBounds[1]; mGrp++)
			{
				const ushort dataId = MakeInstId(bodyGrp, lod, mGrp);
				auto dataIt = m_data.find(dataId);
				if (dataIt.atEnd())
					continue;

				const EGFInstBuffer& buffer = *dataIt;
				if (buffer.numInstances == 0)
					continue;

				drawCmd.vertexBuffers[instanceStreamId] = buffer.instanceVB;
				drawCmd.instanceBuffer = buffer.instanceVB;

				// render model groups that in this body group
				for (int i = 0; i < modDesc->numMeshes; i++)
				{
					const int materialIndex = modDesc->pMesh(i)->materialIndex;
					IMaterial* material = model->GetMaterial(materialIndex, mGrp);

					// sadly, instancer won't draw any transparent objects due to sorting problems
					if (material->GetFlags() & MATERIAL_FLAG_TRANSPARENT)
						continue;

					//materials->SetSkinningEnabled(true);
					drawCmd.material = material;
					

					const CEqStudioGeom::HWGeomRef::Mesh& meshRef = model->m_hwGeomRefs[modelDescId].meshRefs[i];
					drawCmd.primitiveTopology = (EPrimTopology)meshRef.primType;
					drawCmd.SetDrawIndexed(meshRef.indexCount, meshRef.firstIndex, maxVertexCount);

					g_matSystem->Draw(drawCmd);
				}
			} // mGrp
		} // bodyGrp
	} // lod

	Invalidate();
}
