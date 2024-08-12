@echo off
echo Copying files...
xcopy %~dp0\*. %~dp0\..\..\.git\hooks /E /H /C /I /Y
echo %~dp0..\..\.git\hooks\
echo Copy complete.
pause