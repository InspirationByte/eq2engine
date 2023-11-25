//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Studio Geometry Form
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

class IVertexFormat;
class IVertexBuffer;
class IIndexBuffer;
class CBaseEqGeomInstancer;
class IGPURenderPassRecorder;
struct MeshInstanceData;
struct RenderDrawCmd;
struct DecalMakeInfo;
struct DecalData;
struct VertexLayoutDesc;

// egf model hardware vertex
struct EGFHwVertex
{
	enum VertexStreamId : int
	{
		VERT_POS_UV,
		VERT_TBN,
		VERT_BONEWEIGHT,
		VERT_COLOR,

		// TODO: more UVs ?

		VERT_COUNT,
	};

	struct PositionUV
	{
		static const VertexLayoutDesc& GetVertexLayoutDesc();

		PositionUV() = default;
		PositionUV(const studioVertexPosUv_t& initFrom);

		TVec4D<half>	pos;
		TVec2D<half>	texcoord;
	};

	struct TBN
	{
		static const VertexLayoutDesc& GetVertexLayoutDesc();

		TBN() = default;
		TBN(const studioVertexTBN_t& initFrom);

		TVec3D<half>	tangent;
		half			unused1;	// half float types are unsupported with v3d, turn them into v4d
		TVec3D<half>	binormal;
		half			unused2;
		TVec3D<half>	normal;
		half			unused3;
	};

	struct BoneWeights
	{
		static const VertexLayoutDesc& GetVertexLayoutDesc();

		BoneWeights();
		BoneWeights(const studioBoneWeight_t& initFrom);

		uint8			boneIndices[MAX_MODEL_VERTEX_WEIGHTS];
		half			boneWeights[MAX_MODEL_VERTEX_WEIGHTS];
	};

	struct Color
	{
		Color() = default;
		Color(const studioVertexColor_t& initFrom);

		static const VertexLayoutDesc& GetVertexLayoutDesc();
		uint			color{ color_white.pack() };
	};
};

enum EModelLoadingState
{
	MODEL_LOAD_ERROR = -1,
	MODEL_LOAD_IN_PROGRESS = 0,
	MODEL_LOAD_OK,
};

#define EGF_LOADING_CRITICAL_SECTION(m)	\
	while(m->GetLoadingState() != MODEL_LOAD_OK) {	g_parallelJobs->CompleteJobCallbacks(); Platform_Sleep(1); }

// streams in studio models used exclusively in interpolation
class CEqStudioGeom
{
	friend class CStudioCache;
	friend class CBaseEqGeomInstancer;
public:

	struct DrawProps;

	CEqStudioGeom();
	~CEqStudioGeom();

	int							GetCacheIndex() const { return m_cacheIdx; }

	const char*					GetName() const;
	int							GetLoadingState() const;	// EModelLoadingState
	void						LoadMotionPackage(const char* filename);

	int							GetMotionPackageCount() const { return m_motionData.numElem(); }
	int							GetMaterialCount() const { return m_materialCount; }
	int							GetMaterialGroupsCount() const { return m_materialGroupsCount; }

	const studioHdr_t&			GetStudioHdr() const;
	const studioPhysData_t&		GetPhysData() const;
	const studioMotionData_t&	GetMotionData(int index) const;
	const studioJoint_t&		GetJoint(int index) const;
	Matrix4x4					GetLocalTransformMatrix(int transformIdx) const;

	const BoundingBox&			GetBoundingBox() const;

	// Makes dynamic temporary decal
	CRefPtr<DecalData>			MakeDecal(const DecalMakeInfo& info, Matrix4x4* jointMatrices, int bodyGroupFlags, int lod = 0) const;

	// Checks ray-egf intersection. Ray must be in local space
	float						CheckIntersectionWithRay(const Vector3D& rayStart, const Vector3D& rayDir, int bodyGroupFlags, int lod = 0) const;

	// instancing
	void						SetInstancer(CBaseEqGeomInstancer* instancer);
	CBaseEqGeomInstancer*		GetInstancer() const;

	// selects a lod. returns index
	int							SelectLod(float distance) const;
	int							FindManualLod(float value) const;

	void						Draw(const DrawProps& drawProperties, const MeshInstanceData& instData, IGPURenderPassRecorder* rendPassRecorder) const;

	IGPUBufferPtr				GetVertexBuffer(EGFHwVertex::VertexStreamId vertStream) const;
	const IMaterialPtr&			GetMaterial(int materialIdx, int materialGroupIdx = 0) const;

private:

	struct HWGeomRef
	{
		// offset in hw index buffer to this lod, for each geometry group
		struct Mesh
		{
			int		firstIndex{ 0 };
			int		indexCount{ 0 };
			ushort	primType{ 0 };
			bool	supportsSkinning{ false };
		} *meshRefs{ nullptr };
	};

	bool					LoadModel(const char* pszPath, bool useJob = true);
	void					DestroyModel();

	static void				LoadModelJob(void* data, int i);
	static void				LoadVertsJob(void* data, int i);
	static void				LoadPhysicsJob(void* data, int i);
	static void				LoadMotionJob(void* data, int i);
	
	static void				OnLoadingJobComplete(struct eqParallelJob_t* job);
	
	bool					LoadFromFile();
	void					LoadMaterials();
	void					LoadPhysicsData(); // loads physics object data
	bool					LoadGenerateVertexBuffer();
	void					LoadMotionPackages();
	void					LoadSetupBones();

	//-----------------------------------------------

	// array of material index for each group
	FixedArray<IMaterialPtr, MAX_STUDIOMATERIALS>		m_materials;
	FixedArray<studioMotionData_t*, MAX_MOTIONPACKAGES>	m_motionData;

	Array<EqString>			m_additionalMotionPackages{ PP_SL };
	BoundingBox				m_boundingBox; // FIXME: bounding boxes for each groups?
	EqString				m_name;
	
	studioJoint_t*			m_joints{ nullptr };
	HWGeomRef*				m_hwGeomRefs{ nullptr };	// hardware representation of models (indices)

	CBaseEqGeomInstancer*	m_instancer{ nullptr };
	studioHdr_t*			m_studio{ nullptr };
	studioPhysData_t		m_physModel;

	IGPUBufferPtr			m_vertexBuffers[EGFHwVertex::VERT_COUNT];
	IGPUBufferPtr			m_indexBuffer;
	int						m_indexFmt{ -1 };

	int						m_materialCount{ 0 };
	int						m_materialGroupsCount{ 0 };

	int						m_cacheIdx{ -1 };

	mutable int				m_loading{ MODEL_LOAD_ERROR };
	mutable int				m_readyState{ 0 };
};

extern ArrayCRef<EGFHwVertex::VertexStreamId> g_defaultVertexStreamMapping;

struct CEqStudioGeom::DrawProps
{
	using SetupDrawFunc = EqFunction<void(RenderDrawCmd& drawCmd)>;
	using BodyGroupFunc = EqFunction<void(RenderDrawCmd& drawCmd, IMaterial* material, int bodyGroup, int meshIndex)>;

	ArrayCRef<EGFHwVertex::VertexStreamId> vertexStreamMapping{ g_defaultVertexStreamMapping };
	IVertexFormat*	vertexFormat{ nullptr };
	Matrix4x4*		boneTransforms{ nullptr };

	SetupDrawFunc	setupDrawCmd;		// called once before entire EGF is drawn
	BodyGroupFunc	setupBodyGroup;	// called multiple times before body group is drawn
	
	int				bodyGroupFlags{ -1 };
	int				materialGroup{ 0 };
	int				lod{ 0 };

	int				materialFlags{ -1 };
	bool			excludeMaterialFlags{ false };
	bool			skipMaterials{ false };
};