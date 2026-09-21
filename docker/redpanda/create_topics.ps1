$ErrorActionPreference = "Stop"

$container = "nexusflow-redpanda"

Write-Host "============================================================"
Write-Host "NEXUSFLOW - REDPANDA TOPIC PROVISIONING"
Write-Host "============================================================"

Write-Host ""
Write-Host "Checking broker..."

docker exec $container rpk cluster info --brokers localhost:9092

if ($LASTEXITCODE -ne 0) {
    Write-Host "Broker check FAILED"
    return
}

Write-Host ""
Write-Host "Creating NexusFlow topics..."

$topics = @(
    "nexusflow-events",
    "nexusflow-decisions",
    "nexusflow-dead-letter"
)

foreach ($topic in $topics) {

    Write-Host ""
    Write-Host "Creating topic: $topic"

    docker exec $container rpk topic create $topic `
        --brokers localhost:9092 `
        --partitions 4 `
        --replicas 1

    if ($LASTEXITCODE -ne 0) {

        Write-Host "Topic may already exist: $topic"
    }
}

Write-Host ""
Write-Host "Current topics:"

docker exec $container rpk topic list --brokers localhost:9092

Write-Host ""
Write-Host "============================================================"
Write-Host "TOPIC PROVISIONING COMPLETE"
Write-Host "============================================================"
