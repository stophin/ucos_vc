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
File        : fs_x_embos.c
Purpose     : embOS OS Layer for the file system
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
#include "fs_api.h"
#include "fs_os.h"
#include "fs_conf.h"

#if (FS_OS_EMBOS)

#if (FS_OS_TIME_SUPPORT==1)
#include <time.h>
#endif

#include "rtos.h"


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static OS_RSEMA _FS_fh_sema;
static OS_RSEMA _FS_fop_sema;
static OS_RSEMA _FS_mem_sema;
static OS_RSEMA _FS_dop_sema;

#if FS_POSIX_DIR_SUPPORT
static OS_RSEMA _FS_dirh_sema;
static OS_RSEMA _FS_dirop_sema;
#endif  /* FS_POSIX_DIR_SUPPORT */


/*********************************************************************
*
*             Global functions section
*
**********************************************************************
*/

/*********************************************************************
*
*             FS_X_OS_LockFileHandle
*
  Lock global table _FS_filehandle (fs_info.c).
*/

void FS_X_OS_LockFileHandle(void) {
  OS_Use(&_FS_fh_sema);
}


/*********************************************************************
*
*             FS_X_OS_UnlockFileHandle
*
  Unlock global table _FS_filehandle (fs_conf.c).
*/

void FS_X_OS_UnlockFileHandle(void) {
  OS_Unuse(&_FS_fh_sema);
}


/*********************************************************************
*
*             FS_X_OS_LockFileOp
*
  The filesystem does allow unlimited number of file access 
  operations at the same time. It is not allowed to access
  a file during an operation to it is pending. Because different
  fp's may access the same file, an implementation has to check
  fp->fileid_lo, fp->fileid_hi and fp->fileid_ex, if it wants
  to allow multiple file access operations at the same time.

  This implementation does allow only one file access
  operation at the same time.
*/

void FS_X_OS_LockFileOp(FS_FILE *fp) {
  OS_Use(&_FS_fop_sema);
}


/*********************************************************************
*
*             FS_X_OS_UnlockFileOp
*
  Please see FS_X_OS_LockFileOp.
*/

void FS_X_OS_UnlockFileOp(FS_FILE *fp) {
  OS_Unuse(&_FS_fop_sema);
}


/*********************************************************************
*
*             FS_X_OS_LockMem
*
  Lock global table _FS_memblock (fat_misc.c).
*/

void FS_X_OS_LockMem(void) {
  OS_Use(&_FS_mem_sema);
}


/*********************************************************************
*
*             FS_X_OS_UnlockMem
*
    Unlock global table _FS_memblock (fat_misc.c).
*/

void FS_X_OS_UnlockMem(void) {
  OS_Unuse(&_FS_mem_sema);
}


/*********************************************************************
*
*             FS_X_OS_LockDeviceOp
*
  The filesystem does allow unlimited number of device access 
  operations at the same time. It is not allowed to access
  the same driver and unit during an operation to it is already
  pending. 
   
  This implementation does allow only one device access
  operation at the same time.
*/

void FS_X_OS_LockDeviceOp(const FS__device_type *driver, FS_u32 id) {
  OS_Use(&_FS_dop_sema);
}


/*********************************************************************
*
*             FS_X_OS_UnlockDeviceOp
*
    Please see FS_X_OS_LockDeviceOp.
*/

void FS_X_OS_UnlockDeviceOp(const FS__device_type *driver, FS_u32 id) {
  OS_Unuse(&_FS_dop_sema);
}

#if FS_POSIX_DIR_SUPPORT

/*********************************************************************
*
*             FS_X_OS_LockDirHandle
*
  Lock global table _FS_dirhandle (api_dir.c).
*/

void FS_X_OS_LockDirHandle(void) {
  OS_Use(&_FS_dirh_sema);
}


/*********************************************************************
*
*             FS_X_OS_UnlockDirHandle
*
  Unlock global table _FS_dirhandle (api_dir.c).
*/

void FS_X_OS_UnlockDirHandle(void) {
  OS_Unuse(&_FS_dirh_sema);
}


/*********************************************************************
*
*             FS_X_OS_LockDirOp
*
  The filesystem does allow unlimited number of directory access 
  operations at the same time. It is not allowed to access
  a directory during an operation to it is pending. Because different
  dirp's may access the same directory, an implementation has to check
  dirp->dirid_lo, dirp->dirid_hi and dirp->dirid_ex, if it wants
  to allow multiple directory access operations at the same time.

  This implementation does allow only one directory access
  operation at the same time.
*/

void FS_X_OS_LockDirOp(FS_DIR *dirp) {
  OS_Use(&_FS_dirop_sema);
}


/*********************************************************************
*
*             FS_X_OS_UnlockDirOp
*
  Please see FS_X_OS_LockDirOp.
*/

void FS_X_OS_UnlockDirOp(FS_DIR *dirp) {
  OS_Unuse(&_FS_dirop_sema);
}

#endif  /* FS_POSIX_DIR_SUPPORT */


/*********************************************************************
*
*             FS_X_OS_GetDate
*/

FS_u16 FS_X_OS_GetDate(void) {
  FS_u16 fdate;
#if FS_OS_TIME_SUPPORT
  time_t t;
  struct tm *ltime;

  time(&t);
  ltime = localtime(&t);
  fdate = ltime->tm_mday;
  fdate += ((FS_u16) (ltime->tm_mon+1) << 5);
  fdate += ((FS_u16) (ltime->tm_year-80) << 9);
#else
  fdate = 1;
  fdate += ((FS_u16) 1 << 5);
  fdate += ((FS_u16) 0 << 9);
#endif /* FS_OS_TIME_SUPPORT */
  return fdate;
}


/*********************************************************************
*
*             FS_X_OS_GetTime
*/

FS_u16 FS_X_OS_GetTime(void) {
  FS_u16 ftime;
#if FS_OS_TIME_SUPPORT
  time_t t;
  struct tm *ltime;

  time(&t);
  ltime = localtime(&t);
  ftime = ltime->tm_sec/2;
  ftime += ((FS_u16) ltime->tm_min << 5);
  ftime += ((FS_u16) ltime->tm_hour << 11);
#else
  ftime = 0;
  ftime += ((FS_u16) 0 << 5);
  ftime += ((FS_u16) 0 << 11);
#endif /* FS_OS_TIME_SUPPORT */
  return ftime;
}


/*********************************************************************
*
*             FS_X_OS_init
*/

int FS_X_OS_Init(void) {
  OS_CREATERSEMA(&_FS_fh_sema);
  OS_CREATERSEMA(&_FS_fop_sema);
  OS_CREATERSEMA(&_FS_mem_sema);
  OS_CREATERSEMA(&_FS_dop_sema);
#if FS_POSIX_DIR_SUPPORT
  OS_CREATERSEMA(&_FS_dirh_sema);
  OS_CREATERSEMA(&_FS_dirop_sema);
#endif  /* FS_POSIX_DIR_SUPPORT */  
  return 0;
}


/*********************************************************************
*
*             FS_X_OS_Exit
*/

int FS_X_OS_Exit(void) {
  return 0;
}

#endif /* FS_OS_EMBOS */


