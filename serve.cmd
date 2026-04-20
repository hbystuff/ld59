@echo off
cd /d "%~dp0"
start "ld59 server" cmd /k python -m http.server 1337
