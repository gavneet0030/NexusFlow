param(
    [string]$BaseUrl = "http://localhost:9100"
)

$ErrorActionPreference = "Stop"

try {
    $response = Invoke-WebRequest `
        -Uri "$BaseUrl/metrics" `
        -UseBasicParsing `
        -TimeoutSec 5

    if ($response.StatusCode -eq 200) {
        Write-Host "NEXUSFLOW METRICS: PASS"
        exit 0
    }

    Write-Host "NEXUSFLOW METRICS: FAIL"
    exit 1
}
catch {
    Write-Host "NEXUSFLOW METRICS: FAIL"
    Write-Host $_.Exception.Message
    exit 1
}
