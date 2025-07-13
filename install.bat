@echo off

where wsl >nul 2>nul
if errorlevel 1 (
    powershell -Command "wsl --install"
    exit /b
)

setlocal enabledelayedexpansion
set SACPP_FILE=sac++

if not exist !SACPP_FILE! (
    exit /b
)

wsl mkdir -p ~/sacplusplus >nul 2>&1
wsl rm -f ~/sacplusplus/sac++ >nul 2>&1
wsl cp /mnt/%cd:~0,1%/%cd:~2%\sac++ ~/sacplusplus/ >nul 2>&1
wsl bash -c "chmod +x ~/sacplusplus/sac++ && sudo mv ~/sacplusplus/sac++ /usr/local/bin/sac++" >nul 2>&1
wsl bash -c "if ! command -v clang >/dev/null; then sudo apt update -qq && sudo apt install -y clang >/dev/null; fi" >nul 2>&1

echo sac++ installation complete.
pause