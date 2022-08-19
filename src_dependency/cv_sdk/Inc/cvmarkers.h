//-----------------------------------------------------------------------------
//
//  File: cvmarkers.h
//  Copyright (C) 2011 Microsoft Corporation
//  All rights reserved.
//
//  ConcurrencyVisualizer markers API for C language.
//
//-----------------------------------------------------------------------------
#pragma once
#include <windows.h>
#include <evntprov.h>
#include <strsafe.h>

// Supporting both SAL1 and SAL2.
#ifndef _In_reads_bytes_
#define _In_reads_bytes_(size) _In_bytecount_c_(size)
#endif

EXTERN_C __declspec(selectany) const GUID CvDefaultProviderGuid = {0x8d4925ab, 0x505a, 0x483b, {0xa7, 0xe0, 0x6f, 0x82, 0x4a, 0x07, 0xa6, 0xf0}};

EXTERN_C __declspec(selectany) const WCHAR CvDefaultMarkerSeries[] = L"";

EXTERN_C __declspec(selectany) const size_t MaxSeriesNameLengthInChars = 32;

EXTERN_C __declspec(selectany) const int CvAlertCategory = -1;

typedef enum _CV_IMPORTANCE {
    CvImportanceCritical = 1,
    CvImportanceHigh,
    CvImportanceNormal = 4,
    CvImportanceLow

} CV_IMPORTANCE, *PCV_IMPORTANCE;

typedef struct _CV_PROVIDER
{
    PVOID Reserved;
}
CV_PROVIDER, *PCV_PROVIDER;

typedef struct _CV_MARKERSERIES
{
    PVOID Reserved;
}
CV_MARKERSERIES, *PCV_MARKERSERIES;

typedef struct _CV_SPAN
{
    PVOID Reserved;
}
CV_SPAN, *PCV_SPAN;

///<summary>
/// Initializes marker provider. Has to be called before any other CV Marker API functions.
///</summary>
///<param name="pGuid">Provider guid. Cannot be NULL.</param>
///<param name="ppProvider">Address of an output variable which will store provider context. Cannot be NULL.</param>
///<returns>
/// S_OK when provider is successfully initialized or error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
_Check_return_ HRESULT CvInitProvider(
    _In_ const GUID*  pGuid,
    _Out_ PCV_PROVIDER* ppProvider);

///<summary>
/// Releases marker provider. Releasing marker provider will NOT affect previously created marker series of this provider.
/// Marker series have to be released separately by CvReleaseMarkerSeries call. Failure to release provider will cause a memory leak.
///</summary>
///<param name="pProvider">Provider context. Cannot be NULL.</param>
///<returns>
/// S_OK when provider is successfully released or error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
HRESULT CvReleaseProvider(_In_ PCV_PROVIDER pProvider);

///<summary>
/// Creates marker series for a given provider.
///</summary>
///<param name="pProvider">Provider object previously initialized by CvInitProvider. Cannot be NULL.</param>
///<param name="pSeriesName">Marker series name. Cannot be NULL but empty string is allowed.</param>
///<param name="ppMarkerSeries">Address of an output variable which will store marker series context. Cannot be NULL.</param>
///<returns>
/// S_OK when marker series is successfully created or error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
_Check_return_ HRESULT CvCreateMarkerSeriesW(
    _In_ PCV_PROVIDER  pProvider,
    _In_ LPCWSTR pSeriesName,
    _Out_ PCV_MARKERSERIES* ppMarkerSeries);

_Check_return_ HRESULT CvCreateMarkerSeriesA(
    _In_ PCV_PROVIDER  pProvider,
    _In_ LPCSTR pSeriesName,
    _Out_ PCV_MARKERSERIES* ppMarkerSeries);

#ifdef UNICODE
#define CvCreateMarkerSeries CvCreateMarkerSeriesW
#else
#define CvCreateMarkerSeries CvCreateMarkerSeriesA
#endif // !UNICODE

///<summary>
/// Creates marker series for a given provider and specifed code page.
/// This function can be used to specify the code page explicitly for
/// the text written out by marker API ANSI functions. Setting the code
/// page can be useful in case the trace is captured and then analyzed on
/// different machines with different locales/languages.
/// By default the code page returned by GetACP() function is used.
///</summary>
///<param name="pProvider">Provider object previously initialized by CvInitProvider. Cannot be NULL.</param>
///<param name="pSeriesName">Marker series name. Cannot be NULL but empty string is allowed.</param>
///<param name="nTextCodePage">Valid code page.</param>
///<param name="ppMarkerSeries">Address of an output variable which will store marker series context. Cannot be NULL.</param>
///<returns>
/// S_OK when marker series is successfully created or error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
_Check_return_ HRESULT CvCreateMarkerSeriesWithCodePageA(
    _In_ PCV_PROVIDER  pProvider,
    _In_ LPCSTR pSeriesName,
    _In_ UINT nTextCodePage,
    _Out_ PCV_MARKERSERIES* ppMarkerSeries);

///<summary>
/// Creates default marker series of a default provider. This default series should be used 
/// when you need only generate events from a single series under the default provider.
///</summary>
///<param name="ppProvider">Address of provider object variable. Address cannot be NULL, the variable can have any value.</param>
///<param name="ppMarkerSeries">Address of marker series object variable. Address cannot be NULL, the variable can have any value.</param>
///<returns>
/// S_OK when both provider and marker series are successfully created or error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
///<example> The sample below shows how to initialize default marker provider and series.
///<code>
/// PCV_MARKERSERIES series;
/// PCV_PROVIDER provider;
/// HRESULT error = CvCreateDefaultMarkerSeriesOfDefaultProvider(&provider, &series);
/// if (FAILED(error))
/// {
///     printf("Failed to initialize default marker series: 0x%x", error);
///     return -1;
/// }
/// // Now use the series to log events.
/// // more code...
/// // Don't forget to release objects later.
/// error = CvReleaseMarkerSeries(series);
/// _ASSERTE(SUCCEEDED(error));
/// error = CvReleaseProvider(provider);
/// _ASSERTE(SUCCEEDED(error));
///<code>
///</example>
_Check_return_ HRESULT CvCreateDefaultMarkerSeriesOfDefaultProvider(
    _Out_ PCV_PROVIDER* ppProvider,
    _Out_ PCV_MARKERSERIES* ppMarkerSeries);

///<summary>
/// Releases marker series. Do not use marker series object after releasing otherwise the application might crash.
/// Failure to release marker series causes a memory leak.
///</summary>
///<param name="pMarkerSeries">Address of provider object variable. Address cannot be NULL, the variable can have any value.</param>
///<returns>
/// S_OK when marker series is successfully released or error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
HRESULT CvReleaseMarkerSeries(_In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries);

///<summary>
/// Determines whether any session has enabled the markers ETW provider.
/// You generally do not need to call this method as the enabled/disabled 
/// check is performed in Markers API implementation very efficiently at
/// the early stage. However, if your performance analysis shows that
/// overhead from Markers API is big even when provider is not enabled,
/// you may try using this method in your code.
///</summary>
///<param name="pProvider">Valid provider object. Cannot be NULL.</param>
///<returns>
/// S_OK if provider is currently enabled.
/// S_FALSE if provider is currently disabled.
/// Error code in case there were any errors.
/// Use FAILED macro to check for error condition and then check for S_OK/S_FALSE.
///</returns>
HRESULT CvIsEnabled(_In_ PCV_PROVIDER pProvider);

///<summary>
/// Determines whether any session has enabled the provider with the
/// specified level and category.
/// You generally do not need to call this method as the enabled/disabled 
/// check is performed in Markers API implementation very efficiently at
/// the early stage. However, if your performance analysis shows that
/// overhead from Markers API is big even when provider is not enabled,
/// you may try using this method in your code.
///</summary>
///<param name="pProvider">Valid provider object. Cannot be NULL.</param>
///<param name="level">Minimum importance level.</param>
///<param name="category">Category.</param>
///<returns>
/// S_OK if provider is currently enabled.
/// S_FALSE if provider is currently disabled.
/// Error code in case there were any errors.
/// Use FAILED macro to check for error condition and then check for S_OK/S_FALSE.
///</returns>
HRESULT CvIsEnabledEx(
    _In_ PCV_PROVIDER pProvider,
    _In_ CV_IMPORTANCE level,
    _In_ int category);

///<summary>
/// Drops span Enter event with the specified string.
///</summary>
///<param name="pMarkerSeries">Valid marker series context. Cannot be NULL.</param>
///<param name="ppSpan">Address of the variable which will hold resulting span object. Address cannot be NULL, the variable can have any value.</param>
///<param name="pMessage">Message format string. Cannot be NULL.</param>
///<returns>
/// S_OK when the message is successfully written.
/// Error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
///<remarks>
///<para> 
/// ANSI version of the function is CvEnterSpanA. You may use CvEnterSpan which is 
/// defined either as CvEnterSpanW or CvEnterSpanA depending on whether your project
/// is built as Unicode or not.
///</para>
///<para> 
/// In case you need a version of the function which accepts a pointer to an argument list (va_list), 
/// use CvEnterSpanV/CvEnterSpanVW/CvEnterSpanVA.
///</para>
///</remarks>
///<example> The code below shows how to open and close span.
///<code>
/// // series is previously initialized PCV_MARKERSERIES object.
/// PCV_SPAN span1;
/// error = CvEnterSpan(series, &span1, _T("Entering span %d..."), 1);
/// _ASSERTE(SUCCEEDED(error));
/// 
/// error = CvWriteMessage(series, _T("Message: %d"), 10);
/// _ASSERTE(SUCCEEDED(error));
/// 
/// error = CvLeaveSpan(span1);
/// _ASSERTE(SUCCEEDED(error));
///<code>
///</example>
HRESULT CvEnterSpanW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCWSTR pMessage,	
    ...
    );

HRESULT CvEnterSpanA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCSTR pMessage,
    ...
    );

#ifdef UNICODE
#define CvEnterSpan CvEnterSpanW
#else
#define CvEnterSpan CvEnterSpanA
#endif // !UNICODE

HRESULT CvEnterSpanVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCWSTR pMessage,	
    _In_ va_list argList
    );

HRESULT CvEnterSpanVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCSTR pMessage,
    _In_ va_list argList
    );

#ifdef UNICODE
#define CvEnterSpanV CvEnterSpanVW
#else
#define CvEnterSpanV CvEnterSpanVA
#endif // !UNICODE

///<summary>
/// Drops span Enter event with the specified string, level and category.
///</summary>
///<param name="pMarkerSeries">Valid marker series context. Cannot be NULL.</param>
///<param name="level">Importance level.</param>
///<param name="category">Category.</param>
///<param name="ppSpan">Address of the variable which will hold resulting span object. Address cannot be NULL, the variable can have any value.</param>
///<param name="pMessage">Message format string. Cannot be NULL.</param>
///<returns>
/// S_OK when the message is successfully written.
/// Error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
///<remarks>
///<para> 
/// ANSI version of the function is CvEnterSpanExA. You may use CvEnterSpanEx which is 
/// defined either as CvEnterSpanExW or CvEnterSpanExA depending on whether your project
/// is built as Unicode or not.
///</para>
///<para> 
/// In case you need a version of the function which accepts a pointer to an argument list (va_list), 
/// use CvEnterSpanExV/CvEnterSpanExV/CvEnterSpanExV.
///</para>
///</remarks>
HRESULT CvEnterSpanExW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCWSTR pMessage,
    ...
    );

HRESULT CvEnterSpanExA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCSTR pMessage,
    ...
    );

#ifdef UNICODE
#define CvEnterSpanEx CvEnterSpanExW
#else
#define CvEnterSpanEx CvEnterSpanExA
#endif // !UNICODE

HRESULT CvEnterSpanExVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCWSTR pMessage,
    _In_ va_list argList);

HRESULT CvEnterSpanExVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCSTR pMessage,
    _In_ va_list argList);

#ifdef UNICODE
#define CvEnterSpanExV CvEnterSpanExVW
#else
#define CvEnterSpanExV CvEnterSpanExVA
#endif // !UNICODE

///<summary>
/// Drops span Leave event. <see cref="CvEnterSpanW"> for the example of using span.
///</summary>
///<param name="pSpan">Span object returned by previous call to CvEnterSpan*. Cannot be NULL.</param>
///<returns>
/// S_OK when the message is successfully written.
/// Error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
HRESULT CvLeaveSpan(_In_ PCV_SPAN pSpan);

///<summary>
/// Drops Flag event with the specified string.
///</summary>
///<param name="pMarkerSeries">Valid marker series context. Cannot be NULL.</param>
///<param name="pMessage">Message format string. Cannot be NULL.</param>
///<returns>
/// S_OK when the message is successfully written.
/// Error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
///<remarks>
///<para> 
/// ANSI version of the function is CvWriteFlagA. You may use CvWriteFlag which is 
/// defined either as CvWriteFlagW or CvWriteFlagA depending on whether your project
/// is built as Unicode or not.
///</para>
///<para> 
/// In case you need a version of the function which accepts a pointer to an argument list (va_list), 
/// use CvWriteFlagV/CvWriteFlagVW/CvWriteFlagVA.
///</para>
///</remarks>
///<example> The code below shows how to drop flag event.
///<code>
/// // series is previously initialized PCV_MARKERSERIES object.
/// error = CvWriteFlag(series, _T("Flag: %d"), 100);
/// _ASSERTE(SUCCEEDED(error));
///<code>
///</example>
HRESULT CvWriteFlagW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    ...
    );

HRESULT CvWriteFlagA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    ...
    );

#ifdef UNICODE
#define CvWriteFlag CvWriteFlagW
#else
#define CvWriteFlag CvWriteFlagA
#endif // !UNICODE

HRESULT CvWriteFlagVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    _In_ va_list argList);

HRESULT CvWriteFlagVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    _In_ va_list argList);

#ifdef UNICODE
#define CvWriteFlagV CvWriteFlagVW
#else
#define CvWriteFlagV CvWriteFlagVA
#endif // !UNICODE

///<summary>
/// Drops Flag event with the specified string, level and category.
///</summary>
///<param name="pMarkerSeries">Valid marker series context. Cannot be NULL.</param>
///<param name="level">Importance level.</param>
///<param name="category">Category.</param>
///<param name="pMessage">Message format string. Cannot be NULL.</param>
///<returns>
/// S_OK when the message is successfully written.
/// Error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
///<remarks>
///<para> 
/// ANSI version of the function is CvWriteFlagExA. You may use CvWriteFlagEx which is 
/// defined either as CvWriteFlagExW or CvWriteFlagExA depending on whether your project
/// is built as Unicode or not.
///</para>
///<para> 
/// In case you need a version of the function which accepts a pointer to an argument list (va_list), 
/// use CvWriteFlagExV/CvWriteFlagExVW/CvWriteFlagExVA.
///</para>
///</remarks>
HRESULT CvWriteFlagExW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCWSTR pMessage,
    ...
    );

HRESULT CvWriteFlagExA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCSTR pMessage,
    ...
    );

#ifdef UNICODE
#define CvWriteFlagEx CvWriteFlagExW
#else
#define CvWriteFlagEx CvWriteFlagExA
#endif // !UNICODE

HRESULT CvWriteFlagExVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCWSTR pMessage,
    _In_ va_list argList);

HRESULT CvWriteFlagExVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCSTR pMessage,
    _In_ va_list argList);

#ifdef UNICODE
#define CvWriteFlagExV CvWriteFlagExVW
#else
#define CvWriteFlagExV CvWriteFlagExVA
#endif // !UNICODE

///<summary>
/// Drops Alert event with the specified string.
///</summary>
///<param name="pMarkerSeries">Valid marker series context. Cannot be NULL.</param>
///<param name="pMessage">Message format string. Cannot be NULL.</param>
///<returns>
/// S_OK when the message is successfully written.
/// Error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
///<remarks>
///<para> 
/// ANSI version of the function is CvWriteAlertA. You may use CvWriteAlert which is 
/// defined either as CvWriteAlertW or CvWriteAlertA depending on whether your project
/// is built as Unicode or not.
///</para>
///<para> 
/// In case you need a version of the function which accepts a pointer to an argument list (va_list), 
/// use CvWriteAlertV/CvWriteAlertVW/CvWriteAlertVA.
///</para>
///</remarks>
HRESULT CvWriteAlertW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    ...
    );

HRESULT CvWriteAlertA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    ...
    );

#ifdef UNICODE
#define CvWriteAlert CvWriteAlertW
#else
#define CvWriteAlert CvWriteAlertA
#endif // !UNICODE

HRESULT CvWriteAlertVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    _In_ va_list argList);

HRESULT CvWriteAlertVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    _In_ va_list argList);

#ifdef UNICODE
#define CvWriteAlertV CvWriteAlertVW
#else
#define CvWriteAlertV CvWriteAlertVA
#endif // !UNICODE

///<summary>
/// Drops Message event with the specified string.
///</summary>
///<param name="pMarkerSeries">Valid marker series context. Cannot be NULL.</param>
///<param name="pMessage">Message format string. Cannot be NULL.</param>
///<returns>
/// S_OK when the message is successfully written.
/// Error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
///<remarks>
///<para> 
/// ANSI version of the function is CvWriteMessageA. You may use CvWriteMessage which is 
/// defined either as CvWriteMessageW or CvWriteMessageA depending on whether your project
/// is built as Unicode or not.
///</para>
///<para> 
/// In case you need a version of the function which accepts a pointer to an argument list (va_list), 
/// use CvWriteMessageV/CvWriteMessageVW/CvWriteMessageVA.
///</para>
///</remarks>
///<example> The code below shows how to drop message event.
///<code>
/// // series is previously initialized PCV_MARKERSERIES object.
/// error = CvWriteMessage(series, _T("Message: %d"), 10);
/// _ASSERTE(SUCCEEDED(error));
///<code>
///</example>
HRESULT CvWriteMessageW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    ...
    );

HRESULT CvWriteMessageA(
    PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    ...
    );

#ifdef UNICODE
#define CvWriteMessage CvWriteMessageW
#else
#define CvWriteMessage CvWriteMessageA
#endif // !UNICODE

HRESULT CvWriteMessageVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    _In_ va_list argList);

HRESULT CvWriteMessageVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    _In_ va_list argList);

#ifdef UNICODE
#define CvWriteMessageV CvWriteMessageVW
#else
#define CvWriteMessageV CvWriteMessageVA
#endif // !UNICODE

///<summary>
/// Drops Message event with the specified string, level and category.
///</summary>
///<param name="pMarkerSeries">Valid marker series context. Cannot be NULL.</param>
///<param name="level">Importance level.</param>
///<param name="category">Category.</param>
///<param name="pMessage">Message format string. Cannot be NULL.</param>
///<returns>
/// S_OK when the message is successfully written.
/// Error code in case there were any errors.
/// Use SUCCEEDED/FAILED macros to check for error condition.
///</returns>
///<remarks>
///<para> 
/// ANSI version of the function is CvWriteMessageExA. You may use CvWriteMessageEx which is 
/// defined either as CvWriteMessageExW or CvWriteMessageExA depending on whether your project
/// is built as Unicode or not.
///</para>
///<para> 
/// In case you need a version of the function which accepts a pointer to an argument list (va_list), 
/// use CvWriteMessageExV/CvWriteMessageExVW/CvWriteMessageExVA.
///</para>
///</remarks>
HRESULT CvWriteMessageExW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCWSTR pMessage,
    ...
    );

HRESULT CvWriteMessageExA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCSTR pMessage,
    ...
    );

#ifdef UNICODE
#define CvWriteMessageEx CvWriteMessageExW
#else
#define CvWriteMessageEx CvWriteMessageExA
#endif // !UNICODE

HRESULT CvWriteMessageExVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCWSTR pMessage,
    _In_ va_list argList);

HRESULT CvWriteMessageExVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCSTR pMessage,
    _In_ va_list argList);

#ifdef UNICODE
#define CvWriteMessageExV CvWriteMessageExVW
#else
#define CvWriteMessageExV CvWriteMessageExVA
#endif // !UNICODE

//---------------------------------------------------------
// Implementation of Markers API.

#ifndef EVENT_CONTROL_CODE_DISABLE_PROVIDER
#define EVENT_CONTROL_CODE_DISABLE_PROVIDER 0
#endif

#define CvDefaultCategory 0

#define CvDefaultFlagCategory 1

#define CvMaxTextLengthInChars 8192

#define CvManifestTemplateArgumentCount 2

#define CvSpanEventArgumentCount 7
#define CvFlagOrMessageArgumentCount 6

#define CvEnterSpanEventId 1
#define CvLeaveSpanEventId 2
#define CvFlagEventId 3
#define CvMessageEventId 4

EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR CvEnterSpanEvent = {CvEnterSpanEventId, 0x1, 0x10, 0x4, 0x1, 0x1, 0x8000000000000000};
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR CvLeaveSpanEvent = {CvLeaveSpanEventId, 0x1, 0x10, 0x4, 0x2, 0x1, 0x8000000000000000};
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR CvFlagEvent = {CvFlagEventId, 0x1, 0x10, 0x4, 0x0, 0x2, 0x8000000000000000};
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR CvMessageEvent = {CvMessageEventId, 0x1, 0x10, 0x4, 0x0, 0x3, 0x8000000000000000};

typedef struct _CV_PROVIDER_CONTEXT
{
    GUID ProviderGuid;          // ETW provider GUID
    REGHANDLE ProviderHandle;   // ETW provider handle
    ULONG Enabled;              // Is provider enabled.
    UCHAR Level;                // Current provider level.
    ULONGLONG MatchAnyKeyword;  // See MSDN for EnableCallback.
    ULONGLONG MatchAllKeywords; // 
    LPSTR Manifest;             // Provider manifest.
    size_t cbManifestLength;
    LONG ManifestWritten;
    LONG CurrentSpanId;

} CV_PROVIDER_CONTEXT, *PCV_PROVIDER_CONTEXT;

typedef struct _CV_MARKERSERIES_CONTEXT
{
    PCV_PROVIDER_CONTEXT ProviderContext;
    LPWSTR SeriesName;
    size_t SeriesNameLength;
    UINT CodePage;
    
} CV_MARKERSERIES_CONTEXT, *PCV_MARKERSERIES_CONTEXT;

typedef struct _CV_SPAN_CONTEXT
{
    PCV_MARKERSERIES_CONTEXT SeriesContext;
    LONG SpanId;
    int Category;
    BYTE Level;

} CV_SPAN_CONTEXT, *PCV_SPAN_CONTEXT;

typedef struct _CV_MANIFEST_ENVELOPE
{
    BYTE Format;
    BYTE MajorVersion;
    BYTE MinorVersion;
    BYTE Magic;
    USHORT TotalChunks;
    USHORT ChunkNumber;
} CV_MANIFEST_ENVELOPE, *PCV_MANIFEST_ENVELOPE;

__inline ULONGLONG CvGetKeywordsFromCategory(int category)
{
    const int AvailableBitsInEtwKeyword = 48;
    int bitPos = CvAlertCategory == category
        ? 62
        : ((category < 0 ? 0 : category) % AvailableBitsInEtwKeyword);
    return 0x8000000000000000ui64 | (1ui64 << bitPos);
}

__inline HRESULT CvIsEnabledInternal(PCV_PROVIDER_CONTEXT context)
{
    return context->Enabled ? S_OK : S_FALSE;
}

__inline HRESULT CvIsEnabledExInternal(PCV_PROVIDER_CONTEXT context, CV_IMPORTANCE level, int category)
{
    ULONGLONG keywords;
    if (0 == context->Enabled)
    {
        return S_FALSE;
    }

    keywords = CvGetKeywordsFromCategory(category);
    return 
        ((level <= context->Level || context->Level == 0) &&
            ((keywords == 0L) || (((keywords & context->MatchAnyKeyword) != 0L) && ((keywords & context->MatchAllKeywords) == context->MatchAllKeywords))))
        ? S_OK
        : S_FALSE;
}

__inline HRESULT CvFormatMessageW(
    _In_ PWSTR cacheBuffer,
    _In_ size_t cchCacheBufferLength,
    _Out_ PWSTR* heapBuffer,
    _Out_ size_t* cchTextLength,
    _In_ PCWSTR message,
    _In_ va_list argList)
{
    size_t cchRemaining;
    int charsRequired;

    *cchTextLength = 0;
    *heapBuffer = NULL;

    if (SUCCEEDED(StringCchVPrintfExW(cacheBuffer, cchCacheBufferLength, NULL, &cchRemaining, 0, message, argList)))
    {
        *cchTextLength = cchCacheBufferLength - cchRemaining;
        return S_OK;
    }

    charsRequired = _vscwprintf(message, argList);
    if (charsRequired < 0)
    {
        // Give up.
        return E_INVALIDARG;
    }

    if (charsRequired > CvMaxTextLengthInChars)
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    // Include terminating null char.
    ++charsRequired;
    *heapBuffer = (PWSTR)malloc(charsRequired * sizeof(WCHAR));
    if (NULL == *heapBuffer)
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(StringCchVPrintfExW(*heapBuffer, charsRequired, NULL, &cchRemaining, 0, message, argList)))
    {
        free(*heapBuffer);
        *heapBuffer = NULL;
        // Give up.
        return E_INVALIDARG;
    }

    *cchTextLength = charsRequired - cchRemaining;

    return S_OK;
}

__inline HRESULT CvFormatMessageA(
    _In_ UINT codePage,
    _In_ PSTR cacheBuffer,
    _In_ size_t cchCacheBufferLength,
    _Out_ PSTR* heapBuffer,
    _Out_ size_t* cchTextLength,
    _In_ PCSTR message,
    _In_ va_list argList)
{
    int cchCodePageLength;
    size_t cchRemaining;
    int charsRequired;
    // 8 chars are required to represent 32-bit code page id value.
    cchCodePageLength = 8;
    *cchTextLength = 0;
    *heapBuffer = NULL;

    if (SUCCEEDED(StringCchPrintfA(cacheBuffer, cchCacheBufferLength, "%08x", codePage)) &&
        SUCCEEDED(StringCchVPrintfExA(cacheBuffer + cchCodePageLength, cchCacheBufferLength - cchCodePageLength, NULL, &cchRemaining, 0, message, argList)))
    {
        *cchTextLength = cchCacheBufferLength - cchRemaining;
        return S_OK;
    }
     
    charsRequired = _vscprintf(message, argList);
    if (charsRequired < 0)
    {
        // Give up.
        return E_INVALIDARG;
    }

    charsRequired += cchCodePageLength;

    if (charsRequired > CvMaxTextLengthInChars)
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    // Include terminating null char.
    ++charsRequired;
    *heapBuffer = (PSTR)malloc(charsRequired * sizeof(WCHAR));
    if (NULL == *heapBuffer)
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(StringCchPrintfA(*heapBuffer, charsRequired, "%08x", codePage)) ||
        FAILED(StringCchVPrintfExA(*heapBuffer + cchCodePageLength, charsRequired - cchCodePageLength, NULL, &cchRemaining, 0, message, argList)))
    {
        free(*heapBuffer);
        *heapBuffer = NULL;
        // Give up.
        return E_INVALIDARG;
    }

    *cchTextLength = charsRequired - cchRemaining;

    return S_OK;
}

__inline ULONG CvWriteManifestEvent(_In_ PCV_PROVIDER_CONTEXT context)
{
    EVENT_DATA_DESCRIPTOR EventData[CvManifestTemplateArgumentCount];
    EVENT_DESCRIPTOR descriptor = { 0xFFFE, 1, 0, 0, 0xFE, 0, ULLONG_MAX };
    CV_MANIFEST_ENVELOPE envelope = { 1, 1, 0, 0x5B, 1, 0};

    EventDataDescCreate(&EventData[0], &envelope, sizeof(CV_MANIFEST_ENVELOPE));

    EventDataDescCreate(&EventData[1], context->Manifest, (ULONG)context->cbManifestLength);

    return EventWrite(context->ProviderHandle, &descriptor, CvManifestTemplateArgumentCount, EventData);
}

__inline ULONG CvWriteMarkerEventToEtw(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR pDescriptor,
    _In_ BYTE uLevel,
    _In_ BYTE uCategory,
    _In_ LONG spanId,
    _In_ PCWSTR pMarkerSeries,
    _In_ size_t cchMarkerSeriesLength,
    _In_opt_ PCWSTR pTextW,
    _In_ size_t cchTextWLength,
    _In_opt_ PCSTR pTextA,
    _In_ size_t cchTextALength)
{
    #define MaxArgumentCount 7

    ULONG argumentCount;
    size_t textFieldsOffset;
    EVENT_DATA_DESCRIPTOR EventData[MaxArgumentCount];
#pragma warning(push)
// nonstandard extension used in C.
#pragma warning(disable:4204)
    EVENT_DESCRIPTOR descriptor =
    {
        pDescriptor->Id,
        pDescriptor->Version,
        pDescriptor->Channel,
        uLevel,
        pDescriptor->Opcode,
        pDescriptor->Task,
        CvGetKeywordsFromCategory(uCategory)
    };
#pragma warning(pop)

    EventDataDescCreate(&EventData[0], &pDescriptor->Id, sizeof(BYTE));
    EventDataDescCreate(&EventData[1], &uLevel, sizeof(BYTE));
    EventDataDescCreate(&EventData[2], &uCategory, sizeof(BYTE));
    if (CvEnterSpanEventId == descriptor.Id ||
        CvLeaveSpanEventId == descriptor.Id)
    {
        argumentCount = CvSpanEventArgumentCount;
        EventDataDescCreate(&EventData[3], &spanId, sizeof(LONG));
        textFieldsOffset = 4;
    }
    else
    {
        argumentCount = CvFlagOrMessageArgumentCount;
        textFieldsOffset = 3;
    }

    EventDataDescCreate(
        &EventData[textFieldsOffset],
        (pMarkerSeries != NULL) ? pMarkerSeries : CvDefaultMarkerSeries,
        (pMarkerSeries != NULL) ? (ULONG)((cchMarkerSeriesLength + 1) * sizeof(WCHAR)) : (ULONG)sizeof(CvDefaultMarkerSeries));

    EventDataDescCreate(
        &EventData[textFieldsOffset + 1], 
        (pTextW != NULL) ? pTextW : L"",
        (pTextW != NULL) ? (ULONG)((cchTextWLength + 1) * sizeof(WCHAR)) : (ULONG)sizeof(L""));

    EventDataDescCreate(
        &EventData[textFieldsOffset + 2], 
        (pTextA != NULL) ? pTextA : "",
        (pTextA != NULL) ? (ULONG)((cchTextALength + 1) * sizeof(CHAR)) : (ULONG)sizeof(""));

    return EventWrite(RegHandle, &descriptor, argumentCount, EventData);
}

__inline HRESULT CvWriteEventW(
    _In_ PCV_MARKERSERIES_CONTEXT context,
    _In_ PCEVENT_DESCRIPTOR pDescriptor,
    _In_ BYTE level,
    _In_ int category,
    _In_ ULONG spanId,
    _In_ PCWSTR pMessage,
    _In_ va_list argList)
{
    WCHAR stackBuffer[64];
    size_t cchTextLength;
    HRESULT error;
    PWSTR heapBuffer = NULL;

    if (NULL == context)
    {
        return E_INVALIDARG;
    }

    if (InterlockedCompareExchange(&context->ProviderContext->ManifestWritten, 1, 0) == 0)
    {
		CvWriteManifestEvent(context->ProviderContext);
    }

    // Assuming most of the messages will be relatively short so 
    // stack-allocated buffer can be used without adding much 
    // overhead to stack size.
    stackBuffer[0] = L'\0';
    error = CvFormatMessageW(stackBuffer, _countof(stackBuffer), &heapBuffer, &cchTextLength, pMessage, argList);
    if (FAILED(error))
    {
        return error;
    }

    error = HRESULT_FROM_WIN32(CvWriteMarkerEventToEtw(
        context->ProviderContext->ProviderHandle,
        pDescriptor,
        level,
        (BYTE)category,
        spanId,
        context->SeriesName,
        context->SeriesNameLength,
        (NULL == heapBuffer) ? stackBuffer : heapBuffer,
        cchTextLength,
        NULL,
        0));

    if (NULL != heapBuffer)
    {
        free(heapBuffer);
    }

    return error;
}

__inline HRESULT CvWriteEventA(
    _In_ PCV_MARKERSERIES_CONTEXT context,
    _In_ PCEVENT_DESCRIPTOR pDescriptor,
    _In_ BYTE level,
    _In_ int category,
    _In_ ULONG spanId,
    _In_ PCSTR pMessage,
    _In_ va_list argList)
{
    CHAR stackBuffer[64];
    size_t cchTextLength;
    HRESULT error;
    PSTR heapBuffer = NULL;

    if (NULL == context)
    {
        return E_INVALIDARG;
    }

    if (InterlockedCompareExchange(&context->ProviderContext->ManifestWritten, 1, 0) == 0)
    {
        CvWriteManifestEvent(context->ProviderContext);
    }

    // Assuming most of the messages will be relatively short so 
    // stack-allocated buffer can be used without adding much 
    // overhead to stack size.
    stackBuffer[0] = '\0';
    error = CvFormatMessageA(context->CodePage, stackBuffer, _countof(stackBuffer), &heapBuffer, &cchTextLength, pMessage, argList);
    if (FAILED(error))
    {
        return error;
    }
    
    error = HRESULT_FROM_WIN32(CvWriteMarkerEventToEtw(
        context->ProviderContext->ProviderHandle,
        pDescriptor,
        level,
        (BYTE)category,
        spanId,
        context->SeriesName,
        context->SeriesNameLength,
        NULL,
        0,
        (NULL == heapBuffer) ? stackBuffer : heapBuffer,
        cchTextLength));

    if (NULL != heapBuffer)
    {
        free(heapBuffer);
    }

    return error;
}

void NTAPI CvProviderEnableCallback(
    LPCGUID SourceId,
    ULONG IsEnabled,
    UCHAR Level,
    ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeywords,
    PEVENT_FILTER_DESCRIPTOR FilterData,
    PVOID CallbackContext);

EXTERN_C __declspec(selectany) const CHAR CvMarkersManifestTemplate[] = 
"<instrumentationManifest\r\n"
"    xmlns=\"http://schemas.microsoft.com/win/2004/08/events\"\r\n"
"    xmlns:win=\"http://manifests.microsoft.com/win/2004/08/windows/events\"\r\n"
"    xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\r\n"
"  <instrumentation>\r\n"
"    <events>\r\n"
"      <provider\r\n"
"        name=\"ConcurrencyVisualizerMarkers\"\r\n"
"        guid=\"%s\"\r\n"
"        messageFileName=\"\"\r\n"
"        resourceFileName=\"\"\r\n"
"        symbol=\"ConcurrencyVisualizerMarkers\">\r\n"
"        <tasks>\r\n"
"          <task name=\"Span\" value=\"1\" />\r\n"
"          <task name=\"Flag\" value=\"2\" />\r\n"
"          <task name=\"Message\" value=\"3\" />\r\n"
"        </tasks>\r\n"
"        <opcodes>\r\n"
"          <opcode name=\"WriteFlag\" message=\"$(string.opcode_WriteFlag)\" value=\"11\" />\r\n"
"          <opcode name=\"WriteMessage\" message=\"$(string.opcode_WriteMessage)\" value=\"12\" />\r\n"
"        </opcodes>\r\n"
"        <events>\r\n"
"          <event symbol=\"EnterSpan\" value=\"1\" version=\"1\" level=\"win:Informational\" task=\"Span\" opcode=\"win:Start\" template=\"EnterSpanArgs\" />\r\n"
"          <event symbol=\"LeaveSpan\" value=\"2\" version=\"1\" level=\"win:Informational\" task=\"Span\" opcode=\"win:Stop\" template=\"LeaveSpanArgs\" />\r\n"
"          <event symbol=\"WriteFlag\" value=\"3\" version=\"1\" level=\"win:Informational\" task=\"Flag\" opcode=\"WriteFlag\" template=\"WriteFlagArgs\" />\r\n"
"          <event symbol=\"WriteMessage\" value=\"4\" version=\"1\" level=\"win:Informational\" task=\"Message\" opcode=\"WriteMessage\" template=\"WriteMessageArgs\" />\r\n"
"        </events>\r\n"
"        <templates>\r\n"
"          <template tid=\"EnterSpanArgs\">\r\n"
"            <data name=\"cvType\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvImportance\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvCategory\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvSpanId\" inType=\"win:Int32\" />\r\n"
"            <data name=\"cvSeries\" inType=\"win:UnicodeString\" />\r\n"
"            <data name=\"cvTextW\" inType=\"win:UnicodeString\" />\r\n"
"            <data name=\"cvTextA\" inType=\"win:AnsiString\" />\r\n"
"          </template>\r\n"
"          <template tid=\"LeaveSpanArgs\">\r\n"
"            <data name=\"cvType\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvImportance\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvCategory\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvSpanId\" inType=\"win:Int32\" />\r\n"
"            <data name=\"cvSeries\" inType=\"win:UnicodeString\" />\r\n"
"            <data name=\"cvTextW\" inType=\"win:UnicodeString\" />\r\n"
"            <data name=\"cvTextA\" inType=\"win:AnsiString\" />\r\n"
"          </template>\r\n"
"          <template tid=\"WriteFlagArgs\">\r\n"
"            <data name=\"cvType\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvImportance\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvCategory\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvSeries\" inType=\"win:UnicodeString\" />\r\n"
"            <data name=\"cvTextW\" inType=\"win:UnicodeString\" />\r\n"
"            <data name=\"cvTextA\" inType=\"win:AnsiString\" />\r\n"
"          </template>\r\n"
"          <template tid=\"WriteMessageArgs\">\r\n"
"            <data name=\"cvType\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvImportance\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvCategory\" inType=\"win:UInt8\" />\r\n"
"            <data name=\"cvSeries\" inType=\"win:UnicodeString\" />\r\n"
"            <data name=\"cvTextW\" inType=\"win:UnicodeString\" />\r\n"
"            <data name=\"cvTextA\" inType=\"win:AnsiString\" />\r\n"
"          </template>\r\n"
"        </templates>\r\n"
"      </provider>\r\n"
"    </events>\r\n"
"  </instrumentation>\r\n"
"  <localization>\r\n"
"    <resources culture=\"en-US\">\r\n"
"      <stringTable>\r\n"
"        <string id=\"opcode_WriteFlag\" value=\"WriteFlag\"/>\r\n"
"        <string id=\"opcode_WriteMessage\" value=\"WriteMessage\"/>\r\n"
"      </stringTable>\r\n"
"    </resources>\r\n"
"  </localization>\r\n"
"</instrumentationManifest> ";

__inline _Check_return_ HRESULT CvInitProvider(_In_ const GUID* pGuid, _Out_ PCV_PROVIDER* ppProvider)
{
    PCV_PROVIDER_CONTEXT context;
    ULONG result;
    CHAR guidString[40];
    size_t cbMaxManifestLength;

    if (NULL == pGuid || NULL == ppProvider)
    {
        return E_INVALIDARG;
    }

    *ppProvider = NULL;

    if (FAILED(StringCbPrintfA(
        guidString,
        sizeof(guidString),
        "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        pGuid->Data1, pGuid->Data2, pGuid->Data3,  
        pGuid->Data4[0], pGuid->Data4[1],  
        pGuid->Data4[2], pGuid->Data4[3],  
        pGuid->Data4[4], pGuid->Data4[5],  
        pGuid->Data4[6], pGuid->Data4[7])))
    {
        return E_INVALIDARG;
    }

    context = (PCV_PROVIDER_CONTEXT)_aligned_malloc(sizeof(CV_PROVIDER_CONTEXT), 32);
    if (NULL == context)
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(context, sizeof(CV_PROVIDER_CONTEXT));

    context->CurrentSpanId = _I32_MIN;

    if (0 != memcpy_s(&context->ProviderGuid, sizeof(context->ProviderGuid), pGuid, sizeof(GUID)))
    {
        _aligned_free(context);
        return E_INVALIDARG;
    }

    cbMaxManifestLength = sizeof(CvMarkersManifestTemplate) + sizeof(guidString);
    context->Manifest = (LPSTR)malloc(cbMaxManifestLength);
    if (NULL == context->Manifest)
    {
        _aligned_free(context);
        return E_OUTOFMEMORY;
    }

    if (FAILED(StringCbPrintfA(context->Manifest, cbMaxManifestLength, CvMarkersManifestTemplate, guidString)) ||
        FAILED(StringCbLengthA(context->Manifest, cbMaxManifestLength, &context->cbManifestLength)))
    {
        free(context->Manifest);
        _aligned_free(context);
        return E_INVALIDARG;
    }

    result = EventRegister(
        &context->ProviderGuid,
        CvProviderEnableCallback, 
        context,
        &context->ProviderHandle);
    if (ERROR_SUCCESS != result)
    {
        free(context->Manifest);
        _aligned_free(context);
        return HRESULT_FROM_WIN32(result);
    }

    *ppProvider = (PCV_PROVIDER)context;
    return S_OK;
}

__inline HRESULT CvReleaseProvider(_In_ PCV_PROVIDER pProvider)
{
    PCV_PROVIDER_CONTEXT context;
    HRESULT error;

    if (NULL == pProvider)
    {
        return E_INVALIDARG;
    }
    
    context = (PCV_PROVIDER_CONTEXT)pProvider;
    error = HRESULT_FROM_WIN32(EventUnregister(context->ProviderHandle));
    free(context->Manifest);
    _aligned_free(context);
    return error;
}

__inline _Check_return_ HRESULT CvCreateMarkerSeriesW(
	_In_ PCV_PROVIDER  pProvider,
	_In_ LPCWSTR pSeriesNameW,
	_Out_ PCV_MARKERSERIES* ppMarkerSeries) 
{
    PCV_MARKERSERIES_CONTEXT context;
    HRESULT error;
    size_t seriesNameLengthInChars = 0;

    if (NULL == pProvider ||
        NULL == pSeriesNameW ||
        NULL == ppMarkerSeries)
    {
        return E_INVALIDARG;
    }

    *ppMarkerSeries = NULL;

    context = (PCV_MARKERSERIES_CONTEXT)malloc(sizeof(CV_MARKERSERIES_CONTEXT));
    if (NULL == context)
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(context, sizeof(CV_MARKERSERIES_CONTEXT));

    context->ProviderContext = (PCV_PROVIDER_CONTEXT)pProvider;

    if (FAILED(StringCchLengthW(pSeriesNameW, MaxSeriesNameLengthInChars + 1, &seriesNameLengthInChars)))
    {
		free(context);
        return E_INVALIDARG;
    }

    context->SeriesName = (PWSTR)malloc((seriesNameLengthInChars + 1) * sizeof(WCHAR));
    if (NULL == context->SeriesName)
    {
        free(context);
        return E_OUTOFMEMORY;
    }

    context->SeriesNameLength = seriesNameLengthInChars;
    error = StringCchCopyW(context->SeriesName, seriesNameLengthInChars + 1, pSeriesNameW);
    if (FAILED(error))
    {
        free(context->SeriesName);
        free(context);
        return error;
    }

    context->CodePage = GetACP();
    *ppMarkerSeries = (PCV_MARKERSERIES)context;
    return S_OK;
}

__inline _Check_return_ HRESULT CvCreateMarkerSeriesA(
    _In_ PCV_PROVIDER  pProvider,
    _In_ LPCSTR pSeriesNameA,
    _Out_ PCV_MARKERSERIES* ppMarkerSeries)
{
    int requiredChars;
    PWSTR seriesNameW;
    HRESULT error;
    size_t cchSrcLength = 0;

    if (NULL == pProvider ||
        NULL == pSeriesNameA ||
        NULL == ppMarkerSeries)
    {
        return E_INVALIDARG;
    }

    if (FAILED(StringCchLengthA(pSeriesNameA, MaxSeriesNameLengthInChars + 1, &cchSrcLength)))
    {
        return E_INVALIDARG;
    }

    // include terminating null char.
    ++cchSrcLength;
    requiredChars = MultiByteToWideChar(CP_ACP, 0, pSeriesNameA, (int)(cchSrcLength), NULL, 0);
    seriesNameW = (PWSTR)malloc(requiredChars * sizeof(WCHAR));
    if (NULL == seriesNameW)
    {
        return E_OUTOFMEMORY;
    }
    
    if (0 == MultiByteToWideChar(CP_ACP, 0, pSeriesNameA, (int)(cchSrcLength), seriesNameW, requiredChars))
    {
		free(seriesNameW);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    error = CvCreateMarkerSeriesW(pProvider, seriesNameW, ppMarkerSeries);
    free(seriesNameW);
    return error;
}

__inline _Check_return_ HRESULT CvCreateMarkerSeriesWithCodePageA(
    _In_ PCV_PROVIDER  pProvider,
    _In_ LPCSTR pSeriesName,
    _In_ UINT nTextCodePage,
    _Out_ PCV_MARKERSERIES* ppMarkerSeries)
{
    HRESULT r = CvCreateMarkerSeriesA(pProvider, pSeriesName, ppMarkerSeries);
    if (SUCCEEDED(r) &&
        NULL != ppMarkerSeries &&
        NULL != *ppMarkerSeries)
    {
        ((PCV_MARKERSERIES_CONTEXT)(*ppMarkerSeries))->CodePage = nTextCodePage;
    }

    return r;
}

__inline _Check_return_ HRESULT CvCreateDefaultMarkerSeriesOfDefaultProvider(
    _Out_ PCV_PROVIDER* ppProvider,
    _Out_ PCV_MARKERSERIES* ppMarkerSeries)
{
    HRESULT error = CvInitProvider(&CvDefaultProviderGuid, ppProvider);
    if (FAILED(error))
    {
        return error;
    }

    error = CvCreateMarkerSeriesW(*ppProvider, CvDefaultMarkerSeries, ppMarkerSeries);
    return error;
}

__inline HRESULT CvReleaseMarkerSeries(_In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }
    
    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    free(context->SeriesName);
    free(context);
    return S_OK;
}

__inline HRESULT CvIsEnabled(_In_ PCV_PROVIDER pProvider)
{
    if (NULL == pProvider)
    {
        return E_INVALIDARG;
    }

    return CvIsEnabledInternal((PCV_PROVIDER_CONTEXT)pProvider);
}

__inline HRESULT CvIsEnabledEx(_In_ PCV_PROVIDER pProvider, _In_ CV_IMPORTANCE level, _In_ int category)
{
    if (NULL == pProvider)
    {
        return E_INVALIDARG;
    }

    return CvIsEnabledExInternal((PCV_PROVIDER_CONTEXT)pProvider, level, category);
}

__inline HRESULT CvEnterSpanW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCWSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvEnterSpanVW(pMarkerSeries, ppSpan, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvEnterSpanA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvEnterSpanVA(pMarkerSeries, ppSpan, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvEnterSpanVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCWSTR pMessage,	
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;
    PCV_SPAN_CONTEXT spanContext;
    HRESULT error;

    if (NULL == pMarkerSeries || NULL == ppSpan)
    {
        return E_INVALIDARG;
    }

    *ppSpan = NULL;
    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    spanContext = (PCV_SPAN_CONTEXT)malloc(sizeof(CV_SPAN_CONTEXT));
    if (NULL == spanContext)
    {
        return E_OUTOFMEMORY;
    }

    spanContext->SeriesContext = context;
    spanContext->Level = CvImportanceNormal;
    spanContext->Category = CvDefaultCategory;
    InterlockedCompareExchange(&context->ProviderContext->CurrentSpanId, _I32_MIN, _I32_MAX);
    spanContext->SpanId = InterlockedIncrement(&context->ProviderContext->CurrentSpanId);

    error = CvWriteEventW(
        context,
        &CvEnterSpanEvent,
        CvImportanceNormal,
        CvDefaultCategory,
        spanContext->SpanId,
        pMessage,
        argList);

    if (FAILED(error))
    {
        free(spanContext);
    }
    else
    {
        *ppSpan = (PCV_SPAN)spanContext;
    }

    return error;
}

__inline HRESULT CvEnterSpanVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;
    PCV_SPAN_CONTEXT spanContext;
    HRESULT error;

    if (NULL == pMarkerSeries || NULL == ppSpan)
    {
        return E_INVALIDARG;
    }

    *ppSpan = NULL;
    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    spanContext = (PCV_SPAN_CONTEXT)malloc(sizeof(CV_SPAN_CONTEXT));
    if (NULL == spanContext)
    {
        return E_OUTOFMEMORY;
    }

    spanContext->SeriesContext = context;
    spanContext->Level = CvImportanceNormal;
    spanContext->Category = CvDefaultCategory;
    InterlockedCompareExchange(&context->ProviderContext->CurrentSpanId, _I32_MIN, _I32_MAX);
    spanContext->SpanId = InterlockedIncrement(&context->ProviderContext->CurrentSpanId);

    error = CvWriteEventA(
        context,
        &CvEnterSpanEvent,
        CvImportanceNormal,
        CvDefaultCategory,
        spanContext->SpanId,
        pMessage,
        argList);

    if (FAILED(error))
    {
        free(spanContext);
    }
    else
    {
        *ppSpan = (PCV_SPAN)spanContext;
    }

    return error;
}

__inline HRESULT CvEnterSpanExW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCWSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvEnterSpanExVW(pMarkerSeries, level, category, ppSpan, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvEnterSpanExA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvEnterSpanExVA(pMarkerSeries, level, category, ppSpan, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvEnterSpanExVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCWSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;
    PCV_SPAN_CONTEXT spanContext;
    HRESULT error;

    if (NULL == pMarkerSeries || NULL == ppSpan)
    {
        return E_INVALIDARG;
    }

    *ppSpan = NULL;
    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    spanContext = (PCV_SPAN_CONTEXT)malloc(sizeof(CV_SPAN_CONTEXT));
    if (NULL == spanContext)
    {
        return E_OUTOFMEMORY;
    }

    spanContext->SeriesContext = context;
    spanContext->Level = (BYTE)level;
    spanContext->Category = category;
    InterlockedCompareExchange(&context->ProviderContext->CurrentSpanId, _I32_MIN, _I32_MAX);
    spanContext->SpanId = InterlockedIncrement(&context->ProviderContext->CurrentSpanId);

    error = CvWriteEventW(
        context,
        &CvEnterSpanEvent,
        (BYTE)level,
        category,
        spanContext->SpanId,
        pMessage,
        argList);

    if (FAILED(error))
    {
        free(spanContext);
    }
    else
    {
        *ppSpan = (PCV_SPAN)spanContext;
    }

    return error;
}

__inline HRESULT CvEnterSpanExVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _Out_ PCV_SPAN* ppSpan,
    _In_ PCSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;
    PCV_SPAN_CONTEXT spanContext;
    HRESULT error;

    if (NULL == pMarkerSeries || NULL == ppSpan)
    {
        return E_INVALIDARG;
    }

    *ppSpan = NULL;
    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    spanContext = (PCV_SPAN_CONTEXT)malloc(sizeof(CV_SPAN_CONTEXT));
    if (NULL == spanContext)
    {
        return E_OUTOFMEMORY;
    }

    spanContext->SeriesContext = context;
    spanContext->Level = (BYTE)level;
    spanContext->Category = category;
    InterlockedCompareExchange(&context->ProviderContext->CurrentSpanId, _I32_MIN, _I32_MAX);
    spanContext->SpanId = InterlockedIncrement(&context->ProviderContext->CurrentSpanId);

    error = CvWriteEventA(
        context,
        &CvEnterSpanEvent,
        (BYTE)level,
        category,
        spanContext->SpanId,
        pMessage,
        argList);

    if (FAILED(error))
    {
        free(spanContext);
    }
    else
    {
        *ppSpan = (PCV_SPAN)spanContext;
    }

    return error;
}

__inline HRESULT CvLeaveSpan(_In_ PCV_SPAN pSpan)
{
    PCV_SPAN_CONTEXT context;
    HRESULT error;

    if (NULL == pSpan)
    {
        // return ok as CvEnterSpan may return NULL in case provider is disabled.
        return S_OK;
    }

    context = (PCV_SPAN_CONTEXT)pSpan;
    if (S_FALSE == CvIsEnabled((PCV_PROVIDER)context->SeriesContext->ProviderContext))
    {
        // may happen if the provider was disabled during the trace.
        free(context);
        return S_OK;
    }

    error = HRESULT_FROM_WIN32(CvWriteMarkerEventToEtw(
        context->SeriesContext->ProviderContext->ProviderHandle,
        &CvLeaveSpanEvent,
        context->Level,
        (BYTE)context->Category,
        context->SpanId,
        context->SeriesContext->SeriesName,
        context->SeriesContext->SeriesNameLength,
        NULL,
        0,
        NULL,
        0));
    free(context);
    return error;
}

__inline HRESULT CvWriteFlagW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteFlagVW(pMarkerSeries, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteFlagA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteFlagVA(pMarkerSeries, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteFlagVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    return CvWriteEventW(context, &CvFlagEvent, CvImportanceNormal, CvDefaultFlagCategory, 0, pMessage, argList);
}

__inline HRESULT CvWriteFlagVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    return CvWriteEventA(context, &CvFlagEvent, CvImportanceNormal, CvDefaultFlagCategory, 0, pMessage, argList);
}

__inline HRESULT CvWriteFlagExW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCWSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteFlagExVW(pMarkerSeries, level, category, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteFlagExA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteFlagExVA(pMarkerSeries, level, category, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteFlagExVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCWSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledExInternal(context->ProviderContext, level, category))
    {
        return S_OK;
    }

    return CvWriteEventW(context, &CvFlagEvent, (BYTE)level, category, 0, pMessage, argList);
}

__inline HRESULT CvWriteFlagExVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledExInternal(context->ProviderContext, level, category))
    {
        return S_OK;
    }

    return CvWriteEventA(context, &CvFlagEvent, (BYTE)level, category, 0, pMessage, argList);
}

__inline HRESULT CvWriteAlertW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteAlertVW(pMarkerSeries, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteAlertA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteAlertVA(pMarkerSeries, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteAlertVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    return CvWriteEventW(context, &CvFlagEvent, CvImportanceCritical, CvAlertCategory, 0, pMessage, argList);
}

__inline HRESULT CvWriteAlertVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    return CvWriteEventA(context, &CvFlagEvent, CvImportanceCritical, CvAlertCategory, 0, pMessage, argList);
}

__inline HRESULT CvWriteMessageW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteMessageVW(pMarkerSeries, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteMessageA(
    PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteMessageVA(pMarkerSeries, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteMessageVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCWSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    return CvWriteEventW(context, &CvMessageEvent, CvImportanceNormal, CvDefaultCategory, 0, pMessage, argList);
}

__inline HRESULT CvWriteMessageVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ PCSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledInternal(context->ProviderContext))
    {
        return S_OK;
    }

    return CvWriteEventA(context, &CvMessageEvent, CvImportanceNormal, CvDefaultCategory, 0, pMessage, argList);
}

__inline HRESULT CvWriteMessageExW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCWSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteMessageExVW(pMarkerSeries, level, category, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteMessageExA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCSTR pMessage,
    ...
    )
{
    HRESULT error;
    va_list args = NULL;
    va_start(args, pMessage);
    error = CvWriteMessageExVA(pMarkerSeries, level, category, pMessage, args);
    va_end(args);

    return error;
}

__inline HRESULT CvWriteMessageExVW(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCWSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledExInternal(context->ProviderContext, level, category))
    {
        return S_OK;
    }

    return CvWriteEventW(context, &CvMessageEvent, (BYTE)level, category, 0, pMessage, argList);
}

__inline HRESULT CvWriteMessageExVA(
    _In_reads_bytes_(16) PCV_MARKERSERIES pMarkerSeries,
    _In_ CV_IMPORTANCE level,
    _In_ int category,
    _In_ PCSTR pMessage,
    _In_ va_list argList)
{
    PCV_MARKERSERIES_CONTEXT context;

    if (NULL == pMarkerSeries)
    {
        return E_INVALIDARG;
    }

    context = (PCV_MARKERSERIES_CONTEXT)pMarkerSeries;
    if (S_FALSE == CvIsEnabledExInternal(context->ProviderContext, level, category))
    {
        return S_OK;
    }

    return CvWriteEventA(context, &CvMessageEvent, (BYTE)level, category, 0, pMessage, argList);
}

// Cannot inline callback so we tell the compiler not to inline yet 
// avoiding linker error (multiply defined symbol) using inline.
__declspec(noinline) __inline void NTAPI CvProviderEnableCallback(
    LPCGUID SourceId,
    ULONG IsEnabled,
    UCHAR Level,
    ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeywords,
    PEVENT_FILTER_DESCRIPTOR FilterData,
    PVOID CallbackContext)
{
    PCV_PROVIDER_CONTEXT context;
    UNREFERENCED_PARAMETER(SourceId);
    UNREFERENCED_PARAMETER(FilterData);

    context = (PCV_PROVIDER_CONTEXT)CallbackContext;
    context->Enabled = EVENT_CONTROL_CODE_DISABLE_PROVIDER != IsEnabled;
    context->Level = Level;
    context->MatchAnyKeyword = MatchAnyKeyword;
    context->MatchAllKeywords = MatchAllKeywords;
    if (!context->Enabled)
    {
        context->ManifestWritten = 0;
    }
}
