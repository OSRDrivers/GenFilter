# GenFilter
This is a generic WDF filter that can be loaded pretty much anywhere. It can serve as a starting-point for writing any sort of a device filter.

As configured, this filter will instantiate as an upper filter of CD-ROM class devices.  It claims READ, WRITE, and DEVICE CONTROL Requests and prints out the Request handle.
It illustrates how to search for a particular IOCTL control code (look for "IOCTL_YOU_ARE_INTERESTED_IN").  The sample anso demonstrates how to send 
Requests to the Local I/O Target with "send-and-forget" and asynchronously with a Completion Routine Callback.
