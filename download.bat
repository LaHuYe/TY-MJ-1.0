@echo off
REM Define path variables
set "keil_path=D:\Weiai\Software\Keil_v5\UV4"
set "project_path=G:\Weiai\project\»ý°²\G13\MDK-ARM"
set "uvprojx=%project_path%\JIAN_G13.uvprojx"
set "output_file=%project_path%\Prg_Output.txt"

REM Download Keil5 project
"%keil_path%\UV4.exe" -f "%uvprojx%" -o "%output_file%"

REM Use PowerShell to output the compilation result
powershell -Command "$content = Get-Content '%output_file%' -Encoding UTF8; foreach ($line in $content) { if ($line -match 'warning') { Write-Host $line -ForegroundColor Cyan } elseif ($line -match 'error') { Write-Host $line -ForegroundColor Red } else { Write-Host $line } }"
