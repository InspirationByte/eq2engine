#include "core/core_common.h"

void FutureTest()
{
	{
		Promise<bool> testPromise;
		Future<bool> testFuture = testPromise.CreateFuture();
		testFuture.AddCallback([](const FutureResult<bool>& result) {
			if (result.IsError())
			{
				MsgInfo("Future 1 test Error: %d %s\n", result.GetErrorCode(), result.GetErrorMessage());
				return;
			}

			const bool value = *result;
			MsgInfo("Future 1 test Result: %d\n", value);
		});

		testPromise.SetResult(true);

		testFuture.AddCallback([](const FutureResult<bool>& result) {
			if (result.IsError())
			{
				MsgInfo("PostCallback: %d %s\n", result.GetErrorCode(), result.GetErrorMessage());
				return;
			}

			const bool value = *result;
			MsgInfo("PostCallback Result: %d\n", value);
		});

		bool test = testFuture.GetResult();
	}
	{
		Promise<bool> testPromise;
		Future<bool> testFuture = testPromise.CreateFuture();
		testFuture.AddCallback([](const FutureResult<bool>& result) {
			if (result.IsError())
			{
				MsgInfo("Future 2 test Error: %d %s\n", result.GetErrorCode(), result.GetErrorMessage());
				return;
			}

			const bool value = *result;
			MsgInfo("Future 2 test Result: %d\n", value);
		});

		testPromise.SetError(500, "Error fail fuck");

		testFuture.AddCallback([](const FutureResult<bool>& result) {
			if (result.IsError())
			{
				MsgInfo("PostCallback: %d %s\n", result.GetErrorCode(), result.GetErrorMessage());
				return;
			}

			const bool value = *result;
			MsgInfo("PostCallback Result: %d\n", value);
		});

		bool test = testFuture.GetResult();
	}
	{
		Future<bool> testFuture1 = Future<bool>::Succeed(true);
	}
	{
		Future<bool> testFuture2 = Future<bool>::Failure(500, "WHUT");
	}
}