
/*
    Watcom C - Simplified AHCI interface for 32-bit protected mode Watcom C
    Written by Piotr Ulaszewski - December 2019
    Basing on all I have learned from various OS developement sources
*/

#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <i86.h>
#include <time.h>
#include <math.h>
#include "drives.h"
#include "ahci.h"


// ATA status and error registers
extern BYTE status_register;
extern BYTE device_register;
extern BYTE chigh_register;
extern BYTE clow_register;
extern BYTE sector_register;
extern BYTE count_register;
extern BYTE error_register;

extern BYTE lbahigh07_register;
extern BYTE lbahigh815_register;
extern BYTE lbamid07_register;
extern BYTE lbamid815_register;
extern BYTE lbalow07_register;
extern BYTE lbalow815_register;
extern BYTE count07_register;
extern BYTE count815_register;

//  Pci device structure to hold AHCI data - initialize to NULL
AHCI_PCI_DEV ahci_pci = {0};                  
WORD buff[256];

/*****************************************************************************
 * PCI BIOS helper access routines
 * Those C functions simply call the PCI BIOS interrupts with given parameters
 *****************************************************************************/

BYTE  pci_config_read_byte (int index);
WORD  pci_config_read_word (int index);
DWORD pci_config_read_dword (int index);
void  pci_config_write_byte (int index, BYTE data);
void  pci_config_write_word (int index, WORD data);
void  pci_config_write_dword (int index, DWORD data);
BOOL  pci_check_bios (void);
BOOL  pci_find_ahci_device ();
void  pci_enable_io_access ();
void  pci_enable_memory_access ();
void  pci_enable_busmaster ();

DWORD ahci_global_read_dword (unsigned int a);
WORD ahci_global_read_word (unsigned short a);
BYTE ahci_global_read_byte (unsigned char a);
void ahci_global_write_dword (unsigned int a, unsigned int d);
void ahci_global_write_word (unsigned short a, unsigned short d);
void ahci_global_write_byte (unsigned char a, unsigned char d);

DWORD ahci_port_read_dword (int port, unsigned int a);
WORD ahci_port_read_word (int port, unsigned short a);
BYTE ahci_port_read_byte (int port, unsigned char a);
void ahci_port_write_dword (int port, unsigned int a, unsigned int d);
void ahci_port_write_word (int port, unsigned short a, unsigned short d);
void ahci_port_write_byte (int port, unsigned char a, unsigned char d);

/********************************************************************
 *      PCI BIOS helper funtions
 ********************************************************************/

BYTE pci_config_read_byte (int index)
{
        union REGS r;

        memset(&r, 0, sizeof(r));
        r.x.eax = 0x0000B108;                
        r.x.ebx = (DWORD)ahci_pci.device_bus_number;
        r.x.edi = (DWORD)index;
        int386(0x1a, &r, &r);
        if (r.h.ah != 0) r.x.ecx = 0;
        return (BYTE)r.x.ecx;
}

WORD pci_config_read_word (int index)
{
        union REGS r;

        memset(&r, 0, sizeof(r));
        r.x.eax = 0x0000B109;
        r.x.ebx = (DWORD)ahci_pci.device_bus_number;
        r.x.edi = (DWORD)index;
        int386(0x1a, &r, &r);
        if (r.h.ah != 0 ) r.x.ecx = 0;
        return (WORD)r.x.ecx;
}

DWORD pci_config_read_dword (int index)
{
        union REGS r;

        memset(&r, 0, sizeof(r));
        r.x.eax = 0x0000B10A;
        r.x.ebx = (DWORD)ahci_pci.device_bus_number;
        r.x.edi = (DWORD)index;
        int386(0x1a, &r, &r);
        if (r.h.ah != 0 ) r.x.ecx = 0;
        return (DWORD)r.x.ecx;
}

void pci_config_write_byte (int index, BYTE data)
{
        union REGS r;

        memset(&r, 0, sizeof(r));
        r.x.eax = 0x0000B10B;
        r.x.ebx = (DWORD)ahci_pci.device_bus_number;
        r.x.ecx = (DWORD)data;
        r.x.edi = (DWORD)index;
        int386(0x1a, &r, &r);
        if (r.h.ah != 0 ){
            printf("Error : PCI write config byte failed\n");
        }
}

void pci_config_write_word (int index, WORD data)
{
        union REGS r;

        memset(&r, 0, sizeof(r));
        r.x.eax = 0x0000B10C;
        r.x.ebx = (DWORD)ahci_pci.device_bus_number;
        r.x.ecx = (DWORD)data;
        r.x.edi = (DWORD)index;
        int386(0x1a, &r, &r);
        if (r.h.ah != 0 ){
            printf("Error : PCI write config word failed\n");
        }
}

void pci_config_write_dword (int index, DWORD data)
{
        union REGS r;

        memset(&r, 0, sizeof(r));
        r.x.eax = 0x0000B10D;
        r.x.ebx = (DWORD)ahci_pci.device_bus_number;
        r.x.ecx = (DWORD)data;
        r.x.edi = (DWORD)index;
        int386(0x1a, &r, &r);
        if (r.h.ah != 0 ){
            printf("Error : PCI write config dword failed\n");
        }
}

BOOL pci_check_bios (void)
{
        union REGS r;

        memset(&r, 0, sizeof(r));
        r.x.eax = 0x0000B101;
        r.x.edi = 0x00000000;
        int386(0x1a, &r, &r);
        if (r.x.edx != 0x20494350) return FALSE;   // ' ICP' identifier found ?
        return TRUE;
}

BOOL pci_find_ahci_device ()
{
        // base class code (1), sub class code (6) and progamming interface (1) - (1,6,1).
        // AX = B103h
        // ECX = class code
        //    bits 31-24 unused
        //    bits 23-16 class
        //    bits 15-8  subclass
        //    bits 7-0   programming interface
        // SI = device index (0-n)
        // Return: CF clear if successful
        // CF set on error
        // AH = status (00h,86h)
        //    00h successful
        //        BH = bus number
        //        BL = device/function number (bits 7-3 device, bits 2-0 func)
        //    86h device not found

        union REGS r;

        memset(&r, 0, sizeof(r));
        r.x.eax = 0x0000B103;                      // PCI BIOS - find PCI class code
        r.x.ecx = 0x00010601;                      // (1, 6, 1)
        r.x.esi = 0x00000000;                      // device index
        int386(0x1a, &r, &r);
        if (r.h.ah != 0 ) return FALSE;            // device not found
        ahci_pci.device_bus_number = r.w.bx;       // save device & bus/funct number
        return TRUE;                               // device found
}

void pci_enable_io_access ()
{
        pci_config_write_word (PCI_COMMAND, pci_config_read_word(PCI_COMMAND) | BIT0);
}

void pci_enable_memory_access ()
{
        pci_config_write_word (PCI_COMMAND, pci_config_read_word(PCI_COMMAND) | BIT1);
}

void pci_enable_busmaster ()
{
        pci_config_write_word (PCI_COMMAND, pci_config_read_word(PCI_COMMAND) | BIT2);
}

/********************************************************************
 *      AHCI PCI HBA ACCESS functions (global control and ports)
 ********************************************************************/

DWORD ahci_global_read_dword (unsigned int a)
{
        return *(unsigned int *) (ahci_pci.base_ahci_linear + (a));
}

WORD ahci_global_read_word (unsigned short a)
{
        return *(unsigned short *) (ahci_pci.base_ahci_linear + (a));
}

BYTE ahci_global_read_byte (unsigned char a)
{
        return *(unsigned char *) (ahci_pci.base_ahci_linear + (a));
}

void ahci_global_write_dword (unsigned int a, unsigned int d)
{
        *(unsigned int *) (ahci_pci.base_ahci_linear + (a)) = d;
}

void ahci_global_write_word (unsigned short a, unsigned short d)
{
        *(unsigned short *) (ahci_pci.base_ahci_linear + (a)) = d;
}

void ahci_global_write_byte (unsigned char a, unsigned char d)
{
        *(unsigned char *) (ahci_pci.base_ahci_linear + (a)) = d;
}


DWORD ahci_port_read_dword (int port, unsigned int a)
{
        return *(unsigned int *) (ahci_pci.base_ahci_linear + (0x100) + (port * 0x80) + (a));
}

WORD ahci_port_read_word (int port, unsigned short a)
{
        return *(unsigned short *) (ahci_pci.base_ahci_linear + (0x100) + (port * 0x80) + (a));
}

BYTE ahci_port_read_byte (int port, unsigned char a)
{
        return *(unsigned char *) (ahci_pci.base_ahci_linear + (0x100) + (port * 0x80) + (a));
}

void ahci_port_write_dword (int port, unsigned int a, unsigned int d)
{
        *(unsigned int *) (ahci_pci.base_ahci_linear + (0x100) + (port * 0x80) + (a)) = d;
}

void ahci_port_write_word (int port, unsigned short a, unsigned short d)
{
        *(unsigned short *) (ahci_pci.base_ahci_linear + (0x100) + (port * 0x80) + (a)) = d;
}

void ahci_port_write_byte (int port, unsigned char a, unsigned char d)
{
        *(unsigned char *) (ahci_pci.base_ahci_linear + (0x100) + (port * 0x80) + (a)) = d;
}


/********************************************************************
 *      AHCI engine control specific functions
 ********************************************************************/

BOOL ahci_enable_ahci ()
{
        int i;
        DWORD tmp;

        // check if AHCI mode enabled already
        tmp = ahci_global_read_dword (AHCI_REG_GHC);
        if (tmp & AHCI_GHC_AE) return TRUE;
        
        // try 5 times with flush, set AE flag in Global HBA Control register
        for (i = 0; i < 5; i++) {
                tmp |= AHCI_GHC_AE;
                ahci_global_write_dword (AHCI_REG_GHC, tmp);
                tmp = ahci_global_read_dword (AHCI_REG_GHC);
                if (tmp & AHCI_GHC_AE) return TRUE;
                delay(10);
        }
        return FALSE;
}

void ahci_disable_ahci ()
{
        int i;
        DWORD tmp;

        // check if AHCI mode disabled already
        tmp = ahci_global_read_dword (AHCI_REG_GHC);
        if ((tmp & AHCI_GHC_AE) == 0) return;
        
        // try 5 times with flush, clear AE flag in Global HBA Control register
        for (i = 0; i < 5; i++) {
                // tmp &= ~AHCI_GHC_AE;
                tmp = 0;
                ahci_global_write_dword (AHCI_REG_GHC, tmp);
                tmp = ahci_global_read_dword (AHCI_REG_GHC);
                if ((tmp & AHCI_GHC_AE) == 0) return;
                delay(10);
        }
        return;
}

// return 2 if AHCI enabled, 1 if not
int ahci_test_state ()
{
        DWORD tmp;

        tmp = ahci_global_read_dword (AHCI_REG_GHC);
        if (tmp & AHCI_GHC_AE) return 2;
        return 1;
}

// return 1 if global interrupt flag enabled, 0 if disabled
BOOL ahci_test_global_interrupt_flag ()
{
        DWORD tmp;

        tmp = ahci_global_read_dword (AHCI_REG_GHC);
        if (tmp & AHCI_GHC_IR) return 1;
        return 0;
}

BOOL ahci_reset_controller ()
{
        DWORD tmp;
        
        // make sure AHCI mode is enabled
        if (ahci_enable_ahci() == FALSE) return FALSE;
        
        // global controller reset with flush
        tmp = ahci_global_read_dword (AHCI_REG_GHC);
        if ((tmp & AHCI_GHC_HR) == 0) {
                // when HR bit is set by software, an internal reset of the HBA is executed
                ahci_global_write_dword (AHCI_REG_GHC, tmp | AHCI_GHC_HR);
                tmp = ahci_global_read_dword (AHCI_REG_GHC);
                delay(1000);
                // HR bit has to reset within 1000ms
                if (ahci_global_read_dword (AHCI_REG_GHC) & AHCI_GHC_HR) return FALSE;
        }
        return TRUE;    
}

BOOL ahci_port_reset (int portnr)
{
        int i;
        DWORD val;
        
        // disable FIS + Command List - timeout 2000 mseconds, with 4 trys in 500ms intervals
        i = 0;
        while (i < 4) {
                // on second pass the first line will flush the write
                val = ahci_port_read_dword (portnr, AHCI_REG_PORT_CMD);
                // stop if FIS receive not running (FR) and command list not running (CR)
                // FIS Receive Enable is cleared (FRE) and start is cleared (ST)
                if ((val & (AHCI_REG_PORT_CMD_FRE | AHCI_REG_PORT_CMD_ST | AHCI_REG_PORT_CMD_FR | AHCI_REG_PORT_CMD_CR)) == 0) break;
                val &= ~(AHCI_REG_PORT_CMD_FRE | AHCI_REG_PORT_CMD_ST);
                ahci_port_write_dword (portnr, AHCI_REG_PORT_CMD, val);
                delay (500);
                i++;
        }
        if (i == 4) return FALSE;
        
        // disable + clear IRQs
        ahci_port_write_dword (portnr, AHCI_REG_PORT_IE, 0);
        val = ahci_port_read_dword (portnr, AHCI_REG_PORT_IS);
        if (val) ahci_port_write_dword (portnr, AHCI_REG_PORT_IS, val);
        
        return TRUE;
}

BOOL ahci_port_alloc (int portnr)
{
        DWORD size = 0;
        DWORD align = 0;
        DWORD offset = 0;
        
        if (portnr > 31) return FALSE;

        // allocate 1KB aligned on 1KB boundary for Command List Base Address
        size = 1024;
        align = 1024;
        offset = (DWORD) malloc (size + align);
        if (!offset) return FALSE;
        memset ((BYTE *)offset, 0, size + align);
        ahci_pci.ports[portnr].clb_free = (BYTE *)offset;
        offset += align;
        offset &= ~(align - 1);
        ahci_pci.ports[portnr].clb = offset;
        
        // allocate 1KB aligned on 1KB boundary for FIS Base Address
        size = 1024;
        align = 1024;
        offset = (DWORD) malloc (size + align);
        if (!offset) return FALSE;
        memset ((BYTE *)offset, 0, size + align);
        ahci_pci.ports[portnr].fb_free = (BYTE *)offset;
        offset += align;
        offset &= ~(align - 1); 
        ahci_pci.ports[portnr].fb = offset;

        // allocate 1KB aligned on 1KB boundary for Command Table
        size = 1024;
        align = 1024;
        offset = (DWORD) malloc (size + align);
        if (!offset) return FALSE;
        memset ((BYTE *)offset, 0, size + align);
        ahci_pci.ports[portnr].cmdtbl_free = (BYTE *)offset;
        offset += align;
        offset &= ~(align - 1); 
        ahci_pci.ports[portnr].cmdtbl = offset;

        // save old addresses
        ahci_pci.ports[portnr].old_clb = ahci_port_read_dword (portnr, AHCI_REG_PORT_CLB);
        ahci_pci.ports[portnr].old_fb = ahci_port_read_dword (portnr, AHCI_REG_PORT_FB);
        ahci_pci.ports[portnr].old_ie = ahci_port_read_dword (portnr, AHCI_REG_PORT_IE);
        ahci_pci.ports[portnr].old_cmd = ahci_port_read_dword (portnr, AHCI_REG_PORT_CMD);
        
        // write addresses to hardware
        ahci_port_write_dword (portnr, AHCI_REG_PORT_CLB, ahci_pci.ports[portnr].clb);
        ahci_port_write_dword (portnr, AHCI_REG_PORT_FB, ahci_pci.ports[portnr].fb);
        
        return TRUE;
}

void ahci_port_free (int portnr)
{
        ahci_port_reset (portnr);

        // restore previous values
        ahci_port_write_dword (portnr, AHCI_REG_PORT_CLB, ahci_pci.ports[portnr].old_clb);
        ahci_port_write_dword (portnr, AHCI_REG_PORT_FB, ahci_pci.ports[portnr].old_fb);
        ahci_port_write_dword (portnr, AHCI_REG_PORT_IE, ahci_pci.ports[portnr].old_ie);
        ahci_port_write_dword (portnr, AHCI_REG_PORT_CMD, ahci_pci.ports[portnr].old_cmd);

        // free memory
        free (ahci_pci.ports[portnr].clb_free);
        free (ahci_pci.ports[portnr].fb_free);
        free (ahci_pci.ports[portnr].cmdtbl_free);
}

BOOL ahci_port_setup (int portnr)
{
        int i;
        DWORD cmd, status, error, tfd;
        
        // enable FIS receive, set FRE bit in port command register
        cmd = ahci_port_read_dword (portnr, AHCI_REG_PORT_CMD);
        cmd |= AHCI_REG_PORT_CMD_FRE;
        ahci_port_write_dword (portnr, AHCI_REG_PORT_CMD, cmd);

        // spin - up the connected device, set SUD bit in port command register
        if ((cmd & AHCI_REG_PORT_CMD_SUD) == 0) {
                cmd |= AHCI_REG_PORT_CMD_SUD;
                ahci_port_write_dword (portnr, AHCI_REG_PORT_CMD, cmd);
                delay(1000);
        }

        // test AHCI link on selected port - timeout 20s
        i = 0;
        while (i < 200) {
                // Serial ATA Status (SCR0: SStatus)
                status = ahci_port_read_dword (portnr, AHCI_REG_PORT_SSTS);
                // test for : device presence detected and Phy communication established 0x03
                if ((status & 0x07) == 0x03) {
                        printf ("AHCI link open at port : %d\n", portnr);
                        break;
                }
                delay (100);
                i++;
        }
        if (i == 200) return FALSE;

        // clear error status
        error = ahci_port_read_dword (portnr, AHCI_REG_PORT_SERR);
        if (error) ahci_port_write_dword (portnr, AHCI_REG_PORT_SERR, error);

        // wait for device becoming ready - timeout 20s
        i = 0;
        while (i < 200) {
                // read port Task File Data
                tfd = ahci_port_read_dword (portnr, AHCI_REG_PORT_TFD);
                // test ATA flags
                if ((tfd & (ATA_BUSY | ATA_DRQ)) == 0) break;
                delay (100);
                i++;
        }
        if (i == 200) {
                printf ("AHCI device not ready at port : %d\n", portnr);
                return FALSE;
        }

        // start device - don't do it if command list is running (CR bit)
        cmd = ahci_port_read_dword(portnr, AHCI_REG_PORT_CMD);
        while (cmd & AHCI_REG_PORT_CMD_CR) {
                cmd = ahci_port_read_dword(portnr, AHCI_REG_PORT_CMD);
        }
        
        // FIS receive enable
        cmd |= AHCI_REG_PORT_CMD_FRE;
        ahci_port_write_dword(portnr, AHCI_REG_PORT_CMD, cmd);

        // strart to process the command list
        cmd = ahci_port_read_dword(portnr, AHCI_REG_PORT_CMD);
        cmd |= AHCI_REG_PORT_CMD_ST;
        ahci_port_write_dword(portnr, AHCI_REG_PORT_CMD, cmd);

        // flush port command and status register
        cmd = ahci_port_read_dword(portnr, AHCI_REG_PORT_CMD);

        return TRUE;
}

BOOL ahci_port_stop (int portnr)
{
        DWORD cmd;

        // stop device, when start bit cleared in port command register, the HBA may not process the command list
        cmd = ahci_port_read_dword (portnr, AHCI_REG_PORT_CMD);
        cmd &= ~(AHCI_REG_PORT_CMD_ST);
        ahci_port_write_dword (portnr, AHCI_REG_PORT_CMD, cmd);

        return TRUE;
}

int ahci_port_check_type (int portnr)
{
        DWORD val;

        if (portnr > 31) return FALSE;

        val = ahci_port_read_dword (portnr, AHCI_REG_PORT_SIG);

        switch (val) {

            case SATA_SIG_ATA:
                // SATA drive
                return 1;

            case SATA_SIG_ATAPI:
                // S-ATAPI drive
                return 2;

            case SATA_SIG_SEMB:
                // Enclosure Management bridge
                return 3;

            case SATA_SIG_PM:
                // Port multiplier
                return 4;

            default:
                return 0;
        }        
}

BOOL ahci_send_command_internal (int portnr, int command, int features, int count, int sector, int clow, int chigh, int device, int featuresh, int counth, int sectorh, int clowh, int chighh, int direction, BYTE *buffer, int length)
{
    DWORD intstatus, cmd, tfd, val;
    DWORD status, error;
    HBA_CMD_HEADER *cmd_hdr;
    HBA_CMD_TBL *cmd_tbl;
    FIS_REG_H2D *fis;
    HBA_FIS *hba_fis;

    // hires timer
    unsigned __int64 Frequency;
    unsigned __int64 WaitStart;
    unsigned __int64 WaitEnd;
    DWORD WaitTime;

    QueryPerformanceFrequency (&Frequency);                 // frequency of the high resolution timer - ticks for 1 ns

    // memory for command header is already allocated
    cmd_hdr = (HBA_CMD_HEADER *)ahci_pci.ports[portnr].clb; // command list base address (CLB) for given port
    cmd_hdr->cfl = sizeof(FIS_REG_H2D)/sizeof(DWORD);       // Command FIS legth in dwords, size = 5
    cmd_hdr->a = 0;                                         // 1 = ATAPI, 0 = SATA HDD/SSD
    if (direction == 2) cmd_hdr->w = 1;                     // write to device w = 1
    else cmd_hdr->w = 0;
    cmd_hdr->p = 0;                                         // Prefetchable 0 = no
    cmd_hdr->prdtl = 1;                                     // Physical region descriptor table length in entries
    cmd_hdr->prdbc = 0;                                     // Physical region descriptor byte count transferred - set to 0 on start
    cmd_hdr->ctba = ahci_pci.ports[portnr].cmdtbl;          // Command table base address

    // fill in command table - only 1 entry
    cmd_tbl = (HBA_CMD_TBL *)ahci_pci.ports[portnr].cmdtbl;
    cmd_tbl->prdt_entry[0].dba = (DWORD)buffer;                     // dba = data base address
    if (length < 0x200) cmd_tbl->prdt_entry[0].dbc = 0x200 - 1;     // dbc = byte count, 4M max
    else cmd_tbl->prdt_entry[0].dbc = length - 1;
    cmd_tbl->prdt_entry[0].i = 1;                                   // i = 1 interrupt on completion for which we will wait
    
    // prepare AHCI command FIS H2D
    fis = (FIS_REG_H2D *)ahci_pci.ports[portnr].cmdtbl;             // fill in FIS data in the command table entry
    fis->fis_type = FIS_TYPE_REG_H2D;                               // FIS type Host to Device
    fis->c = 1;                                                     // Write command register

    fis->command = (BYTE) command;                                  // ATA command
    fis->featurel = (BYTE) features;                                // Features
    fis->featureh = (BYTE) featuresh;
    fis->lba0 = (BYTE) sector;
    fis->lba1 = (BYTE) clow;                                        // SMART_CYL_LOW
    fis->lba2 = (BYTE) chigh;                                       // SMART_CYL_HI
    fis->device = device;                                           // always master device
    fis->lba3 = (BYTE) sectorh;
    fis->lba4 = (BYTE) clowh;
    fis->lba5 = (BYTE) chighh;
 
    fis->countl = (BYTE) count;
    fis->counth = (BYTE) counth;

    // adjust received FIS pointer
    hba_fis = (HBA_FIS *)ahci_pci.ports[portnr].fb;                 // FIS base address

    // ahci command execute
    intstatus = ahci_port_read_dword (portnr, AHCI_REG_PORT_IS);
    if (intstatus) ahci_port_write_dword (portnr, AHCI_REG_PORT_IS, intstatus);

    // command issue
    ahci_port_write_dword (portnr, AHCI_REG_PORT_CI, 1);

    QueryPerformanceCounter (&WaitStart);
    while (1) {
            intstatus = ahci_port_read_dword (portnr, AHCI_REG_PORT_IS);

            // we wait for a specific interrupt on completion of our task which was to send a H2D FIS
            if (intstatus) {
                    // clear port interrupt status
                    ahci_port_write_dword (portnr, AHCI_REG_PORT_IS, intstatus);
                    
                    // this is experimental - we could only handle DHRS and it would be ok
                    if (intstatus & AHCI_REG_PORT_IS_TFES) {
                            // Task File Error Status
                            status = ahci_port_read_dword (portnr, AHCI_REG_PORT_TFD) & 0xFF;
                            error = (ahci_port_read_dword (portnr, AHCI_REG_PORT_TFD) >> 8) & 0xFF;
                            break;
                    }
                    if (intstatus & AHCI_REG_PORT_IS_DHRS) {
                            // DHRS setup FIS - bit0 - set even on transfer error.
                            // status = hba_fis->rfis.status;
                            // error = hba_fis->rfis.error;
                            status = ahci_port_read_dword (portnr, AHCI_REG_PORT_TFD) & 0xFF;
                            error = (ahci_port_read_dword (portnr, AHCI_REG_PORT_TFD) >> 8) & 0xFF;
                            break;
                    }
                    if (intstatus & AHCI_REG_PORT_IS_PSS) {
                            // PSS - PIO setup FIS interrupt
                            // A PIO setup FIS has been received with the I bit set. It has been copied into system memory
                            // and the data related to that FIS has been transfered.
                            // status = hba_fis->psfis.status;
                            // error = hba_fis->psfis.error;
                            status = ahci_port_read_dword (portnr, AHCI_REG_PORT_TFD) & 0xFF;
                            error = (ahci_port_read_dword (portnr, AHCI_REG_PORT_TFD) >> 8) & 0xFF;
                            break;
                    }
            }
            QueryPerformanceCounter (&WaitEnd);
            WaitTime = (DWORD) ((unsigned __int64)(1000 * (WaitEnd - WaitStart)/Frequency));        // calculate wait time in ms
            if (WaitTime > TIMEOUT_DETECTION) {
                    printf("HBA : Device command request taking too long (timeout 20s)!\n");
                    status = 0xFF;
                    error = 0xFF;
                    break;
            }
    }

    // clear global interrupt bit in AHCI_REG_IS for the corresponding port
    ahci_global_write_dword (AHCI_REG_IS, 1 << portnr);

    // update global ATA status and error registers
    status_register = status;
    error_register = error;

    // status = hba_fis->rfis.status;
    // error = hba_fis->rfis.error;
    device_register = hba_fis->rfis.device;
    chigh_register = hba_fis->rfis.lba2;
    clow_register = hba_fis->rfis.lba1;
    sector_register = hba_fis->rfis.lba0;
    count_register = hba_fis->rfis.countl;

    lbahigh07_register = hba_fis->rfis.lba2;
    lbahigh815_register = hba_fis->rfis.lba5;
    lbamid07_register = hba_fis->rfis.lba1;
    lbamid815_register = hba_fis->rfis.lba4;
    lbalow07_register = hba_fis->rfis.lba0;
    lbalow815_register = hba_fis->rfis.lba3;
    count07_register = hba_fis->rfis.countl;
    count815_register = hba_fis->rfis.counth;

    // return true if success
    if (((status & (ATA_BUSY | ATA_DF | ATA_ERR)) == 0) && (status & ATA_DRDY)) return TRUE;

    // clear start port register
    cmd = ahci_port_read_dword (portnr, AHCI_REG_PORT_CMD);
    cmd &= ~(AHCI_REG_PORT_CMD_ST);
    ahci_port_write_dword (portnr, AHCI_REG_PORT_CMD, cmd);

    // wait for port CMD to clear to 0
    // timeout 10000ms
    QueryPerformanceCounter (&WaitStart);
    while (1) {
            cmd = ahci_port_read_dword (portnr, AHCI_REG_PORT_CMD);
            if ((cmd & AHCI_REG_PORT_CMD_CR) == 0) break;

            QueryPerformanceCounter (&WaitEnd);
            WaitTime = (DWORD) ((unsigned __int64)(1000 * (WaitEnd - WaitStart)/Frequency));        // calculate wait time in ms
            if (WaitTime > 10000) {
                    printf("HBA : Could not stop port engine after device error (timeout 10s)!\n");
                    return FALSE;
            }
    }

    // clear error bits in AHCI_REG_PORT_SERR to enable capturing of new errors
    val = ahci_port_read_dword (portnr, AHCI_REG_PORT_SERR);
    ahci_port_write_dword (portnr, AHCI_REG_PORT_SERR, val);

    // clear interrupt status bits
    intstatus = ahci_port_read_dword (portnr, AHCI_REG_PORT_IS);
    ahci_port_write_dword (portnr, AHCI_REG_PORT_IS, intstatus);

    // issue a COMRESET to the device to put it in an idle state if
    // BSY or DRQ ATA flags are set to 1
    tfd = ahci_port_read_dword (portnr, AHCI_REG_PORT_TFD);
    // test ATA flags
    if (tfd & (ATA_BUSY | ATA_DRQ)) {
            // issuing a COMRESET to stop the device - AHCI_REG_PORT_SCTL
            val = ahci_port_read_dword (portnr, AHCI_REG_PORT_SCTL);
            // set Device Detection Initialization to 1 for 1ms
            ahci_port_write_dword (portnr, AHCI_REG_PORT_SCTL, val | 1);
            delay(1);
            ahci_port_write_dword (portnr, AHCI_REG_PORT_SCTL, val);
    }

    // restart the port engine to reenable issuing new commands
    cmd = ahci_port_read_dword (portnr, AHCI_REG_PORT_CMD);
    cmd |= AHCI_REG_PORT_CMD_ST;
    ahci_port_write_dword (portnr, AHCI_REG_PORT_CMD, cmd);

    // return with error recovery completed
    return FALSE;
}

void ahci_cleanup (void)
{
    DWORD tmp;

    if (ahci_pci.base_ahci) {
        // test if AHCI was enabled on entry
        if (ahci_pci.initial_ahci_state == 2) {
            ahci_enable_ahci ();
            
            // info on ahci status
            printf (">>> BIOS IN AHCI MODE ON PROGRAM START! RESTORING INITIAL AHCI CONTROLLER STATE!\n");

            // enable global IE if it was enabled on program start
            if (ahci_pci.initial_ahci_interrupts) {
                tmp = ahci_global_read_dword (AHCI_REG_GHC);
                tmp |= AHCI_GHC_IR;
                ahci_global_write_dword (AHCI_REG_GHC, tmp);
            }
        }

        // if AHCI was disabled on entry
        if (ahci_pci.initial_ahci_state == 1) {
            // then disable AHCI
            ahci_disable_ahci ();
        }
    }

    // free DMPI address mapping (check if passed address is correct!)
    DPMI_UnmapMemory ((unsigned long *) &ahci_pci.base_ahci_linear);
}

/********************************************************************
 *      AHCI exported functions
 ********************************************************************/

BOOL ahci_detect_ahci (void)
{
    int i;
    int counter, temp;
    DPMIREGS dpmiregs;

    // detect controller - if true our structure is filled with device bus number
    if (pci_find_ahci_device() != TRUE) {
        printf ("Error : Could not find AHCI controller!\n");
        return FALSE;
    }

    // save device ids - execute PCI helper function which require device bus number 
    ahci_pci.vendor_id = pci_config_read_word (PCI_VENDOR_ID);
    ahci_pci.device_id = pci_config_read_word (PCI_DEVICE_ID);

    // read pci configuration
    ahci_pci.command = pci_config_read_word (PCI_COMMAND);
    ahci_pci.irq = pci_config_read_byte (PCI_INTERRUPT_LINE);
    ahci_pci.pin = pci_config_read_byte (PCI_INT_LINE);
    ahci_pci.base0 = pci_config_read_dword (PCI_BASE_ADDRESS_0);   // NAMBAR
    ahci_pci.base0 &= ~7;
    ahci_pci.base1 = pci_config_read_dword (PCI_BASE_ADDRESS_1);   // NABMBAR
    ahci_pci.base1 &= ~7;
    ahci_pci.base2 = pci_config_read_dword (PCI_BASE_ADDRESS_2);   // NABMBAR
    ahci_pci.base2 &= ~7;
    ahci_pci.base3 = pci_config_read_dword (PCI_BASE_ADDRESS_3);   // NABMBAR
    ahci_pci.base3 &= ~7;
    ahci_pci.base4 = pci_config_read_dword (PCI_BASE_ADDRESS_4);   // NABMBAR
    ahci_pci.base4 &= ~7;
    ahci_pci.base5 = pci_config_read_dword (PCI_BASE_ADDRESS_5);   // NABMBAR
    ahci_pci.base5 &= ~7;
    ahci_pci.base_ahci = pci_config_read_dword (PCI_AHCI_BASE_ADDRESS);   // NABMBAR
    ahci_pci.base_ahci &= ~7;

    // map linear address for AHCI base
    DPMI_MapMemory ((unsigned long *)&ahci_pci.base_ahci, (unsigned long *)&ahci_pci.base_ahci_linear, 0x1000);

    // continue to initialize the HBA - just some stuff for debug confirmation that we have AHCI device
    // check if device base class = 1 (storage controller) and sub class = 6 (SATA) and interface = 1 (AHCI)
    // non AHCI IDE interface base class = 1 (storage controller) and sub class = 1 (IDE) and interface = 0x80 (undefined)
    ahci_pci.bcc = pci_config_read_byte (PCI_BCC);
    ahci_pci.scc = pci_config_read_byte (PCI_SCC);
    ahci_pci.pi = pci_config_read_byte (PCI_PI);
    printf ("\n");
    printf ("Confirmation : PCI device Base Class Code is : %#0x\n", ahci_pci.bcc);
    printf ("Confirmation : PCI device Sub Class Code is : %#0x\n", ahci_pci.scc);
    printf ("Confirmation : PCI device Programming Interface is : %#0x\n", ahci_pci.pi);
    printf ("\n");

    if (ahci_pci.bcc != 1) {
        printf ("Error : Device base class code is not of storage controller type (0x01)\n");
        ahci_cleanup ();
        return FALSE;
    }
    if (ahci_pci.scc != 6) {
        printf ("Error : Device sub class code is not SATA (0x06)\n");
        ahci_cleanup ();
        return FALSE;
    }
    if (ahci_pci.pi != 1 ) {
        printf ("Error : Device programming interface is not AHCI (0x01)\n");
        ahci_cleanup ();
        return FALSE;
    }
    if (ahci_pci.base_ahci == 0) {
        printf ("Error: AHCI base BAR5 is not initialized by the BIOS!\n");
        ahci_cleanup ();
        return FALSE;
    }

    // test initial AHCI state
    ahci_pci.initial_ahci_state = ahci_test_state ();

    // default = AHCI global interrupt flag off = interrupts disabled
    ahci_pci.initial_ahci_interrupts = 0;
    if (ahci_pci.initial_ahci_state != 2) {
        // enable AHCI, reset controller if initial AHCI state is not 2 (AHCI enabled)
        ahci_enable_ahci ();
        ahci_reset_controller ();
        ahci_enable_ahci ();
    } else {
        ahci_pci.initial_ahci_interrupts = ahci_test_global_interrupt_flag ();
    }

    // clear the global interrupt status
    ahci_global_write_dword (AHCI_REG_IS, 0xffffffff); 

    // check total ports of the HBA
    ahci_pci.total_ports = ahci_global_read_dword (AHCI_REG_CAP);
    ahci_pci.total_ports &= BIT0 + BIT1 + BIT2 + BIT3 + BIT4;
    ahci_pci.total_ports++;

    // check available ports of the HBA - (PI) ports implemented
    ahci_pci.available_ports_bit = ahci_global_read_dword (AHCI_REG_PI);

    // convert bit values to number for ports info
    temp = ahci_pci.available_ports_bit;
    ahci_pci.available_ports = 0;
    while (temp) {
        if (temp & 1) ahci_pci.available_ports++;
        temp >>= 1;
    }
        
    // take first available port as active (just testing)
    temp = ahci_pci.available_ports_bit;
    ahci_pci.active_port = 0;
    ahci_pci.active_port_bit = 1;
    while (temp) {
        if (temp & 1) break;
        ahci_pci.active_port++;
        ahci_pci.active_port_bit <<= 1;
        temp >>= 1;
    }

    return TRUE;
}

void ahci_close_ahci (void)
{
    // restore GHC controller to initial state
    ahci_port_reset (ahci_pci.active_port);
    ahci_port_free (ahci_pci.active_port);

    // reset controller before exit just in case if not in AHCI mode on start
    if (ahci_pci.initial_ahci_state != 2) ahci_reset_controller (&ahci_pci);

    // perform cleanup
    ahci_cleanup ();
}

int ahci_detect_drives (DISKDRIVE *drive)
{
    int i, total_drives, temp, type;
    DRIVEINFO *driveinfo = (DRIVEINFO *)buff;

    // helper for display found drives
    char tmpstring[256] = {0};

    // AHCI - search SATA hard drives
    total_drives = 0;
    i = 0;
    while (i < 32) {

        temp = ahci_pci.available_ports_bit;
        
        if ((temp >> i) & 1) {
            ahci_pci.active_port = i;
            ahci_pci.active_port_bit = 1 << i;
            type = ahci_port_check_type (ahci_pci.active_port);

            if (type == 1) {
                // SATA drive

                ahci_port_alloc (ahci_pci.active_port);
                ahci_port_reset (ahci_pci.active_port);
                ahci_port_setup (ahci_pci.active_port);

                // allocate port and send identify command to SATA device on that port
                ahci_send_command_internal(ahci_pci.active_port, 0xec, 0, 0, 0, 0, 0, 0xa0, 0, 0, 0, 0, 0, 0, (BYTE *) driveinfo, 512);
                
                // stop port
                ahci_port_stop (ahci_pci.active_port);

                // calculate size with fixed 512 bytes per sector
                drive[total_drives].total_sectors = (__int64) (driveinfo->MaxUserLBA);
                drive[total_drives].bytes_per_sector = 512;
                drive[total_drives].total_gb = (__int64)ceil(((double)drive[total_drives].total_sectors * (double)drive[total_drives].bytes_per_sector) / (double)(1024.0 * 1024.0 * 1024.0));

                // get drive model
                strcpy(tmpstring, text_CutSpacesAfter (text_ConvertToString (driveinfo->sModelNumber, 40)));
                strcpy(drive[total_drives].drive_model, tmpstring);

                // get drive serial
                strcpy(tmpstring, text_CutSpacesBefore (text_ConvertToString (driveinfo->sSerialNumber, 20)));
                strcpy(drive[total_drives].drive_serial, tmpstring);

                // get drive firmware
                strcpy(tmpstring, text_CutSpacesAfter (text_ConvertToString (driveinfo->sFirmwareRev, 8)));
                strcpy(drive[total_drives].drive_firmware, tmpstring);

                // save port nr
                drive[total_drives].ahci_port = ahci_pci.active_port;

                // free the allocated port
                ahci_port_free (ahci_pci.active_port);

                total_drives++;
            } // type = SATA

        } // available port
        i++;
        if (total_drives == 32) break;
    } // all ports

    return total_drives;
}

BOOL ahci_send_command (BYTE command, BYTE features, BYTE count, BYTE direction, DISKDRIVE *sdrive, BYTE *buffer)
{
    BOOL result = FALSE;

    // send random command with return buffer
    // check if SMART enable/disable command
    if (command == SMART_CMD) {
        result = ahci_send_command_internal (sdrive->ahci_port, command, features, count, 0, (BYTE) SMART_CYL_LOW, (BYTE) SMART_CYL_HI, 0xa0, 0, 0, 0, 0, 0, direction, (BYTE *)buffer, direction > 0 ? 512 : 0);
    } else {
        result = ahci_send_command_internal (sdrive->ahci_port, command, features, count, 0, 0, 0, 0xa0, 0, 0, 0, 0, 0, direction, (BYTE *)buffer, direction > 0 ? 512 : 0);
    }
    return result;
}

BOOL ahci_send_command_extended (BYTE command, BYTE features, BYTE count, BYTE sector, BYTE clow, BYTE chigh, BYTE device, BYTE direction, DISKDRIVE *sdrive, BYTE *buffer, int length)
{
    BOOL result = FALSE;

    // send random command with return buffer
    result = ahci_send_command_internal (sdrive->ahci_port, command, features, count, sector, clow, chigh, device, 0, 0, 0, 0, 0, direction, (BYTE *)buffer, length);

    return result;
}

BOOL ahci_send_command_extended_48bit (BYTE command, BYTE features, BYTE count, BYTE sector, BYTE clow, BYTE chigh, BYTE device, BYTE featuresh, BYTE counth, BYTE sectorh, BYTE clowh, BYTE chighh, BYTE direction, DISKDRIVE *sdrive, BYTE *buffer, int length)
{
    BOOL result = FALSE;
    
    // send random 48-bit command with return buffer
    result = ahci_send_command_internal (sdrive->ahci_port, command, features, count, sector, clow, chigh, device, featuresh, counth, sectorh, clowh, chighh, direction, (BYTE *)buffer, length);
    
    return result;
}
