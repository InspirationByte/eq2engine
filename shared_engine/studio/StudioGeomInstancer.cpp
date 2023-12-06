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
	//g_renderAPI->DestroyVertexBuffer(instanceVB);
}

void EGFInstBuffer::Init(int sizeOfInstance)
{
	instanceVB = g_renderAPI->CreateBuffer(BufferInfo(sizeOfInstance, EGF_INST_POOL_MAX_INSTANCES), BUFFERUSAGE_VERTEX | BUFFERUSAGE_COPY_DST, "EGFInstBuffer");
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

void CBaseEqGeomInstancer::Init( IVertexFormat* instVertexFormat, ArrayCRef<EGFHwVertex::VertexStreamId> instVertStreamMapping, int sizeOfInstance)
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
		g_renderAPI->DestroyVertexFormat(m_vertFormat);

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

		buffer.instanceVB->Update(dataStart, updateCount, updateStart);
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

	int instanceStreamId = -1;

	RenderDrawCmd drawCmd;
	drawCmd.SetInstanceFormat(m_vertFormat)
		.SetIndexBuffer(model->m_indexBuffer, static_cast<EIndexFormat>(model->m_indexFmt));

	// setup vertex buffers
	{
		int setVertStreams = 0;
		ArrayCRef<VertexLayoutDesc> layoutDescList = m_vertFormat->GetFormatDesc();
		for (int stream = 0; stream < layoutDescList.numElem(); ++stream)
		{
			const EGFHwVertex::VertexStreamId egfVertStreamId = (EGFHwVertex::VertexStreamId)layoutDescList[stream].userId;
			if (setVertStreams & (1 << int(egfVertStreamId)))
				continue;

			if (layoutDescList[stream].stepMode == VERTEX_STEPMODE_INSTANCE)
			{
				instanceStreamId = stream;
			}
			else
			{
				drawCmd.SetVertexBuffer(stream, model->m_vertexBuffers[egfVertStreamId]);
				setVertStreams |= (1 << int(egfVertStreamId));
			}
		}
	}

	ASSERT_MSG(instanceStreamId != -1, "No instance stream has been configured in vertex format!");

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

				drawCmd.SetInstanceData(buffer.instanceVB, m_instanceSize, buffer.numInstances, 0);

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
					drawCmd.SetDrawIndexed(static_cast<EPrimTopology>(meshRef.primType), meshRef.indexCount, meshRef.firstIndex);

					g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(nullptr, nullptr));
				}
			} // mGrp
		} // bodyGrp
	} // lod

	Invalidate();
}
