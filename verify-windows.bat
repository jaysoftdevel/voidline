@echo off
setlocal
set "SDL_AUDIODRIVER=dummy"
call "%~dp0run.bat" --dx-report
if errorlevel 1 exit /b %ERRORLEVEL%
call "%~dp0run.bat" --renderer dx12 --windowed --smoke-test --scene 20 --frames 3 --width 960 %*
exit /b %ERRORLEVEL%
