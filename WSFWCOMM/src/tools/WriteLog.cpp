#include "WriteLog.h"
#include <iostream>
#include <fstream>

int g_intDebug = FALSE;
int g_iLogState = 0;
CString g_sLogState(L"SyncProgram");


void DebugOnOff(BOOL bOn)
{
	g_intDebug = bOn;
}

void WriteType(int iState)
{
	g_iLogState = iState;
}

void LogName(CString sName)
{
	g_sLogState = sName;
}

void WriteLog(const char *Path,const char *Msg,int state)
{	
	//if(g_intDebug==1)
	{		
		FILE *pf='\0';
		if(state==1)
		{
			pf=fopen(Path,"w");
		}
		else
		{
			pf=fopen(Path,"a");
		}
		 fprintf(pf,Msg);
		 fprintf(pf, "\n");
		 fclose(pf);
	}	
}


void WriteLogString(const CString Msg)
{
	if(g_intDebug==1)
	{
		SYSTEMTIME st;
		//GetSystemTime(&st);
		GetLocalTime(&st);  
		CString Time;
		Time.Format(_T("%0.2d:%0.2d:%0.2d:%0.3d ::  "),st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
	
		CString csTemp;
		csTemp = Msg;
		csTemp = Time + csTemp;

    char pResult[MAX_PATH];
    //strcpy(pResult, (LPCTSTR)csTemp);
		  sprintf(pResult,"%s",(LPCTSTR)csTemp);

		////////////////////////////////////////////////////////////////////////////

    //day + path
		char logPath[MAX_PATH];
    char *buffer;
		if((buffer = getcwd(NULL, 0)) == NULL){
		  sprintf(logPath,"C:\\LOG");
    }
    else
    {
		  sprintf(logPath,"%s",buffer);
    }

    sprintf(logPath,"%s\\log",logPath);

		DWORD ftyp = GetFileAttributesA(logPath);
		if (ftyp == INVALID_FILE_ATTRIBUTES)
			_mkdir(logPath);

    sprintf(logPath,"%s\\%0.4d%0.2d%0.2d_%s.log",logPath,st.wYear,st.wMonth,st.wDay,(LPCTSTR)g_sLogState);

		WriteLog(logPath,pResult,g_iLogState);
	}	
}

void WriteLogString(const char *fmt, ...)
{
	char out[1024];

	va_list body;
	va_start(body, fmt);
	vsprintf(out, fmt, body);
	va_end(body); 
	WriteLogChar(out);
}

void WriteLogChar(const char *Msg)
{
	if(g_intDebug==1)
	{
    //Time + string
		SYSTEMTIME st;
		GetLocalTime(&st);  

		char Time[MAX_PATH];
    sprintf(Time,"%0.2d:%0.2d:%0.2d:%0.3d ::  ",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
	
		char csTemp[MAX_PATH];
    sprintf(csTemp,"%s%s",Time,Msg);
		////////////////////////////////////////////////////////////////////////////

    //day + path
		char logPath[MAX_PATH];
    char *buffer;
		if((buffer = getcwd(NULL, 0)) == NULL){
		  sprintf(logPath,"C:\\LOG");
    }
    else
    {
		  sprintf(logPath,"%s",buffer);
    }

    sprintf(logPath,"%s\\log",logPath);

		DWORD ftyp = GetFileAttributesA(logPath);
		if (ftyp == INVALID_FILE_ATTRIBUTES)
			_mkdir(logPath);

    sprintf(logPath,"%s\\%0.4d%0.2d%0.2d_%s.log",logPath,st.wYear,st.wMonth,st.wDay,(LPCTSTR)g_sLogState);
 
		WriteLog(logPath,csTemp,g_iLogState);
	}	
}