#include "GloriousMOISDK.h"
#include "mmsystem.h"
#pragma comment(lib, "Winmm.lib")

CString g_strUpdatestatus;

void CallbackAsync(uv_work_t* req);
void CallbackFinished(uv_work_t* req);

using namespace v8;

#define DEBUG_HEADER fprintf(stderr, "System [%s:%s() %d]: ", __FILE__, __FUNCTION__, __LINE__); 
#define DEBUG_FOOTER fprintf(stderr, "\n");
#define DEBUG_LOG(...) DEBUG_HEADER fprintf(stderr, __VA_ARGS__); DEBUG_FOOTER
#define MakeDataInRange(data, Max, Min)		do { data = max( min(data, Max), Min ); } while(0)

#define EC_SUCCESS				   0x01    
/**************FWUpdate Value*******************/
#define EC_FW_FILEDATA      	0x50		//Firmware file ERROR
#define EC_FW_SEND_REQUEST      0x51		//Send_bl_request ERROR
#define EC_FW_OPEN_BLDR         0x52        //open bootloader ERROR
#define EC_FW_SEND_BL_START     0x53        //Send_bl_start ERROR
#define EC_FW_SEND_FW_BLOCK     0x54        //Send_fw_block ERROR
#define EC_FW_SEND_BL_COMPLETE  0x55        //Send_bl_complete ERROR
#define EC_FW_NO_DEVICE  		0x56        //Send_bl_complete ERROR
#define EC_FW_RETURN_VERSION  	0x60        //Return firmware version
/**************FWUpdate Value*******************/
Persistent<Value> Callback;

//-------------------------------------------------

CMI_DeviceStatus        MI_Get_device_status = NULL;
CMI_CloceDevice         MI_Close_device = NULL;
CMI_GetFWVersion		MI_Get_fw_version = NULL;
CMI_GetBLVersion   	    MI_Get_bl_version = NULL;
CMI_CaptureCBData		MI_Capture_cb_data = NULL;
CMI_DeviceChange     	MI_Device_change_message = NULL;
CMI_GetDLLVersion		MI_Get_dll_version = NULL;
CMI_PerformanceSetting  MI_Set_performance_settings = NULL;
CMI_LightingSetting  	MI_Update_lighting_settings = NULL;
CMI_ButtonSetting		MI_Set_button_settings = NULL;
CMI_ALLButtonSetting	MI_Set_all_button_settings = NULL;
//
CMI_SETCURRENTPROFILEID  	MI_Set_current_profile_id = NULL;
//
CMI_UPDATE_FIRMWARE  	MI_Update_firmware = NULL;
CMI_FIRMWARE_UPDATE_PROGRESS  	MI_Firmware_update_progress = NULL;
//
CMI_Devicelistener      MI_Start_device_listener = NULL;
//
CMI_SETLEDON      MI_Set_led_on = NULL;
CMI_SETLEDOFF      MI_Set_led_off = NULL;
//
CString g_DLLPath = _T("");
//------------------------------------------------

namespace GloriousMOISDK {  
  using v8::Function;
  using v8::FunctionCallbackInfo;
  using v8::Isolate;
  using v8::Local;
  using v8::Null;
  using v8::Object;
  using v8::String;
  using v8::Value;
  using v8::HandleScope;

#ifdef _UNICODE
#define SPRINTF	swprintf_s
#else
#define SPRINTF	sprintf_s
#endif

int WINAPI Get_Data_Func(BYTE* function, BYTE* button_data)
{	

	DEBUG_LOG("Get button callback data from dll ok");

	//m_DlgConfig->DisplayDeviceData(function, button_data);
	return 1;
}
int WINAPI Device_change(BYTE* device_status)
{
  char msg[512] = {0};
	//---------Receice Device_change data from Dll's data structure----------
	sprintf(msg,"Model_I_device_status: %d \n",device_status);
  DEBUG_LOG(msg);
	return 1;
}
//----------------------------------

BOOL DriverInit(void)
{
	// WriteLogString("DriverInit");
	BOOL bSuccess = FALSE;

	char Path[MAX_PATH];
    // char *buffer;	
	// buffer = getcwd(NULL, 0);
	
	// CString csTemp = _T("backend\\protocol\\");
	// if((buffer = getcwd(NULL, 0)) != NULL){
	// 	sprintf(Path,"./resources");
	// 	int ftyp = _access(Path, 0);

	// 	if (0 == ftyp)
	// 		csTemp.Format(_T("resources\\app\\backend\\protocol\\"));
  // }

	// Load SDK DLL
	CString strPath = _T("");


	// if (IsWow64()){	
	// 	strPath.Format(_T("%snodeDriver\\DllSDK\\model_i_dll.dll"),csTemp);
	// }else{			
	// 	strPath.Format(_T("%snodeDriver\\DllSDK\\model_i_dll.dll"),csTemp);
	// }

// 	//------------------Use g_DLLPath---------------------
// 	if(g_DLLPath.GetLength() > 0){
// 		strPath = g_DLLPath;
// 	}
// 	//------------------Use g_DLLPath---------------------

// 	// WriteLogString("GloriousMOISDK-strPath:%s",strPath);

// 	DEBUG_LOG("DriverInit-strPath:%s",strPath);
// 	HINSTANCE hModelIDriver = LoadLibrary(strPath);//Load DLL Library with DLL's filepath
// 	DEBUG_LOG("LoadLibrary-hModelIDriver:%x",hModelIDriver);

  //--------------------------------	
	HINSTANCE hModelIDriver;
	TCHAR	szDllPath[512];
	TCHAR	tmpBuffer[512];
	//if (!hModelIDriver)
	{
		szDllPath[0] = 0;
		SPRINTF(tmpBuffer, _countof(tmpBuffer), _T("DLLSDK\\%s"), "model_i_dll.dll");
		DEBUG_LOG("LoadLibrary-tmpBuffer:%s",tmpBuffer);
		hModelIDriver = LoadLibrary(tmpBuffer);
	}

	DEBUG_LOG("LoadLibrary-hModelIDriver:%x",hModelIDriver);
  // char msg[512] = {0};
	// sprintf(msg,"LoadLibrary-hModelIDriver:%x\n",hModelIDriver);
  // DebugMessage(msg);
	if (hModelIDriver != NULL)
	{
		//Get device status from firmware DLL
		MI_Get_device_status = (CMI_DeviceStatus)GetProcAddress(hModelIDriver, "Get_device_status");
		MI_Close_device = (CMI_CloceDevice)GetProcAddress(hModelIDriver, "Close_device");	
		//Get firmware version from firmware DLL
		MI_Get_fw_version = (CMI_GetFWVersion)GetProcAddress(hModelIDriver, "Get_fw_version");
		//Get bootloader version from firmware DLL
		MI_Get_bl_version = (CMI_GetBLVersion)GetProcAddress(hModelIDriver, "Get_bl_version");
		MI_Capture_cb_data = (CMI_CaptureCBData)GetProcAddress(hModelIDriver, "Capture_cb_data");
		MI_Device_change_message = (CMI_DeviceChange)GetProcAddress(hModelIDriver, "Device_change_message");
		MI_Get_dll_version = (CMI_GetDLLVersion)GetProcAddress(hModelIDriver, "Get_dll_version");
		//Set performance settings into firmware DLL
		MI_Set_performance_settings = (CMI_PerformanceSetting)GetProcAddress(hModelIDriver, "Set_performance_settings");
		//Set lightig settings into firmware DLL
		MI_Update_lighting_settings = (CMI_LightingSetting)GetProcAddress(hModelIDriver, "Update_lighting_settings");
		//Set button settings into firmware DLL
		MI_Set_button_settings = (CMI_ButtonSetting)GetProcAddress(hModelIDriver, "Set_button_settings");
		//Set All button settings into firmware DLL
		MI_Set_all_button_settings = (CMI_ALLButtonSetting)GetProcAddress(hModelIDriver, "Set_all_button_settings");

		//Change Hardware Profile ID into firmware DLL
		MI_Set_current_profile_id = (CMI_SETCURRENTPROFILEID)GetProcAddress(hModelIDriver, "Set_current_profile_id");
		//Update Model I into firmware DLL
		MI_Update_firmware = (CMI_UPDATE_FIRMWARE)GetProcAddress(hModelIDriver, "Update_firmware");
		MI_Firmware_update_progress = (CMI_FIRMWARE_UPDATE_PROGRESS)GetProcAddress(hModelIDriver, "Firmware_update_progress");
		//Start_device_listener
		MI_Start_device_listener = (CMI_Devicelistener)GetProcAddress(hModelIDriver, "Start_device_listener");
		//Turn on the led light
		MI_Set_led_on = (CMI_SETLEDON)GetProcAddress(hModelIDriver, "Set_led_on");
		//Shut down the led light
		MI_Set_led_off = (CMI_SETLEDOFF)GetProcAddress(hModelIDriver, "Set_led_off");

		bSuccess = TRUE;
	}
	else
		// WriteLogString("GloriousMOISDK-LoadLibrary_Fail");

	// WriteLogString("GloriousMOISDK-DriverInit_end");
	return bSuccess;
}

//---------------------------------
void Initialization(const FunctionCallbackInfo<Value>& args){
	  DEBUG_LOG("in_Initialization");
    bool bInit = false;
	  if (DriverInit()){//Check DLL Driver Initialization is success
      bInit = true;
			
			MI_Start_device_listener();
			MI_Device_change_message(Device_change);
    }
		//------return result value----------
  	args.GetReturnValue().Set(bInit);  
}
void DllPath(const FunctionCallbackInfo<Value>& args){
	DEBUG_LOG("DllPath Begin");
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	v8::String::Utf8Value str(isolate, args[0]);
    std::string foo = std::string(*str);   
	CString cString(foo.c_str());

	g_DLLPath = cString;
  
  char msg[512] = {0};
	sprintf(msg,"DllPath: %s \n",cString);
  DEBUG_LOG(msg);
	args.GetReturnValue().Set(1); 
}

void ChangeCurProfileID(const FunctionCallbackInfo<Value>& args){
	//---------turn App data into Dll's data structure----------
	int iProfile = args[0].As<Int32>()->Value();
	//-----------Send Dll's data structure into dll-------------
	MI_Set_current_profile_id(iProfile);
	args.GetReturnValue().Set(1); 
}
void SetLEDOnOff(const FunctionCallbackInfo<Value>& args){
	//---------turn App data into Dll's data structure----------
	int iOnOff = (int)args[0].As<Int32>()->Value();
	//-----------Send Dll's data structure into dll-------------
	
  DEBUG_LOG("SetLEDOnOff iOnOff: %x", iOnOff);
	if(iOnOff){//LED ON
		MI_Set_led_on();
	}else{//LED OFF
		MI_Set_led_off();
	}
  DEBUG_LOG("SetLEDOnOff Done");
	args.GetReturnValue().Set(1);  
}

void SetLEDEffect(const FunctionCallbackInfo<Value>& args){

	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	char msg[512] = {0};
	//---------turn App data into Dll's data structure----------
	int iProfile = args[0].As<Int32>()->Value();
	sprintf(msg,"Model_I_SetEffect Profile: %d \n",iProfile);
  	DEBUG_LOG(msg);

	int UIEffectId = args[1].As<Int32>()->Value();
	int UIbrightness = args[2].As<Int32>()->Value();
	int UIspeed = args[3].As<Int32>()->Value();
	int UIsleeptime = args[4].As<Int32>()->Value();
	int UImodeaux = args[5].As<Int32>()->Value();
	unsigned char *inBuf = (unsigned char *)node::Buffer::Data(args[6]->ToObject(context).ToLocalChecked());
	unsigned char Data[1024] = {0};

	int iDataLen = *inBuf;
	memcpy(Data, inBuf + 1, iDataLen);
	int iColorLen = inBuf[0];

	sprintf(msg,"SetEffect iDataLen: %d \n",iDataLen);
  DEBUG_LOG(msg);
	sprintf(msg,"SetEffect iColorLen: %d \n",iColorLen);
  DEBUG_LOG(msg);

  ///---------------------------
  LIGHTING_T  lighting_settings;
	lighting_settings.mode = UIEffectId;
	for (int i = 0; i < iColorLen; i++){
      lighting_settings.led[i].red   = Data[i * 3];
      lighting_settings.led[i].green = Data[i * 3 + 1];
      lighting_settings.led[i].blue  = Data[i * 3 + 2];
  }
  
	lighting_settings.brightness = UIbrightness;
	lighting_settings.speed = UIspeed;
	lighting_settings.sleep_timer = UIsleeptime;
	lighting_settings.mode_aux = UImodeaux;

	sprintf(msg,"mode: %d \n",lighting_settings.mode);
  	DEBUG_LOG(msg);
	sprintf(msg,"brightness: %d \n",lighting_settings.brightness);
  	DEBUG_LOG(msg);
	sprintf(msg,"sleep_timer: %d \n",lighting_settings.sleep_timer);
  	DEBUG_LOG(msg);
	sprintf(msg,"speed: %d \n",lighting_settings.speed);
  	DEBUG_LOG(msg);

	//------Send Dll's data structure into dll----------
  	int result = 0;
	result = MI_Update_lighting_settings(iProfile, &lighting_settings);

	//------return result value----------
	args.GetReturnValue().Set(result);  
}
//
void SetButtonFunc(const FunctionCallbackInfo<Value>& args){

	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

  	char msg[512] = {0};
	//---------turn App data into Dll's data structure----------
	int iProfile = (int)args[0].As<Int32>()->Value();
	int iButtonID = (int)args[1].As<Int32>()->Value();
	int DLLfunction = (int)args[2].As<Int32>()->Value();
	int DLLbinding = (int)args[3].As<Int32>()->Value();
	int DLLbinding_aux = (int)args[4].As<Int32>()->Value();
	sprintf(msg,"Model_I_SetButtonFunc Profile: %d \n",iProfile);
  	DEBUG_LOG(msg);
	//---------------------------
	unsigned char *inBuf = (unsigned char *)node::Buffer::Data(args[5]->ToObject(context).ToLocalChecked());
	unsigned char DataMacro[1024] = {0};
	//int iDataLen = *inBuf;
  //DataMacro into NodeDriver Byte Count:3 + 72*5 = 363
	memcpy(DataMacro, inBuf, 363);
	
	//---------------------------
  BUTTON_T  button_settings;
	button_settings.button_id = iButtonID;
	button_settings.function = DLLfunction;
	button_settings.binding = DLLbinding;
	button_settings.binding_aux = DLLbinding_aux;
	//button_settings.macro_t
	memset(&button_settings.macro_t, 0x00, sizeof(&button_settings.macro_t));

  //-----Macro function-----
	//MACRO_T					macro_t;
	button_settings.macro_t.macro_id = DataMacro[0];
	button_settings.macro_t.event_number = DataMacro[1];
	button_settings.macro_t.loop_count = DataMacro[2];

	if(button_settings.macro_t.macro_id != 0){
		sprintf(msg,"loop_count: %d \n",button_settings.macro_t.loop_count);
  	DEBUG_LOG(msg);
	}
	//bool bPrintMacro = (button_settings.macro_t.macro_id > 0 ? true : false);//Check MacroData Is Exist
	int iEventCount = button_settings.macro_t.event_number;
	for (int i = 0; i < iEventCount; i++){
		button_settings.macro_t.event[i].event_type = DataMacro[3 + i*5 +0];
		button_settings.macro_t.event[i].action = DataMacro[3 + i*5 +1];
		button_settings.macro_t.event[i].make = DataMacro[3 + i*5 +2];
		int iDelay = DataMacro[3 + i*5 +3]*256+DataMacro[3 + i*5 +4];
		button_settings.macro_t.event[i].delay = iDelay;
	}
	//------Send Dll's data structure into dll----------
  int result = 0;
	result = MI_Set_button_settings(iProfile, &button_settings);
	//------return result value----------
	args.GetReturnValue().Set(result);
}

//
void SetAllButton(const FunctionCallbackInfo<Value>& args){
	char msg[512] = {0};

	//---------turn App data into Dll's data structure----------
	int iProfile = args[0].As<Int32>()->Value();
	sprintf(msg,"Model_I_SetButtonFunc Profile: %d \n",iProfile);
  	DEBUG_LOG(msg);
	//---------------------------
  	Isolate* isolate = args.GetIsolate();
  	Local<Context> context = isolate->GetCurrentContext();
  	
	//Check is NodeJs Object_array 
	if (!args[1]->IsObject())
	{
		sprintf(msg,"object is incoorect! \n");
		DEBUG_LOG(msg);
		args.GetReturnValue().Set(-1);
	}else{
	//NodeJs is Object_array 
			Local<Array> ObjButton_array = args[1].As<Array>();

			int iLength = ObjButton_array->Length();
			// sprintf(msg,"iLength:%d \n",iLength);
  		// 	DEBUG_LOG(msg);

			//Clear button_settings temp data
  			BUTTON_SETTINGS_T  button_settings;
			memset(&button_settings, 0x00, sizeof(&button_settings));
			//---------assign button setting------------
			for (int i = 0; i < iLength; i++){
				Local<Object> objarr;
				Local<Value> localVal;
				MaybeLocal<Object> tempObj;
				MaybeLocal<Value> maybelocalVal;

				MaybeLocal<Value> objarr1 = ObjButton_array->Get(context, i);

				if(!objarr1.IsEmpty()) {
					objarr1.ToLocal(&localVal);
					tempObj = localVal->ToObject(context);
					tempObj.ToLocal(&objarr);
				}

  		  	maybelocalVal = objarr->Get(context, v8::String::NewFromUtf8(isolate, "DLLButtonID").ToLocalChecked		());

				if(!maybelocalVal.IsEmpty())
  		  		localVal = maybelocalVal.ToLocalChecked();
				int iDLLButtonID = localVal->NumberValue(context).FromMaybe(0);

  		  	maybelocalVal = objarr->Get(context, String::NewFromUtf8(isolate, "DLLfunction").ToLocalChecked());

				if(!maybelocalVal.IsEmpty())
  		  	 	localVal = maybelocalVal.ToLocalChecked();
				 int iDLLfunction = localVal->NumberValue(context).FromMaybe(0);

				maybelocalVal = objarr->Get(context, String::NewFromUtf8(isolate, "DLLbinding").ToLocalChecked());

				if(!maybelocalVal.IsEmpty())
  		  		localVal = maybelocalVal.ToLocalChecked();
				int iDLLbinding = localVal->NumberValue(context).FromMaybe(0);

				maybelocalVal = objarr->Get(context, String::NewFromUtf8(isolate, "DLLbinding_aux").ToLocalChecked());

				if(!maybelocalVal.IsEmpty())
  		  		localVal = maybelocalVal.ToLocalChecked();
				int iDLLbinding_aux = localVal->NumberValue(context).FromMaybe(0);

				//---------assign macro setting Into Dll value------------
				maybelocalVal = objarr->Get(context, String::NewFromUtf8(isolate, "DLLDataMacro").ToLocalChecked());
				if(!maybelocalVal.IsEmpty())
  		  		localVal = maybelocalVal.ToLocalChecked();
				Local<Array> Macro_array = Local<Array>::Cast(localVal);

				//Check MacroData Is Exist By Macro_array's Length
				int iarrLength = Macro_array->Length();


				if(iarrLength>0 && Macro_array->IsArray() ){
					int iMacroLength = (iarrLength-3)/5;
					BYTE ivalue,ivalue2;

					maybelocalVal = Macro_array->Get(context, 0).ToLocalChecked();
					if(!maybelocalVal.IsEmpty())
  		  			localVal = maybelocalVal.ToLocalChecked();			
					ivalue = localVal->NumberValue(context).FromMaybe(0);
					button_settings.button_t[i].macro_t.macro_id = ivalue;

					sprintf(msg,"macro_id:%d \n",ivalue);
  					DEBUG_LOG(msg);

					maybelocalVal = Macro_array->Get(context, 1).ToLocalChecked();
					if(!maybelocalVal.IsEmpty())
  		  			localVal = maybelocalVal.ToLocalChecked();
					ivalue = localVal->NumberValue(context).FromMaybe(0);
					button_settings.button_t[i].macro_t.event_number = ivalue;

					sprintf(msg,"event_number:%d \n",ivalue);
  					DEBUG_LOG(msg);

					maybelocalVal = Macro_array->Get(context, 2).ToLocalChecked();
					if(!maybelocalVal.IsEmpty())
  		  			localVal = maybelocalVal.ToLocalChecked();

					ivalue = localVal->NumberValue(context).FromMaybe(0);
					button_settings.button_t[i].macro_t.loop_count = ivalue;
					sprintf(msg,"loop_count:%d \n",ivalue);
  					DEBUG_LOG(msg);


					// sprintf(msg,"Button ID:%d,iMacroLength:%d \n",i,iMacroLength);
  				// 	DEBUG_LOG(msg);
					for (int ievent = 0; ievent < iMacroLength; ievent++){
						//Macro_array->Get(3 + i*5 +0)->NumberValue()
						maybelocalVal = Macro_array->Get(context, 3+ievent*5 +0).ToLocalChecked();
						if(!maybelocalVal.IsEmpty())
  		  				localVal = maybelocalVal.ToLocalChecked();		
						ivalue = localVal->NumberValue(context).FromMaybe(0);
						button_settings.button_t[i].macro_t.event[ievent].event_type = ivalue;

						maybelocalVal = Macro_array->Get(context, 3+ievent*5 +1).ToLocalChecked();
						if(!maybelocalVal.IsEmpty())
  		  				localVal = maybelocalVal.ToLocalChecked();		
						ivalue = localVal->NumberValue(context).FromMaybe(0);
						button_settings.button_t[i].macro_t.event[ievent].action = ivalue;

						maybelocalVal = Macro_array->Get(context, 3+ievent*5 +2).ToLocalChecked();
						if(!maybelocalVal.IsEmpty())
  		  				localVal = maybelocalVal.ToLocalChecked();		
						ivalue = localVal->NumberValue(context).FromMaybe(0);
						button_settings.button_t[i].macro_t.event[ievent].make = ivalue;

						maybelocalVal = Macro_array->Get(context, 3+ievent*5 +3).ToLocalChecked();
						if(!maybelocalVal.IsEmpty())
  		  				localVal = maybelocalVal.ToLocalChecked();		
						ivalue = localVal->NumberValue(context).FromMaybe(0);
						maybelocalVal = Macro_array->Get(context, 3+ievent*5 +4).ToLocalChecked();
						if(!maybelocalVal.IsEmpty())
  		  				localVal = maybelocalVal.ToLocalChecked();		
						ivalue2 = localVal->NumberValue(context).FromMaybe(0);
						int iDelay = ivalue * 256 + ivalue2;
						button_settings.button_t[i].macro_t.event[ievent].delay = iDelay;
					}
				}
				//---------assign DLL button setting------------
				button_settings.button_t[i].button_id = iDLLButtonID;
				button_settings.button_t[i].function = iDLLfunction;
				button_settings.button_t[i].binding = iDLLbinding;
				button_settings.button_t[i].binding_aux = iDLLbinding_aux;
			}

			//------Send Dll's data structure into dll----------
  		int result = 0;
			result = MI_Set_all_button_settings(iProfile, &button_settings);
			//------return result value----------
			args.GetReturnValue().Set(result);

	}

}
//
void SetPerformance(const FunctionCallbackInfo<Value>& args){

	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

  	char msg[512] = {0};
	//---------turn App data into Dll's data structure----------
	int iProfile = args[0].As<Int32>()->Value();
	int PollingRate = args[1].As<Int32>()->Value();
	int Responsetime = args[2].As<Int32>()->Value();
	int LODValue = args[3].As<Int32>()->Value();
	int DPINumber = args[4].As<Int32>()->Value();
	int DPICurStage = args[5].As<Int32>()->Value();
	sprintf(msg,"Model_I_SetPerformance Profile: %d \n",iProfile);
  	DEBUG_LOG(msg);
	//-------------DPIData Array--------------
  	DEBUG_LOG(msg);
	unsigned char *inBuf = (unsigned char *)node::Buffer::Data(args[6]->ToObject(context).ToLocalChecked());
	unsigned char DataDPIValue[25] = {0};
	int iDPIDataLen = *inBuf;
	memcpy(DataDPIValue, inBuf + 1, iDPIDataLen);
	sprintf(msg,"iDPIDataLen: %d \n",iDPIDataLen);
  	DEBUG_LOG(msg);
	//-------------DPIColorData Array--------------
	unsigned char *inBuf2 = (unsigned char *)node::Buffer::Data(args[7]->ToObject(context).ToLocalChecked());
	unsigned char DataDPIColor[19] = {0};
	int iColorDataLen = *inBuf2;
	memcpy(DataDPIColor, inBuf2 + 1, iColorDataLen);
	sprintf(msg,"iColorDataLen: %d \n",iColorDataLen);
  DEBUG_LOG(msg);
	//--------------PERFORMANCE_T Data Structure-------------
	// POLLING_RATE_T		polling_rate;
	// BYTE  				button_response_time;
	// LOD_T				lod;
	// DPI_GROUP_T			dpi_t---BYTE 		dpi_level_num;
	//													BYTE		dpi_level_current;
	//													DPI_T   dpi[6]--- unsigned short			dpi_x;
	//																					  unsigned short			dpi_y;
	//																						RGB_T					dpi_color;
//-------------------------------------------------------------
  PERFORMANCE_T  performance_settings;
	//memset(&performance_settings, 0x00, sizeof(&performance_settings));
	performance_settings.button_response_time = Responsetime;
	performance_settings.lod = LOW;
	performance_settings.dpi_t.dpi_level_num = DPINumber;
	performance_settings.dpi_t.dpi_level_current = DPICurStage;
	//-------------------Set Polling Rate------------------------
	switch (PollingRate) 
	{
		case 125:
		performance_settings.polling_rate = PR_125;
		break;
		case 250:
		performance_settings.polling_rate = PR_250;
		break;
		case 500:
		performance_settings.polling_rate = PR_500;
		break;
		default://Else:1000Hz
		performance_settings.polling_rate = PR_1000;
		break;
	}
	//-------------------Set Lod Value------------------------
	switch (LODValue) 
	{
		case 1://
		performance_settings.lod = HIGH;
		break;
		default://Else:LOW
		performance_settings.lod = LOW;
		break;
	}
	//-------------------Set Polling Rate------------------------

	//Show Debug log
	sprintf(msg,"polling_rate: %d \n",performance_settings.polling_rate);
  		DEBUG_LOG(msg);
	sprintf(msg,"button_response_time: %d \n",performance_settings.button_response_time);
  		DEBUG_LOG(msg);
	sprintf(msg,"lod: %d \n",performance_settings.lod);
  		DEBUG_LOG(msg);
	sprintf(msg,"dpi_level_num: %d \n",performance_settings.dpi_t.dpi_level_num);
  		DEBUG_LOG(msg);
	sprintf(msg,"dpi_level_current: %d \n",performance_settings.dpi_t.dpi_level_current);
  		DEBUG_LOG(msg);
	
	for (int i = 0; i < DPINumber; i++){
      performance_settings.dpi_t.dpi[i].dpi_x = (short)DataDPIValue[i * 4]*256+DataDPIValue[i * 4 + 1];
      performance_settings.dpi_t.dpi[i].dpi_y = (short)DataDPIValue[i * 4 +2]*256+DataDPIValue[i * 4 + 3];

			sprintf(msg,"DPI-%d ,dpi_x: %d \n",i,performance_settings.dpi_t.dpi[i].dpi_x);
  		DEBUG_LOG(msg);
      performance_settings.dpi_t.dpi[i].dpi_color.red = DataDPIColor[i * 3];
      performance_settings.dpi_t.dpi[i].dpi_color.green = DataDPIColor[i * 3 + 1];
      performance_settings.dpi_t.dpi[i].dpi_color.blue = DataDPIColor[i * 3 + 2];
			sprintf(msg,"DPI-%d ,r: %d,g: %d,b: %d \n",i,DataDPIColor[i * 3],DataDPIColor[i * 3+1],DataDPIColor[i * 3+2]);
  		DEBUG_LOG(msg);
  }
	//------Send Dll's data structure into dll----------
  int result = 0;
	result = MI_Set_performance_settings(iProfile, performance_settings);
	args.GetReturnValue().Set(result);
}
  //Get  Firmware version From DLL
  void GetFWVersion(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
		unsigned char	 uret = 0;
		unsigned short iFWVersion = 0;

		uret = MI_Get_fw_version(&iFWVersion);

    if (uret == EC_SUCCESS)//EC_SUCCESS
    {
      DEBUG_LOG("Get a iFWVersion: %x", iFWVersion);
    }
    else
    {
      DEBUG_LOG("Unable to get a iFWVersion : 0");
    }
    args.GetReturnValue().Set(iFWVersion);
  }
  //Get  DLL version From DLL
  void GetDLLVersion(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
		unsigned char	 uret = 0;
		unsigned short iDLLVersion = 0;

		uret = MI_Get_dll_version(&iDLLVersion);

    if (uret == EC_SUCCESS)//EC_SUCCESS
    {
      DEBUG_LOG("Get a iDLLVersion: %x", iDLLVersion);
    }
    else
    {
      DEBUG_LOG("Unable to get a iDLLVersion : 0");
    }
    args.GetReturnValue().Set(iDLLVersion);
  }
	//Update Model I progress from firmware DLL
	int WINAPI Firmware_progress(BYTE* error, unsigned short* progress)
	{
		unsigned short ndata = *error;
		unsigned short nid = *progress;
		bool bPass = false;
		
		CString strText;
		if (ndata != EC_SUCCESS){
			switch (ndata){
			case EC_FW_FILEDATA:
				strText.Format(_T("PEC_FW_FILEDATA: 0x%x, progress: %d"), ndata, nid);
			break;
			case EC_FW_SEND_REQUEST:
				strText.Format(_T("EC_FW_SEND_REQUEST: 0x%x, progress: %d"), ndata, nid);
			break;
			case EC_FW_OPEN_BLDR:
				strText.Format(_T("EC_FW_OPEN_BLDR: 0x%x, progress: %d"), ndata, nid);
			break;
			case EC_FW_SEND_BL_START:
				strText.Format(_T("EC_FW_SEND_BL_START: 0x%x, progress: %d"), ndata, nid);
			break;
			case EC_FW_SEND_FW_BLOCK:
				strText.Format(_T("EC_FW_SEND_FW_BLOCK: 0x%x, progress: %d"), ndata, nid);
			break;
			case EC_FW_SEND_BL_COMPLETE:
				strText.Format(_T("TEC_FW_SEND_BL_COMPLETE: 0x%x, progress ID: %d"), ndata, nid);
			break;
			case EC_FW_NO_DEVICE:
				strText.Format(_T("EC_FW_NO_DEVICE: 0x%x, progress ID: %d"), ndata, nid);
			break;
			case EC_FW_RETURN_VERSION:{
				float mouse_version = nid / 100.0f;
				strText.Format(_T("Update firmware succeed, Firmware version:%.2f"), mouse_version);
				bPass = true;
			}break;
			default:
				break;
		}
    DEBUG_LOG("%s", strText);
		if(bPass){
			g_strUpdatestatus.Format(_T("GETPROGRESS:PASS"));
		}else{
			g_strUpdatestatus.Format(_T("GETPROGRESS:FAIL-%s",strText));
		}
	}else{//Updating-get progress From DLL and Refresh Progress String
			int		 progressPos = *progress;
	    DEBUG_LOG("Firmware_progress: %d", progressPos);
			g_strUpdatestatus.Format(_T("GETPROGRESS:%d"),progressPos);
	}
			//m_DlgConfig->Display_update_progress(error, progress);
			return 1;
	}
		//Update Model I into firmware DLL
  void UpdateFirmware(const FunctionCallbackInfo<Value>& args) {

	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();

	v8::String::Utf8Value str(isolate, args[0]);
    std::string foo = std::string(*str);  
	CString strPath(foo.c_str()); 
	//String strPath(foo.c_str());
	//--------------------------------
	UINT	bRet = 0;
	FILE * 	file;
	UINT    fileLength = 0;

    DEBUG_LOG("strPath: %s", strPath);
	file = fopen(strPath,"rb");  // r for read, b for binary
	if (file==NULL) {
    	DEBUG_LOG("File error");
	}
  	// obtain file size:
	bRet = fseek(file, 0, SEEK_END);
	fileLength = ftell(file);
    DEBUG_LOG("fileLength: %x", fileLength);
  	rewind (file);
  	// allocate memory to contain the whole file:
		BYTE	*fwfile = new BYTE[(unsigned int)fileLength];
		bRet = fread(fwfile,fileLength,1,file); // read fileLength bytes to our buffer
    DEBUG_LOG("fread: %d", bRet);

		//Check Firmware file is exist
		if(bRet){
			MI_Update_firmware(fileLength, fwfile);//Start Firmware Update
			MI_Firmware_update_progress(Firmware_progress);//Make Callback to the Function For Get Update Process
		}
    args.GetReturnValue().Set(bRet);
  }
//Send Message Into exe winsocket server
void GetUpdateStats(const FunctionCallbackInfo<Value>& args) {  
	Isolate* isolate = args.GetIsolate();
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, g_strUpdatestatus).ToLocalChecked());
}
//Capture CB Data
void CaptureCBData(const FunctionCallbackInfo<Value>& args){
  DEBUG_LOG("MI_Capture_cb_data Begin");
	MI_Capture_cb_data(Get_Data_Func);
  DEBUG_LOG("MI_Capture_cb_data Done");
	args.GetReturnValue().Set(1);  
}

//Init function for backend use
void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "Initialization", Initialization);
  NODE_SET_METHOD(exports, "DllPath", DllPath);

  NODE_SET_METHOD(exports, "SetLEDEffect", SetLEDEffect);
  NODE_SET_METHOD(exports, "SetButtonFunc", SetButtonFunc);
  NODE_SET_METHOD(exports, "SetAllButton", SetAllButton);
  NODE_SET_METHOD(exports, "SetPerformance", SetPerformance);

  NODE_SET_METHOD(exports, "ChangeCurProfileID", ChangeCurProfileID);
	
  NODE_SET_METHOD(exports, "GetFWVersion", GetFWVersion);
  NODE_SET_METHOD(exports, "GetDLLVersion", GetDLLVersion);
  NODE_SET_METHOD(exports, "UpdateFirmware", UpdateFirmware);
  NODE_SET_METHOD(exports, "GetUpdateStats", GetUpdateStats);

  NODE_SET_METHOD(exports, "SetLEDOnOff", SetLEDOnOff);

  NODE_SET_METHOD(exports, "CaptureCBData", CaptureCBData);
}
  NODE_MODULE(NODE_GYP_MODULE_NAME, init)
}  // namespace demo