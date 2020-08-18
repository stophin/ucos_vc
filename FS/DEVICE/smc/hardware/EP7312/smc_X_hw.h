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
File        : smc_hw.h
Purpose     : SMC hardware layer for Cogent EP7312 Development Kit
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef __SMC_X_HW_H__
#define __SMC_X_HW_H__

/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

/* SMC functions used by "smc_phy.c" */
void FS_SMC_HW_X_SetData(FS_u32 id);
void FS_SMC_HW_X_SetCmd(FS_u32 id);
void FS_SMC_HW_X_SetAddr(FS_u32 id);
void FS_SMC_HW_X_SetStandby(FS_u32 id);
char FS_SMC_HW_X_InData(FS_u32 id);
void FS_SMC_HW_X_OutData(FS_u32 id,unsigned char);
void FS_SMC_HW_X_VccOn(FS_u32 id);
void FS_SMC_HW_X_VccOff(FS_u32 id);
char FS_SMC_HW_X_ChkCardIn(FS_u32 id);
char FS_SMC_HW_X_ChkStatus(FS_u32 id);
char FS_SMC_HW_X_ChkWP(FS_u32 id);
char FS_SMC_HW_X_ChkPower(FS_u32 id);
char FS_SMC_HW_X_ChkBusy(FS_u32 id);

/* Timer functions used by "smc_phy.c" */
void FS_SMC_HW_X_WaitTimer(FS_u32 id,unsigned short);
void FS_SMC_HW_X_SetTimer(FS_u32 id,unsigned short);
void FS_SMC_HW_X_StopTimer(FS_u32 id);
int  FS_SMC_HW_X_ChkTimer(FS_u32 id);

/* SMC functions used by "smc_drv.c" */
char FS_SMC_HW_X_DetectStatus(FS_u32 id);
void FS_SMC_HW_X_BusyLedOn(FS_u32 id);
void FS_SMC_HW_X_BusyLedOff(FS_u32 id);


#endif  /* __SMC_X_HW_H__ */


