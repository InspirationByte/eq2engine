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

Vector3D FromFBXVector(const ofbx::Vec3& vec, Matrix3x3& orient)
{
	Vector3D result(vec.x, vec.y, vec.z);

	return orient * result;
}

Vector2D FromFBXVector(const ofbx::Vec2& vec)
{
	return Vector2D(vec.x, 1.0f - vec.y);
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

		const int vertex_count = geom.getVertexCount();
		//const int count = geom.getIndexCount();
		//const int index_count = geom.getIndexCount();

		const ofbx::Vec3* vertices = geom.getVertices();
		const ofbx::Vec3* normals = geom.getNormals(); 
		const ofbx::Vec2* uvs = geom.getUVs();
		// const int* faceIndices = geom.getFaceIndices();
		const int* vertMaterials = geom.getMaterials();
#if 1
		// triangulated
		int triNum = 0;
		for (int j = 0; j < vertex_count; j += 3, triNum++)
		{
			dsmgroup_t* dsmGrp = nullptr;

			const int materialIdx = vertMaterials[triNum];
			auto found = materialGroups.find(materialIdx);
			if (found == materialGroups.end())
			{
				dsmGrp = PPNew dsmgroup_t();

				const ofbx::Material* material = mesh.getMaterialCount() > 0 ? mesh.getMaterial(materialIdx) : nullptr;
				if (material)
					strncpy(dsmGrp->texture, material->name, sizeof(dsmGrp->texture));

				materialGroups.insert(materialIdx, dsmGrp);
				model->groups.append(dsmGrp);
			}
			else
			{
				dsmGrp = *found;
			}

			for(int k = 0; k < 3; k++)
			{
				int jj = j + (invertFaces ? 2-k : k);
				dsmvertex_t vert;
				vert.position = FromFBXVector(vertices[jj], convertMatrix);

				if (normals)
					vert.normal = FromFBXVector(normals[jj], convertMatrix);

				if (uvs)
					vert.texcoord = FromFBXVector(uvs[jj]);

				vert.vertexId = -1;

				dsmGrp->verts.append(vert);
			}
		}
#else
		// TODO: NON-triangulated version

		// convert vertices
		for (int j = 0; j < vertex_count; j++)
		{
			dsmvertex_t vert;
			vert.position = FromFBXVector(vertices[j], convertMatrix);

			if(normals)
				vert.normal = FromFBXVector(normals[j], convertMatrix);

			if(uvs)
				vert.texcoord = FromFBXVector(uvs[j]);

			vert.vertexId = -1;

			dsmGrp->verts.append(vert);
		}


		int numPolyIndices = 0;
		int polyIndices[24] = { -1 };

		for (int j = 0; j < index_count; j++)
		{
			int index = faceIndices[j];
			polyIndices[numPolyIndices++] = DecodeIndex(index);

			Msg("index %d\n", index);

			if (index < 0) // negative marks end - start over
			{
				// triangulate strip
				for (int k = 0; k < numPolyIndices-2; k++)
				{
					dsmGrp->indices.append(polyIndices[k]);
					dsmGrp->indices.append(polyIndices[k+1]);
					dsmGrp->indices.append(polyIndices[k+2]);
				}

				numPolyIndices = 0;
			}
		}
#endif
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