#define INITGUID
#include <wchar.h>
#include <FltKernel.h>
#include "ntddk.h"

 
#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'frPD')
#endif
 
#define VolumeFilter_MAXSTR         64
 
#define SIOCTL_TYPE 40000

#define IOCTL_SET\
 CTL_CODE( SIOCTL_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)

#define IOCTL_REMOVE\
	CTL_CODE(SIOCTL_TYPE, 0x801, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
//
// Device Extension
//
 
typedef struct _DEVICE_EXTENSION {
 
    //
    // Back pointer to device object
    //
    PDEVICE_OBJECT DeviceObject;
 
    //
    // Target Device Object
    //
 
    PDEVICE_OBJECT TargetDeviceObject;
 
    //
    // Physical device object
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

	PVOID DataBuffer;

	PWCHAR		pDeviceName;
	BOOLEAN		bIsTrackingEnabled;	

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct VolumeFilterStruct{

	INT iNum;
	LIST_ENTRY list_entry;
}VolumeFilterStruct,*pVolumeFilterStruct;
 
typedef struct SETVOLUMEINFO
{
	WCHAR wszDeviceName[260];

}SETVOLUMEINFO;

typedef struct OperationStatus{

	INT eCmd;
	INT istatus;
}OperationStatus,*pOperationStatus;

typedef enum COMMAND
{

	SET_VOLUME		= 1001,
	REMOVE_VOLUME

}COMMAND, *PCOMMAND;

typedef struct REMOVEVOLUMEINFO
{
	WCHAR wszDeviceName[260];

}REMOVEVOLUMEINFO;


typedef struct Operations{
	INT eCmd;
	union 
	{
		SETVOLUMEINFO  structSetVolume; 
		REMOVEVOLUMEINFO structRemoveVolume;

	}MyUnion;
}Operations, *pOperations;



#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)
/*
Layout of Per Processor Counters is a contiguous block of memory:
    Processor 1
+-----------------------+     +-----------------------+
|PROCESSOR_COUNTERS_SIZE| ... |PROCESSOR_COUNTERS_SIZE|
+-----------------------+     +-----------------------+
where PROCESSOR_COUNTERS_SIZE is less than sizeof(DISK_PERFORMANCE) since
we only put those we actually use for counting.
*/
 
UNICODE_STRING VolumeFilterRegistryPath;
UNICODE_STRING VolumeFilterDeviceName; 
WCHAR		   DeviceNameBuffer[] = L"\\Device\\MYDEVICE";
WCHAR		   DeviceSymLinkBuffer[] = L"\\DosDevices\\MyDevice";	
PLIST_ENTRY	   pAddDeviceHead;
PDEVICE_OBJECT pCDODeviceObjects;
INT			   iNoOfAddDevice;	
//
// Function declarations
//
 
DRIVER_INITIALIZE DriverEntry;
 
DRIVER_ADD_DEVICE VolumeFilterAddDevice;
 
DRIVER_DISPATCH VolumeFilterSendToNextDriver;
 
DRIVER_UNLOAD VolumeFilterUnload;

NTSTATUS VolumeFilterRead(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
	);
NTSTATUS VolumeFilterWrite(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
	);
NTSTATUS GetDeviceName(
	 IN PDEVICE_OBJECT pDeviceObject,
	 IN PDEVICE_OBJECT pfilterDeviceObject
	 ); 
NTSTATUS 
VolumeFilterCreate(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
);
NTSTATUS 
VolumeFilterDeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS 
VolumeFilterCleanup(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
);

NTSTATUS 
VolumeFilterClose(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
);

//
// Define the sections that allow for discarding (i.e. paging) some of
// the code.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, VolumeFilterAddDevice)
#pragma alloc_text (PAGE, VolumeFilterUnload)
#pragma alloc_text (PAGE, VolumeFilterRead)
#pragma alloc_text (PAGE, VolumeFilterWrite)
#endif