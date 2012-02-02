@echo off
cl /DYY_NO_UNISTD_H /Ilibg3d-0.0.8\include /Iglib-dev_2.22.4-1_win32\lib\glib-2.0\include /Iglib-dev_2.22.4-1_win32\include\glib-2.0 libg3d-0.0.8\plugins\import\%1\*.c /c
dumpbin /ALL %1.obj |find "_plugin" > %1.dump

echo EXPORTS > %1.def
set _tmp="test"
for /f "delims='|' tokens=2 " %%i in (%1.dump) do call :sub %1 %%i

link /OUT:"..\bin\Release\libg3d_plugins\%1.dll" /NOLOGO /DEF:"%1.def" /DLL *.obj /LIBPATH:"glib-dev_2.22.4-1_win32\lib" /LIBPATH:"Debug" /MACHINE:X86 /ERRORREPORT:PROMPT ..\bin\Release\libg3d_loader.lib glib-2.0.lib gmodule-2.0.lib gobject-2.0.lib %2
del *.obj
del %1.*

goto :eof

:sub
set _tmp=%2
echo %_tmp:~1% >> %1.def
