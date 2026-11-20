# Opens two visible console windows: gateway, then Quick Tunnel.
$ErrorActionPreference = "Stop"

$ScriptsDir = $PSScriptRoot
$GatewayScript = Join-Path $ScriptsDir "Start-SmartDustbinGateway.ps1"
$TunnelScript = Join-Path $ScriptsDir "Start-SmartDustbinTunnel.ps1"
$GatewayHealthUrl = "http://127.0.0.1:8788/api/health"
$ReadyTimeoutSec = 20

function Test-GatewayHealthReady {
    param(
        [string]$Url,
        [int]$RequestTimeoutSec = 2
    )
    try {
        $resp = Invoke-WebRequest -Uri $Url -UseBasicParsing -TimeoutSec $RequestTimeoutSec
        return ($resp.StatusCode -eq 200)
    } catch {
        return $false
    }
}

Write-Host "Launching Smart Dustbin gateway (new window)..."
Start-Process powershell -ArgumentList @(
    "-NoExit",
    "-ExecutionPolicy", "Bypass",
    "-File", $GatewayScript
)

Write-Host "Waiting for GET $GatewayHealthUrl -> HTTP 200 (up to $ReadyTimeoutSec s; ESP8266 must be online)..."
$deadline = (Get-Date).AddSeconds($ReadyTimeoutSec)
$ready = $false
while ((Get-Date) -lt $deadline) {
    if (Test-GatewayHealthReady -Url $GatewayHealthUrl) {
        $ready = $true
        break
    }
    Start-Sleep -Milliseconds 500
}

if (-not $ready) {
    Write-Host ""
    Write-Host "FAIL: Gateway did not return HTTP 200 from /api/health on 127.0.0.1:8788." -ForegroundColor Red
    Write-Host "Check: gateway window, config.json upstream_host, ESP8266 power/Wi-Fi."
    Write-Host "Cloudflare Quick Tunnel was NOT started."
    exit 1
}

Write-Host "[OK] Gateway /api/health succeeded locally." -ForegroundColor Green
Write-Host "Launching Cloudflare Quick Tunnel (new window)..."
Start-Process powershell -ArgumentList @(
    "-NoExit",
    "-ExecutionPolicy", "Bypass",
    "-File", $TunnelScript
)

Write-Host ""
Write-Host "Two windows should be open. Shut down in order:"
Write-Host "  1. Ctrl+C in the tunnel window (copy trycloudflare.com URL first)"
Write-Host "  2. Ctrl+C in the gateway window"