@ECHO OFF
IF "%WIN32_DEV%" == "" SET WIN32_DEV="C:\Libraries"
SET CMAKE_ROOT=%ProgramFiles%\CMake
IF "%LUA_DEV%" == "" SET LUA_DEV="%ProgramFiles(x86)%/Lua/5.1"
SET VSVERSION=16
SET SRCDIR=%CD%
REM CD ..
REM SET ERESSEA=%CD%
REM CD %SRCDIR%

IF exist build-vs%VSVERSION% goto HAVEDIR
mkdir build-vs%VSVERSION%
:HAVEDIR
cd build-vs%VSVERSION%
IF NOT EXIST CMakeCache.txt GOTO NOCACHE
DEL CMakeCache.txt
:NOCACHE
"%CMAKE_ROOT%\bin\cmake.exe" -A Win32 -G "Visual Studio %VSVERSION%" -DCMAKE_PREFIX_PATH="%LUA_DEV%;%WIN32_DEV%" -DCMAKE_MODULE_PATH="%SRCDIR%/cmake/Modules" -DCMAKE_SUPPRESS_REGENERATION=TRUE ..
PAUSE
