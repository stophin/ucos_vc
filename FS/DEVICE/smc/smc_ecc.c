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
File        : smc_ecc.c
Purpose     : ECC functions for SMC driver
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
#ifndef FS_FARCHARPTR
#define FS_FARCHARPTR char *
#endif
#include "fs_dev.h"
#include "fs_conf.h"

#if FS_USE_SMC_DRIVER

#include "smc.h"


/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

#define SUCCESS          0          /* SUCCESS     */
#define ERROR           -1          /* ERROR       */
#define CORRECT          1          /* CORRECTABLE */

#define    BIT7    0x80
#define    BIT6    0x40
#define    BIT5    0x20
#define    BIT4    0x10
#define    BIT3    0x08
#define    BIT2    0x04
#define    BIT1    0x02
#define    BIT0    0x01

#define    BIT1BIT0    0x03
#define    BIT23       0x00800000L
#define    MASK_CPS    0x3f
#define    CORRECTABLE 0x00555554L


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static const unsigned char _FS_SMC_ecctable[256] = {
    0x00,0x55,0x56,0x03,0x59,0x0C,0x0F,0x5A,0x5A,0x0F,0x0C,0x59,0x03,0x56,0x55,0x00,
    0x65,0x30,0x33,0x66,0x3C,0x69,0x6A,0x3F,0x3F,0x6A,0x69,0x3C,0x66,0x33,0x30,0x65,
    0x66,0x33,0x30,0x65,0x3F,0x6A,0x69,0x3C,0x3C,0x69,0x6A,0x3F,0x65,0x30,0x33,0x66,
    0x03,0x56,0x55,0x00,0x5A,0x0F,0x0C,0x59,0x59,0x0C,0x0F,0x5A,0x00,0x55,0x56,0x03,
    0x69,0x3C,0x3F,0x6A,0x30,0x65,0x66,0x33,0x33,0x66,0x65,0x30,0x6A,0x3F,0x3C,0x69,
    0x0C,0x59,0x5A,0x0F,0x55,0x00,0x03,0x56,0x56,0x03,0x00,0x55,0x0F,0x5A,0x59,0x0C,
    0x0F,0x5A,0x59,0x0C,0x56,0x03,0x00,0x55,0x55,0x00,0x03,0x56,0x0C,0x59,0x5A,0x0F,
    0x6A,0x3F,0x3C,0x69,0x33,0x66,0x65,0x30,0x30,0x65,0x66,0x33,0x69,0x3C,0x3F,0x6A,
    0x6A,0x3F,0x3C,0x69,0x33,0x66,0x65,0x30,0x30,0x65,0x66,0x33,0x69,0x3C,0x3F,0x6A,
    0x0F,0x5A,0x59,0x0C,0x56,0x03,0x00,0x55,0x55,0x00,0x03,0x56,0x0C,0x59,0x5A,0x0F,
    0x0C,0x59,0x5A,0x0F,0x55,0x00,0x03,0x56,0x56,0x03,0x00,0x55,0x0F,0x5A,0x59,0x0C,
    0x69,0x3C,0x3F,0x6A,0x30,0x65,0x66,0x33,0x33,0x66,0x65,0x30,0x6A,0x3F,0x3C,0x69,
    0x03,0x56,0x55,0x00,0x5A,0x0F,0x0C,0x59,0x59,0x0C,0x0F,0x5A,0x00,0x55,0x56,0x03,
    0x66,0x33,0x30,0x65,0x3F,0x6A,0x69,0x3C,0x3C,0x69,0x6A,0x3F,0x65,0x30,0x33,0x66,
    0x65,0x30,0x33,0x66,0x3C,0x69,0x6A,0x3F,0x3F,0x6A,0x69,0x3C,0x66,0x33,0x30,0x65,
    0x00,0x55,0x56,0x03,0x59,0x0C,0x0F,0x5A,0x5A,0x0F,0x0C,0x59,0x03,0x56,0x55,0x00
};


static const unsigned char _FS_SMC_cis[]={ 0x01,0x03,0xD9,0x01,0xFF,0x18,0x02,0xDF,0x01,0x20 };



/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_SMC_StringCmp
*/

static int _FS_SMC_StringCmp(FS_FARCHARPTR stringA, FS_FARCHARPTR stringB, int count)
{
    int i;
    for (i=0;i<count;i++)
        if (*stringA++ != *stringB++)   return(ERROR);
    return(SUCCESS);
}

/*********************************************************************
*
*             _FS_SMC_trans_result
*
    Transfer result
    LP14,12,10,... & LP15,13,11,... -> LP15,14,13,... & LP7,6,5,..
*/

static void _FS_SMC_trans_result(unsigned char reg2,unsigned char reg3,
                                  unsigned char *ecc1,unsigned char *ecc2)
{
    unsigned char a;                          /* Working for reg2,reg3        */
    unsigned char b;                          /* Working for ecc1,ecc2        */
    unsigned char i;                          /* For counting                 */

    a=BIT7; b=BIT7;                           /* 80h=10000000b                */
    *ecc1=*ecc2=0;                            /* Clear ecc1,ecc2              */
    for(i=0; i<4; ++i) {
        if ((reg3&a)!=0) *ecc1|=b;            /* LP15,13,11,9 -> ecc1         */
        b=b>>1;                               /* Right shift                  */
        if ((reg2&a)!=0) *ecc1|=b;            /* LP14,12,10,8 -> ecc1         */
        b=b>>1;                               /* Right shift                  */
        a=a>>1;                               /* Right shift                  */
    }
    b=BIT7;                                   /* 80h=10000000b                */
    for(i=0; i<4; ++i) {
        if ((reg3&a)!=0) *ecc2|=b;            /* LP7,5,3,1 -> ecc2            */
        b=b>>1;                               /* Right shift                  */
        if ((reg2&a)!=0) *ecc2|=b;            /* LP6,4,2,0 -> ecc2            */
        b=b>>1;                               /* Right shift                  */
        a=a>>1;                               /* Right shift                  */
    }
}


/*********************************************************************
*
*             _FS_SMC_calculate_ecc
*
    Calculating ECC
    data[0-255] -> ecc1,ecc2,ecc3 using CP0-CP5 code table[0-255]
*/

static void _FS_SMC_calculate_ecc(const unsigned char *table,unsigned char *data,
                                    unsigned char *ecc1,unsigned char *ecc2,
                                    unsigned char *ecc3)
{
    unsigned int i;                           /* For counting                 */
    unsigned char a;                          /* Working for table            */
    unsigned char reg1;                       /* D-all,CP5,CP4,CP3,...        */
    unsigned char reg2;                       /* LP14,LP12,L10,...            */
    unsigned char reg3;                       /* LP15,LP13,L11,...            */

    reg1=reg2=reg3=0;                         /* Clear parameter              */

    for(i=0; i<256; ++i) {
        a=table[data[i]];                     /* Get CP0-CP5 code from table  */
        reg1^=(a&MASK_CPS);                   /* XOR with a                   */
        if ((a&BIT6)!=0) {                    /* If D_all(all bit XOR) = 1    */
            reg3^=(unsigned char)i;           /* XOR with counter             */
            reg2^=~((unsigned char)i);        /* XOR with inv. of counter     */
        }
    }

    /* Trans LP14,12,10,... & LP15,13,11,... -> LP15,14,13,... & LP7,6,5,..   */
    _FS_SMC_trans_result(reg2,reg3,ecc1,ecc2);

    *ecc1=~(*ecc1); *ecc2=~(*ecc2);           /* Inv. ecc2 & ecc3             */
    *ecc3=((~reg1)<<2)|BIT1BIT0;              /* Make TEL format              */
}


/*********************************************************************
*
*             _FS_SMC_correct_data
*/
                             
static unsigned char _FS_SMC_correct_data(unsigned char *data,unsigned char *eccdata,
                                            unsigned char ecc1,unsigned char ecc2,
                                            unsigned char ecc3)
{
    unsigned long l;                          /* Working to check d           */
    unsigned long d;                          /* Result of comparison         */
    unsigned int i;                           /* For counting                 */
    unsigned char d1,d2,d3;                   /* Result of comparison         */
    unsigned char a;                          /* Working for add              */
    unsigned char add;                        /* Byte address of cor. DATA    */
    unsigned char b;                          /* Working for bit              */
    unsigned char bit;                        /* Bit address of cor. DATA     */

    d1=ecc1^eccdata[1]; d2=ecc2^eccdata[0];   /* Compare LP's                 */
    d3=ecc3^eccdata[2];                       /* Comapre CP's                 */
    d=((unsigned long)d1<<16)                 /* Result of comparison         */
        +((unsigned long)d2<<8)
        +(unsigned long)d3;

    if (d==0) return(0);                      /* If No error, return          */
    if (((d^(d>>1))&CORRECTABLE)==CORRECTABLE) {    /* If correctable         */
        l=BIT23;
        add=0;                                /* Clear parameter              */
        a=BIT7;
        for(i=0; i<8; ++i) {                  /* Checking 8 bit               */
            if ((d&l)!=0) add|=a;             /* Make byte address from LP's  */
            l>>=2; a>>=1;                     /* Right Shift                  */
        }
        bit=0;                                /* Clear parameter              */
        b=BIT2;
        for(i=0; i<3; ++i) {                  /* Checking 3 bit               */
            if ((d&l)!=0) bit|=b;             /* Make bit address from CP's   */
            l>>=2; b>>=1;                     /* Right shift                  */
        }
        b=BIT0;
        data[add]^=(b<<bit);                  /* Put corrected data           */
        return(1);
    }
    i=0;                                      /* Clear count                  */
    d&=0x00ffffffL;                           /* Masking                      */
    while(d) {                                /* If d=0 finish counting       */
        if (d&BIT0) ++i;                      /* Count number of 1 bit        */
        d>>=1;                                /* Right shift                  */
    }
    if (i==1) {                               /* If ECC error                 */
        eccdata[1]=ecc1; eccdata[0]=ecc2;    /* Put right ECC code           */
        eccdata[2]=ecc3;
        return(2);
    }
    return(3);                                /* Uncorrectable error          */
}


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__SMC_ECC_Chk_CISdata
*/

int FS__SMC_ECC_Chk_CISdata(unsigned char *buf, unsigned char *redundant)
{
    unsigned char ecc1,ecc2,ecc3;
    unsigned int err;
    _FS_SMC_calculate_ecc(_FS_SMC_ecctable,buf,&ecc1,&ecc2,&ecc3);
    err=_FS_SMC_correct_data(buf,redundant+0x0D,ecc1,ecc2,ecc3);
    if(err==0 || err==1 || err==2)
        return(_FS_SMC_StringCmp((FS_FARCHARPTR)buf,(FS_FARCHARPTR)_FS_SMC_cis,10));
    buf+=0x100;
    _FS_SMC_calculate_ecc(_FS_SMC_ecctable,buf,&ecc1,&ecc2,&ecc3);
    err=_FS_SMC_correct_data(buf,redundant+0x08,ecc1,ecc2,ecc3);
    if(err==0 || err==1 || err==2)
        return(_FS_SMC_StringCmp((FS_FARCHARPTR)buf,(FS_FARCHARPTR)_FS_SMC_cis,10));
    return(ERROR);
}


/*********************************************************************
*
*             FS__SMC_ECC_Chk_ECCdata
*/

int FS__SMC_ECC_Chk_ECCdata(unsigned char *buf, unsigned char *redundant)
{
    unsigned char ecc1,ecc2,ecc3;
    unsigned int err, corr=SUCCESS;
    _FS_SMC_calculate_ecc(_FS_SMC_ecctable,buf,&ecc1,&ecc2,&ecc3);
    err=_FS_SMC_correct_data(buf,redundant+0x0D,ecc1,ecc2,ecc3);
    if(err==1 || err==2)    corr=CORRECT;
    else if(err)            return(ERROR);
    buf+=0x100;
    _FS_SMC_calculate_ecc(_FS_SMC_ecctable,buf,&ecc1,&ecc2,&ecc3);
    err=_FS_SMC_correct_data(buf,redundant+0x08,ecc1,ecc2,ecc3);
    if(err==1 || err==2)    corr=CORRECT;
    else if(err)            return(ERROR);
    return(corr);
}


/*********************************************************************
*
*             FS__SMC_ECC_Set_ECCdata
*/

void FS__SMC_ECC_Set_ECCdata(unsigned char *buf, unsigned char *redundant)
{
    _FS_SMC_calculate_ecc(_FS_SMC_ecctable,buf,redundant+0x0E,redundant+0x0D,redundant+0x0F);
    buf+=0x100;
    _FS_SMC_calculate_ecc(_FS_SMC_ecctable,buf,redundant+0x09,redundant+0x08,redundant+0x0A);
}

#endif /* FS_USE_SMC_DRIVER */
