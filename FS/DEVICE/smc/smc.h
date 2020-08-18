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
File        : smc.h
Purpose     : Generic header file for the file system's SMC driver
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef __SMC_H__
#define __SMC_H__

/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

#define SUCCESS          0          /* SUCCESS     */
#define ERROR           -1          /* ERROR       */
#define CORRECT          1          /* CORRECTABLE */


/*********************************************************************
*
*             Error codes
*/

#define NO_ERROR            0x0000  /* NO ERROR                            */
#define ERR_WriteFault      0x0003  /* Peripheral Device Write Fault       */
#define ERR_HwError         0x0004  /* Hardware Error                      */
#define ERR_DataStatus      0x0010  /* DataStatus Error                    */
#define ERR_EccReadErr      0x0011  /* Unrecovered Read Error              */
#define ERR_CorReadErr      0x0018  /* Recovered Read Data with ECC        */
#define ERR_OutOfLBA        0x0021  /* Illegal Logical Block Address       */
#define ERR_WrtProtect      0x0027  /* Write Protected                     */
#define ERR_ChangedMedia    0x0028  /* Medium Changed                      */
#define ERR_UnknownMedia    0x0030  /* Incompatible Medium Installed       */
#define ERR_IllegalFmt      0x0031  /* Medium Format Corrupted             */
#define ERR_NoSmartMedia    0x003A  /* Medium Not Present                  */


/*********************************************************************
*
*             SMC constants
*/

#define K_BYTE          1024    /* Kilo Byte */
#define SECTSIZE        512     /* Sector buffer size */
#define REDTSIZE        16      /* Redundant buffer size */
#define DUMMY_DATA      0xFF    /* No Assign Sector Read Data */


/*********************************************************************
*
*             SMC max values
*/

#define MAX_ZONENUM     0x08    /* Max Zone Numbers in a SmartMedia */
#define MAX_BLOCKNUM    0x0400  /* Max Block Numbers in a Zone */
#define MAX_SECTNUM     0x20    /* Max Sector Numbers in a Block */
#define MAX_LOGBLOCK    1000    /* Max Logical Block Numbers in a Zone */


/*********************************************************************
*
*             Logical to Physical Block Table Data Definition
*/

#define NO_ASSIGN       0xFFFF  /* No Assign Logical Block Address */


/*********************************************************************
*
*             '_FS_SMC_SectCopyMode' Data
*/

#define COMPLETED        0          /* Sector Copy Completed     */
#define REQ_ERASE        1          /* Request Read Block Erase  */
#define REQ_FAIL         2          /* Request Read Block Failed */


/*********************************************************************
*
*             Retry counters
*/

#define RDERR_REASSIGN  1       /* Reassign with Read Error */
#define L2P_ERR_ERASE   1       /* BlockErase for Contradicted L2P Table */


/*********************************************************************
*
*             SMC commands & status
*/

/* SmartMedia Command */
#define WRDATA      0x80
#define READ        0x00
#define READ_REDT   0x50
#define READ1       0x00
#define READ2       0x01
#define READ3       0x50
#define RST_CHIP    0xFF
#define WRITE       0x10
#define ERASE1      0x60
#define ERASE2      0xD0
#define RDSTATUS    0x70
#define READ_ID     0x90

/* SmartMedia Status */
#define WR_FAIL     0x01    /* 0:Pass,          1:Fail */
#define SUSPENDED   0x20    /* 0:Not Suspended, 1:Suspended */
#define READY       0x40    /* 0:Busy,          1:Ready */
#define WR_PRTCT    0x80    /* 0:Protect,       1:Not Protect */


/*********************************************************************
*
*             Timer constants
*/

 /* SmartMedia Busy Time (1bit:0.1ms) */
#define BUSY_PROG   200     /* tPROG  :  20ms ----- Program Time */
#define BUSY_ERASE  4000    /* tBERASE: 400ms ----- Block Erase Time */
#define BUSY_READ   1       /* tR     : 100us ----- Data transfer Time */
#define BUSY_RESET  60      /* tRST   :   6ms ----- Device Resetting Time  */

/* Hardware Timer (1bit:0.1ms) */
#define TIME_PON    3000    /* 300ms ------ Power On Wait Time */
#define TIME_CDCHK  200     /*  20ms ------ Card Check Interval Timer */
#define TIME_WPCHK  50      /*   5ms ------ WP Check Interval Timer */


/*********************************************************************
*
*             Redundant data offset
*/

#define REDT_DATA   0x04
#define REDT_BLOCK  0x05

#define REDT_ADDR1H 0x06
#define REDT_ADDR1L 0x07
#define REDT_ADDR2H 0x0B
#define REDT_ADDR2L 0x0C

#define REDT_ECC10  0x0D
#define REDT_ECC11  0x0E
#define REDT_ECC12  0x0F

#define REDT_ECC20  0x08
#define REDT_ECC21  0x09
#define REDT_ECC22  0x0A


/*********************************************************************
*
*             SmartMedia Model & Attribute
*/

/* SmartMedia Attribute */
#define NOWP        0x00    /* 0... .... No Write Protect */
#define WP          0x80    /* 1... .... Write Protected */
#define MASK        0x00    /* .00. .... NAND MASK ROM Model */
#define FLASH       0x20    /* .01. .... NAND Flash ROM Model */
#define AD3CYC      0x00    /* ...0 .... Address 3-cycle */
#define AD4CYC      0x10    /* ...1 .... Address 4-cycle */
#define BS16        0x00    /* .... 00.. 16page/block */
#define BS32        0x04    /* .... 01.. 32page/block */
#define PS256       0x00    /* .... ..00 256byte/page */
#define PS512       0x01    /* .... ..01 512byte/page */
#define MWP         0x80    /* WriteProtect mask */
#define MFLASH      0x60    /* Flash Rom mask */
#define MADC        0x10    /* Address Cycle */
#define MBS         0x0C    /* BlockSize mask */
#define MPS         0x03    /* PageSize mask */

/* SmartMedia Model */
#define NOSSFDC         0x00    /*   NO SmartMedia */
#define SSFDC1MB        0x01    /*  1MB SmartMedia */
#define SSFDC2MB        0x02    /*  2MB SmartMedia */
#define SSFDC4MB        0x03    /*  4MB SmartMedia */
#define SSFDC8MB        0x04    /*  8MB SmartMedia */
#define SSFDC16MB       0x05    /* 16MB SmartMedia */
#define SSFDC32MB       0x06    /* 32MB SmartMedia */
#define SSFDC64MB       0x07    /* 64MB SmartMedia */
#define SSFDC128MB      0x08    /*128MB SmartMedia */


/*********************************************************************
*
*             Global data types
*
**********************************************************************
*/

struct FS__SMC_ATTRIBTYPE {
    unsigned char  Model;
    unsigned char  Attribute;
    unsigned char  MaxZones;
    unsigned char  MaxSectors;
    unsigned short MaxBlocks;
    unsigned short MaxLogBlocks;
};

struct FS__SMC_PARAMTYPE
{
    unsigned char  Zone;        /* Zone Number */
    unsigned char  Sector;      /* Sector(512byte) Number on Block */
    unsigned short PhyBlock;    /* Physical Block Number on Zone */
    unsigned short LogBlock;    /* Logical Block Number of Zone */
};


/*********************************************************************
*
*             Externals
*
**********************************************************************
*/

extern struct FS__SMC_ATTRIBTYPE FS__SMC_cardattrib[FS_SMC_MAXUNIT];
extern struct FS__SMC_PARAMTYPE  FS__SMC_cardparam[FS_SMC_MAXUNIT];


/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

/*********************************************************************
*
*             smc_ecc.c
*/

int  FS__SMC_ECC_Chk_CISdata(unsigned char *,unsigned char *);
int  FS__SMC_ECC_Chk_ECCdata(unsigned char *,unsigned char *);
void FS__SMC_ECC_Set_ECCdata(unsigned char *,unsigned char *);


/*********************************************************************
*
*             smc_phy.c
*/

void FS__SMC_PHY_Reset(FS_u32 id);
void FS__SMC_PHY_WriteRedtMode(FS_u32 id);
void FS__SMC_PHY_ReadID(FS_u32 id,unsigned short *);
int  FS__SMC_PHY_ReadSect(FS_u32 id,unsigned char *,unsigned char *);
int  FS__SMC_PHY_WriteSect(FS_u32 id,unsigned char *,unsigned char *);
int  FS__SMC_PHY_EraseBlock(FS_u32 id);
int  FS__SMC_PHY_ReadRedtData(FS_u32 id,unsigned char *);
int  FS__SMC_PHY_WriteRedtData(FS_u32 id,unsigned char *);
int  FS__SMC_PHY_CheckStatus(FS_u32 id);
int  FS__SMC_PHY_Set_Model(FS_u32 id,unsigned char);
void FS__SMC_PHY_Cnt_Reset(FS_u32 id);
int  FS__SMC_PHY_Cnt_PowerOn(FS_u32 id);
int  FS__SMC_PHY_Chk_CntPower(FS_u32 id);
int  FS__SMC_PHY_Chk_CardExist(FS_u32 id);
int  FS__SMC_PHY_Chk_CardStsChg(FS_u32 id);
int  FS__SMC_PHY_Chk_WP(FS_u32 id);


/*********************************************************************
*
*             smc_log.c
*/

int FS__SMC_Init(FS_u32 id);
int FS__SMC_ReadSector(FS_u32 id,unsigned long start,unsigned short count,unsigned char *buf);
int FS__SMC_WriteSector(FS_u32 id,unsigned long start,unsigned short count,unsigned char *buf);


#endif /* __SMC_H__ */

