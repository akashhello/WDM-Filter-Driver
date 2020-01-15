#include "Application.h"
 
 
int _tmain(INT argc, TCHAR* argv[])
{

	HANDLE hDevice;
	TCHAR  *tszInputString = L"Volume{1c8bafdb-1c7a-49cf-a3c8-ddd9d6e0e743}";
	DWORD dwBytesRead = 0;
	WCHAR wszDeviceName[MAX_PATH];
	Operations RegisterVolume;
	OperationStatus Status;
	INT iRet;
	
	iRet = QueryDosDevice(tszInputString,wszDeviceName,sizeof(wszDeviceName));
	if(!iRet)
	{
		printf("QueryDosDevice Failed(0x%lu)",GetLastError());
		return 0;
	}

	_tcscpy_s(RegisterVolume.MyUnion.structSetVolume.wszDeviceName, MAX_PATH, wszDeviceName);

	hDevice = CreateFile(L"\\\\.\\MyDevice",GENERIC_READ|GENERIC_WRITE,
							0,NULL,OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);

	if(INVALID_HANDLE_VALUE == hDevice)
	{
		printf("Create File failed(%lu)", GetLastError());
		return 1;	
	}
	
	RegisterVolume.eCmd =  SET_VOLUME;

	DeviceIoControl(hDevice, IOCTL_SET,
		&RegisterVolume,sizeof(Operations),
		&Status,sizeof(OperationStatus),&dwBytesRead,
		NULL);
	
	printf("Status(%d) for eCMD(%d)",Status.istatus,Status.eCmd);
	printf("DwBytesRead(%lu)",dwBytesRead);

	if(0 == _tcscmp(argv[1],_T("Set")))
	{
		iRet = SetVolume(wszDeviceName);
		if(iRet)
		{
			printf( "Error:SetVolume Failed(%d)", iRet );
			return 0;
		}
	}
	else if(0 == _tcscmp(argv[1],_T("Remove")))
	{
		iRet = RemoveVolume(wszDeviceName);
		if(iRet)
		{
			printf("Error:RemoveVolume Failed(%d)",iRet);
			return 0;
		}
	}
	return 0;
}


INT SetVolume(TCHAR *wszDeviceName)
{
	DWORD dwBytesRead = 0;
	Operations RegisterVolume;
	HANDLE hDevice;
	OperationStatus Status;

	_tcscpy_s(RegisterVolume.MyUnion.structSetVolume.wszDeviceName, MAX_PATH, wszDeviceName);

	hDevice = CreateFile(L"\\\\.\\MyDevice",GENERIC_READ|GENERIC_WRITE,
						0,NULL,OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if(INVALID_HANDLE_VALUE == hDevice)
	{
		printf("Create File failed(%lu)", GetLastError());
		return 1;	
	}
	 
	RegisterVolume.eCmd =  SET_VOLUME;

	DeviceIoControl(hDevice, IOCTL_SET,
		&RegisterVolume,sizeof(Operations),
		&Status,sizeof(OperationStatus),&dwBytesRead,
		NULL);
	
	printf("Status(%d) for eCMD(%d)",Status.istatus,Status.eCmd);
	printf("DwBytesRead(%lu)",dwBytesRead);
	 
	CloseHandle(hDevice);
	return 0;
}

INT RemoveVolume(TCHAR *wszDeviceName)
{
	DWORD dwBytesRead = 0;
	Operations RemoveVolume;
	HANDLE hDevice;
	OperationStatus Status;

	_tcscpy_s(RemoveVolume.MyUnion.structRemoveVolume.wszDeviceName, MAX_PATH, wszDeviceName);

	hDevice = CreateFile(L"\\\\.\\MyDevice",GENERIC_READ|GENERIC_WRITE,
						0,NULL,OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if(INVALID_HANDLE_VALUE == hDevice)
	{
		printf("Create File failed(%lu)", GetLastError());
		return 1;	
	}
	 
	RemoveVolume.eCmd =  REMOVE_VOLUME;

	DeviceIoControl(hDevice, IOCTL_REMOVE,
		&RemoveVolume,sizeof(Operations),
		&Status,sizeof(OperationStatus),&dwBytesRead,
		NULL);
	
	printf("Status(%d) for eCMD(%d)",Status.istatus,Status.eCmd);
	printf("DwBytesRead(%lu)",dwBytesRead);

	 
	CloseHandle(hDevice);
	return 0;
}