//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem allocation tracker tests
//////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include "core/core_common.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef PPMEM_DISABLED

TEST(PPMEM_TESTS, Disabled)
{
	GTEST_SKIP() << "PPMem is disabled in this configuration, run tests in Debug";
}

#else

static void FillPattern(void* mem, int size, int seed)
{
	ubyte* bytes = reinterpret_cast<ubyte*>(mem);
	for (int i = 0; i < size; ++i)
		bytes[i] = static_cast<ubyte>(seed + i);
}

static bool CheckPattern(const void* mem, int size, int seed)
{
	const ubyte* bytes = reinterpret_cast<const ubyte*>(mem);
	for (int i = 0; i < size; ++i)
	{
		if (bytes[i] != static_cast<ubyte>(seed + i))
			return false;
	}
	return true;
}

//-------------------------------------------------------------------------------------------------
// tracked allocations

TEST(PPMEM_TESTS, AllocFree)
{
	const size_t baseUsage = PPMemGetUsage();

	void* mem = PPDAlloc(100, PP_SL);
	ASSERT_NE(mem, nullptr);
	EXPECT_EQ(PPMemGetUsage(), baseUsage + 100);

	FillPattern(mem, 100, 1);
	PPDCheck(mem);
	EXPECT_TRUE(CheckPattern(mem, 100, 1));

	PPFree(mem);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, ZeroSizeAlloc)
{
	const size_t baseUsage = PPMemGetUsage();

	void* mem = PPDAlloc(0, PP_SL);
	ASSERT_NE(mem, nullptr);
	PPDCheck(mem);

	PPFree(mem);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, FreeNull)
{
	PPFree(nullptr);
	PPAlignedFree(nullptr);
	PPDCheck(nullptr);
}

TEST(PPMEM_TESTS, UntrackedAlloc)
{
	const size_t baseUsage = PPMemGetUsage();

	// empty source line skips tracking
	void* mem = PPDAlloc(100, PPSourceLine::Empty());
	ASSERT_NE(mem, nullptr);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);

	FillPattern(mem, 100, 2);
	EXPECT_TRUE(CheckPattern(mem, 100, 2));

	PPFree(mem);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, NewDelete)
{
	struct TestObject
	{
		int		values[8];
		float	scale{ 1.0f };
	};

	const size_t baseUsage = PPMemGetUsage();

	int* values = PPNew int[10];
	EXPECT_EQ(PPMemGetUsage(), baseUsage + 10 * sizeof(int));
	delete[] values;
	EXPECT_EQ(PPMemGetUsage(), baseUsage);

	TestObject* object = PPNew TestObject();
	EXPECT_EQ(PPMemGetUsage(), baseUsage + sizeof(TestObject));
	EXPECT_EQ(object->scale, 1.0f);
	delete object;
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

//-------------------------------------------------------------------------------------------------
// guard checkmarks

TEST(PPMEM_TESTS, OverflowDetectedByCheck)
{
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlloc(16, PP_SL));
	const ubyte savedByte = bytes[16];

	bytes[16] = static_cast<ubyte>(savedByte + 1);
	EXPECT_FATAL_FAILURE([&]() { PPDCheck(bytes); }, "buffer overflow");

	bytes[16] = savedByte;
	PPDCheck(bytes);
	PPFree(bytes);
}

TEST(PPMEM_TESTS, OverflowDetectedByFree)
{
	const size_t baseUsage = PPMemGetUsage();
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlloc(16, PP_SL));

	// tail guard is still owned memory, so free can report it and release the block
	bytes[17] ^= 0xFF;
	EXPECT_FATAL_FAILURE([&]() { PPFree(bytes); }, "buffer overflow");
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, UnderflowDetectedByCheck)
{
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlloc(16, PP_SL));
	const ubyte savedByte = bytes[-1];

	bytes[-1] = static_cast<ubyte>(savedByte + 1);
	EXPECT_FATAL_FAILURE([&]() { PPDCheck(bytes); }, "buffer underflow");

	bytes[-1] = savedByte;
	PPFree(bytes);
}

TEST(PPMEM_TESTS, UnderflowDetectedByFree)
{
	const size_t baseUsage = PPMemGetUsage();
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlloc(16, PP_SL));

	// block is still recognized by its ident and released from the real start
	bytes[-1] ^= 0xFF;
	EXPECT_FATAL_FAILURE([&]() { PPFree(bytes); }, "buffer underflow");
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, LongUnderflowDetected)
{
	const size_t baseUsage = PPMemGetUsage();
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlloc(16, PP_SL));

	// whole head guard is damaged, ident stays intact
	memset(bytes - 32, 0, 32);
	EXPECT_FATAL_FAILURE([&]() { PPDCheck(bytes); }, "buffer underflow by 32 bytes");
	EXPECT_FATAL_FAILURE([&]() { PPFree(bytes); }, "buffer underflow by 32 bytes");
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, ReAllocDetectsUnderflow)
{
	const size_t baseUsage = PPMemGetUsage();
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlloc(16, PP_SL));
	FillPattern(bytes, 16, 9);
	bytes[-1] ^= 0xFF;

	EXPECT_FATAL_FAILURE([&]() { bytes = reinterpret_cast<ubyte*>(PPDReAlloc(bytes, 64, PP_SL)); }, "buffer underflow");
	EXPECT_TRUE(CheckPattern(bytes, 16, 9));
	EXPECT_EQ(PPMemGetUsage(), baseUsage + 64);

	// guard is restored by reallocation
	PPDCheck(bytes);
	PPFree(bytes);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, AlignedUnderflowDetected)
{
	const size_t baseUsage = PPMemGetUsage();
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlignedAlloc(32, 64, PP_SL));

	bytes[-1] ^= 0xFF;
	EXPECT_FATAL_FAILURE([&]() { PPAlignedFree(bytes); }, "buffer underflow");
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

//-------------------------------------------------------------------------------------------------
// reallocation

TEST(PPMEM_TESTS, ReAllocNull)
{
	const size_t baseUsage = PPMemGetUsage();

	void* mem = PPDReAlloc(nullptr, 64, PP_SL);
	ASSERT_NE(mem, nullptr);
	EXPECT_EQ(PPMemGetUsage(), baseUsage + 64);

	PPFree(mem);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, ReAllocKeepsData)
{
	const size_t baseUsage = PPMemGetUsage();

	void* mem = PPDAlloc(32, PP_SL);
	FillPattern(mem, 32, 3);

	mem = PPDReAlloc(mem, 1000, PP_SL);
	ASSERT_NE(mem, nullptr);
	EXPECT_EQ(PPMemGetUsage(), baseUsage + 1000);
	EXPECT_TRUE(CheckPattern(mem, 32, 3));
	PPDCheck(mem);

	FillPattern(mem, 1000, 4);
	mem = PPDReAlloc(mem, 8, PP_SL);
	ASSERT_NE(mem, nullptr);
	EXPECT_EQ(PPMemGetUsage(), baseUsage + 8);
	EXPECT_TRUE(CheckPattern(mem, 8, 4));
	PPDCheck(mem);

	PPFree(mem);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, ReAllocDetectsOverflow)
{
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlloc(16, PP_SL));
	FillPattern(bytes, 16, 5);
	bytes[16] ^= 0xFF;

	EXPECT_FATAL_FAILURE([&]() { bytes = reinterpret_cast<ubyte*>(PPDReAlloc(bytes, 64, PP_SL)); }, "buffer overflow");
	EXPECT_TRUE(CheckPattern(bytes, 16, 5));
	PPDCheck(bytes);
	PPFree(bytes);
}

TEST(PPMEM_TESTS, ReAllocUntrackedKeepsData)
{
	const size_t baseUsage = PPMemGetUsage();

	void* mem = PPDAlloc(32, PPSourceLine::Empty());
	FillPattern(mem, 32, 6);

	mem = PPDReAlloc(mem, 1000, PPSourceLine::Empty());
	ASSERT_NE(mem, nullptr);
	EXPECT_TRUE(CheckPattern(mem, 32, 6));
	EXPECT_EQ(PPMemGetUsage(), baseUsage);

	PPFree(mem);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, ReAllocUntrackedWithSourceLineKeepsData)
{
	const size_t baseUsage = PPMemGetUsage();

	void* mem = PPDAlloc(32, PPSourceLine::Empty());
	FillPattern(mem, 32, 7);

	// untracked block stays untracked
	mem = PPDReAlloc(mem, 1000, PP_SL);
	ASSERT_NE(mem, nullptr);
	EXPECT_TRUE(CheckPattern(mem, 32, 7));
	EXPECT_EQ(PPMemGetUsage(), baseUsage);

	PPFree(mem);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, ReAllocTrackedWithoutSourceLineStaysTracked)
{
	const size_t baseUsage = PPMemGetUsage();

	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlloc(32, PP_SL));
	FillPattern(bytes, 32, 10);

	bytes = reinterpret_cast<ubyte*>(PPDReAlloc(bytes, 64, PPSourceLine::Empty()));
	ASSERT_NE(bytes, nullptr);
	EXPECT_TRUE(CheckPattern(bytes, 32, 10));
	EXPECT_EQ(PPMemGetUsage(), baseUsage + 64);

	// guards are still in place
	const ubyte savedByte = bytes[64];
	bytes[64] = static_cast<ubyte>(savedByte + 1);
	EXPECT_FATAL_FAILURE([&]() { PPDCheck(bytes); }, "buffer overflow");
	bytes[64] = savedByte;

	PPFree(bytes);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

TEST(PPMEM_TESTS, ReAllocLoop)
{
	const size_t baseUsage = PPMemGetUsage();

	// growth pattern of containers
	void* mem = nullptr;
	for (int size = 16; size <= 4096; size *= 2)
	{
		mem = PPDReAlloc(mem, size, PP_SL);
		ASSERT_NE(mem, nullptr);
		if (size > 16)
			EXPECT_TRUE(CheckPattern(mem, size / 2, size)) << "size " << size;
		FillPattern(mem, size, size * 2);
		EXPECT_EQ(PPMemGetUsage(), baseUsage + size);
	}

	PPFree(mem);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);
}

//-------------------------------------------------------------------------------------------------
// aligned allocations

TEST(PPMEM_TESTS, AlignedAlloc)
{
	const size_t baseUsage = PPMemGetUsage();

	const size_t alignments[] = { 16, 32, 64, 128 };
	for (const size_t alignment : alignments)
	{
		void* mem = PPDAlignedAlloc(100, alignment, PP_SL);
		ASSERT_NE(mem, nullptr);
		EXPECT_EQ(reinterpret_cast<uintptr_t>(mem) % alignment, 0u) << "alignment " << alignment;
		EXPECT_EQ(PPMemGetUsage(), baseUsage + 100) << "alignment " << alignment;

		FillPattern(mem, 100, 8);
		PPDCheck(mem);

		PPAlignedFree(mem);
		EXPECT_EQ(PPMemGetUsage(), baseUsage) << "alignment " << alignment;
	}
}

TEST(PPMEM_TESTS, AlignedAllocUntracked)
{
	const size_t baseUsage = PPMemGetUsage();

	void* mem = PPDAlignedAlloc(100, 64, PPSourceLine::Empty());
	ASSERT_NE(mem, nullptr);
	EXPECT_EQ(reinterpret_cast<uintptr_t>(mem) % 64, 0u);
	EXPECT_EQ(PPMemGetUsage(), baseUsage);

	PPAlignedFree(mem);
}

TEST(PPMEM_TESTS, AlignedOverflowDetected)
{
	ubyte* bytes = reinterpret_cast<ubyte*>(PPDAlignedAlloc(32, 64, PP_SL));
	const ubyte savedByte = bytes[32];

	bytes[32] = static_cast<ubyte>(savedByte + 1);
	EXPECT_FATAL_FAILURE([&]() { PPDCheck(bytes); }, "buffer overflow");

	bytes[32] = savedByte;
	PPAlignedFree(bytes);
}

//-------------------------------------------------------------------------------------------------
// untracked memory

#ifdef _WIN32
TEST(PPMEM_TESTS, UntrackedNearRegionStart)
{
	// large heap blocks start close to the beginning of their own pages,
	// header lookup must not read the page before them
	ubyte* region = reinterpret_cast<ubyte*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	ASSERT_NE(region, nullptr);

	const int offsets[] = { 4, 16, 48 };
	for (const int offset : offsets)
		PPDCheck(region + offset);

	VirtualFree(region, 0, MEM_RELEASE);
}
#endif // _WIN32

TEST(PPMEM_TESTS, LargeUntrackedFree)
{
	const size_t sizes[] = { 64 * 1024, 1024 * 1024, 16 * 1024 * 1024 };
	for (const size_t size : sizes)
	{
		void* mem = PPDAlloc(size, PPSourceLine::Empty());
		ASSERT_NE(mem, nullptr);
		PPDCheck(mem);
		PPFree(mem);
	}
}

#endif // PPMEM_DISABLED
