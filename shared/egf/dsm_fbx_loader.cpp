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
#include "egf/model.h"


static constexpr const int MaxFramesPerAnimation = 10000;

namespace SharedModel
{

template<typename T>
Vector3D FixOrient(const T& v, const ofbx::GlobalSettings& settings)
{
	Vector3D srcVec(v.x, v.y, v.z);
	return srcVec * Vector3D((settings.CoordAxis == ofbx::CoordinateAxis::NEGATIVE_X) ? -1.0f : 1.0f, 1.0f, 1.0f);
}

static Vector3D FromFBXVector(const ofbx::Vec3& v, const ofbx::GlobalSettings& settings)
{
	return FixOrient(v, settings);
}

static MColor FromFBXColorVector(const ofbx::Vec4& v)
{
	return MColor((float)v.x, (float)v.y, (float)v.z, (float)v.w);
}

static Vector2D FromFBXUvVector(const ofbx::Vec2& vec)
{
	return Vector2D(vec.x, 1.0f - vec.y);
}

static Matrix4x4 FromFBXMatrix(const ofbx::DMatrix& mat, const Matrix3x3& orient)
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

static Matrix4x4 FromFBXMatrix(const ofbx::Matrix& mat, const Matrix3x3& orient)
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

static Matrix4x4 FromFBXMatrix(const ofbx::DMatrix& mat)
{
	Matrix4x4 m(mat.m[0], mat.m[1], mat.m[2], mat.m[3],
		mat.m[4], mat.m[5], mat.m[6], mat.m[7],
		mat.m[8], mat.m[9], mat.m[10], mat.m[11],
		mat.m[12], mat.m[13], mat.m[14], mat.m[15]);

	return m;
}

static Matrix4x4 FromFBXMatrix(const ofbx::Matrix& mat)
{
	Matrix4x4 m(mat.m[0], mat.m[1], mat.m[2], mat.m[3],
		mat.m[4], mat.m[5], mat.m[6], mat.m[7],
		mat.m[8], mat.m[9], mat.m[10], mat.m[11],
		mat.m[12], mat.m[13], mat.m[14], mat.m[15]);

	return m;
}

static void GetFBXConvertMatrix(const ofbx::GlobalSettings& settings, Matrix3x3& convertMatrix, bool& invertFaces)
{
	const float scaleFactor = settings.UnitScaleFactor * 0.01f;

	invertFaces = (settings.CoordAxis == ofbx::CoordinateAxis::NEGATIVE_X);
	convertMatrix = scale3(scaleFactor, scaleFactor, scaleFactor);
}

static void TransformModelGeom(DSModel& model, const Matrix4x4& transform)
{
	Matrix3x3 normalsRotateVec = transform.getRotationComponent();
	normalsRotateVec.rows[0] = normalize(normalsRotateVec.rows[0]);
	normalsRotateVec.rows[1] = normalize(normalsRotateVec.rows[1]);
	normalsRotateVec.rows[2] = normalize(normalsRotateVec.rows[2]);

	for (DSMesh* group : model.meshes)
	{
		for (DSVertex& vert : group->verts)
		{
			vert.position = transformPoint(vert.position, transform);
			vert.normal = rotateVector(vert.normal, normalsRotateVec);
		}
	}
}

static void TransformShapeDataGeom(DSShapeData* shapeData, const Matrix4x4& transform)
{
	Matrix3x3 normalsRotateVec = transform.getRotationComponent();
	normalsRotateVec.rows[0] = normalize(normalsRotateVec.rows[0]);
	normalsRotateVec.rows[1] = normalize(normalsRotateVec.rows[1]);
	normalsRotateVec.rows[2] = normalize(normalsRotateVec.rows[2]);

	for (DSShapeKey* shapeKey : shapeData->shapes)
	{
		for (DSShapeVert& vert : shapeKey->verts)
		{
			vert.position = transformPoint(vert.position, transform);
			vert.normal = rotateVector(vert.normal, normalsRotateVec);
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
	Array<DSBone>			bones{ PP_SL };
	Array<VertexWeightData> weightData{ PP_SL };

	Matrix4x4				transform{ identity4 };

	const ofbx::Mesh*		mesh{ nullptr };
	const ofbx::Geometry*	geom{ nullptr };

	const ofbx::Skin*		skin{ nullptr };

};

static void GetFBXBonesAsDSM(const ofbx::Mesh& mesh, Array<DSBone>& bones, Array<VertexWeightData>& weightData, const ofbx::GlobalSettings& settings, const Matrix4x4& transform, const Matrix3x3& convertMatrix)
{
	const ofbx::Geometry& geom = *mesh.getGeometry();
	const ofbx::GeometryData& geomData = geom.getGeometryData();
	const ofbx::Skin* skin = geom.getSkin();
	const ofbx::Pose& pose = *mesh.getPose();

	if (!skin)
		return;

	const int weightDataStart = weightData.numElem();

	const float matDet = settings.CoordAxis == ofbx::CoordinateAxis::NEGATIVE_X ? -1.0f : 1.0f;
	const Matrix4x4 poseMatrix = FromFBXMatrix(pose.getMatrix());

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
		const ofbx::DMatrix fbxBoneMat = fbxCluster.getTransformLinkMatrix(); 

		// apply mesh transform alongside with FBX conversion matrix
		const Matrix4x4 boneTransform = FromFBXMatrix(fbxBoneMat);
		boneMatries.append(boneTransform);

		VertexWeightData& wd = weightData.append();
		wd.boneId = i;
		wd.sourceBone = fbxBoneLink;
		wd.sourceCluster = &fbxCluster;

		const int* indices = fbxCluster.getIndices();
		const double* weights = fbxCluster.getWeights();
		ASSERT(fbxCluster.getWeightsCount() == fbxCluster.getIndicesCount());

		for (int k = 0; k < fbxCluster.getIndicesCount(); ++k)
			wd.indexWeightMap.insert(indices[k], weights[k]);
	}

	// add bones
	ArrayRef<VertexWeightData> thisWeightData(weightData.ptr() + weightDataStart, weightData.numElem() - weightDataStart);
	for (int i = 0; i < thisWeightData.numElem(); ++i)
	{
		VertexWeightData& wd = thisWeightData[i];

		const ofbx::Cluster& fbxCluster = *wd.sourceCluster;
		const ofbx::Object* fbxBoneLink = wd.sourceBone;

		DSBone& dsBone = bones.append();
		dsBone.name = fbxCluster.name;
		dsBone.boneIdx = i;

		// find parent bone link
		for (int j = 0; j < thisWeightData.numElem(); ++j)
		{
			const ofbx::Cluster& fbxCluserJ = *thisWeightData[j].sourceCluster;
			const ofbx::Object* fbxBoneLinkJ = fbxCluserJ.getLink();

			if (fbxBoneLinkJ == fbxBoneLink->getParent())
			{
				dsBone.parentName = fbxBoneLinkJ->name;
				dsBone.parentIdx = j;
				break;
			}
		}

		// those bones stored as they are for later use
		// to convert animation
		{
			Matrix4x4 boneMatrix = boneMatries[i];
			if (dsBone.parentIdx != -1)
				boneMatrix = boneMatrix * !boneMatries[dsBone.parentIdx];
			wd.boneMatrix = boneMatrix;
		}

		// FIXME: WTF m8, this does seem to work
		Matrix4x4 boneMatrix = boneMatries[i] * identity4 * !poseMatrix;
		if (dsBone.parentIdx != -1)
			boneMatrix = boneMatrix * !(boneMatries[dsBone.parentIdx] * identity4 * !poseMatrix);
		else
			boneMatrix = boneMatrix * transform;

		// in Eq each bone transform is strictly related to it's parent
		{
			dsBone.position = boneMatrix.getTranslationComponent();
			dsBone.angles = EulerMatrixXYZ(boneMatrix.getRotationComponent());

			if (matDet < 0)
			{
				dsBone.angles *= -Vector3D(sign(matDet), 1.0f, 1.0f);
				dsBone.position *= Vector3D(sign(matDet), 1.0f, 1.0f);
			}
		}		
	}
}

static void ConvertFBXMeshToDSM(int meshId, DSModel& model, DSShapeData* shapeData, Map<int, DSMesh*>& geomSkins, const ofbx::Mesh& mesh, const ofbx::GlobalSettings& settings, bool invertFaces, const Matrix4x4& transform, const Matrix3x3& convertMatrix)
{
	MsgInfo("Mesh '%s'\n", mesh.name);
	
	Array<VertexWeightData> weightData(PP_SL);
	GetFBXBonesAsDSM(mesh, model.bones, weightData, settings, transform, convertMatrix);

	const ofbx::Geometry& geom = *mesh.getGeometry();
	const ofbx::GeometryData& geomData = geom.getGeometryData();

	ArrayCRef<int> materials(geomData.getMaterialMap(), geomData.getMaterialMapSize());
	ofbx::Vec3Attributes vertices = geomData.getPositions();
	ofbx::Vec2Attributes uvs = geomData.getUVs();	// TODO: multiple UV channels support (under s_uvs_max)
	ofbx::Vec4Attributes colors = geomData.getColors();
	ofbx::Vec3Attributes normals = geomData.getNormals();
	//ofbx::Vec3Attributes tangents = geomData.getTangents();

	Array<int> vertIndices(PP_SL);
	Array<int> tempIndices(PP_SL);
	Array<float> tempWeights(PP_SL);
	Array<int> tempWeightBones(PP_SL);
	for(int partIdx = 0; partIdx < geomData.getPartitionCount(); ++partIdx)
	{
		ofbx::GeometryPartition geomPart = geomData.getPartition(partIdx);
		vertIndices.setNum(geomPart.triangles_count * 3);
		for(int polyIdx = 0; polyIdx < geomPart.polygon_count; ++polyIdx)
		{
			const ofbx::GeometryPartition::Polygon& polygon = geomPart.polygons[polyIdx];
			tempIndices.reserve(polygon.vertex_count);

			const int indexCount = ofbx::triangulate(geomData, polygon, vertIndices.ptr(), tempIndices.ptr());

			// walk triangles
			for (int vertIdx = 0; vertIdx < indexCount; vertIdx += 3)
			{
				const int skinIdx = meshId | (partIdx << 16);
				auto skinMeshIt = geomSkins.find(skinIdx);
				if (skinMeshIt.atEnd())
				{
					DSMesh* dsmGrp = PPNew DSMesh();

					const ofbx::Material* material = mesh.getMaterialCount() > 0 ? mesh.getMaterial(partIdx) : nullptr;
					if (material)
						dsmGrp->texture = material->name;

					skinMeshIt = geomSkins.insert(skinIdx, dsmGrp);
					model.meshes.append(dsmGrp);
				}

				for (int k = 0; k < 3; ++k)
				{
					const int idx = vertIndices[vertIdx + (invertFaces ? 2 - k : k)];

					DSVertex& vert = skinMeshIt.value()->verts.append();
					vert.position = FromFBXVector(vertices.get(idx), settings);

					if (normals.values)
						vert.normal = FromFBXVector(normals.get(idx), settings);
					if (uvs.values)
						vert.texcoord = FromFBXUvVector(uvs.get(idx));
					if (colors.values)
						vert.color = FromFBXColorVector(colors.get(idx));

					vert.vertexId = vertices.indices ? vertices.indices[idx] : idx;

					ASSERT(length(vert.position) < F_INFINITY);
					ASSERT(vecIsFinite(vert.position) && vecIsValid(vert.position));
					ASSERT(vecIsValid(vert.normal));
					ASSERT(vecIsValid(vert.texcoord));

					tempWeights.clear();
					tempWeightBones.clear();

					// Apply bone weights to vertex
					for (const VertexWeightData& wd : weightData)
					{
						auto it = wd.indexWeightMap.find(vertices.indices ? vertices.indices[idx] : idx);
						if (it.atEnd())
							continue;

						tempWeights.append(*it);
						tempWeightBones.append(wd.boneId);
					}

					const int numNewWeights = SortAndBalanceBones(tempWeights.numElem(), MAX_MODEL_VERTEX_WEIGHTS, tempWeightBones.ptr(), tempWeights.ptr());
					vert.weights.setNum(numNewWeights);
					for (int w = 0; w < numNewWeights; w++)
					{
						DSWeight& weight = vert.weights[w];
						weight.bone = tempWeightBones[w];
						weight.weight = tempWeights[w];
					}
				}
			}
		}
	}

	// get blend shapes
	const ofbx::BlendShape* blendShape = geom.getBlendShape();
	if (blendShape && shapeData)
	{
		const int numBlendShapeChannels = blendShape->getBlendShapeChannelCount();
		Msg("    %d blend shape channels\n", numBlendShapeChannels);
		for (int i = 0; i < numBlendShapeChannels; ++i)
		{
			const ofbx::BlendShapeChannel* blendShapeChan = blendShape->getBlendShapeChannel(i);
			const float deformPercent = blendShapeChan->getDeformPercent();
			const int numBlendShapes = blendShapeChan->getShapeCount();
			Msg("\t %d blend shapes:", numBlendShapes);
			for (int j = 0; j < numBlendShapes; ++j)
			{
				const ofbx::Shape* shape = blendShapeChan->getShape(j);
				MsgInfo(" %s", shape->name);

				DSShapeKey* shapeKey = PPNew DSShapeKey();
				shapeKey->name = shape->name;
				shapeKey->verts.reserve(shape->getVertexCount());

				shapeData->shapes.append(shapeKey);

				const ofbx::Vec3* shapeVertices = shape->getVertices();
				const ofbx::Vec3* shapeNormals = shape->getNormals();
				const int* shapeIndices = shape->getIndices();
				const int indexCount = shape->getIndexCount();

				const int vertCount = shape->getVertexCount();
				for (int vertId = 0; vertId < vertCount; ++vertId)
				{
					// NOTE: shape key positions and normals ARE RELATIVE
					DSShapeVert& shapeVert = shapeKey->verts.append();
					shapeVert.vertexId = shapeIndices[vertId];
					shapeVert.position = FromFBXVector(shapeVertices[vertId], settings);
					shapeVert.normal = FromFBXVector(shapeNormals[vertId], settings);
				}
			}
			Msg("\n");
		}
	}

	TransformModelGeom(model, transform);

	if(shapeData)
		TransformShapeDataGeom(shapeData, transform);
}

bool LoadFBX(Array<DSModelContainer>& modelContainerList, const char* filename)
{
	VSSize fileSize = 0;
	ubyte* fileBuffer = g_fileSystem->GetFileBuffer(filename, &fileSize);
	if (!fileBuffer)
	{
		MsgError("Couldn't open FBX file '%s'\n", filename);
		return false;
	}

	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, (ofbx::u64)ofbx::LoadFlags::KEEP_MATERIAL_MAP);

	if (!scene)
	{
		MsgError("FBX '%s' error: %s\n", filename, ofbx::getError());
		PPFree(fileBuffer);
		return false;
	}

	{
		const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

		static const char* axesNames[] = {
			"X",
			"-X",
			"Y",
			"-Y",
			"Z",
			"-Z",
			"Unknown"
		};
		MsgInfo("  FBX Forward: %s\n", axesNames[static_cast<int>(settings.FrontAxis)]);
		MsgInfo("  FBX Up: %s\n", axesNames[static_cast<int>(settings.UpAxis)]);
		MsgInfo("  Original Up: %s\n", axesNames[static_cast<int>(settings.OriginalUpAxis)]);

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

			Map<int, DSMesh*> materialGroups(PP_SL);
			ConvertFBXMeshToDSM(i, *container.model, container.shapeData, materialGroups, mesh, settings, invertFaces, transform, convertMatrix);

			container.model->name = mesh.name;
			container.shapeData->reference = mesh.name;
			container.transform = transform;

			if (container.shapeData->shapes.isEmpty())
				container.shapeData = nullptr;
		}
	}

	PPFree(fileBuffer);
	return true;
}

// Editor variant
bool LoadFBXCompound( DSModel& model, const char* filename )
{
	VSSize fileSize = 0;
	char* fileBuffer = (char*)g_fileSystem->GetFileBuffer(filename, &fileSize);

	if (!fileBuffer)
	{
		MsgError("Couldn't open FBX file '%s'", filename);
		return false;
	}

	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, (ofbx::u64)ofbx::LoadFlags::KEEP_MATERIAL_MAP);

	if (!scene)
	{
		MsgError("FBX '%s' error: %s\n", filename, ofbx::getError());
		PPFree(fileBuffer);
		return false;
	}

	{
		const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

		Matrix3x3 convertMatrix;
		bool invertFaces;
		GetFBXConvertMatrix(settings, convertMatrix, invertFaces);

		Map<int, DSMesh*> materialGroups(PP_SL);

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
	VSSize fileSize = 0;
	char* fileBuffer = (char*)g_fileSystem->GetFileBuffer(filename, &fileSize);

	if (!fileBuffer)
	{
		MsgError("Couldn't open FBX file '%s'", filename);
		return false;
	}

	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, (ofbx::u64)ofbx::LoadFlags::KEEP_MATERIAL_MAP);

	if (!scene)
	{
		MsgError("FBX '%s' error: %s\n", filename, ofbx::getError());
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

		Map<int, DSMesh*> materialGroups(PP_SL);

		const int meshCount = scene->getMeshCount();
		for (int i = 0; i < meshCount; ++i)
		{
			const ofbx::Mesh& mesh = *scene->getMesh(i);

			// this is used to transform mesh from FBX space
			const Matrix4x4 globalTransform = FromFBXMatrix(mesh.getGlobalTransform());
			const Matrix4x4 geomMatrix = FromFBXMatrix(mesh.getGeometricMatrix());
			const Matrix4x4 transform = globalTransform * geomMatrix * Matrix4x4(convertMatrix);

			ConvertFBXMeshToDSM(i, *modelContainer.model, modelContainer.shapeData, materialGroups, mesh, settings, invertFaces, transform, convertMatrix);
		}
	}

	PPFree(fileBuffer);

	return true;
}

//-------------------------------------------------------------

enum EInterpType : int
{
	INTERP_POSITION = 0,
	INTERP_ANGLES,
};

template<typename T, EInterpType TYPE>
T InterpVec(const Array<T>& src, float x, int n);

template<>
Vector3D InterpVec<Vector3D, INTERP_POSITION>(const Array<Vector3D>& src, float x, int n)
{
	if (x <= 0)
		return src[0];

	if (x >= n - 1)
		return src[n - 1];

	const int j = int(x);
	return lerp(src[j], src[j + 1], x - float(j));
}

template<>
Vector3D InterpVec<Vector3D, INTERP_ANGLES>(const Array<Vector3D>& src, float x, int n)
{
	if (x <= 0)
		return src[0];

	if (x >= n - 1)
		return src[n - 1];

	const int j = int(x);
	Quaternion a1 = rotateXYZ(DEG2RAD(src[j].x), DEG2RAD(src[j].y), DEG2RAD(src[j].z));
	Quaternion a2 = rotateXYZ(DEG2RAD(src[j+1].x), DEG2RAD(src[j+1].y), DEG2RAD(src[j+1].z));
	Quaternion r = slerp(a1, a2, x - float(j));
	return eulersXYZ(r) * M_RAD2DEG;
}

template<class T, EInterpType TYPE>
void ZoomArray(const Array<T>& src, Array<T>& dest, int newLength)
{
	dest.setNum(newLength);
	const int oldLength = src.numElem();
	const float step = float(oldLength - 1) / (newLength - 1);

	for (int j = 0; j < newLength; ++j)
		dest[j] = InterpVec<T, TYPE>(src, float(j) * step, oldLength);
}

void GetFBXCurveAsInterpKeyFrames(const ofbx::AnimationCurveNode* curveNode, Array<Vector3D>& keyFrames, int animationDuration, float localDuration, bool isAngles)
{
	const ofbx::AnimationCurve* nodeX = curveNode->getCurve(0);
	const ofbx::AnimationCurve* nodeY = curveNode->getCurve(1);
	const ofbx::AnimationCurve* nodeZ = curveNode->getCurve(2);

	if (!nodeX || !nodeY || !nodeZ)
	{
		MsgWarning("GetFBXCurveAsInterpKeyFrames error - not enough curves\n");
	}

	Map<ofbx::i64, float> valueX(PP_SL);
	Map<ofbx::i64, float> valueY(PP_SL);
	Map<ofbx::i64, float> valueZ(PP_SL);

	Set<ofbx::i64> allTimes(PP_SL);

	int maxFrameCount = 0;

	auto insertFrames = [&](const ofbx::AnimationCurve* curve, Map<ofbx::i64, float>& destVal)
	{
		if (!curve)
		{
			destVal[0] = 0.0f;
			allTimes.insert(0);
			return;
		}
		const ofbx::i64* times = curve->getKeyTime();
		const float* values = curve->getKeyValue();
		const int keyCount = curve->getKeyCount();

		maxFrameCount = max(maxFrameCount, keyCount);

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

	if(isAngles)
		ZoomArray<Vector3D, INTERP_ANGLES>(intermediateKeyFrames, keyFrames, max(animationDuration, 2));
	else
		ZoomArray<Vector3D, INTERP_POSITION>(intermediateKeyFrames, keyFrames, max(animationDuration, 2));
}

void CollectFBXAnimations(Array<DSAnimData>& animations, ofbx::IScene* scene, const char* meshFilter)
{
	const ofbx::GlobalSettings& settings = *scene->getGlobalSettings();

	Matrix3x3 convertMatrix;
	bool invertFaces;
	GetFBXConvertMatrix(settings, convertMatrix, invertFaces);
	const float matDet = settings.CoordAxis == ofbx::CoordinateAxis::NEGATIVE_X ? -1.0f : 1.0f;

	// get bones from all meshes
	Array<ObjectData> objectDatas(PP_SL);

	// TODO: separate skeletons from each mesh ref
	const int meshCount = scene->getMeshCount();
	for (int i = 0; i < meshCount; ++i)
	{
		const ofbx::Mesh& mesh = *scene->getMesh(i);

		if (CString::CompareCaseIns(mesh.name, meshFilter) != 0)
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
		GetFBXBonesAsDSM(mesh, objData.bones, objData.weightData, settings, transform, convertMatrix);
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

		const int animationDuration = int(localDuration * frameRate + 0.5f);
		const ofbx::AnimationLayer* layer = stack->getLayer(0);

		for (ObjectData& objData : objectDatas)
		{
			if (objData.bones.isEmpty())
				continue;

			const ofbx::Object& skeletonObj = *objData.weightData[0].sourceBone->getParent();
			const Matrix4x4 meshTransform = FromFBXMatrix(objData.mesh->getGlobalTransform());

			AnimationTRS rootAnimation;
			{
				const ofbx::AnimationCurveNode* rootTranslationNode = layer->getCurveNode(skeletonObj, "Lcl Translation");
				const ofbx::AnimationCurveNode* rootRotationNode = layer->getCurveNode(skeletonObj, "Lcl Rotation");
				const ofbx::AnimationCurveNode* rootScalingNode = layer->getCurveNode(skeletonObj, "Lcl Scaling");

				// keyframes are going to be interpolated and resampled in order to restore original keyframing
				if (rootTranslationNode)
					GetFBXCurveAsInterpKeyFrames(rootTranslationNode, rootAnimation.translations, animationDuration, localDuration, false);
				if (rootRotationNode)
					GetFBXCurveAsInterpKeyFrames(rootRotationNode, rootAnimation.rotations, animationDuration, localDuration, true);
				if (rootScalingNode)
					GetFBXCurveAsInterpKeyFrames(rootScalingNode, rootAnimation.scales, animationDuration, localDuration, false);
			}

			const int boneCount = objData.weightData.numElem();

			DSAnimData animation;
			animation.name = stack->name;
			animation.bones = PPNew DSBoneFrames[boneCount];

			// convert bone animation
			for (int j = 0; j < boneCount; ++j)
			{
				const DSBone& bone = objData.bones[j];
				const VertexWeightData& wd = objData.weightData[j];

				// root animation
				// FBX does not apply armature transform animation to the root bone
				// so we have to do it ourselves
				Matrix4x4 invBoneMatrix = wd.boneMatrix;
				if (bone.parentIdx == -1)
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
						GetFBXCurveAsInterpKeyFrames(translationNode, boneAnimation.translations, animationDuration, localDuration, false);
					if (rotationNode)
						GetFBXCurveAsInterpKeyFrames(rotationNode, boneAnimation.rotations, animationDuration, localDuration, true);
					if (scalingNode)
						GetFBXCurveAsInterpKeyFrames(scalingNode, boneAnimation.scales, animationDuration, localDuration, false);
				}

				ASSERT_MSG(boneAnimation.translations.numElem() <= MaxFramesPerAnimation, "Too many frames in animation (%d, limit is %d)", boneAnimation.translations.numElem(), MaxFramesPerAnimation);
				ASSERT_MSG(boneAnimation.translations.numElem() == boneAnimation.rotations.numElem(), "Rotations %d translations %d", boneAnimation.rotations.numElem(), boneAnimation.translations.numElem());
				ASSERT_MSG(rootAnimation.translations.numElem() == boneAnimation.translations.numElem(), "Root and bone animation frame mismatch");

				if (!boneAnimation.translations.numElem() && !boneAnimation.rotations.numElem())
					continue;

				// alloc frames
				const int numFrames = boneAnimation.translations.numElem();
			
				DSBoneFrames& outBoneAnim = animation.bones[j];
				outBoneAnim.numFrames = numFrames;
				outBoneAnim.keyFrames = PPNew DSAnimFrame[numFrames];
			
				// perform conversion of each frame to local space
				for (int k = 0; k < numFrames; ++k)
				{
					DSAnimFrame& outFrame = outBoneAnim.keyFrames[k];

					const Vector3D rotation = boneAnimation.rotations[k];
					const Vector3D translation = boneAnimation.translations[k];

					const ofbx::DVec3 translationFrame{translation.x, translation.y, translation.z};
					const ofbx::DVec3 rotationFrame{rotation.x, rotation.y, rotation.z};

					Matrix4x4 meshAnimTransform = identity4;
					if (bone.parentIdx == -1)
					{
						const Vector3D rotation = rootAnimation.rotations[k];
						const Vector3D translation = rootAnimation.translations[k];

						const ofbx::DVec3 translationFrame{translation.x, translation.y, translation.z};
						const ofbx::DVec3 rotationFrame{rotation.x, rotation.y, rotation.z};

						// we need to apply this parent transform accordingly
						meshAnimTransform = FromFBXMatrix(skeletonObj.evalLocal(translationFrame, rotationFrame)) * meshTransform;
					}

					const Matrix4x4 animBoneMatrix = FromFBXMatrix(wd.sourceBone->evalLocal(translationFrame, rotationFrame)) * meshAnimTransform * invBoneMatrix;

					// in Eq each bone transform is strictly related to it's parent
					{
						outFrame.position = animBoneMatrix.getTranslationComponent();
						outFrame.angles = EulerMatrixXYZ(animBoneMatrix.getRotationComponent());

						if (matDet < 0)
						{
							outFrame.angles *= -Vector3D(sign(matDet), 1.0f, 1.0f);
							outFrame.position *= Vector3D(sign(matDet), 1.0f, 1.0f);
						}
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
				animation.bones = nullptr;
			}
		}
	}
}

bool LoadFBXAnimations(Array<DSAnimData>& animations, const char* filename, const char* meshFilter)
{
	VSSize fileSize = 0;
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

	ofbx::IScene* scene = ofbx::load((ofbx::u8*)fileBuffer, fileSize, (ofbx::u64)ofbx::LoadFlags::KEEP_MATERIAL_MAP);

	if (!scene)
	{
		MsgError("FBX '%s' error: %s\n", filename, ofbx::getError());
		return false;
	}

	CollectFBXAnimations(animations, scene, meshFilter);

	return true;
}

} // namespace