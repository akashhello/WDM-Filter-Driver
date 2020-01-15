#include "VolumeFilter.h"
 
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
 
{
 
    ULONG               ulIndex;
	NTSTATUS			ntStatus;
    PDRIVER_DISPATCH  * dispatch;
	UNICODE_STRING		uniDeviceNameBuffer;
	UNICODE_STRING		uniDeviceSymbolicLink;

    //
    // Remember registry path
    //
	DbgPrint("I am in Driver Entry\n");
    
	RtlInitUnicodeString(&uniDeviceNameBuffer, DeviceNameBuffer);

	RtlInitUnicodeString(&uniDeviceSymbolicLink, DeviceSymLinkBuffer);

	VolumeFilterRegistryPath.MaximumLength = RegistryPath->Length
                                            + sizeof(UNICODE_NULL);
    VolumeFilterRegistryPath.Buffer = (PWCHAR)ExAllocatePool(
                                    PagedPool,
                                    VolumeFilterRegistryPath.MaximumLength);
    if (VolumeFilterRegistryPath.Buffer != NULL)
    {
        RtlCopyUnicodeString(&VolumeFilterRegistryPath, RegistryPath);
    } else {
        VolumeFilterRegistryPath.Length = 0;
        VolumeFilterRegistryPath.MaximumLength = 0;
    }
	DbgPrint("VolumeFilterRegistryPath.Buffer(%S)",VolumeFilterRegistryPath.Buffer);
    //
    // Create dispatch points
    //
    for (ulIndex = 0, dispatch = DriverObject->MajorFunction;
         ulIndex <= IRP_MJ_MAXIMUM_FUNCTION;
         ulIndex++, dispatch++) {
 
        *dispatch = VolumeFilterSendToNextDriver;
    }
	DriverObject->MajorFunction[IRP_MJ_READ]  = VolumeFilterRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = VolumeFilterWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = VolumeFilterDeviceIoControl; 
	DriverObject->MajorFunction[IRP_MJ_CREATE]			= VolumeFilterCreate; 
	DriverObject->MajorFunction[IRP_MJ_CLOSE]			= VolumeFilterClose; 
	DriverObject->MajorFunction[IRP_MJ_CLEANUP]			= VolumeFilterCleanup; 
    //
    // Set up the device driver entry points.
    //
    DriverObject->DriverExtension->AddDevice            = VolumeFilterAddDevice;
    DriverObject->DriverUnload                          = VolumeFilterUnload;
	
	pAddDeviceHead = (PLIST_ENTRY)ExAllocatePoolWithTag(PagedPool, sizeof(LIST_ENTRY), 'gat');
	InitializeListHead(pAddDeviceHead);
 
	//Call IOCreateDevice for DeviceIoControl
	 ntStatus = IoCreateDevice(DriverObject,
                            0,
							&uniDeviceNameBuffer,
							FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &pCDODeviceObjects);
 
    if (!NT_SUCCESS(ntStatus)) {
		DbgPrint("DriverEntry: Cannot create filterDeviceObject(%ld)\n",ntStatus);
       return ntStatus;
    }
	
	ntStatus = IoCreateSymbolicLink(&uniDeviceSymbolicLink, &uniDeviceNameBuffer);
    if (!NT_SUCCESS(ntStatus)) {
		DbgPrint("DriverEntry: IoCreateSymbolicLink..failed(%ld)\n",ntStatus);
       return ntStatus;
    }

	return(STATUS_SUCCESS);
 
} // end DriverEntry()

NTSTATUS 
VolumeFilterDeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
	)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIoStack_Loc;
	pOperations	pBuff;
	pOperationStatus pCmdStatus;
	PDEVICE_EXTENSION pDeviceExtension;
	PDEVICE_OBJECT    pTraverse;
		
	//DbgPrint("VolumeFilterDeviceIoControl\n");
	if(DeviceObject == pCDODeviceObjects)
	{
		pBuff = (pOperations)Irp->AssociatedIrp.SystemBuffer;
		pIoStack_Loc = IoGetCurrentIrpStackLocation(Irp);
		switch(pIoStack_Loc->Parameters.DeviceIoControl.IoControlCode)
		{
			case IOCTL_SET:
				DbgPrint("UsrModeString(%S)\n",pBuff->MyUnion.structSetVolume.wszDeviceName);
				DbgPrint("UsrModeString Set(%S)\n",pBuff->MyUnion.structSetVolume.wszDeviceName);
				pTraverse = pCDODeviceObjects->DriverObject->DeviceObject;
				while(pTraverse)
				{
					if(pTraverse != pCDODeviceObjects)
					{
						pDeviceExtension = (PDEVICE_EXTENSION)pTraverse->DeviceExtension;
						
						if(0 == _wcsicmp(pBuff->MyUnion.structSetVolume.wszDeviceName, pDeviceExtension->pDeviceName))
							pDeviceExtension->bIsTrackingEnabled = TRUE;

						DbgPrint("DeviceName(%S) TrackingFlag(%d)\n",pDeviceExtension->pDeviceName, pDeviceExtension->bIsTrackingEnabled);
					}	
					pTraverse = pTraverse->NextDevice;
				}
				pCmdStatus = (pOperationStatus)Irp->AssociatedIrp.SystemBuffer;
				pCmdStatus->eCmd = pBuff->eCmd; 
				pCmdStatus ->istatus = 0;

				Irp->IoStatus.Status = STATUS_SUCCESS;
				Irp->IoStatus.Information = sizeof(OperationStatus);
				IoCompleteRequest(Irp,IO_NO_INCREMENT);
				break;
		
			case IOCTL_REMOVE:
				DbgPrint("UsrModeString Remove(%S)\n",pBuff->MyUnion.structSetVolume.wszDeviceName);
				pTraverse = pCDODeviceObjects->DriverObject->DeviceObject;
				while(pTraverse)
				{
					if(pTraverse != pCDODeviceObjects)
					{
						pDeviceExtension = (PDEVICE_EXTENSION)pTraverse->DeviceExtension;
						
						if(0 == _wcsicmp(pBuff->MyUnion.structSetVolume.wszDeviceName, pDeviceExtension->pDeviceName))
							pDeviceExtension->bIsTrackingEnabled = FALSE;

						DbgPrint("DeviceName(%S) TrackingFlag(%d)\n",pDeviceExtension->pDeviceName, pDeviceExtension->bIsTrackingEnabled);
					}	
					pTraverse = pTraverse->NextDevice;
				}
				pCmdStatus = (pOperationStatus)Irp->AssociatedIrp.SystemBuffer;
				pCmdStatus->eCmd = pBuff->eCmd; 
				pCmdStatus ->istatus = 0;

				Irp->IoStatus.Status = STATUS_SUCCESS;
				Irp->IoStatus.Information = sizeof(OperationStatus);
				IoCompleteRequest(Irp,IO_NO_INCREMENT);
				break;

			default:
				break;
		}
	}	
	else
	{
		ntStatus= VolumeFilterSendToNextDriver(DeviceObject,Irp);
		//if(!NT_SUCCESS(ntStatus))
			//	DbgPrint("VolumeFilterSendToNextDriver...Failed:(%lu)\n",ntStatus);
	}
	return ntStatus;
}
 
NTSTATUS
VolumeFilterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
{
    NTSTATUS                status;
    PDEVICE_OBJECT          filterDeviceObject;
    PDEVICE_EXTENSION       deviceExtension;
	pVolumeFilterStruct		pNode;
	 
    PAGED_CODE();
 
    //
    // Create a filter device object for this device (partition).
    //
 
    DbgPrint("AddDevice Entry\n");

	status = IoCreateDevice(DriverObject,
                            DEVICE_EXTENSION_SIZE,
							NULL,
							FILE_DEVICE_DISK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &filterDeviceObject);
 
    if (!NT_SUCCESS(status)) {
		DbgPrint("VolumeFilterAddDevice: Cannot create filterDeviceObject(%ld)\n",status);
       return status;
    }
    

    filterDeviceObject->Flags |= DO_DIRECT_IO;
 
    deviceExtension = (PDEVICE_EXTENSION) filterDeviceObject->DeviceExtension;
 
    RtlZeroMemory(deviceExtension, DEVICE_EXTENSION_SIZE);
      //
    // Allocate per processor counters
    //
    // NOTE: To save memory, we don't allocate memory beyond
    // QueryTime. Remember to expand this size if there is a
    // need to use anything beyond
    //
 
    //
    // Attaches the device object to the highest device object in the chain and
    // return the previously highest device object, which is passed to
    // IoCallDriver when pass IRPs down the device stack
    //
 
    deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
 
    deviceExtension->TargetDeviceObject =
        IoAttachDeviceToDeviceStack(filterDeviceObject, PhysicalDeviceObject);
 
    if (deviceExtension->TargetDeviceObject == NULL) {
        IoDeleteDevice(filterDeviceObject);
		DbgPrint("VolumeFilterAddDevice: Unable to attach 0x%p to target 0x%p\n",
            filterDeviceObject, PhysicalDeviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }
 
    deviceExtension->DeviceObject = filterDeviceObject;
 
    filterDeviceObject->Flags |=  DO_POWER_PAGABLE;
 
    //
    // Clear the DO_DEVICE_INITIALIZING flag
    //
 
    filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	
	pNode = (pVolumeFilterStruct)ExAllocatePoolWithTag(PagedPool, sizeof(VolumeFilterStruct), 'eon');
	
	pNode->iNum = iNoOfAddDevice++;
	InsertTailList(pAddDeviceHead,&(pNode->list_entry));
	
	status = GetDeviceName(PhysicalDeviceObject, filterDeviceObject);
	if(!NT_SUCCESS(status))
	{
		DbgPrint("GetDevice...Failed");
	
	}
	deviceExtension->bIsTrackingEnabled = FALSE;
	DbgPrint("Device INFO(%S)\n",deviceExtension->pDeviceName);
	
	DbgPrint("Add Device End\n");
    return STATUS_SUCCESS;
 
} // end VolumeFilterAddDevice()
 
NTSTATUS GetDeviceName(
	 IN PDEVICE_OBJECT pDeviceObject,
	 IN PDEVICE_OBJECT pfilterDeviceObject
	 )
 {
	ULONG		ulReturnedLength;
	ULONG		ulLength;
	NTSTATUS	ntStatus;	
	PUNICODE_STRING pDeviceName;
	PDEVICE_EXTENSION deviceExtension;
	
	deviceExtension = (PDEVICE_EXTENSION) pfilterDeviceObject->DeviceExtension;

	ObQueryNameString((PVOID)pDeviceObject,NULL,0,&ulReturnedLength);

	ulLength = ulReturnedLength;

	pDeviceName = (PUNICODE_STRING)ExAllocatePool(PagedPool,ulLength + sizeof(UNICODE_NULL));
	ntStatus = ObQueryNameString(
		(PVOID)pDeviceObject,
		(POBJECT_NAME_INFORMATION)pDeviceName,
		ulLength,
		&ulReturnedLength
		);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("ObQueryNameString..Failed");
		return ntStatus;
	}

	deviceExtension->pDeviceName = (PWCHAR)ExAllocatePool(PagedPool, pDeviceName->Length * sizeof(WCHAR));
	RtlCopyMemory(deviceExtension->pDeviceName, pDeviceName->Buffer, pDeviceName->Length * sizeof(WCHAR));

	DbgPrint("Device Info(%S)\n",pDeviceName->Buffer);
	DbgPrint("GetDevice Name End\n");
	return STATUS_SUCCESS;
 }

NTSTATUS
VolumeFilterSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
 
{
    PDEVICE_EXTENSION   deviceExtension;
 
    IoSkipCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
 
    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
 
} // end VolumeFilterSendToNextDriver()

VOID
VolumeFilterUnload(
    IN PDRIVER_OBJECT DriverObject
    )
 
{
    PAGED_CODE();
	DbgPrint("Driver unload\n");
    UNREFERENCED_PARAMETER(DriverObject);
 
    return;
}
NTSTATUS VolumeFilterRead(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
	)
{
	ULONG BufLen;	 //Buffer length for user provided buffer
	LONGLONG Offset; //Buffer Offset
	INT		iNum;
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION  p_DVCEXT;
	PDEVICE_EXTENSION  deviceExtension   = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
	PLIST_ENTRY        pTemp = NULL;
	
	if(deviceExtension->bIsTrackingEnabled)
	{

		//Get I/o Stack Location & Device Extension
		DbgPrint("IRP_MJ_READ : Begin\r\n");
	
		pTemp = pAddDeviceHead;
	
		while(pAddDeviceHead != pTemp->Flink){
			pTemp = pTemp->Flink;
			iNum = CONTAINING_RECORD(pTemp,VolumeFilterStruct,list_entry)->iNum;
			DbgPrint("NUM:(%d)\n", iNum);
		}


		p_IO_STK = IoGetCurrentIrpStackLocation(pIrp);
		p_DVCEXT = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

		//Get User Output Buffer & Length 
		BufLen = p_IO_STK->Parameters.Read.Length;
		Offset = p_IO_STK->Parameters.Read.ByteOffset.QuadPart;

		DbgPrint("Output Buffer Length : %lu\r\n", BufLen);
		DbgPrint("Buffer Offset		   : %lld\r\n", Offset);

		IoCopyCurrentIrpStackLocationToNext(pIrp);

		DbgPrint("IRP_MJ_READ : End\r\n");
	}
	else
		IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(deviceExtension->TargetDeviceObject,
                        pIrp);
}
NTSTATUS VolumeFilterWrite(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
	)
{
	ULONG BufLen;	//Buffer length for user provided buffer
	LONGLONG Offset;//Buffer Offset
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	PDEVICE_EXTENSION  deviceExtension   = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;			
	
	if(deviceExtension->bIsTrackingEnabled)
	{
		DbgPrint("IRP_MJ_WRITE : Begin\r\n");
		//Get I/o Stack Location & Device Extension
		p_IO_STK = IoGetCurrentIrpStackLocation(pIrp);
		p_DVCEXT = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

		//Get User Output Buffer & Length 
		BufLen = p_IO_STK->Parameters.Read.Length;
		Offset = p_IO_STK->Parameters.Read.ByteOffset.QuadPart;
		DbgPrint("Output Buffer Length : %lu\r\n", BufLen);
		DbgPrint("Buffer Offset        : %lld\r\n", Offset);
	
		IoCopyCurrentIrpStackLocationToNext(pIrp);
	
		DbgPrint("IRP_MJ_WRITE : End\r\n");
	}
	else
		IoSkipCurrentIrpStackLocation(pIrp);

	return IoCallDriver(deviceExtension->TargetDeviceObject,
                        pIrp);
}

NTSTATUS 
VolumeFilterCreate(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	
//	DbgPrint("Create is called\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS 
VolumeFilterCleanup(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	
//	DbgPrint("Cleanup is called\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS 
VolumeFilterClose(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP			  pIrp
)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	
//	DbgPrint("Close is called\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}