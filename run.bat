@echo off
setlocal
set "VOIDLINE_EXE=%~dp0build-windows\voidline\Release\voidline.exe"
if not exist "%VOIDLINE_EXE%" (
    echo Run build.bat from the game folder first.
    exit /b 1
)
"%VOIDLINE_EXE%" %*
exit /b %ERRORLEVEL%
