# Starts the Smart Dustbin Python gateway on http://127.0.0.1:8788 (loopback only).
$ErrorActionPreference = "Stop"
$GatewayRoot = Split-Path $PSScriptRoot -Parent
Set-Location $GatewayRoot

$configPath = Join-Path $GatewayRoot "config.json"
if (-not (Test-Path $configPath)) {
    Write-Host "Missing config.json. Copy config.example.json and set upstream_host to the ESP8266 IP." -ForegroundColor Yellow
    Write-Host "  Copy-Item config.example.json config.json"
    exit 1
}

Write-Host "Smart Dustbin gateway -> config: $configPath"
Write-Host "Listening on http://127.0.0.1:8788 (Ctrl+C to stop)"
python -m smartdustbin_gateway --config $configPath