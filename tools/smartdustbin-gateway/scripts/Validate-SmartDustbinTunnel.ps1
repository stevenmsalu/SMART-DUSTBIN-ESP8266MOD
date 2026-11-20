# Automated check: mock ESP8266 -> gateway -> Quick Tunnel -> public HTTPS (no firmware required).
$ErrorActionPreference = "Stop"

$GatewayRoot = Split-Path $PSScriptRoot -Parent
$MockScript = Join-Path $PSScriptRoot "dev-mock-esp8266.py"
$LogErr = Join-Path $env:TEMP "smartdustbin-cloudflared-verify.err.log"
$LogOut = Join-Path $env:TEMP "smartdustbin-cloudflared-verify.out.log"
$GatewayReadyTimeoutSec = 20
$TunnelUrlTimeoutSec = 60
$PublicHttpsTimeoutSec = 90

$script:MockJob = $null
$script:GatewayJob = $null
$script:TunnelProc = $null

function Stop-ValidationResources {
    if ($null -ne $script:TunnelProc) {
        try {
            if (-not $script:TunnelProc.HasExited) {
                Stop-Process -Id $script:TunnelProc.Id -Force -ErrorAction SilentlyContinue
            }
        } catch {
            # Process may already be gone.
        }
        $script:TunnelProc = $null
    }

    if ($null -ne $script:MockJob) {
        Stop-Job -Job $script:MockJob -ErrorAction SilentlyContinue
        Remove-Job -Job $script:MockJob -Force -ErrorAction SilentlyContinue
        $script:MockJob = $null
    }

    if ($null -ne $script:GatewayJob) {
        Stop-Job -Job $script:GatewayJob -ErrorAction SilentlyContinue
        Remove-Job -Job $script:GatewayJob -Force -ErrorAction SilentlyContinue
        $script:GatewayJob = $null
    }
}

function Get-QuickTunnelUrlFromLogs {
    param([string]$Text)
    if (-not $Text) {
        return $null
    }
    $pattern = 'https://[a-z0-9]+(-[a-z0-9]+)+\.trycloudflare\.com'
    $match = [regex]::Match($Text, $pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($match.Success) {
        return $match.Value
    }
    foreach ($m in [regex]::Matches($Text, 'https://([a-z0-9-]+)\.trycloudflare\.com', 'IgnoreCase')) {
        $label = $m.Groups[1].Value.ToLowerInvariant()
        if ($label -ne "api" -and $label -ne "www" -and $label -match '-') {
            return $m.Value
        }
    }
    return $null
}

function Test-GatewayLocal {
    param(
        [string]$HealthUrl = "http://127.0.0.1:8788/api/health",
        [int]$TimeoutSec = 2
    )
    try {
        $r = Invoke-WebRequest -Uri $HealthUrl -UseBasicParsing -TimeoutSec $TimeoutSec
        return ($r.StatusCode -eq 200)
    } catch {
        return $false
    }
}

Write-Host "=== Validate Smart Dustbin gateway + Quick Tunnel ==="

try {
    $script:MockJob = Start-Job -Name "SmartDustbinValidateMockEsp8266" -ScriptBlock {
        param($Path)
        Set-Location (Split-Path $Path -Parent)
        python $Path
    } -ArgumentList $MockScript
    Start-Sleep -Seconds 1

    $script:GatewayJob = Start-Job -Name "SmartDustbinValidateGateway" -ScriptBlock {
        param($Root)
        Set-Location $Root
        $env:SMARTDUSTBIN_ESP8266_HOST = "127.0.0.1"
        $env:SMARTDUSTBIN_ESP8266_PORT = "8899"
        python -m smartdustbin_gateway
    } -ArgumentList $GatewayRoot

    $deadline = (Get-Date).AddSeconds($GatewayReadyTimeoutSec)
    $localOk = $false
    while ((Get-Date) -lt $deadline) {
        if (Test-GatewayLocal) {
            $localOk = $true
            break
        }
        Start-Sleep -Milliseconds 500
    }

    if (-not $localOk) {
        throw "Gateway did not return HTTP 200 from /api/health on 127.0.0.1:8788 within $GatewayReadyTimeoutSec seconds (mock upstream 127.0.0.1:8899)."
    }

    Write-Host "[OK] Gateway -> mock ESP8266 /api/health"

    $statusLocal = Invoke-WebRequest -Uri "http://127.0.0.1:8788/api/status" -UseBasicParsing -TimeoutSec 5
    if ($statusLocal.StatusCode -ne 200 -or $statusLocal.Content -notmatch "approach") {
        throw "Local GET /api/status failed or body mismatch."
    }
    Write-Host "[OK] Gateway -> mock ESP8266 /api/status"

    if (-not (Get-Command cloudflared -ErrorAction SilentlyContinue)) {
        throw "cloudflared not found on PATH."
    }

    Remove-Item $LogErr, $LogOut -ErrorAction SilentlyContinue
    $script:TunnelProc = Start-Process -FilePath "cloudflared" -ArgumentList @("tunnel", "--url", "http://127.0.0.1:8788") `
        -RedirectStandardError $LogErr -RedirectStandardOutput $LogOut -PassThru -WindowStyle Hidden

    $tunnelUrl = $null
    $waitUntil = (Get-Date).AddSeconds($TunnelUrlTimeoutSec)
    while ((Get-Date) -lt $waitUntil -and -not $tunnelUrl) {
        Start-Sleep -Seconds 2
        $chunks = @()
        if (Test-Path $LogErr) { $chunks += Get-Content $LogErr -Raw -ErrorAction SilentlyContinue }
        if (Test-Path $LogOut) { $chunks += Get-Content $LogOut -Raw -ErrorAction SilentlyContinue }
        $text = ($chunks -join "`n")
        $tunnelUrl = Get-QuickTunnelUrlFromLogs -Text $text
        if ($tunnelUrl -match 'api\.trycloudflare\.com') {
            $tunnelUrl = $null
        }
    }

    if (-not $tunnelUrl) {
        throw "Could not read Quick Tunnel URL from cloudflared log within $TunnelUrlTimeoutSec seconds (check $LogErr)."
    }

    Write-Host "Tunnel URL: $tunnelUrl"

    $healthOk = $false
    $statusOk = $false
    $lastPublicError = ""
    $publicDeadline = (Get-Date).AddSeconds($PublicHttpsTimeoutSec)

    while ((Get-Date) -lt $publicDeadline -and (-not $healthOk -or -not $statusOk)) {
        if (-not $healthOk) {
            try {
                $h = Invoke-WebRequest -Uri "$tunnelUrl/api/health" -UseBasicParsing -TimeoutSec 25
                if ($h.StatusCode -eq 200 -and $h.Content -match "monitoring_only") {
                    $healthOk = $true
                } else {
                    throw "Unexpected /api/health response"
                }
            } catch {
                $lastPublicError = $_.Exception.Message
            }
        }
        if (-not $statusOk) {
            try {
                $s = Invoke-WebRequest -Uri "$tunnelUrl/api/status" -UseBasicParsing -TimeoutSec 25
                if ($s.StatusCode -eq 200 -and $s.Content -match "approach") {
                    $statusOk = $true
                } else {
                    throw "Unexpected /api/status response"
                }
            } catch {
                $lastPublicError = $_.Exception.Message
            }
        }
        if (-not $healthOk -or -not $statusOk) {
            Start-Sleep -Seconds 4
        }
    }

    if (-not $healthOk -or -not $statusOk) {
        throw "Public HTTPS checks failed within $PublicHttpsTimeoutSec s. Last error: $lastPublicError"
    }

    Write-Host "[OK] Public HTTPS -> /api/health" -ForegroundColor Green
    Write-Host "[OK] Public HTTPS -> /api/status" -ForegroundColor Green
    Write-Host "Validation complete."
} catch {
    Write-Host ""
    Write-Host "VALIDATION FAILED: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    Stop-ValidationResources
    Write-Host "Cleanup: validation mock job, gateway job, and cloudflared process (if started) were stopped."
}