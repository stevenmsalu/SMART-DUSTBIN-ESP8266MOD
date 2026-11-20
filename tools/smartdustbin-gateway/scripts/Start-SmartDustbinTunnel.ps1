# Cloudflare Quick Tunnel to the local gateway only (never point at the ESP8266).
$ErrorActionPreference = "Stop"

$Target = "http://127.0.0.1:8788"

if (-not (Get-Command cloudflared -ErrorAction SilentlyContinue)) {
    Write-Error "cloudflared is not on PATH. Install Cloudflare Tunnel CLI."
}

try {
    $null = Invoke-WebRequest -Uri "$Target/api/health" -UseBasicParsing -TimeoutSec 2 -ErrorAction Stop
    Write-Host "Gateway responded at $Target/api/health"
} catch {
    $code = $_.Exception.Response.StatusCode.value__
    if ($code -eq 502) {
        Write-Host "Gateway is up at $Target but ESP8266 upstream unavailable (HTTP 502)." -ForegroundColor Yellow
    } else {
        Write-Host "WARNING: No response from $Target yet. Start the gateway first (Start-SmartDustbinGateway.ps1)." -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Starting Cloudflare Quick Tunnel -> $Target"
Write-Host "The public HTTPS URL appears below (changes every restart). Copy it for deployment/pd-hosting/config.js."
Write-Host "Stop with Ctrl+C."
Write-Host ""

cloudflared tunnel --url $Target