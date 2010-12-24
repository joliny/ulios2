# Microsoft Developer Studio Project File - Name="ulios2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ulios2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ulios2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ulios2.mak" CFG="ulios2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ulios2 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /pdb:none /machine:IX86
# SUBTRACT LINK32 /verbose
# Begin Target

# Name "ulios2 - Win32 Debug"
# Begin Group "boot"

# PROP Default_Filter ""
# Begin Group "fat32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\boot\fat32\f32boot.asm
# End Source File
# Begin Source File

SOURCE=.\boot\fat32\f32ldr.c
# End Source File
# End Group
# Begin Group "ulifs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\boot\ulifs\uliboot.asm
# End Source File
# Begin Source File

SOURCE=.\boot\ulifs\ulildr.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\boot\c0.asm
# End Source File
# Begin Source File

SOURCE=.\boot\setup.asm
# End Source File
# Begin Source File

SOURCE=.\boot\setup386.asm
# End Source File
# End Group
# Begin Group "driver"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\driver\athd.c
# End Source File
# Begin Source File

SOURCE=.\driver\basesrv.h
# End Source File
# Begin Source File

SOURCE=.\driver\cui.c
# End Source File
# Begin Source File

SOURCE=.\driver\gdi.c
# End Source File
# Begin Source File

SOURCE=.\driver\kbdmus.c
# End Source File
# Begin Source File

SOURCE=.\driver\Makefile
# End Source File
# Begin Source File

SOURCE=.\driver\rep.c
# End Source File
# Begin Source File

SOURCE=.\driver\speaker.c
# End Source File
# Begin Source File

SOURCE=.\driver\time.c
# End Source File
# Begin Source File

SOURCE=.\driver\uart.c
# End Source File
# Begin Source File

SOURCE=.\driver\vesa.c
# End Source File
# End Group
# Begin Group "fs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\fs\cache.c
# End Source File
# Begin Source File

SOURCE=.\fs\fat32.c
# End Source File
# Begin Source File

SOURCE=.\fs\fs.c
# End Source File
# Begin Source File

SOURCE=.\fs\fs.h
# End Source File
# Begin Source File

SOURCE=.\fs\fsalloc.c
# End Source File
# Begin Source File

SOURCE=.\fs\fsapi.c
# End Source File
# Begin Source File

SOURCE=.\fs\fsapi.h
# End Source File
# Begin Source File

SOURCE=.\fs\Makefile
# End Source File
# Begin Source File

SOURCE=.\fs\ulifs.c
# End Source File
# End Group
# Begin Group "gui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gui\control.c
# End Source File
# Begin Source File

SOURCE=.\gui\control.h
# End Source File
# Begin Source File

SOURCE=.\gui\gui.h
# End Source File
# Begin Source File

SOURCE=.\gui\guialloc.c
# End Source File
# Begin Source File

SOURCE=.\gui\guiapi.c
# End Source File
# Begin Source File

SOURCE=.\gui\guiapi.h
# End Source File
# Begin Source File

SOURCE=.\gui\guiobj.c
# End Source File
# Begin Source File

SOURCE=.\gui\Makefile
# End Source File
# End Group
# Begin Group "MicroKernel"

# PROP Default_Filter ""
# Begin Group "include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MicroKernel\bootdata.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\cintr.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\error.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\exec.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\ipc.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\kalloc.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\knldef.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\page.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\task.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\ulidef.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\ulios.h
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\x86cpu.h
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MicroKernel\cintr.c
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\exec.c
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\global.c
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\head.asm
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\intr.asm
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\ipc.c
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\kalloc.c
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\page.c
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\task.c
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\ulios.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\MicroKernel\Makefile
# End Source File
# Begin Source File

SOURCE=.\MicroKernel\ulios.ld
# End Source File
# End Group
# Begin Group "MkApi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MkApi\app.ld
# End Source File
# Begin Source File

SOURCE=.\MkApi\app_align.ld
# End Source File
# Begin Source File

SOURCE=.\MkApi\app_elf32.ld
# End Source File
# Begin Source File

SOURCE=.\MkApi\apphead.c
# End Source File
# Begin Source File

SOURCE=.\MkApi\ulimkapi.h
# End Source File
# End Group
# Begin Group "tools"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tools\fmtboot.c
# End Source File
# Begin Source File

SOURCE=.\tools\ulifsfmt.c
# End Source File
# End Group
# Begin Group "apps"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\apps\3demo.c
# End Source File
# Begin Source File

SOURCE=.\apps\3dline.c
# End Source File
# Begin Source File

SOURCE=.\apps\clock.c
# End Source File
# Begin Source File

SOURCE=.\apps\cmd.c
# End Source File
# Begin Source File

SOURCE=.\apps\jpg.c
# End Source File
# Begin Source File

SOURCE=.\apps\loader.c
# End Source File
# Begin Source File

SOURCE=.\apps\Makefile
# End Source File
# Begin Source File

SOURCE=.\apps\math.h
# End Source File
# Begin Source File

SOURCE=.\apps\workout.c
# End Source File
# End Group
# End Target
# End Project
