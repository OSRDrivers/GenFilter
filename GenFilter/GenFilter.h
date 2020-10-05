///////////////////////////////////////////////////////////////////////////////
//
//    (C) Copyright 1995 - 2020 OSR Open Systems Resources, Inc.
//    All Rights Reserved
//
//    This sofware is supplied for instructional purposes only.
//
//    OSR Open Systems Resources, Inc. (OSR) expressly disclaims any warranty
//    for this software.  THIS SOFTWARE IS PROVIDED  "AS IS" WITHOUT WARRANTY
//    OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
//    THE IMPLIED WARRANTIES OF MECHANTABILITY OR FITNESS FOR A PARTICULAR
//    PURPOSE.  THE ENTIRE RISK ARISING FROM THE USE OF THIS SOFTWARE REMAINS
//    WITH YOU.  OSR's entire liability and your exclusive remedy shall not
//    exceed the price paid for this material.  In no event shall OSR or its
//    suppliers be liable for any damages whatsoever (including, without
//    limitation, damages for loss of business profit, business interruption,
//    loss of business information, or any other pecuniary loss) arising out
//    of the use or inability to use this software, even if OSR has been
//    advised of the possibility of such damages.  Because some states/
//    jurisdictions do not allow the exclusion or limitation of liability for
//    consequential or incidental damages, the above limitation may not apply
//    to you.
//
//    OSR Open Systems Resources, Inc.
//    105 Route 101A Suite 19
//    Amherst, NH 03031  (603) 595-6500 FAX: (603) 595-6503
//    email bugs to: bugs@osr.com
//
//
//    MODULE:
//
//      CDFilter.h
//
//    ABSTRACT:
//
//      Main include file for the WDF filter driver
//
//    AUTHOR(S):
//
//      OSR Open Systems Resources, Inc.
// 
//    REVISION:   
//
//
///////////////////////////////////////////////////////////////////////////////
#pragma once

// ReSharper disable CppInconsistentNaming

#include <wdm.h>
#include <wdf.h>

//
// Warnings that are active for "Microsoft All Rules" that we routinely want to disable
//
#pragma warning(disable: 26438)     // avoid "goto"
#pragma warning(disable: 26440)     // Function can be delcared "noexcept"
#pragma warning(disable: 26485)     // No array to pointer decay
#pragma warning(disable: 26493)     // C-style casts
#pragma warning(disable: 26494)     // Variable is uninitialized


//
// Dummy IOCTL to illustrate how we can check for a particular IOCTL Control Code in our
// EvtIoDeviceControl Event Processing Callback.
//
constexpr auto IOCTL_YOU_ARE_INTERESTED_IN = (ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS);

//
// Our per Device context
//
typedef struct _GENFILTER_DEVICE_CONTEXT {  // NOLINT(cppcoreguidelines-pro-type-member-init)
    WDFDEVICE       WdfDevice;

    //
    // Other interesting stuff would go here
    //

} GENFILTER_DEVICE_CONTEXT, *PGENFILTER_DEVICE_CONTEXT;


//
// Context accessor function
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(GENFILTER_DEVICE_CONTEXT,
                                   GenFilterGetDeviceContext)


//
// Foreward and roll-type declarations
//
extern "C" DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD GenFilterEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_READ GenFilterEvtRead;
EVT_WDF_IO_QUEUE_IO_WRITE GenFilterEvtWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL GenFilterEvtDeviceControl;

EVT_WDF_REQUEST_COMPLETION_ROUTINE GenFilterCompletionCallback;

VOID
GenFilterSendAndForget(_In_ WDFREQUEST Request, _In_ PGENFILTER_DEVICE_CONTEXT DevContext);

VOID
GenFilterSendWithCallback(_In_ WDFREQUEST Request, _In_ PGENFILTER_DEVICE_CONTEXT DevContext);
