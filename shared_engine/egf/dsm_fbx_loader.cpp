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
	int boneId;
	const ofbx::Object* boneObject{ nullptr };
	Map<int, float> indexWeightMap{ PP_SL };
};

void GetFBXBonesAsDSM(const ofbx::Geometry& geom, Array<DSBone*>& bones, Array<VertexWeightData>& weightData, const Matrix4x4& transform)
{
	const ofbx::Skin* skin = geom.getSkin();

	if (!skin)
		return;

	// since it's attached to geom, we need it's matrix
	// to properly convert the BindPose
	const Matrix4x4 geomTransformInv = !FromFBXMatrix(geom.getGlobalTransform());

	const int numBones = skin->getClusterCount();

	const Matrix3x3 boneConvertMatrix = rotateX3(DEG2RAD(-90)) * rotateZ3(DEG2RAD(180));

	Set<const ofbx::Object*> boneSet(PP_SL);

	for (int i = 0; i < numBones; ++i)
	{
		const ofbx::Cluster& fbxCluster = *skin->getCluster(i);
		const ofbx::Object* fbxBoneLink = fbxCluster.getLink();

		if (boneSet.contains(fbxBoneLink))
			continue;
		boneSet.insert(fbxBoneLink);

		DSBone* pBone = PPNew DSBone();
		pBone->name = fbxCluster.name;
		pBone->bone_id = i;

		// find parent bone link
		for (int j = 0; j < numBones; ++j)
		{
			const ofbx::Cluster& fbxCluserJ = *skin->getCluster(j);
			const ofbx::Object* fbxBoneLinkJ = fbxCluserJ.getLink();

			if (fbxBoneLinkJ == fbxBoneLink->getParent())
			{
				pBone->parent_name = fbxBoneLinkJ->name;
				pBone->parent_id = j;
				break;
			}
		}

		// TransformLinkMatrix is Global, convert to Eq Local
		{
			const ofbx::Matrix fbxBoneMat = fbxCluster.getTransformLinkMatrix();
			Matrix4x4 boneMatrixLocal = FromFBXMatrix(fbxBoneMat) * geomTransformInv;
			if (pBone->parent_id != -1)
			{
				const ofbx::Cluster& fbxCluserParent = *skin->getCluster(pBone->parent_id);
				boneMatrixLocal = boneMatrixLocal * !(FromFBXMatrix(fbxCluserParent.getTransformLinkMatrix()) * geomTransformInv);
			}
			const Matrix4x4 boneMatrix = (pBone->parent_id == -1) ? (boneMatrixLocal * Matrix4x4(boneConvertMatrix)) : boneMatrixLocal;

			pBone->position = boneMatrix.getTranslationComponent();
			pBone->angles = EulerMatrixXYZ(boneMatrix.getRotationComponent());

			// Convert RH -> LH
			pBone->position *= Vector3D(-1, 1, 1);
			pBone->angles *= Vector3D(1, -1, -1);
		}

		bones.append(pBone);

		VertexWeightData& wd = weightData.append();
		wd.boneId = i;
		wd.boneObject = fbxBoneLink;

		const int* indices = fbxCluster.getIndices();
		for (int k = 0; k < fbxCluster.getIndicesCount(); ++k)
		{
			wd.indexWeightMap[indices[k]] = fbxCluster.getWeights()[k];
		}
	}
}

void ConvertFBXMeshToDSM(int meshId, DSModel* model, DSShapeData* shapeData, Map<int, DSGroup*>& materialGroups, const ofbx::Mesh& mesh, const ofbx::GlobalSettings& settings, bool invertFaces, const Matrix4x4& transform)
{
	const ofbx::Geometry& geom = *mesh.getGeometry();

	Array<VertexWeightData> weightData(PP_SL);
	GetFBXBonesAsDSM(geom, model->bones, weightData, transform);

	const int vertex_count = geom.getVertexCount();

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
	for (int j = 0; j < vertex_count; j += 3, ++triNum)
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

		Matrix3x3 convertMatrix;
		bool invertFaces;
		GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

		const int mesh_count = scene->getMeshCount();
		for (int i = 0; i < mesh_count; ++i)
		{
			DSModelContainer& container = modelContainerList.append();
			container.model = CRefPtr_new(DSModel);
			container.shapeData = CRefPtr_new(DSShapeData);

			const ofbx::Mesh& mesh = *scene->getMesh(i);

			// this is used to transform mesh from FBX space
			const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
			const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
			const Matrix4x4 transform =  globalTransform * geomMatrix * Matrix4x4(convertMatrix);

			Map<int, DSGroup*> materialGroups(PP_SL);
			ConvertFBXMeshToDSM(i, container.model, container.shapeData, materialGroups, mesh, settings, invertFaces, transform);

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

		const int mesh_count = scene->getMeshCount();
		for (int i = 0; i < mesh_count; ++i)
		{
			const ofbx::Mesh& mesh = *scene->getMesh(i);

			// this is used to transform mesh from FBX space
			const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
			const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
			const Matrix4x4 transform = globalTransform * geomMatrix * Matrix4x4(convertMatrix);

			ConvertFBXMeshToDSM(i, model, nullptr, materialGroups, mesh, settings, invertFaces, transform);
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

		const int mesh_count = scene->getMeshCount();
		for (int i = 0; i < mesh_count; ++i)
		{
			const ofbx::Mesh& mesh = *scene->getMesh(i);

			// this is used to transform mesh from FBX space
			const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
			const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
			const Matrix4x4 transform = globalTransform * geomMatrix * Matrix4x4(convertMatrix);

			ConvertFBXMeshToDSM(i, modelContainer.model, modelContainer.shapeData, materialGroups, mesh, settings, invertFaces, transform);
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

void GetFBXCurveAsInterpKeyFrames(const ofbx::AnimationCurveNode* curveNode, Array<Vector3D>& keyFrames, int animationDuration, float localDuration, bool angles)
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

void ConvertFBXToESA(Array<studioAnimation_t>& animations, ofbx::IScene* scene)
{
	const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

	Matrix3x3 convertMatrix;
	bool invertFaces;
	GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

	Matrix3x3 boneConvertMatrix = rotateX3(DEG2RAD(-90)) * rotateZ3(DEG2RAD(180));
	Quaternion boneConvertRotation(boneConvertMatrix);

	// get bones from all meshes
	Array<DSBone*> bones(PP_SL);
	Array<VertexWeightData> weightData(PP_SL);

	// TODO: separate skeletons from each mesh ref
	const int mesh_count = scene->getMeshCount();
	for (int i = 0; i < mesh_count; ++i)
	{
		const ofbx::Mesh& mesh = *scene->getMesh(i);
		const ofbx::Geometry& geom = *mesh.getGeometry();

		// this is used to transform mesh from FBX space
		const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
		const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
		const Matrix4x4 transform = globalTransform * geomMatrix * Matrix4x4(convertMatrix);

		GetFBXBonesAsDSM(geom, bones, weightData, transform);
	}

	float frameRate = scene->getSceneFrameRate();
	if (frameRate <= 0)
		frameRate = 30.0f;

	const int anim_count = scene->getAnimationStackCount();

	Msg("Animation count: %d\n", anim_count);
	for (int i = 0; i < anim_count; ++i)
	{
		const ofbx::AnimationStack* stack = scene->getAnimationStack(i);
		const ofbx::AnimationLayer* layer = stack->getLayer(0);
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

		const int animationDuration = int(localDuration * frameRate + 0.5f);
		const int boneCount = weightData.numElem();

		studioAnimation_t animation;
		strncpy(animation.name, stack->name, sizeof(animation.name));
		animation.name[sizeof(animation.name) - 1] = 0;

		animation.bones = PPAllocStructArray(studioBoneAnimation_t, boneCount);

		for (int j = 0; j < boneCount; ++j)
		{
			const ofbx::AnimationCurveNode* translationNode = layer->getCurveNode(*weightData[j].boneObject, "Lcl Translation");
			const ofbx::AnimationCurveNode* rotationNode = layer->getCurveNode(*weightData[j].boneObject, "Lcl Rotation");

			Array<Vector3D> rotations(PP_SL);
			Array<Vector3D> translations(PP_SL);

			// keyframes are going to be interpolated and resampled in order to restore original keyframing
			if (translationNode)
				GetFBXCurveAsInterpKeyFrames(translationNode, translations, animationDuration, localDuration, false);
			if (rotationNode)
				GetFBXCurveAsInterpKeyFrames(rotationNode, rotations, animationDuration, localDuration, true);

			ASSERT_MSG(translations.numElem() == rotations.numElem(), "Rotations %d translations %d", rotations.numElem(), translations.numElem());

			if (!translations.numElem() && !rotations.numElem())
				continue;

			animation.bones[j].numFrames = translations.numElem();
			animation.bones[j].keyFrames = PPAllocStructArray(animframe_t, translations.numElem());

			const Vector3D boneRestRotation = bones[j]->angles;
			const Vector3D boneRestPosition = bones[j]->position;

			const Quaternion boneRestRotationQuat = !Quaternion(boneRestRotation.x, boneRestRotation.y, boneRestRotation.z);

			// perform conversion of each frame to local space
			for (int k = 0; k < translations.numElem(); ++k)
			{
				animframe_t& frame = animation.bones[j].keyFrames[k];

				// Convert RH -> LH
				const Vector3D rotRad = DEG2RAD(rotations[k]) * Vector3D(1, -1, -1);

				Quaternion rotation = Quaternion(ConstrainAnglePI(rotRad.x), ConstrainAnglePI(rotRad.y), ConstrainAnglePI(rotRad.z));
				Vector3D translation = translations[k] * Vector3D(-1, 1, 1);

				if (bones[j]->parent_id == -1)
				{
					rotation = rotation * boneConvertRotation;
					translation = rotateVector(translation, boneConvertRotation);
				}

				// convert to local space and assign
				frame.vecBonePosition = translation - boneRestPosition;
				frame.angBoneAngles = eulersXYZ(rotation * boneRestRotationQuat);

				//if (bones[j]->parent_id == -1)
				//	Msg("anim %s frame %d root bone angles: %f %f %f\n", stack->name, k, frame.angBoneAngles.x, frame.angBoneAngles.y, frame.angBoneAngles.z);

				//if (bones[j]->parent_id == -1)
				//	Msg("anim %s frame %d root bone pos: %f %f %f\n", stack->name, k, frame.vecBonePosition.x, frame.vecBonePosition.y, frame.vecBonePosition.z);

				// Hmm, WTF?! This needs to be fixed!!!
				frame.vecBonePosition *= Vector3D(1, 1, -1);
			}
		}

		if (animation.bones[0].numFrames > 0)
		{
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

bool LoadFBXAnimations(Array<studioAnimation_t>& animations, const char* filename)
{
	long fileSize = 0;
	char* fileBuffer = (char*)g_fileSystem->GetFileBuffer(filename, &fileSize);

	if (!fileBuffer)
	{
		MsgError("Couldn't open FBX file '%s'\n", filename);
		return false;
	}

	ofbx::u64 loadFlags = (ofbx::u64)ofbx::LoadFlags::IGNORE_BLEND_SHAPES;
	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, loadFlags);

	if (!scene)
	{
		MsgError("FBX '%s' error: %s\n", filename, ofbx::getError());
		PPFree(fileBuffer);
		return false;
	}

	ConvertFBXToESA(animations, scene);

	PPFree(fileBuffer);
	return true;
}

} // namespace