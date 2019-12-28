
/*
    Comments, defines and explanation taken straight from Intel "Serial ATA Advanced Host Controller Interface"
    FIS setup data structures D2H, H2D, DATA, PIO setup and DMA setup structures are based on "SATA 1.0 specification"
    AHCI specification revision 0.95
    Assembled toogether by Piotr Ulaszewski - December 2019
*/

// timeout for commands
#define TIMEOUT 20000


/*****************************************************************************
 * General PCI static defines
 ******************************************************************************/

#define PCI_CLASS_AHCI              0x0601
#define PCI_CLASS_STORAGE_AHCI      0x010601
 
#define PCI_VENDOR_ID               0x00
#define PCI_DEVICE_ID               0x02
#define PCI_COMMAND                 0x04
#define PCI_STATUS                  0x06
#define PCI_REVISION_ID             0x08
#define PCI_PI                      0x09   // PCI programming interface (AHCI = 1)
#define PCI_SCC                     0x0a   // PCI sub class code (SATA = 6)
#define PCI_BCC                     0x0b   // PCI base class code (storage controller = 1)
#define PCI_CACHE_LINE_SIZE         0x0c
#define PCI_MASTER_LATENCY_TIMER    0x0d
#define PCI_HEADER_TYPE             0x0e
#define PCI_BUILT_IN_SELF_TEST      0x0f

#define PCI_MEM_BASE_ADDRESS_0      0x10
#define PCI_MEM_BASE_ADDRESS_1      0x14
#define PCI_MEM_BASE_ADDRESS_2      0x18
#define PCI_MEM_BASE_ADDRESS_3      0x1c
#define PCI_MEM_BASE_ADDRESS_4      0x20
#define PCI_MEM_BASE_ADDRESS_5      0x24

#define PCI_BASE_ADDRESS_0          0x10
#define PCI_BASE_ADDRESS_1          0x14
#define PCI_BASE_ADDRESS_2          0x18
#define PCI_BASE_ADDRESS_3          0x1c
#define PCI_BASE_ADDRESS_4          0x20
#define PCI_BASE_ADDRESS_5          0x24

#define PCI_AHCI_BASE_ADDRESS       0x24

#define PCI_SUBSYSTEM_VENDOR_ID     0x2c
#define PCI_SUBSYSTEM_ID            0x2e
#define PCI_EXPANSION_ROM_ADDRESS   0x30
#define PCI_CAPABILITIES_POINTER    0x34
#define PCI_INTERRUPT_LINE          0x3c
#define PCI_INT_LINE                0x3d
#define PCI_MIN_GRANT               0x3e
#define PCI_MAX_LATENCY             0x3f

// PCI Command register bits

#define PCI_COMMAND_IO              0x01
#define PCI_COMMAND_MEMORY          0x02
#define PCI_COMMAND_MASTER          0x04
#define PCI_COMMAND_PARITY          0x40
#define PCI_COMMAND_SERR            0x100


/*****************************************************************************
 * AHCI Global register set
 ******************************************************************************/

#define AHCI_HBA_SIZE               0x100

#define AHCI_REG_CAP                0x00   // This register indicates basic capabilities of the HBA to driver software
#define AHCI_REG_GHC                0x04   // Global HBA Control

#define AHCI_GHC_AE BIT31                  // When set, software shall only communicate with the HBA using AHCI
#define AHCI_GHC_IR BIT1                   // This global bit enables interrupts from the HBA
#define AHCI_GHC_HR BIT0                   // When set by SW, this bit causes an internal reset of the HBA

#define AHCI_REG_IS                 0x08   // Interrupt Status Register
#define AHCI_REG_PI                 0x0c   // Ports Implemented - Intel only uses 6 ports
#define AHCI_REG_VS                 0x10   // AHCI Version, 0000-0905h indicates 0.95, 0001-0300h indicates 1.30
#define AHCI_CCC_CTL                0x14   // Command Completion Coalescing Control
#define AHCI_CCC_PORTS              0x18   // Command Completion Coalescing Ports
#define AHCI_EM_LOC                 0x1c   // Enclosure Management Location
#define AHCI_EM_CTL                 0x20   // Enclosure Management Control
#define AHCI_CAP2                   0x24   // HBA Capabilities Extended
#define AHCI_BOHC                   0x28   // BIOS/OS Handoff Control and Status

// Registers at offset A0h to FFh are vendor specific
// Port register set, Port offset = 0x100 + (PI Asserted Bit Position * 0x80)

#define AHCI_PORT_SIZE              0x80

#define AHCI_REG_PORT_CLB           0x00   // Port x Command List Base Address
#define AHCI_REG_PORT_CLBU          0x04   // Port x Command List Base Address Upper 32-Bits
#define AHCI_REG_PORT_FB            0x08   // Port x FIS Base Address
#define AHCI_REG_PORT_FBU           0x0c   // Port x FIS Base Address Upper 32-Bits
#define AHCI_REG_PORT_IS            0x10   // Port x Interrupt Status

#define AHCI_REG_PORT_IS_DHRS       BIT0   // Device to Host Register FIS Interrupt
#define AHCI_REG_PORT_IS_PSS        BIT1   // PIO Setup FIS Interrupt
#define AHCI_REG_PORT_IS_DSS        BIT2   // DMA Setup FIS Interrupt
#define AHCI_REG_PORT_IS_SDBS       BIT3   // Set Device Bits Interrupt
#define AHCI_REG_PORT_IS_US         BIT4   // Unknown FIS Interrupt
#define AHCI_REG_PORT_IS_DPS        BIT5   // Descriptor Processed
#define AHCI_REG_PORT_IS_IPMS       BIT23  // HBA received a FIS from a device whose Port Multiplier field did not match what was expected
#define AHCI_REG_PORT_IS_OFS        BIT24  // HBA received more bytes from a device than was specified in the PRD table for the command
#define AHCI_REG_PORT_IS_INFS       BIT26  // HBA encountered an error on the SATA interface but was able to continue operation
#define AHCI_REG_PORT_IS_IFS        BIT27  // Interface Fatal Error Status - HBA encountered an error on the SATA interface which caused the transfer to stop
#define AHCI_REG_PORT_IS_HBDS       BIT28  // Host Bus Data Error Status - HBA encountered an data error when reading from or writing to system memory
#define AHCI_REG_PORT_IS_HBFS       BIT29  // Host Bus Fatal Error Status - HBA encountered a host bus error that it cannot recover from, such as a bad software pointer
#define AHCI_REG_PORT_IS_TFES       BIT30  // Task File Error Status - set when the status register is updated by the device and the error bit0 is set
#define AHCI_REG_PORT_IS_CPDS       BIT31  // When set, a device status has changed as detected by the cold presence detect logic

#define AHCI_REG_PORT_IE            0x14   // Port x Interrupt Enable
#define AHCI_REG_PORT_CMD           0x18   // Port x Command and Status

#define AHCI_REG_PORT_CMD_ST        BIT0   // Start - When set, the HBA may process the command list. When cleared, the HBA may not process the command list.
#define AHCI_REG_PORT_CMD_SUD       BIT1   // SpinUp - Spin up device
#define AHCI_REG_PORT_CMD_POD       BIT2   // PowerUp - Power up device
#define AHCI_REG_PORT_CMD_FRE       BIT4   // FIS Receive Enable - When set, the HBA may post received FISes into the FIS receive area pointed to by PxFB.
#define AHCI_REG_PORT_CMD_FR        BIT14  // FIS Receive Running - When set, the FIS Receive DMA engine for the port is running.
#define AHCI_REG_PORT_CMD_CR        BIT15  // Command List Running - When this bit is set, the command list DMA engine for the port is running.

#define AHCI_REG_PORT_TFD           0x20   // Port x Task File Data
#define AHCI_REG_PORT_SIG           0x24   // Port x Signature
#define AHCI_REG_PORT_SSTS          0x28   // Port x Serial ATA Status (SCR0: SStatus)
#define AHCI_REG_PORT_SCTL          0x2c   // Port x Serial ATA Control (SCR2: SControl)
#define AHCI_REG_PORT_SERR          0x30   // Port x Serial ATA Error (SCR1: SError)
#define AHCI_REG_PORT_SACT          0x34   // Port x Serial ATA Active (SCR3: SActive)
#define AHCI_REG_PORT_CI            0x38   // Port x Command Issue
#define AHCI_REG_PORT_SNTF          0x3f   // Port x Serial ATA Notification (SCR4: SNotification)
#define AHCI_REG_PORT_RMCS          0x40   // Port x Raw FIS Mode Control and Status

// offsets 70h-7Fh AHCI_REG_PORT_VS - Port x Vendor Specific
// bits in ATA command block registers

#define ATA_BUSY                    BIT7     // BSY status bit
#define ATA_DRDY                    BIT6     // device ready
#define ATA_DF                      BIT5     // device fault
#define ATA_DRQ                     BIT3     // data request i/o
#define ATA_ERR                     BIT0     // have an error
#define ATA_SRST                    BIT2     // software reset
#define ATA_ABORTED                 BIT2     // command aborted

// device type attched to port
#define SATA_SIG_ATA                0x00000101      // SATA drive (1)
#define SATA_SIG_ATAPI              0xEB140101      // S-ATAPI drive (2)
#define SATA_SIG_SEMB               0xC33C0101      // Enclosure management bridge (3)
#define SATA_SIG_PM                 0x96690101      // Port multiplier (4)

// ATA commands
#define ATA_CMD_READ_VERIFY         0x40
#define ATA_CMD_READ_VERIFY_EX      0x42
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_READ_DMA_EX         0x25
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_WRITE_DMA_EX        0x35
#define ATA_CMD_READ                0x20        // 28-bit read
#define ATA_CMD_READ_NR             0x21        // 28-bit read without retry
#define ATA_CMD_READ_EX             0x24        // 48-bit extended read
#define ATA_CMD_WRITE               0x30        // 48-bit extended write
#define ATA_CMD_WRITE_EX            0x34        // 48-bit extended write
#define ATA_CMD_SET_FEATURES        0xEF        // set features function
#define ATA_CMD_CACHE_FLUSH         0xE7
#define ATA_CMD_CACHE_FLUSH_EXT     0xEA
#define ATA_CMD_IDENTIFY            0xEC
#define SMART_CMD                   0xB0 
#define SMART_CYL_LOW               0x4F
#define SMART_CYL_HI                0xC2 


/*****************************************************************************
 * HBA MEMORY AREA - GLOBAL AND PORTS
 ******************************************************************************/ 

#pragma pack(push,1)
typedef volatile struct
{
        DWORD   clb;            // 0x00, command list base address, 1K-byte aligned
        DWORD   clbu;           // 0x04, command list base address upper 32 bits
        DWORD   fb;             // 0x08, FIS base address, 256-byte aligned
        DWORD   fbu;            // 0x0C, FIS base address upper 32 bits
        DWORD   is;             // 0x10, interrupt status
        DWORD   ie;             // 0x14, interrupt enable
        DWORD   cmd;            // 0x18, command and status
        DWORD   res0;           // 0x1C, Reserved
        DWORD   tfd;            // 0x20, task file data
        DWORD   sig;            // 0x24, signature
        DWORD   ssts;           // 0x28, SATA status (SCR0:SStatus)
        DWORD   sctl;           // 0x2C, SATA control (SCR2:SControl)
        DWORD   serr;           // 0x30, SATA error (SCR1:SError)
        DWORD   sact;           // 0x34, SATA active (SCR3:SActive)
        DWORD   ci;             // 0x38, command issue
        DWORD   sntf;           // 0x3C, SATA notification (SCR4:SNotification)
        DWORD   rmcs;           // 0x40, FIS Mode Control and Status
        DWORD   res1[11];       // 0x44 ~ 0x6F, Reserved
        DWORD   vendor[4];      // 0x70 ~ 0x7F, vendor specific

        // extra data not present in the memory structure, implemented for the engine
        DWORD   cmdtbl;         // command table
        BYTE *  clb_free;       // original start of memory allocated for clb (without alignment)
        BYTE *  fb_free;        // original start of memory allocated for fb (without alignment)
        BYTE *  cmdtbl_free;    // original start of memory allocated for cmdtbl (without alignment)
        DWORD   old_clb;        // restore CLB for BIOS
        DWORD   old_fb;         // restore FB for BIOS
        DWORD   old_ie;         // restore IE for BIOS
        DWORD   old_cmd;        // restore CMD for BIOS        
} HBA_PORT;

typedef volatile struct
{
        // 0x00 - 0x2B, Generic Host Control
        DWORD   cap;            // 0x00, Host capability
        DWORD   ghc;            // 0x04, Global host control
        DWORD   is;             // 0x08, Interrupt status
        DWORD   pi;             // 0x0C, Port implemented
        DWORD   vs;             // 0x10, Version
        DWORD   ccc_control;    // 0x14, Command completion coalescing control
        DWORD   ccc_ports;      // 0x18, Command completion coalescing ports
        DWORD   em_location;    // 0x1C, Enclosure management location
        DWORD   em_control;     // 0x20, Enclosure management control
        DWORD   cap2;           // 0x24, Host capabilities extended
        DWORD   bohcs;          // 0x28, BIOS/OS handoff control and status
 
        // 0x2C - 0x9F, Reserved
        BYTE    res[0x74];      // 0xA0 - 0x2C = 0x74
 
        // 0xA0 - 0xFF, Vendor specific registers
        BYTE    vendor[0x60];   // 0x100 - 0xA0 = 0x60
 
        // 0x100 - 0x10FF, Port control registers
        HBA_PORT ports[1];      // 1 ~ 32
} HBA_GLOBAL;


/*****************************************************************************
 * FIS SETUP DATA STRUCTURES AS DEFINED IN SATA 1.0 SPECIFCATION
 ******************************************************************************/ 

typedef enum
{ 
        FIS_TYPE_REG_H2D = 0x27,        // Register FIS - host to device
        FIS_TYPE_REG_D2H = 0x34,        // Register FIS - device to host
        FIS_TYPE_DMA_ACT = 0x39,        // DMA activate FIS - device to host
        FIS_TYPE_DMA_SETUP = 0x41,      // DMA setup FIS - bidirectional
        FIS_TYPE_DATA = 0x46,           // Data FIS - bidirectional 
        FIS_TYPE_BIST = 0x58,           // BIST activate FIS - bidirectional
        FIS_TYPE_PIO_SETUP = 0x5F,      // PIO setup FIS - device to host 
        FIS_TYPE_DEV_BITS = 0xA1,       // Set device bits FIS - device to host
} FIS_TYPE;
 
 typedef struct
{
        // DWORD 0
        BYTE    fis_type;    // FIS_TYPE_REG_H2D
 
        BYTE    pmport:4;    // Port multiplier
        BYTE    res0:3;      // Reserved
        BYTE    c:1;         // 1: Command, 0: Control
 
        BYTE    command;     // Command register
        BYTE    featurel;    // Feature register, 7:0
 
        // DWORD 1
        BYTE    lba0;        // LBA low register, 7:0 (sector number)
        BYTE    lba1;        // LBA mid register, 15:8 (cyl low)
        BYTE    lba2;        // LBA high register, 23:16 (cyl high)
        BYTE    device;      // Device register (dev/head)
 
        // DWORD 2
        BYTE    lba3;        // LBA register, 31:24 (sector number high)
        BYTE    lba4;        // LBA register, 39:32 (cyl low high)
        BYTE    lba5;        // LBA register, 47:40 (cyl high high)
        BYTE    featureh;    // Feature register, 15:8 (features high)
 
        // DWORD 3
        BYTE    countl;      // Count register, 7:0
        BYTE    counth;      // Count register, 15:8
        BYTE    icc;         // Isochronous command completion
        BYTE    control;     // Control register
 
        // DWORD 4
        BYTE    res1[4];     // Reserved
} FIS_REG_H2D;
 
 typedef struct
{
        // DWORD 0
        BYTE    fis_type;    // FIS_TYPE_REG_D2H
 
        BYTE    pmport:4;    // Port multiplier
        BYTE    res0:2;      // Reserved
        BYTE    i:1;         // Interrupt bit
        BYTE    res1:1;      // Reserved
 
        BYTE    status;      // Status register
        BYTE    error;       // Error register
 
        // DWORD 1
        BYTE    lba0;        // LBA low register, 7:0 (sector number)
        BYTE    lba1;        // LBA mid register, 15:8 (cyl low)
        BYTE    lba2;        // LBA high register, 23:16 (cyl high)
        BYTE    device;      // Device register (dev/head)
 
        // DWORD 2
        BYTE    lba3;        // LBA register, 31:24 (sector number high)
        BYTE    lba4;        // LBA register, 39:32 (cyl low high)
        BYTE    lba5;        // LBA register, 47:40 (cyl high high)
        BYTE    res2;        // Reserved
 
        // DWORD 3
        BYTE    countl;      // Count register, 7:0
        BYTE    counth;      // Count register, 15:8
        BYTE    res3[2];     // Reserved
 
        // DWORD 4
        BYTE    res4[4];     // Reserved
} FIS_REG_D2H;
 
 typedef struct
{
        // DWORD 0
        BYTE    fis_type;       // FIS_TYPE_DATA
 
        BYTE    pmport:4;       // Port multiplier
        BYTE    res0:4;         // Reserved
 
        BYTE    res1[2];        // Reserved
 
        // DWORD 1 ~ N
        DWORD   data[1];        // Payload
} FIS_DATA;
 
 typedef struct
{
        // DWORD 0
        BYTE    fis_type;       // FIS_TYPE_PIO_SETUP
 
        BYTE    pmport:4;       // Port multiplier
        BYTE    res0:1;         // Reserved
        BYTE    d:1;            // Data transfer direction, 1 - device to host
        BYTE    i:1;            // Interrupt bit
        BYTE    res1:1;
 
        BYTE    status;         // Status register
        BYTE    error;          // Error register
 
        // DWORD 1
        BYTE    lba0;           // LBA low register, 7:0
        BYTE    lba1;           // LBA mid register, 15:8
        BYTE    lba2;           // LBA high register, 23:16
        BYTE    device;         // Device register
 
        // DWORD 2
        BYTE    lba3;           // LBA register, 31:24
        BYTE    lba4;           // LBA register, 39:32
        BYTE    lba5;           // LBA register, 47:40
        BYTE    res2;           // Reserved
 
        // DWORD 3
        BYTE    countl;         // Count register, 7:0
        BYTE    counth;         // Count register, 15:8
        BYTE    res3;           // Reserved
        BYTE    e_status;       // New value of status register
 
        // DWORD 4
        WORD    tc;             // Transfer count
        BYTE    res4[2];        // Reserved
} FIS_PIO_SETUP;
 
 typedef struct
{
        // DWORD 0
        BYTE    fis_type;       // FIS_TYPE_DMA_SETUP
 
        BYTE    pmport:4;       // Port multiplier
        BYTE    res0:1;         // Reserved
        BYTE    d:1;            // Data transfer direction, 1 - device to host
        BYTE    i:1;            // Interrupt bit
        BYTE    a:1;            // Auto-activate. Specifies if DMA Activate FIS is needed
 
        BYTE    res[2];         // Reserved
 
        //DWORD 1&2
 
        QWORD   DMAbufferID;    // DMA Buffer Identifier. Used to Identify DMA buffer in host memory. SATA Spec says host specific and not in Spec. Trying AHCI spec might work.
 
        //DWORD 3
        DWORD   res1;           //More reserved
 
        //DWORD 4
        DWORD   DMAbufOffset;   //Byte offset into buffer. First 2 bits must be 0
 
        //DWORD 5
        DWORD   TransferCount;  //Number of bytes to transfer. Bit 0 must be 0
 
        //DWORD 6
        DWORD   res2;          //Reserved
 
} FIS_DMA_SETUP;
 
 typedef volatile DWORD FIS_DEV_BITS;
 
 typedef volatile struct
{
        // 0x00
        FIS_DMA_SETUP   dsfis;          // DMA Setup FIS
        BYTE            pad0[4];
 
        // 0x20
        FIS_PIO_SETUP   psfis;          // PIO Setup FIS
        BYTE            pad1[12];
 
        // 0x40
        FIS_REG_D2H     rfis;           // Register – Device to Host FIS
        BYTE            pad2[4];
 
        // 0x58
        FIS_DEV_BITS    sdbfis;         // Set Device Bit FIS
 
        // 0x60
        BYTE            ufis[64];
 
        // 0xA0
        BYTE            res[0x100-0xA0];
} HBA_FIS;


/*****************************************************************************
 * FIS COMMAND LIST
 ******************************************************************************/ 

typedef struct
{
        // DW0
        BYTE    cfl:5;          // bit 0-4  Command FIS length in DWORDS, 2 ~ 16                - 05 (ATA command)
        BYTE    a:1;            // bit 5     ATAPI                                              - 00
        BYTE    w:1;            // bit 6     Write, 1: H2D, 0: D2H                              - 01 (H2D)
        BYTE    p:1;            // bit 7     Prefetchable                                       - 00
 
        BYTE    r:1;            // bit 8     Reset                                              - 00
        BYTE    b:1;            // bit 9     BIST                                               - 00
        BYTE    c:1;            // bit 10    Clear busy upon R_OK                               - 00
        BYTE    res0:1;         // bit 11    Reserved                                           - 00
        BYTE    pmp:4;          // bit 12-15 Port multiplier port                               - 00 (not attached)
 
        WORD    prdtl;          // Physical region descriptor table length in entries - 1 entry length (each entry is 4 dwords)
 
        // DW1
        volatile
        DWORD   prdbc;          // Physical region descriptor byte count transferred - 0 bytes transfered
 
        // DW2, 3
        DWORD   ctba;           // Command table descriptor base address
        DWORD   ctbau;          // Command table descriptor base address upper 32 bits
 
        // DW4 - 7
        DWORD   res1[4];        // Reserved
} HBA_CMD_HEADER;

typedef struct
{
        DWORD   dba;            // Data base address
        DWORD   dbau;           // Data base address upper 32 bits
        DWORD   res0;           // Reserved
 
        // DW3
        DWORD   dbc:22;         // Byte count, 4M max
        DWORD   res1:9;         // Reserved
        DWORD   i:1;            // Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct
{
        // 0x00
        BYTE    cfis[64];       // Command FIS
 
        // 0x40
        BYTE    acmd[16];       // ATAPI command, 12 or 16 bytes
 
        // 0x50
        BYTE    res[48];        // Reserved
 
        // 0x80
        HBA_PRDT_ENTRY  prdt_entry[32]; // Physical region descriptor table entries, 0 ~ 65535
} HBA_CMD_TBL;
#pragma pack(pop)


/*****************************************************************************
 * Internal header file: Intel AHCI and compatibles.
 ******************************************************************************/

#pragma pack(push,1)
 typedef struct {
        // PCI device IDs
        WORD vendor_id;
        WORD device_id;
        WORD sub_vendor_id;
        WORD sub_device_id;
        WORD device_bus_number;
        BYTE irq;                       // PCI IRQ
        BYTE pin;                       // PCI IRQ PIN
        WORD command;                   // PCI command reg
        BYTE bcc;                       // PCI base class code
        BYTE scc;                       // PCI sub class code
        BYTE pi;                        // PCI programming interface
        BYTE reserved1;                 // reserved to stay DWORD aligned for base addresses
        BYTE reserved2;
        BYTE reserved3;
        DWORD base0;                    // PCI BAR for mixer registers NAMBAR_REG
        DWORD base1;                    // PCI BAR for bus master registers NABMBAR_REG
        DWORD base2;
        DWORD base3;
        DWORD base4;
        DWORD base5;                    // AHCI base register
        DWORD base_ahci;                // AHCI base register
        DWORD base_ahci_linear;         // AHCI base register after linar address mapping
        DWORD device_type;              // any

        // global host control initial state
        int initial_ahci_state;         // 0 = unknown (value not initialized), 1 = disabled, 2 = enabled
        int initial_ahci_interrupts;    // 0 = disabled, 1 = enabled
                
        // port specific data
        DWORD total_ports;              // total number of ports available to the HBA (from CAP.NP)
        DWORD available_ports;          // PI - in numbers
        DWORD available_ports_bit;      // PI - in bits
        DWORD active_port;              // currently active port
        DWORD active_port_bit;          // currently active port bit (port0 = bit0, port1 = bit1, etc.)
        HBA_PORT ports[32];             // array of HBA  ports
} AHCI_PCI_DEV;
#pragma pack(pop)

// function prototypes
BOOL ahci_detect_ahci (void);
void ahci_close_ahci (void);
int ahci_detect_drives (DISKDRIVE *drive);
BOOL ahci_send_command (BYTE command, BYTE features, BYTE count, BYTE direction, DISKDRIVE *sdrive, BYTE *buffer);
BOOL ahci_send_command_extended (BYTE command, BYTE features, BYTE count, BYTE sector, BYTE clow, BYTE chigh, BYTE device, BYTE direction, DISKDRIVE *sdrive, BYTE *buffer, int length);
BOOL ahci_send_command_extended_48bit (BYTE command, BYTE features, BYTE count, BYTE sector, BYTE clow, BYTE chigh, BYTE device, BYTE featuresh, BYTE counth, BYTE sectorh, BYTE clowh, BYTE chighh, BYTE direction, DISKDRIVE *sdrive, BYTE *buffer, int length);
