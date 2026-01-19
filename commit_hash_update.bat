@echo off
cd /D G:\Weiai\project\积安\G13

REM 从 Git 获取最新的提交哈希
for /f %%i in ('git rev-parse HEAD') do set __COMMIT_HASH__=%%i
echo __COMMIT_HASH__: %__COMMIT_HASH__%

REM 更新 application.h 文件
powershell -Command "$content = (Get-Content Inc\application.h -Encoding UTF8) -replace '#define __COMMIT_HASH__\s+\"[\s\Sa-zA-Z0-9]+\"', ('#define __COMMIT_HASH__ \"%__COMMIT_HASH__%\"'); Set-Content -Path Inc\application.h -Value $content -Encoding UTF8"

