/***
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* This sample shows usage of the Concurrency Visualizer C++ markers interface 
*   together with Parallel Patterns Library (PPL)
* These 2 products are independent, but the first may substantially help user
*   understand the other's behavior - for troubleshooting, performance tuning, or just evaluating 
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include <tchar.h>
#include <windows.h>
#include <ppl.h>
#include <time.h>
#include <cvmarkersobj.h>

using namespace Concurrency;

//
// Banking Sample simulates 2 independent streams of events: transactions and audits.
//    There are 2 bank accounts, having 100$ in total
//    Transactions move some sums between accounts, but the sum should always be 100$
//    At random times there is an audit validating this,
// The "transaction" though is implemented incorrectly - instead of one atomic action
//    it is comprised of 2 independent actions. Audit may catch it or not, depending on random timing.
//  Using markers illustrates how transactions and audits happen in time and makes it easy to 
//    understand the mistake.
// 

// Random number generation for a given range
int getrand(int range_min, int range_max)
{
    int u = (int)((double)rand() / (RAND_MAX + 1) * (range_max - range_min) + range_min);
    return u;
}

// Sample
int main()
{
    const unsigned int total_sum = 100;

    volatile unsigned int account1 = 50;
    volatile unsigned int account2 = 50;

    srand( (unsigned)time( NULL ) );

    diagnostic::marker_series ppa_series(L"Banking sample");		// This timeline will show what happens in the bank

    task_group tg;
    tg.run([&] () {                                                // This tak group implements "transactions", though in a wrong way
        while(true)
        {
            if( tg.is_canceling() )
            {
                return;
            }
            Sleep(100);
            {
				diagnostic::span span(ppa_series, L"transaction");  // shows the span of the "transaction" on the timeline
				InterlockedExchangeSubtract(&account1, 10);			// First part of the action
				Sleep(getrand(50,100));								// imagine that it takes 50 to 100 ms to wire money
				InterlockedExchangeAdd(&account2, 10);				// Seconf part of the action
            }
        }
    });

    tg.run([&]() {                                                  // This task group runs audits concurrently to transactions 
        while(true)
        {
            Sleep(getrand(500,1000));
            unsigned int sum = account1 + account2;
            if( sum == total_sum )
            {
                ppa_series.write_flag(L"audit succeeded" );
            }
            else
            {
                ppa_series.write_flag(diagnostic::high_importance, CvAlertCategory, 
					                     L"audit failed! Sum=%d", sum );		// -1 means Alert Category
				ppa_series.write_message(diagnostic::high_importance,			// Some more details as a footnote 
					                     L"account1=%d, account2=%d, total=%d", account1, account2, total_sum);
																	
                Sleep(100);
                tg.cancel();
                return;
            }
        }
    });

    tg.wait();

    return 0;
}


