py %~dp0\gen_manifest.py || exit /b 1
cl code\build.c /Feprogram.exe /Zi
