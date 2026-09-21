$ErrorActionPreference = "Stop"

$container = "nexusflow-redpanda"
$topic = "nexusflow-events"

$eventCount = 100

Write-Host "============================================================"
Write-Host "NEXUSFLOW - REDPANDA STREAMING SMOKE TEST"
Write-Host "============================================================"

Write-Host ""
Write-Host "Broker:"
docker exec $container rpk cluster info --brokers localhost:9092

if ($LASTEXITCODE -ne 0) {
    Write-Host "BROKER CONNECTIVITY: FAILED"
    return
}

Write-Host ""
Write-Host "Topic metadata:"
docker exec $container rpk topic describe $topic --brokers localhost:9092

if ($LASTEXITCODE -ne 0) {
    Write-Host "TOPIC CHECK: FAILED"
    return
}

Write-Host ""
Write-Host "Producing $eventCount test events..."

$producerLines = @()

for ($i = 0; $i -lt $eventCount; $i++) {

    $timestamp = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()

    $payload = @{
        event_id = $i
        timestamp_ms = $timestamp
        source = "nexusflow-smoke-test"
        event_type = "transaction"
        value = [Math]::Round((($i + 1) * 1.25), 4)
        priority = "normal"
    } | ConvertTo-Json -Compress

    $producerLines += $payload
}

$producerFile = Join-Path $env:TEMP "nexusflow_redpanda_events.jsonl"

$producerLines | Set-Content -Path $producerFile -Encoding UTF8

docker cp $producerFile "${container}:/tmp/nexusflow_events.jsonl"

if ($LASTEXITCODE -ne 0) {
    Write-Host "EVENT COPY: FAILED"
    return
}

docker exec $container sh -c "cat /tmp/nexusflow_events.jsonl | rpk topic produce $topic --brokers localhost:9092"

if ($LASTEXITCODE -ne 0) {
    Write-Host "PRODUCER: FAILED"
    return
}

Write-Host "PRODUCER: PASS"

Write-Host ""
Write-Host "Reading topic offsets..."

docker exec $container rpk topic describe $topic --brokers localhost:9092

if ($LASTEXITCODE -ne 0) {
    Write-Host "OFFSET CHECK: FAILED"
    return
}

Write-Host ""
Write-Host "Consuming $eventCount events..."

$consumeOutput = docker exec $container rpk topic consume `
    $topic `
    --brokers localhost:9092 `
    --num $eventCount `
    --format '%v'

if ($LASTEXITCODE -ne 0) {
    Write-Host "CONSUMER: FAILED"
    return
}

$received = @(
    $consumeOutput |
    Where-Object {
        $_ -and
        $_.Trim().StartsWith("{")
    }
)

$receivedCount = $received.Count

Write-Host ""
Write-Host "EXPECTED EVENTS: $eventCount"
Write-Host "RECEIVED EVENTS: $receivedCount"

if ($receivedCount -ne $eventCount) {

    Write-Host "MESSAGE COUNT INTEGRITY: FAILED"
    return
}

Write-Host "MESSAGE COUNT INTEGRITY: PASS"

Write-Host ""
Write-Host "Validating event IDs..."

$ids = New-Object System.Collections.Generic.HashSet[int]

foreach ($line in $received) {

    try {

        $event = $line | ConvertFrom-Json

        [void]$ids.Add([int]$event.event_id)

    }
    catch {

        Write-Host "Invalid event payload detected."
        Write-Host "PAYLOAD INTEGRITY: FAILED"
        return
    }
}

if ($ids.Count -ne $eventCount) {

    Write-Host "UNIQUE EVENT INTEGRITY: FAILED"
    return
}

Write-Host "UNIQUE EVENT INTEGRITY: PASS"

Remove-Item $producerFile -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "============================================================"
Write-Host "REDPANDA STREAMING SMOKE TEST: PASS"
Write-Host "============================================================"
Write-Host "Broker connectivity: PASS"
Write-Host "Topic metadata: PASS"
Write-Host "Producer: PASS"
Write-Host "Consumer: PASS"
Write-Host "Message count integrity: PASS"
Write-Host "Payload integrity: PASS"
Write-Host "============================================================"
