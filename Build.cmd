@echo off
cd "%~dp0/src"

rd /q /s "bin"
md "bin"

rd /q /s "obj"
md "obj"

windres.exe -i "Resources\Library.rc" -o "obj\Library.o"
gcc.exe -Oz -s -Wl,--gc-sections,--exclude-all-symbols -shared -municode -nostdlib -e DllMain -DINITGUID "Library.c" "obj\Library.o" -lntdll -lkernel32 -luser32 -o "bin\gamelaunchhelper.dll"