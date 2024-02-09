rem This is in a separate batch file because that's the best way to get travis
rem to open a cmd.exe prompt without causing the job to hang, and for some
rem reason calling bootstrap-vcpkg.bat was also causing it to hang.

git clone https://github.com/microsoft/vcpkg
call .\vcpkg\bootstrap-vcpkg.bat
vcpkg\vcpkg install getopt:x64-windows
