@echo off
rem Compiles the engine sources, continuing past errors, and prints a per-file error count.
call "%~dp0build.bat" --target engine_objs -- -k 0 > "%TEMP%\gm_engine.log" 2>&1
findstr /R /C:"error C" "%TEMP%\gm_engine.log" > "%TEMP%\gm_engine_errors.log"
for /f %%n in ('find /c /v "" ^< "%TEMP%\gm_engine_errors.log"') do echo %%n errors total; see %TEMP%\gm_engine_errors.log
