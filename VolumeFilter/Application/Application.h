#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#ifndef _APPLICATION_H
#define _APPLICATION_H


// Device type
#define SIOCTL_TYPE 40000
 
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
#define IOCTL_SET\
 CTL_CODE( SIOCTL_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)

#define IOCTL_REMOVE\
	CTL_CODE(SIOCTL_TYPE, 0x801, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)


typedef struct SETVOLUMEINFO
{
	WCHAR wszDeviceName[MAX_PATH];

}SETVOLUMEINFO;


typedef struct REMOVEVOLUMEINFO
{
	WCHAR wszDeviceName[MAX_PATH];

}REMOVEVOLUMEINFO;

typedef enum COMMAND
{
	SET_VOLUME		= 1001,
	REMOVE_VOLUME

}COMMAND, *PCOMMAND;

typedef struct OperationStatus{

	INT eCmd;
	INT istatus;
}OperationStatus,*pOperationStatus;

typedef struct Operations{
	
	INT eCmd;
	union 
	{
		SETVOLUMEINFO  structSetVolume; 
		REMOVEVOLUMEINFO structRemoveVolume;

	}MyUnion;

}Operations;

INT SetVolume(TCHAR *tszInputString);
INT RemoveVolume(TCHAR *tszInputString);


#endif