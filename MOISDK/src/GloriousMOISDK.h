
#ifndef _VirtualKey_H
#define _VirtualKey_H

#include <windows.h>

#include <node.h>
#include <node_buffer.h>
#include <v8.h>

#include <atlstr.h>//Use For CString

 #include <tlhelp32.h>


#include <uv.h>
#include <uv_async_queue.h>


#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>

typedef struct GloriousMOISDK_INFO{
	int Profile;
	std::string Filename;
};

//---------

/*****************************ENUM Defined Data**************************************/
typedef enum
{
	BUTTON_1 = 0x01,
	BUTTON_2 = 0x02,
	BUTTON_3 = 0x03,
	BUTTON_4 = 0x04,
	BUTTON_5 = 0x05,
	BUTTON_6 = 0x06,
	BUTTON_7 = 0x07,
	BUTTON_8 = 0x08,
	BUTTON_9 = 0x09,
	
	BUTTON_10 = 0x0A,
	BUTTON_11 = 0x0B,
	BUTTON_12 = 0x0C,
	BUTTON_13 = 0x0D,
	BUTTON_14 = 0x0E,
	BUTTON_15 = 0x0F,
	BUTTON_16 = 0x10,
	BUTTON_17 = 0x11,
	BUTTON_18 = 0x12
} BUTTON_ID;
typedef enum
{
	KEYBOARD_KEY_FUN		= 0x10,
	MACRO_FUN				= 0x11,
	SHORTCUTS_FUN			= 0x12,
	MOUSE_BUTTON_FUN		= 0x13,
	DPI_FUN				    = 0x14,
	MEDIA_FUN				= 0x15,
	DISABLE_BUTTON_FUN		= 0x0A,
	SHIFT_BUTTON_FUN 		= 0x0B,

} FUN_ID;
typedef enum
{
	LEFT_CLICK				= 0x01,
	RIGHT_CLICK				= 0x02,
	MIDDLE_CLICK			= 0x04,
	IE_BACKWARD				= 0x08,
	IE_FORWARD			    = 0x10,
	SCROLL_UP				= 0x11,
	SCROLL_DOWN				= 0x12,
	TRIPLE_CLICK			= 0x13,
	FIRE_KEY				= 0x14,
	PROFILE_CYCLE			= 0x15,
} MOUSE_FUN_ID;
typedef enum
{
	NEXT_CPI			= 0x00,
	PREVIOUS_CPI		= 0x01,
	NEXT_CPI_CYCLE		= 0x03,
	lOCK_CPI			= 0x0A,
} DPI_FUN_ID;
typedef enum
{
	M_MEDIA_PLAYER		= 0x01,
	M_PLAY_PAUSE		= 0x02,
	M_NEXT_TRACK		= 0x03,
	M_PREV_TRACK		= 0x04,
	M_STOP				= 0x05,
	M_VOL_MUTE			= 0x06,
	M_VOL_UP			= 0x07,
	M_VOL_DOWN			= 0x08,
} MEDIA_FUN_ID;
typedef enum
{
	OS_EMAIL		= 0x00,
	OS_CALCULATOR	= 0x01,
	OS_MY_PC		= 0x02,
	OS_EXPLORER		= 0x03,
	OS_IE_HOME		= 0x04,
	OS_IE_REFRESH	= 0x05,
	OS_IE_STOP		= 0x07,
	OS_IE_BACK		= 0x08,
	OS_IE_FORWARD	= 0x09,
	OS_IE_SEARCH	= 0x10,
	OS_KEYBOARD_HOME = 0x11,
	LAUNCH_PROGRAM	= 0x23,
	OPEN_FILE_LINK  = 0x24,
}SHORTCUTS_FUN_ID;
typedef enum            
{
	HotKey_Ctrl		= 0x01,
	HotKey_Shift	= 0x02,
	HotKey_Alt		= 0x04,
	HotKey_Win		= 0x08,
}HOT_KEY_VALUE;
/*********************** Struct DATA For Model I **********************************************/
/**********************LED data****************************************/
typedef enum
{
	LED_OFF = 0x00,
	GLORIOUS_MODE = 0x01,
	SEAMLESS_BREATHING = 0x02,
	BREATHING_RGB = 0x04,
	BREATHING_SINGLECOLOR = 0x05,
	SINGLE_COLOR = 0x06,
	TAIL_MODE = 0x07,
	RAVE_MODE = 0x13,
	WAVE_MODE = 0x09,
	CUSTOM_COLOR = 0x0F,          // 13 individual colors
} LED_EFFECT_T;

typedef struct
{
	BYTE  red;
	BYTE  green;
	BYTE  blue;
} RGB_T;
typedef struct
{
	int		    			mode;	//LED_EFFECT_T>int
	RGB_T                   led[13];
	BYTE					mode_aux;
	BYTE						brightness;
	BYTE						speed;
	BYTE					sleep_timer;
} LIGHTING_T;

/**********************button data****************************************/
/**********************Profile data****************************************/
typedef enum
{
	PROFILE_1 = 0x01,
	PROFILE_2 = 0x02,
	PROFILE_3 = 0x03,
} PROFILE_ID_T;

typedef struct
{
	BYTE					path_data[6][1024];
	unsigned short			data_length[6];
}PATH_DATA_T;
/**********************sensor data****************************************/
typedef enum
{
	PR_125 = 125,
	PR_250 = 250,
	PR_500 = 500,
	PR_1000 = 1000,
} POLLING_RATE_T;

typedef enum
{
	ON = 0x01,
	OFF = 0x00,
} ANGLE_SNAPPING_T;
typedef enum
{
	LOW = 0x00,     //1mm (default)
	HIGH = 0x01,    //2mm
} LOD_T;
typedef struct
{
	unsigned short			dpi_x;
	unsigned short			dpi_y;
	RGB_T					dpi_color;
} DPI_T;
typedef struct
{
	BYTE 					dpi_level_num;
	BYTE					dpi_level_current;
	DPI_T                   dpi[6];
} DPI_GROUP_T;
typedef struct
{
	POLLING_RATE_T			polling_rate;
	BYTE  					button_response_time;
	LOD_T					lod;//LOD_T->BYTE
	DPI_GROUP_T				dpi_t;
}PERFORMANCE_T;

//macro & button data
typedef struct
{
	BYTE					event_type;
	unsigned short			action;
	BYTE					make;
	UINT					delay;
} MACRO_EVENT_T;
typedef struct
{
	BYTE					macro_id;
	unsigned short			event_number;
	BYTE					loop_count;
	MACRO_EVENT_T			event[72];
	//MACRO_EVENT_T			event[74];
} MACRO_T;
//button data
typedef struct
{
	int						button_id;//BUTTON_ID>int
	int						function;//FUN_ID>int
	BYTE					binding;//
	unsigned short          binding_aux;
	MACRO_T					macro_t;
} BUTTON_T;

typedef struct
{
	BUTTON_T				button_t[18];//0~8:Normalkey,9~17:Shift Key
	//BUTTON_T				button_t[9];
} BUTTON_SETTINGS_T;
/*********************** Struct DATA For Model I ****************************************************/

typedef int (__stdcall *CB_MOUSEDATA)(BYTE* function, BYTE* button_data);
typedef int (__stdcall *CMI_CaptureCBData)(CB_MOUSEDATA);
//FIRMWARE_UPDATE Nouse
typedef int (__stdcall *CB_FIRMWARE_UPDATE)(BYTE* error, unsigned short* progress);
typedef int (__stdcall *FIRMWARE_UPDATE_PROGRESS)(CB_FIRMWARE_UPDATE);
//
typedef int (__stdcall *CB_DEVICE_CHANGE)(BYTE* device_status);
typedef int (__stdcall *CMI_DeviceChange)(CB_DEVICE_CHANGE);
//
typedef BYTE(*CMI_DeviceStatus)(unsigned short * p_status);
typedef BYTE(*CMI_CloceDevice)();
typedef BYTE(*CMI_GetFWVersion)(unsigned short * fw_version);
typedef BYTE(*CMI_GetBLVersion)(unsigned short * bl_version);
typedef BOOL(*CMI_GetDLLVersion)(unsigned short  * dll_version);
//
typedef BYTE(*CMI_PerformanceSetting)(UINT profile_id, PERFORMANCE_T  p_settings);
typedef BYTE(*CMI_LightingSetting)(UINT profile_id, LIGHTING_T*  lighting_settings);
typedef BYTE(*CMI_ButtonSetting)(UINT  profile_id, BUTTON_T*  button_settings);
typedef BYTE(*CMI_ALLButtonSetting)(UINT  profile_id, BUTTON_SETTINGS_T* button_sets_t);
//
typedef BYTE(*CMI_SETCURRENTPROFILEID)(UINT profile_id);

typedef BYTE(*CMI_UPDATE_FIRMWARE)(UINT file_length, BYTE* fw_data);

typedef int (WINAPI *CB_FIRMWARE_UPDATE)(BYTE* error, unsigned short* progress);
typedef BYTE(*CMI_FIRMWARE_UPDATE_PROGRESS)(CB_FIRMWARE_UPDATE);
//
typedef int(*CMI_Devicelistener)();
//
typedef BYTE(*CMI_SETLEDON)();
typedef BYTE(*CMI_SETLEDOFF)();
//

extern CMI_DeviceStatus         MI_Get_device_status;
extern CMI_CloceDevice          MI_Close_device;
extern CMI_GetFWVersion			MI_Get_fw_version;
extern CMI_GetBLVersion   	    MI_Get_bl_version;
extern CMI_CaptureCBData		MI_Capture_cb_data;

extern CMI_DeviceChange     	MI_Device_change_message;

extern CMI_GetDLLVersion		MI_Get_dll_version;
extern CMI_PerformanceSetting   MI_Set_performance_settings;
extern CMI_LightingSetting  	MI_Update_lighting_settings;
extern CMI_ButtonSetting		MI_Set_button_settings;
extern CMI_ALLButtonSetting		MI_Set_all_button_settings;


extern CMI_SETCURRENTPROFILEID  MI_Set_current_profile_id;

extern CMI_UPDATE_FIRMWARE  	MI_Update_firmware;
extern CMI_FIRMWARE_UPDATE_PROGRESS  	MI_Firmware_update_progress;

extern CMI_Devicelistener     	MI_Start_device_listener;   

extern CMI_SETLEDON  MI_Set_led_on;
extern CMI_SETLEDOFF  MI_Set_led_off;




//-------------
#endif