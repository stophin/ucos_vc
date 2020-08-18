# Microsoft Developer Studio Project File - Name="ucos_vc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ucos_vc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ucos_vc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ucos_vc.mak" CFG="ucos_vc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ucos_vc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ucos_vc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ucos_vc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "ucos_vc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "OUTPUT/Debug"
# PROP Intermediate_Dir "OUTPUT/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /I "UCOS/CPU" /I "UCOS/SOURCE" /I "PC" /I "UCOS/CONFIG" /I "FS/API" /I "FS/CLIB" /I "FS/CONFIG/WIN32" /I "FS/DEVICE/RAM" /I "FS/FSL/FAT" /I "FS/LBL" /I "FS/OS" /I "UCOS/shell" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib winmm.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /out:"ucos_vc.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ucos_vc - Win32 Release"
# Name "ucos_vc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "UCOS"

# PROP Default_Filter ""
# Begin Group "cpu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UCOS\CPU\OS_CPU.H
# End Source File
# Begin Source File

SOURCE=.\UCOS\CPU\OS_CPU_C.C
# End Source File
# End Group
# Begin Group "core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_CORE.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_FLAG.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_MBOX.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_MEM.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_MUTEX.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_Q.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_SEM.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_TASK.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\OS_TIME.C
# End Source File
# Begin Source File

SOURCE=.\UCOS\SOURCE\uCOS_II.H
# End Source File
# End Group
# Begin Group "cfg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UCOS\Config\INCLUDES.H
# End Source File
# Begin Source File

SOURCE=.\UCOS\Config\OS_CFG.H
# End Source File
# End Group
# Begin Group "shell"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UCOS\Shell\commands.c
# End Source File
# Begin Source File

SOURCE=.\UCOS\Shell\commands.h
# End Source File
# Begin Source File

SOURCE=.\UCOS\Shell\shelltask.c
# End Source File
# Begin Source File

SOURCE=.\UCOS\Shell\shelltask.h
# End Source File
# End Group
# End Group
# Begin Group "FS"

# PROP Default_Filter ""
# Begin Group "API"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FS\API\api_dir.c
# End Source File
# Begin Source File

SOURCE=.\FS\API\api_in.c
# End Source File
# Begin Source File

SOURCE=.\FS\API\api_int.h
# End Source File
# Begin Source File

SOURCE=.\FS\API\api_misc.c
# End Source File
# Begin Source File

SOURCE=.\FS\API\api_out.c
# End Source File
# Begin Source File

SOURCE=.\FS\API\fs_api.h
# End Source File
# Begin Source File

SOURCE=.\FS\API\fs_dev.h
# End Source File
# Begin Source File

SOURCE=.\FS\API\fs_fsl.h
# End Source File
# Begin Source File

SOURCE=.\FS\API\fs_info.c
# End Source File
# Begin Source File

SOURCE=.\FS\API\fs_int.h
# End Source File
# End Group
# Begin Group "CLIB"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FS\CLIB\clibmisc.c
# End Source File
# Begin Source File

SOURCE=.\FS\CLIB\fs_clib.h
# End Source File
# End Group
# Begin Group "CONFIG"

# PROP Default_Filter ""
# Begin Group "WIN32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FS\CONFIG\Win32\fs_conf.h
# End Source File
# Begin Source File

SOURCE=.\FS\CONFIG\Win32\fs_port.h
# End Source File
# End Group
# End Group
# Begin Group "DEVICE"

# PROP Default_Filter ""
# Begin Group "RAM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FS\DEVICE\ram\r_misc.c
# End Source File
# End Group
# End Group
# Begin Group "FSL"

# PROP Default_Filter ""
# Begin Group "FAT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FS\FSL\fat\fat_data.c
# End Source File
# Begin Source File

SOURCE=.\FS\FSL\fat\fat_dir.c
# End Source File
# Begin Source File

SOURCE=.\FS\FSL\fat\fat_in.c
# End Source File
# Begin Source File

SOURCE=.\FS\FSL\fat\fat_ioct.c
# End Source File
# Begin Source File

SOURCE=.\FS\FSL\fat\fat_misc.c
# End Source File
# Begin Source File

SOURCE=.\FS\FSL\fat\fat_open.c
# End Source File
# Begin Source File

SOURCE=.\FS\FSL\fat\fat_out.c
# End Source File
# Begin Source File

SOURCE=.\FS\FSL\fat\fs_fat.h
# End Source File
# End Group
# End Group
# Begin Group "LBL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FS\LBL\fs_lbl.h
# End Source File
# Begin Source File

SOURCE=.\FS\LBL\lb_misc.c
# End Source File
# End Group
# Begin Group "OS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FS\OS\fs_os.h
# End Source File
# Begin Source File

SOURCE=.\FS\OS\fs_x_ucos_ii.c
# End Source File
# End Group
# End Group
# Begin Group "PC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\PC\pc.c
# End Source File
# Begin Source File

SOURCE=.\PC\pc.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\main.c
# End Source File
# End Group
# End Target
# End Project
