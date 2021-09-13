//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: ESM model loader
//////////////////////////////////////////////////////////////////////////////////

#include "dsm_esm_loader.h"
#include "core/DebugInterface.h"
#include "core/IFileSystem.h"

#include "math/Matrix.h"
#include "math/DkMath.h"

#include <ofbx.h>

namespace SharedModel
{

Vector3D FromFBXVector(const ofbx::Vec3& vec, Matrix3x3& orient)
{
	Vector3D result(vec.x, vec.y, vec.z);

	return orient * result;
}

Vector2D FromFBXVector(const ofbx::Vec2& vec)
{
	return Vector2D(vec.x, vec.y);
}

void ConvertFBXToDSM(dsmmodel_t* model, ofbx::IScene* scene)
{
	// convert FBX scene into DSM groups
	int mesh_count = scene->getMeshCount();

	const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

	
	const float scaleFactor = settings.UnitScaleFactor;
	// start off with this because Blender and 3DS max has same weird coordinate system
	Matrix3x3 convertMatrix = scale3(scaleFactor, scaleFactor, -scaleFactor) * rotateX3(DEG2RAD(-90));

	Matrix3x3 axisMatrix(Vector3D(0.0f), Vector3D(0.0f), Vector3D(0.0f));
	axisMatrix.rows[1][settings.UpAxis] = float(settings.UpAxisSign);
	axisMatrix.rows[2][settings.FrontAxis] = float(settings.FrontAxisSign);
	axisMatrix.rows[0] = cross(axisMatrix.rows[2], axisMatrix.rows[1]);
	axisMatrix = transpose(axisMatrix);

	convertMatrix = axisMatrix * convertMatrix;

	for (int i = 0; i < mesh_count; ++i)
	{
		const ofbx::Mesh& mesh = *scene->getMesh(i);
		const ofbx::Geometry& geom = *mesh.getGeometry();

		const int vertex_count = geom.getVertexCount();
		const int count = geom.getIndexCount();
		const int index_count = geom.getIndexCount();

		const ofbx::Vec3* vertices = geom.getVertices();
		const ofbx::Vec3* normals = geom.getNormals(); 
		const ofbx::Vec2* uvs = geom.getUVs();
		const int* faceIndices = geom.getFaceIndices();

		const ofbx::Material* material = mesh.getMaterialCount() > 0 ? mesh.getMaterial(0) : nullptr;

		dsmgroup_t* newGrp = new dsmgroup_t();
		model->groups.append(newGrp);

		if (material)
			strncpy(newGrp->texture, material->name, sizeof(newGrp->texture));

		// convert vertices
		for (int i = 0; i < vertex_count; ++i)
		{
			dsmvertex_t vert;
			vert.position = FromFBXVector(vertices[i], convertMatrix);

			if(normals)
				vert.normal = FromFBXVector(normals[i], convertMatrix);

			if(uvs)
				vert.texcoord = FromFBXVector(uvs[i]);

			vert.vertexId = -1;

			newGrp->verts.append(vert);
		}

		for (int i = 0; i < index_count; ++i)
		{
			newGrp->indices.append(faceIndices[i]);
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