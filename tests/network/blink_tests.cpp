#include <gtest/gtest.h>

#include "core/core_common.h"
#include "ds/MemoryStream.h"
#include "utils/KeyValues.h"
#include "blink/BlinkMesh.h"
#include "blink/BlinkStudio.h"
#include "egf/model.h"

// two batches, a quad and a triangle, with colors
static BlinkMesh BlinkTestMesh()
{
	BlinkMesh mesh;
	mesh.BeginBatch("mat_a");
	for (int i = 0; i < 4; ++i)
	{
		mesh.positions.append(Vector3D(static_cast<float>(i & 1), 0.0f, static_cast<float>(i >> 1)));
		mesh.normals.append(Vector3D(0.0f, 1.0f, 0.0f));
		mesh.texcoords.append(Vector2D(static_cast<float>(i & 1), static_cast<float>(i >> 1)));
		mesh.colors.append(TVec4D<ubyte>(static_cast<ubyte>(i * 10), 255, 0, 7));
	}
	for (const int index : { 0, 1, 2, 2, 1, 3 })
		mesh.indices.append(index);

	mesh.BeginBatch("mat_b");
	for (const int index : { 0, 2, 3 })
		mesh.indices.append(index);
	return mesh;
}

static CRefPtr<CMemoryStream> BlinkTestWrite(const BlinkMesh& mesh, KVSection& params)
{
	CRefPtr<CMemoryStream> payload = CRefPtr_new(CMemoryStream);
	payload->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	mesh.Write(params, *payload);
	payload->Seek(0, FS_SEEK_SET);
	return payload;
}

TEST(BLINK_MESH_TESTS, WriteRead)
{
	const BlinkMesh mesh = BlinkTestMesh();
	KVSection params;
	CRefPtr<CMemoryStream> payload = BlinkTestWrite(mesh, params);

	BlinkMesh read;
	EqString error;
	ASSERT_TRUE(read.Read(params, *payload, error)) << error.ToCString();

	ASSERT_EQ(read.positions.numElem(), 4);
	ASSERT_EQ(read.indices.numElem(), 9);
	ASSERT_EQ(read.colors.numElem(), 4);
	EXPECT_EQ(read.positions[3], mesh.positions[3]);
	EXPECT_EQ(read.texcoords[1], mesh.texcoords[1]);
	EXPECT_EQ(read.colors[2].x, 20);
	ASSERT_EQ(read.batches.numElem(), 2);
	EXPECT_STREQ(read.batches[1].material.ToCString(), "mat_b");
	EXPECT_EQ(read.batches[1].firstIndex, 6);
}

TEST(BLINK_MESH_TESTS, ReadWithoutColors)
{
	BlinkMesh mesh = BlinkTestMesh();
	mesh.colors.clear();

	// a payload of the four arrays only, as an older writer sends it
	KVSection params;
	CRefPtr<CMemoryStream> full = BlinkTestWrite(mesh, params);
	CRefPtr<CMemoryStream> payload = CRefPtr_new(CMemoryStream);
	payload->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	payload->Write(full->GetBasePointer(), 1, full->GetSize() - sizeof(int));	// without the empty colors array
	payload->Seek(0, FS_SEEK_SET);

	BlinkMesh read;
	EqString error;
	ASSERT_TRUE(read.Read(params, *payload, error)) << error.ToCString();
	EXPECT_EQ(read.colors.numElem(), 0);
}

TEST(BLINK_MESH_TESTS, ReadRejectsBadIndex)
{
	BlinkMesh mesh = BlinkTestMesh();
	mesh.indices[4] = 4;

	KVSection params;
	CRefPtr<CMemoryStream> payload = BlinkTestWrite(mesh, params);

	BlinkMesh read;
	EqString error;
	EXPECT_FALSE(read.Read(params, *payload, error));
	EXPECT_GT(error.Length(), 0);
}

TEST(BLINK_MESH_TESTS, ReadRejectsBadBatches)
{
	const BlinkMesh mesh = BlinkTestMesh();
	KVSection params;
	CRefPtr<CMemoryStream> payload = BlinkTestWrite(mesh, params);

	// the batches do not cover the indices
	params.RemoveSectionByName("batch");
	KVSection& batch = params.CreateSection("batch");
	batch.SetKey("material", "mat_a");
	batch.SetKey("firstIndex", 0);
	batch.SetKey("indexCount", 6);

	BlinkMesh read;
	EqString error;
	EXPECT_FALSE(read.Read(params, *payload, error));
}

TEST(BLINK_MESH_TESTS, ReadRejectsShortPayload)
{
	const BlinkMesh mesh = BlinkTestMesh();
	KVSection params;
	CRefPtr<CMemoryStream> full = BlinkTestWrite(mesh, params);

	// the positions count says more than there is
	CRefPtr<CMemoryStream> payload = CRefPtr_new(CMemoryStream);
	payload->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	const int hugeCount = 1 << 28;
	payload->Write(&hugeCount, 1, sizeof(hugeCount));
	payload->Write(full->GetBasePointer() + sizeof(int), 1, 16);
	payload->Seek(0, FS_SEEK_SET);

	BlinkMesh read;
	EqString error;
	EXPECT_FALSE(read.Read(params, *payload, error));
	EXPECT_EQ(read.positions.numElem(), 0);
}

// two body groups: a strip with a degenerate end and no normals, a triangle list
struct BlinkTestStudio
{
	studioHdr_t				hdr;
	studioBodyGroup_t		bodyGroups[2];
	studioLodModel_t		lodModels[2];
	studioMeshGroupDesc_t	meshGroups[2];
	studioMeshDesc_t		meshes[2];
	studioMaterialDesc_t	materials[1];
	studioVertexPosUv_t		vertices[4];
	uint32					stripIndices[6];
	uint32					listIndices[3];
};

static int BlinkTestOffset(const void* from, const void* to)
{
	return static_cast<int>(reinterpret_cast<const ubyte*>(to) - reinterpret_cast<const ubyte*>(from));
}

static void BlinkTestMakeStudio(BlinkTestStudio& studio)
{
	memset(&studio, 0, sizeof(studio));
	studioHdr_t& hdr = studio.hdr;
	hdr.numBodyGroups = 2;
	hdr.bodyGroupsOffset = BlinkTestOffset(&hdr, studio.bodyGroups);
	hdr.numLods = 2;
	hdr.lodsOffset = BlinkTestOffset(&hdr, studio.lodModels);
	hdr.numMeshGroups = 2;
	hdr.meshGroupsOffset = BlinkTestOffset(&hdr, studio.meshGroups);
	hdr.numMaterials = 1;
	hdr.materialsOffset = BlinkTestOffset(&hdr, studio.materials);
	strcpy(studio.materials[0].materialname, "mat_egf");

	for (int i = 0; i < 2; ++i)
	{
		studio.bodyGroups[i].lodModelIndex = i;
		memset(studio.lodModels[i].meshIndexes, EGF_INVALID_IDX, sizeof(studio.lodModels[i].meshIndexes));
		studio.lodModels[i].meshIndexes[0] = i;		// only LOD 0

		studio.meshGroups[i].numMeshes = 1;
		studio.meshGroups[i].meshesOffset = BlinkTestOffset(&studio.meshGroups[i], &studio.meshes[i]);

		studioMeshDesc_t& mesh = studio.meshes[i];
		mesh.materialIndex = 0;
		mesh.vertexType = STUDIO_VERTFLAG_POS_UV;
		mesh.numVertices = i == 0 ? 4 : 3;
		mesh.vertexOffset = BlinkTestOffset(&mesh, studio.vertices);
	}

	for (int i = 0; i < 4; ++i)
	{
		studio.vertices[i].point = Vector3D(static_cast<float>(i & 1), 0.0f, static_cast<float>(i >> 1));
		studio.vertices[i].texCoord = Vector2D(static_cast<float>(i), 0.0f);
	}

	const uint32 stripIndices[] = { 0, 1, 2, 3, 3, 0 };
	memcpy(studio.stripIndices, stripIndices, sizeof(stripIndices));
	studio.meshes[0].primitiveType = STUDIO_PRIM_TRI_STRIP;
	studio.meshes[0].numIndices = 6;
	studio.meshes[0].indicesOffset = BlinkTestOffset(&studio.meshes[0], studio.stripIndices);

	const uint32 listIndices[] = { 0, 1, 2 };
	memcpy(studio.listIndices, listIndices, sizeof(listIndices));
	studio.meshes[1].primitiveType = STUDIO_PRIM_TRIANGLES;
	studio.meshes[1].numIndices = 3;
	studio.meshes[1].indicesOffset = BlinkTestOffset(&studio.meshes[1], studio.listIndices);
}

TEST(BLINK_MESH_TESTS, StudioMesh)
{
	BlinkTestStudio studio;
	BlinkTestMakeStudio(studio);

	// the first body group, LOD 2 falls back to LOD 0
	BlinkMesh mesh;
	BlinkMakeStudioMesh(studio.hdr, nullptr, 1, 2, mesh);
	ASSERT_EQ(mesh.positions.numElem(), 4);
	ASSERT_EQ(mesh.normals.numElem(), 4);
	EXPECT_EQ(mesh.normals[0], vec3_zero);
	EXPECT_EQ(mesh.texcoords[3], Vector2D(3.0f, 0.0f));
	const int stripTriangles[] = { 0, 1, 2, 3, 2, 1 };		// the second one flipped, the degenerate ones skipped
	ASSERT_EQ(mesh.indices.numElem(), 6);
	for (int i = 0; i < 6; ++i)
		EXPECT_EQ(mesh.indices[i], stripTriangles[i]);
	ASSERT_EQ(mesh.batches.numElem(), 1);
	EXPECT_STREQ(mesh.batches[0].material.ToCString(), "mat_egf");

	// both body groups: the list after the strip, its vertices after the strip ones
	BlinkMesh both;
	BlinkMakeStudioMesh(studio.hdr, nullptr, 3, 0, both);
	ASSERT_EQ(both.positions.numElem(), 7);
	ASSERT_EQ(both.indices.numElem(), 9);
	EXPECT_EQ(both.indices[6], 4);
	EXPECT_EQ(both.indices[8], 6);
	ASSERT_EQ(both.batches.numElem(), 2);
	EXPECT_EQ(both.batches[1].firstIndex, 6);

	// the mesh is valid for BlinkMesh::Read
	KVSection params;
	CRefPtr<CMemoryStream> payload = BlinkTestWrite(both, params);
	BlinkMesh read;
	EqString error;
	EXPECT_TRUE(read.Read(params, *payload, error)) << error.ToCString();
}
