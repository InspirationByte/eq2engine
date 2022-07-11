//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: ESM model loader
//////////////////////////////////////////////////////////////////////////////////

#include <ofbx.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/Tokenizer.h"
#include "dsm_loader.h"

namespace SharedModel
{

Vector3D FromFBXRotation(const ofbx::Vec3& vec, const Matrix3x3& orient)
{
	Quaternion o(orient);
	Quaternion q(vec.x, vec.y, vec.z);

	return eulersXYZ(q * o);
}

Vector3D FromFBXVector(const ofbx::Vec3& vec, const Matrix3x3& orient)
{
	Vector3D result(vec.x, vec.y, vec.z);

	return orient * result;
}

Vector2D FromFBXVector(const ofbx::Vec2& vec)
{
	return Vector2D(vec.x, 1.0f - vec.y);
}

Matrix4x4 FromFBXMatrix(const ofbx::Matrix& mat, const Matrix3x3& orient)
{
	Matrix4x4 m(mat.m[0], mat.m[1], mat.m[2], mat.m[3],
				mat.m[4], mat.m[5], mat.m[6], mat.m[7], 
				mat.m[8], mat.m[9], mat.m[10], mat.m[11], 
				mat.m[12], mat.m[13], mat.m[14], mat.m[15]);

	m.rows[0] = Vector4D(orient * m.rows[0].xyz(), 0.0f);
	m.rows[1] = Vector4D(orient * m.rows[1].xyz(), 0.0f);
	m.rows[2] = Vector4D(orient * m.rows[2].xyz(), 0.0f);
	m.rows[3] = Vector4D(orient * m.rows[3].xyz(), 1.0f);

	return m;
}

int DecodeIndex(int idx)
{
	return (idx < 0) ? (-idx - 1) : idx;
}

void ConvertFBXToDSM(dsmmodel_t* model, ofbx::IScene* scene)
{
	// convert FBX scene into DSM groups
	int mesh_count = scene->getMeshCount();

	const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

	bool invertFaces = true;

	const float scaleFactor = settings.UnitScaleFactor;
	// start off with this because Blender and 3DS max has same weird coordinate system
	Matrix3x3 convertMatrix = scale3(-scaleFactor, scaleFactor, -scaleFactor) * rotateX3(DEG2RAD(-90));

	if (settings.FrontAxisSign)
		invertFaces ^= 1;

	if (settings.UpAxisSign)
		invertFaces ^= 1;

	Matrix3x3 axisMatrix(Vector3D(0.0f), Vector3D(0.0f), Vector3D(0.0f));
	axisMatrix.rows[1][settings.UpAxis] = float(settings.UpAxisSign);
	axisMatrix.rows[2][settings.FrontAxis] = float(settings.FrontAxisSign);
	axisMatrix.rows[0] = cross(axisMatrix.rows[2], axisMatrix.rows[1]);
	axisMatrix = transpose(axisMatrix);

	convertMatrix = axisMatrix * convertMatrix;

	Map<int, dsmgroup_t*> materialGroups{ PP_SL };

	for (int i = 0; i < mesh_count; ++i)
	{
		const ofbx::Mesh& mesh = *scene->getMesh(i);
		const ofbx::Geometry& geom = *mesh.getGeometry();

		Msg("Mesh '%s'\n", mesh.name);

		const ofbx::Skin* skin = geom.getSkin();

		struct VertexWeightData
		{
			int boneId;
			Map<int, float> indexWeightMap{ PP_SL };
		};
		Array<VertexWeightData> weightData{ PP_SL };

		if (skin) 
		{
			const int numBones = skin->getClusterCount();
			Msg("\t has %d bones\n", numBones);
			for (int j = 0; j < numBones; ++j) 
			{
				const ofbx::Cluster& fbxCluster = *skin->getCluster(j);

				const ofbx::Object* fbxBoneLink = fbxCluster.getLink();
				const ofbx::Object* fbxParentBoneLink = fbxBoneLink->getParent();

				ofbx::Matrix fbxBoneMat = fbxBoneLink->evalLocal(fbxBoneLink->getLocalTranslation(), fbxBoneLink->getLocalRotation());
				Matrix4x4 boneMatrix = FromFBXMatrix(fbxBoneMat, convertMatrix);

				dsmskelbone_t* pBone = PPNew dsmskelbone_t();
				strcpy(pBone->name, fbxCluster.name);
				
				pBone->position = boneMatrix.getTranslationComponent();
				pBone->angles = eulersXYZ(boneMatrix.getRotationComponent());
				pBone->bone_id = i;

				for (int k = 0; k < numBones; ++k)
				{
					const ofbx::Cluster& pcheckCluster = *skin->getCluster(k);
					const ofbx::Object* pcheckLink = fbxCluster.getLink();
					if (pcheckLink == fbxParentBoneLink)
					{
						strcpy(pBone->parent_name, pcheckCluster.name);
						pBone->parent_id = k;
						break;
					}
				}

				Msg("\t\t%s (parent: %s)\n", pBone->name, pBone->parent_name);

				model->bones.append(pBone);

				VertexWeightData& wd = weightData.append();
				wd.boneId = j;
				const int* indices = fbxCluster.getIndices();
				for (int k = 0; k < fbxCluster.getIndicesCount(); ++k)
				{
					wd.indexWeightMap[indices[k]] = fbxCluster.getWeights()[k];
				}
			}
			
		}

		const int vertex_count = geom.getVertexCount();

		const ofbx::Vec3* vertices = geom.getVertices();
		const ofbx::Vec3* normals = geom.getNormals(); 
		const ofbx::Vec2* uvs = geom.getUVs();

		// const int* faceIndices = geom.getFaceIndices();
		const int* vertMaterials = geom.getMaterials();

		// NOTE: OpenFBX prefers triangulation without indexation
		// and it's not possible to validly obtain the indices with messing up the geometry.
		int triNum = 0;
		for (int j = 0; j < vertex_count; j += 3, triNum++)
		{
			dsmgroup_t* dsmGrp = nullptr;

			const int materialIdx = vertMaterials ? vertMaterials[triNum] : 0;
			const int materialGroupIdx = i | (materialIdx << 16);
			auto found = materialGroups.find(materialGroupIdx);
			if (found == materialGroups.end())
			{
				dsmGrp = PPNew dsmgroup_t();

				const ofbx::Material* material = mesh.getMaterialCount() > 0 ? mesh.getMaterial(materialIdx) : nullptr;
				if (material)
					strncpy(dsmGrp->texture, material->name, sizeof(dsmGrp->texture));

				materialGroups.insert(materialGroupIdx, dsmGrp);
				model->groups.append(dsmGrp);
			}
			else
			{
				dsmGrp = *found;
			}

			for(int k = 0; k < 3; k++)
			{
				const int jj = j + (invertFaces ? 2-k : k);
				dsmvertex_t& vert = dsmGrp->verts.append();
				vert.position = FromFBXVector(vertices[jj], convertMatrix);

				if (normals)
					vert.normal = FromFBXVector(normals[jj], convertMatrix);

				if (uvs)
					vert.texcoord = FromFBXVector(uvs[jj]);

				vert.vertexId = jj;

				// Apply bone weights to vertex
				for (int w = 0; w < weightData.numElem(); ++w)
				{
					const VertexWeightData& wd = weightData[w];
					auto it = wd.indexWeightMap.find(jj);
					if (it == wd.indexWeightMap.end())
						continue;

					dsmweight_t& weight = vert.weights.append();
					weight.bone = wd.boneId;
					weight.weight = *it;
				}
			}
		}
	}
}

bool LoadFBX( dsmmodel_t* model, const char* filename )
{
	ASSERT(model);

	long fileSize = 0;
	char* fileBuffer = g_fileSystem->GetFileBuffer(filename, &fileSize);

	if (!fileBuffer)
	{
		MsgError("Couldn't open FBX file '%s'", filename);
		return false;
	}

	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);

	if (!scene)
	{
		MsgError("FBX '%s' error: ", filename, ofbx::getError());
		PPFree(fileBuffer);
		return false;
	}

	ConvertFBXToDSM(model, scene);

	PPFree(fileBuffer);
	return true;
}

} // namespace