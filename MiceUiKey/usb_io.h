#ifndef _USB_IO_H
#define _USB_IO_H

#include "winioctl.h"
//#include "int_hdr.h"
// Drive type names
#define DRVUNKNOWN		0
#define DRVFIXED		1
#define DRVREMOTE		2
#define DRVRAM			3
#define DRVCD			4
#define DRVREMOVE		5

#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER
// IOCTL control code
#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH        CTL_CODE(IOCTL_SCSI_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH_DIRECT  CTL_CODE(IOCTL_SCSI_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_SCSI_GET_INQUIRY_DATA     CTL_CODE(IOCTL_SCSI_BASE, 0x0403, METHOD_BUFFERED, FILE_ANY_ACCESS)

//// The following structures all can find at MSDN.
// enumeration type specifies the various types of storage buses


// retrieve the storage device descriptor data for a device. 
typedef struct _STORAGE_DEVICE_DESCRIPTOR {
  ULONG  Version;
  ULONG  Size;
  UCHAR  DeviceType;
  UCHAR  DeviceTypeModifier;
  BOOLEAN  RemovableMedia;
  BOOLEAN  CommandQueueing;
  ULONG  VendorIdOffset;
  ULONG  ProductIdOffset;
  ULONG  ProductRevisionOffset;
  ULONG  SerialNumberOffset;
  STORAGE_BUS_TYPE  BusType;
  ULONG  RawPropertiesLength;
  UCHAR  RawDeviceProperties[1];

} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

// retrieve the properties of a storage device or adapter. 
typedef enum _STORAGE_QUERY_TYPE {
  PropertyStandardQuery = 0,
  PropertyExistsQuery,
  PropertyMaskQuery,
  PropertyQueryMaxDefined

} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

// retrieve the properties of a storage device or adapter. 
typedef enum _STORAGE_PROPERTY_ID {
  StorageDeviceProperty = 0,
  StorageAdapterProperty,
  StorageDeviceIdProperty

} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

// retrieve the properties of a storage device or adapter. 
typedef struct _STORAGE_PROPERTY_QUERY {
  STORAGE_PROPERTY_ID  PropertyId;
  STORAGE_QUERY_TYPE  QueryType;
  UCHAR  AdditionalParameters[1];

} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;


typedef struct _SCSI_PASS_THROUGH {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;

typedef struct _SCSI_PASS_THROUGH_DIRECT {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[32];
    UCHAR             ucDataBuf[128];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[32];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;


#define CDB6GENERIC_LENGTH                   6
#define CDB10GENERIC_LENGTH                  10
#define CDB12GENERIC_LENGTH                  12

#define SETBITON                             1
#define SETBITOFF                            0

//
// Mode Sense/Select page constants.
//

#define MODE_PAGE_ERROR_RECOVERY        0x01
#define MODE_PAGE_DISCONNECT            0x02
#define MODE_PAGE_FORMAT_DEVICE         0x03
#define MODE_PAGE_RIGID_GEOMETRY        0x04
#define MODE_PAGE_FLEXIBILE             0x05
#define MODE_PAGE_VERIFY_ERROR          0x07
#define MODE_PAGE_CACHING               0x08
#define MODE_PAGE_PERIPHERAL            0x09
#define MODE_PAGE_CONTROL               0x0A
#define MODE_PAGE_MEDIUM_TYPES          0x0B
#define MODE_PAGE_NOTCH_PARTITION       0x0C
#define MODE_SENSE_RETURN_ALL           0x3f
#define MODE_SENSE_CURRENT_VALUES       0x00
#define MODE_SENSE_CHANGEABLE_VALUES    0x40
#define MODE_SENSE_DEFAULT_VAULES       0x80
#define MODE_SENSE_SAVED_VALUES         0xc0
#define MODE_PAGE_DEVICE_CONFIG         0x10
#define MODE_PAGE_MEDIUM_PARTITION      0x11
#define MODE_PAGE_DATA_COMPRESS         0x0f

#define SCSI_IOCTL_DATA_OUT          0
#define SCSI_IOCTL_DATA_IN           1
#define SCSI_IOCTL_DATA_UNSPECIFIED  2

#ifndef offsetof
	#ifdef  _WIN64
	#define offsetof(s,m)   (size_t)( (ptrdiff_t)&(((s *)0)->m) )
	#else
	#define offsetof(s,m)   (size_t)&(((s *)0)->m)
	#endif
#endif
//
// SCSI CDB operation codes
//

#define SCSIOP_TEST_UNIT_READY     0x00
#define SCSIOP_REZERO_UNIT         0x01
#define SCSIOP_REWIND              0x01
#define SCSIOP_REQUEST_BLOCK_ADDR  0x02
#define SCSIOP_REQUEST_SENSE       0x03
#define SCSIOP_FORMAT_UNIT         0x04
#define SCSIOP_READ_BLOCK_LIMITS   0x05
#define SCSIOP_REASSIGN_BLOCKS     0x07
#define SCSIOP_READ6               0x08
#define SCSIOP_RECEIVE             0x08
#define SCSIOP_WRITE6              0x0A
#define SCSIOP_PRINT               0x0A
#define SCSIOP_SEND                0x0A
#define SCSIOP_SEEK6               0x0B
#define SCSIOP_TRACK_SELECT        0x0B
#define SCSIOP_SLEW_PRINT          0x0B
#define SCSIOP_SEEK_BLOCK          0x0C
#define SCSIOP_PARTITION           0x0D
#define SCSIOP_READ_REVERSE        0x0F
#define SCSIOP_WRITE_FILEMARKS     0x10
#define SCSIOP_FLUSH_BUFFER        0x10
#define SCSIOP_SPACE               0x11
#define SCSIOP_INQUIRY             0x12
#define SCSIOP_VERIFY6             0x13
#define SCSIOP_RECOVER_BUF_DATA    0x14
#define SCSIOP_MODE_SELECT         0x15
#define SCSIOP_RESERVE_UNIT        0x16
#define SCSIOP_RELEASE_UNIT        0x17
#define SCSIOP_COPY                0x18
#define SCSIOP_ERASE               0x19
#define SCSIOP_MODE_SENSE          0x1A
#define SCSIOP_START_STOP_UNIT     0x1B
#define SCSIOP_STOP_PRINT          0x1B
#define SCSIOP_LOAD_UNLOAD         0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC  0x1C
#define SCSIOP_SEND_DIAGNOSTIC     0x1D
#define SCSIOP_MEDIUM_REMOVAL      0x1E
#define SCSIOP_READ_CAPACITY       0x25
#define SCSIOP_READ                0x28
#define SCSIOP_WRITE               0x2A
#define SCSIOP_SEEK                0x2B
#define SCSIOP_LOCATE              0x2B
#define SCSIOP_WRITE_VERIFY        0x2E
#define SCSIOP_VERIFY              0x2F
#define SCSIOP_SEARCH_DATA_HIGH    0x30
#define SCSIOP_SEARCH_DATA_EQUAL   0x31
#define SCSIOP_SEARCH_DATA_LOW     0x32
#define SCSIOP_SET_LIMITS          0x33
#define SCSIOP_READ_POSITION       0x34
#define SCSIOP_SYNCHRONIZE_CACHE   0x35
#define SCSIOP_COMPARE             0x39
#define SCSIOP_COPY_COMPARE        0x3A
#define SCSIOP_WRITE_DATA_BUFF     0x3B
#define SCSIOP_READ_DATA_BUFF      0x3C
#define SCSIOP_CHANGE_DEFINITION   0x40
#define SCSIOP_READ_SUB_CHANNEL    0x42
#define SCSIOP_READ_TOC            0x43
#define SCSIOP_READ_HEADER         0x44
#define SCSIOP_PLAY_AUDIO          0x45
#define SCSIOP_PLAY_AUDIO_MSF      0x47
#define SCSIOP_PLAY_TRACK_INDEX    0x48
#define SCSIOP_PLAY_TRACK_RELATIVE 0x49
#define SCSIOP_PAUSE_RESUME        0x4B
#define SCSIOP_LOG_SELECT          0x4C
#define SCSIOP_LOG_SENSE           0x4D

//avision vandor command
#define READ_FILE_SIZE				 0x52
#define READ_FILE					 0x53
#define SCAN2PC2                 	 0x54
#define SCAN2FLASH2                  0x55
#define SCAN2PC_CARD		         0x56
#define SCAN2PC_TIFF				 0x57


#define UPDATE_FW_START		         0x5C
#define UPDATE_FW_DATA		 		 0x5D
#define UPDATE_FW_END				 0x5E
#define RW_DBG_DTATA		         0x5F
/*SCSI endian transfer*/
#define set_double(var,val)	var[0] = ((val) >> 8 ) & 0xff ; \
							var[1] = ((val) 	 ) & 0xff
							
#define set_triple(var,val)	var[0] = ((val) >> 16) & 0xff ; \
							var[1] = ((val) >> 8 ) & 0xff ; \
							var[2] = ((val) 	 ) & 0xff 

#define set_quad(var,val)	var[0] = ((val) >> 24) & 0xff ; \
							var[1] = ((val) >> 16) & 0xff ; \
							var[2] = ((val) >> 8 ) & 0xff ; \
							var[3] = ((val) 	 ) & 0xff 

#define get_double(var)	((*var << 8) + *(var + 1))
#define get_triple(var)	((*var << 16) + 	\
						 (*(var + 1)<<8) + *(var+2))
						 
#define get_quad(var)	( (*var << 24) + \
						(*(var + 1) << 16) + \
						(*(var + 2) <<  8) + *(var + 3))

#define SCSI_CMD_INVAL			0xFF
#define SCSI_CMD_TESTUNITREADY	0x00
#define SCSI_CMD_REQUESTSENSE	0x03
#define SCSI_CMD_MEDIACHECK		0x08
#define SCSI_CMD_INQUIRY		0x12
#define SCSI_CMD_RESERVEUNIT	0x16
#define SCSI_CMD_RELEASEUNIT	0x17
#define SCSI_CMD_SCAN			0x1B
#define SCSI_CMD_SENDDIAG		0x1D
#define SCSI_CMD_SETWINDOW		0x24
#define SCSI_CMD_READ			0x28
#define SCSI_CMD_RW_MEM  	    0x29
#define SCSI_CMD_SEND			0x2A
#define SCSI_CMD_OBJPOS			0x31
#define SCSI_CMD_BUFSTATUS		0x34

#define SCSI_CMD_PRINTER_READ   0x41
#define SCSI_CMD_PRINTER_SEND   0x42

#define SCSI_RET_INVAL			0xFF
#define SCSI_RET_OK				0x00	// return ok
#define SCSI_RET_RS				0x02	// return request sense
#define SCSI_RET_BZ				0x08	// return busy
#define SCSI_RET_END			0x09	// no job.
#define SCSI_RET_TEST			0x10
#define SCSI_RET_FULL			0x20
#define SCSI_RET_EMPTY			0x21

// define SCSI send command : data type code.
// send print data
#define SCSI_SDTC_PRINT_DATA	0x40
// send print info
#define SCSI_SDTC_PRINT_INFO 0x41
//Send test data
#define SCSI_SDTC_TEST_DATA    0x6E
// set gamma table
#define SCSI_SDTC_GAMMA	0x81
// set calibration data
#define SCSI_SDTC_CALIB	0x82
// set 3x3 matrix
#define SCSI_SDTC_M3x3	0x83
// set nvram data
#define SCSI_SDTC_NVRAM	0x85
// set flash data
#define SCSI_SDTC_FLASH	0x86
// send gain value
#define SCSI_SDTC_GAIN	0x91

#define SCSI_SDTC_THREAD_INFO   0x95 //  send thread info
#define SCSI_SDTC_COPY_INFO		0x96 // set copy info
#define SCSI_SDTC_RESTART		0x97 // restart MFP
#define SCSI_SDTC_FAX_TEST_INFO	0x98 // set fax test info
#define SCSI_SDTC_GET_ERROR		0x99 // Display error message and clear 
// send light source control
#define SCSI_SDTC_LC		0xA0

#define SCSI_SDTC_BUTTON		0xA1

// send power-saving timer
#define SCSI_SDTC_TIMER	0xA2

//Send ASIC register value
#define SCSI_SDTC_ASIC_REG		0xB0

// send nvram pointer
#define SCSI_SDTC_NVRAM_POINTER	0xD0

// set pregamma linear 
#define SCSI_SDTC_SET_PG_LINEAR		0xe1 
// set pregamma nonlinear
#define SCSI_SDTC_SET_PG_NONLINEAR	0xe2

// set start scan position
#define SCSI_SDTC_SET_START_POS		0xe3

// set target value
#define SCSI_SDTC_LED_TARGET		0xe4
#define SCSI_SDTC_OFFSET_TARGET		0xe5

// set sekw distance
#define SCSI_SDTC_SKEW_DISTANCE		0xe6
 
// define SCSI read command : data type code
// Image Data
#define SCSI_RDTC_IMAGE		0x00
// Printer infomation
#define SCSI_RDTC_PRINTER	0x20
// end of page
#define SCSI_RDTC_EOP		0x40
// Get calibration format
#define SCSI_RDTC_CALIB_FMT	0x60
// Read data for gray calib.
#define SCSI_RDTC_CALIB_GRAY 	0x61
//Read data for color calibration
#define SCSI_RDTC_CALIB_COLOR	0x62
// Detect accessories
#define SCSI_RDTC_DETECT_ACC	0x64 
// get exposure info.
#define SCSI_RDTC_EXPOSURE		0x65
// get dark calib. data
#define SCSI_RDTC_CALIB_DARK	0x66
// Read NVRAM data
#define SCSI_RDTC_NVRAM_DATA	0x69
// Read Flash RAM information
#define SCSI_RDTC_FLASH_INFO    0x6A
// Read Flash RAM data
#define SCSI_RDTC_FLASH_DATA	0x6b
//Read acceleration information
#define SCSI_RDTC_ACCEL_INFO    0x6C
//Read test data information
#define SCSI_RDTC_TEST_INFO     0x6D
//Read test data
#define SCSI_RDTC_TEST_DATA     0x6E
// Get RAW data format
#define SCSI_RDTC_RAW_FMT		0x70
// Get Raw data
#define SCSI_RDTC_RAW_DATA		0x71
// Get 3x3 color matrix
#define SCSI_RDTC_M3x3			0x83
// Optical unit control
#define SCSI_RDTC_OPTICAL		0x88
//  Firmware Status
#define SCSI_RDTC_FIRMWARE		0x90
// Read scanner state
#define SCSI_RDTC_SCANNER		0x92
// Read Image block size
#define SCSI_RDTC_BLOCK_SIZE	0x94

// read thread infomation
#define SCSI_RDTC_THREAD_INFO	0x95 
// read shading's header
#define SCSI_RDTC_SHADING_HEADER	0x96 

// read taget value
#define SCSI_RDTC_LED_TARGET		0x97
#define SCSI_RDTC_OFFSET_TARGET		0x98

// Light source status
#define SCSI_RDTC_LIGHT			0xa0
// button status
#define SCSI_RDTC_BUTTON		0xa1

//Read ASIC register value
#define SCSI_RDTC_ASIC_REG		0xB0

#define FRTYPE_CTAB			1
#define FRTYPE_FIRMWARE		2
#define FRTYPE_RAWLINE		3
#define FRTYPE_FMDBG		4
#define FRTYPE_DEBUG		5
#define FRTYPE_FONT			6
#define FRTYPE_DEBUG_FILE   7
#define FRTYPE_DEBUG_LEVEL	8

#define RESERVE_UNIT_PAGE_START      0
#define RESERVE_UNIT_JOB_START       2
#define RELEASE_UNIT_PAGE_END        0
#define RELEASE_UNIT_ABORT           1
#define RELEASE_UNIT_JOB_END         2

#define DGB_FILE_INIT   1
#define DGB_FILE_OPEN   2
#define DGB_FILE_CLOSE   3

typedef struct _scsi_file_format {
    DWORD      fileSize;
    char        fileName[24];	
    DWORD      offset;
    BYTE       state;
    char        *pBuf;
} scsi_dbg_file_format ;

typedef struct
{
	unsigned char  BytesPerUnit;  
	unsigned char  Ability1;      
	unsigned long  FlashRamSize[8]; 
	unsigned char  Reserved[6];	
} FR_INFO;

typedef struct
{
	Uint8				isADFScan;
	Uint8				txQuality;
	Uint16				jobID;
	Uint8				brightness;
	Uint8				headerOn;
	Uint8				isJpeg;
	Uint8				isRealTimeTx;
	Uint8      			scanSize;		
	Uint8				dialType;
	Uint8				dialIndex;
	Uint8				dummy;
	Uint8				headerBuf[124];	
	Uint32				delayMinutes;
	char				phNum[60+2];
}stFaxScanSetting;

typedef enum
{
	eScDark0 = 0,
	eScDark2 = 2,
	eScDark3 = 3,
	eScDark4 = 4,
	eScDark8 = 8

}enumDarkFactor;

typedef struct
{
	Uint8*			pWhiteShadingData;
	Uint8*			pDarkShadingData;
	Uint8			darkFactor;	
	Uint8			bitsPerChannel;
	Uint8			dummy[2];
	Uint32			shadingSize;
}stScShadingData;

typedef struct
{
	Uint16			red;
	Uint16			green;
	Uint16			blue;
	Uint16			ccd;

}stLedSetting;

typedef enum
{
	e0DPI	= 0,
	e150DPI = 150,
	e300DPI = 300,
	e600DPI = 600,
	e1200DPI = 1200
}enumDpi;

typedef enum
{
	ePCScan = 0,
	eTextSlowCopy,	//zoom != 100 %
	eTextFastCopy,	//zoom == 100 %
	eMixSlowCopy,	//zoom != 100 %
	eMixFastCopy,	//zoom == 100 %
	eFax,
	eNetScan
					//(因為txt跟photo設定相同,所以photo請使用 txt slow 或 txt fast)
}enumRequestType;
typedef enum
{
	eScSourceFlatbed = 0,
	eScSourceADFFront,
	eScSourceADFRear,
	eScSourceADFDuplex,
	eScSourceADFDuplexFront,
	eScSourceADFDuplexRear
}enumScImageSource;
typedef enum
{
	eColorScanMode=0,
	eRedScanMode,
	eGreenScanMode,
	eBlueScanMode,
	eTrueGrayScanMode

}enumScanSettingMode;
typedef enum
{
	e150dpi=0,
	e300dpi,
	e600dpi,
	e1200dpi

}enumSettingXdpi;
/*
	CorrespondingMode
	
		
	bits	   31 ~ 24       23 ~ 18            17 ~ 12              11 ~ 6                5 ~ 0  
			+----------+-----------------+-------------------+---------------------+-----------------+
	value	| reserved | enumRequestType | enumScImageSource | enumScanSettingMode | enumSettingXdpi |
			+----------+-----------------+-------------------+---------------------+-----------------+
	
	Resolution is unnecessary for copy setting.
		
*/

typedef struct
{
	Uint32			    CorrespondingMode;
	Uint32              ExposureTime;
	int					HasCaliDataFlag;
	int					NeedRecord2Flash;
	stScShadingData		ShadingRecord;
	Uint32 			    ShadingDataSize;
	Uint16				Xdpi;
	Uint16				Ydpi;
	stLedSetting        LedRecord;
	Uint16              GainRecord;
	Uint16              OffsetRecord;
	Uint32          	pModeState;
	Uint32            	pModeData;
}stCaliProfile;

#ifdef	_WINDOWS	//for Windows		// Window predfines symbols
#pragma pack(1)							// Avision's DLL use 1 byte alignment
#endif

#ifdef	macintosh	//for Macintosh		// MetroWerk CodeWarriar predfines symbols
#pragma once		on					// This header will be included only once.
#pragma options align=packed
#endif





enum _photometry {kLinear, grayLinear, rgbLinear, cmykLinear, \
    faxlab, yccLinear, cielab };
typedef enum _photometry PHOTOMETRY;

enum _units {UNKNOWN, CENTIMETERS, INCH};   /*singular, plural*/
typedef enum _units UNITS;
struct _int_hdr {
    int     bits_per_sample;    /*bits per pixel per separation*/
    int     bytes_per_sl;       /*bytes per scan line per separation*/
    int     pixels;             /*pixels per scan line*/
    int     scanlines;          /*scan lines per separation*/
    int     imageparts;         /*number of input separations*/
    int     white;              /*value of white, default 0*/
    short   res_x;           
	short   res_y;
    PHOTOMETRY      photometry;
    UNITS   units;
    double  resolution,         /**/
            aspect;             /**/
};
typedef struct _int_hdr INT_HDR;

typedef struct
{
   unsigned long				imageBytes;
   unsigned long				widthPix;
   unsigned long				lengthPix;
   unsigned long				bytes_per_sl ; // Avision add this for more comfortable access.
   unsigned long				resX;
   unsigned long				resY;
   unsigned short				bitsPerSample;
   unsigned short				samplesPerPix;
   unsigned short          photometric; //from TIFF spec
   unsigned short          compression; //from TIFF spec
   unsigned short          planarConfig; //from TIFF spec
   unsigned short				unused;
   unsigned long				page_info_ng ;
} ImageDescType;

typedef struct
{    
    WORD  Ability;
    WORD  TestDataSize[16];    
    BYTE  Reserved[6];  
}TESTDATAINFO, *LPTESTDATAINFO;

typedef struct _thread_info {
	unsigned char   disableLog;	
	char   			name[25];
	unsigned short	id;	
} stThreadInfo , *stThreadInfoPtr;

typedef struct
{
  DWORD id;
  BYTE IDCode[8];
  UINT64 DataSize;
  DWORD CheckCode;
  WORD OPCode;
  WORD Class;
  BYTE Reserve[4];
}AVMFP_Command;

typedef struct
{
  DWORD ID;
  DWORD StatusCode;
  UINT64 DataSize;
  DWORD CheckCode;
  BYTE Reserved[12];
}AVMFP_Status;

#ifdef	_WINDOWS	//for Windows		// Window predfines symbols
#pragma pack()
#endif

#ifdef	macintosh	//for Macintosh		// MetroWerk CodeWarriar predfines symbols
#pragma options align=reset
#endif

#define     MACHINE_STATE           0 
#define     CONTENTS_OF_PANEL       1                                     
#define     DEVICE_SETTINGS         2
#define     BUTTON_ID               3

void  open_usb( HANDLE *s_hdl, HANDLE *hdl_dbg_in, HANDLE *hdl_dbg_out, WORD vid , WORD pid );
int set_cmd_data(HANDLE f_usb,BYTE *cmd, unsigned int cmd_size, BYTE *pData, unsigned int dataLen);
int read_scan_image(HANDLE f_usb,char *buf,unsigned int size);
int  test_unit_ready(HANDLE f_usb,unsigned int);
int  get_status(HANDLE f_usb);
void close_usb(HANDLE f_usb, HANDLE f_dbg_in, HANDLE f_dbg_out);
int read_data(HANDLE f_usb,char *buf,unsigned int size);
int  check_data(char *buf,unsigned int size);
int  inq_data(HANDLE f_usb,char *) ;
int  scan_data(HANDLE f_usb);
int release_unit(HANDLE f_usb, BYTE type);
int reserve_unit(HANDLE f_usb, BYTE type);
int write_data(HANDLE f_usb,char *buf,unsigned int size);
int write_profile(HANDLE f_usb,char *buf,unsigned int size);
int write_info(HANDLE f_usb,char *buf,unsigned int size);
int wait_ready(HANDLE f_usb) ;
int read_dbg_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char type, BOOL bPrinter) ;
int read_large_dbg_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char type, BOOL bPrinter);
int send_dbg_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char type, BOOL bPrinter);
int read_dbg_size(HANDLE f_usb,unsigned int *size) ;
int read_fw_data(HANDLE f_usb,char *buf,unsigned int size);
int wait_for_image_info(HANDLE f_usb,/*ImageDescType *info*/char *,unsigned int );
int read_image(HANDLE f_usb,ImageDescType *info,char *fn) ;
int read_block_size(HANDLE f_usb,DWORD *blockSize,unsigned int size);
int media_check(HANDLE f_usb, unsigned char  *buf);
//void set_hdr(INT_HDR *hdr,ImageDescType *info);
int WriteNVRAM(HANDLE f_usb_in,HANDLE f_usb_out, unsigned short pos, unsigned char  *buf, unsigned int size, BOOL bPrinter);
int ReadNVRAM(HANDLE f_usb_in,HANDLE f_usb_out, unsigned short pos ,unsigned char  *buf, unsigned int size, BOOL bPrinter);
int WriteTimingRegister(HANDLE f_usb, unsigned short pos, unsigned char  *buf, unsigned int size);
int ReadTimingRegister(HANDLE f_usb, unsigned short pos ,unsigned char  *buf, unsigned int size);

DWORD USBControlIn(HANDLE f_usb, WORD wValue, WORD wIndex, WORD wLength, LPBYTE lpbuf);
int read_cmd_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char cmd, unsigned char type,BOOL bPrinter);
int send_cmd_data(HANDLE f_usb_in,HANDLE f_usb_out,char *buf,unsigned int size, unsigned char SendCmd, unsigned char type,BOOL bPrinter);
DWORD GetPipeConfiguration(HANDLE f_usb, LPBYTE lpbuf, DWORD bufLength);
DWORD ResetPipe(HANDLE f_usb, WORD wValue, WORD wIndex, WORD wLength, LPBYTE lpbuf);
BOOL GetEvent(LPBYTE lpbuf, DWORD bufLength);
DWORD ReadInquiryCommand(HANDLE fileHandle, char *strCompany, char *strModel, char *strVersion);
DWORD DbgCommand(HANDLE fileHandle, BYTE subCmd, BYTE *dataBuffer, BOOL isWrite, DWORD readSize);
int read_mem_data(HANDLE f_usb_in,HANDLE f_usb_out,unsigned int addr, char *buf,unsigned int size);
void  open_printer(HANDLE *s_hdl, char *szCheckStr);
int ReadPrinterDgbLog(HANDLE hPrinter, LPBYTE lpbuf, WORD length);
int SwitchPrinterDesc(HANDLE hPrinter);
int ResetPrinter(HANDLE hPrinter);
DWORD DbgLevelCommand(HANDLE hPrinter, char *lpbuf, BOOL isWrite);
DWORD ReadLargeDbgCommand(HANDLE fileHandle, BYTE subCmd, BYTE *dataBuffer, BOOL isWrite, DWORD readSize);
#endif
