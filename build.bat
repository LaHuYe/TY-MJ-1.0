@echo off
REM Define path variables
set "keil_path=D:\Weiai\Software\Keil_v5\UV4" 
set "project_root=D:\Weiai\project\ChickenCoopDoor\2.0ScreenVersion"
set "project_path=%project_root%\MDK-ARM"
set "build_output=%project_path%\Build_Output.txt"
set "uvprojx=%project_path%\ChickenCoopDoor.uvprojx"

REM Call the update script
call "%project_root%\commit_hash_update.bat"

REM Set default build parameter to -b
set "build_param=-b"

REM Check if -all parameter exists
if "%1"=="-all" (
    set "build_param=-r"
)

REM Keil build
"%keil_path%\UV4.exe" -j0 %build_param% "%uvprojx%" -o "%build_output%"

REM Use PowerShell to output the build result
powershell -Command "$content = Get-Content '%build_output%' -Encoding UTF8; foreach ($line in $content) { if ($line -match 'warning') { Write-Host $line -ForegroundColor Cyan } elseif ($line -match 'error') { Write-Host $line -ForegroundColor Red } else { Write-Host $line } }"
