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
Purpose     : IDE hardware layer for Segger SED137XE board accessing
              a CF card in true IDE mode with P7,P8,P9,P10.

                 CF                       CPU
              =============================================
               A00 - A02        <-      P70 - P72
               A03 - A10        <-      GND
               PDIAG            <->     PU
               DASP             <->     PU
               CD1 - CD2         ->     P80 - P81
               CS0 - CS1        <-      P73 - P74
               CSEL             <-      GND
               D00 - D15        <->     P90 - P107
               IORD             <-      P75
               IOWR             <-      P76
               ATA SEL          <-      GND
               INTRQ             ->     P83 
               REG              <-      VCC
               RESET            <-      P77
               VS1               ->     PU 
               VS2               ->     PU 
               IORDY             ->     P82
               WE               <-      VCC
               VCC              <-      VCC switched by P84
              =============================================
   
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

/* 
  The following header file is part of the IAR compiler for M16C/80. 
  If you use a different compiler, you may have to include a different 
  file or you may even have to replace the SFR access within this 
  file.
*/
#include <IOMC80.H>


/*********************************************************************
*
*             #define Macros
*
**********************************************************************
*/

/*
   To meet timing specification, you may have to add delays here
   when porting to a different CPU. For MC80 at 16MHz, there is no
   delay required.
*/

#define FS__IDE_DELAY_WRITE   
#define FS__IDE_DELAY_READ
#define FS__IDE_DELAY_RESET


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_IDE_HW_SetA0A2
*
  Description:
  FS driver internal function. Set the address lines of the IDE 
  interface.

  Parameters:
  Unit        - Unit number. 
  Address     - Value for A0..A2.
 
  Return value:
  None.
*/

static void _FS_IDE_HW_SetA0A2(FS_u32 Unit, unsigned char Addr) {
  unsigned char x;

  if (Unit != 0) {
    return;
  }
  x = P7 & 0xf8;
  x |= (Addr & 0x07);
  P7 = x;
}


/*********************************************************************
*
*             _FS_IDE_HW_SetCS0CS1
*
  Description:
  FS driver internal function. Set the CS0CS1 lines of the IDE 
  interface.

  Parameters:
  Unit        - Unit number. 
  Data        - Value for CS0..CS1.
 
  Return value:
  None.
*/

static void _FS_IDE_HW_SetCS0CS1(FS_u32 Unit, unsigned char Data) {
  unsigned char x;

  if (Unit != 0) {
    return;
  }
  x = P7 & 0xe7;
  x |= ((Data & 3) << 3);
  P7 = x;
}


/*********************************************************************
*
*             _FS_IDE_HW_SetD0D15
*
  Description:
  FS driver internal function. Set the data lines of the IDE interface.

  Parameters:
  Unit        - Unit number. 
  Data        - Value for D0..D15.
 
  Return value:
  None.
*/

static void _FS_IDE_HW_SetD0D15(FS_u32 Unit, FS_u16 Data) {
  if (Unit != 0) {
    return;
  }
  /* Set mode output */
  PRCR |= 0x04;
  PD9  = 0xff;
  PD10 = 0xff;
  PRCR &= 0xfb;
  /* Set Data */
  P9  = Data & 0xff;
  P10 = ((Data >> 8) & 0xff);
}


/*********************************************************************
*
*             _FS_IDE_HW_GetD0D15
*
  Description:
  FS driver internal function. Read the data lines of the IDE interface.

  Parameters:
  Unit        - Unit number. 
 
  Return value:
  Value of D0..D15.
*/

static FS_u16 _FS_IDE_HW_GetD0D15(FS_u32 Unit) {
  FS_u16 x;
  
  if (Unit != 0) {
    return 0;
  }
  /* Set mode input */
  PRCR |= 0x04;
  PD9  = 0x00;
  PD10 = 0x00;
  PRCR &= 0xfb;
  /* Get data */
  x = P10;
  x = x << 8;
  x |= P9;
  return x;
}


/*********************************************************************
*
*             _FS_IDE_HW_SetIORD
*
  Description:
  FS driver internal function. Set the IORD line of the IDE interface.

  Parameters:
  Unit        - Unit number. 
  Data        - Value for IORD.
 
  Return value:
  None.
*/

static void _FS_IDE_HW_SetIORD(FS_u32 Unit, unsigned char Data) {
  if (Unit != 0) {
    return;
  }
  if (Data) {
    P7 |= 0x20;
  }
  else {
    P7 &= 0xdf;
  }
}


/*********************************************************************
*
*             _FS_IDE_HW_SetIOWR
*
  Description:
  FS driver internal function. Set the IOWR line of the IDE interface.

  Parameters:
  Unit        - Unit number. 
  Data        - Value for IOWR.
 
  Return value:
  None.
*/

static void _FS_IDE_HW_SetIOWR(FS_u32 Unit, unsigned char Data) {
  if (Unit != 0) {
    return;
  }
  if (Data) {
    P7 |= 0x40;
  }
  else {
    P7 &= 0xbf;
  }
}


/*********************************************************************
*
*             _FS_IDE_HW_GetIORDY
*
  Description:
  FS driver internal function. Read the IORDY line of the IDE interface.

  Parameters:
  Unit        - Unit number. 
 
  Return value:
  ==0         - Value of IORDY.
  ==1         - Value of IORDY.
  ==0xff      - An error has occured.
*/

static char _FS_IDE_HW_GetIORDY(FS_u32 Unit) {
  if (Unit != 0) {
    return 0xff;
  }
  if (P8 & 0x04) {
    return 1;
  }
  return 0;
}


/*********************************************************************
*
*             _FS_IDE_HW_WriteBUS
*
  Description:
  FS driver internal function. Write a value to an IDE register.

  Parameters:
  Unit        - Unit number. 
  CS          - Value for CS0..CS1.
  Addr        - Address of the register.
  Data        - Data to be written to the register
 
  Return value:
  None.
*/

static void _FS_IDE_HW_WriteBUS(FS_u32 Unit, unsigned char CS, unsigned char Addr, FS_u16 Data) {
  _FS_IDE_HW_SetA0A2(Unit, Addr);
  _FS_IDE_HW_SetCS0CS1(Unit, CS);
  _FS_IDE_HW_SetD0D15(Unit, Data);
  _FS_IDE_HW_SetIOWR(Unit, 0);
  FS__IDE_DELAY_WRITE;                /* delay */
  while (!_FS_IDE_HW_GetIORDY(Unit)) {
  }
  _FS_IDE_HW_SetIOWR(Unit, 1);
  _FS_IDE_HW_SetCS0CS1(Unit, 0x03);     /* Standby Mode */
  _FS_IDE_HW_SetA0A2(Unit, 0x00);
}


/*********************************************************************
*
*             _FS_IDE_HW_ReadBUS
*
  Description:
  FS driver internal function. Read value of an IDE register.

  Parameters:
  Unit        - Unit number. 
  CS          - Value for CS0..CS1.
  Addr        - Address of the register.
   
  Return value:
  Value of the IDE register.
*/

static FS_u16 _FS_IDE_HW_ReadBUS(FS_u32 Unit, unsigned char CS, unsigned char Addr) {
  FS_u16 data;
  
  _FS_IDE_HW_SetA0A2(Unit, Addr);
  _FS_IDE_HW_SetCS0CS1(Unit, CS);
  _FS_IDE_HW_SetIORD(Unit, 0);
  FS__IDE_DELAY_READ;                 /* delay */
  while (!_FS_IDE_HW_GetIORDY(Unit)) {
  }
  data = _FS_IDE_HW_GetD0D15(Unit);
  _FS_IDE_HW_SetIORD(Unit, 1);
  _FS_IDE_HW_SetCS0CS1(Unit, 0x03);     /* Standby Mode */
  _FS_IDE_HW_SetA0A2(Unit, 0x00);
  return data;
}


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
  _FS_IDE_HW_WriteBUS(Unit, 0x02, 0x00, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x02, 0x00);
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
  _FS_IDE_HW_WriteBUS(Unit, 0x02, 0x01, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x02, 0x01) & 0xff;
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
  _FS_IDE_HW_WriteBUS(Unit, 0x02, 0x02, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x02, 0x02) & 0xff;
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
  _FS_IDE_HW_WriteBUS(Unit, 0x02, 0x03, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x02, 0x03) & 0xff;
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
  _FS_IDE_HW_WriteBUS(Unit, 0x02, 0x04, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x02, 0x04) & 0xff;
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
  _FS_IDE_HW_WriteBUS(Unit, 0x02, 0x05, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x02, 0x05) & 0xff;
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
  _FS_IDE_HW_WriteBUS(Unit, 0x02, 0x06, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x02, 0x06) & 0xff;
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
  _FS_IDE_HW_WriteBUS(Unit, 0x02, 0x07, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x02, 0x07) & 0xff;
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
  _FS_IDE_HW_WriteBUS(Unit, 0x01, 0x06, Data);
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

  data = _FS_IDE_HW_ReadBUS(Unit, 0x01, 0x06) & 0xff;
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

  if (!init) {
    init  = 1;
    PD7   =  0xff;    /* P70 - P77 output */
    P7    =  0xff;
    PD8   =  0xf0;    /* P80 - P83 input / P84 - P87 output  */
    P8    =  0xff;
    PRCR  |= 0x04;    /* allow PD9 change */
    PD9   =  0x00;    /* P90 - P97 input */
    PRCR  &= 0xfb;    /* disable PD9 change */
    PD10  =  0x00;    /* P100 - P107 input */
  }
  /* check CD1 & CD2 */
  if (P8 & 0x03) {
    return 1;
  }
  return 0;
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
  volatile unsigned long int to;
  
  if (Unit!=0) {
    return;
  }
  P8 |= 0x10;             /* power off */
  PD7     = 0x00;
  to = 10000;
  while (to) {
    to--;
  }
  PD7 = 0xff;
  P8 &= 0xef;             /* power on */
  P7 |= 0x80;             /* RESET high */
  P7 &= 0x7f;             /* RESET low */
  FS__IDE_DELAY_RESET;
  P7 |= 0x80;             /* RESET high */
}

#endif /* FS_USE_IDE_DRIVER */


