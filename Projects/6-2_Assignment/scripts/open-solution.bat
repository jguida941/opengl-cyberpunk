@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_DIR=%%~fI"
set "SOLUTION_PATH=%PROJECT_DIR%\6-2_Assignment.sln"

if not exist "%SOLUTION_PATH%" (
    echo [ERROR] Solution not found: "%SOLUTION_PATH%"
    exit /b 1
)

start "" "%SOLUTION_PATH%"
echo Opened "%SOLUTION_PATH%"
