
#define TIMEOUT 20000
#define TIMEOUT_DETECTION 20000

#ifndef BIT0
#define BIT0           1
#define BIT1           2
#define BIT2           4
#define BIT3           8
#define BIT4           0x10
#define BIT5           0x20
#define BIT6           0x40
#define BIT7           0x80
#define BIT8           0x100
#define BIT9           0x200
#define BIT10          0x400
#define BIT11          0x800
#define BIT12          0x1000
#define BIT13          0x2000
#define BIT14          0x4000
#define BIT15          0x8000
#define BIT16          0x10000
#define BIT17          0x20000
#define BIT18          0x40000
#define BIT19          0x80000
#define BIT20          0x100000
#define BIT21          0x200000
#define BIT22          0x400000
#define BIT23          0x800000
#define BIT24          0x1000000
#define BIT25          0x2000000
#define BIT26          0x4000000
#define BIT27          0x8000000
#define BIT28          0x10000000
#define BIT29          0x20000000
#define BIT30          0x40000000
#define BIT31          0x80000000
#endif

#ifndef BOOL
typedef unsigned char  BOOL;
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned long  QWORD;
typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned int   UINT;
typedef unsigned long  ULONG;
#endif

#ifndef LOWORD
#define LOWORD(l)      ((WORD)((DWORD)(l)))
#define HIWORD(l)      ((WORD)(((DWORD)(l)>>16)&0xFFFF))
#define LOBYTE(w)      ((BYTE)(w))
#define HIBYTE(w)      ((BYTE)(((WORD)(w)>>8)&0xFF))
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

// DPMI regs structure to simulate real mode interrupt
#pragma pack (push, 1);
typedef struct _DPMIREGS{
    DWORD edi;
    DWORD esi;
    DWORD ebp;
    DWORD reserved;
    DWORD ebx;
    DWORD edx;
    DWORD ecx;
    DWORD eax;
    WORD flags;
    WORD es;
    WORD ds;
    WORD fs;
    WORD gs;
    WORD ip;
    WORD cs;
    WORD sp;
    WORD ss;
} DPMIREGS;


// ATA identify structure
typedef struct {
    WORD wGenConfig;                        // 0     15bit:0 = ATA, 7bit:1=??
    WORD wNumCyls;                          // 1
    WORD wSpecificConfiguration;            // 2
    WORD wNumHeads;                         // 3
    WORD wBytesPerTrack;                    // 4
    WORD wBytesPerSector;                   // 5
    WORD wSectorsPerTrack;                  // 6
    WORD wVendorUnique[3];                  // 7 - 9
    char sSerialNumber[20];                 // 10 - 19
    WORD wBufferType;                       // 20
    WORD wBufferSize;                       // 21
    WORD wECCSize;                          // 22
    char sFirmwareRev[8];                   // 23 - 26
    char sModelNumber[40];                  // 27 - 46
    WORD wMaxNumPerInterupt;                // 47
    WORD wDoubleWordIO;                     // 48
    WORD wCapabilities;                     // 49    8bit:1 = DMA, 9bit:1 = LBA, 10bit:1 = IORDY, 11bit:1 = IORDY
    WORD wCapabilities2;                    // 50
    WORD wPIOTiming;                        // 51
    WORD wDMATiming;                        // 52
    WORD wBS;                               // 53    2bit:1 = 88?, 1bit:1 = 7064?
    WORD wNumCurrentCyls;                   // 54
    WORD wNumCurrentHeads;                  // 55
    WORD wNumCurrentSectorsPerTrack;        // 56
    DWORD ulCurrentSectorCapacity;          // 57 - 58
    WORD wMultSectorStuff;                  // 59
    DWORD ulTotalAddressableSectors;        // 60 - 61
    WORD wSingleWordDMA;                    // 62
    WORD wMultiWordDMA;                     // 63    0bit:1=mode0, 1bit:1=mode1, 2bit:1=mode2, 8bit:1=mode0, 9bit1=mode1, 10bit:1=mode2
    WORD wPIOMode;                          // 64
    WORD wMinMultiwordDMACycleTime;         // 65
    WORD wRecommendedMultiwordDMACycleTime; // 66
    WORD wMinPIOCycleTimewoFlowCtrl;        // 67
    WORD wMinPIOCycleTimeWithFlowCtrl;      // 68
    WORD wReserved2[6];                     // 69 - 74
    WORD wQueueDepth;                       // 75
    WORD SATACap1;                          // 76
    WORD SATACap2;                          // 77
    WORD SATAFeaturesSupported;             // 78
    WORD SATAFeaturesEnabled;               // 79
    WORD wMajorVersion;                     // 80    4bit:1=ATA/ATAPI-4, 5bit:1=ATA/ATAPI-5, 6bit:1=ATA/ATAPI-6, 7bit:1=ATA/ATAPI-7
    WORD wMinorVersion;                     // 81
    WORD wCommandSetSupported1;             // 82    0bit:1=SMART, 1bit:SecurityMode
    WORD wCommandSetSupported2;             // 83    0bit:1=DOWNLOAD Microcode
    WORD wCommandSetSupported3;             // 84    0bit:1=SMART error logging, 1bit:1=SMART self-test
    WORD wCommandSetEnable1;                // 85    0bit:1=SMART
    WORD wCommandSetEnable2;                // 86    0bit:1=DOWNLOAD Microcode
    WORD wCommandSetDefault;                // 87
    WORD wUltraDMAMode;                     // 88    0bit:1=UltraDMAmode0, 1bit:1=UltraDMAmode1
    WORD wTimeReqForSecurityErase;          // 89
    WORD wTimeReqForEnhancedSecure;         // 90
    WORD wCurrentPowerManagement;           // 91
    WORD wMasterPasswordRevision;           // 92
    WORD wHardwareResetResult;              // 93
    WORD wAcoustricmanagement;              // 94
    WORD wStreamMinRequestSize;             // 95
    WORD wStreamingTimeDMA;                 // 96
    WORD wStreamingAccessLatency;           // 97
    DWORD ulStreamingPerformance;           // 98 - 99
    __int64 MaxUserLBA;                     // 100 - 103
    WORD wStremingTimePIO;                  // 104
    WORD wReserved4;                        // 105
    WORD wSectorSize;                       // 106
    WORD wInterSeekDelay;                   // 107
    WORD wIEEEOUI;                          // 108
    WORD wUniqueID3;                        // 109
    WORD wUniqueID2;                        // 110
    WORD wUniqueID1;                        // 111
    WORD wReserved5[4];                     // 112 - 115
    WORD wReserved6;                        // 116
    DWORD ulWordsPerLogicalSector;          // 117-118
    WORD wCommandSetSupported4;             // 119 (BIT2 - WRITE UNCORRECTABLE EXT is supported)
    WORD wCommandSetEnable4;                // 120 (BIT2 - WRITE UNCORRECTABLE EXT is supported)
    WORD wReserved7[6];                     // 119-126
    WORD wRemovableMediaStatus;             // 127
    WORD wSecurityStatus;                   // 128
    WORD CurrentSetFeature;                 // 129 (Hitachi 0bit:1=write cache, 1bit:1=read cache, 2bit:1=reverting enabled, 3bit:1=auto reassign enabled
    WORD pwVendorSpecific[30];              // 130 - 159
    WORD wCFAPowerMode1;                    // 160
    WORD wReserved8[8];                     // 161 - 175
    WORD wTrimSupport;                      // 169 if BIT0 is set then it is an SSD drive
    WORD wReserved9[6];                     // 170 - 175
    char sCurrentMediaSerialNo[60];         // 176 - 205
    WORD wSCTCommandTransport;              // 206
    WORD wCEATA1;                           // 207
    WORD wCEATA2;                           // 208
    WORD wAlignment;                        // 209
    WORD wReserved10[45];                   // 210 - 254
    WORD wIntegrityWord;                    // 255   0-7bit:Signature, 8-15bit:Checksum
} DRIVEINFO;

// custom helper struct to be expanded in the future
typedef struct {
    DWORD ahci_port;
    WORD bytes_per_sector;
    __int64 total_sectors;
    __int64 total_gb;
    char drive_model[256];
    char drive_serial[256];
    char drive_firmware[256];
} DISKDRIVE;
#pragma pack (pop);

// function prototypes
void QueryPerformanceFrequency (unsigned __int64 *freq);
void QueryPerformanceCounter (unsigned __int64 *count);

BOOL DPMI_SimulateRMI (BYTE IntNum, DPMIREGS *regs);
BOOL DPMI_DOSmalloc (DWORD size, WORD *segment, WORD *selector);
void DPMI_DOSfree (WORD *selector);
BOOL DPMI_MapMemory (unsigned long *physaddress, unsigned long *linaddress, unsigned long size);
BOOL DPMI_UnmapMemory (unsigned long *linaddress);

char *text_ConvertToString (char stringdata[256], int count);
char *text_CutSpacesAfter (char *str);
char *text_CutSpacesBefore (char *str);
