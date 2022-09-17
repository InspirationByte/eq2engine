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
#include "dsm_esm_loader.h"
#include "dsm_fbx_loader.h"
#include "modelloader_shared.h"
#pragma optimize("", off)

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

Vector3D FromFBXVector(const ofbx::Vec3& vec, const Matrix3x3& orient)
{
	Vector3D result(vec.x, vec.y, vec.z);

	return orient * result;
}

Vector3D FromFBXVector(const Vector3D& vec, const Matrix3x3& orient)
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

Matrix4x4 FromFBXMatrix(const ofbx::Matrix& mat)
{
	Matrix4x4 m(mat.m[0], mat.m[1], mat.m[2], mat.m[3],
		mat.m[4], mat.m[5], mat.m[6], mat.m[7],
		mat.m[8], mat.m[9], mat.m[10], mat.m[11],
		mat.m[12], mat.m[13], mat.m[14], mat.m[15]);

	return m;
}

int DecodeIndex(int idx)
{
	return (idx < 0) ? (-idx - 1) : idx;
}

void GetFBXConvertMatrix(const ofbx::GlobalSettings& settings, Matrix3x3& convertMatrix, bool& invertFaces)
{
	const float scaleFactor = settings.UnitScaleFactor;

	// this is Blender - only!
	convertMatrix = scale3(scaleFactor, scaleFactor, -scaleFactor) * rotateX3(DEG2RAD(-90));
	invertFaces = true;
}

struct VertexWeightData
{
	int boneId;
	const ofbx::Object* boneObject{ nullptr };
	Map<int, float> indexWeightMap{ PP_SL };
};

void GetFBXBonesAsDSM(const ofbx::Skin* skin, const Matrix3x3& convertMatrix, ofbx::UpVector upAxis, Array<dsmskelbone_t*>& bones, Array<VertexWeightData>& weightData)
{
	const int numBones = skin->getClusterCount();

	Matrix3x3 boneConvertMatrix = rotateX3(DEG2RAD(-90)) * rotateZ3(DEG2RAD(180));

	for (int i = 0; i < numBones; ++i)
	{
		const ofbx::Cluster& fbxCluster = *skin->getCluster(i);
		const ofbx::Object* fbxBoneLink = fbxCluster.getLink();

		dsmskelbone_t* pBone = PPNew dsmskelbone_t();
		strcpy(pBone->name, fbxCluster.name);
		pBone->bone_id = i;

		// find parent bone link
		for (int j = 0; j < numBones; ++j)
		{
			const ofbx::Cluster& fbxCluserJ = *skin->getCluster(j);
			const ofbx::Object* fbxBoneLinkJ = fbxCluserJ.getLink();

			if (fbxBoneLinkJ == fbxBoneLink->getParent())
			{
				strcpy(pBone->parent_name, fbxBoneLinkJ->name);
				pBone->parent_id = j;
				break;
			}
		}

		const ofbx::Matrix fbxBoneMat = fbxBoneLink->getLocalTransform();
		const Matrix4x4 boneMatrix = (pBone->parent_id == -1) ? FromFBXMatrix(fbxBoneMat, boneConvertMatrix) : FromFBXMatrix(fbxBoneMat);

		pBone->position = boneMatrix.getTranslationComponent();
		pBone->angles = EulerMatrixXYZ(boneMatrix.getRotationComponent());

		pBone->position *= Vector3D(-1, 1, 1);
		pBone->angles *= Vector3D(1, -1, -1);

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

void ConvertFBXToDSM(dsmmodel_t* model, esmshapedata_t* shapeData, ofbx::IScene* scene)
{
	const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

	Matrix3x3 convertMatrix;
	bool invertFaces;
	GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

	// convert FBX scene into DSM groups
	Map<int, dsmgroup_t*> materialGroups(PP_SL);

	const int mesh_count = scene->getMeshCount();
	for (int i = 0; i < mesh_count; ++i)
	{
		const ofbx::Mesh& mesh = *scene->getMesh(i);
		const ofbx::Geometry& geom = *mesh.getGeometry();

		// Msg("Mesh '%s'\n", mesh.name);

		const ofbx::Skin* skin = geom.getSkin();

		Array<VertexWeightData> weightData(PP_SL);
		
		if (skin)
			GetFBXBonesAsDSM(skin, convertMatrix, settings.UpAxis, model->bones, weightData);

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

					esmshapekey_t* shapeKey = PPNew esmshapekey_t();
					shapeData->shapes.append(shapeKey);

					shapeKey->name = shape->name;

					const ofbx::Vec3* shapeVertices = shape->getVertices();
					const ofbx::Vec3* shapeNormals = shape->getNormals();

					const int vertCount = shape->getVertexCount();
					for (int vertId = 0; vertId < vertCount; vertId += 3)
					{
						for (int tVert = 0; tVert < 3; ++tVert)
						{
							const int tVertId = vertId + (invertFaces ? 2 - tVert : tVert);

							esmshapevertex_t vert;
							vert.vertexId = tVertId;
							vert.position = FromFBXVector(shapeVertices[tVertId], convertMatrix);
							vert.normal = FromFBXVector(shapeNormals[tVertId], convertMatrix);
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

			for(int k = 0; k < 3; ++k)
			{
				const int jj = j + (invertFaces ? 2-k : k);
				dsmvertex_t& vert = dsmGrp->verts.append();
				vert.position = FromFBXVector(vertices[jj], convertMatrix);

				if (normals)
					vert.normal = FromFBXVector(normals[jj], convertMatrix);

				if (uvs)
					vert.texcoord = FromFBXVector(uvs[jj]);

				vert.vertexId = jj;

				const int numWeights = weightData.numElem();
				Array<float> tempWeights(PP_SL);
				Array<int> tempWeightBones(PP_SL);

				// Apply bone weights to vertex
				for (int w = 0; w < numWeights; ++w)
				{
					const VertexWeightData& wd = weightData[w];
					auto it = wd.indexWeightMap.find(jj);
					if (it == wd.indexWeightMap.end())
						continue;

					tempWeights.append(*it);
					tempWeightBones.append(wd.boneId);
				} // w

				const int numNewWeights = SortAndBalanceBones(tempWeights.numElem(), MAX_MODEL_VERTEX_WEIGHTS, tempWeightBones.ptr(), tempWeights.ptr());

				// copy weights
				for (int w = 0; w < numNewWeights; w++)
				{
					dsmweight_t& weight = vert.weights.append();
					weight.bone = tempWeightBones[w];
					weight.weight = tempWeights[w];
				}
			} // k
		} // j
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

	ConvertFBXToDSM(model, nullptr, scene);

	PPFree(fileBuffer);
	return true;
}

bool LoadFBXShapes(dsmmodel_t* model, esmshapedata_t* shapeData, const char* filename)
{
	ASSERT(model);
	ASSERT(shapeData);

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

	shapeData->reference = filename;
	ConvertFBXToDSM(model, shapeData, scene);

	PPFree(fileBuffer);

	return true;
}

void ConvertFBXToESA(Array<studioAnimation_t>& animations, ofbx::IScene* scene)
{
	const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

	Matrix3x3 convertMatrix;
	bool invertFaces;
	GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

	Matrix3x3 boneConvertMatrix = rotateX3(DEG2RAD(-90)) * rotateZ3(DEG2RAD(180));

	// get bones from all meshes
	Array<dsmskelbone_t*> bones(PP_SL);
	Array<VertexWeightData> weightData(PP_SL);

	const int mesh_count = scene->getMeshCount();
	for (int i = 0; i < mesh_count; ++i)
	{
		const ofbx::Mesh& mesh = *scene->getMesh(i);
		const ofbx::Geometry& geom = *mesh.getGeometry();

		// Msg("Mesh '%s'\n", mesh.name);

		const ofbx::Skin* skin = geom.getSkin();

		if (skin)
			GetFBXBonesAsDSM(skin, convertMatrix, settings.UpAxis, bones, weightData);
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
			continue;

		const float localDuration = takeInfo->local_time_to - takeInfo->local_time_from;
		if (localDuration <= F_EPS)
			continue;

		const int animationDuration = int(localDuration * frameRate + 0.5f);
		const int boneCount = weightData.numElem();

		studioAnimation_t animation;
		strncpy(animation.name, stack->name, sizeof(animation.name));
		animation.name[sizeof(animation.name) - 1] = 0;

		animation.bones = (studioBoneFrame_t*)PPAlloc(sizeof(studioBoneFrame_t) * boneCount);

		for (int j = 0; j < boneCount; ++j)
		{
			const ofbx::AnimationCurveNode* translationNode = layer->getCurveNode(*weightData[i].boneObject, "Lcl Translation");
			const ofbx::AnimationCurveNode* rotationNode = layer->getCurveNode(*weightData[i].boneObject, "Lcl Rotation");

			int rotationFrameCount = 0;
			int translationFrameCount = 0;

			Array<Vector3D> rotations(PP_SL);
			Array<Vector3D> translations(PP_SL);

			// Local bone rotation
			if (rotationNode)
			{
				const ofbx::AnimationCurve* nodeX = rotationNode->getCurve(0);
				const ofbx::AnimationCurve* nodeY = rotationNode->getCurve(1);
				const ofbx::AnimationCurve* nodeZ = rotationNode->getCurve(2);

				const ofbx::i64* timesX = nodeX->getKeyTime();
				const ofbx::i64* timesY = nodeY->getKeyTime();
				const ofbx::i64* timesZ = nodeZ->getKeyTime();

				const int minAnimFramesCheck = min(min(nodeX->getKeyCount(), nodeY->getKeyCount()), nodeZ->getKeyCount());
				const int maxAnimFramesCheck = max(max(nodeX->getKeyCount(), nodeY->getKeyCount()), nodeZ->getKeyCount());
				ASSERT_MSG(minAnimFramesCheck == maxAnimFramesCheck, "Please tell Soapy that his code sucks!!!");

				rotationFrameCount = nodeX->getKeyCount();

				for (int k = 0; k < rotationFrameCount; ++k)
				{
					const float boneFrameTime = ofbx::fbxTimeToSeconds(timesX[k]);
					const Vector3D rotationValue(
						nodeX->getKeyValue()[k],
						nodeY->getKeyValue()[k],
						nodeZ->getKeyValue()[k]
					);
					rotations.append(rotationValue);
				}
			}

			// Local bone offset
			if (translationNode)
			{
				const ofbx::AnimationCurve* nodeX = translationNode->getCurve(0);
				const ofbx::AnimationCurve* nodeY = translationNode->getCurve(1);
				const ofbx::AnimationCurve* nodeZ = translationNode->getCurve(2);

				const ofbx::i64* timesX = nodeX->getKeyTime();
				const ofbx::i64* timesY = nodeY->getKeyTime();
				const ofbx::i64* timesZ = nodeZ->getKeyTime();

				const int minAnimFramesCheck = min(min(nodeX->getKeyCount(), nodeY->getKeyCount()), nodeZ->getKeyCount());
				const int maxAnimFramesCheck = max(max(nodeX->getKeyCount(), nodeY->getKeyCount()), nodeZ->getKeyCount());
				ASSERT_MSG(minAnimFramesCheck == maxAnimFramesCheck, "Please tell Soapy that his code sucks!!!");

				translationFrameCount = nodeX->getKeyCount();

				for (int k = 0; k < translationFrameCount; ++k)
				{
					const float boneFrameTime = ofbx::fbxTimeToSeconds(timesX[k]);
					const Vector3D translationValue(
						nodeX->getKeyValue()[k],
						nodeY->getKeyValue()[k],
						nodeZ->getKeyValue()[k]
					);
					translations.append(translationValue);
				}
			}

			ASSERT_MSG(translationFrameCount == rotationFrameCount, "Please tell Soapy to de-hardcode FBX converter!!!");

			animation.bones[j].numFrames = translationFrameCount;
			animation.bones[j].keyFrames = PPNew animframe_t[translationFrameCount];
			for (int k = 0; k < translationFrameCount; ++k)
			{
				animframe_t& frame = animation.bones[j].keyFrames[k];
				frame.vecBonePosition = translations[k];
				frame.angBoneAngles = rotations[k];

				frame.vecBonePosition *= Vector3D(-1, 1, 1);
				frame.angBoneAngles *= Vector3D(1, -1, -1);
			}
		}

		Msg("  Anim: %s, duration: %d, frames: %d\n", stack->name, animationDuration, animation.bones[0].numFrames);

		if (animation.bones[0].numFrames > 0)
			animations.append(animation);
		else
			Studio_FreeAnimationData(&animation, boneCount);
	}
}

bool LoadFBXAnimations(Array<studioAnimation_t>& animations, const char* filename)
{
	long fileSize = 0;
	char* fileBuffer = g_fileSystem->GetFileBuffer(filename, &fileSize);

	if (!fileBuffer)
	{
		MsgError("Couldn't open FBX file '%s'\n", filename);
		return false;
	}

	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);

	if (!scene)
	{
		MsgError("FBX '%s' error: %s\n", filename, ofbx::getError());
		PPFree(fileBuffer);
		return false;
	}

	ConvertFBXToESA(animations, scene);

	PPFree(fileBuffer);
}

} // namespace