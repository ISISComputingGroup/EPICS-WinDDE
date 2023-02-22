@echo off
tasklist /fi "ImageName eq WinDDEClient.exe" /fo csv 2>NUL | find /I "WinDDEClient.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo Program WinDDEClient is already running in another window
	echo If that one is not working properly, please close that window
	echo and try running this file again
	pause
	exit /b 1
)
:LOOP
%~dp0WinDDEClient.exe
goto LOOP
