
/*
    Watcom C - Simple hard drives detection for 32-bit protected mode using AHCI under a DOS extender
    Written by Piotr Ulaszewski (pulaweb) on the 27th of December 2019
    This is just a demonstartion on how to obtain hard drive info on any AHCI system
*/

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <i86.h>
#include <math.h>
#include "drives.h"
#include "ahci.h"


// frequency of the high resolution timer
unsigned __int64 calculated_frequency = 0;

// ATA status and error registers
BYTE status_register = 0;
BYTE device_register = 0;
BYTE chigh_register = 0;
BYTE clow_register = 0;
BYTE sector_register = 0;
BYTE count_register = 0;
BYTE error_register = 0;

BYTE lbahigh07_register = 0;
BYTE lbahigh815_register = 0;
BYTE lbamid07_register = 0;
BYTE lbamid815_register = 0;
BYTE lbalow07_register = 0;
BYTE lbalow815_register = 0;
BYTE count07_register = 0;
BYTE count815_register = 0;

// detected drives array
DISKDRIVE diskdrive[128];

// temporary string buffer
char tmpstring[1024];


int main (void) {

    int i;
    BOOL status = FALSE;
    unsigned __int64 calculated_frequency_start = 0;
    unsigned __int64 calculated_frequency_stop = 0;
    int total_ahci_drives = 0;

    // calculate frequency in cycles for 1ns of pentium timer
    QueryPerformanceCounter(&calculated_frequency_start);
    delay(1000);
    QueryPerformanceCounter(&calculated_frequency_stop);
    calculated_frequency = (calculated_frequency_stop - calculated_frequency_start);

    // detect AHCI
    if (ahci_detect_ahci() == FALSE) {
        printf("AHCI controller not detected error!\n");
        exit(0);
    }
    
    total_ahci_drives = ahci_detect_drives (diskdrive);
    printf("\n");

    // walk through all detected drives and display info
    for (i = 0; i < total_ahci_drives; i++) {      
        printf("Drive nr %d size in sectors : %d, ", i, diskdrive[i].total_sectors);
        printf("drive nr %d size in GB : %d", i, diskdrive[i].total_gb);
        printf("\n");
                    
        // print drives info on the screen
        printf("Drive model : ");
        printf(diskdrive[i].drive_model);
        printf("\n");
                    
        printf("Drive serial number : ");
        printf(diskdrive[i].drive_serial);
        printf("\n");
                    
        printf("Drive firmware revision : ");
        printf(diskdrive[i].drive_firmware);
        printf("\n\n");                    
    }

    ahci_close_ahci ();

    return 1;
}


/*************************************/
/* temporary for testing rdtsc timer */
/*************************************/

void QueryPerformanceFrequency (unsigned __int64 *freq)
{
    *freq = calculated_frequency;
}

void QueryPerformanceCounter (unsigned __int64 *count)
{
    _asm rdtsc;   // pentium or above
    _asm mov ebx, count;
    _asm mov [ebx], eax;
    _asm mov [ebx + 4], edx;
}


/******************/
/* DPMI functions */
/******************/

BOOL DPMI_SimulateRMI (BYTE IntNum, DPMIREGS *regs) {
    BYTE noerror = 0;

    _asm {
        mov edi, [regs]
        sub ecx,ecx
        sub ebx,ebx
        mov bl, [IntNum]
        mov eax, 0x300
        int 0x31
        setnc [noerror]
    }   
    return noerror;
}

BOOL DPMI_DOSmalloc (DWORD size, WORD *segment, WORD *selector) {
    BYTE noerror = 0;
    
    _asm {
        mov eax, 0x100
        mov ebx, [size]
        add ebx, 0x0f
        shr ebx, 4
        int 0x31
        setnc [noerror]
        mov ebx, [segment]
        mov [ebx], ax
        mov ebx, [selector]
        mov [ebx], dx
    }
    return noerror;
}

void DPMI_DOSfree (WORD *selector) {
    
    _asm {
        mov eax, [selector]
        mov dx, [eax]
        mov eax, 0x101
        int 0x31
    }
}

BOOL DPMI_MapMemory (unsigned long *physaddress, unsigned long *linaddress, unsigned long size) {
    BYTE noerror = 0;

    _asm {
        mov eax,[physaddress]          ; pointer to physical address
        mov ebx,[eax]                  ; physical address
        mov cx,bx
        shr ebx,16                     ; BX:CX = physical address
        mov esi,[size]                 ; size in bytes
        mov di,si
        shr esi,16                     ; SI:DI = size in bytes
        mov eax,0x800                  ; physical address mapping
        int 0x31
        setnc [noerror]
        shl ebx,16
        mov bx,cx
        mov eax,[linaddress]           ; pointer to linear address
        mov [eax],ebx                  ; linaddress = BX:CX
    }
    return noerror;
}

BOOL DPMI_UnmapMemory (unsigned long *linearaddress) {
    BYTE noerror = 0;

    _asm {
        mov eax,[linearaddress]        ; pointer to linear address
        mov ebx,[eax]                  ; linear address
        mov cx,bx
        shr ebx,16                     ; BX:CX = linear address
        mov eax,0x801                  ; free physical address mapping
        int 0x31
        setnc [noerror]
    }
    return noerror;
}


/******************/
/* text functions */
/******************/

char *text_ConvertToString (char stringdata[256], int count)
{
    int i = 0;
    static char string[512];

    // characters are stored backwards
    for (i = 0; i < count; i += 2) {
        string [i] = (char) (stringdata[i + 1]);
        string [i + 1] = (char) (stringdata[i]);
    }

    // end the string
    string[i] = '\0';

    return string;
}

char *text_CutSpacesAfter (char *str)
{
    int i = 0;
    static char cut [512];

    // cut spaces after text
    strcpy (cut, str);
    for (i = strlen(cut) - 1; i > 0 && cut[i] == ' '; i--)
        cut[i] = '\0';

    return cut;
}

char *text_CutSpacesBefore (char *str)
{
    int i = 0;
    int j = 0;
    static char cut [512];

    // cut spaces before text
    for (i = 0; i < strlen(str); i++)
        if (str[i] != ' ') {
            cut[j] = str[i];
            j++;
        }
    cut[j] = '\0';

    return cut;
}
