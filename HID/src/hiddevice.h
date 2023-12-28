#ifndef _HIDDEVICE_H
#define _HIDDEVICE_H

#include <windows.h>

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <string.h>
#include <tchar.h>
#include <winreg.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include <dbt.h>
#include <vector>
#include <ntsecapi.h>


#include <tlhelp32.h>
#include <psapi.h>
#include <WtsApi32.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <Shellapi.h>
#include <accctrl.h>
#include <aclapi.h>
#include <shlwapi.h>
#include <AppModel.h>

#include <map>
#include <uv.h>
#include <uv_async_queue.h>
#include <hidapi.h>


typedef struct _HIDD_ATTRIBUTES{
	ULONG Size;
	USHORT VendorID;
	USHORT ProductID;
	USHORT VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;


typedef struct _DEVICE_ATTRIBUTES{
	USHORT VendorID;
	USHORT ProductID;
	USHORT Location;
	BOOL Status;
} DEVICE_ATTRIBUTES, *PDEVICE_ATTRIBUTES;

typedef struct _CALLBACK_DATA{
	char Text[512];
} CALLBACK_DATA, *PCALLBACK_DATA;
// typedef struct _DEVICE_READ_DATA{
// 	INT size;
// 	UCHAR Data[128];
// 	Persistent<Value> DeviceDataCallback;
// 	uv_thread_t thread;
// 	hid_device *h_datadevice;
// } DEVICE_READ_DATA, *PDEVICE_READ_DATA;

// struct deviceData {
// 	int dataSize;
// 	unsigned char Data[128];
// 	Persistent<Value> DeviceDataCallback;
// 	uv_thread_t thread;
// 	hid_device *h_datadevice;
// };

typedef USHORT USAGE;



#endif