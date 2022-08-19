/***
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* This samples shows usage of the Concurrency Visualizer C++ markers interface 
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

int SPINCOUNT = 25;

//Spins for a fixed number of loops
#pragma optimize("", off)
void delay()
{
    for(int i=0;i < SPINCOUNT;++i);
};
#pragma optimize("", on)


// This sample shows parallel calculation of Fibonacci numbers
//   and uses markers to make obvious the calculation dynamics

// Random number generation for aq given range
int getrand(int range_min, int range_max)
{
    int u = (int)((double)rand() / (RAND_MAX + 1) * (range_max - range_min) + range_min);
    return u;
}

// Computes the fibonacci number of 'n' serially
int fib(int n)
{
    delay();
    if (n< 2)
        return n;
    int n1, n2;	
    n1 = fib(n-1);
    n2 = fib(n-2);
    return n1 + n2;
}


// Uses ParallelFor for 10 iterations
//     How does the PPL runtime use threads and cores on your system for this calculation? 
//
void CombinableSample()
{
    combinable<int> c;

	// this call is unnecessary in VS2012 and has been deprecated but is left for completeness
    // EnableTracing();

    diagnostic::marker_series ppa_series(L"Fibonacci sample");

    parallel_for(0, 10, [&] (int i) {
		
      // Span will show one iteration
      diagnostic::span span(ppa_series, diagnostic::high_importance, i<5 ? 2 : 4, L"i=%d",i); // separate colors for the first and last 5

      int n = fib(30-i); // calculate a fibonacci number

      // Flag will show the newly-calculated data
      ppa_series.write_flag(L"Fibonacci number for iteration %d is %d", i, n);

      // Accumulates results in a combinable
      c.local() += n;

      Sleep(10);
    });

    int sum = c.combine(std::plus<int>());
	 ppa_series.write_message(diagnostic::high_importance, L"The sum of all Fibonacci is %d", sum);
}



// These 2 samples have no relation to one another
int main()
{
    CombinableSample();
    return 0;
}

