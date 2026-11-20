# Local diagnostics: gateway vs ESP8266 upstream (no public tunnel test).
param(
    [string]$GatewayBase = "http://127.0.0.1:8788"
)

$ErrorActionPreference = "Continue"

function Test-Endpoint {
    param(
        [string]$Url,
        [string]$Label,
        [int[]]$AcceptCodes = @(200)
    )
    try {
        $resp = Invoke-WebRequest -Uri $Url -UseBasicParsing -TimeoutSec 5
        if ($AcceptCodes -contains $resp.StatusCode) {
            Write-Host "[OK] $Label -> HTTP $($resp.StatusCode)" -ForegroundColor Green
            return $resp.StatusCode
        }
        Write-Host "[WARN] $Label -> HTTP $($resp.StatusCode) (expected $($AcceptCodes -join ','))" -ForegroundColor Yellow
        return $resp.StatusCode
    } catch {
        $code = $_.Exception.Response.StatusCode.value__
        if ($code) {
            if ($AcceptCodes -contains $code) {
                Write-Host "[OK] $Label -> HTTP $code" -ForegroundColor Green
                return $code
            }
            Write-Host "[..] $Label -> HTTP $code" -ForegroundColor Yellow
            return $code
        }
        Write-Host "[FAIL] $Label -> $($_.Exception.Message)" -ForegroundColor Red
        return 0
    }
}

Write-Host "=== Smart Dustbin local checks ==="
Write-Host "Gateway base: $GatewayBase"
Write-Host "PD origin (CORS): https://kamanishasalle.pdinformationsystems.com"
Write-Host ""

$listenerOk = $false
try {
    $tcp = Test-NetConnection -ComputerName 127.0.0.1 -Port 8788 -WarningAction SilentlyContinue
    if ($tcp.TcpTestSucceeded) {
        Write-Host "[OK] Listener on 127.0.0.1:8788" -ForegroundColor Green
        $listenerOk = $true
    } else {
        Write-Host "[FAIL] Nothing listening on 127.0.0.1:8788" -ForegroundColor Red
    }
} catch {
    Write-Host "[FAIL] Could not probe 127.0.0.1:8788 $($_.Exception.Message)" -ForegroundColor Red
}

if (-not $listenerOk) {
    Write-Host "Run Start-SmartDustbinGateway.ps1"
    exit 1
}

$healthCode = Test-Endpoint "$GatewayBase/api/health" "Gateway /api/health"
$statusCode = Test-Endpoint "$GatewayBase/api/status" "Gateway /api/status"

if ($healthCode -eq 502 -or $statusCode -eq 502) {
    Write-Host ""
    Write-Host "[INFO] HTTP 502: gateway reachable but ESP8266 upstream failed (wrong IP, offline, or not on Wi-Fi)." -ForegroundColor Yellow
} elseif ($healthCode -ne 200 -or $statusCode -ne 200) {
    Write-Host ""
    Write-Host "[WARN] Expected HTTP 200 on both API routes when ESP8266 is online." -ForegroundColor Yellow
}

try {
    Invoke-WebRequest -Uri "$GatewayBase/" -UseBasicParsing -TimeoutSec 3 -ErrorAction Stop
    Write-Host "[WARN] Root path should be 404" -ForegroundColor Yellow
} catch {
    if ($_.Exception.Response.StatusCode.value__ -eq 404) {
        Write-Host "[OK] Gateway rejects / (not proxied to ESP8266)" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "For public HTTPS tests, use the tunnel URL from Start-SmartDustbinTunnel.ps1 output."
Write-Host "Update deployment/pd-hosting/config.js API_BASE_URL when the tunnel URL changes."

if ($healthCode -ne 200 -and $statusCode -ne 200) {
    exit 1
}
exit 0