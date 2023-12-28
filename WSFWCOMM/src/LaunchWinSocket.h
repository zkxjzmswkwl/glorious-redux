
#ifndef _VirtualKey_H
#define _VirtualKey_H

#include <windows.h>

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

// #include <stdlib.h>
// #include <stdio.h>
// #include <locale.h>
// #include <direct.h>
//#include <atlstr.h>

//#include <string.h>
//#include <tchar.h>
// #include <winreg.h>
// #include <Shlwapi.h>
// #include <strsafe.h>
// #include <dbt.h>
// #include <vector>
//#include <ntsecapi.h>

 #include <tlhelp32.h>
// #include <psapi.h>
 //#include <WtsApi32.h>
// #include <shlobj.h>
// #include <shlwapi.h>
// #include <objbase.h>
// #include <Shellapi.h>
// #include <accctrl.h>
// #include <aclapi.h>
// #include <shlwapi.h>
// #include <AppModel.h>

//#include <map>
// #include <hidapi.h>

#include <uv.h>
#include <uv_async_queue.h>

#include "tools\WriteLog.h"

#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>

typedef struct LaunchWinSocket_INFO{
	int Profile;
	std::string Filename;
};



#endif