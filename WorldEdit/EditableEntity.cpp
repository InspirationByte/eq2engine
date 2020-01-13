//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable entity class
//////////////////////////////////////////////////////////////////////////////////

#include "EditableEntity.h"
#include "IDebugOverlay.h"
#include "EditorLevel.h"
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/MeshBuilder.h"
#include "SelectionEditor.h"
#include "EditorMainFrame.h"
#include "EditableEntityParameters.h"

#include "utils/strtools.h"


CEditableEntity::CEditableEntity()
{
	m_pDefinition = NULL;
	m_pModel = NULL;
	m_pSprite = NULL;
	m_szClassName = "default";
}

CEditableEntity::~CEditableEntity()
{

}

extern void DkDrawSphere(const Vector3D& origin, float radius, int sides);
extern void DkDrawFilledSphere(const Vector3D& origin, float radius, int sides);

void RenderPhysModel(IEqModel* pModel)
{
	if(!g_editormainframe->IsPhysmodelDrawEnabled() || pModel->GetHWData()->physModel.numObjects == 0)
		return;

	g_pShaderAPI->Reset();
	materials->SetDepthStates(true,false);
	materials->SetRasterizerStates(CULL_BACK,FILL_WIREFRAME);
	g_pShaderAPI->Apply();

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	studioPhysData_t& phys_data = pModel->GetHWData()->physModel;

	for (int i = 0; i < phys_data.numObjects; i++)
	{
		for (int j = 0; j < phys_data.objects[i].object.numShapes; j++)
		{
			int nShape = phys_data.objects[i].object.shape_indexes[j];
			if (nShape < 0 || nShape > phys_data.numShapes)
			{
				continue;
			}

			int startIndex = phys_data.shapes[nShape].shape_info.startIndices;
			int moveToIndex = startIndex + phys_data.shapes[nShape].shape_info.numIndices;
			/*
			if (m_boneTransforms != NULL && m_pRagdoll)
			{
				int visualMatrixIdx = m_pRagdoll->m_pBoneToVisualIndices[i];
				Matrix4x4 boneFrame = m_pRagdoll->m_pJoints[i]->GetFrameTransformA();

				materials->SetMatrix(MATRIXMODE_WORLD, worldPosMatrix*transpose(!boneFrame*m_boneTransforms[visualMatrixIdx]));
				materials->BindMaterial(materials->GetDefaultMaterial());
			}*/

			meshBuilder.Begin(PRIM_TRIANGLES);
			for (int k = startIndex; k < moveToIndex; k++)
			{
				meshBuilder.Color4f(1, 0, 1, 1);
				meshBuilder.Position3fv(phys_data.vertices[phys_data.indices[k]]);// + phys_data.objects[i].object.offset);

				meshBuilder.AdvanceVertex();
			}
			meshBuilder.End();
		}
	}
}

extern float g_frametime;

void CEditableEntity::Render(int nViewRenderFlags)
{
#ifdef VR_TEST
	
	dlight_t *light = viewrenderer->AllocLight();
	light->radius = 512;
	light->position = m_position;
	light->color = m_groupColor;
	light->nFlags = LFLAG_NOSHADOWS;

	viewrenderer->AddLight( light );
#endif // VR_TEST

	//AdvanceFrame(g_frametime);

	//UpdateBones();

	//Update3DView();

	// if it has model, render it, or draw bbox.
	if(m_pModel)
	{
		materials->SetCullMode(CULL_BACK);
		materials->SetMatrix(MATRIXMODE_WORLD, GetRenderWorldTransform());

		studiohdr_t* pHdr = m_pModel->GetHWData()->studio;

		//materials->SetSkinningEnabled(true);

		int bodyGroupFlags = 0x1;
		int nStartLOD = 0;

		for (int i = 0; i < pHdr->numBodyGroups; i++)
		{
			// check bodygroups for rendering
			if (!(bodyGroupFlags & (1 << i)))
				continue;

			int bodyGroupLOD = nStartLOD;
			int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
			studiolodmodel_t* lodModel = pHdr->pLodModel(nLodModelIdx);

			int nModDescId = lodModel->modelsIndexes[bodyGroupLOD];

			// get the right LOD model number
			while (nModDescId == -1 && bodyGroupLOD > 0)
			{
				bodyGroupLOD--;
				nModDescId = lodModel->modelsIndexes[bodyGroupLOD];
			}

			if (nModDescId == -1)
				continue;

			studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

			// render model groups that in this body group
			for (int j = 0; j < modDesc->numGroups; j++)
			{
				materials->SetSkinningEnabled(true);

				materials->BindMaterial(m_pModel->GetMaterial(modDesc->pGroup(j)->materialIndex), 0);

				//m_pModel->PrepareForSkinning(m_boneTransforms);
				m_pModel->DrawGroup(nModDescId, j);

				materials->SetSkinningEnabled(false);
			}
		}


		materials->SetSkinningEnabled(false);

		RenderPhysModel(m_pModel);
	}
	else
	{
		CMeshBuilder builder(materials->GetDynamicMesh());

		g_pShaderAPI->Reset(STATE_RESET_VBO);

		ColorRGBA color = ColorRGBA(1,0,1,1);

		if(m_pDefinition)
		{
			if(m_pDefinition->colorfromkey)
				color = ColorRGBA(m_groupColor,1);
			else
				color = m_pDefinition->drawcolor;
		}

		if(m_nFlags & EDFL_SELECTED)
			color = ColorRGBA(1,0,0,1);

		materials->SetAmbientColor(color);

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
		
		Vector3D min = GetBBoxMins();
		Vector3D max = GetBBoxMaxs();

		if(m_pSprite)
		{
			if(m_pDefinition)
			{
				if(m_pDefinition->colorfromkey)
					color = ColorRGBA(m_groupColor,1);
				else
					color = ColorRGBA(1);
			}

			if(m_nFlags & EDFL_SELECTED)
				color = ColorRGBA(1,0,0,1);

			materials->SetAmbientColor(color);

			materials->SetCullMode(CULL_NONE);
			materials->BindMaterial(m_pSprite);

			float size = 16;
			if(m_pDefinition)
				size = length(m_pDefinition->bboxmaxs);

			float texW = size*0.5f;
			float texH = size*0.5f;

			Matrix4x4 view;
			materials->GetMatrix(MATRIXMODE_VIEW, view);
			view = scale4(1.0f,1.0f,-1.0f)*view;

			Vector3D up = view.rows[1].xyz();
			Vector3D rt = view.rows[0].xyz();

			builder.Begin(PRIM_TRIANGLE_STRIP);
				builder.Position3fv(GetPosition() - up*texW - rt*texH);
				builder.TexCoord2f(0,1);
				builder.AdvanceVertex();

				builder.Position3fv(GetPosition() + up*texW - rt*texH);
				builder.TexCoord2f(0,0);
				builder.AdvanceVertex();

				builder.Position3fv(GetPosition() - up*texW + rt*texH);
				builder.TexCoord2f(1,1);
				builder.AdvanceVertex();

				builder.Position3fv(GetPosition() + up*texW + rt*texH);
				builder.TexCoord2f(1,0);
				builder.AdvanceVertex();
			builder.End();
		}
		else
		{
			materials->BindMaterial(g_pLevel->GetFlatMaterial());

			builder.Begin(PRIM_TRIANGLE_STRIP);
				builder.Position3f(min.x, max.y, max.z);
				builder.AdvanceVertex();
				builder.Position3f(max.x, max.y, max.z);
				builder.AdvanceVertex();
				builder.Position3f(min.x, max.y, min.z);
				builder.AdvanceVertex();
				builder.Position3f(max.x, max.y, min.z);
				builder.AdvanceVertex();
				builder.Position3f(min.x, min.y, min.z);
				builder.AdvanceVertex();
				builder.Position3f(max.x, min.y, min.z);
				builder.AdvanceVertex();
				builder.Position3f(min.x, min.y, max.z);
				builder.AdvanceVertex();
				builder.Position3f(max.x, min.y, max.z);
				builder.AdvanceVertex();

				builder.Position3f(max.x, min.y, max.z);
				builder.AdvanceVertex();

				builder.Position3f(max.x, min.y, min.z);
				builder.AdvanceVertex();

				builder.Position3f(max.x, min.y, min.z);
				builder.AdvanceVertex();
				builder.Position3f(max.x, max.y, min.z);
				builder.AdvanceVertex();
				builder.Position3f(max.x, min.y, max.z);
				builder.AdvanceVertex();
				builder.Position3f(max.x, max.y, max.z);
				builder.AdvanceVertex();
				builder.Position3f(min.x, min.y, max.z);
				builder.AdvanceVertex();
				builder.Position3f(min.x, max.y, max.z);
				builder.AdvanceVertex();
				builder.Position3f(min.x, min.y, min.z);
				builder.AdvanceVertex();
				builder.Position3f(min.x, max.y, min.z);
				builder.AdvanceVertex();
			builder.End();
		}

		for(int i = 0; i < m_EntTypedVars.numElem(); i++)
			m_EntTypedVars[i]->Render(nViewRenderFlags);

		/*
		if((m_nFlags & EDFL_SELECTED) && m_pDefinition && m_pDefinition->showradius)
		{
			materials->SetAmbientColor(ColorRGBA(m_groupColor,0.25f));
			materials->BindMaterial(g_pLevel->GetFlatMaterial());

			DkDrawSphere(GetPosition(), m_fRadius, 32);
			DkDrawFilledSphere(GetPosition(), m_fRadius, 32);
		}
		*/

		if(m_pDefinition && m_pDefinition->showanglearrow)
		{
			materials->SetAmbientColor(color*0.8f);
			materials->BindMaterial(g_pLevel->GetFlatMaterial());

			Vector3D start = GetPosition();

			Matrix4x4 result_rot = !rotateXYZ4(DEG2RAD(m_angles.x), DEG2RAD(m_angles.y), DEG2RAD(m_angles.z));

			Vector3D end = start + result_rot.rows[2].xyz()*50.0f;
			Vector3D end2 = start + result_rot.rows[2].xyz()*40.0f;

			builder.Begin(PRIM_LINES);

				builder.Position3fv(start);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end2 + result_rot.rows[0].xyz()*10.0f);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end2 + result_rot.rows[1].xyz()*10.0f);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end2 - result_rot.rows[0].xyz()*10.0f);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end2 - result_rot.rows[1].xyz()*10.0f);
				builder.AdvanceVertex();

			builder.End();
		}

	}
	
}

const char* CEditableEntity::GetClassname()
{
	return m_szClassName.GetData();
}

void CEditableEntity::SetClassName(const char* pszClassname)
{
	m_szClassName = pszClassname;

	m_pModel = NULL;
	m_pSprite = NULL;
	
	// set definition
	m_pDefinition = EDef_Find(pszClassname);

	if(m_pDefinition && m_pDefinition->colorfromkey)
		m_groupColor = color3_white;

	if(m_pDefinition)
	{
		m_pModel = m_pDefinition->model;
		m_pSprite = m_pDefinition->sprite;
	}

	for(int i = 0; i < m_EntTypedVars.numElem(); i++)
		delete m_EntTypedVars[i];

	m_EntTypedVars.clear();

	// update paramters
	UpdateParameters(); 
}

void CEditableEntity::RenderGhost(Vector3D &addpos, Vector3D &addscale, Vector3D &addrot, bool use_center, Vector3D &center)
{
	//debugoverlay->GetFont()->DrawText(GetClassName(),

	materials->SetCullMode(CULL_BACK);

	// if it has model, render it, or draw bbox.
	if(m_pModel)
	{
		Matrix4x4 curr_rotation = rotateXYZ4(DEG2RAD(m_angles.x),DEG2RAD(m_angles.y),DEG2RAD(m_angles.z));
		Matrix4x4 rotation = rotateXYZ4(DEG2RAD(addrot.x),DEG2RAD(addrot.y),DEG2RAD(addrot.z));

		Matrix4x4 result_rot = rotation*curr_rotation;

		result_rot = translate(m_position+addpos)*result_rot*scale4(1.0f,1.0f,1.0f);

		if(use_center)
		{
			Vector3D diffPos = m_position - center;

			// rotate diff
			Vector3D pos = transpose(!rotation)*diffPos + center;

			// is at center now
			curr_rotation = rotateXYZ4(DEG2RAD(m_angles.x),DEG2RAD(m_angles.y),DEG2RAD(m_angles.z)) * translate(diffPos);

			// rotate this
			result_rot = curr_rotation*rotation * translate(addpos);


		}

		materials->SetMatrix(MATRIXMODE_WORLD, result_rot);

		//m_pModel->PrepareForSkinning(m_BoneMatrixList);

		studiohdr_t* pHdr = m_pModel->GetHWData()->studio;
		int nLod = 0;

		materials->BindMaterial(g_pLevel->GetFlatMaterial());

		for(int i = 0; i < pHdr->numBodyGroups; i++)
		{
			int nLodableModelIndex = pHdr->pBodyGroups(i)->lodModelIndex;
			int nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];

			while(nLod > 0 && nModDescId != -1)
			{
				nLod--;
				nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];
			}

			if(nModDescId == -1)
				continue;

			for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numGroups; j++)
			{
				//IMaterial* pMaterial = m_pModel->GetMaterial(nModDescId, j);
				m_pModel->DrawGroup(nModDescId, j);
			}
		}
	}
	else
	{
		CMeshBuilder builder(materials->GetDynamicMesh());

		g_pShaderAPI->Reset(STATE_RESET_VBO);

		if(m_pDefinition)
		{
			if(m_pDefinition->colorfromkey)
				materials->SetAmbientColor(ColorRGBA(m_groupColor,1));
			else
				materials->SetAmbientColor(m_pDefinition->drawcolor);
		}
		else
			materials->SetAmbientColor(ColorRGBA(1,0,1,1));

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
		materials->BindMaterial(g_pLevel->GetFlatMaterial());
		
		Vector3D min = GetBBoxMins()+addpos;
		Vector3D max = GetBBoxMaxs()+addpos;

		builder.Begin(PRIM_LINES);
			builder.Position3f(min.x, max.y, min.z);
			builder.AdvanceVertex();
			builder.Position3f(min.x, max.y, max.z);
			builder.AdvanceVertex();

			builder.Position3f(max.x, max.y, max.z);
			builder.AdvanceVertex();
			builder.Position3f(max.x, max.y, min.z);
			builder.AdvanceVertex();

			builder.Position3f(max.x, min.y, min.z);
			builder.AdvanceVertex();
			builder.Position3f(max.x, min.y, max.z);
			builder.AdvanceVertex();

			builder.Position3f(min.x, min.y, max.z);
			builder.AdvanceVertex();
			builder.Position3f(min.x, min.y, min.z);
			builder.AdvanceVertex();

			builder.Position3f(min.x, min.y, max.z);
			builder.AdvanceVertex();
			builder.Position3f(min.x, max.y, max.z);
			builder.AdvanceVertex();

			builder.Position3f(max.x, min.y, max.z);
			builder.AdvanceVertex();
			builder.Position3f(max.x, max.y, max.z);
			builder.AdvanceVertex();

			builder.Position3f(min.x, min.y, min.z);
			builder.AdvanceVertex();
			builder.Position3f(min.x, max.y, min.z);
			builder.AdvanceVertex();

			builder.Position3f(max.x, min.y, min.z);
			builder.AdvanceVertex();
			builder.Position3f(max.x, max.y, min.z);
			builder.AdvanceVertex();

			builder.Position3f(min.x, max.y, min.z);
			builder.AdvanceVertex();
			builder.Position3f(max.x, max.y, min.z);
			builder.AdvanceVertex();

			builder.Position3f(min.x, max.y, max.z);
			builder.AdvanceVertex();
			builder.Position3f(max.x, max.y, max.z);
			builder.AdvanceVertex();

			builder.Position3f(min.x, min.y, min.z);
			builder.AdvanceVertex();
			builder.Position3f(max.x, min.y, min.z);
			builder.AdvanceVertex();

			builder.Position3f(min.x, min.y, max.z);
			builder.AdvanceVertex();
			builder.Position3f(max.x, min.y, max.z);
			builder.AdvanceVertex();

			if(m_pDefinition && m_pDefinition->showanglearrow)
			{
				Vector3D start = GetPosition();

				Matrix4x4 curr_rotation = rotateXYZ4(DEG2RAD(m_angles.x),DEG2RAD(m_angles.y),DEG2RAD(m_angles.z));
				Matrix4x4 rotation = rotateXYZ4(DEG2RAD(addrot.x),DEG2RAD(addrot.y),DEG2RAD(addrot.z));

				Matrix4x4 result_rot = !(rotation*curr_rotation);

				Vector3D end = start + result_rot.rows[2].xyz()*50.0f;
				Vector3D end2 = start + result_rot.rows[2].xyz()*40.0f;

				builder.Position3fv(start);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end2 + result_rot.rows[0].xyz()*10.0f);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end2 + result_rot.rows[1].xyz()*10.0f);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end2 - result_rot.rows[0].xyz()*10.0f);
				builder.AdvanceVertex();

				builder.Position3fv(end);
				builder.AdvanceVertex();

				builder.Position3fv(end2 - result_rot.rows[1].xyz()*10.0f);
				builder.AdvanceVertex();
			}
		builder.End();
	}

	for(int i = 0; i < m_EntTypedVars.numElem(); i++)
		m_EntTypedVars[i]->RenderGhost(0);

	/*
	if((m_nFlags & EDFL_SELECTED) && m_pDefinition && m_pDefinition->showradius)
	{
		for(int i = 0; i < m_EntTypedVars.numElem(); i++)
			m_EntTypedVars[i]->RenderGhost(0);

		
		materials->GetConfiguration().wireframeMode = false;

		materials->SetAmbientColor(ColorRGBA(m_groupColor,0.25f));
		materials->BindMaterial(g_pLevel->GetFlatMaterial());

		DkDrawSphere(GetPosition(), m_fRadius, 32);
		DkDrawFilledSphere(GetPosition(), m_fRadius, 32);
		
	}
	*/
}

Matrix4x4 CEditableEntity::GetRenderWorldTransform()
{
	return translate(m_position)*rotateXYZ4(DEG2RAD(m_angles.x),DEG2RAD(m_angles.y),DEG2RAD(m_angles.z))*scale4(1.0f,1.0f,1.0f);
}

// rendering bbox
Vector3D CEditableEntity::GetBBoxMins()
{
	if(m_pModel)
	{
		const BoundingBox& modelbox = m_pModel->GetAABB();
		Matrix4x4 trans = GetRenderWorldTransform();

		/*
		BoundingBox bbox;
		bbox.AddVertex((trans*Vector4D(mins, 1)).xyz());
		bbox.AddVertex((trans*Vector4D(maxs, 1)).xyz());
		*/

		BoundingBox bbox;

		for(int i = 0; i < 8; i++)
			bbox.AddVertex((trans*Vector4D(modelbox.GetVertex(i), 1)).xyz());

		return bbox.minPoint;
	}
	else
	{
		if(m_pDefinition)
			return m_pDefinition->bboxmins + m_position;

		return Vector3D(-4) + m_position;
	}
}

Vector3D CEditableEntity::GetBBoxMaxs()
{
	if(m_pModel)
	{
		const BoundingBox& modelbox = m_pModel->GetAABB();
		Matrix4x4 trans = GetRenderWorldTransform();

		BoundingBox bbox;

		for(int i = 0; i < 8; i++)
			bbox.AddVertex((trans*Vector4D(modelbox.GetVertex(i), 1)).xyz());

		/*
		bbox.AddVertex((trans*Vector4D(mins, 1)).xyz());
		bbox.AddVertex((trans*Vector4D(maxs, 1)).xyz());
		*/

		return bbox.maxPoint;
	}
	else
	{
		if(m_pDefinition)
			return m_pDefinition->bboxmaxs + m_position;

		return Vector3D(4) + m_position;
	}
}

Vector3D CEditableEntity::GetBoundingBoxMins()
{
	return GetBBoxMins();
}

Vector3D CEditableEntity::GetBoundingBoxMaxs()
{
	return GetBBoxMaxs();
}

void CEditableEntity::OnRemove(bool bOnLevelCleanup)
{
	for(int i = 0; i < m_EntTypedVars.numElem(); i++)
		delete m_EntTypedVars[i];

	m_EntTypedVars.clear();

	//DestroyAnimationThings();
}

// saves this object
bool CEditableEntity::WriteObject(IVirtualStream* pStream)
{
	char str[128];
	strcpy(str, m_szClassName.GetData());
	pStream->Write(str, 1, 128);

	strcpy(str, m_szName.GetData());
	pStream->Write(str, 1, 128);

	int numKeys = m_kvPairs.numElem();
	pStream->Write(&numKeys, 1, sizeof(int));

	for(int i = 0; i < numKeys; i++)
		pStream->Write(&m_kvPairs[i], 1, sizeof(edkeyvalue_t));

	int numOutputs = m_Outputs.numElem();
	pStream->Write(&numOutputs, 1, sizeof(int));

	for(int i = 0; i < m_Outputs.numElem(); i++)
	{
		edkeyvalue_t pPair;
		strcpy(pPair.name, "EntityOutput");
		strcpy(pPair.value, varargs("%s,%s,%s,%d,%f,%s",	m_Outputs[i].szOutputName.GetData(),
															m_Outputs[i].szTargetInput.GetData(),
															m_Outputs[i].szOutputTarget.GetData(),
															m_Outputs[i].nFireTimes,
															m_Outputs[i].fDelay,
															m_Outputs[i].szOutputValue.GetData()));

		pStream->Write(&pPair, 1, sizeof(edkeyvalue_t));
	}

	return true;
}

// read this object
bool CEditableEntity::ReadObject(IVirtualStream* pStream)
{
	char str[128];
	pStream->Read(str, 1, 128);

	SetClassName(str);

	str[0] = 0;
	pStream->Read(str, 1, 128);

	SetName(str);

	int numKeys = 0;
	pStream->Read(&numKeys, 1, sizeof(int));

	//Msg("keys: %d\n", numKeys);

	for(int i = 0; i < numKeys; i++)
	{
		edkeyvalue_t pair;

		pStream->Read(&pair, 1, sizeof(edkeyvalue_t));
		//m_kvPairs.append(pair);
		SetKey(pair.name, pair.value, false);
	}

	int numOutputs = 0;
	pStream->Read(&numOutputs, 1, sizeof(int));

	//Msg("object outputs: %d\n", numOutputs);

	for(int i = 0; i < numOutputs; i++)
	{
		edkeyvalue_t pair;

		pStream->Read(&pair, 1, sizeof(edkeyvalue_t));

		AddOutput(pair.value);
	}

	UpdateParameters();

	return true;
}

/*
// called when whole level builds
void CEditableEntity::BuildObject(level_build_data_t* pLevelBuildData)
{
	kvsection_t* newSec = new kvsection_t;
	strcpy(newSec->sectionname, "entitydesc");

	pLevelBuildData->entitydata.GetRootSection()->sections.append(newSec);
	
	kvkeyvaluepair_t* pClassName = new kvkeyvaluepair_t;
	strcpy(pClassName->keyname, "classname");
	strcpy(pClassName->value, m_szClassName.getData());
	
	newSec->pKeys.append(pClassName);

	for(int i = 0; i < m_kvPairs.numElem(); i++)
	{
		kvkeyvaluepair_t* pNewPair = new kvkeyvaluepair_t;
		*pNewPair = m_kvPairs[i];

		newSec->pKeys.append(pNewPair);
	}
}
*/

// stores object in keyvalues
void CEditableEntity::SaveToKeyValues(kvkeybase_t* pSection)
{
	kvkeybase_t* pParamsSec = pSection->AddKeyBase("params");

	pParamsSec->AddKeyBase("classname", m_szClassName.GetData());
	pParamsSec->AddKeyBase("name", m_szName.GetData());

	for(int i = 0; i < m_kvPairs.numElem(); i++)
		pParamsSec->AddKeyBase(m_kvPairs[i].name, m_kvPairs[i].value);

	for(int i = 0; i < m_Outputs.numElem(); i++)
	{
		pParamsSec->AddKeyBase("EntityOutput", varargs("%s,%s,%s,%d,%f,%s",	m_Outputs[i].szOutputName.GetData(),
																m_Outputs[i].szTargetInput.GetData(),
																m_Outputs[i].szOutputTarget.GetData(),
																m_Outputs[i].nFireTimes,
																m_Outputs[i].fDelay,
																m_Outputs[i].szOutputValue.GetData()));
	}
}

int splitstring_singlecharseparator(char* str, char separator, DkList<EqString> &outStrings)
{
	char c = str[0];

	char* iterator = str;

	char* pFirst = str;
	char* pLast = NULL;

	while(c != 0)
	{
		c = *iterator;

		if(c == separator || c == 0)
		{
			pLast = iterator;

			int char_count = pLast - pFirst;

			if(char_count > 0)
			{
				// add new string
				outStrings.append(EqString(pFirst, 0, char_count));
			}

			pFirst = iterator+1;
		}

		iterator++;
	}

	return outStrings.numElem();
}

void CEditableEntity::AddOutput(char* pszString)
{
	OutputData_t outdata;

	// subdivide the string
	DkList<EqString> strings;
	strings.resize(5);

	xstrsplit(pszString, ",", strings);

	outdata.szOutputName = strings[0];
	outdata.szTargetInput = strings[1];
	outdata.szOutputTarget = strings[2];
	outdata.nFireTimes = atoi(strings[3].GetData());
	outdata.fDelay = atof(strings[4].GetData());

	if(strings.numElem() > 5)
		outdata.szOutputValue = strings[5];
	else
		outdata.szOutputValue = "";

	m_Outputs.append(outdata);
}

// stores object in keyvalues
bool CEditableEntity::LoadFromKeyValues(kvkeybase_t* pSection)
{
	kvkeybase_t* pParamsSec = pSection->FindKeyBase("params", KV_FLAG_SECTION);

	for(int i = 0; i < pParamsSec->keys.numElem(); i++)
	{
		//Msg("%s = %s\n", pParamsSec->pKeys[i]->keyname, pParamsSec->pKeys[i]->value);
		if(!stricmp(pParamsSec->keys[i]->name, "classname"))
		{
			SetClassName(KV_GetValueString(pParamsSec->keys[i]));
		}
		else if(!stricmp(pParamsSec->keys[i]->name, "targetname") || !stricmp(pParamsSec->keys[i]->name, "name"))
		{
			SetName(KV_GetValueString(pParamsSec->keys[i]));
		}
		else if(!stricmp(pParamsSec->keys[i]->name, "EntityOutput"))
		{
			DkList<EqString> strings;

			splitstring_singlecharseparator((char*)KV_GetValueString(pParamsSec->keys[i]), ';', strings);

			for(int outp = 0; outp < strings.numElem(); outp++)
				AddOutput((char*)strings[outp].GetData());
		}
		else
			SetKey(pParamsSec->keys[i]->name, KV_GetValueString(pParamsSec->keys[i]), false);
	}

	UpdateParameters();

	return true;
}

// copies this object
CBaseEditableObject* CEditableEntity::CloneObject()
{
	CEditableEntity* pClone = new CEditableEntity;

	pClone->SetClassName(GetClassname());
	pClone->SetName(GetName());
	pClone->m_angles = m_angles;
	pClone->m_position = m_position;
	pClone->m_scale = m_scale;

	pClone->m_kvPairs.append(m_kvPairs);
	//pClone->m_kvPairVars.append(m_kvPairVars);

	pClone->m_Outputs.append(m_Outputs);

	pClone->UpdateParameters();

	return pClone;
}

void CEditableEntity::SetName(const char* pszName)
{
	//SetKey("name", (char*)pszName);

	CBaseEditableObject::SetName(pszName);
}

// selection ray
float CEditableEntity::CheckLineIntersection(const Vector3D &start, const Vector3D &end, Vector3D &outPos)
{
	Vector3D ray_dir = end-start;

	if(m_pModel)
	{
		float best_dist = MAX_COORD_UNITS;
		float fraction = 1.0f;

		studiohdr_t* pHdr = m_pModel->GetHWData()->studio;
		int nLod = 0;

		Matrix4x4 transform = GetRenderWorldTransform();

		for(int i = 0; i < pHdr->numBodyGroups; i++)
		{
			int nLodableModelIndex = pHdr->pBodyGroups(i)->lodModelIndex;
			int nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];

			while(nLod > 0 && nModDescId != -1)
			{
				nLod--;
				nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];
			}

			if(nModDescId == -1)
				continue;

			for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numGroups; j++)
			{
				modelgroupdesc_t* pGroup = pHdr->pModelDesc(nModDescId)->pGroup(j);

				uint32 *pIndices = pGroup->pVertexIdx(0);

				int numIndices = (pGroup->primitiveType == EGFPRIM_TRI_STRIP) ? pGroup->numIndices - 2 : pGroup->numIndices;
				int indexStep = (pGroup->primitiveType == EGFPRIM_TRI_STRIP) ? 1 : 3;

				for (uint32 k = 0; k < numIndices; k += indexStep)
				{
					// skip strip degenerates
					if (pIndices[k] == pIndices[k + 1] || pIndices[k] == pIndices[k + 2] || pIndices[k + 1] == pIndices[k + 2])
						continue;

					Vector3D v0, v1, v2;

					int even = k % 2;
					// handle flipped triangles on EGFPRIM_TRI_STRIP
					if (even && pGroup->primitiveType == EGFPRIM_TRI_STRIP)
					{
						v0 = pGroup->pVertex(pIndices[k + 2])->point;
						v1 = pGroup->pVertex(pIndices[k + 1])->point;
						v2 = pGroup->pVertex(pIndices[k])->point;
					}
					else
					{
						v0 = pGroup->pVertex(pIndices[k])->point;
						v1 = pGroup->pVertex(pIndices[k + 1])->point;
						v2 = pGroup->pVertex(pIndices[k + 2])->point;
					}

					v0 = (transform*Vector4D(v0,1)).xyz();
					v1 = (transform*Vector4D(v1,1)).xyz();
					v2 = (transform*Vector4D(v2,1)).xyz();

					float dist = MAX_COORD_UNITS+1;

					if(IsRayIntersectsTriangle(v0,v1,v2, start, ray_dir, dist))
					{
						if(dist < best_dist && dist > 0)
						{
							best_dist = dist;
							fraction = dist;

							outPos = lerp(start, end, dist);
						}
					}
				}
			}
		}

		return fraction;
	}
	else
	{
		Volume bboxVolume;
		bboxVolume.LoadBoundingBox(GetBBoxMins(), GetBBoxMaxs());

		if(bboxVolume.IsIntersectsRay(start, normalize(ray_dir), outPos))
		{
			float frac = lineProjection(start,end,outPos);

			if(frac < 1 && frac > 0)
				return frac;
		}
	}

	return 1.0f;
}

void CEditableEntity::Translate(Vector3D &position)
{
	m_position += position;
	SetKey("origin", varargs("%g %g %g", m_position.x,m_position.y,m_position.z));
}

void CEditableEntity::Scale(Vector3D &scale, bool use_center, Vector3D &scale_center)
{
	m_scale *= scale;
	SetKey("scale", varargs("%g %g %g", m_scale.x,m_scale.y,m_scale.z));
}

void CEditableEntity::SetPosition(Vector3D &position)
{
	m_position = position;
	SetKey("origin", varargs("%g %g %g", m_position.x,m_position.y,m_position.z));
}

void CEditableEntity::SetAngles(Vector3D &angles)
{
	m_angles = angles;
	SetKey("angles", varargs("%g %g %g", m_angles.x,m_angles.y,m_angles.z));
}

void CEditableEntity::SetScale(Vector3D &scale)
{
	m_scale = scale;
	SetKey("scale", varargs("%g %g %g", m_scale.x,m_scale.y,m_scale.z));
}

void CEditableEntity::Rotate(Vector3D &rotation_angles, bool use_center, Vector3D &rotation_center)
{
	// rotation that will be applied
	Matrix4x4 rotation = rotateXYZ4(DEG2RAD(rotation_angles.x),
									DEG2RAD(rotation_angles.y),
									DEG2RAD(rotation_angles.z));

	Matrix4x4 object_matrix = rotateXYZ4(DEG2RAD(m_angles.x),DEG2RAD(m_angles.y),DEG2RAD(m_angles.z)) * translate(m_position);

	if(use_center)
	{
		Vector3D diffPos = m_position - rotation_center;

		// rotate diff
		m_position = transpose(!rotation)*diffPos + rotation_center;

		// is at center now
		object_matrix = rotateXYZ4(DEG2RAD(m_angles.x),DEG2RAD(m_angles.y),DEG2RAD(m_angles.z)) * translate(diffPos);

		// rotate this
		Matrix4x4 result = object_matrix*rotation;

		m_angles = EulerMatrixXYZ(result.getRotationComponent());
		m_angles = VRAD2DEG(m_angles);
	}
	else
	{
		Matrix3x3 curr_rotation = object_matrix.getRotationComponent();
		Matrix3x3 rotation3 = rotation.getRotationComponent();

		Matrix3x3 result = rotation3*curr_rotation;

		m_angles = EulerMatrixXYZ(result);
		m_angles = VRAD2DEG(m_angles);
	}

	SetKey("angles", varargs("%g %g %g", m_angles.x,m_angles.y,m_angles.z));
}

edef_entity_t* CEditableEntity::GetDefinition()
{
	return m_pDefinition;
}

DkList<edkeyvalue_t>* CEditableEntity::GetPairs()
{
	return &m_kvPairs;
}

void CEditableEntity::SetKey(const char* pszKey, const char* pszValue, bool updateParam)
{
	for(int i = 0; i < m_kvPairs.numElem(); i++)
	{
		if(!stricmp(m_kvPairs[i].name, pszKey))
		{
			if(strlen(pszValue) == 0)
			{
				RemoveKey(pszKey);
				return;
			}

			// copy new value and get out
			strcpy(m_kvPairs[i].value, pszValue);
			//m_kvPairVars[i].SetValue(pszValue);
			return;
		}
	}

	edkeyvalue_t newPair;
	strcpy(newPair.name, pszKey);
	strcpy(newPair.value, pszValue);

	m_kvPairs.append(newPair);

	evariable_t var;
	var.varType = PARAM_TYPE_STRING;

	if(m_pDefinition)
	{
		edef_param_t* pParam = EDef_FindParameter(newPair.name, m_pDefinition);
		if(pParam)
			var.varType = pParam->type;
	}

	var.SetValue(pszValue);
	//m_kvPairVars.append(var);

	if(updateParam)
		UpdateParameters();
}

char* CEditableEntity::GetKeyValue(const char* pszKey)
{
	for(int i = 0; i < m_kvPairs.numElem(); i++)
	{
		if(!stricmp(m_kvPairs[i].name, pszKey))
			return m_kvPairs[i].value;
	}

	return NULL;
}

Vector3D UTIL_StringToVector3D(const char *str)
{
	Vector3D vec(0,0,0);

	sscanf(str, "%f %f %f", &vec.x, &vec.y, &vec.z);

	return vec;
}

void CEditableEntity::UpdateParameters()
{
	for(int i = 0; i < m_kvPairs.numElem(); i++)
	{
		if(!stricmp(m_kvPairs[i].name, "name") || !stricmp(m_kvPairs[i].name, "targetname"))
		{
			CBaseEditableObject::SetName(m_kvPairs[i].value);
			RemoveKey("name");
			RemoveKey("targetname");
		}
		if(!stricmp(m_kvPairs[i].name, "origin"))
		{
			m_position = UTIL_StringToVector3D(m_kvPairs[i].value);
		}
		if(!stricmp(m_kvPairs[i].name, "color") && m_pDefinition && m_pDefinition->colorfromkey)
		{
			m_groupColor = UTIL_StringToVector3D(m_kvPairs[i].value);
		}
		else if(!stricmp(m_kvPairs[i].name, "angles"))
		{
			m_angles = UTIL_StringToVector3D(m_kvPairs[i].value);
			if(m_angles == vec3_zero)
			{
				RemoveKey("angles");
				i--;
				continue;
			}
		}
		else if(!stricmp(m_kvPairs[i].name, "scale"))
		{
			m_scale = UTIL_StringToVector3D(m_kvPairs[i].value);
			if(m_scale == Vector3D(1))
			{
				RemoveKey("scale");
				i--;
				continue;
			}
		}
		else if(!stricmp(m_kvPairs[i].name, "model"))
		{
			m_pModel = NULL;

			if(m_pDefinition)
				m_pModel = m_pDefinition->model;

			//DestroyAnimationThings();

			// * means that engine/eqwc will use the object
			//if(m_kvPairs[i].value[0] != '*')
			{
				// precache model
				int cache_id = g_studioModelCache->PrecacheModel(m_kvPairs[i].value);
				m_pModel = g_studioModelCache->GetModel(cache_id);

				//InitAnimationThings();

				//SetSequence(0);
				//m_sequenceTimer.active = true;
			}

			/*
			edef_param_t* pParam = EDef_FindParameter("model", m_pDefinition);

			if(pParam && pParam->type == PARAM_TYPE_MODEL)
			{
				// precache model
				int cache_id = g_studioModelCache->PrecacheModel(m_kvPairs[i].value);
				m_pModel = g_studioModelCache->GetModel(cache_id);
			}
			*/
		}

		// check parameter
		IEntityVariable* pVar = GetTypedVariableByKeyName( m_kvPairs[i].name );
		if(!pVar)
		{
			edef_param_t* pParam = EDef_FindParameter(m_kvPairs[i].name, m_pDefinition);
			if(pParam)
			{
				pVar = CreateRegisteredVariable((char*)pParam->typeString.GetData(), this, m_kvPairs[i].value);

				if(pVar)
				{
					pVar->SetKey(m_kvPairs[i].name);
					m_EntTypedVars.append(pVar);
				}
			}
		}

		if(pVar)
			pVar->SetValue(m_kvPairs[i].value);
	}
}

void CEditableEntity::RemoveKey(const char* pszKey)
{
	for(int i = 0; i < m_kvPairs.numElem(); i++)
	{
		if(!stricmp(m_kvPairs[i].name, pszKey))
		{
			IEntityVariable* pVar = GetTypedVariableByKeyName(m_kvPairs[i].name);
			if(pVar)
				m_EntTypedVars.remove(pVar);

			m_kvPairs.removeIndex(i);

			return;
		}
	}
}

IEntityVariable* CEditableEntity::GetTypedVariableByKeyName(char* pszKey)
{
	for(int i = 0; i < m_EntTypedVars.numElem(); i++)
	{
		if(!stricmp(m_EntTypedVars[i]->GetKey(), pszKey))
			return m_EntTypedVars[i];
	}

	return NULL;
}

DkList<OutputData_t>* CEditableEntity::GetOutputs()
{
	return &m_Outputs;
}