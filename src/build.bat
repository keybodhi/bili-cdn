@echo off
setlocal
cd /d "%~dp0"

set "VCDIR=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build"
call "%VCDIR%\vcvars64.bat" >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo vcvars64.bat failed
    exit /b 1
)

set "KITBIN=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"
set "KITINC=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0"
set "KITLIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0"

echo === Compiling resource ===
"%KITBIN%\rc.exe" /nologo /fo resource.res resource.rc
if %ERRORLEVEL% neq 0 exit /b 1

echo === Compiling C source ===
cl.exe /nologo /c /O1 /GS- /Gs9999999 /Gy /Gw /utf-8 ^
  /I"%KITINC%\um" /I"%KITINC%\shared" ^
  bilicdn.c
if %ERRORLEVEL% neq 0 exit /b 1

echo === Linking ===
link.exe /nologo ^
  /NODEFAULTLIB ^
  /ENTRY:Entry ^
  /SUBSYSTEM:WINDOWS ^
  /OPT:REF /OPT:ICF ^
  /MANIFEST:NO ^
  /DEBUG:NONE ^
  /INCREMENTAL:NO ^
  kernel32.lib user32.lib gdi32.lib winhttp.lib ^
  /LIBPATH:"%KITLIB%\um\x64" ^
  /LIBPATH:"%KITLIB%\ucrt\x64" ^
  bilicdn.obj resource.res ^
  /OUT:bilicdn.exe
if %ERRORLEVEL% neq 0 exit /b 1

echo.
echo === Size ===
for %%f in (bilicdn.exe) do echo Final: %%~zf bytes

echo.
echo Done: bilicdn.exe
