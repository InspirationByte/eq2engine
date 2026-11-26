#include <gtest/gtest.h>

#include "core/core_common.h"
#include "ds/MemoryStream.h"
#include "utils/KeyValues.h"
#include "blink/BlinkMesh.h"

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
