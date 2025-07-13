@echo off
setlocal enabledelayedexpansion

set SACPP_FILE=sac++

REM Check if WSL exists
where wsl >nul 2>nul
if errorlevel 1 (
    powershell -ExecutionPolicy Bypass -Command "wsl --install"
    if errorlevel 1 (
        echo WSL installation failed.
        pause
        exit /b
    )
    echo WSL installed. Please reboot and rerun the installer.
    pause
    exit /b
)

REM Check if sac++ exists
if not exist !SACPP_FILE! (
    echo sac++ not found in current directory.
    pause
    exit /b
)

REM Attempt to copy to WSL
wsl mkdir -p ~/sacplusplus >nul 2>&1
if errorlevel 1 (
    echo Failed to create directory in WSL.
    pause
    exit /b
)

wsl rm -f ~/sacplusplus/sac++ >nul 2>&1
wsl cp /mnt/%cd:~0,1%/%cd:~2%\sac++ ~/sacplusplus/ >nul 2>&1
if errorlevel 1 (
    echo Failed to copy sac++ into WSL.
    pause
    exit /b
)

REM Move + chmod
wsl bash -c "chmod +x ~/sacplusplus/sac++ && sudo mv ~/sacplusplus/sac++ /usr/local/bin/sac++" >nul 2>&1
if errorlevel 1 (
    echo Failed to install sac++ into WSL path.
    pause
    exit /b
)

REM Install clang silently if missing
wsl bash -c "if ! command -v clang >/dev/null; then sudo apt update -qq && sudo apt install -y clang >/dev/null; fi" >nul 2>&1
if errorlevel 1 (
    echo Failed to install clang.
    pause
    exit /b
)

echo sac++ installation complete.
pause
