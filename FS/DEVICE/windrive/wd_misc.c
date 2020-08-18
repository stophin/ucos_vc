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
File        : wd_misc.c
Purpose     : Device Driver using Windows I/O function for logical sector
              access.
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
#include "fs_lbl.h" 
#include "fs_conf.h"
#include "fs_api.h"
#include "fs_clib.h"

#if FS_USE_WINDRIVE_DRIVER

#ifndef _WIN32
#error Driver requires Windows API
#endif /* _WIN32 */

#include <windows.h>


/*********************************************************************
*
*             Local data types
*
**********************************************************************
*/

typedef struct {
  FS_u32 block;
  char buffer[FS_WD_BLOCKSIZE];
} _FS_wd_cachetype;


/*********************************************************************
*
*             Local variables        
*
**********************************************************************
*/

static unsigned char      _workbuffer[512];
static HANDLE             _hdrive[FS_WD_MAXUNIT];
static _FS_wd_cachetype   _FS_wd_cache[FS_WD_MAXUNIT][FS_WD_CACHENUM];
static int                _FS_wd_cache_index[FS_WD_MAXUNIT];
static _FS_wd_cachetype   _FS_wd_wbuffer[FS_WD_MAXUNIT][FS_WD_WBUFFNUM];
static int                _online[FS_WD_MAXUNIT];


/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             _FS_WD_DevStatus
*
  Description:
  FS driver function. Get status of the media.

  Parameters:
  Unit        - Unit number.
 
  Return value:
  ==1 (FS_LBL_MEDIACHANGED) - The media of the device has changed.
  ==0                       - Device okay and ready for operation.
  <0                        - An error has occured.
*/

static int _FS_WD_DevStatus(FS_u32 Unit) {
  int i;

  if (!_online[Unit]) {
    _online[Unit] = 1;
    if (Unit == 0) {
      _hdrive[Unit] = CreateFile(FS_WD_DEV0NAME, 
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                  NULL);
    }
    else {
      _hdrive[Unit] = CreateFile(FS_WD_DEV1NAME, 
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                  NULL);
    }
    for (i = 0; i < FS_WD_CACHENUM; i++) {
      _FS_wd_cache[Unit][i].block = -1;
    }
    _FS_wd_cache_index[Unit] = 0;
    for (i = 0; i < FS_WD_WBUFFNUM; i++) {
      _FS_wd_wbuffer[Unit][i].block = -1;
    }
    return FS_LBL_MEDIACHANGED;
  }
  return 0;
}


/*********************************************************************
*
*             _FS_WD_DevRead
*
  Description:
  FS driver function. Read a sector from the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be read from the device.
  pBuffer     - Pointer to buffer for storing the data.
 
  Return value:
  ==0         - Sector has been read and copied to pBuffer.
  <0          - An error has occured.
*/

static int _FS_WD_DevRead(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  DWORD bytenum;
  BOOL success;
  int i;
  int x;

  x = 0;
  /* check if pBuffer in wbuffer */
  i = 0;
  while (i < FS_WD_WBUFFNUM) {
    if (_FS_wd_wbuffer[Unit][i].block == Sector) {
      FS__CLIB_memcpy(pBuffer, _FS_wd_wbuffer[Unit][i].buffer, 512);
      return 0;
    }
    i++;
  }
  for (i = 0; i < FS_WD_CACHENUM; i++) {
    if (_FS_wd_cache[Unit][i].block == Sector) {
      FS__CLIB_memcpy(pBuffer, _FS_wd_cache[Unit][i].buffer, 512);
      return 0;
    }
  }
  SetFilePointer(_hdrive[Unit],Sector * 512, 0, FILE_BEGIN);
  success = ReadFile(_hdrive[Unit], pBuffer, 512, &bytenum, NULL);
  if (!success) {
    x = -1;
  }
  _FS_wd_cache[Unit][_FS_wd_cache_index[Unit]].block = Sector;
  FS__CLIB_memcpy(_FS_wd_cache[Unit][_FS_wd_cache_index[Unit]].buffer, pBuffer, 512);
  _FS_wd_cache_index[Unit]++;
  if (_FS_wd_cache_index[Unit] >= FS_WD_CACHENUM) {
    _FS_wd_cache_index[Unit] = 0;
  }
  return x;
}


/*********************************************************************
*
*             _FS_WD_DevWrite
*
  Description:
  FS driver function. Write sector to the media.

  Parameters:
  Unit        - Unit number.
  Sector      - Sector to be written to the device.
  pBuffer     - Pointer to data to be stored.
 
  Return value:
  ==0         - Sector has been written to the device.
  <0          - An error has occured.
*/

static int _FS_WD_DevWrite(FS_u32 Unit, FS_u32 Sector, void *pBuffer) {
  DWORD bytenum;
  BOOL success;
  int i;
  int j;
  int x;

  x = 0;
  /* clear in read cache */
  for (i = 0; i < FS_WD_CACHENUM; i++) {
    if (_FS_wd_cache[Unit][i].block == Sector) {
      _FS_wd_cache[Unit][i].block = -1;
    }
  }
  /* check if pBuffer in wbuffer */
  i = 0;
  while (i < FS_WD_WBUFFNUM) {
    if (_FS_wd_wbuffer[Unit][i].block == Sector) {
      FS__CLIB_memcpy(_FS_wd_wbuffer[Unit][i].buffer, pBuffer, 512);
      return 0;
    }
    i++;
  }
  /* check for free wbuffer */
  i = 0;
  while (i < FS_WD_WBUFFNUM) {
    if (_FS_wd_wbuffer[Unit][i].block == -1) {
      FS__CLIB_memcpy(_FS_wd_wbuffer[Unit][i].buffer, pBuffer, 512);
      _FS_wd_wbuffer[Unit][i].block = Sector;
      return 0;
    }
    i++;
  }
  /* write back complete cache  */
  i = 0;
  while (1) {
    if (i >= FS_WD_WBUFFNUM) {
      break;  /* End of cache reached */
    }
    if (_FS_wd_wbuffer[Unit][i].block != -1) {
      break;  /* Valid cache entry found */
    }
    i++;
  }
  while (i < FS_WD_WBUFFNUM) {
    for (j = 0; j < FS_WD_WBUFFNUM; j++) {
      if (_FS_wd_wbuffer[Unit][j].block != -1) {
        if (_FS_wd_wbuffer[Unit][j].block < _FS_wd_wbuffer[Unit][i].block) {
          i = j;
        }
      }
    }
    if (_FS_wd_wbuffer[Unit][i].block != -1) {
      SetFilePointer(_hdrive[Unit], _FS_wd_wbuffer[Unit][i].block * 512, 0, FILE_BEGIN);
      success = WriteFile(_hdrive[Unit], _FS_wd_wbuffer[Unit][i].buffer, 512, &bytenum, NULL);
      if (!success) {
        x = -1;
      }
      _FS_wd_wbuffer[Unit][i].block = -1;
    }
    i = 0;
    while (1) {
      if (i >= FS_WD_WBUFFNUM) {
        break;  /* End of cache reached. */
      }
      if (_FS_wd_wbuffer[Unit][i].block != -1) {
        break;  /* Valid entry found */
      }
      i++;
    }
  } 
  FS__CLIB_memcpy(_FS_wd_wbuffer[Unit][0].buffer, pBuffer, 512);
  _FS_wd_wbuffer[Unit][0].block = Sector;
  return x;
}


/*********************************************************************
*
*             _FS_WD_DevIoCtl
*
  Description:
  FS driver function. Execute device command.

  Parameters:
  Unit        - Unit number.
  Cmd         - Command to be executed.
  Aux         - Parameter depending on command.
  pBuffer     - Pointer to a buffer used for the command.
 
  Return value:
  Command specific. In general a negative value means an error.
*/

static int _FS_WD_DevIoCtl(FS_u32 Unit, FS_i32 Cmd, FS_i32 Aux, void *pBuffer) {
  FS_u32 *info;
  DWORD bytenum;
  BOOL success;
  int i;
  int x;
  int lexp;

  x = 0;
  if (Cmd == FS_CMD_FLUSH_CACHE) {
    if (_online[Unit]) {
      i = 0;
      while (i < FS_WD_WBUFFNUM) {
        if (_FS_wd_wbuffer[Unit][i].block != -1) {
          SetFilePointer(_hdrive[Unit], _FS_wd_wbuffer[Unit][i].block * 512, 0, FILE_BEGIN);
          success = WriteFile(_hdrive[Unit], _FS_wd_wbuffer[Unit][i].buffer, 512, &bytenum, NULL);
          if (!success) {
            x = -1;
          }
          _FS_wd_wbuffer[Unit][i].block = -1;
        }
        i++;
      }
      CloseHandle(_hdrive[Unit]);
      _online[Unit] = 0;
    }
  }
  else if (Cmd == FS_CMD_GET_DEVINFO)  {
    if (!pBuffer) {
      return -1;
    }
    if (!_online[Unit]) {
      if (Unit == 0) {
        _hdrive[Unit] = CreateFile(FS_WD_DEV0NAME, 
				                            GENERIC_READ | GENERIC_WRITE,
				                            FILE_SHARE_READ | FILE_SHARE_WRITE,
				                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
				                            NULL);
      }
      else {
        _hdrive[Unit] = CreateFile(FS_WD_DEV1NAME, 
				                            GENERIC_READ | GENERIC_WRITE,
				                            FILE_SHARE_READ | FILE_SHARE_WRITE,
				                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
				                            NULL);
      }
    }
    SetFilePointer(_hdrive[Unit], 0, 0, FILE_BEGIN);
    success = ReadFile(_hdrive[Unit], _workbuffer, 512, &bytenum, NULL);
    lexp = !success;
    lexp = lexp || (_workbuffer[510] != 0x55);
    lexp = lexp || (_workbuffer[511] != 0xaa);
    if (lexp) {
      if (!_online[Unit]) {
        CloseHandle(_hdrive[Unit]);
      }
      return -1;
    }
    info = pBuffer;
    /* hidden */
    *info = (FS_u32)_workbuffer[28] + 0x100UL * _workbuffer[29] + 0x10000UL * _workbuffer[30] + 0x1000000UL * _workbuffer[31];
    info++;
    /* headnum */
    *info = (FS_u32)_workbuffer[26] + 0x100UL * _workbuffer[27];
    info++;
    /* secpertrk */
    *info = (FS_u32)_workbuffer[24] + 0x100UL * _workbuffer[25];
    info++;
    /* total sectors */
    *info = (FS_u32)_workbuffer[32] + 0x100UL * _workbuffer[33] + 0x10000UL * _workbuffer[34] + 0x1000000UL * _workbuffer[35]
            + _workbuffer[19] + 0x100UL * _workbuffer[20];
    if (!_online[Unit]) {
      CloseHandle(_hdrive[Unit]);
    }
  }
  return x;
}


/*********************************************************************
*
*             Global variables        
*
**********************************************************************
*/

const FS__device_type FS__windrive_driver = {
  "Windrive driver",
  _FS_WD_DevStatus,
  _FS_WD_DevRead,
  _FS_WD_DevWrite,
  _FS_WD_DevIoCtl
};

#endif /* FS_USE_WINDRIVE_DRIVER */


