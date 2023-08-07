//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: ESM model loader
//////////////////////////////////////////////////////////////////////////////////

#include <ofbx.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/Tokenizer.h"
#include "math/Utility.h"
#include "dsm_loader.h"
#include "dsm_esm_loader.h"
#include "dsm_fbx_loader.h"
#include "studiofile/StudioLoader.h"

static constexpr const int MaxFramesPerAnimation = 10000;

namespace SharedModel
{

Vector3D FromFBXRotation(const ofbx::Vec3& vec, const Matrix3x3& orient)
{
	Quaternion o(orient);
	Quaternion q(vec.x, vec.y, vec.z);

	return eulersXYZ(o * q);
}

Vector3D FromFBXRotation(const Vector3D& vec, const Matrix3x3& orient)
{
	Quaternion o(orient);
	Quaternion q(vec.x, vec.y, vec.z);

	return eulersXYZ(o * q);
}

template<typename T>
Vector3D FixOrient(const T& v, const ofbx::GlobalSettings& settings)
{
	switch (settings.UpAxis) {
		case ofbx::UpVector_AxisY: return Vector3D(v.x, v.y, v.z);
		case ofbx::UpVector_AxisZ: return Vector3D(v.x, v.z, -v.y);
		case ofbx::UpVector_AxisX: return Vector3D(-v.y, v.x, v.z);
	}
	return Vector3D(v.x, v.y, v.z);
}

Vector3D FromFBXVector(const ofbx::Vec3& v, const ofbx::GlobalSettings& settings)
{
	return FixOrient(v, settings);
}

Vector3D FromFBXVector(const Vector3D& v, const ofbx::GlobalSettings& settings)
{
	return FixOrient(v, settings);
}

Quaternion FixOrient(const Quaternion& v, const ofbx::GlobalSettings& settings)
{
	switch (settings.UpAxis)
	{
		case ofbx::UpVector_AxisY: return Quaternion(v.x, v.y, v.z, v.w);
		case ofbx::UpVector_AxisZ: return Quaternion(v.x, v.z, -v.y, v.w);
		case ofbx::UpVector_AxisX: return Quaternion(-v.y, v.x, v.z, v.w);
	}
	return Quaternion(v.x, v.y, v.z, v.w);
}

Vector2D FromFBXUvVector(const ofbx::Vec2& vec)
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

Matrix4x4 FromFBXMatrix(const ofbx::Matrix& mat)
{
	Matrix4x4 m(mat.m[0], mat.m[1], mat.m[2], mat.m[3],
		mat.m[4], mat.m[5], mat.m[6], mat.m[7],
		mat.m[8], mat.m[9], mat.m[10], mat.m[11],
		mat.m[12], mat.m[13], mat.m[14], mat.m[15]);

	return m;
}

void GetFBXConvertMatrix(const ofbx::GlobalSettings& settings, Matrix3x3& convertMatrix, bool& invertFaces)
{
	const float scaleFactor = settings.UnitScaleFactor * 0.01f;
	Vector3D scale(scaleFactor);
	if (settings.CoordAxis == ofbx::CoordSystem::CoordSystem_RightHanded)
	{
		scale.x *= -1.0f;
		invertFaces = true;
	}
	else
		invertFaces = false;

	convertMatrix = scale3(scale.x, scale.y, scale.z);
}

void TransformModelGeom(DSModel* model, const Matrix4x4& transform)
{
	for (DSGroup* group : model->groups)
	{
		for (DSVertex& vert : group->verts)
		{
			vert.position = transformPoint(vert.position, transform);
			vert.normal = transformVector(vert.normal, transform);
		}
	}
}

void TransformShapeDataGeom(DSShapeData* shapeData, const Matrix4x4& transform)
{
	for (DSShapeKey* shapeKey : shapeData->shapes)
	{
		for (DSShapeVert& vert : shapeKey->verts)
		{
			vert.position = transformPoint(vert.position, transform);
			vert.normal = transformVector(vert.normal, transform);
		}
	}
}

struct VertexWeightData
{
	Map<int, float>			indexWeightMap{ PP_SL };
	const ofbx::Object*		sourceBone{ nullptr };
	const ofbx::Cluster*	sourceCluster{nullptr};
	Matrix4x4				boneMatrix{ identity4 };
	int						boneId{ -1 };
};

struct ObjectData
{
	const ofbx::Mesh*		mesh{ nullptr };
	const ofbx::Geometry*	geom{ nullptr };

	const ofbx::Skin*		skin{ nullptr };
	Array<DSBone*>			bones{PP_SL};
	Array<VertexWeightData> weightData{PP_SL};

	Matrix4x4				transform{ identity4 };
};

void GetFBXBonesAsDSM(const ofbx::Mesh& mesh, Array<DSBone*>& bones, Array<VertexWeightData>& weightData, const Matrix4x4& transform, const Matrix3x3& convertMatrix)
{
	const ofbx::Geometry& geom = *mesh.getGeometry();
	const ofbx::Skin* skin = geom.getSkin();

	if (!skin)
		return;

	const int weightDataStart = weightData.numElem();

	const Matrix4x4 skinConvertMatrix = Matrix4x4(convertMatrix);

	const Matrix3x3 normalizedConvertMatrix = Matrix3x3(
		normalize(convertMatrix.rows[0]), 
		normalize(convertMatrix.rows[1]),
		normalize(convertMatrix.rows[2]));

	const float matDet = det(normalizedConvertMatrix);
	const Vector3D convertMatrixScale = Vector3D(
		dot(convertMatrix.rows[0], normalizedConvertMatrix.rows[0]),
		dot(convertMatrix.rows[1], normalizedConvertMatrix.rows[1]),
		dot(convertMatrix.rows[2], normalizedConvertMatrix.rows[2]));

	// collect matrices and weights first
	Array<Matrix4x4> boneMatries(PP_SL);
	Set<const ofbx::Object*> boneSet(PP_SL);

	const int numClusters = skin->getClusterCount();
	for (int i = 0; i < numClusters; ++i)
	{
		const ofbx::Cluster& fbxCluster = *skin->getCluster(i);
		const ofbx::Object* fbxBoneLink = fbxCluster.getLink();

		if (boneSet.contains(fbxBoneLink))
			continue;
		boneSet.insert(fbxBoneLink);

		// link matrix must be same as fbxBoneLink->getGlobalTransform()
		const ofbx::Matrix fbxBoneMat = fbxCluster.getTransformLinkMatrix(); 

		// apply mesh transform alongside with FBX conversion matrix
		const Matrix4x4 boneTransform = FromFBXMatrix(fbxBoneMat);
		boneMatries.append(boneTransform);

		VertexWeightData& wd = weightData.append();
		wd.boneId = i;
		wd.sourceBone = fbxBoneLink;
		wd.sourceCluster = &fbxCluster;

		const int* indices = fbxCluster.getIndices();
		for (int k = 0; k < fbxCluster.getIndicesCount(); ++k)
		{
			wd.indexWeightMap.insert(indices[k], fbxCluster.getWeights()[k]);
		}
	}

	MsgInfo("	Mesh %s\n", mesh.name);

	// add bones
	ArrayRef<VertexWeightData> thisWeightData(weightData.ptr() + weightDataStart, weightData.numElem() - weightDataStart);
	for (int i = 0; i < thisWeightData.numElem(); ++i)
	{
		VertexWeightData& wd = thisWeightData[i];

		const ofbx::Cluster& fbxCluster = *wd.sourceCluster;
		const ofbx::Object* fbxBoneLink = wd.sourceBone;

		DSBone* pBone = PPNew DSBone();
		pBone->name = fbxCluster.name;
		pBone->bone_id = i;

		// find parent bone link
		for (int j = 0; j < thisWeightData.numElem(); ++j)
		{
			const ofbx::Cluster& fbxCluserJ = *thisWeightData[j].sourceCluster;
			const ofbx::Object* fbxBoneLinkJ = fbxCluserJ.getLink();

			if (fbxBoneLinkJ == fbxBoneLink->getParent())
			{
				pBone->parent_name = fbxBoneLinkJ->name;
				pBone->parent_id = j;
				break;
			}
		}

		Matrix4x4 boneMatrix = boneMatries[i];
		if (pBone->parent_id != -1)
			boneMatrix = boneMatrix * !boneMatries[pBone->parent_id];

		wd.boneMatrix = boneMatrix;

		// in Eq each bone transform is strictly related to it's parent
		{
			pBone->position = boneMatrix.getTranslationComponent();
			pBone->angles = EulerMatrixXYZ(boneMatrix.getRotationComponent());

			if(matDet < 0)
			{
				pBone->angles *= -Vector3D(sign(matDet), 1.0f, 1.0f);
				pBone->position *= Vector3D(sign(matDet), 1.0f, 1.0f);
			}

			if(pBone->parent_id == -1)
				pBone->position = inverseTransformVector(pBone->position, normalizedConvertMatrix) * convertMatrixScale;
		}

		bones.append(pBone);
	}
}

void ConvertFBXMeshToDSM(int meshId, DSModel* model, DSShapeData* shapeData, Map<int, DSGroup*>& materialGroups, const ofbx::Mesh& mesh, const ofbx::GlobalSettings& settings, bool invertFaces, const Matrix4x4& transform, const Matrix3x3& convertMatrix)
{
	Array<VertexWeightData> weightData(PP_SL);
	GetFBXBonesAsDSM(mesh, model->bones, weightData, transform, convertMatrix);

	const ofbx::Geometry& geom = *mesh.getGeometry();
	const ofbx::Vec3* vertices = geom.getVertices();
	const ofbx::Vec3* normals = geom.getNormals();
	const ofbx::Vec2* uvs = geom.getUVs();

	// get blend shapes
	const ofbx::BlendShape* blendShape = geom.getBlendShape();
	if (blendShape && shapeData)
	{
		const int numBlendShapeChannels = blendShape->getBlendShapeChannelCount();
		Msg("    has %d blend shape channels\n", numBlendShapeChannels);
		for (int j = 0; j < numBlendShapeChannels; ++j)
		{
			const ofbx::BlendShapeChannel* blendShapeChan = blendShape->getBlendShapeChannel(j);
			const int numBlendShapes = blendShapeChan->getShapeCount();
			Msg("\t has %d blend shapes\n", numBlendShapes);
			for (int k = 0; k < numBlendShapes; ++k)
			{
				const ofbx::Shape* shape = blendShapeChan->getShape(k);
				Msg("\t\t %s\n", shape->name);

				DSShapeKey* shapeKey = PPNew DSShapeKey();
				shapeKey->name = shape->name;

				shapeData->shapes.append(shapeKey);

				const ofbx::Vec3* shapeVertices = shape->getVertices();
				const ofbx::Vec3* shapeNormals = shape->getNormals();

				const int vertCount = shape->getVertexCount();
				for (int vertId = 0; vertId < vertCount; vertId += 3)
				{
					for (int tVert = 0; tVert < 3; ++tVert)
					{
						const int tVertId = vertId + (invertFaces ? 2 - tVert : tVert);

						DSShapeVert vert;
						vert.vertexId = tVertId;
						vert.position = FromFBXVector(shapeVertices[tVertId], settings);
						vert.normal = FromFBXVector(shapeNormals[tVertId], settings);
						shapeKey->verts.append(vert);
					}
				} // vertId
			} // k
		} // j
	}

	// const int* faceIndices = geom.getFaceIndices();
	const int* vertMaterials = geom.getMaterials();

	// NOTE: OpenFBX prefers triangulation without indexation
	// and it's not possible to validly obtain the indices with messing up the geometry.
	int triNum = 0;
	const int vertexCount = geom.getVertexCount();
	for (int j = 0; j < vertexCount; j += 3, ++triNum)
	{
		DSGroup* dsmGrp = nullptr;

		const int materialIdx = vertMaterials ? vertMaterials[triNum] : 0;
		const int materialGroupIdx = meshId | (materialIdx << 16);
		auto found = materialGroups.find(materialGroupIdx);
		if (found.atEnd())
		{
			dsmGrp = PPNew DSGroup();

			const ofbx::Material* material = mesh.getMaterialCount() > 0 ? mesh.getMaterial(materialIdx) : nullptr;
			if (material)
				dsmGrp->texture = material->name;

			materialGroups.insert(materialGroupIdx, dsmGrp);
			model->groups.append(dsmGrp);
		}
		else
		{
			dsmGrp = *found;
		}

		for (int k = 0; k < 3; ++k)
		{
			const int jj = j + (invertFaces ? 2 - k : k);
			DSVertex& vert = dsmGrp->verts.append();
			vert.position = FromFBXVector(vertices[jj], settings);

			if (normals)
				vert.normal = FromFBXVector(normals[jj], settings);

			if (uvs)
				vert.texcoord = FromFBXUvVector(uvs[jj]);

			vert.vertexId = jj;

			const int numWeights = weightData.numElem();
			Array<float> tempWeights(PP_SL);
			Array<int> tempWeightBones(PP_SL);

			// Apply bone weights to vertex
			for (int w = 0; w < numWeights; ++w)
			{
				const VertexWeightData& wd = weightData[w];
				auto it = wd.indexWeightMap.find(jj);
				if (it.atEnd())
					continue;

				tempWeights.append(*it);
				tempWeightBones.append(wd.boneId);
			} // w

			const int numNewWeights = SortAndBalanceBones(tempWeights.numElem(), MAX_MODEL_VERTEX_WEIGHTS, tempWeightBones.ptr(), tempWeights.ptr());

			// copy weights
			for (int w = 0; w < numNewWeights; w++)
			{
				DSWeight& weight = vert.weights.append();
				weight.bone = tempWeightBones[w];
				weight.weight = tempWeights[w];
			}
		} // k
	} // j

	TransformModelGeom(model, transform);

	if(shapeData)
		TransformShapeDataGeom(shapeData, transform);
}


bool LoadFBX(Array<DSModelContainer>& modelContainerList, const char* filename)
{
	long fileSize = 0;
	char* fileBuffer = (char*)g_fileSystem->GetFileBuffer(filename, &fileSize);

	if (!fileBuffer)
	{
		MsgError("Couldn't open FBX file '%s'\n", filename);
		return false;
	}

	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);

	if (!scene)
	{
		MsgError("FBX '%s' error: ", filename, ofbx::getError());
		PPFree(fileBuffer);
		return false;
	}

	{
		const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

		Matrix3x3 convertMatrix = identity3;
		bool invertFaces = false;
		GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

		const int meshCount = scene->getMeshCount();
		for (int i = 0; i < meshCount; ++i)
		{
			DSModelContainer& container = modelContainerList.append();
			container.model = CRefPtr_new(DSModel);
			container.shapeData = CRefPtr_new(DSShapeData);

			const ofbx::Mesh& mesh = *scene->getMesh(i);

			// this is used to transform mesh from FBX space
			const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
			const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
			const Matrix4x4 transform = globalTransform * geomMatrix * Matrix4x4(convertMatrix);

			Map<int, DSGroup*> materialGroups(PP_SL);
			ConvertFBXMeshToDSM(i, container.model, container.shapeData, materialGroups, mesh, settings, invertFaces, transform, convertMatrix);

			container.model->name = mesh.name;
			container.shapeData->reference = mesh.name;
			container.transform = transform;

			if (container.shapeData->shapes.numElem() == 0)
				container.shapeData = nullptr;
		}
	}

	PPFree(fileBuffer);
	return true;
}

// Editor variant
bool LoadFBXCompound( DSModel* model, const char* filename )
{
	ASSERT(model);

	long fileSize = 0;
	char* fileBuffer = (char*)g_fileSystem->GetFileBuffer(filename, &fileSize);

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

	{
		const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

		Matrix3x3 convertMatrix;
		bool invertFaces;
		GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

		Map<int, DSGroup*> materialGroups(PP_SL);

		const int meshCount = scene->getMeshCount();
		for (int i = 0; i < meshCount; ++i)
		{
			const ofbx::Mesh& mesh = *scene->getMesh(i);

			// this is used to transform mesh from FBX space
			const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
			const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
			const Matrix4x4 transform = globalTransform * geomMatrix * Matrix4x4(convertMatrix);

			ConvertFBXMeshToDSM(i, model, nullptr, materialGroups, mesh, settings, invertFaces, transform, convertMatrix);
		}
	}

	PPFree(fileBuffer);
	return true;
}

// EGF compiler variant
bool LoadFBXShapes(DSModelContainer& modelContainer, const char* filename)
{
	long fileSize = 0;
	char* fileBuffer = (char*)g_fileSystem->GetFileBuffer(filename, &fileSize);

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

	modelContainer.model = CRefPtr_new(DSModel);
	modelContainer.shapeData = CRefPtr_new(DSShapeData);

	modelContainer.shapeData->reference = filename;
	{
		const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

		Matrix3x3 convertMatrix;
		bool invertFaces;
		GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

		Map<int, DSGroup*> materialGroups(PP_SL);

		const int meshCount = scene->getMeshCount();
		for (int i = 0; i < meshCount; ++i)
		{
			const ofbx::Mesh& mesh = *scene->getMesh(i);

			// this is used to transform mesh from FBX space
			const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
			const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
			const Matrix4x4 transform = globalTransform * geomMatrix * Matrix4x4(convertMatrix);

			ConvertFBXMeshToDSM(i, modelContainer.model, modelContainer.shapeData, materialGroups, mesh, settings, invertFaces, transform, convertMatrix);
		}
	}

	PPFree(fileBuffer);

	return true;
}

//-------------------------------------------------------------

template<class T>
void ZoomArray(const Array<T>& src, Array<T>& dest, int newLength)
{
	auto interp1 = [&src](float x, int n)
	{
		if (x <= 0) 
			return src[0];

		if (x >= n - 1) 
			return src[n - 1];

		const int j = int(x);
		return src[j] + (x - j) * (src[j + 1] - src[j]);
	};

	dest.setNum(newLength);
	const int oldLength = src.numElem();
	const float step = float(oldLength - 1) / (newLength - 1);

	for (int j = 0; j < newLength; ++j)
	{
		dest[j] = interp1(j * step, oldLength);
	}
}

void GetFBXCurveAsInterpKeyFrames(const ofbx::AnimationCurveNode* curveNode, Array<Vector3D>& keyFrames, int animationDuration, float localDuration)
{
	const ofbx::AnimationCurve* nodeX = curveNode->getCurve(0);
	const ofbx::AnimationCurve* nodeY = curveNode->getCurve(1);
	const ofbx::AnimationCurve* nodeZ = curveNode->getCurve(2);

	Map<ofbx::i64, float> valueX(PP_SL);
	Map<ofbx::i64, float> valueY(PP_SL);
	Map<ofbx::i64, float> valueZ(PP_SL);

	Set<ofbx::i64> allTimes(PP_SL);

	auto insertFrames = [&allTimes](const ofbx::AnimationCurve* curve, Map<ofbx::i64, float>& destVal)
	{
		const ofbx::i64* times = curve->getKeyTime();
		const float* values = curve->getKeyValue();
		const int keyCount = curve->getKeyCount();

		for (int i = 0; i < keyCount; ++i)
		{
			destVal[times[i]] = values[i];
			allTimes.insert(times[i]);
		}
	};

	insertFrames(nodeX, valueX);
	insertFrames(nodeY, valueY);
	insertFrames(nodeZ, valueZ);

	// convert frames
	int keyframeCounter = 0;
	IVector3D lastKeyframes(0);

	const int maxFrameCount = max(max(nodeX->getKeyCount(), nodeY->getKeyCount()), nodeZ->getKeyCount());

	Array<Vector3D> intermediateKeyFrames(PP_SL);
	intermediateKeyFrames.resize(maxFrameCount);

	auto interpKeyFrames = [&intermediateKeyFrames](int from, int to, int axis) {
		const float fromValue = intermediateKeyFrames[from][axis];
		const float toValue = intermediateKeyFrames[to][axis];
		for (int i = from; i < to; ++i)
		{
			const float percentage = RemapVal(i, from, to, 0.0f, 1.0f);
			intermediateKeyFrames[i][axis] = lerp(fromValue, toValue, percentage);
		}
	};

	for (auto it = allTimes.begin(); !it.atEnd(); ++it, ++keyframeCounter)
	{
		Vector3D& vecValue = intermediateKeyFrames.append();
		vecValue = F_UNDEF;
		auto fx = valueX.find(it.key());
		auto fy = valueY.find(it.key());
		auto fz = valueZ.find(it.key());

		// interpolate previous frames from last keyframe to this new one

		if (!fx.atEnd())
		{
			vecValue.x = *fx;
			interpKeyFrames(lastKeyframes.x, keyframeCounter, 0);
			lastKeyframes.x = keyframeCounter;
		}

		if (!fy.atEnd())
		{
			vecValue.y = *fy;
			interpKeyFrames(lastKeyframes.y, keyframeCounter, 1);
			lastKeyframes.y = keyframeCounter;
		}

		if (!fz.atEnd())
		{
			vecValue.z = *fz;
			interpKeyFrames(lastKeyframes.z, keyframeCounter, 2);
			lastKeyframes.z = keyframeCounter;
		}
	}

	ZoomArray(intermediateKeyFrames, keyFrames, max(animationDuration, 2));
}

void CollectFBXAnimations(Array<studioAnimation_t>& animations, ofbx::IScene* scene, const char* meshFilter)
{
	const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

	Matrix3x3 convertMatrix;
	bool invertFaces;
	GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

	const Matrix4x4 skinConvertMatrix = Matrix4x4(convertMatrix);
	const Matrix3x3 normalizedConvertMatrix = Matrix3x3(
		normalize(convertMatrix.rows[0]),
		normalize(convertMatrix.rows[1]),
		normalize(convertMatrix.rows[2]));

	const float matDet = det(normalizedConvertMatrix);
	const Vector3D convertMatrixScale = Vector3D(
		dot(convertMatrix.rows[0], normalizedConvertMatrix.rows[0]),
		dot(convertMatrix.rows[1], normalizedConvertMatrix.rows[1]),
		dot(convertMatrix.rows[2], normalizedConvertMatrix.rows[2]));

	// get bones from all meshes
	Array<ObjectData> objectDatas(PP_SL);

	// TODO: separate skeletons from each mesh ref
	const int meshCount = scene->getMeshCount();
	for (int i = 0; i < meshCount; ++i)
	{
		const ofbx::Mesh& mesh = *scene->getMesh(i);

		if (stricmp(mesh.name, meshFilter) != 0)
			continue;

		// this is used to transform mesh from FBX space
		const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
		const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
		const Matrix4x4 transform = globalTransform * geomMatrix * Matrix4x4(convertMatrix);

		ObjectData& objData = objectDatas.append();
		objData.transform = transform;
		objData.mesh = &mesh;
		objData.geom = mesh.getGeometry();
		objData.skin = objData.geom ? objData.geom->getSkin() : nullptr;
		GetFBXBonesAsDSM(mesh, objData.bones, objData.weightData, transform, convertMatrix);
	}

	if (!objectDatas.numElem())
	{
		MsgError("Cannot get meshes from FBX under name '%s', fix your meshFilter\n", meshFilter);
		return;
	}

	float frameRate = scene->getSceneFrameRate();
	if (frameRate <= 0)
		frameRate = 30.0f;

	struct AnimationTRS
	{
		Array<Vector3D> translations{PP_SL};
		Array<Vector3D> rotations{PP_SL};
		Array<Vector3D> scales{PP_SL};
	};

	const int animCount = scene->getAnimationStackCount();
	Msg("Animation count: %d\n", animCount);
	for (int i = 0; i < animCount; ++i)
	{
		const ofbx::AnimationStack* stack = scene->getAnimationStack(i);
		const ofbx::TakeInfo* takeInfo = scene->getTakeInfo(stack->name);
		if (takeInfo == nullptr)
		{
			Msg("No take info for %s\n", stack->name);
			continue;
		}

		const float localDuration = takeInfo->local_time_to - takeInfo->local_time_from;
		if (localDuration <= F_EPS)
		{
			Msg("Duration of %s is too short\n", stack->name);
			continue;
		}

		MsgWarning("Stack %s\n", stack->name);

		const int animationDuration = int(localDuration * frameRate + 0.5f);
		const ofbx::AnimationLayer* layer = stack->getLayer(0);

		for (ObjectData& objData : objectDatas)
		{
			if (objData.bones.numElem() == 0)
				continue;

			MsgWarning("Object %s\n", objData.mesh->name);

			const ofbx::Object& skeletonObj = *objData.weightData[0].sourceBone->getParent();
			const Matrix4x4 meshTransform = FromFBXMatrix(objData.mesh->getGlobalTransform());

			AnimationTRS rootAnimation;
			{
				const ofbx::AnimationCurveNode* rootTranslationNode = layer->getCurveNode(skeletonObj, "Lcl Translation");
				const ofbx::AnimationCurveNode* rootRotationNode = layer->getCurveNode(skeletonObj, "Lcl Rotation");
				const ofbx::AnimationCurveNode* rootScalingNode = layer->getCurveNode(skeletonObj, "Lcl Scaling");

				// keyframes are going to be interpolated and resampled in order to restore original keyframing
				if (rootTranslationNode)
					GetFBXCurveAsInterpKeyFrames(rootTranslationNode, rootAnimation.translations, animationDuration, localDuration);
				if (rootRotationNode)
					GetFBXCurveAsInterpKeyFrames(rootRotationNode, rootAnimation.rotations, animationDuration, localDuration);
				if (rootScalingNode)
					GetFBXCurveAsInterpKeyFrames(rootScalingNode, rootAnimation.scales, animationDuration, localDuration);
			}

			const int boneCount = objData.weightData.numElem();

			studioAnimation_t animation;
			strncpy(animation.name, stack->name, sizeof(animation.name));
			animation.name[sizeof(animation.name) - 1] = 0;
			animation.bones = PPAllocStructArray(studioBoneAnimation_t, boneCount);

			// convert bone animation
			for (int j = 0; j < boneCount; ++j)
			{
				const DSBone* bone = objData.bones[j];
				const VertexWeightData& wd = objData.weightData[j];

				// root animation
				// FBX does not apply armature transform animation to the root bone
				// so we have to do it ourselves
				Matrix4x4 invBoneMatrix = wd.boneMatrix;
				if (bone->parent_id == -1)
				{
					// don't forget to calc correct rest bone matrix
					invBoneMatrix = wd.boneMatrix * meshTransform;
				}
				invBoneMatrix = !invBoneMatrix;

				// bone animations
				AnimationTRS boneAnimation;
				{
					const ofbx::AnimationCurveNode* translationNode = layer->getCurveNode(*wd.sourceBone, "Lcl Translation");
					const ofbx::AnimationCurveNode* rotationNode = layer->getCurveNode(*wd.sourceBone, "Lcl Rotation");
					const ofbx::AnimationCurveNode* scalingNode = layer->getCurveNode(*wd.sourceBone, "Lcl Scaling");

					// keyframes are going to be interpolated and resampled in order to restore original keyframing
					if (translationNode)
						GetFBXCurveAsInterpKeyFrames(translationNode, boneAnimation.translations, animationDuration, localDuration);
					if (rotationNode)
						GetFBXCurveAsInterpKeyFrames(rotationNode, boneAnimation.rotations, animationDuration, localDuration);
					if (scalingNode)
						GetFBXCurveAsInterpKeyFrames(scalingNode, boneAnimation.scales, animationDuration, localDuration);
				}

				ASSERT_MSG(boneAnimation.translations.numElem() <= MaxFramesPerAnimation, "Too many frames in animation (%d, limit is %d)", boneAnimation.translations.numElem(), MaxFramesPerAnimation);
				ASSERT_MSG(boneAnimation.translations.numElem() == boneAnimation.rotations.numElem(), "Rotations %d translations %d", boneAnimation.rotations.numElem(), boneAnimation.translations.numElem());
				ASSERT_MSG(rootAnimation.translations.numElem() == boneAnimation.translations.numElem(), "Root and bone animation frame mismatch");

				if (!boneAnimation.translations.numElem() && !boneAnimation.rotations.numElem())
					continue;

				// alloc frames
				const int numFrames = boneAnimation.translations.numElem();
			
				studioBoneAnimation_t& outBoneAnim = animation.bones[j];
				outBoneAnim.numFrames = numFrames;
				outBoneAnim.keyFrames = PPAllocStructArray(animframe_t, numFrames);
			
				// perform conversion of each frame to local space
				for (int k = 0; k < numFrames; ++k)
				{
					animframe_t& outFrame = outBoneAnim.keyFrames[k];

					const Vector3D rotation = boneAnimation.rotations[k];
					const Vector3D translation = boneAnimation.translations[k];

					const ofbx::Vec3 translationFrame{translation.x, translation.y, translation.z};
					const ofbx::Vec3 rotationFrame{rotation.x, rotation.y, rotation.z};

					Matrix4x4 meshAnimTransform = identity4;
					if (bone->parent_id == -1)
					{
						const Vector3D rotation = rootAnimation.rotations[k];
						const Vector3D translation = rootAnimation.translations[k];

						const ofbx::Vec3 translationFrame{translation.x, translation.y, translation.z};
						const ofbx::Vec3 rotationFrame{rotation.x, rotation.y, rotation.z};

						// we need to apply this parent transform accordingly
						meshAnimTransform = FromFBXMatrix(skeletonObj.evalLocal(translationFrame, rotationFrame)) * meshTransform;
					}

					const Matrix4x4 animBoneMatrix = FromFBXMatrix(wd.sourceBone->evalLocal(translationFrame, rotationFrame)) * meshAnimTransform * invBoneMatrix;

					// in Eq each bone transform is strictly related to it's parent
					{
						outFrame.vecBonePosition = animBoneMatrix.getTranslationComponent();
						outFrame.angBoneAngles = EulerMatrixXYZ(animBoneMatrix.getRotationComponent());

						if (matDet < 0)
						{
							outFrame.angBoneAngles *= -Vector3D(sign(matDet), 1.0f, 1.0f);
							outFrame.vecBonePosition *= Vector3D(sign(matDet), 1.0f, 1.0f);
						}

						//if (bone->parent_id == -1)
						//	outFrame.vecBonePosition = transformVector(outFrame.vecBonePosition, normalizedConvertMatrix) * convertMatrixScale;
					}
				}
			}

			if (animation.bones[0].numFrames)
			{
				ASSERT_MSG(animation.bones[0].numFrames <= MaxFramesPerAnimation, "Too many frames in animation (%d, limit is %d)", animation.bones[0].numFrames, MaxFramesPerAnimation);

				Msg("  Anim: %s, duration: %d, frames: %d\n", stack->name, animationDuration, animation.bones[0].numFrames);
				animations.append(animation);
			}
			else
			{
				for (int i = 0; i < boneCount; i++)
					PPFree(animation.bones[i].keyFrames);
				PPFree(animation.bones);
			}
		}
	}

	for (ObjectData& objData : objectDatas)
	{
		for (int i = 0; i < objData.bones.numElem(); ++i)
			delete objData.bones[i];
	}
}

bool LoadFBXAnimations(Array<studioAnimation_t>& animations, const char* filename, const char* meshFilter)
{
	long fileSize = 0;
	char* fileBuffer = (char*)g_fileSystem->GetFileBuffer(filename, &fileSize);
	if (!fileBuffer)
	{
		MsgError("Couldn't open FBX file '%s'\n", filename);
		return false;
	}

	defer{
		PPFree(fileBuffer);
	};

	if(meshFilter == nullptr || *meshFilter == 0)
	{
		MsgWarning("FBXSource meshFilter must be specified\n");
		return false;
	}

	ofbx::u64 loadFlags = 0;// (ofbx::u64)ofbx::LoadFlags::IGNORE_BLEND_SHAPES;
	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, loadFlags);

	if (!scene)
	{
		MsgError("FBX '%s' error: %s\n", filename, ofbx::getError());
		return false;
	}

	CollectFBXAnimations(animations, scene, meshFilter);

	return true;
}

} // namespace