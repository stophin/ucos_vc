/*
**********************************************************************
*                          Micrium, Inc.
*                      949 Crestview Circle
*                     Weston,  FL 33327-1848
*
*                            uC/FS
*
*             (c) Copyright 2001 - 2003, Micrium, Inc.
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : ide_X_hw.c
Purpose     : IDE hardware layer for EP7312
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "fs_port.h"
#include "fs_conf.h"

#if FS_USE_IDE_DRIVER

#include "ide_x_hw.h"

/*********************************************************************
*
*             #define Macros
*
**********************************************************************
*/

/* SFR definition of EP7312 */
#define __PBDR                  *(volatile unsigned char*)0x80000001
#define __PDDR                  *(volatile unsigned char*)0x80000003
#define __PBDDR                 *(volatile unsigned char*)0x80000041
#define __PDDDR                 *(volatile unsigned char*)0x80000043
#define __SYSCON1               *(volatile unsigned int*)0x80000100
#define __SYSFLG1               *(volatile unsigned int*)0x80000140
#define __MEMCFG2               *(volatile unsigned int*)0x800001C0
#define __INTSR1                *(volatile unsigned int*)0x80000240
#define __INTMR1                *(volatile unsigned int*)0x80000280
#define __TC1D                  *(volatile unsigned short*)0x80000300
#define __TC2D                  *(volatile unsigned short*)0x80000340
#define __UARTDR1               *(volatile unsigned short*)0x80000480
#define __UBRLCR1               *(volatile unsigned int*)0x800004C0
#define __TC1EOI                *(volatile unsigned int*)0x800006C0
#define __TC2EOI                *(volatile unsigned int*)0x80000700
#define __SYSFLG2               *(volatile unsigned int*)0x80001140
#define __INTMR2                *(volatile unsigned int*)0x80001280
#define __SYSCON3               *(volatile unsigned int*)0x80002200
#define __PLLW                  *(volatile unsigned int*)0x80002610

/* CSB238 IDE-Bus */
#define __IDE_DATA              *(volatile unsigned short*) 0x50000000
#define __IDE_FC                *(volatile unsigned char*)0x50000001
#define __IDE_SC                *(volatile unsigned char*)0x50000002
#define __IDE_SN                *(volatile unsigned char*)0x50000003
#define __IDE_CL                *(volatile unsigned char*)0x50000004
#define __IDE_CH                *(volatile unsigned char*)0x50000005
#define __IDE_DH                *(volatile unsigned char*)0x50000006
#define __IDE_CMD               *(volatile unsigned char*)0x50000007
#define __IDE_DC                *(volatile unsigned char*)0x5000000e
#define __IDE_PIO3              *(volatile unsigned char*)0x50010000


#define HW__DELAY400NS          FS_IDE_HW_X_GetAltStatus(Unit); FS_IDE_HW_X_GetAltStatus(Unit); \
                                FS_IDE_HW_X_GetAltStatus(Unit); FS_IDE_HW_X_GetAltStatus(Unit)


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static char _HW_DevicePresent[2];


/*********************************************************************
*
*             Global functions section
*
**********************************************************************
*/

/*********************************************************************
*
*             FS_IDE_HW_X_BusyLedOn
*
  Description:
  FS driver hardware layer function. Turn on busy led.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_BusyLedOn(FS_u32 Unit) {
}


/*********************************************************************
*
*             FS_IDE_HW_X_BusyLedOff
*
  Description:
  FS driver hardware layer function. Turn off busy led.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_BusyLedOff(FS_u32 Unit) {
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetData
*
  Description:
  FS driver hardware layer function. Set the WR DATA register.

  Parameters:
  Unit        - Unit number.
  Data        - Data to be set.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetData(FS_u32 Unit, FS_u16 Data) {
  __MEMCFG2   = 0x1c13;     /* CS5 16 bit */
  __IDE_DATA  = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetData
*
  Description:
  FS driver hardware layer function. Read the RD DATA register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the RD DATA register.
*/

FS_u16 FS_IDE_HW_X_GetData(FS_u32 Unit) {
  FS_u16 data;

  __MEMCFG2   = 0x1c13;     /* CS5 16 bit */
  data = __IDE_DATA;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetFeatures
*
  Description:
  FS driver hardware layer function. Set the FEATURES register.

  Parameters:
  Unit        - Unit number.
  Data        - Value to write to the FEATURES register.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetFeatures(FS_u32 Unit, unsigned char Data) {
  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  __IDE_FC    = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetError
*
  Description:
  FS driver hardware layer function. Read the ERROR register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the ERROR register.
*/

unsigned char FS_IDE_HW_X_GetError(FS_u32 Unit) {
  unsigned char data;

  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  data = __IDE_FC;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetSectorCount
*
  Description:
  FS driver hardware layer function. Set the SECTOR COUNT register.

  Parameters:
  Unit        - Unit number.
  Data        - Value to write to the SECTOR COUNT register.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetSectorCount(FS_u32 Unit, unsigned char Data) {
  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  __IDE_SC    = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetSectorCount
*
  Description:
  FS driver hardware layer function. Read the SECTOR COUNT register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the SECTOR COUNT register.
*/

unsigned char FS_IDE_HW_X_GetSectorCount(FS_u32 Unit) {
  unsigned char data;

  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  data = __IDE_SC;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetSectorNo
*
  Description:
  FS driver hardware layer function. Set the SECTOR NUMBER register.

  Parameters:
  Unit        - Unit number.
  Data        - Value to write to the SECTOR NUMBER register.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetSectorNo(FS_u32 Unit, unsigned char Data) {
  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  __IDE_SN    = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetSectorNo
*
  Description:
  FS driver hardware layer function. Read the SECTOR NUMBER register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the SECTOR NUMBER register.
*/

unsigned char FS_IDE_HW_X_GetSectorNo(FS_u32 Unit) {
  unsigned char data;

  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  data = __IDE_SN;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetCylLow
*
  Description:
  FS driver hardware layer function. Set the CYLINDER LOW register.

  Parameters:
  Unit        - Unit number.
  Data        - Value to write to the CYLINDER LOW register.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetCylLow(FS_u32 Unit, unsigned char Data) {
  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  __IDE_CL    = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetCylLow
*
  Description:
  FS driver hardware layer function. Read the CYLINDER LOW register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the CYLINDER LOW register.
*/

unsigned char FS_IDE_HW_X_GetCylLow(FS_u32 Unit) {
  unsigned char data;

  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  data = __IDE_CL;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetCylHigh
*
  Description:
  FS driver hardware layer function. Set the CYLINDER HIGH register.

  Parameters:
  Unit        - Unit number.
  Data        - Value to write to the CYLINDER HIGH register.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetCylHigh(FS_u32 Unit, unsigned char Data) {
  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  __IDE_CH    = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetCylHigh
*
  Description:
  FS driver hardware layer function. Read the CYLINDER HIGH register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the CYLINDER HIGH register.
*/

unsigned char FS_IDE_HW_X_GetCylHigh(FS_u32 Unit) {
  unsigned char data;

  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  data = __IDE_CH;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetDevice
*
  Description:
  FS driver hardware layer function. Set the DEVICE/HEAD register.

  Parameters:
  Unit        - Unit number.
  Data        - Value to write to the DEVICE/HEAD register.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetDevice(FS_u32 Unit, unsigned char Data) {
  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  __IDE_DH    = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetDevice
*
  Description:
  FS driver hardware layer function. Read the DEVICE/HEAD register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the DEVICE/HEAD register.
*/

unsigned char FS_IDE_HW_X_GetDevice(FS_u32 Unit) {
  unsigned char data;

  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  data = __IDE_DH;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetCommand
*
  Description:
  FS driver hardware layer function. Set the COMMAND register.

  Parameters:
  Unit        - Unit number.
  Data        - Value to write to the COMMAND register.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetCommand(FS_u32 Unit, unsigned char Data) {
  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  __IDE_CMD    = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetStatus
*
  Description:
  FS driver hardware layer function. Read the STATUS register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the STATUS register.
*/

unsigned char FS_IDE_HW_X_GetStatus(FS_u32 Unit) {
  unsigned char data;

  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  data = __IDE_CMD;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_SetDevControl
*
  Description:
  FS driver hardware layer function. Set the DEVICE CONTROL register.

  Parameters:
  Unit        - Unit number.
  Data        - Value to write to the DEVICE CONTROL register.
 
  Return value:
  None.
*/

void FS_IDE_HW_X_SetDevControl(FS_u32 Unit, unsigned char Data) {
  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  __IDE_DC    = Data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_GetAltStatus
*
  Description:
  FS driver hardware layer function. Read the ALTERNATE STATUS register.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  Value of the ALTERNATE STATUS register.
*/

unsigned char FS_IDE_HW_X_GetAltStatus(FS_u32 Unit) {
  unsigned char data;

  __MEMCFG2   = 0x1f13;     /* CS5 8 bit */
  data = __IDE_DC;
  return data;
}


/*********************************************************************
*
*             FS_IDE_HW_X_DetectStatus
*
  Description:
  FS driver hardware layer function. Check if the device is present.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  ==0         - Device is connected.
  !=0         - Device has not been found.
*/

char FS_IDE_HW_X_DetectStatus(FS_u32 Unit) {
  static char init;
  int lexp;
  unsigned char a;
  unsigned char b;

  if (!init) {
    init  = 1;
    __MEMCFG2  = 0x1f13;        /* CS5 8 bit */
    __SYSCON1 |= 0x40000ul;     /* enable expansion clock */
  }
  if (Unit == 0) {
    FS_IDE_HW_X_SetDevice(Unit, 0xa0);
  }
  else {
    FS_IDE_HW_X_SetDevice(Unit, 0xe0);
  }
  HW__DELAY400NS;
  FS_IDE_HW_X_SetSectorCount(Unit, 0x55);
  FS_IDE_HW_X_SetSectorNo(Unit, 0xaa);
  FS_IDE_HW_X_SetSectorCount(Unit, 0xaa);
  FS_IDE_HW_X_SetSectorNo(Unit, 0x55);
  FS_IDE_HW_X_SetSectorCount(Unit, 0x55);
  FS_IDE_HW_X_SetSectorNo(Unit, 0xaa);
  a = FS_IDE_HW_X_GetSectorCount(Unit);
  b = FS_IDE_HW_X_GetSectorNo(Unit);
  lexp = (a == 0x55);
  lexp = lexp && (b == 0xaa);
  if (lexp) {
    _HW_DevicePresent[Unit] = 1;
  }
  else {
    _HW_DevicePresent[Unit] = 0;
  }
  return (!_HW_DevicePresent[Unit]);
}


/*********************************************************************
*
*             FS_IDE_HW_X_HWReset
*
  Description:
  FS driver hardware layer function. This function is called, when 
  the driver detects a new media is present. For ATA HD drives, there 
  is no action required and this function can be empty.
  When using a CF card, please be aware, that the card needs to be
  power cycled while ~OE is grounded. If the card is inserted, VCC & 
  GND will provide the card before ~OE is connected and the card will 
  be in PC Card ATA mode.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  None.

   
*/

void FS_IDE_HW_X_HWReset(FS_u32 Unit) {
  if (Unit != 0) {
    return;
  }
}

#endif /* FS_USE_IDE_DRIVER */

