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
File        : smc_phy.c
Purpose     : SMC generic physical media access
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
#include "fs_dev.h"
#include "fs_conf.h"

#if FS_USE_SMC_DRIVER

#include "smc.h"
#include "smc_x_hw.h"


/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

#define EVEN             0          /* Even Page for 256byte/page */
#define ODD              1          /* Odd  Page for 256byte/page */


/*********************************************************************
*
*             Global variables        
*
**********************************************************************
*/

struct FS__SMC_ATTRIBTYPE FS__SMC_cardattrib[FS_SMC_MAXUNIT];
struct FS__SMC_PARAMTYPE  FS__SMC_cardparam[FS_SMC_MAXUNIT];


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_SMC_Set_SsfdcCmd
*/

static void _FS_SMC_Set_SsfdcCmd(FS_u32 id,unsigned char cmd)
{
    FS_SMC_HW_X_SetCmd(id);
    FS_SMC_HW_X_OutData(id,cmd);
    FS_SMC_HW_X_SetData(id);
}


/*********************************************************************
*
*             _FS_SMC_Set_SsfdcAddr
*/

static void _FS_SMC_Set_SsfdcAddr(FS_u32 id,unsigned char add)
{
    unsigned short addr;
    addr=(unsigned short)FS__SMC_cardparam[id].Zone*FS__SMC_cardattrib[id].MaxBlocks+FS__SMC_cardparam[id].PhyBlock;
    addr=addr*(unsigned short)FS__SMC_cardattrib[id].MaxSectors+FS__SMC_cardparam[id].Sector;
    if((FS__SMC_cardattrib[id].Attribute &MPS)==PS256)       /* for 256byte/page */
        addr=addr*2+(unsigned short)add;
/*-----------------------------------------------*/
    FS_SMC_HW_X_SetAddr(id);
    FS_SMC_HW_X_OutData(id,0x00);
    FS_SMC_HW_X_OutData(id,(unsigned char)addr);
    FS_SMC_HW_X_OutData(id,(unsigned char)(addr/0x0100));
    if((FS__SMC_cardattrib[id].Attribute &MADC)==AD4CYC)
        FS_SMC_HW_X_OutData(id,(unsigned char)(FS__SMC_cardparam[id].Zone/2)); /* Patch */
    FS_SMC_HW_X_SetData(id);
}


/*********************************************************************
*
*             _FS_SMC_Set_SsfdcBlock
*/

static void _FS_SMC_Set_SsfdcBlock(FS_u32 id)
{
    unsigned short addr;
    addr=(unsigned short)FS__SMC_cardparam[id].Zone*FS__SMC_cardattrib[id].MaxBlocks+FS__SMC_cardparam[id].PhyBlock;
    addr=addr*(unsigned short)FS__SMC_cardattrib[id].MaxSectors;
    if((FS__SMC_cardattrib[id].Attribute &MPS)==PS256)       /* for 256byte/page */
        addr=addr*2;
/*-----------------------------------------------*/
    FS_SMC_HW_X_SetAddr(id);
    FS_SMC_HW_X_OutData(id,(unsigned char)addr);
    FS_SMC_HW_X_OutData(id,(unsigned char)(addr/0x0100));
    if((FS__SMC_cardattrib[id].Attribute &MADC)==AD4CYC)
        FS_SMC_HW_X_OutData(id,(unsigned char)(FS__SMC_cardparam[id].Zone/2)); /* Patch */
    FS_SMC_HW_X_SetData(id);
}


/*********************************************************************
*
*             _FS_SMC_Set_SsfdcChip
*/

static void _FS_SMC_Set_SsfdcChip(FS_u32 id)
{
    FS_SMC_HW_X_SetAddr(id);
    FS_SMC_HW_X_OutData(id,0x00);
    FS_SMC_HW_X_SetData(id);
}

/*********************************************************************
*
*             _FS_SMC_Set_SsfdcStandby
*/

static void _FS_SMC_Set_SsfdcStandby(FS_u32 id)
{
    FS_SMC_HW_X_SetStandby(id);
}


/*********************************************************************
*
*             _FS_SMC_Chk_SsfdcBusy
*/

static int _FS_SMC_Chk_SsfdcBusy(FS_u32 id,unsigned short time)
{
    FS_SMC_HW_X_SetTimer(id,time);
    while(FS_SMC_HW_X_ChkTimer(id))
        if(! FS_SMC_HW_X_ChkBusy(id)) {
            FS_SMC_HW_X_StopTimer(id);
            return(SUCCESS);
        }
    FS_SMC_HW_X_StopTimer(id);
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Chk_SsfdcStatus
*/

static int _FS_SMC_Chk_SsfdcStatus(FS_u32 id)
{
    if(FS_SMC_HW_X_InData(id) &WR_FAIL)   return(ERROR);
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Reset_SsfdcErr
*/

static void _FS_SMC_Reset_SsfdcErr(FS_u32 id)
{
    FS_SMC_HW_X_SetCmd(id);
    FS_SMC_HW_X_OutData(id,RST_CHIP);
    FS_SMC_HW_X_SetData(id);
    FS_SMC_HW_X_SetTimer(id,BUSY_RESET);
    while(FS_SMC_HW_X_ChkTimer(id))
        if(! FS_SMC_HW_X_ChkBusy(id))     break;
    FS_SMC_HW_X_StopTimer(id);
    FS_SMC_HW_X_SetStandby(id);
}


/*********************************************************************
*
*             _FS_SMC_Read_SsfdcBuf
*/

static void _FS_SMC_Read_SsfdcBuf(FS_u32 id,unsigned char *databuf,unsigned char *redundant)
{
    int i;
    for(i=0x00;i<(((FS__SMC_cardattrib[id].Attribute &MPS)==PS256)?0x100:0x200);i++)
        *databuf++   =FS_SMC_HW_X_InData(id);
    for(i=0x00;i<(((FS__SMC_cardattrib[id].Attribute &MPS)==PS256)?0x08:0x10);i++)
        *redundant++ =FS_SMC_HW_X_InData(id);
}


/*********************************************************************
*
*             _FS_SMC_Write_SsfdcBuf
*/

static void _FS_SMC_Write_SsfdcBuf(FS_u32 id,unsigned char *databuf,unsigned char *redundant)
{
    int i;
    for(i=0x00;i<(((FS__SMC_cardattrib[id].Attribute &MPS)==PS256)?0x100:0x200);i++)
        FS_SMC_HW_X_OutData(id,*databuf++);
    for(i=0x00;i<(((FS__SMC_cardattrib[id].Attribute &MPS)==PS256)?0x08:0x10);i++)
        FS_SMC_HW_X_OutData(id,*redundant++);
}


/*********************************************************************
*
*             _FS_SMC_Read_SsfdcWord
*/

static void _FS_SMC_Read_SsfdcWord(FS_u32 id,unsigned short *pdata)
{
    *pdata =FS_SMC_HW_X_InData(id)*0x100;
    *pdata|=(unsigned char)FS_SMC_HW_X_InData(id);
}


/*********************************************************************
*
*             _FS_SMC_ReadRedt_SsfdcBuf
*/

static void _FS_SMC_ReadRedt_SsfdcBuf(FS_u32 id,unsigned char *redundant)
{
    char i;
    for(i=0x00;i<(((FS__SMC_cardattrib[id].Attribute &MPS)==PS256)?0x08:0x10);i++)
        redundant[i] =FS_SMC_HW_X_InData(id);
}


/*********************************************************************
*
*             _FS_SMC_WriteRedt_SsfdcBuf
*/

static void _FS_SMC_WriteRedt_SsfdcBuf(FS_u32 id,unsigned char *redundant)
{
    char i;
    for(i=0x00;i<(((FS__SMC_cardattrib[id].Attribute &MPS)==PS256)?0x08:0x10);i++)
        FS_SMC_HW_X_OutData(id,*redundant++);
}


/*********************************************************************
*
*             _FS_SMC_Check_DevCode
*/

static unsigned char _FS_SMC_Check_DevCode(unsigned char dcode)
{
    switch(dcode){
        case 0x6E:
        case 0xE8:
        case 0xEC:  return(SSFDC1MB);       /*  8Mbit (1M) NAND */
        case 0x64:
        case 0xEA:  return(SSFDC2MB);       /* 16Mbit (2M) NAND */
        case 0x6B:
        case 0xE3:
        case 0xE5:  return(SSFDC4MB);       /* 32Mbit (4M) NAND */
        case 0xE6:  return(SSFDC8MB);       /* 64Mbit (8M) NAND */
        case 0x73:  return(SSFDC16MB);      /*128Mbit (16M)NAND */
        case 0x75:  return(SSFDC32MB);      /*256Mbit (32M)NAND */
        case 0x76:  return(SSFDC64MB);      /*512Mbit (64M)NAND */
        case 0x79:  return(SSFDC128MB);     /*  1Gbit(128M)NAND */
        default:    return(0xff);
    }
}


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__SMC_PHY_Reset
*/

void FS__SMC_PHY_Reset(FS_u32 id)
{
    _FS_SMC_Set_SsfdcCmd(id,RST_CHIP);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
    _FS_SMC_Chk_SsfdcBusy(id,BUSY_RESET);
#else
    FS_SMC_HW_X_WaitTimer(id,BUSY_RESET);
#endif
    _FS_SMC_Set_SsfdcCmd(id,READ);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
    _FS_SMC_Chk_SsfdcBusy(id,BUSY_READ);
#else
    FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
    _FS_SMC_Set_SsfdcStandby(id);
}


/*********************************************************************
*
*             FS__SMC_PHY_WriteRedtMode
*/

void FS__SMC_PHY_WriteRedtMode(FS_u32 id)
{
    _FS_SMC_Set_SsfdcCmd(id,RST_CHIP);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
    _FS_SMC_Chk_SsfdcBusy(id,BUSY_RESET);
#else
    FS_SMC_HW_X_WaitTimer(id,BUSY_RESET);
#endif
    _FS_SMC_Set_SsfdcCmd(id,READ_REDT);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
    _FS_SMC_Chk_SsfdcBusy(id,BUSY_READ);
#else
    FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
    _FS_SMC_Set_SsfdcStandby(id);
}


/*********************************************************************
*
*             FS__SMC_PHY_ReadID
*/

void FS__SMC_PHY_ReadID(FS_u32 id,unsigned short *pid)
{
    _FS_SMC_Set_SsfdcCmd(id,READ_ID);
    _FS_SMC_Set_SsfdcChip(id);
    _FS_SMC_Read_SsfdcWord(id,pid);
    _FS_SMC_Set_SsfdcStandby(id);
}


/*********************************************************************
*
*             FS__SMC_PHY_ReadSect
*/

int FS__SMC_PHY_ReadSect(FS_u32 id,unsigned char *buf,unsigned char *redundant)
{
    _FS_SMC_Set_SsfdcCmd(id,READ);
    _FS_SMC_Set_SsfdcAddr(id,EVEN);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
    if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_READ))
        { _FS_SMC_Reset_SsfdcErr(id);        return(ERROR); }
#else
    FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
    _FS_SMC_Read_SsfdcBuf(id,buf,redundant);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
    if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_READ))
        { _FS_SMC_Reset_SsfdcErr(id);        return(ERROR); }
#else
    FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
    if((FS__SMC_cardattrib[id].Attribute &MPS)==PS256) {
        _FS_SMC_Set_SsfdcCmd(id,READ);
        _FS_SMC_Set_SsfdcAddr(id,ODD);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
        if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_READ))
            { _FS_SMC_Reset_SsfdcErr(id);    return(ERROR); }
#else
        FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
        _FS_SMC_Read_SsfdcBuf(id,buf+0x100,redundant+0x08);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
        if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_READ))
            { _FS_SMC_Reset_SsfdcErr(id);    return(ERROR); }
#else
        FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
    }
    _FS_SMC_Set_SsfdcStandby(id);
    return(SUCCESS);
}


/*********************************************************************
*
*             FS__SMC_PHY_WriteSect
*/

int FS__SMC_PHY_WriteSect(FS_u32 id,unsigned char *buf,unsigned char *redundant)
{
    _FS_SMC_Set_SsfdcCmd(id,WRDATA);
    _FS_SMC_Set_SsfdcAddr(id,EVEN);
    _FS_SMC_Write_SsfdcBuf(id,buf,redundant);
    _FS_SMC_Set_SsfdcCmd(id,WRITE);
    if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_PROG))   
        { _FS_SMC_Reset_SsfdcErr(id);        return(ERROR); }
    if((FS__SMC_cardattrib[id].Attribute &MPS)==PS256) {
        _FS_SMC_Set_SsfdcCmd(id,RDSTATUS);
        if(_FS_SMC_Chk_SsfdcStatus(id))    
            { _FS_SMC_Set_SsfdcStandby(id);  return(SUCCESS); }
        _FS_SMC_Set_SsfdcCmd(id,WRDATA);
        _FS_SMC_Set_SsfdcAddr(id,ODD);
        _FS_SMC_Write_SsfdcBuf(id,buf+0x100,redundant+0x08);
        _FS_SMC_Set_SsfdcCmd(id,WRITE);
        if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_PROG)) 
            { _FS_SMC_Reset_SsfdcErr(id);    return(ERROR); }
    }
    _FS_SMC_Set_SsfdcStandby(id);
    return(SUCCESS);
}


/*********************************************************************
*
*             FS__SMC_PHY_EraseBlock
*/

int FS__SMC_PHY_EraseBlock(FS_u32 id)
{
    _FS_SMC_Set_SsfdcCmd(id,ERASE1);
    _FS_SMC_Set_SsfdcBlock(id);
    _FS_SMC_Set_SsfdcCmd(id,ERASE2);
    if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_ERASE))  
        { _FS_SMC_Reset_SsfdcErr(id);        return(ERROR); }
    _FS_SMC_Set_SsfdcStandby(id);
    return(SUCCESS);
}


/*********************************************************************
*
*             FS__SMC_PHY_ReadRedtData
*/

int FS__SMC_PHY_ReadRedtData(FS_u32 id,unsigned char *redundant)
{
    _FS_SMC_Set_SsfdcCmd(id,READ_REDT);
    _FS_SMC_Set_SsfdcAddr(id,EVEN);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
    if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_READ))
        { _FS_SMC_Reset_SsfdcErr(id);        return(ERROR); }
#else
    FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
    _FS_SMC_ReadRedt_SsfdcBuf(id,redundant);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
    if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_READ))
        { _FS_SMC_Reset_SsfdcErr(id);        return(ERROR); }
#else
    FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
    if((FS__SMC_cardattrib[id].Attribute &MPS)==PS256) {
        _FS_SMC_Set_SsfdcCmd(id,READ_REDT);
        _FS_SMC_Set_SsfdcAddr(id,ODD);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
        if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_READ))
            { _FS_SMC_Reset_SsfdcErr(id);    return(ERROR); }
#else
        FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
        _FS_SMC_ReadRedt_SsfdcBuf(id,redundant+0x08);
#if FS_SMC_HW_SUPPORT_BSYLINE_CHECK
        if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_READ))
            { _FS_SMC_Reset_SsfdcErr(id);    return(ERROR); }
#else
        FS_SMC_HW_X_WaitTimer(id,BUSY_READ);
#endif
    }
    _FS_SMC_Set_SsfdcStandby(id);
    return(SUCCESS);
}


/*********************************************************************
*
*             FS__SMC_PHY_WriteRedtData
*/

int FS__SMC_PHY_WriteRedtData(FS_u32 id,unsigned char *redundant)
{
    _FS_SMC_Set_SsfdcCmd(id,WRDATA);
    _FS_SMC_Set_SsfdcAddr(id,EVEN);
    _FS_SMC_WriteRedt_SsfdcBuf(id,redundant);
    _FS_SMC_Set_SsfdcCmd(id,WRITE);
    if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_PROG))     
        { _FS_SMC_Reset_SsfdcErr(id);        return(ERROR); }
    if((FS__SMC_cardattrib[id].Attribute &MPS)==PS256) {
        _FS_SMC_Set_SsfdcCmd(id,RDSTATUS);
        if(_FS_SMC_Chk_SsfdcStatus(id))        
            { _FS_SMC_Set_SsfdcStandby(id);  return(SUCCESS); }
        _FS_SMC_Set_SsfdcCmd(id,WRDATA);
        _FS_SMC_Set_SsfdcAddr(id,ODD);
        _FS_SMC_WriteRedt_SsfdcBuf(id,redundant+0x08);
        _FS_SMC_Set_SsfdcCmd(id,WRITE);
        if(_FS_SMC_Chk_SsfdcBusy(id,BUSY_PROG))   
            { _FS_SMC_Reset_SsfdcErr(id);    return(ERROR); }
    }
    _FS_SMC_Set_SsfdcStandby(id);
    return(SUCCESS);
}


/*********************************************************************
*
*             FS__SMC_PHY_CheckStatus
*/

int FS__SMC_PHY_CheckStatus(FS_u32 id)
{
    _FS_SMC_Set_SsfdcCmd(id,RDSTATUS);
    if(_FS_SMC_Chk_SsfdcStatus(id))
        { _FS_SMC_Set_SsfdcStandby(id);      return(ERROR); }
    _FS_SMC_Set_SsfdcStandby(id);
    return(SUCCESS);
}


/*********************************************************************
*
*             FS__SMC_PHY_Set_Model
*/

int FS__SMC_PHY_Set_Model(FS_u32 id,unsigned char dcode)
{
    switch(_FS_SMC_Check_DevCode(dcode))   {
        case SSFDC1MB:
            FS__SMC_cardattrib[id].Model        = SSFDC1MB;
            FS__SMC_cardattrib[id].Attribute    = FLASH | AD3CYC | BS16 | PS256;
            FS__SMC_cardattrib[id].MaxZones     = 1;
            FS__SMC_cardattrib[id].MaxBlocks    = 256;
            FS__SMC_cardattrib[id].MaxLogBlocks = 250;
            FS__SMC_cardattrib[id].MaxSectors   = 8;
            break;
        case SSFDC2MB:
            FS__SMC_cardattrib[id].Model        = SSFDC2MB;
            FS__SMC_cardattrib[id].Attribute    = FLASH | AD3CYC | BS16 | PS256;
            FS__SMC_cardattrib[id].MaxZones     = 1;
            FS__SMC_cardattrib[id].MaxBlocks    = 512;
            FS__SMC_cardattrib[id].MaxLogBlocks = 500;
            FS__SMC_cardattrib[id].MaxSectors   = 8;
            break;
        case SSFDC4MB:
            FS__SMC_cardattrib[id].Model        = SSFDC4MB;
            FS__SMC_cardattrib[id].Attribute    = FLASH | AD3CYC | BS16 | PS512;
            FS__SMC_cardattrib[id].MaxZones     = 1;
            FS__SMC_cardattrib[id].MaxBlocks    = 512;
            FS__SMC_cardattrib[id].MaxLogBlocks = 500;
            FS__SMC_cardattrib[id].MaxSectors   = 16;
            break;
        case SSFDC8MB:
            FS__SMC_cardattrib[id].Model        = SSFDC8MB;
            FS__SMC_cardattrib[id].Attribute    = FLASH | AD3CYC | BS16 | PS512;
            FS__SMC_cardattrib[id].MaxZones     = 1;
            FS__SMC_cardattrib[id].MaxBlocks    = 1024;
            FS__SMC_cardattrib[id].MaxLogBlocks = 1000;
            FS__SMC_cardattrib[id].MaxSectors   = 16;
            break;
        case SSFDC16MB:
            FS__SMC_cardattrib[id].Model        = SSFDC16MB;
            FS__SMC_cardattrib[id].Attribute    = FLASH | AD3CYC | BS32 | PS512;
            FS__SMC_cardattrib[id].MaxZones     = 1;
            FS__SMC_cardattrib[id].MaxBlocks    = 1024;
            FS__SMC_cardattrib[id].MaxLogBlocks = 1000;
            FS__SMC_cardattrib[id].MaxSectors   = 32;
            break;
        case SSFDC32MB:
            FS__SMC_cardattrib[id].Model        = SSFDC32MB;
            FS__SMC_cardattrib[id].Attribute    = FLASH | AD3CYC | BS32 | PS512;
            FS__SMC_cardattrib[id].MaxZones     = 2;
            FS__SMC_cardattrib[id].MaxBlocks    = 1024;
            FS__SMC_cardattrib[id].MaxLogBlocks = 1000;
            FS__SMC_cardattrib[id].MaxSectors   = 32;
            break;
        case SSFDC64MB:
            FS__SMC_cardattrib[id].Model        = SSFDC64MB;
            FS__SMC_cardattrib[id].Attribute    = FLASH | AD4CYC | BS32 | PS512;
            FS__SMC_cardattrib[id].MaxZones     = 4;
            FS__SMC_cardattrib[id].MaxBlocks    = 1024;
            FS__SMC_cardattrib[id].MaxLogBlocks = 1000;
            FS__SMC_cardattrib[id].MaxSectors   = 32;
            break;
        case SSFDC128MB:
            FS__SMC_cardattrib[id].Model        = SSFDC128MB;
            FS__SMC_cardattrib[id].Attribute    = FLASH | AD4CYC | BS32 | PS512;
            FS__SMC_cardattrib[id].MaxZones     = 8;
            FS__SMC_cardattrib[id].MaxBlocks    = 1024;
            FS__SMC_cardattrib[id].MaxLogBlocks = 1000;
            FS__SMC_cardattrib[id].MaxSectors   = 32;
            break;
    default:
            FS__SMC_cardattrib[id].Model        = NOSSFDC;
            return(ERROR);
    }
    return(SUCCESS);
}


/*********************************************************************
*
*             FS__SMC_PHY_Cnt_Reset
*/

void FS__SMC_PHY_Cnt_Reset(FS_u32 id)
{
    FS_SMC_HW_X_SetStandby(id);
    FS_SMC_HW_X_VccOff(id);
}


/*********************************************************************
*
*             FS__SMC_PHY_Cnt_PowerOn
*/

int FS__SMC_PHY_Cnt_PowerOn(FS_u32 id)
{
    FS_SMC_HW_X_VccOn(id);
    FS_SMC_HW_X_WaitTimer(id,TIME_PON);
    if(FS_SMC_HW_X_ChkPower(id))
        return(SUCCESS);
    FS_SMC_HW_X_VccOff(id);
    return(ERROR);
}


/*********************************************************************
*
*             FS__SMC_PHY_Chk_CntPower
*/

int FS__SMC_PHY_Chk_CntPower(FS_u32 id)
{
    if(FS_SMC_HW_X_ChkPower(id))
        return(SUCCESS);    /* Power On */
    return(ERROR);          /* Power Off */
}


/*********************************************************************
*
*             FS__SMC_PHY_Chk_CardExist
*/

int FS__SMC_PHY_Chk_CardExist(FS_u32 id)
{
    char i,j,k;
    if(! FS_SMC_HW_X_ChkStatus(id))                   /***** Not Status Change *****/
        if(FS_SMC_HW_X_ChkCardIn(id)) return(SUCCESS);    /* Card exist in Slot */
    for(i=0,j=0,k=0; i<0x10; i++) {
        if(FS_SMC_HW_X_ChkCardIn(id))                 /***** Status Change *****/
             { j++; k=0; }
        else { j=0; k++; }
        if(j>3)     return(SUCCESS);            /* Card exist in Slot */
        if(k>3)     return(ERROR);              /* NO Card exist in Slot */
        FS_SMC_HW_X_WaitTimer(id,TIME_CDCHK);
    }
    return(ERROR);
}


/*********************************************************************
*
*             FS__SMC_PHY_Chk_CardStsChg
*/

int FS__SMC_PHY_Chk_CardStsChg(FS_u32 id)
{
    if(FS_SMC_HW_X_ChkStatus(id))
        return(ERROR);      /* Status Change */
    return(SUCCESS);        /* Not Status Change */
}


/*********************************************************************
*
*             FS__SMC_PHY_Chk_WP
*/

int FS__SMC_PHY_Chk_WP(FS_u32 id)
{   /* ERROR: WP, SUCCESS: Not WP */
    char i;
    for(i=0; i<0x08; i++) {
        if(FS_SMC_HW_X_ChkWP(id))
            return(ERROR);
        FS_SMC_HW_X_WaitTimer(id,TIME_WPCHK);
    }
    return(SUCCESS);
}


#endif /* FS_USE_SMC_DRIVER */
