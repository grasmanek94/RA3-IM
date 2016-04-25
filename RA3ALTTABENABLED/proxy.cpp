#include <Windows.h>
#include <Xinput.h>
#include <thread>
#include <atomic>

//void FrameTick();
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow);
HINSTANCE mainLOL = NULL;
std::atomic<bool> running = true;
void initializator(void)
{
//	while(running)
//	{
//		FrameTick();
//	}
	WinMain(mainLOL,
                   NULL,
                   "",
                   SW_SHOW);
	return;
}

std::thread * runner = NULL;
HANDLE timer_handle_;
void TimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    DeleteTimerQueueTimer(NULL, timer_handle_, NULL);
    timer_handle_ = NULL;
	static bool done = false;
	if(!done)
	{
		runner = new std::thread(initializator);
		done = true;
	}
}

void DllMain2(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		mainLOL = hinstDLL;
		CreateTimerQueueTimer(&timer_handle_, NULL, (WAITORTIMERCALLBACK)TimerProc, 0, 0, 0, 0);
	}
	if(fdwReason == DLL_PROCESS_DETACH)
	{
		running = false;
		runner->join();
	}
}

typedef BOOL *(WINAPI* DllMainType)(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved);
typedef void *(*XInputEnableType)(BOOL enable);
typedef DWORD *(*XInputGetAudioDeviceIdsType)(DWORD dwUserIndex,LPWSTR pRenderDeviceId,UINT *pRenderCount,LPWSTR pCaptureDeviceId,UINT *pCaptureCount);
typedef DWORD *(*XInputGetBatteryInformationType)(DWORD dwUserIndex,BYTE devType,XINPUT_BATTERY_INFORMATION *pBatteryInformation);
typedef DWORD *(*XInputGetCapabilitiesType)(DWORD dwUserIndex,DWORD dwFlags,XINPUT_CAPABILITIES *pCapabilities);
typedef DWORD *(*XInputGetDSoundAudioDeviceGuidsType)(DWORD dwUserIndex,GUID* pDSoundRenderGuid,GUID* pDSoundCaptureGuid);
typedef DWORD *(*XInputGetKeystrokeType)(DWORD dwUserIndex,DWORD dwReserved,PXINPUT_KEYSTROKE pKeystroke);
typedef DWORD *(*XInputGetStateType)(DWORD dwUserIndex,XINPUT_STATE *pState);
typedef DWORD *(*XInputSetStateType)(DWORD dwUserIndex,XINPUT_VIBRATION *pVibration);

namespace Original
{	
	HMODULE module= NULL;
	DllMainType	xDllMain;
	XInputEnableType xXInputEnable;
	XInputGetAudioDeviceIdsType xXInputGetAudioDeviceIds;
	XInputGetBatteryInformationType xXInputGetBatteryInformation;
	XInputGetCapabilitiesType xXInputGetCapabilities;
	XInputGetDSoundAudioDeviceGuidsType xXInputGetDSoundAudioDeviceGuids;
	XInputGetKeystrokeType xXInputGetKeystroke;
	XInputGetStateType xXInputGetState;
	XInputSetStateType xXInputSetState;
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	if(Original::module == NULL)
	{
		Original::module =  LoadLibrary("xinput1_32.dll");
		if(Original::module == NULL)
			return false;
		Original::xDllMain = (DllMainType)GetProcAddress(Original::module,"DllMain");
		Original::xXInputEnable = (XInputEnableType)GetProcAddress(Original::module,"XInputEnable");
		Original::xXInputGetAudioDeviceIds = (XInputGetAudioDeviceIdsType)GetProcAddress(Original::module,"XInputGetAudioDeviceIds");
		Original::xXInputGetBatteryInformation = (XInputGetBatteryInformationType)GetProcAddress(Original::module,"XInputGetBatteryInformation");
		Original::xXInputGetCapabilities = (XInputGetCapabilitiesType)GetProcAddress(Original::module,"XInputGetCapabilities");
		Original::xXInputGetDSoundAudioDeviceGuids = (XInputGetDSoundAudioDeviceGuidsType)GetProcAddress(Original::module,"XInputGetDSoundAudioDeviceGuids");
		Original::xXInputGetKeystroke = (XInputGetKeystrokeType)GetProcAddress(Original::module,"XInputGetKeystroke");
		Original::xXInputGetState = (XInputGetStateType)GetProcAddress(Original::module,"XInputGetState");
		Original::xXInputSetState = (XInputSetStateType)GetProcAddress(Original::module,"XInputSetState");
	}
	DllMain2(hinstDLL, fdwReason, lpvReserved);
	return true;
}

void mXInputEnable(BOOL enable)
{
	return (void)Original::xXInputEnable(enable);
}

DWORD mXInputGetAudioDeviceIds(DWORD dwUserIndex,LPWSTR pRenderDeviceId, UINT *pRenderCount,LPWSTR pCaptureDeviceId,UINT *pCaptureCount)
{
	return (DWORD)Original::xXInputGetAudioDeviceIds(dwUserIndex,pRenderDeviceId,pRenderCount,pCaptureDeviceId,pCaptureCount);
}

DWORD mXInputGetBatteryInformation(DWORD dwUserIndex,BYTE devType,XINPUT_BATTERY_INFORMATION *pBatteryInformation)
{
	return (DWORD)Original::xXInputGetBatteryInformation(dwUserIndex,devType,pBatteryInformation);
}

DWORD mXInputGetCapabilities(DWORD dwUserIndex,DWORD dwFlags,XINPUT_CAPABILITIES *pCapabilities)
{
	return (DWORD)Original::xXInputGetCapabilities(dwUserIndex,dwFlags,pCapabilities);
}

DWORD WINAPI mXInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex,GUID* pDSoundRenderGuid,GUID* pDSoundCaptureGuid)
{
	return (DWORD)Original::xXInputGetDSoundAudioDeviceGuids(dwUserIndex,pDSoundRenderGuid,pDSoundCaptureGuid);
}

DWORD WINAPI mXInputGetKeystroke(DWORD dwUserIndex,DWORD dwReserved,PXINPUT_KEYSTROKE pKeystroke)
{
	return (DWORD)Original::xXInputGetKeystroke(dwUserIndex,dwReserved,pKeystroke);
}

DWORD mXInputGetState(DWORD dwUserIndex,XINPUT_STATE *pState)
{
	return (DWORD)Original::xXInputGetState(dwUserIndex,pState);
}

DWORD mXInputSetState(DWORD dwUserIndex,XINPUT_VIBRATION *pVibration)
{
	return (DWORD)Original::xXInputSetState(dwUserIndex,pVibration);
}