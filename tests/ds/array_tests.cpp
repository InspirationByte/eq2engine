//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Array, FixedArray, BufferedArray and array storage tests
//////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include "core/core_common.h"

// counts live instances to catch leaked or double-destroyed array elements
struct TrackedItem
{
	static int s_alive;

	TrackedItem() { ++s_alive; }
	TrackedItem(int value) : value(value) { ++s_alive; }
	TrackedItem(const TrackedItem& other) : value(other.value) { ++s_alive; }
	TrackedItem(TrackedItem&& other) : value(other.value) { other.value = -1; ++s_alive; }
	~TrackedItem() { --s_alive; }

	TrackedItem& operator=(const TrackedItem& other) = default;
	bool operator==(const TrackedItem& other) const { return value == other.value; }

	int value{ 0 };
};

int TrackedItem::s_alive = 0;

class ARRAY_LIFETIME_TESTS : public testing::Test
{
protected:
	void SetUp() override		{ TrackedItem::s_alive = 0; }
	void TearDown() override	{ EXPECT_EQ(TrackedItem::s_alive, 0); }
};

template<typename ARRAY_TYPE>
static void FillSequence(ARRAY_TYPE& arr, int count)
{
	for (int i = 0; i < count; ++i)
		arr.append(i);
}

template<typename ARRAY_TYPE>
static bool PointsInsideObject(const ARRAY_TYPE& arr)
{
	const ubyte* objStart = reinterpret_cast<const ubyte*>(&arr);
	const ubyte* data = reinterpret_cast<const ubyte*>(arr.ptr());
	return data >= objStart && data < objStart + sizeof(arr);
}

//-------------------------------------------------------------------------------------------------
// Array (dynamic storage) general behaviour

TEST(ARRAY_TESTS, Empty)
{
	Array<int> arr(PP_SL);

	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.numAllocated(), 0);
	EXPECT_TRUE(arr.isEmpty());
	EXPECT_EQ(arr.ptr(), nullptr);
	EXPECT_EQ(arr.getGranularity(), 16);
	EXPECT_FALSE(arr.inRange(0));
}

TEST(ARRAY_TESTS, InitializerList)
{
	Array<int> arr({ 5, 6, 7 }, PP_SL);

	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr[0], 5);
	EXPECT_EQ(arr[1], 6);
	EXPECT_EQ(arr[2], 7);
}

TEST(ARRAY_TESTS, Append)
{
	Array<int> arr(PP_SL);

	EXPECT_EQ(arr.append(10), 0);
	EXPECT_EQ(arr.append(20), 1);
	EXPECT_EQ(arr.append(30), 2);

	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_FALSE(arr.isEmpty());
	EXPECT_EQ(arr[0], 10);
	EXPECT_EQ(arr[1], 20);
	EXPECT_EQ(arr[2], 30);
	EXPECT_EQ(arr.front(), 10);
	EXPECT_EQ(arr.back(), 30);
	EXPECT_TRUE(arr.inRange(2));
	EXPECT_FALSE(arr.inRange(3));
	EXPECT_FALSE(arr.inRange(-1));
}

TEST(ARRAY_TESTS, AppendGrowsBeyondGranularity)
{
	Array<int> arr(PP_SL, 4);
	FillSequence(arr, 100);

	ASSERT_EQ(arr.numElem(), 100);
	EXPECT_GE(arr.numAllocated(), 100);
	EXPECT_EQ(arr.numAllocated() % 4, 0);

	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);
}

TEST(ARRAY_TESTS, AppendDefault)
{
	Array<int> arr(PP_SL);

	int& item = arr.append();
	EXPECT_EQ(item, 0);
	item = 42;

	ASSERT_EQ(arr.numElem(), 1);
	EXPECT_EQ(arr[0], 42);
}

TEST(ARRAY_TESTS, AppendEmplace)
{
	struct Pair
	{
		int a;
		float b;
	};

	Array<Pair> arr(PP_SL);
	EXPECT_EQ(arr.appendEmplace(3, 4.5f), 0);

	ASSERT_EQ(arr.numElem(), 1);
	EXPECT_EQ(arr[0].a, 3);
	EXPECT_EQ(arr[0].b, 4.5f);
}

TEST(ARRAY_TESTS, AppendPointerAndCount)
{
	const int values[] = { 1, 2, 3, 4 };

	Array<int> arr(PP_SL);
	arr.append(0);

	EXPECT_EQ(arr.append(values, 0), 1);
	EXPECT_EQ(arr.append(values, elementsOf(values)), 5);

	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);
}

TEST(ARRAY_TESTS, AppendOtherArray)
{
	Array<int> arr({ 0, 1 }, PP_SL);
	Array<int> other({ 2, 3 }, PP_SL);
	FixedArray<int, 4> fixedOther;
	fixedOther.append(4);
	fixedOther.append(5);

	EXPECT_EQ(arr.append(other), 4);
	EXPECT_EQ(arr.append(fixedOther), 6);
	EXPECT_EQ(arr.append(Array<int>(PP_SL)), 6);

	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);
}

TEST(ARRAY_TESTS, AppendWithTransform)
{
	Array<int> source({ 1, 2, 3, 4, 5, 6 }, PP_SL);
	Array<int> arr(PP_SL);

	// keep only odd numbers and square them
	arr.append(source, [](int& out, const int& in) {
		if (in % 2 == 0)
			return false;
		out = in * in;
		return true;
	});

	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr[0], 1);
	EXPECT_EQ(arr[1], 9);
	EXPECT_EQ(arr[2], 25);
}

TEST(ARRAY_TESTS, Insert)
{
	Array<int> arr({ 1, 3 }, PP_SL);

	EXPECT_EQ(arr.insert(0, 0), 0);
	EXPECT_EQ(arr.insert(2, 2), 2);
	EXPECT_EQ(arr.insert(4, arr.numElem()), 4);

	ASSERT_EQ(arr.numElem(), 5);
	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);
}

TEST(ARRAY_TESTS, InsertDefault)
{
	// NOTE: Array<int> makes insert(int index) ambiguous with insert(T&&, int)
	Array<float> arr({ 1.0f, 2.0f }, PP_SL);

	float& item = arr.insert(1);
	EXPECT_EQ(item, 0.0f);
	item = 7.0f;

	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr[0], 1.0f);
	EXPECT_EQ(arr[1], 7.0f);
	EXPECT_EQ(arr[2], 2.0f);
}

TEST(ARRAY_TESTS, InsertIntoEmpty)
{
	Array<int> arr(PP_SL);
	arr.insert(5, 0);

	ASSERT_EQ(arr.numElem(), 1);
	EXPECT_EQ(arr[0], 5);
}

TEST(ARRAY_TESTS, InsertInvalidIndex)
{
#ifdef PPMEM_DISABLED
	GTEST_SKIP() << "Insert writes before the buffer, only PPMem guards make it safe to test";
#endif

	// array lives in the scope so PPMem reports the damaged guard when it is freed
	EXPECT_FATAL_FAILURE([&]() {
		Array<int> arr({ 1, 2 }, PP_SL);
		EXPECT_FATAL_FAILURE([&]() { arr.insert(5, -1); }, "index >= 0");
	}, "buffer underflow");
}

TEST(ARRAY_TESTS, AddUnique)
{
	Array<int> arr(PP_SL);

	EXPECT_EQ(arr.addUnique(5), 0);
	EXPECT_EQ(arr.addUnique(6), 1);
	EXPECT_EQ(arr.addUnique(5), 0);
	EXPECT_EQ(arr.numElem(), 2);

	// comparator considers values equal by their tens
	auto sameTens = [](const int& a, const int& b) { return a / 10 == b / 10; };
	EXPECT_EQ(arr.addUnique(9, sameTens), 0);
	EXPECT_EQ(arr.addUnique(15, sameTens), 2);
	EXPECT_EQ(arr.numElem(), 3);
}

TEST(ARRAY_TESTS, Find)
{
	Array<int> arr({ 10, 20, 30 }, PP_SL);

	{
		auto it = arr.find(20);
		ASSERT_FALSE(it.atEnd());
		EXPECT_EQ(it.index, 1);
		EXPECT_EQ(*it, 20);
	}

	{
		auto it = arr.find(25);
		EXPECT_TRUE(it.atEnd());
	}

	{
		auto it = arr.find([](const int& item) { return item > 15; });
		ASSERT_FALSE(it.atEnd());
		EXPECT_EQ(it.index, 1);
	}

	{
		auto it = arr.find(31, [](const int& a, const int& b) { return a / 10 == b / 10; });
		ASSERT_FALSE(it.atEnd());
		EXPECT_EQ(it.index, 2);
	}
}

TEST(ARRAY_TESTS, NumElemPredicate)
{
	Array<int> arr({ 1, 2, 3, 4, 5 }, PP_SL);
	EXPECT_EQ(arr.numElem([](const int& item) { return item % 2 == 1; }), 3);
	EXPECT_EQ(arr.numElem([](const int& item) { return item > 10; }), 0);
}

TEST(ARRAY_TESTS, Iterate)
{
	Array<int> arr(PP_SL);
	FillSequence(arr, 10);

	int expected = 0;
	for (int& item : arr)
	{
		EXPECT_EQ(item, expected);
		++expected;
	}
	EXPECT_EQ(expected, 10);

	Array<int> empty(PP_SL);
	for (int& item : empty)
		ADD_FAILURE() << "empty array iterated";
}

TEST(ARRAY_TESTS, IndexOutOfRange)
{
	Array<int> arr({ 1, 2 }, PP_SL);
	EXPECT_FATAL_FAILURE([&]() { arr[-1]; }, "invalid index");
}

TEST(ARRAY_TESTS, RemoveIndex)
{
	Array<int> arr({ 0, 1, 2, 3 }, PP_SL);

	EXPECT_TRUE(arr.removeIndex(1));
	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr[0], 0);
	EXPECT_EQ(arr[1], 2);
	EXPECT_EQ(arr[2], 3);

	EXPECT_TRUE(arr.removeIndex(2));
	ASSERT_EQ(arr.numElem(), 2);
	EXPECT_EQ(arr.back(), 2);
}

TEST(ARRAY_TESTS, RemoveRange)
{
	Array<int> arr(PP_SL);
	FillSequence(arr, 8);

	EXPECT_TRUE(arr.removeRange(2, 3));
	ASSERT_EQ(arr.numElem(), 5);
	EXPECT_EQ(arr[0], 0);
	EXPECT_EQ(arr[1], 1);
	EXPECT_EQ(arr[2], 5);
	EXPECT_EQ(arr[3], 6);
	EXPECT_EQ(arr[4], 7);

	EXPECT_FALSE(arr.removeRange(1, 0));
	EXPECT_EQ(arr.numElem(), 5);

	// tail removal
	EXPECT_TRUE(arr.removeRange(3, 2));
	EXPECT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr.back(), 5);
}

TEST(ARRAY_TESTS, RemoveRangeInvalid)
{
	Array<int> arr({ 0, 1, 2 }, PP_SL);
	EXPECT_FATAL_FAILURE([&]() { arr.removeRange(2, 5); }, "index + count <= m_numElem");
	EXPECT_EQ(arr.numElem(), 2);
}

TEST(ARRAY_TESTS, FastRemoveIndex)
{
	Array<int> arr({ 0, 1, 2, 3 }, PP_SL);

	EXPECT_TRUE(arr.fastRemoveIndex(1));
	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr[0], 0);
	EXPECT_EQ(arr[1], 3);
	EXPECT_EQ(arr[2], 2);

	// removing last item must not disturb others
	EXPECT_TRUE(arr.fastRemoveIndex(2));
	ASSERT_EQ(arr.numElem(), 2);
	EXPECT_EQ(arr[0], 0);
	EXPECT_EQ(arr[1], 3);
}

TEST(ARRAY_TESTS, FastRemoveIndexInvalid)
{
	Array<int> arr({ 0, 1 }, PP_SL);
	EXPECT_FATAL_FAILURE([&]() { EXPECT_FALSE(arr.fastRemoveIndex(2)); }, "index < m_numElem");
	EXPECT_EQ(arr.numElem(), 2);
}

TEST(ARRAY_TESTS, RemoveByValue)
{
	Array<int> arr({ 5, 6, 7, 8 }, PP_SL);

	EXPECT_TRUE(arr.remove(6));
	EXPECT_FALSE(arr.remove(42));
	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr[0], 5);
	EXPECT_EQ(arr[1], 7);
	EXPECT_EQ(arr[2], 8);

	EXPECT_TRUE(arr.fastRemove(5));
	EXPECT_FALSE(arr.fastRemove(42));
	ASSERT_EQ(arr.numElem(), 2);
	EXPECT_EQ(arr[0], 8);
	EXPECT_EQ(arr[1], 7);
}

TEST(ARRAY_TESTS, Pop)
{
	Array<int> arr({ 0, 1, 2, 3, 4 }, PP_SL);

	EXPECT_EQ(arr.popBack(), 4);
	EXPECT_EQ(arr.popFront(), 0);
	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr[0], 1);
	EXPECT_EQ(arr[1], 2);
	EXPECT_EQ(arr[2], 3);

	EXPECT_EQ(arr.fastPopFront(), 1);
	ASSERT_EQ(arr.numElem(), 2);
	EXPECT_EQ(arr[0], 3);
	EXPECT_EQ(arr[1], 2);
}

TEST(ARRAY_TESTS, Clear)
{
	Array<int> arr(PP_SL);
	FillSequence(arr, 20);
	const int allocated = arr.numAllocated();

	arr.clear();
	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.numAllocated(), allocated);
	EXPECT_NE(arr.ptr(), nullptr);

	arr.append(1);
	arr.clear(true);
	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.numAllocated(), 0);
	EXPECT_EQ(arr.ptr(), nullptr);
}

TEST(ARRAY_TESTS, Reserve)
{
	Array<int> arr(PP_SL, 8);

	arr.reserve(3);
	EXPECT_EQ(arr.numAllocated(), 8);
	EXPECT_EQ(arr.numElem(), 0);

	arr.reserve(9);
	EXPECT_EQ(arr.numAllocated(), 16);

	// reserve never shrinks
	arr.reserve(1);
	EXPECT_EQ(arr.numAllocated(), 16);
}

TEST(ARRAY_TESTS, ResizeKeepsContents)
{
	Array<int> arr(PP_SL, 4);
	FillSequence(arr, 3);
	const int* oldPtr = arr.ptr();

	arr.resize(100);
	EXPECT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr.numAllocated(), 100);
	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);

	// same capacity after granularity rounding must not reallocate
	const int* grownPtr = arr.ptr();
	arr.resize(99);
	EXPECT_EQ(arr.ptr(), grownPtr);
	(void)oldPtr;
}

TEST(ARRAY_TESTS, ResizeShrink)
{
	Array<int> arr(PP_SL, 1);
	FillSequence(arr, 10);

	arr.resize(4);
	EXPECT_EQ(arr.numElem(), 4);
	EXPECT_EQ(arr.numAllocated(), 4);
	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);
}

TEST(ARRAY_TESTS, ResizeToZero)
{
	Array<int> arr(PP_SL);
	FillSequence(arr, 10);

	arr.resize(0);
	EXPECT_EQ(arr.numAllocated(), 0);
	EXPECT_EQ(arr.ptr(), nullptr);
	EXPECT_EQ(arr.numElem(), 0);
}

TEST(ARRAY_TESTS, SetNum)
{
	Array<int> arr(PP_SL);
	arr.setNum(5);

	ASSERT_EQ(arr.numElem(), 5);
	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], 0);

	arr[1] = 11;
	arr.setNum(2);
	ASSERT_EQ(arr.numElem(), 2);
	EXPECT_EQ(arr[1], 11);

	arr.setNum(20, false);
	EXPECT_EQ(arr.numElem(), 20);
	EXPECT_GE(arr.numAllocated(), 20);
	EXPECT_EQ(arr[1], 11);
	EXPECT_EQ(arr[19], 0);
}

TEST(ARRAY_TESTS, SetGranularity)
{
	Array<int> arr(PP_SL, 4);
	FillSequence(arr, 5);
	EXPECT_EQ(arr.numAllocated(), 8);

	arr.setGranularity(32);
	EXPECT_EQ(arr.getGranularity(), 32);
	EXPECT_EQ(arr.numAllocated(), 32);
	ASSERT_EQ(arr.numElem(), 5);
	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);
}

TEST(ARRAY_TESTS, AssureSize)
{
	Array<int> arr({ 1, 2 }, PP_SL);

	arr.assureSize(10);
	EXPECT_EQ(arr.numElem(), 10);
	EXPECT_GE(arr.numAllocated(), 10);
	EXPECT_EQ(arr[0], 1);
	EXPECT_EQ(arr[1], 2);

	Array<int> emplaced({ 1, 2 }, PP_SL);
	emplaced.assureSizeEmplace(5, 9);
	ASSERT_EQ(emplaced.numElem(), 5);
	EXPECT_EQ(emplaced[0], 1);
	EXPECT_EQ(emplaced[1], 2);
	EXPECT_EQ(emplaced[2], 9);
	EXPECT_EQ(emplaced[4], 9);
}

TEST(ARRAY_TESTS, CopyConstruct)
{
	Array<int> arr({ 1, 2, 3 }, PP_SL);
	Array<int> copy(arr);

	ASSERT_EQ(copy.numElem(), 3);
	EXPECT_NE(copy.ptr(), arr.ptr());
	EXPECT_EQ(copy.getGranularity(), arr.getGranularity());
	for (int i = 0; i < copy.numElem(); ++i)
		EXPECT_EQ(copy[i], arr[i]);

	copy[0] = 100;
	EXPECT_EQ(arr[0], 1);
}

TEST(ARRAY_TESTS, CopyConstructEmpty)
{
	Array<int> arr(PP_SL);
	Array<int> copy(arr);

	EXPECT_EQ(copy.numElem(), 0);
	EXPECT_EQ(copy.ptr(), nullptr);
}

TEST(ARRAY_TESTS, CopyAssign)
{
	Array<int> arr({ 1, 2, 3 }, PP_SL);
	Array<int> target({ 7, 8, 9, 10, 11 }, PP_SL, 4);

	target = arr;
	ASSERT_EQ(target.numElem(), 3);
	EXPECT_NE(target.ptr(), arr.ptr());
	EXPECT_EQ(target.getGranularity(), arr.getGranularity());
	for (int i = 0; i < target.numElem(); ++i)
		EXPECT_EQ(target[i], arr[i]);
}

TEST(ARRAY_TESTS, MoveConstruct)
{
	Array<int> arr({ 1, 2, 3 }, PP_SL);
	const int* data = arr.ptr();

	Array<int> moved(std::move(arr));
	EXPECT_EQ(moved.ptr(), data);
	ASSERT_EQ(moved.numElem(), 3);
	EXPECT_EQ(moved[2], 3);

	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.ptr(), nullptr);
	EXPECT_EQ(arr.numAllocated(), 0);
}

TEST(ARRAY_TESTS, Swap)
{
	Array<int> a({ 1, 2, 3 }, PP_SL, 4);
	Array<int> b({ 9 }, PP_SL, 8);
	const int* aData = a.ptr();
	const int* bData = b.ptr();

	a.swap(b);
	EXPECT_EQ(a.ptr(), bData);
	EXPECT_EQ(b.ptr(), aData);
	ASSERT_EQ(a.numElem(), 1);
	ASSERT_EQ(b.numElem(), 3);
	EXPECT_EQ(a[0], 9);
	EXPECT_EQ(b[2], 3);
	EXPECT_EQ(a.getGranularity(), 8);
	EXPECT_EQ(b.getGranularity(), 4);
}

TEST(ARRAY_TESTS, SwapRaw)
{
	Array<int> arr({ 1, 2, 3 }, PP_SL);
	const int* data = arr.ptr();

	int* raw = nullptr;
	int rawNum = 0;
	arr.swap(raw, rawNum);

	EXPECT_EQ(raw, data);
	EXPECT_EQ(rawNum, 3);
	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.ptr(), nullptr);

	arr.swap(raw, rawNum);
	EXPECT_EQ(raw, nullptr);
	EXPECT_EQ(rawNum, 0);
	ASSERT_EQ(arr.numElem(), 3);
	EXPECT_EQ(arr.ptr(), data);
	EXPECT_EQ(arr[1], 2);
}

TEST(ARRAY_TESTS, NestedArrays)
{
	Array<Array<int>> arr(PP_SL);

	Array<int>& first = arr.append();
	first.append(1);
	arr.append().append(2);
	arr.setNum(4);

	ASSERT_EQ(arr.numElem(), 4);
	ASSERT_EQ(arr[0].numElem(), 1);
	EXPECT_EQ(arr[0][0], 1);
	EXPECT_EQ(arr[1][0], 2);
	EXPECT_TRUE(arr[3].isEmpty());
}

TEST(ARRAY_TESTS, NonTrivialItems)
{
	Array<EqString> arr(PP_SL, 2);
	for (int i = 0; i < 10; ++i)
		arr.append(EqString::Format("item %d", i));

	arr.insert("first", 0);
	arr.removeIndex(5);

	ASSERT_EQ(arr.numElem(), 10);
	EXPECT_EQ(arr[0], EqStringRef("first"));
	EXPECT_EQ(arr[1], EqStringRef("item 0"));
	EXPECT_EQ(arr[4], EqStringRef("item 3"));
	EXPECT_EQ(arr[5], EqStringRef("item 5"));
	EXPECT_EQ(arr[9], EqStringRef("item 9"));
}

//-------------------------------------------------------------------------------------------------
// element lifetime tracking

TEST_F(ARRAY_LIFETIME_TESTS, AppendAndDestroy)
{
	{
		Array<TrackedItem> arr(PP_SL, 2);
		for (int i = 0; i < 10; ++i)
			arr.append(TrackedItem(i));

		EXPECT_EQ(TrackedItem::s_alive, 10);
	}
	EXPECT_EQ(TrackedItem::s_alive, 0);
}

TEST_F(ARRAY_LIFETIME_TESTS, AppendMove)
{
	Array<TrackedItem> arr(PP_SL);
	TrackedItem item(5);

	arr.append(std::move(item));
	EXPECT_EQ(item.value, -1);
	EXPECT_EQ(arr[0].value, 5);
}

TEST_F(ARRAY_LIFETIME_TESTS, Remove)
{
	Array<TrackedItem> arr(PP_SL);
	for (int i = 0; i < 6; ++i)
		arr.append(TrackedItem(i));

	arr.removeIndex(0);
	EXPECT_EQ(TrackedItem::s_alive, 5);

	arr.fastRemoveIndex(0);
	EXPECT_EQ(TrackedItem::s_alive, 4);

	arr.removeRange(1, 2);
	EXPECT_EQ(TrackedItem::s_alive, 2);

	arr.popBack();
	EXPECT_EQ(TrackedItem::s_alive, 1);

	arr.popFront();
	EXPECT_EQ(TrackedItem::s_alive, 0);
}

TEST_F(ARRAY_LIFETIME_TESTS, ClearAndSetNum)
{
	Array<TrackedItem> arr(PP_SL);

	arr.setNum(8);
	EXPECT_EQ(TrackedItem::s_alive, 8);

	arr.setNum(3);
	EXPECT_EQ(TrackedItem::s_alive, 3);

	arr.resize(1);
	EXPECT_EQ(TrackedItem::s_alive, 1);

	arr.clear();
	EXPECT_EQ(TrackedItem::s_alive, 0);
}

TEST_F(ARRAY_LIFETIME_TESTS, Copy)
{
	Array<TrackedItem> arr(PP_SL);
	arr.append(TrackedItem(1));
	arr.append(TrackedItem(2));

	{
		Array<TrackedItem> copy(arr);
		EXPECT_EQ(TrackedItem::s_alive, 4);
	}
	EXPECT_EQ(TrackedItem::s_alive, 2);
}

TEST_F(ARRAY_LIFETIME_TESTS, CopyAssignOverExisting)
{
	Array<TrackedItem> arr(PP_SL);
	arr.append(TrackedItem(1));

	{
		Array<TrackedItem> target(PP_SL);
		for (int i = 0; i < 4; ++i)
			target.append(TrackedItem(i));

		target = arr;
		EXPECT_EQ(target.numElem(), 1);
		EXPECT_EQ(TrackedItem::s_alive, 2);
	}
	EXPECT_EQ(TrackedItem::s_alive, 1);
}

//-------------------------------------------------------------------------------------------------
// DynamicArrayStorage

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, Empty)
{
	DynamicArrayStorage<int> storage(PP_SL, 8);

	EXPECT_EQ(storage.getSize(), 0);
	EXPECT_EQ(storage.getData(), nullptr);
	EXPECT_EQ(storage.getGranularity(), 8);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, InvalidGranularity)
{
	EXPECT_FATAL_FAILURE([&]() { DynamicArrayStorage<int> storage(PP_SL, 0); }, "m_granularity > 0");

	DynamicArrayStorage<int> storage(PP_SL, 4);
	EXPECT_FATAL_FAILURE([&]() { storage.setGranularity(-1); }, "newGranularity > 0");
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, ResizeRoundsToGranularity)
{
	DynamicArrayStorage<int> storage(PP_SL, 8);
	int numElem = 0;

	storage.resize(1, numElem);
	EXPECT_EQ(storage.getSize(), 8);
	EXPECT_NE(storage.getData(), nullptr);

	storage.resize(9, numElem);
	EXPECT_EQ(storage.getSize(), 16);

	storage.resize(16, numElem);
	EXPECT_EQ(storage.getSize(), 16);
	EXPECT_EQ(numElem, 0);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, ResizeKeepsData)
{
	DynamicArrayStorage<int> storage(PP_SL, 4);
	int numElem = 0;

	storage.resize(4, numElem);
	for (int i = 0; i < 4; ++i)
		storage.getData()[i] = i * 10;

	storage.resize(1000, numElem);
	for (int i = 0; i < 4; ++i)
		EXPECT_EQ(storage.getData()[i], i * 10);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, ResizeSameCapacityKeepsPointer)
{
	DynamicArrayStorage<int> storage(PP_SL, 16);
	int numElem = 0;

	storage.resize(10, numElem);
	const int* data = storage.getData();

	storage.resize(16, numElem);
	EXPECT_EQ(storage.getData(), data);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, ShrinkClampsElementCount)
{
	DynamicArrayStorage<int> storage(PP_SL, 1);
	int numElem = 0;

	storage.resize(10, numElem);
	numElem = 10;

	storage.resize(4, numElem);
	EXPECT_EQ(storage.getSize(), 4);
	EXPECT_EQ(numElem, 4);

	// growing must leave element count alone
	storage.resize(20, numElem);
	EXPECT_EQ(numElem, 4);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, ResizeToZeroFrees)
{
	DynamicArrayStorage<int> storage(PP_SL, 4);
	int numElem = 0;

	storage.resize(10, numElem);
	numElem = 10;

	storage.resize(0, numElem);
	EXPECT_EQ(storage.getSize(), 0);
	EXPECT_EQ(storage.getData(), nullptr);
	EXPECT_EQ(numElem, 0);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, ResizeToZeroWhenEmpty)
{
	DynamicArrayStorage<int> storage(PP_SL, 4);
	int numElem = 0;

	storage.resize(0, numElem);
	EXPECT_EQ(storage.getSize(), 0);
	EXPECT_EQ(storage.getData(), nullptr);
	EXPECT_EQ(numElem, 0);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, ResizeToZeroThenGrow)
{
	DynamicArrayStorage<int> storage(PP_SL, 4);
	int numElem = 0;

	storage.resize(10, numElem);
	numElem = 10;
	storage.resize(0, numElem);

	storage.resize(3, numElem);
	EXPECT_EQ(storage.getSize(), 4);
	EXPECT_NE(storage.getData(), nullptr);
	EXPECT_EQ(numElem, 0);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, NegativeResize)
{
	DynamicArrayStorage<int> storage(PP_SL, 4);
	int numElem = 0;
	EXPECT_FATAL_FAILURE([&]() { storage.resize(-1, numElem); }, "requested capacity is -1");
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, Free)
{
	DynamicArrayStorage<int> storage(PP_SL, 4);
	int numElem = 0;

	storage.resize(10, numElem);
	storage.free();
	EXPECT_EQ(storage.getSize(), 0);
	EXPECT_EQ(storage.getData(), nullptr);

	// free is safe to call repeatedly
	storage.free();
	EXPECT_EQ(storage.getData(), nullptr);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, CopyAllocatesOwnBuffer)
{
	DynamicArrayStorage<int> storage(PP_SL, 4);
	int numElem = 0;
	storage.resize(6, numElem);

	DynamicArrayStorage<int> copy(storage);
	EXPECT_EQ(copy.getSize(), storage.getSize());
	EXPECT_EQ(copy.getGranularity(), storage.getGranularity());
	EXPECT_NE(copy.getData(), nullptr);
	EXPECT_NE(copy.getData(), storage.getData());

	DynamicArrayStorage<int> empty(PP_SL, 4);
	DynamicArrayStorage<int> emptyCopy(empty);
	EXPECT_EQ(emptyCopy.getSize(), 0);
	EXPECT_EQ(emptyCopy.getData(), nullptr);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, Move)
{
	DynamicArrayStorage<int> storage(PP_SL, 4);
	int numElem = 0;
	storage.resize(6, numElem);
	const int* data = storage.getData();

	DynamicArrayStorage<int> moved(std::move(storage));
	EXPECT_EQ(moved.getData(), data);
	EXPECT_EQ(moved.getSize(), 8);
	EXPECT_EQ(moved.getGranularity(), 4);

	EXPECT_EQ(storage.getData(), nullptr);
	EXPECT_EQ(storage.getSize(), 0);
}

TEST(DYNAMIC_ARRAY_STORAGE_TESTS, Swap)
{
	DynamicArrayStorage<int> a(PP_SL, 4);
	DynamicArrayStorage<int> b(PP_SL, 8);
	int numElem = 0;
	a.resize(4, numElem);
	const int* aData = a.getData();

	a.swap(b);
	EXPECT_EQ(a.getData(), nullptr);
	EXPECT_EQ(a.getSize(), 0);
	EXPECT_EQ(a.getGranularity(), 8);
	EXPECT_EQ(b.getData(), aData);
	EXPECT_EQ(b.getSize(), 4);
	EXPECT_EQ(b.getGranularity(), 4);
}

//-------------------------------------------------------------------------------------------------
// FixedArrayStorage

TEST(FIXED_ARRAY_STORAGE_TESTS, Properties)
{
	FixedArrayStorage<int, 8> storage;

	EXPECT_EQ(storage.getSize(), 8);
	EXPECT_EQ(storage.getGranularity(), 1);
	EXPECT_NE(storage.getData(), nullptr);

	// granularity is fixed
	storage.setGranularity(16);
	EXPECT_EQ(storage.getGranularity(), 1);

	const ubyte* objStart = reinterpret_cast<const ubyte*>(&storage);
	const ubyte* data = reinterpret_cast<const ubyte*>(storage.getData());
	EXPECT_GE(data, objStart);
	EXPECT_LE(data + 8 * sizeof(int), objStart + sizeof(storage));
}

TEST(FIXED_ARRAY_STORAGE_TESTS, Alignment)
{
	struct alignas(16) AlignedItem
	{
		float v[4];
	};

	FixedArrayStorage<AlignedItem, 3> storage;
	EXPECT_EQ(reinterpret_cast<uintptr_t>(storage.getData()) % 16, 0u);
}

TEST(FIXED_ARRAY_STORAGE_TESTS, Resize)
{
	FixedArrayStorage<int, 4> storage;
	int numElem = 2;

	storage.resize(4, numElem);
	EXPECT_EQ(storage.getSize(), 4);
	EXPECT_EQ(numElem, 2);

	EXPECT_FATAL_FAILURE([&]() { storage.resize(5, numElem); }, "capacity is 4, required 5");
	EXPECT_EQ(storage.getSize(), 4);
}

TEST(FIXED_ARRAY_STORAGE_TESTS, ResizeToZero)
{
	FixedArrayStorage<int, 4> storage;
	const int* data = storage.getData();
	int numElem = 3;

	// capacity is fixed, but element count must follow the requested capacity
	storage.resize(0, numElem);
	EXPECT_EQ(storage.getSize(), 4);
	EXPECT_EQ(storage.getData(), data);
	EXPECT_EQ(numElem, 0);
}

TEST(FIXED_ARRAY_STORAGE_TESTS, Swap)
{
	FixedArrayStorage<int, 3> a;
	FixedArrayStorage<int, 3> b;
	for (int i = 0; i < 3; ++i)
	{
		a.getData()[i] = i;
		b.getData()[i] = i + 10;
	}

	a.swap(b);
	for (int i = 0; i < 3; ++i)
	{
		EXPECT_EQ(a.getData()[i], i + 10);
		EXPECT_EQ(b.getData()[i], i);
	}
}

TEST(FIXED_ARRAY_STORAGE_TESTS, CopyAndMove)
{
	FixedArrayStorage<int, 3> storage;
	for (int i = 0; i < 3; ++i)
		storage.getData()[i] = i + 1;

	FixedArrayStorage<int, 3> copy(storage);
	FixedArrayStorage<int, 3> moved(std::move(storage));
	EXPECT_NE(copy.getData(), storage.getData());
	for (int i = 0; i < 3; ++i)
	{
		EXPECT_EQ(copy.getData()[i], i + 1);
		EXPECT_EQ(moved.getData()[i], i + 1);
	}
}

TEST(FIXED_ARRAY_TESTS, Basics)
{
	FixedArray<int, 4> arr;

	EXPECT_TRUE(arr.isEmpty());
	EXPECT_FALSE(arr.isFull());
	EXPECT_EQ(arr.numAllocated(), 4);
	EXPECT_TRUE(PointsInsideObject(arr));

	const int* data = arr.ptr();
	FillSequence(arr, 4);
	EXPECT_TRUE(arr.isFull());
	EXPECT_EQ(arr.ptr(), data);
	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);

	arr.removeIndex(0);
	arr.insert(10, 2);
	ASSERT_EQ(arr.numElem(), 4);
	EXPECT_EQ(arr[0], 1);
	EXPECT_EQ(arr[1], 2);
	EXPECT_EQ(arr[2], 10);
	EXPECT_EQ(arr[3], 3);
}

TEST(FIXED_ARRAY_TESTS, Overflow)
{
	FixedArray<int, 2> arr;
	FillSequence(arr, 2);

	EXPECT_FATAL_FAILURE([&]() { arr.reserve(3); }, "capacity is 2, required 3");
	EXPECT_EQ(arr.numElem(), 2);
}

TEST(FIXED_ARRAY_TESTS, ClearKeepsCapacity)
{
	FixedArray<int, 4> arr;
	FillSequence(arr, 3);

	arr.clear(true);
	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.numAllocated(), 4);
	EXPECT_TRUE(PointsInsideObject(arr));
}

TEST(FIXED_ARRAY_TESTS, ResizeShrink)
{
	FixedArray<int, 8> arr;
	FillSequence(arr, 6);

	arr.resize(2);
	EXPECT_EQ(arr.numAllocated(), 8);
	EXPECT_EQ(arr.numElem(), 2);
}

TEST(FIXED_ARRAY_TESTS, ResizeToZero)
{
	FixedArray<int, 4> arr;
	FillSequence(arr, 3);

	arr.resize(0);
	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.numAllocated(), 4);
	EXPECT_TRUE(PointsInsideObject(arr));
}

TEST(FIXED_ARRAY_TESTS, CopyAndMove)
{
	FixedArray<int, 4> arr;
	FillSequence(arr, 3);

	FixedArray<int, 4> copy(arr);
	ASSERT_EQ(copy.numElem(), 3);
	EXPECT_TRUE(PointsInsideObject(copy));
	copy[0] = 100;
	EXPECT_EQ(arr[0], 0);

	FixedArray<int, 4> assigned;
	assigned.append(55);
	assigned = arr;
	ASSERT_EQ(assigned.numElem(), 3);
	EXPECT_EQ(assigned[0], 0);
	EXPECT_EQ(assigned[2], 2);

	FixedArray<int, 4> moved(std::move(arr));
	ASSERT_EQ(moved.numElem(), 3);
	EXPECT_TRUE(PointsInsideObject(moved));
	EXPECT_EQ(moved[2], 2);
	EXPECT_EQ(arr.numElem(), 0);
}

TEST(FIXED_ARRAY_TESTS, Swap)
{
	FixedArray<int, 4> a;
	FixedArray<int, 4> b;
	FillSequence(a, 3);
	b.append(42);

	a.swap(b);
	ASSERT_EQ(a.numElem(), 1);
	ASSERT_EQ(b.numElem(), 3);
	EXPECT_EQ(a[0], 42);
	EXPECT_EQ(b[0], 0);
	EXPECT_EQ(b[2], 2);
}

TEST_F(ARRAY_LIFETIME_TESTS, FixedArrayItems)
{
	{
		FixedArray<TrackedItem, 4> arr;
		arr.append(TrackedItem(1));
		arr.append(TrackedItem(2));
		EXPECT_EQ(TrackedItem::s_alive, 2);

		{
			FixedArray<TrackedItem, 4> copy(arr);
			EXPECT_EQ(TrackedItem::s_alive, 4);
		}
		EXPECT_EQ(TrackedItem::s_alive, 2);

		arr.popBack();
		EXPECT_EQ(TrackedItem::s_alive, 1);
	}
	EXPECT_EQ(TrackedItem::s_alive, 0);
}

//-------------------------------------------------------------------------------------------------
// BufferedArrayStorage


TEST(BUFFERED_ARRAY_STORAGE_TESTS, StartsWithFixedBuffer)
{
	BufferedArrayStorage<int, 4> storage(PP_SL, 8);

	EXPECT_EQ(storage.getSize(), 4);
	EXPECT_EQ(storage.getGranularity(), 8);
	EXPECT_NE(storage.getData(), nullptr);
	EXPECT_EQ(storage.getDynamicStorage().getData(), nullptr);

	// staying within the fixed buffer must not allocate
	int numElem = 0;
	storage.resize(4, numElem);
	EXPECT_EQ(storage.getDynamicStorage().getData(), nullptr);
	EXPECT_EQ(storage.getSize(), 4);
}

TEST(BUFFERED_ARRAY_STORAGE_TESTS, GrowsToDynamic)
{
	BufferedArrayStorage<int, 4> storage(PP_SL, 8);
	int numElem = 4;
	for (int i = 0; i < 4; ++i)
		storage.getData()[i] = i + 1;
	const int* fixedData = storage.getData();

	storage.resize(5, numElem);
	EXPECT_EQ(storage.getSize(), 8);
	EXPECT_NE(storage.getDynamicStorage().getData(), nullptr);
	EXPECT_EQ(storage.getData(), storage.getDynamicStorage().getData());
	EXPECT_NE(storage.getData(), fixedData);

	// contents of the fixed buffer must be carried over
	for (int i = 0; i < 4; ++i)
		EXPECT_EQ(storage.getData()[i], i + 1);
}

TEST(BUFFERED_ARRAY_STORAGE_TESTS, GrowDynamicKeepsData)
{
	BufferedArrayStorage<int, 2> storage(PP_SL, 4);
	int numElem = 0;

	storage.resize(4, numElem);
	for (int i = 0; i < 4; ++i)
		storage.getData()[i] = i * 3;

	storage.resize(100, numElem);
	for (int i = 0; i < 4; ++i)
		EXPECT_EQ(storage.getData()[i], i * 3);
}

TEST(BUFFERED_ARRAY_STORAGE_TESTS, FreeReturnsToFixedBuffer)
{
	BufferedArrayStorage<int, 4> storage(PP_SL, 8);
	const int* fixedData = storage.getData();
	int numElem = 0;

	storage.resize(10, numElem);
	storage.free();
	EXPECT_EQ(storage.getDynamicStorage().getData(), nullptr);
	EXPECT_EQ(storage.getData(), fixedData);
	EXPECT_EQ(storage.getSize(), 4);
}

TEST(BUFFERED_ARRAY_STORAGE_TESTS, ResizeToZeroInFixedBuffer)
{
	BufferedArrayStorage<int, 4> storage(PP_SL, 8);
	const int* fixedData = storage.getData();
	int numElem = 3;

	storage.resize(0, numElem);
	EXPECT_EQ(storage.getSize(), 4);
	EXPECT_EQ(storage.getData(), fixedData);
	EXPECT_EQ(storage.getDynamicStorage().getData(), nullptr);
	EXPECT_EQ(numElem, 0);
}

TEST(BUFFERED_ARRAY_STORAGE_TESTS, ResizeToZeroFromDynamic)
{
	BufferedArrayStorage<int, 4> storage(PP_SL, 8);
	const int* fixedData = storage.getData();
	int numElem = 0;

	storage.resize(10, numElem);
	numElem = 10;

	storage.resize(0, numElem);
	EXPECT_EQ(storage.getDynamicStorage().getData(), nullptr);
	EXPECT_EQ(storage.getData(), fixedData);
	EXPECT_EQ(storage.getSize(), 4);
	EXPECT_EQ(numElem, 0);
}

TEST(BUFFERED_ARRAY_TESTS, StaysInFixedBuffer)
{
	BufferedArray<int, 4> arr(PP_SL);

	EXPECT_EQ(arr.numAllocated(), 4);
	EXPECT_TRUE(PointsInsideObject(arr));

	FillSequence(arr, 4);
	EXPECT_TRUE(PointsInsideObject(arr));
	EXPECT_TRUE(arr.isFull());
	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);
}

TEST(BUFFERED_ARRAY_TESTS, SpillsToHeap)
{
	BufferedArray<int, 4> arr(PP_SL, 4);
	FillSequence(arr, 20);

	EXPECT_FALSE(PointsInsideObject(arr));
	EXPECT_GE(arr.numAllocated(), 20);
	ASSERT_EQ(arr.numElem(), 20);
	for (int i = 0; i < arr.numElem(); ++i)
		EXPECT_EQ(arr[i], i);
}

TEST(BUFFERED_ARRAY_TESTS, ClearDeallocateReturnsToFixedBuffer)
{
	BufferedArray<int, 4> arr(PP_SL);
	FillSequence(arr, 20);

	arr.clear(true);
	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.numAllocated(), 4);
	EXPECT_TRUE(PointsInsideObject(arr));

	FillSequence(arr, 3);
	EXPECT_TRUE(PointsInsideObject(arr));
	EXPECT_EQ(arr[2], 2);
}

TEST(BUFFERED_ARRAY_TESTS, ResizeToZero)
{
	BufferedArray<int, 4> arr(PP_SL);
	FillSequence(arr, 10);

	arr.resize(0);
	EXPECT_EQ(arr.numElem(), 0);
	EXPECT_EQ(arr.numAllocated(), 4);
	EXPECT_TRUE(PointsInsideObject(arr));
}

TEST(BUFFERED_ARRAY_TESTS, Copy)
{
	BufferedArray<int, 4> small(PP_SL);
	FillSequence(small, 3);

	BufferedArray<int, 4> smallCopy(small);
	EXPECT_TRUE(PointsInsideObject(smallCopy));
	ASSERT_EQ(smallCopy.numElem(), 3);
	EXPECT_EQ(smallCopy[2], 2);

	BufferedArray<int, 4> big(PP_SL);
	FillSequence(big, 10);

	BufferedArray<int, 4> bigCopy(big);
	EXPECT_NE(bigCopy.ptr(), big.ptr());
	ASSERT_EQ(bigCopy.numElem(), 10);
	for (int i = 0; i < bigCopy.numElem(); ++i)
		EXPECT_EQ(bigCopy[i], i);
}

TEST(BUFFERED_ARRAY_TESTS, Move)
{
	BufferedArray<int, 4> small(PP_SL);
	FillSequence(small, 3);

	BufferedArray<int, 4> smallMoved(std::move(small));
	EXPECT_TRUE(PointsInsideObject(smallMoved));
	ASSERT_EQ(smallMoved.numElem(), 3);
	EXPECT_EQ(smallMoved[2], 2);
	EXPECT_EQ(small.numElem(), 0);

	BufferedArray<int, 4> big(PP_SL);
	FillSequence(big, 10);
	const int* bigData = big.ptr();

	BufferedArray<int, 4> bigMoved(std::move(big));
	EXPECT_EQ(bigMoved.ptr(), bigData);
	ASSERT_EQ(bigMoved.numElem(), 10);
	EXPECT_EQ(bigMoved[9], 9);
	EXPECT_EQ(big.numElem(), 0);
	EXPECT_TRUE(PointsInsideObject(big));
}

TEST_F(ARRAY_LIFETIME_TESTS, BufferedArrayItems)
{
	{
		BufferedArray<TrackedItem, 2> arr(PP_SL, 2);
		for (int i = 0; i < 6; ++i)
			arr.append(TrackedItem(i));

		EXPECT_EQ(TrackedItem::s_alive, 6);
		for (int i = 0; i < arr.numElem(); ++i)
			EXPECT_EQ(arr[i].value, i);
	}
	EXPECT_EQ(TrackedItem::s_alive, 0);
}

