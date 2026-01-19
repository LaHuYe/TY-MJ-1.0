<#
.SYNOPSIS
串口数据监视脚本，支持动态选择端口、波特率，并自动保存日志到Log文件夹
#>

# 配置参数
$script:port = $null
$script:logToFile = $false
$script:logFilePath = ""
$script:baudRate = 115200
$script:logDir = "Log"  # 日志目录名称

# 创建日志目录（如果不存在）
if (-not (Test-Path $script:logDir)) {
    New-Item -ItemType Directory -Path $script:logDir | Out-Null
    Write-Host "已创建日志目录: $($PWD.Path)\$script:logDir" -ForegroundColor Yellow
}

# 获取可用串口列表
function Get-AvailablePorts {
    $ports = @()
    try {
        if ($IsWindows -or $PSVersionTable.PSVersion.Major -le 5) {
            $ports = [System.IO.Ports.SerialPort]::GetPortNames()
        } else {
            $ports = Get-ChildItem /dev/tty* | Where-Object { $_.Name -match 'tty(USB|ACM|S)' } | Select-Object -ExpandProperty Name
        }
    } catch {}
    return $ports
}

# 显示交互式菜单
function Show-SelectionMenu {
    param(
        [Parameter(Mandatory=$true)]
        [array]$Options,
        [string]$Title = "请选择",
        [string]$Prompt = "输入数字选择"
    )

    Write-Host "`n$Title" -ForegroundColor Cyan
    for ($i=0; $i -lt $Options.Count; $i++) {
        Write-Host "$($i+1). $($Options[$i])"
    }
    do {
        $choice = Read-Host $Prompt
    } while (-not ($choice -match "^\d+$" -and [int]$choice -ge 1 -and [int]$choice -le $Options.Count))
    return $Options[[int]$choice-1]
}

# 主程序
try {
    # 检测可用串口
    $availablePorts = Get-AvailablePorts
    if ($availablePorts.Count -eq 0) {
        Write-Host "未检测到可用串口！" -ForegroundColor Red
        exit
    }

    # 选择串口
    $selectedPort = Show-SelectionMenu -Options $availablePorts -Title "检测到以下串口"

    # 选择波特率
    $commonBaudRates = @(300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 460800, 921600)
    $script:baudRate = Show-SelectionMenu -Options $commonBaudRates -Title "选择波特率"

    # 是否保存日志
    $saveChoice = Read-Host "`n是否保存日志到文件？(y/n) [默认n]"
    if ($saveChoice -match "^[yY]") {
        $script:logToFile = $true
        $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
        $script:logFilePath = "$script:logDir\SerialLog_${timestamp}.txt"
        Write-Host "日志将保存到: $($PWD.Path)\$script:logFilePath" -ForegroundColor Green
    }

    # 初始化串口
    $script:port = New-Object System.IO.Ports.SerialPort $selectedPort,$script:baudRate,None,8,One
    $script:port.Open()

    Write-Host "`n开始监听 $selectedPort @ $script:baudRate baud (按 Ctrl+C 退出)..." -ForegroundColor Green

    # 主循环
    while ($true) {
        if ($script:port.BytesToRead -gt 0) {
            $data = $script:port.ReadExisting()
            $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss.fff"
            $outputLine = "[$timestamp] $data"
            
            Write-Host $outputLine -NoNewline
            
            if ($script:logToFile) {
                $outputLine | Out-File -FilePath $script:logFilePath -Append -Encoding UTF8
            }
        }
        Start-Sleep -Milliseconds 50
    }
} catch {
    Write-Host "`n发生错误: $_" -ForegroundColor Red
} finally {
    if ($script:port -ne $null -and $script:port.IsOpen) {
        $script:port.Close()
        Write-Host "`n串口已关闭" -ForegroundColor Yellow
    }
}