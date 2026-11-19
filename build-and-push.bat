@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-itch.ps1" -Push %*
set EXITCODE=%ERRORLEVEL%
echo.
if %EXITCODE% NEQ 0 (
    echo Build/Push failed with exit code %EXITCODE%.
)
pause
exit /b %EXITCODE%
