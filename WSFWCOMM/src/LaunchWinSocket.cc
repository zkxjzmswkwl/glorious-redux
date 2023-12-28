#include "LaunchWinSocket.h"
#include "mmsystem.h"
#pragma comment(lib, "Winmm.lib")


// // This GUID is for all USB serial host PnP drivers, but you can replace it 
// // with any valid device class guid.
// GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72, 
//                       0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,01xc8,0x35 };

//HANDLE hid_thread;
//DWORD threadId;
//DWORD WINAPI ListenerThread(LPVOID lpParam);
//HANDLE m_CompSwHandle;
std::string m_csPrograms[10][10];

void CallbackAsync(uv_work_t* req);
void CallbackFinished(uv_work_t* req);

using namespace v8;

#define DEBUG_HEADER fprintf(stderr, "System [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__); 
#define DEBUG_FOOTER fprintf(stderr, "\n");
#define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#define MakeDataInRange(data, Max, Min)		do { data = max( min(data, Max), Min ); } while(0)

Persistent<Value> Callback;

namespace LaunchWinSocket {  
  using v8::Function;
  using v8::FunctionCallbackInfo;
  using v8::Isolate;
  using v8::Local;
  using v8::Null;
  using v8::Object;
  using v8::String;
  using v8::Value;
  using v8::HandleScope;

MMRESULT g_wTimerID = NULL;


void Initialization(const FunctionCallbackInfo<Value>& args)
{ 	
	DEBUG_LOG("in_Initialization");
  	args.GetReturnValue().Set(TRUE);  
}

void ShowDebug(const FunctionCallbackInfo<Value>& args)
{
  	//uint32_t iShow = args[0]->Uint32Value();
	uint32_t iShow = args[0].As<Int32>()->Value();
	
	DebugOnOff((BOOL)iShow);
}

void Executefile(const FunctionCallbackInfo<Value>& args) {  
	DEBUG_LOG("Executefile");

	Isolate* isolate = args.GetIsolate();

	//uint32_t iProfile = args[0]->Uint32Value();
	uint32_t iProfile = args[0].As<Int32>()->Value();
	DEBUG_LOG("iProfile:%x",iProfile);
	
	v8::String::Utf8Value str(isolate, args[0]);
    //std::string foo = std::string(*str);   
	std::string ProgramsData(*str);
	//DEBUG_LOG("LaunchWinSocket_%d:%s",0,ProgramsData);
	
	std::string csOutput[10];

	int iCount  = 0;

   	//::ShellExecuteA(NULL, "open", ProgramsData, param.c_str(), NULL,SW_SHOWNORMAL);
	//-------------------------------
	
	//-------------------------------
}
void SendMessageToServer(const FunctionCallbackInfo<Value>& args) {  
	Isolate* isolate = args.GetIsolate();
	v8::String::Utf8Value str(isolate, args[0]);
	std::string ProgramName(*str); 
	CString csPrecess(ProgramName.c_str());
	//
	v8::String::Utf8Value str2(isolate, args[1]);
	std::string CommandName(*str2); 
	CString csCommand(CommandName.c_str());
	//SendMsgToServer(strSend,strRecv,START,true);

	CString csRtn;

	//------------------------------
	WSADATA wsaData;
	int iRet =0;
	iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRet != 0)
	{
		csRtn = _T("WSAStartup Failed!");
		
    	DEBUG_LOG(csRtn);
    	args.GetReturnValue().Set(String::NewFromUtf8(isolate, csRtn).ToLocalChecked());
		return;
	}
	if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion))
	{
		csRtn = _T("WSADATA version is not correct!");
    	DEBUG_LOG(csRtn);
		WSACleanup();
    	args.GetReturnValue().Set(String::NewFromUtf8(isolate, csRtn).ToLocalChecked());
		return;
	}

	//创建套接字
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		csRtn = _T("create clientSocket failed!");
    	DEBUG_LOG(csRtn);
    	args.GetReturnValue().Set(String::NewFromUtf8(isolate, csRtn).ToLocalChecked());
		return;
	}

	//初始化服务器端地址族变量
	SOCKADDR_IN srvAddr;
	srvAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(8124);

	//连接服务器
	iRet = connect(clientSocket, (SOCKADDR*)&srvAddr, sizeof(SOCKADDR));
	if (0 != iRet)
	{
		csRtn = _T("ClientSocket connect failed!");
    	DEBUG_LOG(csRtn);
    	args.GetReturnValue().Set(String::NewFromUtf8(isolate, csRtn).ToLocalChecked());
		return;
	}

    //DEBUG_LOG("SendMessage");
	//发送消息
	char sendBuf[1024] = "";
	memset(sendBuf, 0, sizeof(sendBuf));   
    //DEBUG_LOG("SignedFlag"); 
	//声明标识
	strcpy_s(sendBuf, csCommand);
	send(clientSocket, sendBuf, strlen(sendBuf), 0);

    //DEBUG_LOG("RecvMessage"); 
	//接收消息
	char recvBuf[100] = "";
	recv(clientSocket, recvBuf, 100, 0);
	csRtn = A2T(recvBuf);
    DEBUG_LOG(csRtn);
	//csRtn.Format("%S",(char*)m_ValueBuffer);

	CString strEnd = _T("END");
	strcpy_s(sendBuf, strEnd);
	send(clientSocket, sendBuf, strlen(sendBuf), 0);
	//清理
	closesocket(clientSocket);
	WSACleanup();

	//-----------------------

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, csRtn).ToLocalChecked());

	//-----------------------
}
void FindWindowProcess(const FunctionCallbackInfo<Value>& args) {
	Isolate *isolate = args.GetIsolate();
	v8::String::Utf8Value str(isolate, args[0]);
	std::string ProgramName(*str); 

	HWND hWnd;
	HANDLE hProcessHandle;
	ULONG nProcessID;
	int rtn = 0;
	CString csRtn;
	//-------------------------------------
	CString csPrecess(ProgramName.c_str());
	// CString csPrecess;
	// csPrecess.Format("%S",ProgramName);
	hWnd = ::FindWindow(NULL, csPrecess);
	if (hWnd != NULL)
		rtn = 1;
	else
		rtn = 0;

    args.GetReturnValue().Set(rtn); 
}
void TerminateProcess(const FunctionCallbackInfo<Value>& args) { 
	Isolate *isolate = args.GetIsolate(); 
	v8::String::Utf8Value str(isolate, args[0]);
	std::string ProgramName(*str);

	HWND hWnd;
	HANDLE hProcessHandle;
	ULONG nProcessID;
	int rtn = 0;
	CString csRtn;
	//-------------------------------------
	CString csPrecess(ProgramName.c_str());
	hWnd = ::FindWindow(NULL, csPrecess);
    DEBUG_LOG("FindWindow"); 
	if (hWnd != NULL)
	{
		//PostMessage(hWnd, WM_CLOSE, NULL, NULL);
    	DEBUG_LOG("GetWindowThreadProcessId"); 
		::GetWindowThreadProcessId(hWnd, &nProcessID );
    	DEBUG_LOG("OpenProcess PROCESS_TERMINATE"); 
		hProcessHandle =::OpenProcess( PROCESS_TERMINATE,FALSE, nProcessID );
    	DEBUG_LOG("TerminateProcess"); 
		::TerminateProcess( hProcessHandle, 4 ); 
		csRtn = _T("Exit App Updater Server");
		rtn = 1;
	}else{
		csRtn = _T("Not Found Updater App Server!!!");
	}

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, csRtn).ToLocalChecked());
}


void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "Initialization", Initialization);
  NODE_SET_METHOD(exports, "Executefile", Executefile);
  NODE_SET_METHOD(exports, "SendMessageToServer", SendMessageToServer);

  NODE_SET_METHOD(exports, "FindWindowProcess", FindWindowProcess);
  NODE_SET_METHOD(exports, "TerminateProcess", TerminateProcess);
}
  
  NODE_MODULE(NODE_GYP_MODULE_NAME, init)
  
}  // namespace demo