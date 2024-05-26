#include <gtest/gtest.h>

#include "core/core_common.h"

static constexpr const int s_FutureErrorCode = 500;
static constexpr const char* s_FutureErrorMessage = "Error Message";
static constexpr const char* s_FutureTestStr = "Testing String Value";
static constexpr int s_FutureTestInt = 0xDEADBEEF;

TEST(FUTURE_TESTS, ErrorCodeMessage)
{
	Promise<bool> testPromise;
	Future<bool> testFuture = testPromise.CreateFuture();

	testPromise.SetError(s_FutureErrorCode, s_FutureErrorMessage);

	EXPECT_EQ(testFuture.GetErrorCode(), s_FutureErrorCode);
	EXPECT_EQ(testFuture.GetErrorMessage(), s_FutureErrorMessage);
}

TEST(FUTURE_TESTS, CallbackOnSetResult)
{
	Promise<bool> testPromise;
	Future<bool> testFuture = testPromise.CreateFuture();

	bool callbackCalled = false;
	testFuture.AddCallback([&](const FutureResult<bool>& result) {
		EXPECT_FALSE(result.IsError());
		callbackCalled = true;
	});

	testPromise.SetResult(true);

	EXPECT_TRUE(callbackCalled);
	EXPECT_TRUE(testFuture.HasResult());
	EXPECT_FALSE(testFuture.HasErrors());
}

TEST(FUTURE_TESTS, CallbackAfterSetResultOnAddCallback)
{
	Promise<bool> testPromise;
	Future<bool> testFuture = testPromise.CreateFuture();

	testPromise.SetResult(true);

	bool callbackCalled = false;
	testFuture.AddCallback([&](const FutureResult<bool>& result) {
		EXPECT_FALSE(result.IsError());
		callbackCalled = true;
	});

	EXPECT_TRUE(callbackCalled);
	EXPECT_TRUE(testFuture.HasResult());
	EXPECT_FALSE(testFuture.HasErrors());
}

TEST(FUTURE_TESTS, CallbackAfterSetError)
{
	Promise<bool> testPromise;
	Future<bool> testFuture = testPromise.CreateFuture();

	bool callbackCalled = false;
	testFuture.AddCallback([&](const FutureResult<bool>& result) {
		EXPECT_TRUE(result.IsError());
		callbackCalled = true;
	});

	testPromise.SetError(s_FutureErrorCode, s_FutureErrorMessage);

	EXPECT_TRUE(callbackCalled);
	EXPECT_TRUE(testFuture.HasResult());
	EXPECT_TRUE(testFuture.HasErrors());
}

TEST(FUTURE_TESTS, CallbackOnSetError)
{
	Promise<bool> testPromise;
	Future<bool> testFuture = testPromise.CreateFuture();

	testPromise.SetError(s_FutureErrorCode, s_FutureErrorMessage);

	bool callbackCalled = false;
	testFuture.AddCallback([&](const FutureResult<bool>& result) {
		EXPECT_TRUE(result.IsError());
		callbackCalled = true;
	});

	EXPECT_TRUE(callbackCalled);
	EXPECT_TRUE(testFuture.HasResult());
	EXPECT_TRUE(testFuture.HasErrors());
	EXPECT_EQ(testFuture.GetErrorCode(), s_FutureErrorCode);
	EXPECT_EQ(testFuture.GetErrorMessage(), s_FutureErrorMessage);
}

TEST(FUTURE_TESTS, Value)
{
	struct TestValue {
		Array<int>	ints;
		EqString	str;
		int			testInt;
	};

	Promise<TestValue> testPromise;
	Future<TestValue> testFuture = testPromise.CreateFuture();

	TestValue abcValue{
		PP_SL,
		s_FutureTestStr,
		s_FutureTestInt,
	};
	abcValue.ints.append(512);
	abcValue.ints.append(640);
	abcValue.ints.append(8192);

	bool callbackCalled = false;
	testFuture.AddCallback([&](const FutureResult<TestValue>& result) {
		callbackCalled = true;
		const TestValue& value = *result;
		EXPECT_EQ(value.ints[0], 512);
		EXPECT_EQ(value.ints[1], 640);
		EXPECT_EQ(value.ints[2], 8192);

		EXPECT_EQ(value.str, s_FutureTestStr);
		EXPECT_EQ(value.testInt, s_FutureTestInt);
	});

	testPromise.SetResult(std::move(abcValue));

	EXPECT_TRUE(callbackCalled);
	EXPECT_TRUE(testFuture.HasResult());
	EXPECT_FALSE(testFuture.HasErrors());
}

TEST(FUTURE_TESTS, DataTransferToOtherPromise)
{
	FutureImpl::FutureData<int>* data = nullptr;

	// create promise inside and set it's value, then grab pointer to data.
	{
		Promise<int> testPromise;
		int value = s_FutureTestInt;
		testPromise.SetResult(std::move(value));

		data = testPromise.GrabDataPtr();

		// should ref_grab inside
		EXPECT_EQ(data->Ref_Count(), 2);
	}

	// Push this data to new promise
	Promise<int> otherPromise(data);
	bool callbackCalled = false;

	{
		Future<int> testFuture = otherPromise.CreateFuture();
		testFuture.AddCallback([&](const FutureResult<int>& result) {
			EXPECT_EQ(*result, s_FutureTestInt);
			callbackCalled = true;
		});
		EXPECT_TRUE(testFuture.HasResult());
		EXPECT_FALSE(testFuture.HasErrors());
	}

	// only otherPromise must own this
	EXPECT_EQ(data->Ref_Count(), 1);
	EXPECT_TRUE(callbackCalled);
}

TEST(FUTURE_TESTS, TestDefaultSuccess)
{
	int testValue = s_FutureTestInt;
	Future<int> testFuture = Future<int>::Succeed(std::move(testValue));
	EXPECT_TRUE(testFuture.HasResult());
	EXPECT_FALSE(testFuture.HasErrors());

	EXPECT_EQ(testFuture.GetResult(), s_FutureTestInt);
}

TEST(FUTURE_TESTS, TestDefaultFailure)
{
	Future<bool> testFuture = Future<bool>::Failure(s_FutureErrorCode, s_FutureErrorMessage);
	EXPECT_TRUE(testFuture.HasResult());
	EXPECT_TRUE(testFuture.HasErrors());

	EXPECT_EQ(testFuture.GetErrorCode(), s_FutureErrorCode);
	EXPECT_EQ(testFuture.GetErrorMessage(), s_FutureErrorMessage);
}