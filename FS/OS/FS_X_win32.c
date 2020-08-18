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
File        : fs_x_win32.c
Purpose     : Win32 API OS Layer for the file system
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
#include "fs_api.h"
#include "fs_dev.h"
#include "fs_os.h"
#include "fs_conf.h"

#if (FS_OS_WINDOWS)

#if FS_OS_TIME_SUPPORT
#include <time.h>
#endif

#include <windows.h>


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

static HANDLE _FS_fh_sema=NULL;
static HANDLE _FS_fop_sema=NULL;
static HANDLE _FS_mem_sema=NULL;
static HANDLE _FS_dop_sema=NULL;

#if FS_POSIX_DIR_SUPPORT
static HANDLE _FS_dirh_sema=NULL;
static HANDLE _FS_dirop_sema=NULL;
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
  if (_FS_fh_sema==NULL) {
    return;
  }
  WaitForSingleObject(_FS_fh_sema,INFINITE);
}


/*********************************************************************
*
*             FS_X_OS_UnlockFileHandle
*
  Unlock global table _FS_filehandle (fs_info.c).
*/

void FS_X_OS_UnlockFileHandle(void) {
  if (_FS_fh_sema==NULL) {
    return;
  }
  ReleaseSemaphore(_FS_fh_sema, 1,NULL);
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
  if (_FS_fop_sema==NULL) {
    return;
  }
  WaitForSingleObject(_FS_fop_sema,INFINITE);
}


/*********************************************************************
*
*             FS_X_OS_UnlockFileOp
*
  Please see FS_X_OS_LockFileOp.
*/

void FS_X_OS_UnlockFileOp(FS_FILE *fp) {
  if (_FS_fop_sema==NULL) {
    return;
  }
  ReleaseSemaphore(_FS_fop_sema, 1,NULL);
}


/*********************************************************************
*
*             FS_X_OS_LockMem
*
  Lock global table _FS_memblock (fat_misc.c).
*/

void FS_X_OS_LockMem(void) {
  if (_FS_mem_sema==NULL) {
    return;
  }
  WaitForSingleObject(_FS_mem_sema,INFINITE);
}


/*********************************************************************
*
*             FS_X_OS_UnlockMem
*
    Unlock global table _FS_memblock (fat_misc.c).
*/

void FS_X_OS_UnlockMem(void) {
  if (_FS_mem_sema==NULL) {
    return;
  }
  ReleaseSemaphore(_FS_mem_sema, 1,NULL);
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
  if (_FS_dop_sema==NULL) {
    return;
  }
  WaitForSingleObject(_FS_dop_sema,INFINITE);
}


/*********************************************************************
*
*             FS_X_OS_UnlockDeviceOp
*
    Please see FS_X_OS_LockDeviceOp.
*/

void FS_X_OS_UnlockDeviceOp(const FS__device_type *driver, FS_u32 id) {
  if (_FS_dop_sema==NULL) {
    return;
  }
  ReleaseSemaphore(_FS_dop_sema, 1,NULL);
}


#if FS_POSIX_DIR_SUPPORT

/*********************************************************************
*
*             FS_X_OS_LockDirHandle
*
  Lock global table _FS_dirhandle (api_dir.c).
*/

void FS_X_OS_LockDirHandle(void) {
  if (_FS_dirh_sema==NULL) {
    return;
  }
  WaitForSingleObject(_FS_dirh_sema,INFINITE);
}


/*********************************************************************
*
*             FS_X_OS_UnlockDirHandle
*
  Unlock global table _FS_dirhandle (api_dir.c).
*/

void FS_X_OS_UnlockDirHandle(void) {
  if (_FS_dirh_sema==NULL) {
    return;
  }
  ReleaseSemaphore(_FS_dirh_sema, 1,NULL);
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
  if (_FS_dirop_sema==NULL) {
    return;
  }
  WaitForSingleObject(_FS_dirop_sema,INFINITE);
}


/*********************************************************************
*
*             FS_X_OS_UnlockDirOp
*
  Please see FS_X_OS_LockDirOp.
*/

void FS_X_OS_UnlockDirOp(FS_DIR *dirp) {
  if (_FS_dirop_sema==NULL) {
    return;
  }
  ReleaseSemaphore(_FS_dirop_sema, 1,NULL);
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
*             FS_X_OS_Init
*/

int FS_X_OS_Init(void) {
  _FS_fh_sema = CreateSemaphore(NULL, 1, 1, NULL);
  if (_FS_fh_sema == NULL) {
    return -1;
  }
  _FS_fop_sema = CreateSemaphore(NULL, 1, 1, NULL);
  if (_FS_fop_sema == NULL) {
    return -1;
  }
  _FS_mem_sema = CreateSemaphore(NULL, 1, 1, NULL);
  if (_FS_mem_sema == NULL) {
    return -1;
  }
  _FS_dop_sema = CreateSemaphore(NULL, 1, 1, NULL);
  if (_FS_dop_sema == NULL) {
    return -1;
  }
#if FS_POSIX_DIR_SUPPORT
  _FS_dirh_sema = CreateSemaphore(NULL, 1, 1, NULL);
  if (_FS_dirh_sema == NULL) {
    return -1;
  }
  _FS_dirop_sema = CreateSemaphore(NULL, 1, 1, NULL);
  if (_FS_dirop_sema == NULL) {
    return -1;
  }
#endif  /* FS_POSIX_DIR_SUPPORT */
  return 0;
}


/*********************************************************************
*
*             FS_OS_Exit
*/

int FS_X_OS_Exit(void) {
  if (_FS_fh_sema!=NULL) {
    CloseHandle(_FS_fh_sema);
  }
  if (_FS_fop_sema!=NULL) {
    CloseHandle(_FS_fop_sema);
  }
  if (_FS_mem_sema!=NULL) {
    CloseHandle(_FS_mem_sema);
  }
  if (_FS_dop_sema!=NULL) {
    CloseHandle(_FS_dop_sema);
  }
#if FS_POSIX_DIR_SUPPORT
  if (_FS_dirh_sema!=NULL) {
    CloseHandle(_FS_dirh_sema);
  }
  if (_FS_dirop_sema!=NULL) {
    CloseHandle(_FS_dirop_sema);
  }
#endif  /* FS_POSIX_DIR_SUPPORT */
  return 0;
}

#endif /* FS_OS_WINDOWS */

