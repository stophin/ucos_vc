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
File        : smc_hw.c
Purpose     : SMC hardware layer for Cogent EP7312 Development Kit
              This layer does support the on board NAND FLASH (id==1)
              and the Smart Media Socket (id==0) of the Cogent board.
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
#include "smc_x_hw.h"


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

/* CSB238 On-Board NAND FLASH & SmartMedia Socket */
#define __SMCPORT               *(volatile char*)0x10000000


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

/*
  Following variables are used instead of HW pin detection, which is
  not supported by Cogent EP7312 Development Kit. Their values are
  set by "FS__SMC_HW_DetectStatus".
*/

static char _Hw_card_changed[2]  = {1,1};
static char _Hw_card_wprotect[2];
static char _Hw_card_in[2];


/*********************************************************************
*
*             Global functions section 1
*
**********************************************************************

  All functions in this section are called by "smc_phy.c".
  These functions implement a 0.1 ms timer by using Timer 2 of
  the EP7312.
*/

/*********************************************************************
*
*             FS_SMC_HW_X_StopTimer
*/

void FS_SMC_HW_X_StopTimer(FS_u32 id)  {   
}


/*********************************************************************
*
*             FS_SMC_HW_X_ChkTimer
*/

int FS_SMC_HW_X_ChkTimer(FS_u32 id) {
  short int x;

  x = __TC2D;
  if (x < 0) {
    x = 0;
  }
  /* 
     Make sure it does return !=0 at least one time.
     Current "smc_phy.c" implementation does not check for condition,
     if time has already elapsed for some reason.
  */
  return x; 
}


/*********************************************************************
*
*             FS_SMC_HW_X_SetTimer
*/

void FS_SMC_HW_X_SetTimer(FS_u32 id,unsigned short time) {
  __INTMR1  &= ~0x200ul;     /* disable TC2 underflow interrupt */
  __SYSCON1 |= 0x80ul;       /* Set clock in 512kHz mode */
  __SYSCON1 &= ~0x40ul;     /* free running */
  __TC2D     = time * 50;      /* Load counter */
}


/*********************************************************************
*
*             FS_SMC_HW_X_WaitTimer
*/

void FS_SMC_HW_X_WaitTimer(FS_u32 id,unsigned short time) {
  short x;
  
  __INTMR1  &= ~0x200ul;     /* disable TC2 underflow interrupt */
  __SYSCON1 |= 0x80ul;       /* Set clock in 512kHz mode */
  __SYSCON1 &= ~0x40ul;     /* free running */
  __TC2D     = time * 50;      /* Load counter */
  do {
    x = __TC2D;
  } while (x > 0);
}


/*********************************************************************
*
*             Global functions section 2
*
**********************************************************************

  All functions in this section are called by "smc_phy.c".
  Functions in this section access the SMC control signals and
  SMC data port of the Cogent EP7312 Development Kit.
*/

/*********************************************************************
*
*             FS_SMC_HW_X_SetData
*/

void FS_SMC_HW_X_SetData(FS_u32 id) {
  if (id == 0) {
    __PBDR      = 0x4f;       /* nCE low, CLE low, ALE low  */
  }
  else if (id == 1) {
    __PBDR      = 0x8f;       /* nCE low, CLE low, ALE low  */
  }
}


/*********************************************************************
*
*             FS_SMC_HW_X_SetCmd
*/

void FS_SMC_HW_X_SetCmd(FS_u32 id) {
  if (id == 0) {
    __PBDR      = 0x5f;       /* nCE low, CLE high, ALE low  */
  }
  else if (id == 1) {
    __PBDR      = 0x9f;       /* nCE low, CLE high, ALE low  */
  }
}


/*********************************************************************
*
*             FS_SMC_HW_X_SetAddr
*/

void FS_SMC_HW_X_SetAddr(FS_u32 id) {
  if (id == 0) {
    __PBDR      = 0x6f;       /* nCE low, CLE low, ALE high */
  }
  else if (id == 1) {
    __PBDR      = 0xaf;       /* nCE low, CLE low, ALE high  */
  }
}


/*********************************************************************
*
*             FS_SMC_HW_X_SetStandby
*/

void FS_SMC_HW_X_SetStandby(FS_u32 id) {
  __PBDR      = 0x0f;       /* nCE high, CLE low, ALE low  */
}


/*********************************************************************
*
*             FS_SMC_HW_X_InData
*/

char FS_SMC_HW_X_InData(FS_u32 id) {
  return __SMCPORT;
}


/*********************************************************************
*
*             FS_SMC_HW_X_OutData
*/

void FS_SMC_HW_X_OutData(FS_u32 id,unsigned char data) {
  __SMCPORT = (char)data;
}


/*********************************************************************
*
*             FS_SMC_HW_X_VccOn
*/

void FS_SMC_HW_X_VccOn(FS_u32 id) {
}


/*********************************************************************
*
*             FS_SMC_HW_X_VccOff
*/

void FS_SMC_HW_X_VccOff(FS_u32 id) {
}


/*********************************************************************
*
*             FS_SMC_HW_X_ChkCardIn
*/

char FS_SMC_HW_X_ChkCardIn(FS_u32 id) {
  return _Hw_card_in[id];
}


/*********************************************************************
*
*             FS_SMC_HW_X_ChkStatus
*/

char FS_SMC_HW_X_ChkStatus(FS_u32 id) {
  if (_Hw_card_changed[id]) {
    _Hw_card_changed[id] = 0;
    return 1;
  }
  return 0;
}


/*********************************************************************
*
*             FS_SMC_HW_X_ChkWP
*/

char FS_SMC_HW_X_ChkWP(FS_u32 id) {
  return _Hw_card_wprotect[id];
}


/*********************************************************************
*
*             FS_SMC_HW_X_ChkPower
*/

char FS_SMC_HW_X_ChkPower(FS_u32 id) {
  return 1;
}


/*********************************************************************
*
*             FS_SMC_HW_X_ChkBusy
*/

char FS_SMC_HW_X_ChkBusy(FS_u32 id) {
  char x;

  FS_SMC_HW_X_SetCmd(id);        /* nCE low, CLE high, ALE low  */
  __SMCPORT   = 0x70;             /* cmd read status   */
  FS_SMC_HW_X_SetData(id);       /* nCE low, CLE low, ALE low  */
  x  = __SMCPORT;
  FS_SMC_HW_X_SetStandby(id);    /* nCE high, CLE low, ALE low  */
  if (x&0x40) {
    return 0;
  }
  return 1;
}


/*********************************************************************
*
*             Global functions section 3
*
**********************************************************************

  All functions in this section are called by "smc_drv.c", which is
  the main module of file system's SMC driver. Module "smc_phy.c" does
  not use these functions.
*/

/*********************************************************************
*
*             FS_SMC_HW_X_BusyLedOn
*/

void FS_SMC_HW_X_BusyLedOn(FS_u32 id) {
  if (id == 0) {
    __PDDR |= 0x01;           /* DIAG LED on */
  }
}


/*********************************************************************
*
*             FS_SMC_HW_X_BusyLedOff
*/

void FS_SMC_HW_X_BusyLedOff(FS_u32 id) {
  if (id == 0) {
    __PDDR &= ~0x01;        /* DIAG LED off */
  }
}


/*********************************************************************
*
*             FS_SMC_HW_X_DetectStatus
*
  This function is called by "smc_drv.c" only and is used to detect
  a diskchange. It is not used by "smc_phy.c", which uses functions
  "FS__SMC_HW_ChkCardIn", "FS__SMC_HW_ChkStatus", "FS__SMC_HW_ChkWP"
  and "FS__SMC_HW_ChkPower" for that purpose.

  This function needs to be called periodically to make sure, that
  every  diskchange is detected, because there is no HW diskchange
  detect in EP7312 Development Kit. This is done by executing
  "FS_IoCtl("smc:",FS_CMD_CHK_DSKCHANGE,0,0)".
*/

char FS_SMC_HW_X_DetectStatus(FS_u32 id) {
  char x;

  FS_SMC_HW_X_SetCmd(id);        /* nCE low, CLE high, ALE low  */
  __SMCPORT   = 0x70;             /* cmd read status   */
  FS_SMC_HW_X_SetData(id);       /* nCE low, CLE low, ALE low  */
  x  = __SMCPORT;
  FS_SMC_HW_X_SetStandby(id);    /* nCE high, CLE low, ALE low  */
  if ((x == 0x70) || (x & 0x01) || (!(x & 0x40))) {
    _Hw_card_changed[id]  = 1;
    _Hw_card_wprotect[id] = 1;
    _Hw_card_in[id]       = 0;
    return 1;
  }
  _Hw_card_in[id]         = 1;
  if (x&0x80) {
    _Hw_card_wprotect[id] = 0;
  }
  else {
    _Hw_card_wprotect[id] = 1;
  }
  return 0;
}

