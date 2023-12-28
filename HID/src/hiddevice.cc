#include "hiddevice.h"


// This GUID is for all USB serial host PnP drivers, but you can replace it 
// with any valid device class guid.
GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72, 
                      0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 };

HANDLE hid_thread;
DWORD threadId;
DWORD WINAPI ListenerThread(LPVOID lpParam);

HANDLE devicePNPEvent;

void NotifyAsync(uv_work_t* req);
void NotifyFinished(uv_work_t* req);

void CallbackAsync(uv_work_t* req);
void CallbackFinished(uv_work_t* req);

using namespace v8;

#define DEBUG_HEADER fprintf(stderr, "System [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__); 
#define DEBUG_FOOTER fprintf(stderr, "\n");
#define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER

Persistent<Value> HIDPNPCallback;
Persistent<Value> DebugMessageCallback;


Persistent<Value> g_DeviceDataCallback;
/*********************** Struct DATA For HIDDevice **********************************************/
typedef struct _DEVICE_INFO{
  int iVID;
  int iPID;
  int iUsagePage;
  int iUsage;
  int iFWVersion;

} DEVICE_INFO, *PDEVICE_INFO;

typedef struct _DEVICE_READ_DATA{
  INT size;
  UCHAR Data[128];
  UCHAR tempData[128];
  uv_thread_t thread;
  hid_device *h_datadevice;
  DEVICE_INFO dEP3info;
  
	TCHAR DevicePath[MAX_PATH];
} DEVICE_READ_DATA, *PDEVICE_READ_DATA;

typedef struct _DEVICE_WRITE_DATA{
  hid_device *hiddevice;
  DEVICE_INFO DeviceInfo;
  INT SetDataError;
} DEVICE_WRITE_DATA, *PDEVICE_WRITE_DATA;

/*********************** Struct DATA For HIDDevice **********************************************/

//hidapi
hid_device* h_hiddevice;
DEVICE_WRITE_DATA g_hiddevice_info[50];
UINT g_iHidCount = 0;

//DEVICE_READ_DATA device_ep3_info;
DEVICE_READ_DATA g_device_ep3_info[50];

int g_iEp3Count = 0;

bool g_bHidIntercept = false;
namespace hiddevice {
  
  using v8::Function;
  using v8::FunctionCallbackInfo;
  using v8::Isolate;
  using v8::Local;
  using v8::Null;
  using v8::Object;
  using v8::String;
  using v8::Value;

//Defined  Hid Input Data
  void ReadHidData(DEVICE_READ_DATA *hidData);
  UVQueue<DEVICE_READ_DATA *> completionQueue(ReadHidData);

//Callback Hid VID/PID/Status and Input Data Into Backend
  void HidCallback(PDEVICE_ATTRIBUTES deviceInfo)
  {
    DEBUG_LOG("vid:%d pid:%d status:%d",deviceInfo->VendorID,deviceInfo->ProductID,deviceInfo->Status);
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    
    Local<Context> context = isolate->GetCurrentContext();

    Local<Object> device = Object::New(isolate);
    Local<String> vid = String::NewFromUtf8(isolate, "vid").ToLocalChecked();  
    device->Set(context, vid, Integer::New(isolate, deviceInfo->VendorID));

    Local<String> pid = String::NewFromUtf8(isolate, "pid").ToLocalChecked();
    device->Set(context, pid, Integer::New(isolate, deviceInfo->ProductID));
    
    Local<String> status = String::NewFromUtf8(isolate, "status").ToLocalChecked();
    device->Set(context, status, Integer::New(isolate, deviceInfo->Status));

    const unsigned argc = 1;  
    Local<Value> argv[argc] = {device};
    
    Local<Value> cbVal=Local<Value>::New(isolate, HIDPNPCallback);
    Local<Function> cb = Local<Function>::Cast(cbVal);
    cb->Call(context, Null(isolate), argc, argv).ToLocalChecked();
  }

//***********Callback debug message Into Backend*****************
  void MessageCallBack(CALLBACK_DATA *message)
  {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    //Local<Value> argv[1] = { String::NewFromUtf8(isolate, (CALLBACK_DATA *)message->Text) };
    
    Local<Context> context = isolate->GetCurrentContext();
    //-----------------------------------
    Local<Object> CallBackData = Object::New(isolate);
    CallBackData->Set(context, String::NewFromUtf8(isolate, "Text").ToLocalChecked(), String::NewFromUtf8(isolate,(char *)message->Text).ToLocalChecked());
    //-----------------------------------
    Local<Value> argv[1] = { CallBackData };
    Local<Value> cbVal = Local<Value>::New(isolate, DebugMessageCallback);
    Local<Function> cb = Local<Function>::Cast(cbVal);
    cb->Call(context, Null(isolate),1, argv).ToLocalChecked();
  }

  void CallbackAsync(uv_work_t* req) {
    // WaitForSingleObject(devicePNPEvent, INFINITE);
  }

  void CallbackFinished(uv_work_t* req) {
    CALLBACK_DATA * msg = static_cast<CALLBACK_DATA *>(req->data);
    //DEBUG_LOG("CallbackFinished_%s",msg->Text);
    MessageCallBack(msg);
  }

  void DebugMessage(char * msg)
  {
    //char *msg1=msg;
	  PCALLBACK_DATA CallBack = new CALLBACK_DATA;		
		//CallBack->Text = (char)msg;//Error
    sprintf(CallBack->Text,"%s",msg);//Do Not use Assign
    
    DEBUG_LOG("DebugMessage_%s",CallBack->Text);
    uv_work_t* req = new uv_work_t();
    req->data = CallBack;
    uv_queue_work(uv_default_loop(), req, CallbackAsync, (uv_after_work_cb)CallbackFinished);

  }
//***********Callback debug message Into Backend*****************

  void NotifyAsync(uv_work_t* req) {
    WaitForSingleObject(devicePNPEvent, INFINITE);
  }

  void NotifyFinished(uv_work_t* req) {

    PDEVICE_ATTRIBUTES device = (PDEVICE_ATTRIBUTES)req->data;
    DEBUG_LOG("-----------NotifyFinished----------");

    HidCallback(device);
  }
  //**********Callback HidDevice Plug message Into Backend*****************
  void NotifyHidDevicePNP(PDEV_BROADCAST_DEVICEINTERFACE info, bool status)
  {
    DEVICE_ATTRIBUTES *device = new DEVICE_ATTRIBUTES;
    TCHAR dbccName[512] = {0};
    
    memcpy(dbccName, info->dbcc_name, 512);
    
    //DebugMessage(dbccName);
    //DEBUG_LOG(dbccName);   

    // //Find VID
    char *VID = strstr(dbccName, "VID_");
    char *PID = strstr(dbccName, "PID_");

    char *VID2 = strstr(dbccName, "VID&");
    char *PID2 = strstr(dbccName, "PID&");
 
    if(VID != NULL){
      char tmpVID[5] = {0};
      tmpVID[4] = '\0';
      strncpy(tmpVID, VID+strlen("VID_"), 4);
      device->VendorID = strtol(tmpVID, 0, 16);

    }else if(VID2 != NULL){
      char tmpVID[7] = {0};
      tmpVID[6] = '\0';
      strncpy(tmpVID, VID2+strlen("VID&"), 6);
      DEBUG_LOG(tmpVID);   
      device->VendorID = strtol(tmpVID, 0, 16);
      
    }
    // //Find PID
    char tmpPID[5] = {0};
    if(PID != NULL){
      tmpPID[4] = '\0';
      strncpy(tmpPID, PID+strlen("PID_"), 4);
      device->ProductID = strtol(tmpPID, 0, 16);

    }else if(PID2 != NULL){
      tmpPID[4] = '\0';
      strncpy(tmpPID, PID2+strlen("PID&"), 4);
      device->ProductID = strtol(tmpPID, 0, 16);
    }

    if(status == true)
      device->Status = 1;
    else
      device->Status = 0;
    
    SetEvent(devicePNPEvent);

    uv_work_t* req = new uv_work_t();
    req->data = device;
    uv_queue_work(uv_default_loop(), req, NotifyAsync, (uv_after_work_cb)NotifyFinished);
  }
  //**********Callback HidDevice Plug message Into Backend*****************
  void RunDebugCallback(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DebugMessageCallback.Reset(isolate, args[0]);
  } 
  void RunHidCallback(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    HIDPNPCallback.Reset(isolate,args[0]);
  }
  void HIDReadThreadFn(void* arr)
  {
    int rtn = 0;
    // unsigned char ReadBuffer[64];    
    
    DEVICE_READ_DATA * _hidInfo = (DEVICE_READ_DATA *)arr;

    DEBUG_LOG("_hidHandle:%p", _hidInfo->h_datadevice);
    while(_hidInfo->h_datadevice){
      memset(_hidInfo->Data, 0, sizeof(_hidInfo->Data));
      rtn = hid_read(_hidInfo->h_datadevice, _hidInfo->Data, sizeof(_hidInfo->Data));
      
      if(rtn > 0){
        //DEBUG_LOG("rtn:%x", rtn);
        memset(_hidInfo->tempData, 0, sizeof(_hidInfo->tempData));
        memcpy(_hidInfo->tempData, _hidInfo->Data, sizeof(_hidInfo->Data) );

        _hidInfo->size = rtn;
        completionQueue.ref();
        completionQueue.post(_hidInfo);
      }
      Sleep(10);
    }

    // trans->HidHandle = NULL;
    DEBUG_LOG("TransferThreadFn Exit : %p", _hidInfo->h_datadevice);
  }


  void ReadHidData(DEVICE_READ_DATA * hidData)
  { 
      Isolate* isolate = Isolate::GetCurrent();
      HandleScope scope(isolate);
      completionQueue.unref();

      Local<Value> cbVal=Local<Value>::New(isolate, g_DeviceDataCallback);
      Local<Function> cb = Local<Function>::Cast(cbVal);
      const unsigned argc = 2;

      //DEBUG_LOG("RE:hid_read size:%d",hidData->size);

      Local<Object> buf  = node::Buffer::Copy(isolate, (char *)hidData->tempData, hidData->size).ToLocalChecked();

      Local<Context> context = isolate->GetCurrentContext();
      //-----------------------------------
      Local<Object> device = Object::New(isolate);
      device->Set(context, String::NewFromUtf8(isolate, "vid").ToLocalChecked(), Integer::New(isolate, hidData->dEP3info.iVID));
      device->Set(context, String::NewFromUtf8(isolate, "pid").ToLocalChecked(), Integer::New(isolate, hidData->dEP3info.iPID));
      //-----------------------------------
      Local<Value> argv[argc] = { buf ,device };
      //DEBUG_LOG("cbVal : ,%x", cbVal );
      
      DEBUG_LOG("buf : ,%x, %x, %x, %x, %x, %x, %x, %x, %x", hidData->tempData[0], hidData->tempData[1], hidData->tempData[2], hidData->tempData[3], hidData->tempData[4], hidData->tempData[5], hidData->tempData[6], hidData->tempData[7], hidData->tempData[8]);
      
      cb->Call(context,Null(isolate), argc, argv);
  }

  void RunDeviceDataCallback(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

      uint32_t usagePage = args[0].As<Int32>()->Value();
      uint32_t usage = args[1].As<Int32>()->Value();
      uint32_t VID = args[2].As<Int32>()->Value();
      uint32_t PID = args[3].As<Int32>()->Value();      
          
      bool rtn = false;
      char msg[512] = {0};

      //DEBUG_LOG("RunReadDataCallback... %04hx %04hx   usage_page: %d usage: %d\n", VID, PID, usagePage, usage);

      struct hid_device_info *devs, *cur_dev;
      char dev_ep3path[2056] = {0};

    //Enumerate all End point By using VID/PID
      devs = hid_enumerate(VID, PID);
      cur_dev = devs;	
      while (cur_dev) {
        //Match End point By using usagePage/usage,and get device path 
        if(cur_dev->usage_page == usagePage && cur_dev->usage == usage){
          // DEBUG_LOG("Device Found type: %04hx %04hx  usage_page: %04hx  usage: %04hx\n", cur_dev->vendor_id, cur_dev->product_id, cur_dev->usage_page, cur_dev->usage);
          memcpy(dev_ep3path, cur_dev->path, 2056);
        }
        cur_dev = cur_dev->next;
      }
		  sprintf(msg,"Release HID's enumeration \n");
      DEBUG_LOG(msg);
      DebugMessage(msg);
      //Release HID's enumeration
      hid_free_enumeration(devs);

		  // sprintf(msg,"open ep3hid hid_open_path: %s \n",dev_ep3path);
      // DEBUG_LOG(msg);
      // DebugMessage(msg);

      //Open HID's HID_API_CALL with input device path
      hid_device *h_datadevice;
      h_datadevice = hid_open_path(dev_ep3path);

      //Assign into input hiddevice array
      if(h_datadevice != NULL){
        //-------------Assign By Device Count------------
        int iExist = -1;
        for(int i=0 ;i < g_iEp3Count ;i++){
          if(g_device_ep3_info[i].dEP3info.iVID == VID
           &&g_device_ep3_info[i].dEP3info.iPID == PID
           &&g_device_ep3_info[i].dEP3info.iUsagePage == usagePage
           &&g_device_ep3_info[i].dEP3info.iUsage == usage
          )
          iExist = i;
        }
        //When its Exist,It will OverWrite one g_hiddevice_info value
        //if not,Stack one hiddevice value into g_hiddevice_info array
        if(iExist == -1){        
          iExist = g_iEp3Count;
          g_iEp3Count ++;
          // DEBUG_LOG("g_iEp3Count++,iExist: %d",iExist);
        }else{
          CloseHandle(g_device_ep3_info[iExist].thread);
          memset(&g_device_ep3_info[iExist],0,sizeof(g_device_ep3_info[iExist]));
          // DEBUG_LOG("g_device_ep3_info Close: %d",iExist);
        }
        //-------------Assign By Device Count------------
        g_device_ep3_info[iExist].h_datadevice = h_datadevice;
        g_device_ep3_info[iExist].dEP3info.iVID = VID;
        g_device_ep3_info[iExist].dEP3info.iPID = PID;
        g_device_ep3_info[iExist].dEP3info.iUsagePage = usagePage;
        g_device_ep3_info[iExist].dEP3info.iUsage = usage;
		    sprintf(g_device_ep3_info[iExist].DevicePath,"%s",dev_ep3path);
        
        DEBUG_LOG("open read data handle success..., DevicePath:%s",g_device_ep3_info[iExist].DevicePath); 
        

        rtn = true;
        
		    sprintf(msg,"open read data handle success...,g_iEp3Count: %d \n",(int)g_iEp3Count);
        DEBUG_LOG(msg);
        DebugMessage(msg);
  
        //g_device_ep3_info[iExist].DeviceDataCallback.Reset(isolate,args[4]);

        uv_thread_create(&g_device_ep3_info[iExist].thread, HIDReadThreadFn, &g_device_ep3_info[iExist]);
      }
	    //------return result value----------
      args.GetReturnValue().Set(Integer::New(isolate,rtn));
  } 
  
  void SetDeviceCallbackFunc(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    bool rtn = false;

    g_DeviceDataCallback.Reset(isolate,args[0]);
    rtn = true;

    args.GetReturnValue().Set(Integer::New(isolate,rtn));
  } 
  //The Function is not for use currently
  // void OpenDeviceHID(const FunctionCallbackInfo<Value>& args) {
  //   Isolate* isolate = args.GetIsolate();

  //   // DebugMessage("hid_init()...");
  //   if (hid_init()) {
  //     //DebugMessage("ErrorHandler OpenDeviceHID fails...");
  //     return;
  //   }

  //   uint32_t VID = (short)args[0]->Int32Value();
  //   uint32_t PID = (short)args[1]->Int32Value();

  //   char msg[256] = {0};
  //   sprintf(msg, "hid_open vid:%4x, pid:%4x...", VID, PID);
  //   //DebugMessage(msg);
  //   h_hiddevice = hid_open(VID, PID, NULL);
  // }

//Find Device ,By using VID/PID and End point's usagePage/usage Data
  void FindDevice(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

	//---------turn App data into Hid's data structure----------

    uint32_t usagePage = args[0].As<Int32>()->Value();
    uint32_t usage = args[1].As<Int32>()->Value();
    uint32_t VID = args[2].As<Int32>()->Value();
    uint32_t PID = args[3].As<Int32>()->Value();

    uint32_t iFWVersion = 0;

    int rtn = 0;
    char msg[512] = {0};

    // DEBUG_LOG("find device vid:%4d, pid:%4d", VID, PID);
    // DEBUG_LOG("find device usagePage:%4x, usage:%4x", usagePage, usage);

    struct hid_device_info *devs, *cur_dev;
    char dev_path[1024] = {0};

    //Enumerate all End point By using VID/PID
    devs = hid_enumerate(VID, PID);
	  cur_dev = devs;	
    while (cur_dev) {

      //DEBUG_LOG("Device Found type: %04hx %04hx  usage_page: %d  usage: %d interface_number: %d \n", cur_dev->vendor_id, cur_dev->product_id, //cur_dev->usage_page, cur_dev->usage, cur_dev->interface_number);

      // DEBUG_LOG("cur_dev->path: %s \n",cur_dev->path );

      //Match End point By using usagePage/usage,and get device path 
      if(cur_dev->usage_page == usagePage && cur_dev->usage == usage)
      {
          iFWVersion = cur_dev->release_number;

          memcpy(dev_path, cur_dev->path, 1024);
          DEBUG_LOG(dev_path);
      }
      
      cur_dev = cur_dev->next;
    }
    //DEBUG_LOG("hid_free_enumeration: %s \n",devs->path );
    //Release HID's enumeration
	  hid_free_enumeration(devs);

		// sprintf(msg,"open hid feature report...\n");
    // DEBUG_LOG(msg);
    // DebugMessage(msg);
    
    //Open HID's HID_API_CALL with device path
    // DEBUG_LOG("hid_open_path: %s \n",dev_path );
    hid_device *hiddevice;
    hiddevice = hid_open_path(dev_path);

    //Assign into hiddevice array
    if(hiddevice != NULL){
      //-------------Assign By Device Count------------
      int iExist = -1;
      for(int i=0 ;i<g_iHidCount;i++){
        if(   g_hiddevice_info[i].DeviceInfo.iVID == VID
            &&g_hiddevice_info[i].DeviceInfo.iPID == PID
            &&g_hiddevice_info[i].DeviceInfo.iUsagePage == usagePage
            &&g_hiddevice_info[i].DeviceInfo.iUsage == usage
        )
        iExist = i;
        //break;
      }
      //When its Exist,It will OverWrite one g_hiddevice_info value
      //if not,Stack one hiddevice value into g_hiddevice_info array
      if(iExist == -1){        
        iExist = g_iHidCount;
        g_iHidCount ++;
        
        // sprintf(msg,"g_iHidCount++:%d,iExist: %d \n",(int)g_iHidCount,(int)iExist);
        // DEBUG_LOG(msg);
      }else{
        memset(&g_hiddevice_info[iExist],0,sizeof(g_hiddevice_info[iExist]));
        
		    // sprintf(msg,"g_iHidCount++:%d , iExist: %d \n",(int)g_iHidCount,(int)iExist);
        // DEBUG_LOG(msg);
        //DebugMessage(msg);
      }
      //-------------Assign By Device Count------------
      g_hiddevice_info[iExist].hiddevice = hiddevice;
      g_hiddevice_info[iExist].DeviceInfo.iVID = VID;
      g_hiddevice_info[iExist].DeviceInfo.iPID = PID;
      g_hiddevice_info[iExist].DeviceInfo.iUsagePage = usagePage;
      g_hiddevice_info[iExist].DeviceInfo.iUsage = usage;
      g_hiddevice_info[iExist].DeviceInfo.iFWVersion = iFWVersion;
      g_hiddevice_info[iExist].SetDataError = 0;

      rtn = iExist+1;
		  sprintf(msg,"open feature report handle success...,g_iHidCount: %d \n",(int)g_iHidCount);
      DEBUG_LOG(msg);
      DebugMessage(msg);
    }
	//------return result value----------
    args.GetReturnValue().Set(Integer::New(isolate,rtn));
  }

  //Write Device by using data
  void Set_hid_write(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();


    uint32_t deviceId = args[0].As<Int32>()->Value() - 1;
    uint32_t ReportID = args[1].As<Int32>()->Value();
    uint32_t wLength = args[2].As<Int32>()->Value();

    Local<Context> context = isolate->GetCurrentContext();
    
    unsigned char* inBuf = (unsigned char*)node::Buffer::Data(args[3]->ToObject(context).ToLocalChecked());
    unsigned char data[2056] = {0};


    data[0] = ReportID;
    memcpy(data, inBuf, wLength * sizeof(char));

    //Write Device Feature Report into hiddevice
    //if success,return value is data Length
    DEBUG_LOG("Set_hid_write ... data : %x, %x, %x, %x, %x, %x, %x, %x", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    int res = 0;    
    res = hid_write(g_hiddevice_info[deviceId].hiddevice, data, wLength);

    if (res < 0) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Unable to send a hid write.").ToLocalChecked()));
      DEBUG_LOG("Unable to send a hid write...");
	  }
	  args.GetReturnValue().Set(res);  
  }



  void ClearErrorCount(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
	  //uint32_t deviceId = args[0]->Uint32Value() - 1;
    uint32_t deviceId = args[0].As<Int32>()->Value() - 1;
    int res = 0;
    if (g_hiddevice_info[deviceId].SetDataError > 0 || g_hiddevice_info[deviceId].hiddevice != NULL)
    {
      DEBUG_LOG("SetDataError ClearErrorCount Cleared");
      g_hiddevice_info[deviceId].SetDataError = 0;//Clear
      res = 1;
    }else{
      TCHAR msg[512] = {0};
      if(g_hiddevice_info[deviceId].SetDataError <= 0){
		      sprintf(msg,"SetDataError ClearErrorCount Failed-SetDataError: %d \n",(int)g_hiddevice_info[deviceId].SetDataError);
      }else if(g_hiddevice_info[deviceId].hiddevice == NULL){
		      sprintf(msg,"SetDataError ClearErrorCount Failed-hiddevice: %d \n",(int)g_hiddevice_info[deviceId].hiddevice);
      }
      DEBUG_LOG(msg);
    }
	  //------return result value----------
	  args.GetReturnValue().Set(res);  
  }

  void SetFeatureReport(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    TCHAR msg[512] = {0};

	  // uint32_t deviceId = args[0]->Uint32Value() - 1;
    // uint32_t ReportID = args[1]->Uint32Value();
	  // uint32_t wLength = args[2]->Uint32Value();

    uint32_t deviceId = args[0].As<Int32>()->Value() - 1;
    uint32_t ReportID = args[1].As<Int32>()->Value();
    uint32_t wLength = args[2].As<Int32>()->Value();

    Local<Context> context = isolate->GetCurrentContext();
    unsigned char* inBuf = (unsigned char*)node::Buffer::Data(args[3]->ToObject(context).ToLocalChecked());

    unsigned char data[2056];
    memset(&data, 0, sizeof(data));
    //DEBUG_LOG("SetDataError Begin3");
    memcpy(&data[0], inBuf, wLength * sizeof(char));
    //DEBUG_LOG("SetDataError Begin4");
    data[0] = ReportID;

    int res = 0;
    if (g_hiddevice_info[deviceId].SetDataError>3 || g_hiddevice_info[deviceId].hiddevice == NULL)
    {
      DEBUG_LOG("SetDataError SetFeatureReport refused");
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"SetFeatureReport refused").ToLocalChecked()));
	    args.GetReturnValue().Set(res); 
    }else{
      
        res = hid_send_feature_report(g_hiddevice_info[deviceId].hiddevice, data, wLength);
		    //sprintf(msg,"SetDataError SetFeatureReport res: %d \n",(int)res);
        //DEBUG_LOG(msg);

        if (res < 0) {
          DEBUG_LOG("Unable to send a feature report...-res:%d",res);

          g_hiddevice_info[deviceId].SetDataError++;
          isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Unable to send a feature    report.").ToLocalChecked()));


		      sprintf(msg,"SetDataError Count: %d \n",(int)g_hiddevice_info[deviceId].SetDataError);
          DEBUG_LOG(msg);
          DebugMessage(msg);

	      }
	      args.GetReturnValue().Set(res);

    }
    
  }

  void GetFeatureReport(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    unsigned char data[2056] = {0};
	  // uint32_t deviceId = args[0]->Uint32Value() - 1;
    // uint32_t ReportID = args[1]->Uint32Value();
	  // uint32_t wLength = args[2]->Uint32Value();

    uint32_t deviceId = args[0].As<Int32>()->Value() - 1;
    uint32_t ReportID = args[1].As<Int32>()->Value();
    uint32_t wLength = args[2].As<Int32>()->Value();

    data[0] = ReportID;

    //DEBUG_LOG("Get Featurereport ReprotID : %x, Length : %d...", ReportID, wLength);

    int res = 0;    
    if (g_hiddevice_info[deviceId].SetDataError>3 || g_hiddevice_info[deviceId].hiddevice == NULL)
    {
      DEBUG_LOG("SetDataError GetFeatureReport refused");
      //DebugMessage("SetDataError GetFeatureReport refused");
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"GetFeatureReport refused").ToLocalChecked()));
	    //args.GetReturnValue().Set(res); 
      args.GetReturnValue().Set(-1);
    }else{
      res = hid_get_feature_report(g_hiddevice_info[deviceId].hiddevice, data, wLength);

      if (res < 0) {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Unable to get a feature   report.").ToLocalChecked()));
        DEBUG_LOG("Unable to get a feature report....");

        args.GetReturnValue().Set(res);
      }else{
        Local<Object> buf;
        buf = node::Buffer::Copy(isolate, (char *)(data+1), res-1).ToLocalChecked();
        DEBUG_LOG("Get a feature report.... reportID : data : %x, %x, %x, %x, %x, %x, %x, %x", data[0], data  [1], data[2], data[3], data[4], data[5], data[6], data[7]);
        args.GetReturnValue().Set(buf);
      }
    }
  }
    //Get Device EP Temp Data
  void GetEPTempData(const FunctionCallbackInfo<Value>& args) {
    
    Isolate* isolate = args.GetIsolate();
    unsigned char data[2056] = {0};
    uint32_t deviceId = args[0].As<Int32>()->Value() - 1;
   
    //Check the hiddevice has failed or it's exist
    if (g_device_ep3_info[deviceId].h_datadevice == NULL)
    {
      DEBUG_LOG("SetDataError GetFeatureReport refused");
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"GetEPTempData refused").ToLocalChecked()));
      
      args.GetReturnValue().Set(-1);
    }else{
      memcpy(&data, &g_device_ep3_info[deviceId].tempData, sizeof(g_device_ep3_info[deviceId].tempData));

      //Copy Temp data to char buffer
      Local<Object> buf;
      buf = node::Buffer::Copy(isolate, (char *)(data), sizeof(g_device_ep3_info[deviceId].tempData) )  .ToLocalChecked();

      DEBUG_LOG("GetEPTempData-data : %x, %x, %x, %x, %x, %x, %x, %x", data[0], data[1], data[2], data[3],  data[4], data[5], data[6], data[7]);
	    //------return buffer data----------
      args.GetReturnValue().Set(buf);

    }
  }
  //Get Endpoint's Firmware version
  void GetFWVersion(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    unsigned char data[2056] = {0};
	  //uint32_t deviceId = args[0]->Uint32Value() - 1;

    uint32_t deviceId = args[0].As<Int32>()->Value() - 1;

	  uint32_t iFWVersion = 0;

    iFWVersion = g_hiddevice_info[deviceId].DeviceInfo.iFWVersion;
    if (iFWVersion == 0)
    {
      DEBUG_LOG("Unable to get a iFWVersion : 0");
    }
    else
    {
      DEBUG_LOG("Get a iFWVersion: %x", iFWVersion);
    }
    args.GetReturnValue().Set(iFWVersion);
  }
//
  // WinProcCallback
  //
  INT_PTR WINAPI WinProcCallback( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
  // Routine Description:
  //     Simple Windows callback for handling messages.
  //     This is where all the work is done because the example
  //     is using a window to process messages. This logic would be handled 
  //     differently if registering a service instead of a window.

  // Parameters:
  //     hWnd - the window handle being registered for events.
  //     message - the message being interpreted.
  //     wParam and lParam - extended information provided to this
  //          callback by the message sender.

  //     For more information regarding these parameters and return value,
  //     see the documentation for WNDCLASSEX and CreateWindowEx.
  {
    LRESULT lRet = 1;
    static HDEVNOTIFY hDeviceNotify;
    static HWND hEditWnd;
    static ULONGLONG msgCount = 0;

    switch (message)
    {
      case WM_DEVICECHANGE:
      {
        if(g_bHidIntercept){  
            DEBUG_LOG("WM_DEVICECHANGE-HidIntercept:%x",g_bHidIntercept);      
            break;
        }
        //
        // This is the actual message from the interface via Windows messaging.
        // This code includes some additional decoding for this particular device type
        // and some common validation checks.
        //
        // Note that not all devices utilize these optional parameters in the same
        // way. Refer to the extended information for your particular device type 
        // specified by your GUID.
        //
        DEBUG_LOG("WM_DEVICECHANGE: wParam:%x lParam:%x",wParam,lParam);
        switch (wParam)
        {
          case DBT_DEVICEARRIVAL:
          {
            DEBUG_LOG("DBT_DEVICEARRIVAL");   
            PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;

            if(pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
              PDEV_BROADCAST_DEVICEINTERFACE info = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
              NotifyHidDevicePNP(info, true);
            }                    
            break;
          }
          case DBT_DEVICEREMOVECOMPLETE:
          {
            DEBUG_LOG("DBT_DEVICEREMOVECOMPLETE");          
            PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;

            if(pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
              PDEV_BROADCAST_DEVICEINTERFACE info = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;

          	      DEBUG_LOG("info->dbcc_name:%s",info->dbcc_name); 
              NotifyHidDevicePNP(info, false);
              //When Device is removed, Stop HIDReadThreadFn loop
              
              for(int iExist = 0 ;iExist < g_iEp3Count ;iExist++){
                //stricmp is not for case of strcmp
                if(stricmp(info->dbcc_name, g_device_ep3_info[iExist].DevicePath) == 0){//Equal
          	      DEBUG_LOG("DEVICE REMOVED! DevicePath:%s",g_device_ep3_info[iExist].DevicePath); 
                  g_device_ep3_info[iExist].h_datadevice = NULL;
                  break;
                }
              }

            }                    
            break;
          }
          //case DBT_DEVNODES_CHANGED:
          //{
          //  DEBUG_LOG("DBT_DEVNODES_CHANGED");          
          //  break;
          //}

        }
      }
      break;
      case WM_CLOSE:
        if ( ! UnregisterDeviceNotification(hDeviceNotify) ){
          // ErrorHandler(TEXT("UnregisterDeviceNotification")); 
        }
        DestroyWindow(hWnd);
        break;
      case WM_DESTROY:
        PostQuitMessage(0);
      break;
      default:
        // Send all other messages on to the default windows handler.
        // lRet = DefWindowProc(hWnd, message, wParam, lParam);
      break;
    }
    return lRet;
  }

  //Create Device Notify Thread
  DWORD WINAPI ListenerThread( LPVOID lpParam ) {
  
    WNDCLASSA window_class;

    window_class .style         = CS_DBLCLKS | CS_PARENTDC;
    window_class .lpfnWndProc   = (WNDPROC)WinProcCallback;
    window_class .cbClsExtra    = 0;
    window_class .cbWndExtra    = 0;
    window_class .hInstance     = GetModuleHandle(0);
    window_class .hIcon         = NULL;
    window_class .hCursor       = NULL;
    window_class .hbrBackground = NULL;
    window_class .lpszMenuName  = NULL;
    window_class .lpszClassName = "Dummy_window";

    RegisterClassA( &window_class );

    HWND hWnd = CreateWindowExA(
        WS_EX_CLIENTEDGE | WS_EX_APPWINDOW | WS_EX_TOPMOST,
        "Dummy_window",
        "Dummy_window",
        0, // style
        0, 0, 
        0, 0,
        NULL, NULL, 
        0, 
        NULL);

    if ( hWnd == NULL ){
      // ErrorHandler(TEXT("CreateWindowEx: main appwindow hWnd"));
      //DebugMessage("ErrorHandler CreateWindowEx: main appwindow hWnd...");
      return -1;
    }
    /*
    _GUID GUID_DEVINTERFACE_USB_DEVICE;
     GUID_DEVINTERFACE_USB_DEVICE.Data1 = 0xA5DCBF10L;
     GUID_DEVINTERFACE_USB_DEVICE.Data2 = 0x6530;
     GUID_DEVINTERFACE_USB_DEVICE.Data3 = 0x11D2;
     GUID_DEVINTERFACE_USB_DEVICE.Data4[0] = 0x90;
     GUID_DEVINTERFACE_USB_DEVICE.Data4[1] = 0x1F;
     GUID_DEVINTERFACE_USB_DEVICE.Data4[2] = 0x00;
     GUID_DEVINTERFACE_USB_DEVICE.Data4[3] = 0xC0;
     GUID_DEVINTERFACE_USB_DEVICE.Data4[4] = 0x4F;
     GUID_DEVINTERFACE_USB_DEVICE.Data4[5] = 0xB9;
     GUID_DEVINTERFACE_USB_DEVICE.Data4[6] = 0x51;
     GUID_DEVINTERFACE_USB_DEVICE.Data4[7] = 0xED;
    */
    _GUID GUID_DEVINTERFACE_HID_DEVICE;

    GUID_DEVINTERFACE_HID_DEVICE = { 0x4d1e55b2, 0xf16f, 0x11cf,{ 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };//For HID Driver
    
    // GUID_DEVINTERFACE_HID_DEVICE = { 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };// GUID_DEVINTERFACE_USB_DEVICE
    //
  
    DEV_BROADCAST_DEVICEINTERFACE_A notifyFilter = { 0 };

    ZeroMemory(&notifyFilter, sizeof(notifyFilter));
    notifyFilter.dbcc_size = sizeof(notifyFilter);
    notifyFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    notifyFilter.dbcc_classguid = GUID_DEVINTERFACE_HID_DEVICE;

    HDEVNOTIFY hDevNotify =
        RegisterDeviceNotificationA(hWnd, &notifyFilter,
        // DEVICE_NOTIFY_ALL_INTERFACE_CLASSES |
        DEVICE_NOTIFY_WINDOW_HANDLE);

    if ( !hDevNotify ){
        // ErrorHandler(TEXT("RegisterDeviceNotification"));
        //DebugMessage("ErrorHandler RegisterDeviceNotification...");
        return FALSE;
    }

    MSG msg;
    while(TRUE) {
      BOOL bRet = GetMessage(&msg, hWnd, 0, 0);
      if ((bRet == 0) || (bRet == -1)) {
        break;
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    return 0;
  }

    //Cause Update Firmware or other request, temporary closure Checking
  void SwitchHidIntercept(const FunctionCallbackInfo<Value>& args) {
    
    Isolate* isolate = args.GetIsolate();
    bool bHidIntercept = args[0].As<Int32>()->Value();
    g_bHidIntercept = bHidIntercept;

    DEBUG_LOG("HidIntercept:%x",g_bHidIntercept);     
    int rtn = 0;
	  //------return buffer data----------
    args.GetReturnValue().Set(Integer::New(isolate,rtn));
  }

  //Create Device Notify Thread
  void RegisterHid() {
    devicePNPEvent = CreateEvent(NULL, false , true , "");

    //Create thread
    hid_thread = CreateThread(NULL, 0, ListenerThread, NULL, 0, &threadId);  
  } 
  //Function to Device Notify Thread
  void StartHidPnpNotify(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    RegisterHid();
  }
  //Init function for backend use
  void init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "StartHidPnpNotify", StartHidPnpNotify);
    NODE_SET_METHOD(exports, "DebugMessageCallback", RunDebugCallback);
    NODE_SET_METHOD(exports, "HIDPnpCallBack", RunHidCallback);
    //NODE_SET_METHOD(exports, "OpenDeviceHID", OpenDeviceHID);
    NODE_SET_METHOD(exports, "FindDevice", FindDevice);
    NODE_SET_METHOD(exports, "SetFeatureReport", SetFeatureReport);
    NODE_SET_METHOD(exports, "GetFeatureReport", GetFeatureReport);
    
    NODE_SET_METHOD(exports, "ClearErrorCount", ClearErrorCount);

    NODE_SET_METHOD(exports, "GetFWVersion", GetFWVersion);
    
    NODE_SET_METHOD(exports, "DeviceDataCallback", RunDeviceDataCallback);
    
    NODE_SET_METHOD(exports, "SetDeviceCallbackFunc", SetDeviceCallbackFunc);

    NODE_SET_METHOD(exports, "SetHidWrite", Set_hid_write);
    
    NODE_SET_METHOD(exports, "GetEPTempData", GetEPTempData);

    NODE_SET_METHOD(exports, "SwitchHidIntercept", SwitchHidIntercept);
  }
  NODE_MODULE(NODE_GYP_MODULE_NAME, init)
  
}