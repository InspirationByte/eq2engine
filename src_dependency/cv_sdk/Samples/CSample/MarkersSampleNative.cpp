// MarkersSampleNative.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <crtdbg.h>
#include "cvmarkers.h"

int _tmain(int argc, _TCHAR* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    
    PCV_MARKERSERIES series;
    PCV_PROVIDER provider;
    HRESULT error = CvCreateDefaultMarkerSeriesOfDefaultProvider(&provider, &series);
    _ASSERTE(SUCCEEDED(error));

    error = CvWriteMessage(series, L"Message: Starting, TickCount=%d", GetTickCount64());
    _ASSERTE(SUCCEEDED(error));
    
	Sleep(100);

	error = CvWriteFlagEx(series, CvImportanceCritical, CvAlertCategory, L"Hello world");
    _ASSERTE(SUCCEEDED(error));

	Sleep(120);

	PCV_SPAN span1;
    error = CvEnterSpan(series, &span1, L"My First Span");
    _ASSERTE(SUCCEEDED(error));

	Sleep(100);

	error = CvWriteFlag(series, L"Flag: previos error was %d", error);
    _ASSERTE(SUCCEEDED(error));

	Sleep(300);

	error = CvWriteFlagEx(series, CvImportanceHigh, 1, L"High Importance Flag of category 1");
    _ASSERTE(SUCCEEDED(error));
    
	Sleep(200);

	error = CvLeaveSpan(span1);
    _ASSERTE(SUCCEEDED(error));

    Sleep(200);

	error = CvWriteMessage(series, L"Message: Ending, TickCount=%d", GetTickCount64());
    _ASSERTE(SUCCEEDED(error));
    
	Sleep(100);

    error = CvReleaseMarkerSeries(series);
    _ASSERTE(SUCCEEDED(error));
    
    error = CvReleaseProvider(provider);
    _ASSERTE(SUCCEEDED(error));

    printf("Done!");

	return 0;
}
