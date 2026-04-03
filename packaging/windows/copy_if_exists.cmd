@echo off
if exist "%~1" (
    copy /Y "%~1" "%~2" >nul
) else (
    echo Warning: Missing "%~1"
)
exit /b 0
