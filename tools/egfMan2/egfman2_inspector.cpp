//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: EGFMan2 - the inspector panel: everything an EGF model holds, and its
//              highlights and vertex data drawn over the model
//////////////////////////////////////////////////////////////////////////////////

#include <imgui.h>

#include "core/core_common.h"
#include "studio/StudioCache.h"
#include "studio/StudioGeom.h"
#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

#include "CAnimatedModel.h"
#include "egfman2_viewer.h"

static constexpr ImGuiWindowFlags s_inspectorFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
	| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
static constexpr ImGuiTableFlags s_inspectorTableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
static constexpr int s_inspectorOverlayBatch = 30000;		// vertices of one dynamic mesh draw (it holds 32767)

static const ImVec4 s_inspectorErrorColor(1.0f, 0.4f, 0.4f, 1.0f);

//-------------------------------------------------------------------------------------------------

// a label and a formatted value row of a two column table
static void InspectorRow(const char* label, const char* fmt, ...)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(label);
	ImGui::TableNextColumn();

	va_list args;
	va_start(args, fmt);
	ImGui::TextV(fmt, args);
	va_end(args);
}

static EqString InspectorVertexStreams(int vertexType)
{
	EqString streams;
	if (vertexType & STUDIO_VERTFLAG_POS_UV)
		streams.Append("pos+uv ");
	if (vertexType & STUDIO_VERTFLAG_TBN)
		streams.Append("tbn ");
	if (vertexType & STUDIO_VERTFLAG_BONEWEIGHT)
		streams.Append("weights ");
	if (vertexType & STUDIO_VERTFLAG_COLOR)
		streams.Append("color ");
	return streams;
}

static int InspectorTriangleCount(const studioMeshDesc_t& mesh)
{
	if (mesh.primitiveType == STUDIO_PRIM_TRI_STRIP)
		return max(static_cast<int>(mesh.numIndices) - 2, 0);
	return mesh.numIndices / 3;
}

// the mesh group a body group draws at lod: a missing LOD falls back to a lower one, as the renderer does
static int InspectorBodyGroupMeshGroup(const studioHdr_t& studio, int bodyGroupIdx, int lod)
{
	const studioLodModel_t* lodModel = studio.pLodModel(studio.pBodyGroups(bodyGroupIdx)->lodModelIndex);
	for (int i = clamp(lod, 0, MAX_MODEL_LODS - 1); i >= 0; --i)
	{
		if (lodModel->meshIndexes[i] != EGF_INVALID_IDX)
			return lodModel->meshIndexes[i];
	}
	return -1;
}

static const char* InspectorBoneName(const studioHdr_t& studio, int boneIdx)
{
	if (boneIdx < 0 || boneIdx >= studio.numBones)
		return "-";
	return studio.pBone(boneIdx)->name;
}

static const char* InspectorPhysObjectName(const StudioPhysData& physData, int objectIdx)
{
	if (!physData.objects.inRange(objectIdx))
		return "-";
	return physData.objects[objectIdx].name.Length() ? physData.objects[objectIdx].name.ToCString() : "(unnamed)";
}

//-------------------------------------------------------------------------------------------------

void CState_EGFViewer::DrawInspector(const ImVec2& pos, const ImVec2& size)
{
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);

	if (ImGui::Begin("##egfman2Inspector", nullptr, s_inspectorFlags))
	{
		if (!g_viewerModel.m_model)
		{
			ImGui::TextDisabled("No model is open");
		}
		else if (ImGui::BeginTabBar("##egfman2Tabs"))
		{
			if (ImGui::BeginTabItem("Model"))
			{
				DrawModelTab();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Skeleton"))
			{
				DrawSkeletonTab();
				ImGui::EndTabItem();
			}

			const ImGuiTabItemFlags motionTabFlags = m_selectMotionTab ? ImGuiTabItemFlags_SetSelected : 0;
			m_selectMotionTab = false;
			if (ImGui::BeginTabItem("Motion", nullptr, motionTabFlags))
			{
				DrawMotionTab();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Physics"))
			{
				DrawPhysicsTab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

//-------------------------------------------------------------------------------------------------
// Model tab

void CState_EGFViewer::DrawModelTab()
{
	const CEqStudioGeom& geom = *g_viewerModel.m_model;
	const studioHdr_t& studio = geom.GetStudioHdr();
	const int drawnLod = g_viewerModel.GetDrawnLod();

	if (ImGui::CollapsingHeader("Summary", ImGuiTreeNodeFlags_DefaultOpen) && ImGui::BeginTable("##summary", 2, s_inspectorTableFlags))
	{
		// what is drawn now: the enabled body groups at the drawn LOD
		int drawnVertices = 0;
		int drawnTriangles = 0;
		for (int i = 0; i < studio.numBodyGroups; ++i)
		{
			const int meshGroupIdx = (g_viewerModel.m_bodyGroupFlags & (1 << i)) ? InspectorBodyGroupMeshGroup(studio, i, drawnLod) : -1;
			if (meshGroupIdx == -1)
				continue;

			const studioMeshGroupDesc_t* meshGroup = studio.pMeshGroupDesc(meshGroupIdx);
			for (int j = 0; j < meshGroup->numMeshes; ++j)
			{
				drawnVertices += meshGroup->pMesh(j)->numVertices;
				drawnTriangles += InspectorTriangleCount(*meshGroup->pMesh(j));
			}
		}

		const BoundingBox& bbox = geom.GetBoundingBox();
		const Vector3D bboxSize = bbox.GetSize();
		InspectorRow("File", "%s", geom.GetName());
		InspectorRow("Name", "%s", studio.modelName);
		InspectorRow("Version", "%d", studio.version);
		InspectorRow("Bounds min", "%.3f %.3f %.3f", bbox.minPoint.x, bbox.minPoint.y, bbox.minPoint.z);
		InspectorRow("Bounds max", "%.3f %.3f %.3f", bbox.maxPoint.x, bbox.maxPoint.y, bbox.maxPoint.z);
		InspectorRow("Size", "%.3f %.3f %.3f", bboxSize.x, bboxSize.y, bboxSize.z);
		InspectorRow("Drawn", "LOD %d: %d vertices, %d triangles", drawnLod, drawnVertices, drawnTriangles);
		InspectorRow("Mesh groups", "%d", studio.numMeshGroups);
		InspectorRow("Body groups", "%d", studio.numBodyGroups);
		InspectorRow("LODs", "%d (%d LOD models)", studio.numLodParams, studio.numLods);
		InspectorRow("Materials", "%d, %d skins", geom.GetMaterials(0).numElem(), geom.GetSkinCount());
		InspectorRow("Bones", "%d", studio.numBones);
		int numAttachments = 0;
		for (int i = 0; i < studio.numTransforms; ++i)
			numAttachments += studio.pTransform(i)->attachBoneIdx != EGF_INVALID_IDX;
		InspectorRow("Transforms", "%d (%d attachments)", studio.numTransforms, numAttachments);
		InspectorRow("IK chains", "%d", studio.numIKChains);
		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Body groups and LODs", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("LOD");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(90.0f);
		if (ImGui::InputInt("##lod", &m_lod))
			m_lod = clamp(m_lod, 0, MAX_MODEL_LODS - 1);
		ImGui::SameLine();
		ImGui::Checkbox("override", &m_lodOverride);
		ImGui::SameLine();
		ImGui::TextDisabled(m_lodOverride ? "(the LOD drawn)" : "(added to the distance LOD)");

		// a column per LOD: the mesh group of each body group, the drawn one highlighted
		const int numLods = clamp(static_cast<int>(studio.numLodParams), 1, MAX_MODEL_LODS);
		if (ImGui::BeginTable("##bodyGroups", numLods + 1, s_inspectorTableFlags | ImGuiTableFlags_ScrollX))
		{
			ImGui::TableSetupColumn("Body group", ImGuiTableColumnFlags_WidthFixed);
			for (int j = 0; j < numLods; ++j)
			{
				EqString header = EqString::Format("LOD %d", j);
				if (j < studio.numLodParams)
				{
					const studioLodParams_t* lodParams = studio.pLodParams(j);
					header.AppendFmt(" %g%s", lodParams->distance, (lodParams->flags & STUDIO_LOD_FLAG_MANUAL) ? " manual" : "");
				}
				ImGui::TableSetupColumn(header, ImGuiTableColumnFlags_WidthFixed);
			}
			ImGui::TableHeadersRow();

			for (int i = 0; i < studio.numBodyGroups; ++i)
			{
				ImGui::PushID(i);
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				bool enabled = (g_viewerModel.m_bodyGroupFlags & (1 << i)) != 0;
				if (ImGui::Checkbox(studio.pBodyGroups(i)->name, &enabled))
				{
					if (enabled)
						g_viewerModel.m_bodyGroupFlags |= (1 << i);
					else
						g_viewerModel.m_bodyGroupFlags &= ~(1 << i);
				}

				const studioLodModel_t* lodModel = studio.pLodModel(studio.pBodyGroups(i)->lodModelIndex);
				for (int j = 0; j < numLods; ++j)
				{
					ImGui::TableNextColumn();
					if (j == drawnLod && enabled)
						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImGuiCol_HeaderActive));

					const int meshGroupIdx = lodModel->meshIndexes[j];
					if (meshGroupIdx == EGF_INVALID_IDX)
					{
						ImGui::TextDisabled("-");
						continue;
					}

					ImGui::PushID(j);
					if (ImGui::Selectable(EqString::Format("group %d", meshGroupIdx), m_selectedMeshGroup == meshGroupIdx && m_selectedMesh == -1))
					{
						m_selectedMeshGroup = meshGroupIdx;
						m_selectedMesh = -1;
					}
					ImGui::PopID();
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		if (geom.GetSkinCount() > 1)
		{
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Skin");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.0f);
			if (ImGui::InputInt("##skin", &g_viewerModel.m_skinIdx))
				g_viewerModel.m_skinIdx = clamp(g_viewerModel.m_skinIdx, 0, geom.GetSkinCount() - 1);
			ImGui::SameLine();
			ImGui::TextDisabled("of %d", geom.GetSkinCount());
		}
	}

	if (ImGui::CollapsingHeader("Mesh groups"))
		DrawMeshGroups();

	if (ImGui::CollapsingHeader("Materials"))
		DrawMaterials();
}

void CState_EGFViewer::DrawMeshGroups()
{
	const CEqStudioGeom& geom = *g_viewerModel.m_model;
	const studioHdr_t& studio = geom.GetStudioHdr();

	ImGui::TextDisabled("A selected group or mesh is outlined in the scene");
	for (int i = 0; i < studio.numMeshGroups; ++i)
	{
		const studioMeshGroupDesc_t* meshGroup = studio.pMeshGroupDesc(i);

		int numVertices = 0;
		int numTriangles = 0;
		for (int j = 0; j < meshGroup->numMeshes; ++j)
		{
			numVertices += meshGroup->pMesh(j)->numVertices;
			numTriangles += InspectorTriangleCount(*meshGroup->pMesh(j));
		}

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (m_selectedMeshGroup == i && m_selectedMesh == -1)
			flags |= ImGuiTreeNodeFlags_Selected;

		const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr>(i)), flags, "Group %d: %d meshes, %d vertices, %d triangles",
			i, meshGroup->numMeshes, numVertices, numTriangles);
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			m_selectedMeshGroup = i;
			m_selectedMesh = -1;
		}

		// a mesh group placed by a transform (not written by the compiler yet)
		if (meshGroup->transformIdx < studio.numTransforms)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("on %s", studio.pTransform(meshGroup->transformIdx)->name);
		}

		if (!open)
			continue;

		for (int j = 0; j < meshGroup->numMeshes; ++j)
		{
			const studioMeshDesc_t* mesh = meshGroup->pMesh(j);
			const IMaterialPtr& material = geom.GetMaterial(mesh->materialIndex, clamp(g_viewerModel.m_skinIdx, 0, max(0, geom.GetSkinCount() - 1)));

			ImGui::PushID(j);
			if (ImGui::Selectable(EqString::Format("%d: %s", j, material ? material->GetName() : "-"), m_selectedMeshGroup == i && m_selectedMesh == j))
			{
				m_selectedMeshGroup = i;
				m_selectedMesh = j;
			}
			ImGui::PopID();

			ImGui::Indent();
			ImGui::TextDisabled("%d vertices, %d triangles (%s), %s", mesh->numVertices, InspectorTriangleCount(*mesh),
				mesh->primitiveType == STUDIO_PRIM_TRI_STRIP ? "strip" : "list", InspectorVertexStreams(mesh->vertexType).ToCString());
			ImGui::Unindent();
		}
		ImGui::TreePop();
	}
}

void CState_EGFViewer::DrawMaterials()
{
	const CEqStudioGeom& geom = *g_viewerModel.m_model;
	const studioHdr_t& studio = geom.GetStudioHdr();
	const int skinIdx = clamp(g_viewerModel.m_skinIdx, 0, max(0, geom.GetSkinCount() - 1));
	ArrayCRef<IMaterialPtr> materials = geom.GetMaterials(skinIdx);

	// the skins of the EGF follow the first one; the ones of a .skin file have no EGF names
	ImGui::TextDisabled("Skin %d", skinIdx);
	if (ImGui::BeginTable("##materials", 3, s_inspectorTableFlags))
	{
		ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("EGF name");
		ImGui::TableSetupColumn("Material");
		ImGui::TableHeadersRow();

		const IMaterialPtr errorMaterial = g_studioCache->GetErrorMaterial();
		for (int i = 0; i < materials.numElem(); ++i)
		{
			const int egfIdx = skinIdx * materials.numElem() + i;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%d", i);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(egfIdx < studio.numMaterials ? studio.pMaterial(egfIdx)->materialname : "-");
			ImGui::TableNextColumn();
			if (!materials[i] || materials[i] == errorMaterial)
				ImGui::TextColored(s_inspectorErrorColor, "not found");
			else
				ImGui::TextUnformatted(materials[i]->GetName());
		}
		ImGui::EndTable();
	}

	ImGui::TextUnformatted("Search paths");
	ImGui::Indent();
	for (int i = 0; i < studio.numMaterialSearchPaths; ++i)
		ImGui::Text("'%s'", studio.pMaterialSearchPath(i)->searchPath);
	ImGui::Unindent();
}

//-------------------------------------------------------------------------------------------------
// Skeleton tab

void CState_EGFViewer::DrawSkeletonTab()
{
	const CEqStudioGeom& geom = *g_viewerModel.m_model;
	const studioHdr_t& studio = geom.GetStudioHdr();
	ArrayCRef<StudioJoint> joints = geom.GetJoints();

	if (ImGui::CollapsingHeader("Bones", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (!studio.numBones)
			ImGui::TextDisabled("No bones");

		for (int i = 0; i < joints.numElem(); ++i)
		{
			if (joints[i].parent == -1)
				DrawBoneTree(i);
		}

		if (g_viewerModel.m_selectedBone >= 0 && g_viewerModel.m_selectedBone < studio.numBones && ImGui::BeginTable("##bone", 2, s_inspectorTableFlags))
		{
			const studioBoneDesc_t* bone = studio.pBone(g_viewerModel.m_selectedBone);
			const Vector3D rotation = bone->rotation * M_RAD2DEG;
			InspectorRow("Bone", "%d: %s", g_viewerModel.m_selectedBone, bone->name);
			InspectorRow("Parent", "%s", InspectorBoneName(studio, bone->parent));
			InspectorRow("Position", "%.3f %.3f %.3f", bone->position.x, bone->position.y, bone->position.z);
			InspectorRow("Rotation", "%.1f %.1f %.1f", rotation.x, rotation.y, rotation.z);
			if (joints.inRange(g_viewerModel.m_selectedBone) && joints[g_viewerModel.m_selectedBone].ikChainId != -1)
				InspectorRow("IK chain", "%s, link %d", studio.pIkChain(joints[g_viewerModel.m_selectedBone].ikChainId)->name, joints[g_viewerModel.m_selectedBone].ikLinkId);
			ImGui::EndTable();
			ImGui::TextDisabled("View > Weights of the selected bone shows its vertices");
		}
	}

	// the attachments are the transforms on a bone, the others are in the model space
	if (ImGui::CollapsingHeader("Transforms", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (!studio.numTransforms)
			ImGui::TextDisabled("No transforms");

		for (int i = 0; i < studio.numTransforms; ++i)
		{
			const studioTransform_t* transform = studio.pTransform(i);
			const Vector3D position = transform->transform.rows[3].xyz();
			const bool onBone = transform->attachBoneIdx != EGF_INVALID_IDX;

			ImGui::PushID(i);
			if (ImGui::Selectable(EqString::Format("%s (%s%s)", transform->name, onBone ? "attachment, bone " : "model", onBone ? InspectorBoneName(studio, transform->attachBoneIdx) : ""), g_viewerModel.m_selectedAttachment == i))
				g_viewerModel.m_selectedAttachment = (g_viewerModel.m_selectedAttachment == i) ? -1 : i;
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::TextDisabled("%.3f %.3f %.3f", position.x, position.y, position.z);
		}
	}

	if (ImGui::CollapsingHeader("IK chains"))
	{
		if (!studio.numIKChains)
			ImGui::TextDisabled("No IK chains");

		for (int i = 0; i < studio.numIKChains; ++i)
		{
			const studioIkChain_t* chain = studio.pIkChain(i);
			if (!ImGui::TreeNodeEx(chain->name, ImGuiTreeNodeFlags_SpanAvailWidth, "%s: %d links", chain->name, chain->numLinks))
				continue;

			if (ImGui::BeginTable("##ikLinks", 4, s_inspectorTableFlags))
			{
				ImGui::TableSetupColumn("Bone");
				ImGui::TableSetupColumn("Min");
				ImGui::TableSetupColumn("Max");
				ImGui::TableSetupColumn("Damping");
				ImGui::TableHeadersRow();
				for (int j = 0; j < chain->numLinks; ++j)
				{
					const studioIkLink_t* link = chain->pLink(j);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (ImGui::Selectable(InspectorBoneName(studio, link->bone), g_viewerModel.m_selectedBone == link->bone, ImGuiSelectableFlags_SpanAllColumns))
						g_viewerModel.m_selectedBone = link->bone;
					ImGui::TableNextColumn();
					ImGui::Text("%g %g %g", link->mins.x, link->mins.y, link->mins.z);
					ImGui::TableNextColumn();
					ImGui::Text("%g %g %g", link->maxs.x, link->maxs.y, link->maxs.z);
					ImGui::TableNextColumn();
					ImGui::Text("%g", link->damping);
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
	}
}

void CState_EGFViewer::DrawBoneTree(int boneIdx)
{
	const CEqStudioGeom& geom = *g_viewerModel.m_model;
	const StudioJoint& joint = geom.GetJoints()[boneIdx];

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
	if (joint.childs.isEmpty())
		flags |= ImGuiTreeNodeFlags_Leaf;
	if (g_viewerModel.m_selectedBone == boneIdx)
		flags |= ImGuiTreeNodeFlags_Selected;

	const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr>(boneIdx)), flags, "%s", joint.bone->name);
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		g_viewerModel.m_selectedBone = (g_viewerModel.m_selectedBone == boneIdx) ? -1 : boneIdx;

	if (!open)
		return;

	for (const int childIdx : joint.childs)
		DrawBoneTree(childIdx);
	ImGui::TreePop();
}

//-------------------------------------------------------------------------------------------------
// Motion tab

void CState_EGFViewer::DrawMotionTab()
{
	if (ImGui::BeginTable("##motion", 2, ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("##value");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Sequence");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		const char* motionPreview = m_motionSelection != -1 ? m_motionItems[m_motionSelection].name.ToCString() : "select sequence...";
		if (ImGui::BeginCombo("##motion", motionPreview))
		{
			for (int i = 0; i < m_motionItems.numElem(); ++i)
			{
				ImGui::PushID(i);
				if (ImGui::Selectable(m_motionItems[i].name.ToCString(), i == m_motionSelection))
				{
					m_motionSelection = i;
					OnSequenceSelected();
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Frame");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderInt("##timeline", &m_timelineFrame, 0, max(m_timelineMax, 0)))
			g_viewerModel.SetSequenceTime(m_timelineFrame, 0);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("FPS");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(60.0f);
		ImGui::InputText("##fps", m_animFramerate, sizeof(m_animFramerate), ImGuiInputTextFlags_CharsDecimal);
		ImGui::SameLine();
		if (ImGui::Button("Play"))
		{
			g_viewerModel.ResetSequenceTime(0);
			g_viewerModel.PlaySequence(0);
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
			g_viewerModel.StopSequence(0);

		ArrayCRef<AnimPoseController> poseControllers = g_viewerModel.GetPoseControllers();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Pose controller");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		const char* posePreview = poseControllers.inRange(m_poseControllerSelection) ? poseControllers[m_poseControllerSelection].desc->name : "select controller...";
		if (ImGui::BeginCombo("##poseController", posePreview))
		{
			for (int i = 0; i < poseControllers.numElem(); ++i)
			{
				ImGui::PushID(i);
				if (ImGui::Selectable(poseControllers[i].desc->name, i == m_poseControllerSelection))
				{
					m_poseControllerSelection = i;
					OnPoseControllerSelected();
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Value");
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##poseValue", &m_poseValue, m_poseValueMin, m_poseValueMax) && m_poseControllerSelection != -1)
			g_viewerModel.SetPoseControllerValue(m_poseControllerSelection, m_poseValue);

		ImGui::EndTable();
	}

	if (m_motionSelection != -1 && ImGui::CollapsingHeader("Sequence", ImGuiTreeNodeFlags_DefaultOpen))
		DrawSequenceInfo();

	if (ImGui::CollapsingHeader("Motion packages"))
	{
		const studioHdr_t& studio = g_viewerModel.m_model->GetStudioHdr();
		for (int i = 0; i < studio.numMotionPackages; ++i)
			ImGui::Text("%s (the model)", studio.pPackage(i)->packageName);

		for (const AnimDataProvider& animData : g_viewerModel.GetAnimData())
			ImGui::Text("%s: %d sequences, %d animations", animData.motionData->name.ToCString(), animData.sequences.numElem(), animData.animations.numElem());
	}
}

void CState_EGFViewer::DrawSequenceInfo()
{
	const AnimSequence* seq = g_viewerModel.m_sequencesList[m_motionSelection];
	const sequencedesc_t* desc = seq->desc;
	ArrayCRef<AnimPoseController> poseControllers = g_viewerModel.GetPoseControllers();

	if (ImGui::BeginTable("##sequence", 2, s_inspectorTableFlags))
	{
		EqString flags;
		if (desc->flags & SEQFLAG_LOOP)
			flags.Append("loop ");
		if (desc->flags & SEQFLAG_SLOTBLEND)
			flags.Append("slot blend ");

		InspectorRow("Name", "%s", desc->name);
		InspectorRow("Activity", "%s, slot %d", desc->activity[0] ? desc->activity : "-", desc->activityslot);
		InspectorRow("Flags", "%s", flags.Length() ? flags.ToCString() : "-");
		InspectorRow("Frame rate", "%g", desc->framerate);
		InspectorRow("Transition", "%g s", desc->transitiontime);
		InspectorRow("Time controller", "%s", poseControllers.inRange(desc->timecontroller) ? poseControllers[desc->timecontroller].desc->name : "-");
		InspectorRow("Pose controller", "%s", poseControllers.inRange(desc->posecontroller) ? poseControllers[desc->posecontroller].desc->name : "-");
		ImGui::EndTable();
	}

	ImGui::TextUnformatted("Animations (weights)");
	ImGui::Indent();
	for (int i = 0; i < desc->numAnimations; ++i)
	{
		if (seq->animations[i])
			ImGui::Text("%d: %s, %d frames", i, seq->animations[i]->desc->name, seq->animations[i]->numFrames);
	}
	ImGui::Unindent();

	if (desc->numSequenceBlends)
	{
		ImGui::TextUnformatted("Layers");
		ImGui::Indent();
		for (int i = 0; i < desc->numSequenceBlends; ++i)
		{
			if (seq->blends[i])
				ImGui::Text("%d: %s", i, seq->blends[i]->desc->name);
		}
		ImGui::Unindent();
	}

	if (desc->numEvents && ImGui::BeginTable("##events", 3, s_inspectorTableFlags))
	{
		ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Command");
		ImGui::TableSetupColumn("Parameter");
		ImGui::TableHeadersRow();
		for (int i = 0; i < desc->numEvents; ++i)
		{
			const sequenceevent_t* event = seq->events[i];
			if (!event)
				continue;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (ImGui::Selectable(EqString::Format("%g##event%d", event->frame, i), static_cast<int>(event->frame) == m_timelineFrame, ImGuiSelectableFlags_SpanAllColumns))
			{
				m_timelineFrame = static_cast<int>(event->frame);
				g_viewerModel.SetSequenceTime(event->frame, 0);
			}
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(event->command);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(event->parameter);
		}
		ImGui::EndTable();
	}
}

//-------------------------------------------------------------------------------------------------
// Physics tab

void CState_EGFViewer::DrawPhysicsTab()
{
	const StudioPhysData& physData = g_viewerModel.m_model->GetPhysData();

	static const char* s_usageNames[] = { "none", "rigid (compound)", "ragdoll", "dynamic" };
	static const char* s_shapeTypeNames[] = { "concave", "movable concave", "convex" };

	if (physData.objects.isEmpty())
	{
		ImGui::TextDisabled("No physics objects");
		return;
	}

	ImGui::Text("Usage: %s", physData.usageType >= 0 && physData.usageType < elementsOf(s_usageNames) ? s_usageNames[physData.usageType] : "unknown");
	ImGui::Text("%d objects, %d shapes, %d joints", physData.objects.numElem(), physData.shapes.numElem(), physData.joints.numElem());

	ImGui::TextUnformatted("Simulation");
	ImGui::SameLine();
	if (ImGui::Button(g_viewerModel.IsPhysicsEnabled() ? "Pause" : "Run"))
		g_viewerModel.TogglePhysicsState();
	ImGui::SameLine();
	if (ImGui::Button("Reset"))
		g_viewerModel.ResetPhysics();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::SliderFloat("##timescale", &m_simTimescale, 0.0f, 2.0f, "timescale %.2f");

	if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen) && ImGui::BeginTable("##physObjects", 4, s_inspectorTableFlags))
	{
		ImGui::TableSetupColumn("Object");
		ImGui::TableSetupColumn("Mass", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Surface");
		ImGui::TableSetupColumn("Shapes");
		ImGui::TableHeadersRow();

		for (int i = 0; i < physData.objects.numElem(); ++i)
		{
			const physobject_t& desc = physData.objects[i].desc;

			EqString shapes = EqString::Format("%d", desc.numShapes);
			for (int j = 0; j < desc.numShapes; ++j)
			{
				const int shapeType = physData.shapes[desc.shapeIndex[j]].desc.type;
				shapes.AppendFmt("%s%s", j ? ", " : ": ", shapeType >= 0 && shapeType < elementsOf(s_shapeTypeNames) ? s_shapeTypeNames[shapeType] : "?");
			}

			ImGui::PushID(i);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (ImGui::Selectable(InspectorPhysObjectName(physData, i), g_viewerModel.m_selectedPhysObject == i, ImGuiSelectableFlags_SpanAllColumns))
				g_viewerModel.m_selectedPhysObject = (g_viewerModel.m_selectedPhysObject == i) ? -1 : i;
			ImGui::TableNextColumn();
			ImGui::Text("%g", desc.mass);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(desc.surfaceprops);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(shapes);
			ImGui::PopID();
		}
		ImGui::EndTable();

		if (physData.objects.inRange(g_viewerModel.m_selectedPhysObject) && ImGui::BeginTable("##physObject", 2, s_inspectorTableFlags))
		{
			const physobject_t& desc = physData.objects[g_viewerModel.m_selectedPhysObject].desc;
			InspectorRow("Offset", "%.3f %.3f %.3f", desc.offset.x, desc.offset.y, desc.offset.z);
			InspectorRow("Mass center", "%.3f %.3f %.3f", desc.massCenter.x, desc.massCenter.y, desc.massCenter.z);
			InspectorRow("Body part", "%d", desc.bodyPartId);
			ImGui::EndTable();
			ImGui::TextDisabled("View > Show physics objects draws the others dimmed");
		}
	}

	if (physData.joints.numElem() && ImGui::CollapsingHeader("Joints", ImGuiTreeNodeFlags_DefaultOpen) && ImGui::BeginTable("##physJoints", 4, s_inspectorTableFlags))
	{
		ImGui::TableSetupColumn("Joint");
		ImGui::TableSetupColumn("Objects");
		ImGui::TableSetupColumn("Min");
		ImGui::TableSetupColumn("Max");
		ImGui::TableHeadersRow();

		for (const physjoint_t& joint : physData.joints)
		{
			const Vector3D minLimit = joint.minLimit * M_RAD2DEG;
			const Vector3D maxLimit = joint.maxLimit * M_RAD2DEG;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(joint.name);
			ImGui::TableNextColumn();
			ImGui::Text("%s - %s", InspectorPhysObjectName(physData, joint.objA), InspectorPhysObjectName(physData, joint.objB));
			ImGui::TableNextColumn();
			ImGui::Text("%.0f %.0f %.0f", minLimit.x, minLimit.y, minLimit.z);
			ImGui::TableNextColumn();
			ImGui::Text("%.0f %.0f %.0f", maxLimit.x, maxLimit.y, maxLimit.z);
		}
		ImGui::EndTable();
	}
}

//-------------------------------------------------------------------------------------------------
// Overlays

// dynamic mesh draws of one primitive type, split before the dynamic mesh is full
class CInspectorOverlay
{
public:
	CInspectorOverlay(IGPURenderPassRecorder* recorder, EPrimTopology topology, int primitiveVertices, bool depthTest)
		: m_meshBuilder(g_matSystem->GetDynamicMesh())
		, m_topology(topology)
		, m_primitiveVertices(primitiveVertices)
		, m_recorder(recorder)
	{
		m_drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());
		m_renderPass.blendMode = depthTest ? SHADER_BLEND_NONE : SHADER_BLEND_TRANSLUCENT;
		m_renderPass.depthTest = depthTest;
		m_renderPass.depthWrite = depthTest;
		m_meshBuilder.Begin(m_topology);
	}

	~CInspectorOverlay()
	{
		Flush(false);
	}

	void Vertex(const Vector3D& position, const ColorRGBA& color)
	{
		m_meshBuilder.Color4fv(color);
		m_meshBuilder.Position3fv(position);
		m_meshBuilder.AdvanceVertex();

		if (++m_vertexCount >= s_inspectorOverlayBatch && m_vertexCount % m_primitiveVertices == 0)
			Flush(true);
	}

private:
	void Flush(bool restart)
	{
		if (m_vertexCount && m_meshBuilder.End(m_drawCmd))
			g_matSystem->SetupDrawCommand(m_drawCmd, RenderPassContext(m_recorder, &m_renderPass));
		else
			m_meshBuilder.End();

		m_vertexCount = 0;
		if (restart)
			m_meshBuilder.Begin(m_topology);
	}

	CMeshBuilder			m_meshBuilder;
	RenderDrawCmd			m_drawCmd;
	MatSysDefaultRenderPass	m_renderPass;
	EPrimTopology			m_topology;
	int						m_primitiveVertices;
	int						m_vertexCount{ 0 };
	IGPURenderPassRecorder*	m_recorder;
};

// a studio mesh moved by the skinning matrices of the current pose, as the renderer draws it
struct InspectorSkinnedVertex
{
	Vector3D	position;
	Vector3D	normal;
	Vector3D	tangent;
	Vector3D	binormal;
};

static InspectorSkinnedVertex InspectorSkinVertex(const studioMeshDesc_t& mesh, int vertexIdx, ArrayCRef<Matrix4x4> skinning)
{
	InspectorSkinnedVertex result;
	const Vector3D position = mesh.pPosUvs(vertexIdx)->point;
	const bool hasTBN = mesh.vertexType & STUDIO_VERTFLAG_TBN;
	const studioVertexTBN_t* tbn = hasTBN ? mesh.pTBNs(vertexIdx) : nullptr;

	const studioBoneWeight_t* weights = (mesh.vertexType & STUDIO_VERTFLAG_BONEWEIGHT) ? mesh.pBoneWeight(vertexIdx) : nullptr;
	if (!weights || !weights->numweights || skinning.isEmpty())
	{
		result.position = position;
		result.normal = tbn ? tbn->normal : vec3_zero;
		result.tangent = tbn ? tbn->tangent : vec3_zero;
		result.binormal = tbn ? tbn->binormal : vec3_zero;
		return result;
	}

	result.position = result.normal = result.tangent = result.binormal = vec3_zero;
	for (int i = 0; i < weights->numweights; ++i)
	{
		if (!skinning.inRange(weights->bones[i]))
			continue;

		const Matrix4x4& matrix = skinning[weights->bones[i]];
		const float weight = weights->weight[i];
		result.position += transformPoint(position, matrix) * weight;
		if (tbn)
		{
			result.normal += rotateVector(tbn->normal, matrix) * weight;
			result.tangent += rotateVector(tbn->tangent, matrix) * weight;
			result.binormal += rotateVector(tbn->binormal, matrix) * weight;
		}
	}
	return result;
}

// the triangles of a studio mesh as vertex index triples, the strips flipped every second one
template<typename F>
static void InspectorForEachTriangle(const studioMeshDesc_t& mesh, F func)
{
	const uint32* indices = mesh.pVertexIdx(0);
	const bool strip = mesh.primitiveType == STUDIO_PRIM_TRI_STRIP;
	const int numIndices = static_cast<int>(mesh.numIndices);
	for (int k = 0; k + 2 < numIndices; k += strip ? 1 : 3)
	{
		const uint32 a = indices[k], b = indices[k + 1], c = indices[k + 2];
		if (a == b || a == c || b == c)
			continue;

		const bool flip = strip && (k % 2);
		func(flip ? c : a, b, flip ? a : c);
	}
}

static float InspectorBoneWeight(const studioMeshDesc_t& mesh, int vertexIdx, int boneIdx)
{
	if (!(mesh.vertexType & STUDIO_VERTFLAG_BONEWEIGHT))
		return 0.0f;

	const studioBoneWeight_t* weights = mesh.pBoneWeight(vertexIdx);
	float weight = 0.0f;
	for (int i = 0; i < weights->numweights; ++i)
	{
		if (weights->bones[i] == boneIdx)
			weight += weights->weight[i];
	}
	return weight;
}

static ColorRGBA InspectorVertexColor(const studioMeshDesc_t& mesh, int vertexIdx, int display, int boneIdx)
{
	if (display == VERTEX_DISPLAY_WEIGHTS)
	{
		const float weight = InspectorBoneWeight(mesh, vertexIdx, boneIdx);
		return weight > 0.0f ? ColorRGBA(weight, 0.2f, 1.0f - weight, 1.0f) : ColorRGBA(0.15f, 0.15f, 0.15f, 1.0f);
	}

	if (!(mesh.vertexType & STUDIO_VERTFLAG_COLOR))
		return ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	return MColor(mesh.pColor(vertexIdx)->color).v;
}

void CState_EGFViewer::RenderOverlays(IGPURenderPassRecorder* rendPassRecorder)
{
	if (!g_viewerModel.m_model)
		return;

	const CEqStudioGeom& geom = *g_viewerModel.m_model;
	const studioHdr_t& studio = geom.GetStudioHdr();
	ArrayCRef<StudioJoint> joints = geom.GetJoints();
	const Matrix4x4* boneMatrices = g_viewerModel.GetBoneMatrices();

	// the matrices of ConvertBoneMatricesToQuaternions: the bind pose to the current one
	Array<Matrix4x4> skinning(PP_SL);
	if (boneMatrices)
	{
		skinning.setNum(joints.numElem());
		for (int i = 0; i < joints.numElem(); ++i)
			skinning[i] = joints[i].invAbsTrans * boneMatrices[i];
	}

	g_matSystem->SetMatrix(MATRIXMODE_WORLD, g_viewerModel.GetWorldMatrix());

	const float vectorLength = length(geom.GetBoundingBox().GetSize()) * 0.01f;
	const float colorOffset = vectorLength * 0.05f;		// the colored triangles are moved out of the model surface

	// vertex data of the drawn meshes: the enabled body groups at the drawn LOD
	if (m_vertexDisplay != VERTEX_DISPLAY_NONE)
	{
		const bool lines = m_vertexDisplay == VERTEX_DISPLAY_NORMALS || m_vertexDisplay == VERTEX_DISPLAY_TANGENTS;
		CInspectorOverlay overlay(rendPassRecorder, lines ? PRIM_LINES : PRIM_TRIANGLES, lines ? 2 : 3, !lines);

		for (int i = 0; i < studio.numBodyGroups; ++i)
		{
			const int meshGroupIdx = (g_viewerModel.m_bodyGroupFlags & (1 << i)) ? InspectorBodyGroupMeshGroup(studio, i, g_viewerModel.GetDrawnLod()) : -1;
			if (meshGroupIdx == -1)
				continue;

			const studioMeshGroupDesc_t* meshGroup = studio.pMeshGroupDesc(meshGroupIdx);
			for (int j = 0; j < meshGroup->numMeshes; ++j)
			{
				const studioMeshDesc_t& mesh = *meshGroup->pMesh(j);
				if (lines)
				{
					for (int k = 0; k < mesh.numVertices; ++k)
					{
						const InspectorSkinnedVertex vertex = InspectorSkinVertex(mesh, k, skinning);
						if (m_vertexDisplay == VERTEX_DISPLAY_NORMALS)
						{
							overlay.Vertex(vertex.position, ColorRGBA(0.2f, 1.0f, 0.2f, 1.0f));
							overlay.Vertex(vertex.position + vertex.normal * vectorLength, ColorRGBA(0.2f, 1.0f, 0.2f, 1.0f));
							continue;
						}
						overlay.Vertex(vertex.position, ColorRGBA(1.0f, 0.2f, 0.2f, 1.0f));
						overlay.Vertex(vertex.position + vertex.tangent * vectorLength, ColorRGBA(1.0f, 0.2f, 0.2f, 1.0f));
						overlay.Vertex(vertex.position, ColorRGBA(0.2f, 0.4f, 1.0f, 1.0f));
						overlay.Vertex(vertex.position + vertex.binormal * vectorLength, ColorRGBA(0.2f, 0.4f, 1.0f, 1.0f));
					}
					continue;
				}

				InspectorForEachTriangle(mesh, [&](uint32 a, uint32 b, uint32 c) {
					for (const uint32 vertexIdx : { a, b, c })
					{
						const InspectorSkinnedVertex vertex = InspectorSkinVertex(mesh, vertexIdx, skinning);
						overlay.Vertex(vertex.position + vertex.normal * colorOffset, InspectorVertexColor(mesh, vertexIdx, m_vertexDisplay, g_viewerModel.m_selectedBone));
					}
				});
			}
		}
	}

	// the selected mesh group or mesh outlined, whatever the LOD and the body groups
	if (m_selectedMeshGroup >= 0 && m_selectedMeshGroup < studio.numMeshGroups)
	{
		CInspectorOverlay overlay(rendPassRecorder, PRIM_LINES, 2, false);
		const ColorRGBA outlineColor(1.0f, 0.9f, 0.1f, 0.6f);

		const studioMeshGroupDesc_t* meshGroup = studio.pMeshGroupDesc(m_selectedMeshGroup);
		for (int j = 0; j < meshGroup->numMeshes; ++j)
		{
			if (m_selectedMesh != -1 && m_selectedMesh != j)
				continue;

			const studioMeshDesc_t& mesh = *meshGroup->pMesh(j);
			InspectorForEachTriangle(mesh, [&](uint32 a, uint32 b, uint32 c) {
				const Vector3D pa = InspectorSkinVertex(mesh, a, skinning).position;
				const Vector3D pb = InspectorSkinVertex(mesh, b, skinning).position;
				const Vector3D pc = InspectorSkinVertex(mesh, c, skinning).position;
				overlay.Vertex(pa, outlineColor);
				overlay.Vertex(pb, outlineColor);
				overlay.Vertex(pb, outlineColor);
				overlay.Vertex(pc, outlineColor);
				overlay.Vertex(pc, outlineColor);
				overlay.Vertex(pa, outlineColor);
			});
		}
	}
}
