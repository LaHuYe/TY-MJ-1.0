@echo off

@REM 可执行文件（Hex）文件路径
set HEX_PATH=G:\Weiai\project\天誉\MJ-1.0\发射板

@REM 定制Hex输出路径
set OUTPUT_PATH=%HEX_PATH%\Output

@REM 软件版本文件路径
set VERSION_FILE_PATH=..\Inc\application.h

@REM 软件版本字符串的格式
set SOFTWARE_VERSION="#define __VERSION__"

@REM 获取系统日期和时间
set YEAR=%DATE:~2,2%
set MONTH=%DATE:~5,2%
set DAY=%DATE:~8,2%
for /f "tokens=1 delims=:" %%A in ("%TIME%") do set HOUR=%%A
if %HOUR% LSS 10 set HOUR=0%HOUR%
set MINUTE=%TIME:~3,2%
set SECOND=%TIME:~6,2%
set CURRENT_DATE=%YEAR%%MONTH%%DAY%_%HOUR%%MINUTE%%SECOND%

@REM 获取软件版本
for /f "tokens=3 delims= " %%i in ('findstr /C:%SOFTWARE_VERSION% %VERSION_FILE_PATH%') do set SW_Ver=%%i
set SW_Ver=%SW_Ver:~1,-1%

@REM 定制Hex文件名
set output_file_name=%SW_Ver%_%CURRENT_DATE%

@REM 显示并复制Hex文件
echo "Output hex file: %OUTPUT_PATH%\%output_file_name%.hex"
copy %HEX_PATH%\MDK-ARM\Objects\MJ-1.hex %OUTPUT_PATH%\%output_file_name%.hex

exit