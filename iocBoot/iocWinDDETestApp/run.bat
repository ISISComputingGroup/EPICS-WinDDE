@echo OFF
setlocal
set MYDIR=%~dp0
call %MYDIR%dllPath.bat
%MYDIR%..\..\bin\%EPICS_HOST_ARCH%\WinDDETest.exe st.cmd
endlocal
