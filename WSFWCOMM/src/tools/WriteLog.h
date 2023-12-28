#ifndef _WRITELOG_H
#define _WRITELOG_H

#include <direct.h>
#include <atlstr.h>

void DebugOnOff(BOOL bOn = FALSE);
void WriteType(int iState = 0);
void LogName(CString sName); 
void WriteLogString(const CString Msg);
void WriteLogString(const char *fmt, ...);
void WriteLogChar(const char *Msg);
#endif