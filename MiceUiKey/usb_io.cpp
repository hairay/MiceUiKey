#include "stdafx.h"
#include <ole2.h>     // Required for CLSIDFromString()
#include "winioctl.h"
#include "standard.h"
#include "usb_io.h"
#include "usbscan.h"
#include "OSUtil.h"
 #include "usbprint.h"
#include <setupapi.h>

// OAK OTI-4110 USB VID/PID

//unsigned short	USB_VID = 0x03f0;
//unsigned short	USB_PID = 0x0b01;
#define CLSID_STR_WEIUSB (L"{6bdd1fc6-810f-11d0-bec7-08002be2092f}")

unsigned short	USB_VID = 0x0638;
unsigned short	USB_PID = 0x0a41;
//unsigned short	USB_VID = 0x043d;
//unsigned short	USB_PID = 0x001d;
unsigned short	USB_VID2 = 0x132B;
unsigned short	USB_VID_VEGAS = 0x043D;
unsigned short	USB_VID_ZORAN = 0x0999;

#define IOCTL_GET_DEVICE_DESCRIPTOR     CTL_CODE(FILE_DEVICE_USB_SCAN,IOCTL_INDEX+6, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FILE_DEVICE_USB_SCAN    0x8000
#define IOCTL_INDEX             0x0800

/*
typedef struct _DEVICE_DESCRIPTOR {
	unsigned short usVendorId;
	unsigned short usProductId;
	unsigned short usBcdDevice;
	unsigned short usLanguageId;
} DEVICE_DESCRIPTOR;
*/

HANDLE	ghMutex=NULL;
HANDLE	gEventHdl = NULL;

void close_usb(HANDLE f_usb, HANDLE f_dbg_in, HANDLE f_dbg_out)
{
	if( f_usb )
		CloseHandle(f_usb);
	if( f_dbg_in )
		CloseHandle(f_dbg_in);
	if( f_dbg_out )
		CloseHandle(f_dbg_out);

	if(gEventHdl)
	{
		CloseHandle(gEventHdl);
		gEventHdl = NULL;
	}
    CloseHandle(ghMutex);
    ghMutex = NULL;
}

BOOL SeizeSYSControl()
{
	DWORD				waitSignal=0;
	
	if(ghMutex == NULL)
	{
		return TRUE;
	}

	waitSignal = WaitForSingleObject(ghMutex, 60000L);

    switch (waitSignal) 
    {
        // The thread got mutex ownership.
        case WAIT_OBJECT_0:            
            return TRUE; 

        // Cannot get mutex ownership due to time-out.
        case WAIT_TIMEOUT:    		
            return FALSE; 

        // Got ownership of the abandoned mutex object.
        case WAIT_ABANDONED:            
            return FALSE; 

        default:
            
            return FALSE;
    }

	return TRUE;
}

BOOL ReleaseSYSControl()
{
    if(NULL != ghMutex)
    {
        if(ReleaseMutex(ghMutex))
        {            
            return TRUE;
        }
        else
        {            
            return FALSE;
        }    	
    }
    else
    {       
        return TRUE;
    }
}

void  open_usb( HANDLE *s_hdl, HANDLE *hdl_dbg_in, HANDLE *hdl_dbg_out, WORD vid , WORD pid )
{
	GUID     DevClass;    // CLSID holder for the WEIUSB device class
	HDEVINFO hDevInfoSet; // Handle to device information set
	DWORD    dwIndex;     // Index of current device info set record
	DWORD    dwRequired;  // Required buffer length
	DWORD    dwError;     // Error value from GetLastError()
	BOOL     bResult;     // Boolean return result value		
	HANDLE				f_usb =(HANDLE)0 ;
	HANDLE				f_usb_dbg_in =(HANDLE)0 ;
	HANDLE				f_usb_dbg_out =(HANDLE)0 ;		
	OSVERSIONINFO	        osVer;
	SECURITY_ATTRIBUTES		mySecurity;
	SECURITY_DESCRIPTOR		mySecurityDescriptor;
	BOOL ret;

	SP_DEVICE_INTERFACE_DATA         DevNode;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DevDetails;

	// Convert the registry-formatted CLSID for the WEIUSB device
	// class to a GUID structure.
	CLSIDFromString(CLSID_STR_WEIUSB, &DevClass);

	// Generate the device information set.  This is the actual
	// enumeration process.  We are specifying the class GUID of
	// the device type we want to enumerate, in this case only
	// WEIUSB devices.  The second argument would allow us to
	// constrain the enumeration to those devices attached to a
	// specified enumerator, or bus.  We could, for example,
	// enumerate only those devices attached via USB.
	hDevInfoSet = SetupDiGetClassDevs(
		&DevClass, // Only get WEIUSB devices
		NULL,      // Not specific to any bus
		NULL,      // Not associated with any window
		DIGCF_DEVICEINTERFACE + DIGCF_PRESENT);

	// Make sure enumeration completed without errors.
	if (hDevInfoSet == INVALID_HANDLE_VALUE)
	{
		printf("Unable to create device information set (Error 0x%08X)\n",GetLastError());
		return;
	}
	printf("Successfully created device information set.\n");
	// Iterate through the device info set.
	for (dwIndex = 0; ; dwIndex++)
	{
		// Retrieve the data from the next node index within the
		// device information set.
		DevNode.cbSize = sizeof(DevNode);
		bResult = SetupDiEnumDeviceInterfaces(
			hDevInfoSet,   // Handle to device info set
			NULL,          // Do not apply advanced filtering
			&DevClass,     // Class of device to retrieve
			dwIndex,       // List index of requested record
			&DevNode);     // Pointer to structure to receive data

		// If the previous call failed, do not continue trying
		// to enumerate devices.
		if (!bResult)
		{
			dwError = GetLastError();
			if (dwError != ERROR_NO_MORE_ITEMS)
			{
				printf("Error enumerating devices (0x%08X).\n",dwError);
			}
			break;
		}

		// The device information data represents some device
		// flags and the class GUID associated with the device
		// instance.  The device details must be retrieved to
		// get the device path that can be used to open the
		// device.  This is a two-step process.  First the API
		// must be called with the pointer to a buffer set to
		// NULL, and the size of the buffer specified as zero.
		// This will cause the API to fail, but the API will
		// provide a value in the dwRequired argument indicating
		// how much memory is needed to store the file path.
		// The memory can then be allocated and the API called a
		// second time, specifying the pointer to the buffer and
		// the buffer size, to receive the actual device path.
		SetupDiGetDeviceInterfaceDetail(
			hDevInfoSet,  // Handle to device information set
			&DevNode,     // Specify a pointer to the current node
			NULL,         // Pointer to structure to receive path data
			0,            // Currently no space is allocated
			&dwRequired,  // Pointer to var to receive required buffer size
			NULL);        // Pointer to var to receive additional device info

		// Allocate memory required.
		DevDetails = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(dwRequired);
		if (DevDetails == NULL)
		{
			printf("Unable to allocate memory for buffer.  Stopping.\n");
			break;
		}

		// Initialize the structure before using it.
		memset(DevDetails, 0, dwRequired);
		DevDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		// Call the API a second time to retrieve the actual
		// device path string.
		bResult = SetupDiGetDeviceInterfaceDetail(
			hDevInfoSet,  // Handle to device information set
			&DevNode,     // Pointer to current node in devinfo set
			DevDetails,   // Pointer to buffer to receive device path
			dwRequired,   // Length of user-allocated buffer
			&dwRequired,  // Pointer to arg to receive required buffer length
			NULL);        // Not interested in additional data

		if (!bResult)
		{
			// Some error occurred retrieve the device path.
			printf("ERROR: Unable to retrieve device path (0x%08X).\n",GetLastError());
		}
		else
		{			
			char f_name_in[1024];
			char f_name_out[1024];
			char f_name_event[1024];
			DEVICE_DESCRIPTOR	f_info;				
			DWORD               write_bytes;
			USBSCAN_PIPE_CONFIGURATION	pipeData;		

			// Successfully retrieved the device path.  Go ahead
			// and print it to the console.
			printf("   %s\n", DevDetails->DevicePath);						
			f_usb = CreateFile(DevDetails->DevicePath,
							  GENERIC_WRITE | GENERIC_READ,
							  FILE_SHARE_WRITE | FILE_SHARE_READ,
							  NULL,
							  OPEN_EXISTING,
							  0,
							  NULL);
			bResult = DeviceIoControl(
								f_usb,
								(DWORD)IOCTL_GET_DEVICE_DESCRIPTOR,
								NULL, 
								0, 
								&f_info,
								sizeof(DEVICE_DESCRIPTOR), 
								&write_bytes,
								NULL);

			

			printf("vid=%x pid=%x\n",f_info.usVendorId, f_info.usProductId);		
			if(bResult == TRUE && (f_info.usVendorId == vid || vid == 0) && (f_info.usProductId == pid || pid ==0))
			{
				if(GetPipeConfiguration(f_usb, (unsigned char *)&pipeData, sizeof(USBSCAN_PIPE_CONFIGURATION)) != TRUE)
				{
					pipeData.NumberOfPipes = 2;
				}
				strcpy( f_name_in, DevDetails->DevicePath );
				strcpy( f_name_out, DevDetails->DevicePath );	
				strcpy( f_name_event, DevDetails->DevicePath );
				strcat( f_name_in, "\\0" );
				strcat( f_name_out, "\\1" );
				strcat( f_name_event, "\\4" );
				if(pipeData.NumberOfPipes >= 4)
				{
					f_usb_dbg_in = CreateFile(f_name_in,
											  GENERIC_WRITE | GENERIC_READ,
											  FILE_SHARE_WRITE | FILE_SHARE_READ,
											  NULL,
											  OPEN_EXISTING,
											  0,
											  NULL);

					f_usb_dbg_out = CreateFile(f_name_out,
											   GENERIC_WRITE | GENERIC_READ,
											   FILE_SHARE_WRITE | FILE_SHARE_READ,
											   NULL,
											   OPEN_EXISTING,
											   0,
											   NULL);	
				}
				if(pipeData.NumberOfPipes == 5)
				{
					gEventHdl = CreateFile(f_name_event,
										   GENERIC_WRITE | GENERIC_READ,
										   FILE_SHARE_WRITE | FILE_SHARE_READ,
										   NULL,
										   OPEN_EXISTING,
										   0,
										   NULL);
				}
				free(DevDetails);
				if(f_usb_dbg_in == INVALID_HANDLE_VALUE || f_usb_dbg_out == INVALID_HANDLE_VALUE )
				{
					if( f_usb_dbg_in != INVALID_HANDLE_VALUE )
						CloseHandle(f_usb_dbg_in);
					if( f_usb_dbg_out != INVALID_HANDLE_VALUE )
						CloseHandle(f_usb_dbg_out);
					f_usb_dbg_in = 0;
					f_usb_dbg_out = 0;
				}
				break;
			}
			else
			{
				CloseHandle(f_usb);
				f_usb = (HANDLE)0 ;
			}
		}

		// Deallocate memory used
		free(DevDetails);

	} // for (dwIndex = 0;; dwIndex++)

	// Print output notification for user.
	printf("Enumeration listing complete.  %d device(s) detected.\n", dwIndex);

	// Deallocate resources used by Windows for enumeration.
	SetupDiDestroyDeviceInfoList(hDevInfoSet);
	
    osVer.dwOSVersionInfoSize = sizeof(osVer);
	if( GetVersionEx(&osVer) )
	{
		printf("osVer.dwPlatformId = %ld\n",osVer.dwPlatformId);
		printf("osVer.dwMinorVersion  = %ld\n",osVer.dwMinorVersion);
		printf("osVer.dwMajorVersion  = %ld\n",osVer.dwMajorVersion);
		printf("osVer.dwBuildNumber  = %ld\n",osVer.dwBuildNumber);		
	}
	
	mySecurity.nLength = sizeof(SECURITY_ATTRIBUTES);
	mySecurity.lpSecurityDescriptor = &mySecurityDescriptor;
	mySecurity.bInheritHandle = TRUE;
	ret = InitializeSecurityDescriptor(&mySecurityDescriptor,SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&mySecurityDescriptor, TRUE, NULL, FALSE);
    if(ghMutex == NULL)	
        ghMutex = CreateMutex(&mySecurity, FALSE, "AM3XTEST");

	*s_hdl = f_usb;
	*hdl_dbg_in = f_usb_dbg_in;
	*hdl_dbg_out = f_usb_dbg_out;
	return ;
}


//
// -1 : ReadFile failed.
//	0 : OK
//  1 : Busy
//  2 : Request sense.
//

int get_status(HANDLE f_usb)
{
	unsigned char status ;
	int ret ;
	unsigned long read_bytes ,i=0;

    if(!SeizeSYSControl())
        return 0;
    
	ret = ReadFile(f_usb,&status,1,&read_bytes,NULL);
    ReleaseSYSControl();
    
	if(ret == 0) {
		return -1 ;
	}
	return (int)status ;
}

void SetCommand(AVMFP_Command *pCmd,WORD wOPCode,WORD wClass,UINT64 qwDataSize)
{
  srand((unsigned)time( NULL ));

  ZeroMemory(pCmd,sizeof(AVMFP_Command));
  pCmd->id = 0x43425355;
  strncpy((char *)pCmd->IDCode,"#AJAX123",sizeof(pCmd->IDCode));
  pCmd->DataSize=qwDataSize;
  pCmd->CheckCode=(DWORD)rand();
  pCmd->OPCode=wOPCode;
  pCmd->Class=wClass;
  
}

int test_unit_ready(HANDLE f_usb,unsigned int size)
{
	int				ret = TRUE;
	unsigned long	write_bytes ,read_bytes;
	AVMFP_Command	cmd;
	//AVMFP_Status	status;
	BYTE			buf[6] = "HOST";

	SetCommand(&cmd,0x0000,0,0);
    ret = SeizeSYSControl();
    if(!ret)
        return 0;
	
	ret = ReadFile(f_usb,&buf,3,&read_bytes,NULL);
	buf[read_bytes] = 0;
	
	if( !WriteFile(f_usb , &cmd ,3 ,&write_bytes ,NULL) )
	{
		ReleaseSYSControl();
		return 0 ;
	}   

	if( !WriteFile(f_usb , &cmd ,8 ,&write_bytes ,NULL) )
	{
		ReleaseSYSControl();
		return 0 ;
	}   

	if( !WriteFile(f_usb , &cmd ,16 ,&write_bytes ,NULL) )
	{
		ReleaseSYSControl();
		return 0 ;
	}   

/*
	if( !WriteFile(f_usb , &cmd ,sizeof(AVMFP_Command) ,&write_bytes ,NULL) )
	{
		ReleaseSYSControl();
		return 0 ;
	}   
	*/
	if(write_bytes != sizeof(AVMFP_Command) ) return 0;
	//ret = get_status(f_usb);
	//ret = ReadFile(f_usb,&status,sizeof(AVMFP_Status),&read_bytes,NULL);
	if(ret == TRUE)
		ret = SCSI_RET_OK;
	else
		ret = -1;

    ReleaseSYSControl();
	return (ret==SCSI_RET_OK) ? 1 : 0 ;
}


/*
int test_unit_ready(HANDLE f_usb,unsigned int size)
{
	int ret ;
	unsigned long write_bytes ;
	BYTE cmd[10] = {SCSI_CMD_TESTUNITREADY,0x00, 0x00 ,0x0,0x0,0x0,0x0, 0x80,0x00,0x00 } ;

    ret = SeizeSYSControl();
    if(!ret)
        return 0;
	if( !WriteFile(f_usb ,cmd ,sizeof(cmd) ,&write_bytes ,NULL) )
   {
        ReleaseSYSControl();
		return 0 ;
   }   
	if(write_bytes != size ) return 0;
	ret = get_status(f_usb);
    ReleaseSYSControl();
	return (ret==SCSI_RET_OK) ? 1 : 0 ;
}
*/
int media_check(HANDLE f_usb, unsigned char  *buf)
{
    unsigned long write_bytes,read_bytes ;
	unsigned char media_cmd[]= { SCSI_CMD_MEDIACHECK,0x00,0x0,0x00,0x01,0x00,0x00,0x00,0x00,0x00 } ;    
	int sts ;

    if(!SeizeSYSControl())
        return 0;
    
	if( !WriteFile(f_usb ,media_cmd ,sizeof(media_cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }

	if(write_bytes != sizeof(media_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	if(!ReadFile(f_usb, buf , 1 ,&read_bytes,NULL) )
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(read_bytes != 1 )
    {
        ReleaseSYSControl();
        return 0;
    }
	sts = get_status(f_usb);
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;
}

int set_cmd_data(HANDLE f_usb,BYTE *cmd, unsigned int cmd_size, BYTE *pData, unsigned int dataLen)
{
    int ret ;
	unsigned long write_bytes ;	

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb ,cmd ,cmd_size ,&write_bytes ,NULL) )
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != cmd_size ) 
    {
        ReleaseSYSControl();
        return 0;
    }
    if( !WriteFile(f_usb ,pData ,dataLen ,&write_bytes ,NULL) )
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != dataLen ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	ret = get_status(f_usb);
    ReleaseSYSControl();
	return (ret==SCSI_RET_OK) ? 1 : 0 ;
}

int scan_data(HANDLE f_usb)
{
	int ret ;
	unsigned long write_bytes ;
	BYTE cmd[10] = {SCSI_CMD_SCAN,0x00, 0x00 ,0x0,0x0,0x0,0x0, 0x80,0x00,0x00 } ;

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb ,cmd ,sizeof(cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	ret = get_status(f_usb);
    ReleaseSYSControl();
	return (ret==SCSI_RET_OK) ? 1 : 0 ;
}

int reserve_unit(HANDLE f_usb, BYTE type)
{
	int ret ;
	unsigned long write_bytes ;
	BYTE cmd[10] = {SCSI_CMD_RESERVEUNIT,0x00, 0x00 ,0x0,0x0,type,0x0, 0x80,0x00,0x00 } ;

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb ,cmd ,sizeof(cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	ret = get_status(f_usb);
    ReleaseSYSControl();
	return (ret==SCSI_RET_OK) ? 1 : 0 ;
}

int release_unit(HANDLE f_usb, BYTE type)
{
	int ret ;
	unsigned long write_bytes ;
	BYTE cmd[10] = {SCSI_CMD_RELEASEUNIT,0x00, 0x00 ,0x0,0x0,type,0x0, 0x80,0x00,0x00 } ;

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb ,cmd ,sizeof(cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	ret = get_status(f_usb);
    ReleaseSYSControl();
	return (ret==SCSI_RET_OK) ? 1 : 0 ;
}

int read_scan_image(HANDLE f_usb,char *buf,unsigned int size) 
{
	unsigned long write_bytes,read_bytes ;
	unsigned char read_cmd[]= { SCSI_CMD_READ,0x00,SCSI_RDTC_IMAGE,0x00,0x0a,0x0d,0x00,0x00,0x00,0x00 } ;
	int sts ;
	unsigned char *ptr = read_cmd + 6 ;
	int read_img_size = size;
	int each_size ;
	
	
	set_quad(ptr,read_img_size );

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb ,read_cmd ,sizeof(read_cmd) ,&write_bytes ,NULL) ) 
    {		
        ReleaseSYSControl();
		return 0 ;
	}
	if(write_bytes != sizeof(read_cmd) ) 
    {
        ReleaseSYSControl();
		return 0;
	}
	
	do {
		each_size = read_img_size ;
		if(!ReadFile(f_usb, buf , each_size ,&read_bytes,NULL) ) 
        {
            ReleaseSYSControl();
			return 0 ;
		}		
        buf += read_bytes;
		read_img_size -= read_bytes ;

	} while ( read_img_size > 0 ) ;

	sts = get_status(f_usb);	
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;
}

int read_block_size(HANDLE f_usb,DWORD *blockSize,unsigned int size) 
{
	unsigned long write_bytes,read_bytes ;
	unsigned char read_cmd[] = { SCSI_CMD_READ,0x00,SCSI_RDTC_BLOCK_SIZE,0x00,0x00,0x0,0x00,0x00,0x00,0x00 } ;
	int sts ;
	unsigned char *ptr = read_cmd + 6 ;
	set_triple(ptr,size);

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb ,read_cmd ,sizeof(read_cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(read_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	if(!ReadFile(f_usb, (BYTE *)blockSize , size ,&read_bytes,NULL) )
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(read_bytes != size ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	sts = get_status(f_usb);
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;

}

int inq_data(HANDLE f_usb,char *buf) 
{
	unsigned long write_bytes,read_bytes ;
	unsigned char inq_cmd[]= { SCSI_CMD_INQUIRY,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x00 } ;
	unsigned char inq_data[96] ;
	int sts ;

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb ,inq_cmd ,10 ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != 10 ) 
    {
        ReleaseSYSControl();
        return 0;
    }

	if(!ReadFile(f_usb, inq_data, 96 ,&read_bytes,NULL) )
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(read_bytes != 96 ) return 0;
	memcpy(buf,inq_data,96);

	sts = get_status(f_usb);
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;
}

int read_cmd_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char cmd, unsigned char type,BOOL bPrinter) 
{
	unsigned long write_bytes,read_bytes ;
	unsigned char read_cmd[] = { SCSI_CMD_READ,0x00,cmd,type,0x00,type,0x00,0x00,0x00,0x00 } ;
	int sts , start;
	unsigned char *ptr = read_cmd + 6 ;
	set_triple(ptr,size);

	if(bPrinter	== TRUE)
		read_cmd[0]	 = SCSI_CMD_PRINTER_READ;

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb_out ,read_cmd ,sizeof(read_cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(read_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }

	sts = get_status(f_usb_in);
	if(sts != SCSI_RET_OK)
	{
		ReleaseSYSControl();
        return 0;
	}

    start = 0;
    while(size >0)
    {
	    if(!ReadFile(f_usb_in, buf+start , size ,&read_bytes,NULL) )
        {
            ReleaseSYSControl();
		    return 0 ;
        }
	    if(read_bytes != size )
        {
            //ReleaseSYSControl();
            //return 0;
            start += read_bytes;
            size -= read_bytes;
        }
        else
        {
            break;
        }
    }
	sts = get_status(f_usb_in);
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;
}


int read_mem_data(HANDLE f_usb_in,HANDLE f_usb_out,unsigned int addr, char *buf,unsigned int size) 
{
	unsigned long write_bytes,read_bytes ;
	unsigned char read_cmd[] = { SCSI_CMD_RW_MEM,0x00,0,0,0x00,0,0x00,0x00,0x00,0x00 } ;
	int sts , start;
	unsigned char *ptr = read_cmd + 6 ;
	set_quad(ptr,size);
	ptr = read_cmd + 2 ;
	set_quad(ptr,addr);

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb_out ,read_cmd ,sizeof(read_cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(read_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }

	sts = get_status(f_usb_in);
	if(sts != SCSI_RET_OK)
	{
		ReleaseSYSControl();
        return 0;
	}

    start = 0;
    while(size >0)
    {
	    if(!ReadFile(f_usb_in, buf+start , size ,&read_bytes,NULL) )
        {
            ReleaseSYSControl();
		    return 0 ;
        }
	    if(read_bytes != size )
        {
            //ReleaseSYSControl();
            //return 0;
            start += read_bytes;
            size -= read_bytes;
        }
        else
        {
            break;
        }
    }
	sts = get_status(f_usb_in);
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;
}

int send_cmd_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char SendCmd, unsigned char type,BOOL bPrinter) 
{
	int sts;
	unsigned long write_bytes ;
	unsigned char  cmd[10] = {SCSI_CMD_SEND,0x00,SendCmd,type,0x00,type,0x00,0x00,0x00,0x00 } ;	
	unsigned char  *ptr = cmd + 6 ;
    
	set_triple(ptr,size);
	if(bPrinter	== TRUE)
		cmd[0]	 = SCSI_CMD_PRINTER_SEND;

	if(!SeizeSYSControl())
        return 0;

	if( !WriteFile(f_usb_out ,cmd ,sizeof(cmd),&write_bytes ,NULL) ) 
	{
		ReleaseSYSControl();	
		return 0 ;
	}
	if(write_bytes != sizeof(cmd) ) 
	{
		ReleaseSYSControl();
		return 0;
	}
    
	sts = get_status(f_usb_in);
	if(sts != SCSI_RET_OK)
	{
		ReleaseSYSControl();
        return 0;
	}
	if(size)
	{
		if( !WriteFile(f_usb_out ,buf ,size,&write_bytes ,NULL) ) 
		{
			ReleaseSYSControl();
			return 0 ;
		}
		if(write_bytes != size ) 
		{
			ReleaseSYSControl();
			return 0;
		}
	}
	sts= get_status(f_usb_in) ;
	ReleaseSYSControl();

	return (sts==SCSI_RET_OK)?1:0 ;
}

int read_dbg_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char type, BOOL bPrinter) 
{
	unsigned char temp[65536];	
	unsigned long write_bytes,read_bytes ;
	unsigned char read_cmd[] = { SCSI_CMD_READ,0x00,SCSI_RDTC_FLASH_DATA,type,0x00,type,0x00,0x00,0x00,0x00 } ;
	int sts , start, errCode;
	unsigned char *ptr = read_cmd + 6 ;
	set_triple(ptr,size);

	if(bPrinter	== TRUE)
		read_cmd[0]	 = SCSI_CMD_PRINTER_READ;

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb_out ,read_cmd ,sizeof(read_cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		errCode = GetLastError();
		return 0 ;
    }
	if(write_bytes != sizeof(read_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
    //Sleep(2);
    start = 0;
    while(size >0)
    {
	    if(!ReadFile(f_usb_in, buf+start , size ,&read_bytes,NULL) )
        {
            ReleaseSYSControl();
			errCode = GetLastError();
		    return 0 ;
        }
	    if(read_bytes != size )
        {
            //ReleaseSYSControl();
            //return 0;
            start += read_bytes;
            size -= read_bytes;
        }
        else
        {
            break;
        }
    }
#if 0
	sts = get_status(f_usb_in);
#else
	temp[0] = (unsigned char)-1;
	ReadFile(f_usb_in,temp,sizeof(temp),&read_bytes,NULL);
	sts = temp[0];
#endif
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;

}

int read_large_dbg_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char type, BOOL bPrinter) 
{
	int start, leftSize, oneTimeSize;
	
	start = 0;
	leftSize = size;
	oneTimeSize = 65536;

	while(leftSize >0)
    {
		if(oneTimeSize > leftSize)
			oneTimeSize = leftSize;
		
	    if(!read_dbg_data(f_usb_in, f_usb_out, buf+start , oneTimeSize ,type,bPrinter) )
        {            
		    return 0 ;
        }	    
        start += oneTimeSize;
        leftSize -= oneTimeSize;        
    }
	return 1;
}


int send_dbg_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char type, BOOL bPrinter) 
{
	int sts;
	unsigned long write_bytes ;
	unsigned char  cmd[10] = {SCSI_CMD_SEND,0x00,SCSI_SDTC_FLASH ,0x00,0x00,type,0x00,0x00,0x00,0x00};
	unsigned char  *ptr = cmd + 6 ;
    set_triple(ptr,size);

	if(bPrinter	== TRUE)
		cmd[0]	 = SCSI_CMD_PRINTER_SEND;

	if(!SeizeSYSControl())
        return 0;

	if( !WriteFile(f_usb_out ,cmd ,sizeof(cmd),&write_bytes ,NULL) ) 
	{
		ReleaseSYSControl();	
		return 0 ;
	}
	if(write_bytes != sizeof(cmd) ) 
	{
		ReleaseSYSControl();
		return 0;
	}
    
	if( !WriteFile(f_usb_out ,buf ,size,&write_bytes ,NULL) ) 
	{
		ReleaseSYSControl();
		return 0 ;
	}
	if(write_bytes != size) 
	{
		ReleaseSYSControl();
		return 0;
	}
	sts= get_status(f_usb_in) ;
	ReleaseSYSControl();

	return (sts==SCSI_RET_OK)?1:0 ;
}


int read_fw_data(HANDLE f_usb,char  *buf,unsigned int size) 
{
	unsigned long write_bytes,read_bytes ;
	unsigned char read_cmd[] = { SCSI_CMD_READ,0x00,SCSI_RDTC_FLASH_DATA,FRTYPE_FIRMWARE,0x00,FRTYPE_FIRMWARE,0x00,0x00,0x00,0x00 } ;
	int sts ;
	unsigned char *ptr = read_cmd + 6 ;
	set_triple(ptr,size);

    if(!SeizeSYSControl())
        return 0;
	if( !WriteFile(f_usb ,read_cmd ,sizeof(read_cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(read_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	if(!ReadFile(f_usb, buf , size ,&read_bytes,NULL) )
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(read_bytes != size ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	sts = get_status(f_usb);
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;

}

int set_nvram_ptr(HANDLE f_usb_in,HANDLE f_usb_out,unsigned short pos, BOOL bPrinter)
{
	int sts;
	unsigned long write_bytes ;
	unsigned char  cmd[10] = {SCSI_CMD_SEND,0x00,SCSI_SDTC_NVRAM_POINTER ,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char  *ptr = cmd + 6 ;
    unsigned char  buf[2];
	set_triple(ptr,2);

	if(bPrinter	== TRUE)
		cmd[0]	 = SCSI_CMD_PRINTER_SEND;
	
	if(!SeizeSYSControl())
        return 0;

	if( !WriteFile(f_usb_out ,cmd ,sizeof(cmd),&write_bytes ,NULL) ) 
	{
		ReleaseSYSControl();	
		return 0 ;
	}
	if(write_bytes != sizeof(cmd) ) 
	{
		ReleaseSYSControl();
		return 0;
	}
    buf[0] = (char)(pos  >> 8);
    buf[1] = (char)(pos);
	if( !WriteFile(f_usb_out ,buf ,2,&write_bytes ,NULL) ) 
	{
		ReleaseSYSControl();
		return 0 ;
	}
	if(write_bytes != 2 ) 
	{
		ReleaseSYSControl();
		return 0;
	}
	sts= get_status(f_usb_in) ;
	ReleaseSYSControl();

	return (sts==SCSI_RET_OK)?1:0 ;
}

int ReadNVRAM(HANDLE f_usb_in,HANDLE f_usb_out, unsigned short pos ,unsigned char  *buf, unsigned int size, BOOL bPrinter)
{
    unsigned long write_bytes,read_bytes ;
	unsigned char read_cmd[]= { SCSI_CMD_READ,0x00,SCSI_RDTC_NVRAM_DATA,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;
    
	int sts ;
	unsigned char *ptr = read_cmd + 6 ;
	set_triple(ptr,size);

	if(bPrinter	== TRUE)
		read_cmd[0]	 = SCSI_CMD_PRINTER_READ;
	
    if(!SeizeSYSControl())
        return 0;
    if(!set_nvram_ptr(f_usb_in,f_usb_out,pos,bPrinter))
    {
        ReleaseSYSControl();
        return 0;
    }
	if( !WriteFile(f_usb_out ,read_cmd ,sizeof(read_cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }

	if(write_bytes != sizeof(read_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	if(!ReadFile(f_usb_in, buf , size ,&read_bytes,NULL) )
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(read_bytes != size )
    {
        ReleaseSYSControl();
        return 0;
    }
	sts = get_status(f_usb_in);
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;
}

int WriteNVRAM(HANDLE f_usb_in,HANDLE f_usb_out, unsigned short pos, unsigned char  *buf, unsigned int size, BOOL bPrinter)
{
    int sts;
	unsigned long write_bytes ;
	unsigned char  cmd[10] = {SCSI_CMD_SEND,0x00,SCSI_SDTC_NVRAM ,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char  *ptr = cmd + 6 ;
	set_triple(ptr,size);

	if(bPrinter	== TRUE)
		cmd[0]	 = SCSI_CMD_PRINTER_SEND;
	
    if(!SeizeSYSControl())
        return 0;
    if(!set_nvram_ptr(f_usb_in,f_usb_out,pos,bPrinter))
    {
        ReleaseSYSControl();
        return 0;
    }

	if( !WriteFile(f_usb_out ,cmd ,sizeof(cmd),&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	if( !WriteFile(f_usb_out ,buf ,size,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != size ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	sts= get_status(f_usb_in) ;
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;
}

int ReadTimingRegister(HANDLE f_usb, unsigned short pos ,unsigned char  *buf, unsigned int size)
{
    unsigned long write_bytes,read_bytes ;
	unsigned char read_cmd[]= { SCSI_CMD_READ,0x00,SCSI_RDTC_ASIC_REG,0x00,0x00,0x00,0x00,0x00,0x00,0x00 } ;
    
	int sts ;
	unsigned char *ptr = read_cmd + 6 ;
	set_triple(ptr,size);

	read_cmd[4]=pos>>8;
	read_cmd[5]=pos&0xFF;
		
     if(!SeizeSYSControl())
        return 0;
    
	if( !WriteFile(f_usb ,read_cmd ,sizeof(read_cmd) ,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }

	if(write_bytes != sizeof(read_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	if(!ReadFile(f_usb, buf , size ,&read_bytes,NULL) )
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(read_bytes != size )
    {
        ReleaseSYSControl();
        return 0;
    }
	sts = get_status(f_usb);
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;
}
int WriteTimingRegister(HANDLE f_usb, unsigned short pos, unsigned char  *buf, unsigned int size)
{
	int sts;
	unsigned long write_bytes ;
	unsigned char  write_cmd[10] = {SCSI_CMD_SEND,0x00,SCSI_SDTC_ASIC_REG ,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char  *ptr = write_cmd + 6 ;
	set_triple(ptr,size);

	write_cmd[4]=pos>>8;
	write_cmd[5]=pos&0xFF;
	
    if(!SeizeSYSControl())
        return 0;
   

	if( !WriteFile(f_usb ,write_cmd ,sizeof(write_cmd),&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != sizeof(write_cmd) ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	if( !WriteFile(f_usb ,buf ,size,&write_bytes ,NULL) ) 
    {
        ReleaseSYSControl();
		return 0 ;
    }
	if(write_bytes != size ) 
    {
        ReleaseSYSControl();
        return 0;
    }
	sts= get_status(f_usb) ;
    ReleaseSYSControl();
	return (sts==SCSI_RET_OK)?1:0 ;

}

DWORD USBControlIn(HANDLE f_usb, WORD wValue, WORD wIndex, WORD wLength, LPBYTE lpbuf)
{
    DWORD                   result = 1;
    DWORD                   bytesTransfered=0;
    DWORD                   lastErr = 0;
//  DWORD                   tmpDataLen;
    BOOL                    ret;
    IO_BLOCK                io_block;

    if(!SeizeSYSControl())
        return 0;
    if(f_usb != INVALID_HANDLE_VALUE)
    {
        io_block.uOffset = wValue;
        io_block.uIndex = wIndex;
        io_block.pbyData = lpbuf;
        io_block.uLength = wLength;
            ret = DeviceIoControl(f_usb,
                IOCTL_READ_REGISTERS,
                &io_block,
                sizeof(io_block),
                (PVOID)lpbuf,
                wLength,
                &bytesTransfered,
                NULL);
        if(!ret)
        {
            result = 0;
            //gDeviceExtension.errcode = AV_SYSTEM_ERROR;
            lastErr = GetLastError();        
			if(lastErr == 31)
				result = 0;            
        }
        else
        {
            if(wLength != bytesTransfered)
            {
                result = 0;                             
            }
        }       
    }
    else
    {     
       
        result = 0;
    }
    ReleaseSYSControl();
    return result;

}


DWORD GetPipeConfiguration(HANDLE f_usb, LPBYTE lpbuf, DWORD bufLength)
{
    DWORD                   result = 1;
    DWORD                   bytesTransfered=0;
    DWORD                   lastErr = 0;
    BOOL                    ret;   

    if(!SeizeSYSControl())
        return 0;
    if(f_usb != INVALID_HANDLE_VALUE)
    {       
		ret = DeviceIoControl(f_usb,
			IOCTL_GET_PIPE_CONFIGURATION,
			NULL,
			0,
			(PVOID)lpbuf,
			bufLength,
			&bytesTransfered,
			NULL);
        if(!ret)
        {
            result = 0;            
            lastErr = GetLastError();        
			if(lastErr == 31)
				   result = 0;            
        }         
    }
    else
    {     
       
        result = 0;
    }
    ReleaseSYSControl();
    return result;

}

int GetEvent(LPBYTE lpbuf, DWORD bufLength)
{
	BOOL        ret = TRUE;  	
	int		cbRead;	

	while(ret == TRUE)
	{
		cbRead = 0;	
		ret = DeviceIoControl(
							gEventHdl,
							IOCTL_WAIT_ON_DEVICE_EVENT,
							NULL,
							0,
							lpbuf,
							bufLength,
							(LPDWORD)&cbRead,
							NULL
							);
		if(cbRead > 0)
			break;

		if(!ret)
			cbRead = -(int)GetLastError();
		Sleep(1);
	}	

	return cbRead;
}

DWORD ResetPipe(HANDLE f_usb, WORD wValue, WORD wIndex, WORD wLength, LPBYTE lpbuf)
{
    DWORD                   result = 1;
    DWORD                   bytesTransfered=0;
    DWORD                   lastErr = 0;
//  DWORD                   tmpDataLen;
    BOOL                    ret;
    IO_BLOCK                io_block;

    if(!SeizeSYSControl())
        return 0;
    if(f_usb != INVALID_HANDLE_VALUE)
    {
        io_block.uOffset = wValue;
        io_block.uIndex = wIndex;
        io_block.pbyData = lpbuf;
        io_block.uLength = wLength;
            ret = DeviceIoControl(f_usb,
                IOCTL_RESET_PIPE ,
                (PVOID)lpbuf,
                wLength,
                (PVOID)lpbuf,
                wLength,
                &bytesTransfered,
                NULL);
        if(!ret)
        {
            result = 0;
            //gDeviceExtension.errcode = AV_SYSTEM_ERROR;
            lastErr = GetLastError();        
			   if(lastErr == 31)
				   result = 0;            
        }        
    }
    else
    {     
       
        result = 0;
    }
    ReleaseSYSControl();
    return result;

}


DWORD ReadInquiryCommand(HANDLE fileHandle, char *strCompany, char *strModel, char *strVersion)
{
	BOOL		retApi;
	DWORD       result = 0, returned, length;		
	SCSI_PASS_THROUGH_WITH_BUFFERS g_sptwb;

	ZeroMemory(&g_sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    g_sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
    g_sptwb.spt.PathId = 0;
    g_sptwb.spt.TargetId = 1;
    g_sptwb.spt.Lun = 0;
    g_sptwb.spt.CdbLength = CDB6GENERIC_LENGTH;
    g_sptwb.spt.SenseInfoLength = 32;
    g_sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
    g_sptwb.spt.DataTransferLength = 80;
    g_sptwb.spt.TimeOutValue = 2;
    g_sptwb.spt.DataBufferOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    g_sptwb.spt.SenseInfoOffset = 
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    g_sptwb.spt.Cdb[0] = SCSIOP_INQUIRY;   
	g_sptwb.spt.Cdb[4] = 80;
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) +
       g_sptwb.spt.DataTransferLength;

    retApi = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH,
                             &g_sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &g_sptwb,
                             length,
                             &returned,
                             FALSE);

	if(retApi == FALSE)	
	{
		result = GetLastError();   		
	}
	else
	{
		memcpy(strCompany, &g_sptwb.ucDataBuf[8], 8);
		strCompany[8] = 0;

		memcpy(strModel, &g_sptwb.ucDataBuf[16], 16);
		strModel[16] = 0;

		memcpy(strVersion, &g_sptwb.ucDataBuf[32], 4);
		strVersion[4] = 0;
	}
	
	return result;
}

DWORD DbgCommand(HANDLE fileHandle, BYTE subCmd, BYTE *dataBuffer, BOOL isWrite, DWORD readSize)
{
	BOOL		retApi;
	DWORD       result = 0, returned, length;		
	unsigned char		*data;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;

	ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    
    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
    sptdwb.sptd.SenseInfoLength = 32;
	if(isWrite == TRUE)
		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	else
		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;	
    sptdwb.sptd.DataTransferLength = readSize;
    sptdwb.sptd.TimeOutValue = 30;
    sptdwb.sptd.DataBuffer = dataBuffer;
    sptdwb.sptd.SenseInfoOffset =
       offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);

    sptdwb.sptd.Cdb[0] = RW_DBG_DTATA;   
	
	//1:dbg header 2:dbg data  3:level
	sptdwb.sptd.Cdb[1] = subCmd;
	
	if(isWrite == TRUE)
	{
		DWORD level;
		data = &(sptdwb.sptd.Cdb[2]);

		level = *((DWORD *)dataBuffer);
		ScsiSetQuad(data,level);		
	}
    length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    retApi = DeviceIoControl(fileHandle,
                             IOCTL_SCSI_PASS_THROUGH_DIRECT,
                             &sptdwb,
                             length,
                             &sptdwb,
                             length,
                             &returned,
                             FALSE);

	if(retApi == FALSE )	
	{
		result = GetLastError();        	
	}

	
	return retApi;
	
}

DWORD ReadLargeDbgCommand(HANDLE fileHandle, BYTE subCmd, BYTE *dataBuffer, BOOL isWrite, DWORD readSize)
{
	int start, leftSize, oneTimeSize;
	
	start = 0;
	leftSize = readSize;
	oneTimeSize = 65536;

	while(leftSize >0)
    {
		if(oneTimeSize > leftSize)
			oneTimeSize = leftSize;
				
	    if(!DbgCommand(fileHandle, subCmd, dataBuffer+start , isWrite, oneTimeSize))
        {            
		    return 0 ;
        }			
        start += oneTimeSize;
        leftSize -= oneTimeSize;        
    }
	return 1;
}

void UsbUtil_GetPrinterGuid(GUID *Guid) 
{
	static const GUID USBPRINTER = {0x28d78fad, 0x5a12, 0x11D1, {0xae,
		0x5b, 0x00, 0x00, 0xf8, 0x03, 0xa8, 0xc2}};
	*Guid =  USBPRINTER;
	
}

DWORD DisplayLastError(char *str)
{
	LPVOID lpMsgBuf;
	USHORT Index = 0;
	DWORD error;
	
	error = GetLastError();
	
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	
	
	printf("%s\t%s\n",str,(char *)lpMsgBuf);
	
	LocalFree(lpMsgBuf); 
	return error;
}

char *FindPrinterPath(void)
{
	PSP_DEVICE_INTERFACE_DETAIL_DATA        detailData;
	
	
	ULONG                   Required;
	
	HANDLE                  hDevInfo;
	GUID                    Guid;
	
	ULONG                   Length;
	
	SP_DEVICE_INTERFACE_DATA                        devInfoData;
	int     MemberIndex = 0;
	bool    MyDeviceDetected = FALSE; 
	LONG    Result;
	char *DevicePath   = NULL;
	
	
	Length = 0;
	detailData = NULL;
	
    
    
	UsbUtil_GetPrinterGuid(&Guid);        
    
    
	hDevInfo=SetupDiGetClassDevs 
		(&Guid, 
		NULL, 
		NULL, 
		DIGCF_PRESENT|DIGCF_INTERFACEDEVICE);
    
	if(INVALID_HANDLE_VALUE==hDevInfo)
    {
		DisplayLastError("SetupDiGetClassDevs: ");
		return DevicePath;
    }
	
	devInfoData.cbSize = sizeof(devInfoData);
    
	MemberIndex = 0;
	
	MyDeviceDetected=FALSE;
	
	while(MyDeviceDetected==FALSE)
    {
        
        
        
		Result=SetupDiEnumDeviceInterfaces 
			(hDevInfo, 
			0, 
			&Guid, 
			MemberIndex, 
			&devInfoData);
        
		if(Result==0)
        {   
			DisplayLastError("SetupDiEnumDeviceInterfaces: ");
			break;
        }
        
        
		Result = SetupDiGetDeviceInterfaceDetail 
			(hDevInfo, 
			&devInfoData, 
			NULL, 
			0, 
			&Length, 
			NULL);
        
        
        
		//Allocate memory for the hDevInfo structure, using the returned Length.
		detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(Length);
        
		//Set cbSize in the detailData structure.
		detailData -> cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        
		//Call the function again, this time passing it the returned buffer size.
		Result = SetupDiGetDeviceInterfaceDetail 
			(hDevInfo, 
			&devInfoData, 
			detailData, 
			Length, 
			&Required, 
			NULL);
        
		if(Result==0)
        {   
			DisplayLastError("SetupDiGetDeviceInterfaceDetail: ");
			break;
        }
		
		DevicePath = _strdup(detailData->DevicePath); 
		MyDeviceDetected=TRUE;
        
    }            
    
	free(detailData);
	SetupDiDestroyDeviceInfoList(hDevInfo);
    
    
	return DevicePath;
}

char *GetPrinterDeviceID(HANDLE hPrinter)
{
	const DWORD MAX_1284_ID = 1024;
	PUCHAR DeviceID = (PUCHAR)malloc(MAX_1284_ID);
	//UCHAR varCommand[3];
	//int		varStatus[3];

	if(DeviceID)
    {
		BOOL bResult;
		DWORD nBytesReturned;
		
		memset(DeviceID, 0, MAX_1284_ID);
		bResult = DeviceIoControl(
			(HANDLE)  hPrinter, 
			IOCTL_USBPRINT_GET_1284_ID, // dwIoControlCode 
			NULL,   // lpInBuffer
			0,      // nInBufferSize
			(PUCHAR)  DeviceID,
			(DWORD)  MAX_1284_ID, 
			(LPDWORD)  &nBytesReturned,
			(LPOVERLAPPED) NULL );
		if(bResult==0)
		{
			DisplayLastError("GetPritneDeviceID:  ");
			free (DeviceID);
			DeviceID = NULL;
		}
		
		
    }		
	return (char*)DeviceID;
}

int ReadPrinterDgbLog(HANDLE hPrinter, LPBYTE lpbuf, WORD length)
{
	BOOL bResult = TRUE;
	WORD readLength;
	LPBYTE pTemp = lpbuf;
	UCHAR varCommand[3];
	DWORD nBytesReturned;
	
	varCommand[0] = 0x40;
	varCommand[1] = 0x00;
	varCommand[2] = 0x0A;

	while(bResult == TRUE && length > 0)	
	{
		readLength = length;			
		bResult = DeviceIoControl ((HANDLE)  hPrinter, IOCTL_USBPRINT_VENDOR_GET_COMMAND,
				varCommand, 3, pTemp, readLength, (LPDWORD)&nBytesReturned, (LPOVERLAPPED)NULL);

		pTemp += readLength;
		length -= readLength;		
	}
	
	return bResult;	
}

int SwitchPrinterDesc(HANDLE hPrinter)
{
	BOOL bResult;
	UCHAR varCommand[3], dummy[20];
	DWORD nBytesReturned;
	
	varCommand[0] = 0x40;
	varCommand[1] = 0x00;
	varCommand[2] = 0x0C;

	bResult = DeviceIoControl ((HANDLE)  hPrinter, IOCTL_USBPRINT_VENDOR_GET_COMMAND,
			varCommand, 3, dummy, 12, (LPDWORD)&nBytesReturned, (LPOVERLAPPED)NULL);
	
	return bResult;	
}

int ResetPrinter(HANDLE hPrinter)
{
	BOOL bResult;
	UCHAR varCommand[3], dummy[20];
	DWORD nBytesReturned;
	
	varCommand[0] = 0x40;
	varCommand[1] = 0x00;
	varCommand[2] = 0x0B;

	bResult = DeviceIoControl ((HANDLE)  hPrinter, IOCTL_USBPRINT_VENDOR_GET_COMMAND,
			varCommand, 3, dummy, 12, (LPDWORD)&nBytesReturned, (LPOVERLAPPED)NULL);
	
	return bResult;	
}

DWORD DbgLevelCommand(HANDLE hPrinter, char *lpbuf, BOOL isWrite)
{
	BOOL bResult;
	char varCommand[12], data[20];
	DWORD nBytesReturned;
	
	varCommand[0] = 0x40;
	varCommand[1] = 0x00;
	varCommand[2] = 0x0D;

	if(isWrite)	
	{
		memcpy(&varCommand[4], lpbuf, 4);
		bResult = DeviceIoControl ((HANDLE)  hPrinter, IOCTL_USBPRINT_VENDOR_SET_COMMAND,
			varCommand, 8, NULL, 0, (LPDWORD)&nBytesReturned, (LPOVERLAPPED)NULL);
	}
	else
	{
		bResult = DeviceIoControl ((HANDLE)  hPrinter, IOCTL_USBPRINT_VENDOR_GET_COMMAND,
			varCommand, 3, data, 4, (LPDWORD)&nBytesReturned, (LPOVERLAPPED)NULL);
		memcpy(lpbuf, data, 4);
	}
	return bResult;	
}

void  open_printer(HANDLE *s_hdl, char *szCheckStr)
{
	char *printer_name = NULL;
	HANDLE hPrinter;
	char *devid = NULL;

	*s_hdl = NULL;
	if(printer_name = FindPrinterPath())
	{
		hPrinter = CreateFile(printer_name, GENERIC_WRITE, FILE_SHARE_READ,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if(hPrinter==INVALID_HANDLE_VALUE)
			goto RETURN;

		devid = GetPrinterDeviceID(hPrinter);
		if(devid == NULL || strstr(devid,szCheckStr) == NULL)
		{
			CloseHandle(hPrinter);
		}
		else
		{
			*s_hdl = hPrinter;
		}		
	}

RETURN:
	if(printer_name != NULL)
		free(printer_name);
	if(devid != NULL)
		free(devid);
}
