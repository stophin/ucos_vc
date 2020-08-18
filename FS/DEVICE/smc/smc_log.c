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
File        : smc_log.c
Purpose     : Generic logical sector read/write access module for SMC
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


/*********************************************************************
*
*             #define macros
*
**********************************************************************
*/

#define Set_X_Bit(a,b)    (a[(unsigned char)((b)/8)]|= _FS_SMC_BitData[(b)%8])
#define Clr_X_Bit(a,b)    (a[(unsigned char)((b)/8)]&=~_FS_SMC_BitData[(b)%8])
#define Chk_X_Bit(a,b)    (a[(unsigned char)((b)/8)] & _FS_SMC_BitData[(b)%8])


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static const char     _FS_SMC_BitData[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };
static unsigned int   _FS_SMC_ErrCode[FS_SMC_MAXUNIT];
static unsigned char  _FS_SMC_SectBuf[SECTSIZE];
static unsigned char  _FS_SMC_WorkBuf[SECTSIZE];
static unsigned char  _FS_SMC_Redundant[REDTSIZE];
static unsigned char  _FS_SMC_WorkRedund[REDTSIZE];
static unsigned short _FS_SMC_Log2Phy[FS_SMC_MAXUNIT][2][MAX_LOGBLOCK];
static unsigned char  _FS_SMC_Assign[FS_SMC_MAXUNIT][2][MAX_BLOCKNUM/8];
static unsigned short _FS_SMC_AssignStart[FS_SMC_MAXUNIT][2];
static unsigned short _FS_SMC_AssignZone[FS_SMC_MAXUNIT];
static unsigned short _FS_SMC_ReadBlock[FS_SMC_MAXUNIT];
static unsigned short _FS_SMC_WriteBlock[FS_SMC_MAXUNIT];
static unsigned int   _FS_SMC_MediaChange[FS_SMC_MAXUNIT];
static unsigned int   _FS_SMC_SectCopyMode[FS_SMC_MAXUNIT];


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_SMC_Bit_Count
*/

static char _FS_SMC_Bit_Count(unsigned char cdata)
{
    char bitcount=0;
    while(cdata) {
        bitcount+=(cdata &0x01);
        cdata   /=2;
    }
    return(bitcount);
}


/*********************************************************************
*
*             _FS_SMC_Bit_CountWord
*/

static char _FS_SMC_Bit_CountWord(unsigned short cdata)
{
    char bitcount=0;
    while(cdata) {
        bitcount+=(cdata &0x01);
        cdata   /=2;
    }
    return(bitcount);
}

/*********************************************************************
*
*             _FS_SMC_Chk_DataBlank
*/

static int _FS_SMC_Chk_DataBlank(unsigned char *redundant)
{
    char i;
    for(i=0; i<REDTSIZE; i++)
        if(*redundant++!=0xFF)      return(ERROR);
    return(SUCCESS);
}

/*********************************************************************
*
*             _FS_SMC_Chk_FailBlock
*/

static int _FS_SMC_Chk_FailBlock(unsigned char *redundant)
{
    redundant+=REDT_BLOCK;
    if(*redundant==0xFF)            return(SUCCESS);
    if(! *redundant)                return(ERROR);
    if(_FS_SMC_Bit_Count(*redundant)<7)     return(ERROR);
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Chk_CisBlock
*/

static int _FS_SMC_Chk_CisBlock(unsigned char *redundant)
{
    if(! (*(redundant+REDT_ADDR1H)|*(redundant+REDT_ADDR1L)))
        return(SUCCESS);
    if(! (*(redundant+REDT_ADDR2H)|*(redundant+REDT_ADDR2L)))
        return(SUCCESS);
    return(ERROR);
}

/*********************************************************************
*
*             _FS_SMC_Chk_DataStatus
*/

static int _FS_SMC_Chk_DataStatus(unsigned char *redundant)
{
    redundant+=REDT_DATA;
    if(*redundant==0xFF)            return(SUCCESS);
    if(! *redundant)                return(ERROR);
    if(_FS_SMC_Bit_Count(*redundant)<5)     return(ERROR);
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Load_LogBlockAddr
*/

static int _FS_SMC_Load_LogBlockAddr(FS_u32 id,unsigned char *redundant)
{
    unsigned short addr1,addr2;
    addr1=*(redundant+REDT_ADDR1H)*0x100+*(redundant+REDT_ADDR1L);
    addr2=*(redundant+REDT_ADDR2H)*0x100+*(redundant+REDT_ADDR2L);
    if(addr1==addr2)
        if((addr1 &0xF000)==0x1000)
            { FS__SMC_cardparam[id].LogBlock=(addr1 &0x0FFF)/2; return(SUCCESS); }
    if(_FS_SMC_Bit_CountWord(addr1^addr2)>1)            return(ERROR);
    if((addr1 &0xF000)==0x1000)
        if(! (_FS_SMC_Bit_CountWord(addr1) &0x0001))
            { FS__SMC_cardparam[id].LogBlock=(addr1 &0x0FFF)/2; return(SUCCESS); }
    if((addr2 &0xF000)==0x1000)
        if(! (_FS_SMC_Bit_CountWord(addr2) &0x0001))
            { FS__SMC_cardparam[id].LogBlock=(addr2 &0x0FFF)/2; return(SUCCESS); }
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Clr_RedundantData
*/

static void _FS_SMC_Clr_RedundantData(unsigned char *redundant)
{
    char i;
    for(i=0; i<REDTSIZE; i++)   *(redundant+i)=0xFF;
}


/*********************************************************************
*
*             _FS_SMC_Set_LogBlockAddr
*/

static void _FS_SMC_Set_LogBlockAddr(FS_u32 id,unsigned char *redundant)
{
    unsigned short addr;
    *(redundant+REDT_BLOCK)=0xFF;
    *(redundant+REDT_DATA) =0xFF;
    addr=FS__SMC_cardparam[id].LogBlock*2+0x1000;
    if((_FS_SMC_Bit_CountWord(addr)%2)) addr++;
    *(redundant+REDT_ADDR1H)=*(redundant+REDT_ADDR2H)=addr/0x100;
    *(redundant+REDT_ADDR1L)=*(redundant+REDT_ADDR2L)=(unsigned char)addr;
}


/*********************************************************************
*
*             _FS_SMC_Set_FailBlock
*/

static void _FS_SMC_Set_FailBlock(unsigned char *redundant)
{
    char i;
    for(i=0; i<REDTSIZE; i++)
        *redundant++=((i==REDT_BLOCK)?0xF0:0xFF);
}

/*********************************************************************
*
*             _FS_SMC_Set_DataStatus
*/

static void _FS_SMC_Set_DataStatus(unsigned char *redundant)
{
    redundant+=REDT_DATA;
    *redundant=0x00;
}


/*********************************************************************
*
*             _FS_SMC_Init_Media
*/

static void _FS_SMC_Init_Media(FS_u32 id)
{
    _FS_SMC_ErrCode[id]=NO_ERROR;
    _FS_SMC_MediaChange[id]=(unsigned int) ERROR;
    _FS_SMC_SectCopyMode[id]=COMPLETED;
    FS__SMC_PHY_Cnt_Reset(id);
}


/*********************************************************************
*
*             _FS_SMC_Chk_MediaPower
*/

static int _FS_SMC_Chk_MediaPower(FS_u32 id)
{
    if(FS__SMC_PHY_Chk_CardStsChg(id))      _FS_SMC_MediaChange[id]=(unsigned int) ERROR;
    if((! FS__SMC_PHY_Chk_CntPower(id))&&(! _FS_SMC_MediaChange[id]))          return(SUCCESS);
    if(FS__SMC_PHY_Chk_CardExist(id))
        { _FS_SMC_MediaChange[id]=(unsigned int) ERROR; _FS_SMC_ErrCode[id]=ERR_NoSmartMedia; return(ERROR); }
    if(FS__SMC_PHY_Cnt_PowerOn(id))
        { _FS_SMC_MediaChange[id]=(unsigned int) ERROR; _FS_SMC_ErrCode[id]=ERR_NoSmartMedia; return(ERROR); }
    FS__SMC_PHY_Reset(id);
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Chk_MediaWP
*/

static int _FS_SMC_Chk_MediaWP(FS_u32 id)
{
    if(FS__SMC_cardattrib[id].Attribute &MWP)
        { _FS_SMC_ErrCode[id]=ERR_WrtProtect;   return(ERROR); }
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_MarkFail_PhyOneBlock
*/

static int _FS_SMC_MarkFail_PhyOneBlock(FS_u32 id)
{
    unsigned char sect;
    sect=FS__SMC_cardparam[id].Sector;
    _FS_SMC_Set_FailBlock(_FS_SMC_WorkRedund);
    FS__SMC_PHY_WriteRedtMode(id);
    for(FS__SMC_cardparam[id].Sector=0; FS__SMC_cardparam[id].Sector<FS__SMC_cardattrib[id].MaxSectors; FS__SMC_cardparam[id].Sector++)
        if(FS__SMC_PHY_WriteRedtData(id,_FS_SMC_WorkRedund)) {
            FS__SMC_PHY_Reset(id);
            FS__SMC_cardparam[id].Sector=sect;
            _FS_SMC_ErrCode[id]=ERR_HwError;
            return(ERROR);
        }   /* NO Status Check */
    FS__SMC_PHY_Reset(id);
    FS__SMC_cardparam[id].Sector=sect;
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Set_PhyFmtValue
*/

static int _FS_SMC_Set_PhyFmtValue(FS_u32 id)
{
    unsigned short idcode;
    FS__SMC_PHY_ReadID(id,&idcode);
    if(FS__SMC_PHY_Set_Model(id,(unsigned char)idcode))
        return(ERROR);
    if(FS__SMC_PHY_Chk_WP(id))
        FS__SMC_cardattrib[id].Attribute|=WP;
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Search_CIS
*/

static int _FS_SMC_Search_CIS(FS_u32 id,unsigned short *pcis)
{
    FS__SMC_cardparam[id].Zone=0;   FS__SMC_cardparam[id].Sector=0;
    for(FS__SMC_cardparam[id].PhyBlock=0; FS__SMC_cardparam[id].PhyBlock<(FS__SMC_cardattrib[id].MaxBlocks-FS__SMC_cardattrib[id].MaxLogBlocks-1); FS__SMC_cardparam[id].PhyBlock++) {
        if(FS__SMC_PHY_ReadRedtData(id,_FS_SMC_Redundant))
            { FS__SMC_PHY_Reset(id);        return(ERROR); }
        if(! _FS_SMC_Chk_FailBlock(_FS_SMC_Redundant)) {
            if(_FS_SMC_Chk_CisBlock(_FS_SMC_Redundant))
                { FS__SMC_PHY_Reset(id);    return(ERROR); }
            break;
        }
    }
    while(FS__SMC_cardparam[id].Sector<FS__SMC_cardattrib[id].MaxSectors) {
        if(FS__SMC_cardparam[id].Sector)
            if(FS__SMC_PHY_ReadRedtData(id,_FS_SMC_Redundant))
                { FS__SMC_PHY_Reset(id);    return(ERROR); }
        if(! _FS_SMC_Chk_DataStatus(_FS_SMC_Redundant)) {
            if(FS__SMC_PHY_ReadSect(id,_FS_SMC_WorkBuf,_FS_SMC_Redundant))
                { FS__SMC_PHY_Reset(id);    return(ERROR); }
            if(FS__SMC_ECC_Chk_CISdata(_FS_SMC_WorkBuf,_FS_SMC_Redundant))
                { FS__SMC_PHY_Reset(id);    return(ERROR); }
            *pcis=FS__SMC_cardparam[id].PhyBlock;
            FS__SMC_PHY_Reset(id);
            return(SUCCESS);
        }
        FS__SMC_cardparam[id].Sector++;
    }
    FS__SMC_PHY_Reset(id);
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Make_LogTable
*/

static int _FS_SMC_Make_LogTable(FS_u32 id,unsigned short start)
{
    unsigned short block;
    FS__SMC_cardparam[id].Sector=0;
        /*****/
        for(FS__SMC_cardparam[id].LogBlock=0; FS__SMC_cardparam[id].LogBlock<FS__SMC_cardattrib[id].MaxLogBlocks; FS__SMC_cardparam[id].LogBlock++)
            _FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock]=NO_ASSIGN;
        for(FS__SMC_cardparam[id].PhyBlock=0; FS__SMC_cardparam[id].PhyBlock<(MAX_BLOCKNUM/8); FS__SMC_cardparam[id].PhyBlock++)
            _FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].PhyBlock]=0x00;
        /*******************************************************************/
        for(FS__SMC_cardparam[id].PhyBlock=0; FS__SMC_cardparam[id].PhyBlock<FS__SMC_cardattrib[id].MaxBlocks; FS__SMC_cardparam[id].PhyBlock++) {
            if((! FS__SMC_cardparam[id].Zone) && (FS__SMC_cardparam[id].PhyBlock<start)) {
                Set_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],FS__SMC_cardparam[id].PhyBlock);
                continue;
            }
            if(FS__SMC_PHY_ReadRedtData(id,_FS_SMC_Redundant))
                { FS__SMC_PHY_Reset(id); return(ERROR); }
            if(! _FS_SMC_Chk_DataBlank(_FS_SMC_Redundant))
                continue;
            Set_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],FS__SMC_cardparam[id].PhyBlock);
            if(_FS_SMC_Chk_FailBlock(_FS_SMC_Redundant))
                continue;
            if(_FS_SMC_Load_LogBlockAddr(id,_FS_SMC_Redundant))
                continue;
            if(FS__SMC_cardparam[id].LogBlock>=FS__SMC_cardattrib[id].MaxLogBlocks)
                continue;
            if(_FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock]==NO_ASSIGN) {
                _FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock]=FS__SMC_cardparam[id].PhyBlock;
                continue;
            }
            FS__SMC_cardparam[id].Sector=FS__SMC_cardattrib[id].MaxSectors-1;
            if(FS__SMC_PHY_ReadRedtData(id,_FS_SMC_Redundant))
                { FS__SMC_PHY_Reset(id); return(ERROR); }
            FS__SMC_cardparam[id].Sector=0;
            block=FS__SMC_cardparam[id].LogBlock;
            if(! _FS_SMC_Load_LogBlockAddr(id,_FS_SMC_Redundant))
                if(FS__SMC_cardparam[id].LogBlock==block) {
#ifdef L2P_ERR_ERASE    /***************************************************/
                    block=_FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock];
                    _FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock]=FS__SMC_cardparam[id].PhyBlock;
                    FS__SMC_cardparam[id].PhyBlock=block;
                    if(!(FS__SMC_cardattrib[id].Attribute &MWP)) {
                        FS__SMC_PHY_Reset(id);
                        if(FS__SMC_PHY_EraseBlock(id))      return(ERROR);
                        if(FS__SMC_PHY_CheckStatus(id))
                            { if(_FS_SMC_MarkFail_PhyOneBlock(id))    return(ERROR); }
                        else Clr_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],FS__SMC_cardparam[id].PhyBlock);
                    }
                    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock];
#else   /*******************************************************************/
                    _FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock]=FS__SMC_cardparam[id].PhyBlock;
#endif  /*******************************************************************/
                    continue;
                }
#ifdef L2P_ERR_ERASE    /***************************************************/
            if(!(FS__SMC_cardattrib[id].Attribute &MWP)) {
                FS__SMC_PHY_Reset(id);
                if(FS__SMC_PHY_EraseBlock(id))      return(ERROR);
                if(FS__SMC_PHY_CheckStatus(id))
                    { if(_FS_SMC_MarkFail_PhyOneBlock(id))    return(ERROR); }
                else Clr_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],FS__SMC_cardparam[id].PhyBlock);
            }
#endif  /*******************************************************************/
        }
        _FS_SMC_AssignStart[id][FS__SMC_cardparam[id].Zone?1:0]=0;
        /*****/
    FS__SMC_PHY_Reset(id);
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Check_MediaFmt
*/

static int _FS_SMC_Check_MediaFmt(FS_u32 id)
{
    unsigned short cis;
    if(! _FS_SMC_MediaChange[id])               return(SUCCESS);
    _FS_SMC_MediaChange[id]=(unsigned int) ERROR;
    _FS_SMC_SectCopyMode[id]=COMPLETED;
    if(_FS_SMC_Set_PhyFmtValue(id))
        { _FS_SMC_ErrCode[id]=ERR_UnknownMedia; return(ERROR); }
    if(_FS_SMC_Search_CIS(id,&cis))
        { _FS_SMC_ErrCode[id]=ERR_IllegalFmt;   return(ERROR); }
    _FS_SMC_AssignZone[id]=0;
    FS__SMC_cardparam[id].Zone=0;
    if(_FS_SMC_Make_LogTable(id,cis+1))
        { _FS_SMC_ErrCode[id]=ERR_IllegalFmt;   return(ERROR); }
    _FS_SMC_MediaChange[id]=SUCCESS;
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Conv_MediaAddr
*/

static int _FS_SMC_Conv_MediaAddr(FS_u32 id,unsigned long addr)
{
    unsigned long temp;
    unsigned short block;
    unsigned char sect;
    temp          =addr/FS__SMC_cardattrib[id].MaxSectors;
    FS__SMC_cardparam[id].Sector  =addr%FS__SMC_cardattrib[id].MaxSectors;
    FS__SMC_cardparam[id].LogBlock=temp%FS__SMC_cardattrib[id].MaxLogBlocks;
    FS__SMC_cardparam[id].Zone    =temp/FS__SMC_cardattrib[id].MaxLogBlocks;
    if(FS__SMC_cardparam[id].Zone<FS__SMC_cardattrib[id].MaxZones) {
        if(FS__SMC_cardparam[id].Zone!=0 && FS__SMC_cardparam[id].Zone!=_FS_SMC_AssignZone[id]) {
            _FS_SMC_AssignZone[id]=0;
            block=FS__SMC_cardparam[id].LogBlock;
            sect=FS__SMC_cardparam[id].Sector;
            if(_FS_SMC_Make_LogTable(id,0))
                { _FS_SMC_ErrCode[id]=ERR_IllegalFmt;   return(ERROR); }
            _FS_SMC_AssignZone[id]=FS__SMC_cardparam[id].Zone;
            FS__SMC_cardparam[id].LogBlock=block;
            FS__SMC_cardparam[id].Sector=sect;
        }
        _FS_SMC_Clr_RedundantData(_FS_SMC_Redundant);
        _FS_SMC_Set_LogBlockAddr(id,_FS_SMC_Redundant);
        FS__SMC_cardparam[id].PhyBlock=_FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock];
        return(SUCCESS);
    }
    _FS_SMC_ErrCode[id]=ERR_OutOfLBA;
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Inc_MediaAddr
*/

static int _FS_SMC_Inc_MediaAddr(FS_u32 id)
{
    unsigned short block;
    unsigned char sect;
    if(++FS__SMC_cardparam[id].Sector<FS__SMC_cardattrib[id].MaxSectors)
        return(SUCCESS);
    FS__SMC_cardparam[id].Sector=0;
    if(++FS__SMC_cardparam[id].LogBlock<FS__SMC_cardattrib[id].MaxLogBlocks) {
        _FS_SMC_Clr_RedundantData(_FS_SMC_Redundant);
        _FS_SMC_Set_LogBlockAddr(id,_FS_SMC_Redundant);
        FS__SMC_cardparam[id].PhyBlock=_FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock];
        return(SUCCESS);
    }
    FS__SMC_cardparam[id].LogBlock=0;
    if(++FS__SMC_cardparam[id].Zone<FS__SMC_cardattrib[id].MaxZones) {
        if(FS__SMC_cardparam[id].Zone!=_FS_SMC_AssignZone[id]) {
            _FS_SMC_AssignZone[id]=0;
            block=FS__SMC_cardparam[id].LogBlock;
            sect=FS__SMC_cardparam[id].Sector;
            if(_FS_SMC_Make_LogTable(id,0))
                { _FS_SMC_ErrCode[id]=ERR_IllegalFmt;   return(ERROR); }
            _FS_SMC_AssignZone[id]=FS__SMC_cardparam[id].Zone;
            FS__SMC_cardparam[id].LogBlock=block;
            FS__SMC_cardparam[id].Sector=sect;
        }
        _FS_SMC_Clr_RedundantData(_FS_SMC_Redundant);
        _FS_SMC_Set_LogBlockAddr(id,_FS_SMC_Redundant);
        FS__SMC_cardparam[id].PhyBlock=_FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock];
        return(SUCCESS);
    }
    FS__SMC_cardparam[id].Zone=0;
    _FS_SMC_ErrCode[id]=ERR_OutOfLBA;
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Chk_FirstSect
*/

static int _FS_SMC_Chk_FirstSect(FS_u32 id)
{
    if(! FS__SMC_cardparam[id].Sector)
        return(SUCCESS);
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Chk_LastSect
*/

static int _FS_SMC_Chk_LastSect(FS_u32 id)
{
    if(FS__SMC_cardparam[id].Sector<(FS__SMC_cardattrib[id].MaxSectors-1))
        return(ERROR);
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Copy_PhyOneSect
*/

static int _FS_SMC_Copy_PhyOneSect(FS_u32 id)
{
    int i;
    unsigned int err, retry;
    if(_FS_SMC_ReadBlock[id]!=NO_ASSIGN) {
        FS__SMC_cardparam[id].PhyBlock=_FS_SMC_ReadBlock[id];
        for(retry=0; retry<2; retry++) {
            err=SUCCESS;
            if(FS__SMC_PHY_ReadSect(id,_FS_SMC_WorkBuf,_FS_SMC_WorkRedund))
                { _FS_SMC_ErrCode[id]=ERR_HwError;  return(ERROR); }
            if(_FS_SMC_Chk_DataStatus(_FS_SMC_WorkRedund))
                { err=(unsigned int) ERROR;    break; }
            err=FS__SMC_ECC_Chk_ECCdata(_FS_SMC_WorkBuf,_FS_SMC_WorkRedund);
            if(err==CORRECT)    break;
            if(! err)           break;
            _FS_SMC_SectCopyMode[id]=REQ_FAIL;
        }
    } else {
        err=SUCCESS;
        for(i=0; i<SECTSIZE; i++)
            _FS_SMC_WorkBuf[i]=DUMMY_DATA;
        _FS_SMC_Clr_RedundantData(_FS_SMC_WorkRedund);
    }
    _FS_SMC_Set_LogBlockAddr(id,_FS_SMC_WorkRedund);
    if(err==CORRECT)
        FS__SMC_ECC_Set_ECCdata(_FS_SMC_WorkBuf,_FS_SMC_WorkRedund);
    if(err==(unsigned int)ERROR) {
        FS__SMC_ECC_Set_ECCdata(_FS_SMC_WorkBuf,_FS_SMC_WorkRedund);
        _FS_SMC_Set_DataStatus(_FS_SMC_WorkRedund);
    }
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
    if(FS__SMC_PHY_WriteSect(id,_FS_SMC_WorkBuf,_FS_SMC_WorkRedund))
        { _FS_SMC_ErrCode[id]=ERR_HwError;      return(ERROR); }
    if(FS__SMC_PHY_CheckStatus(id))
        { _FS_SMC_ErrCode[id]=ERR_WriteFault;   return(ERROR); }
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_ReadBlock[id];
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Read_PhyOneSect
*/

static int _FS_SMC_Read_PhyOneSect(FS_u32 id,unsigned char *buf)
{
    int i;
    unsigned int err, retry;
    if(FS__SMC_cardparam[id].PhyBlock==NO_ASSIGN) {
        for(i=0; i<SECTSIZE; i++)
            *buf++=DUMMY_DATA;
        return(SUCCESS);
    }
    for(retry=0; retry<2; retry++) {
        if(FS__SMC_PHY_ReadSect(id,buf,_FS_SMC_Redundant))
            { _FS_SMC_ErrCode[id]=ERR_HwError;      return(ERROR); }
        if(_FS_SMC_Chk_DataStatus(_FS_SMC_Redundant))
            { _FS_SMC_ErrCode[id]=ERR_DataStatus;   return(ERROR); }
        if(! (err=FS__SMC_ECC_Chk_ECCdata(buf,_FS_SMC_Redundant)))
            return(SUCCESS);
    }
    if(err==CORRECT)
         _FS_SMC_ErrCode[id]=ERR_CorReadErr;
    else _FS_SMC_ErrCode[id]=ERR_EccReadErr;
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Write_PhyOneSect
*/

static int _FS_SMC_Write_PhyOneSect(FS_u32 id,unsigned char *buf)
{
    FS__SMC_ECC_Set_ECCdata(buf,_FS_SMC_Redundant);
    if(FS__SMC_PHY_WriteSect(id,buf,_FS_SMC_Redundant))
        { _FS_SMC_ErrCode[id]=ERR_HwError;      return(ERROR); }
    if(FS__SMC_PHY_CheckStatus(id))
        { _FS_SMC_ErrCode[id]=ERR_WriteFault;   return(ERROR); }
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Erase_PhyOneBlock
*/

static int _FS_SMC_Erase_PhyOneBlock(FS_u32 id)
{
    if(FS__SMC_PHY_EraseBlock(id))
        { _FS_SMC_ErrCode[id]=ERR_HwError;      return(ERROR); }
    if(FS__SMC_PHY_CheckStatus(id))
        { _FS_SMC_ErrCode[id]=ERR_WriteFault;   return(ERROR); }
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Assign_WriteBlock
*/

static int _FS_SMC_Assign_WriteBlock(FS_u32 id)
{
    _FS_SMC_ReadBlock[id]=FS__SMC_cardparam[id].PhyBlock;
    for(_FS_SMC_WriteBlock[id]=_FS_SMC_AssignStart[id][FS__SMC_cardparam[id].Zone?1:0]; _FS_SMC_WriteBlock[id]<FS__SMC_cardattrib[id].MaxBlocks; _FS_SMC_WriteBlock[id]++)
        if(! Chk_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],_FS_SMC_WriteBlock[id])) {
            Set_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],_FS_SMC_WriteBlock[id]);
            _FS_SMC_AssignStart[id][FS__SMC_cardparam[id].Zone?1:0]=_FS_SMC_WriteBlock[id]+1;
            FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
            _FS_SMC_SectCopyMode[id]=REQ_ERASE;
            return(SUCCESS);
        }
    for(_FS_SMC_WriteBlock[id]=0; _FS_SMC_WriteBlock[id]<_FS_SMC_AssignStart[id][FS__SMC_cardparam[id].Zone?1:0]; _FS_SMC_WriteBlock[id]++)
        if(! Chk_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],_FS_SMC_WriteBlock[id])) {
            Set_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],_FS_SMC_WriteBlock[id]);
            _FS_SMC_AssignStart[id][FS__SMC_cardparam[id].Zone?1:0]=_FS_SMC_WriteBlock[id]+1;
            FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
            _FS_SMC_SectCopyMode[id]=REQ_ERASE;
            return(SUCCESS);
        }
    _FS_SMC_WriteBlock[id]=NO_ASSIGN;
    _FS_SMC_ErrCode[id]=ERR_WriteFault;
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Release_ReadBlock
*/

static int _FS_SMC_Release_ReadBlock(FS_u32 id)
{
    unsigned int mode;
    mode=_FS_SMC_SectCopyMode[id];
    _FS_SMC_SectCopyMode[id]=COMPLETED;
    if(mode==COMPLETED)             return(SUCCESS);
    _FS_SMC_Log2Phy[id][FS__SMC_cardparam[id].Zone?1:0][FS__SMC_cardparam[id].LogBlock]=_FS_SMC_WriteBlock[id];
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_ReadBlock[id];
    if(FS__SMC_cardparam[id].PhyBlock==NO_ASSIGN)
      { FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];  return(SUCCESS); }
    if(mode==REQ_ERASE) {
        if(_FS_SMC_Erase_PhyOneBlock(id)) {
            if(_FS_SMC_ErrCode[id]==ERR_HwError)    return(ERROR);
            if(_FS_SMC_MarkFail_PhyOneBlock(id))  return(ERROR);
        } else Clr_X_Bit(_FS_SMC_Assign[id][FS__SMC_cardparam[id].Zone?1:0],FS__SMC_cardparam[id].PhyBlock);
    } else if(_FS_SMC_MarkFail_PhyOneBlock(id))   return(ERROR);
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Release_WriteBlock
*/

static int _FS_SMC_Release_WriteBlock(FS_u32 id)
{
    _FS_SMC_SectCopyMode[id]=COMPLETED;
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
    if(_FS_SMC_MarkFail_PhyOneBlock(id))      return(ERROR);
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_ReadBlock[id];
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Copy_BlockAll
*/

static int _FS_SMC_Copy_BlockAll(FS_u32 id,unsigned int mode)
{
    unsigned char sect;
    sect=FS__SMC_cardparam[id].Sector;
    if(_FS_SMC_Assign_WriteBlock(id))         return(ERROR);
    if(mode==REQ_FAIL)      _FS_SMC_SectCopyMode[id]=REQ_FAIL;
    for(FS__SMC_cardparam[id].Sector=0; FS__SMC_cardparam[id].Sector<FS__SMC_cardattrib[id].MaxSectors; FS__SMC_cardparam[id].Sector++)
        if(_FS_SMC_Copy_PhyOneSect(id)) {
            if(_FS_SMC_ErrCode[id]==ERR_HwError)    return(ERROR);
            if(_FS_SMC_Release_WriteBlock(id))    return(ERROR);
            _FS_SMC_ErrCode[id]=ERR_WriteFault;
            FS__SMC_cardparam[id].PhyBlock=_FS_SMC_ReadBlock[id];
            FS__SMC_cardparam[id].Sector=sect;
            return(ERROR);
        }
    if(_FS_SMC_Release_ReadBlock(id))         return(ERROR);
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
    FS__SMC_cardparam[id].Sector=sect;
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Copy_BlockHead
*/

static int _FS_SMC_Copy_BlockHead(FS_u32 id)
{
    unsigned char sect;
    sect=FS__SMC_cardparam[id].Sector;
    if(_FS_SMC_Assign_WriteBlock(id))         return(ERROR);
    for(FS__SMC_cardparam[id].Sector=0; FS__SMC_cardparam[id].Sector<sect; FS__SMC_cardparam[id].Sector++)
        if(_FS_SMC_Copy_PhyOneSect(id)) {
            if(_FS_SMC_ErrCode[id]==ERR_HwError)    return(ERROR);
            if(_FS_SMC_Release_WriteBlock(id))    return(ERROR);
            _FS_SMC_ErrCode[id]=ERR_WriteFault;
            FS__SMC_cardparam[id].PhyBlock=_FS_SMC_ReadBlock[id];
            FS__SMC_cardparam[id].Sector=sect;
            return(ERROR);
        }
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
    FS__SMC_cardparam[id].Sector=sect;
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Copy_BlockTail
*/

static int _FS_SMC_Copy_BlockTail(FS_u32 id)
{
    unsigned char sect;
    for(sect=FS__SMC_cardparam[id].Sector; FS__SMC_cardparam[id].Sector<FS__SMC_cardattrib[id].MaxSectors; FS__SMC_cardparam[id].Sector++)
        if(_FS_SMC_Copy_PhyOneSect(id)) {
            if(_FS_SMC_ErrCode[id]==ERR_HwError)    return(ERROR);
            FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
            FS__SMC_cardparam[id].Sector=sect;
            return(ERROR);
        }
    if(_FS_SMC_Release_ReadBlock(id))         return(ERROR);
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
    FS__SMC_cardparam[id].Sector=sect;
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Reassign_BlockHead
*/

static int _FS_SMC_Reassign_BlockHead(FS_u32 id)
{
    unsigned int mode;
    unsigned short block;
    unsigned char sect;
    mode=_FS_SMC_SectCopyMode[id];
    block=_FS_SMC_ReadBlock[id];
    sect=FS__SMC_cardparam[id].Sector;
    if(_FS_SMC_Assign_WriteBlock(id))         return(ERROR);
    _FS_SMC_SectCopyMode[id]=REQ_FAIL;
    for(FS__SMC_cardparam[id].Sector=0; FS__SMC_cardparam[id].Sector<sect; FS__SMC_cardparam[id].Sector++)
        if(_FS_SMC_Copy_PhyOneSect(id)) {
            if(_FS_SMC_ErrCode[id]==ERR_HwError)    return(ERROR);
            if(_FS_SMC_Release_WriteBlock(id))    return(ERROR);
            _FS_SMC_ErrCode[id]=ERR_WriteFault;
            _FS_SMC_SectCopyMode[id]=mode;
            _FS_SMC_WriteBlock[id]=_FS_SMC_ReadBlock[id];
            _FS_SMC_ReadBlock[id]=block;
            FS__SMC_cardparam[id].Sector=sect;
            FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
            return(ERROR);
        }
    if(_FS_SMC_Release_ReadBlock(id))         return(ERROR);
    _FS_SMC_SectCopyMode[id]=mode;
    _FS_SMC_ReadBlock[id]=block;
    FS__SMC_cardparam[id].Sector=sect;
    FS__SMC_cardparam[id].PhyBlock=_FS_SMC_WriteBlock[id];
    return(SUCCESS);
}


/*********************************************************************
*
*             _FS_SMC_Media_ReadOneSect
*/

static int _FS_SMC_Media_ReadOneSect(FS_u32 id,unsigned char *buf)
{
    unsigned int err, retry;
    if(! _FS_SMC_Read_PhyOneSect(id,buf))      return(SUCCESS);
    if(_FS_SMC_ErrCode[id]==ERR_HwError)        return(ERROR);
    if(_FS_SMC_ErrCode[id]==ERR_DataStatus)     return(ERROR);
#ifdef RDERR_REASSIGN   /***************************************************/
    if(FS__SMC_cardattrib[id].Attribute &MWP) {
        if(_FS_SMC_ErrCode[id]==ERR_CorReadErr) return(SUCCESS);
        return(ERROR);
    }
    err=_FS_SMC_ErrCode[id];
    for(retry=0; retry<2; retry++) {
        if(_FS_SMC_Copy_BlockAll(id,(err==ERR_EccReadErr)?REQ_FAIL:REQ_ERASE)) {
            if(_FS_SMC_ErrCode[id]==ERR_HwError) return(ERROR);
            continue;
        }
        _FS_SMC_ErrCode[id]=err;
        if(_FS_SMC_ErrCode[id]==ERR_CorReadErr) return(SUCCESS);
        return(ERROR);
    }
#else   /*******************************************************************/
    if(_FS_SMC_ErrCode[id]==ERR_CorReadErr)     return(SUCCESS);
#endif  /*******************************************************************/
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Media_WriteOneSect
*/

static int _FS_SMC_Media_WriteOneSect(FS_u32 id,unsigned char *buf)
{
    unsigned int retry;
    if(! _FS_SMC_Write_PhyOneSect(id,buf))     return(SUCCESS);
    if(_FS_SMC_ErrCode[id]==ERR_HwError)        return(ERROR);
    for(retry=1; retry<2; retry++) {
        if(_FS_SMC_Reassign_BlockHead(id)) {
            if(_FS_SMC_ErrCode[id]==ERR_HwError) return(ERROR);
            continue;
        }
        if(! _FS_SMC_Write_PhyOneSect(id,buf)) return(SUCCESS);
        if(_FS_SMC_ErrCode[id]==ERR_HwError)    return(ERROR);
    }
    if(_FS_SMC_Release_WriteBlock(id))        return(ERROR);
    _FS_SMC_ErrCode[id]=ERR_WriteFault;
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Media_CopyBlockHead
*/

static int _FS_SMC_Media_CopyBlockHead(FS_u32 id)
{
    unsigned int retry;
    for(retry=0; retry<2; retry++) {
        if(! _FS_SMC_Copy_BlockHead(id))      return(SUCCESS);
        if(_FS_SMC_ErrCode[id]==ERR_HwError)    return(ERROR);
    }
    return(ERROR);
}


/*********************************************************************
*
*             _FS_SMC_Media_CopyBlockTail
*/

static int _FS_SMC_Media_CopyBlockTail(FS_u32 id)
{
    unsigned int retry;
    if(! _FS_SMC_Copy_BlockTail(id))          return(SUCCESS);
    if(_FS_SMC_ErrCode[id]==ERR_HwError)        return(ERROR);
    for(retry=1; retry<2; retry++) {
        if(_FS_SMC_Reassign_BlockHead(id)) {
            if(_FS_SMC_ErrCode[id]==ERR_HwError) return(ERROR);
            continue;
        }
        if(! _FS_SMC_Copy_BlockTail(id))      return(SUCCESS);
        if(_FS_SMC_ErrCode[id]==ERR_HwError)    return(ERROR);
    }
    if(_FS_SMC_Release_WriteBlock(id))        return(ERROR);
    _FS_SMC_ErrCode[id]=ERR_WriteFault;
    return(ERROR);
}


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__SMC_ReadSector
*/

int FS__SMC_ReadSector(FS_u32 id,unsigned long start,unsigned short count,unsigned char *buf)
{
    int i;
    if(_FS_SMC_Chk_MediaPower(id))          return(_FS_SMC_ErrCode[id]);
    if(_FS_SMC_Check_MediaFmt(id))          return(_FS_SMC_ErrCode[id]);
    if(_FS_SMC_Conv_MediaAddr(id,start))    return(_FS_SMC_ErrCode[id]);
    while(1) {
        if(_FS_SMC_Media_ReadOneSect(id,_FS_SMC_SectBuf))
          { _FS_SMC_ErrCode[id]=ERR_EccReadErr; return(_FS_SMC_ErrCode[id]); }
        for(i=0;i<SECTSIZE;i++)
            *buf++=_FS_SMC_SectBuf[i];
        if(--count<=0)      break;
        if(_FS_SMC_Inc_MediaAddr(id))         return(_FS_SMC_ErrCode[id]);
    }
    return(NO_ERROR);
}


/*********************************************************************
*
*             FS__SMC_WriteSector
*/

int FS__SMC_WriteSector(FS_u32 id,unsigned long start,unsigned short count,unsigned char *buf)
{
    int i;
    if(_FS_SMC_Chk_MediaPower(id))          return(_FS_SMC_ErrCode[id]);
    if(_FS_SMC_Check_MediaFmt(id))          return(_FS_SMC_ErrCode[id]);
    if(_FS_SMC_Chk_MediaWP(id))             return(_FS_SMC_ErrCode[id]);
    if(_FS_SMC_Conv_MediaAddr(id,start))    return(_FS_SMC_ErrCode[id]);
    if(_FS_SMC_Chk_FirstSect(id))
        if(_FS_SMC_Media_CopyBlockHead(id))
          { _FS_SMC_ErrCode[id]=ERR_WriteFault; return(_FS_SMC_ErrCode[id]); }
    while(1) {
        if(! _FS_SMC_Chk_FirstSect(id))
            if(_FS_SMC_Assign_WriteBlock(id)) return(_FS_SMC_ErrCode[id]);
        for(i=0;i<SECTSIZE;i++)
            _FS_SMC_SectBuf[i]=*buf++;
        if(_FS_SMC_Media_WriteOneSect(id,_FS_SMC_SectBuf))
          { _FS_SMC_ErrCode[id]=ERR_WriteFault; return(_FS_SMC_ErrCode[id]); }
        if(! _FS_SMC_Chk_LastSect(id)) {
            if(_FS_SMC_Release_ReadBlock(id))
                if(_FS_SMC_ErrCode[id]==ERR_HwError) {
                    _FS_SMC_ErrCode[id]=ERR_WriteFault;
                    return(_FS_SMC_ErrCode[id]);
                }
        }
        if(--count<=0)      break;
        if(_FS_SMC_Inc_MediaAddr(id))         return(_FS_SMC_ErrCode[id]);
    }
    if(! _FS_SMC_Chk_LastSect(id))          return(NO_ERROR);
    if(_FS_SMC_Inc_MediaAddr(id))             return(_FS_SMC_ErrCode[id]);
    if(_FS_SMC_Media_CopyBlockTail(id))
        { _FS_SMC_ErrCode[id]=ERR_WriteFault;   return(_FS_SMC_ErrCode[id]); }
    return(NO_ERROR);
}


/*********************************************************************
*
*             FS__SMC_Init
*/

int FS__SMC_Init(FS_u32 id)
{
    _FS_SMC_Init_Media(id);
    return(NO_ERROR);
}

#endif /* FS_USE_SMC_DRIVER */
