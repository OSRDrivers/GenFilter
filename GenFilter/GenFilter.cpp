///
/// @file GenFilter.cpp
///
//
// Copyright 2004-2020 OSR Open Systems Resources, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
// 
//    This software is supplied for instructional purposes only.  It is not
//    complete, and it is not suitable for use in any production environment.
//
//    OSR Open Systems Resources, Inc. (OSR) expressly disclaims any warranty
//    for this software.  THIS SOFTWARE IS PROVIDED  "AS IS" WITHOUT WARRANTY
//    OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
//    THE IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR
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

#include "GenFilter.h"

///////////////////////////////////////////////////////////////////////////////
//
//  DriverEntry
//
//    This routine is called by Windows when the driver is first loaded.  It
//    is the responsibility of this routine to create the WDFDRIVER
//
//  INPUTS:
//
//      DriverObject - Address of the DRIVER_OBJECT created by Windows for this
//                     driver.
//
//      RegistryPath - UNICODE_STRING which represents this driver's key in the
//                     Registry.  
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      STATUS_SUCCESS, otherwise an error indicating why the driver could not
//                      load.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
extern "C"
NTSTATUS
DriverEntry(PDRIVER_OBJECT  DriverObject,
            PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS          status;

#if DBG
    DbgPrint("GenFilter...Compiled %s %s\n",
             __DATE__,
             __TIME__);
#endif

    //
    // Initialize our driver config structure, specifying our 
    // EvtDeviceAdd event processing callback.
    //
    WDF_DRIVER_CONFIG_INIT(&config,
                           GenFilterEvtDeviceAdd);

    //
    // Create our WDFDEVICE Object and hook-up with the Framework
    //
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDriverCreate failed - 0x%x\n",
                 status);
#endif
        goto done;
    }

    status = STATUS_SUCCESS;

done:

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//
//  GenFilterEvtDeviceAdd
//
//    This routine is called by the Framework when a device of
//    the type we filter is found in the system.
//
//  INPUTS:
//
//      DriverObject - Our WDFDRIVER object
//
//      DeviceInit   - The device initialization structure we'll
//                     be using to create our WDFDEVICE
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      STATUS_SUCCESS, otherwise an error indicating why the driver could not
//                      load.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
NTSTATUS
GenFilterEvtDeviceAdd(WDFDRIVER       Driver,
                      PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS                  status;
    WDF_OBJECT_ATTRIBUTES     wdfObjectAttr;
    WDFDEVICE                 wdfDevice;
    PGENFILTER_DEVICE_CONTEXT devContext;
    WDF_IO_QUEUE_CONFIG       ioQueueConfig;

#if DBG
    DbgPrint("GenFilterEvtDeviceAdd: Adding device...\n");
#endif

    UNREFERENCED_PARAMETER(Driver);

    //
    // Indicate that we're creating a FILTER Device, as opposed to a FUNCTION Device.
    //
    // This will cause the Framework to attach us correctly to the device stack,
    // auto-forward any requests we don't explicitly handle, and use the
    // appropriate state machine for PnP/Power management among
    // several other things.
    //
    WdfFdoInitSetFilter(DeviceInit);

    //
    // Setup our device attributes specifying our per-Device context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&wdfObjectAttr,
                                            GENFILTER_DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit,
                             &wdfObjectAttr,
                             &wdfDevice);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDeviceCreate failed - 0x%x\n",
                 status);
#endif
        goto done;
    }

    //
    // Save our WDFDEVICE handle in the Device context for convenience
    //
    devContext = GenFilterGetDeviceContext(wdfDevice);
    devContext->WdfDevice = wdfDevice;

    //
    // Create our default Queue -- This is how we receive Requests.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                           WdfIoQueueDispatchParallel);

    //
    // Specify callbacks for those Requests that we want this driver to "see."
    //
    // Note that because this driver is a FILTER, if we do not specify a
    // callback for a particular Request type the Framework will automatically
    // forward all Requests that it receives of that type to our Local I/O
    // Target.  So, for example, if you're not interested in inspecting
    // READ Requests, you can just not specify an EvtIoRead callback and the
    // Framework will "do the right thing" and send it along.
    //
    ioQueueConfig.EvtIoRead          = GenFilterEvtRead;
    ioQueueConfig.EvtIoWrite         = GenFilterEvtWrite;
    ioQueueConfig.EvtIoDeviceControl = GenFilterEvtDeviceControl;

    //
    // Create the queue...
    //
    status = WdfIoQueueCreate(devContext->WdfDevice,
                              &ioQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfIoQueueCreate failed - 0x%x\n",
                 status);
#endif
        goto done;
    }

    status = STATUS_SUCCESS;

done:

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//
//  GenFilterEvtDeviceControl
//
//    This routine is called by the Framework when there is a
//    device control Request for us to process.
//
//  INPUTS:
//
//      Queue    - Handle to our default queue
//
//      Request  - Handle to a device control Request
//
//      OutputBufferLength - The length of the output buffer
//
//      InputBufferLength  - The length of the input buffer
//
//      IoControlCode      - The operation being performed
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
VOID
GenFilterEvtDeviceControl(WDFQUEUE   Queue,
                          WDFREQUEST Request,
                          size_t     OutputBufferLength,
                          size_t     InputBufferLength,
                          ULONG      IoControlCode)
{
    PGENFILTER_DEVICE_CONTEXT devContext;

    devContext = GenFilterGetDeviceContext(WdfIoQueueGetDevice(Queue));

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

#if DBG
    DbgPrint("GenFilterEvtDeviceControl -- Request 0x%p\n",
             Request);
#endif

    //
    // We're searching for one specific IOCTL function code that we're interested in
    //
    if (IoControlCode == IOCTL_YOU_ARE_INTERESTED_IN) {

#if DBG
        DbgPrint("GenFilterEvtDeviceControl -- The IOCTL we're looking for was found!  Request 0x%p\n",
                 Request);
#endif
        //
        // Do something useful.
        //

        //
        // We want to see the results for this particular Request... so send it
        // and request a callback for when the Request has been completed.
        GenFilterSendWithCallback(Request,
                                    devContext);

        return;
    }

    GenFilterSendAndForget(Request,
                           devContext);
}


///////////////////////////////////////////////////////////////////////////////
//
//  GenFilterEvtRead
//
//    This routine is called by the Framework when there is a
//    read Request for us to process
//
//  INPUTS:
//
//      Queue    - Handle to our default queue
//
//      Request  - Handle to read Request
//
//      Length   - The length of the read operation
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
VOID
GenFilterEvtRead(WDFQUEUE   Queue,
                 WDFREQUEST Request,
                 size_t     Length)
{
    PGENFILTER_DEVICE_CONTEXT devContext;

    UNREFERENCED_PARAMETER(Length);

    devContext = GenFilterGetDeviceContext(WdfIoQueueGetDevice(Queue));

#if DBG
    DbgPrint("GenFilterEvtRead -- Request 0x%p\n",
             Request);
#endif

    GenFilterSendAndForget(Request,
                           devContext);
}

///////////////////////////////////////////////////////////////////////////////
//
//  GenFilterEvtWrite
//
//    This routine is called by the Framework when there is a
//    read Request for us to process.
//
//  INPUTS:
//
//      Queue    - Handle to our default queue
//
//      Request  - Handle to a read Request
//
//      Length   - The length of the read operation
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL
//
//  NOTES:
//
//
///////////////////////////////////////////// //////////////////////////////////
_Use_decl_annotations_
VOID
GenFilterEvtWrite(WDFQUEUE   Queue,
                  WDFREQUEST Request,
                  size_t     Length)
{
    PGENFILTER_DEVICE_CONTEXT devContext;

    UNREFERENCED_PARAMETER(Length);

    devContext = GenFilterGetDeviceContext(WdfIoQueueGetDevice(Queue));

#if DBG
    DbgPrint("GenFilterEvtWrite -- Request 0x%p\n",
             Request);
#endif

    GenFilterSendAndForget(Request,
                           devContext);
}

///////////////////////////////////////////////////////////////////////////////
//
//  GenFilterSendAndForget
//
//      Sends a Request to our Local I/O Target and does no further processing
//      for that Request (that is, no completion callback is provided).
//
//      If our attempt to send the Request fails, this routine will complete the
//      Request with an appropriate status.  In all cases, then, this routine
//      returns with the Request considered "complete" by our driver and the caller
//      loses ownership of the Request and must not handle it again.
//
//  INPUTS:
//
//      Request     - Handle to a WDFREQUEST to send to our local I/O Target
//
//      DevContext  - Pointer to our WDFDEVICE context
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
VOID
GenFilterSendAndForget(WDFREQUEST                Request,
                       PGENFILTER_DEVICE_CONTEXT DevContext)
{
    NTSTATUS status;

    WDF_REQUEST_SEND_OPTIONS sendOpts;

    //
    // We want to send this Request and not deal with it again.  Note two
    // important things about send-and-forget:
    //
    // 1. Sending a Request with send-and-forget is the logical equivalent of
    //    completing the Request for the sending driver.  If WdfRequestSend returns
    //    TRUE, the Request is no longer owned by the sending driver.
    //
    // 2.  Send-and-forget is pretty much restricted to use only with the Local I/O Target.
    //     That's how we use it here.
    //
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOpts,
                                  WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    if (!WdfRequestSend(Request,
                        WdfDeviceGetIoTarget(DevContext->WdfDevice),
                        &sendOpts)) {

        //
        // Oops! The Framework was unable to give the Request to the specified
        // I/O Target.  Note that getting back TRUE from WdfRequestSend does not
        // imply that the I/O Target processed the Request with an ultimate status
        // of STATUS_SUCCESS. Rather, WdfRequestSend returning TRUE simply means
        // that the Framework was successful in delivering the Request to the
        // I/O Target for processing by the driver for that Target.
        //
        status = WdfRequestGetStatus(Request);
#if DBG
        DbgPrint("WdfRequestSend 0x%p failed - 0x%x\n",
                 Request,
                 status);
#endif
        WdfRequestComplete(Request,
                           status);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  GenFilterCompletionCallback
//
//    This routine is called by the Framework when a Request
//    has been completed by the I/O Target to which we sent it.
//
//    In this example, we always just complete the Request.  However,
//    alternatively, if we wanted to, we COULD choose to NOT complete the
//    Request in certain cases and keep it active.  This would allow us to
//    alter it, and/or send it to another I/O Target.
//
//  INPUTS:
//
//      Request  - The write request
//
//      Target   - The I/O target we sent the write to
//
//      Params   - Parameter information from the completed
//                 request
//
//      Context  - The context supplied to
//                 WdfRequestSetCompletionRoutine (NULL in
//                 our case)
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
VOID
GenFilterCompletionCallback(WDFREQUEST                     Request,
                            WDFIOTARGET                    Target,
                            PWDF_REQUEST_COMPLETION_PARAMS Params,
                            WDFCONTEXT                     Context)
{
    NTSTATUS status;
    auto*    devContext = (PGENFILTER_DEVICE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(devContext);

    DbgPrint("GenFilterCompletionCallback: Request=%p, Status=0x%x; Information=0x%Ix\n",
             Request,
             Params->IoStatus.Status,
             Params->IoStatus.Information);

    status = Params->IoStatus.Status;

    //
    // Potentially do something interesting here
    //

    WdfRequestComplete(Request,
                       status);
}

///////////////////////////////////////////////////////////////////////////////
//
//  GenFilterSendWithCallback
//
//      Sends a Request to our local I/O Target with a completion routine
//      callback so that the results of the operation can be examined.
//
//      If our attempt to send the Request fails, this routine completes the
//      Request with an appropriate status.  In all cases, then, the caller
//      must not handle the Request after calling this routine.
//
//
//  INPUTS:
//
//      Request     - Handle to a Request to be sent to our local I/O Target
//
//      DevContext  - Pointer to our WDFDEVICE context
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL.
//
//  NOTES:
//
///////////////////////////////////////////////////////////////////////////////
_Use_decl_annotations_
VOID
GenFilterSendWithCallback(WDFREQUEST                Request,
                          PGENFILTER_DEVICE_CONTEXT DevContext)
{
    NTSTATUS status;

    DbgPrint("Sending %p with completion\n",
             Request);

    //
    // Setup the request for the next driver
    //
    WdfRequestFormatRequestUsingCurrentType(Request);

    //
    // Set the completion routine...
    //
    WdfRequestSetCompletionRoutine(Request,
                                   GenFilterCompletionCallback,
                                   DevContext);
    //
    // And send it!
    // 
    if (!WdfRequestSend(Request,
                        WdfDeviceGetIoTarget(DevContext->WdfDevice),
                        WDF_NO_SEND_OPTIONS)) {

        //
        // Oops! Something bad happened, complete the request
        //
        status = WdfRequestGetStatus(Request);

        DbgPrint("WdfRequestSend failed = 0x%x\n",
                 status);
        WdfRequestComplete(Request,
                           status);
    }

    //
    // When we return the Request is always "gone"
    //
}
