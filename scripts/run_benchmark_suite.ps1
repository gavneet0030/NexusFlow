$ErrorActionPreference = "Continue"

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$buildDir = Join-Path $root "build\Debug"
$resultDir = Join-Path $root "benchmarks\results"
$logFile = Join-Path $resultDir "benchmark_suite.log"

New-Item -ItemType Directory -Force $resultDir | Out-Null

"============================================================" | Set-Content $logFile
"NexusFlow Reproducible Benchmark Suite" | Add-Content $logFile
"Started: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Add-Content $logFile
"============================================================" | Add-Content $logFile

$benchmarks = @(
    "worker_scaling_benchmark",
    "backpressure_benchmark",
    "persistent_worker_pool_benchmark",
    "adaptive_worker_integration_benchmark",
    "priority_event_queue_benchmark",
    "queue_latency_experiment",
    "priority_adaptive_pipeline_benchmark",
    "priority_latency_benchmark",
    "adaptive_vs_fixed_benchmark",
    "controlled_workload_benchmark",
    "backpressure_stress_benchmark",
    "latency_under_load_benchmark",
    "queue_comparison_benchmark",
    "resource_utilization_benchmark",
    "fault_recovery_benchmark"
)

$passed = 0
$failed = 0
$skipped = 0

foreach ($benchmark in $benchmarks) {

    $executable = Join-Path $buildDir "$benchmark.exe"

    Write-Host ""
    Write-Host "============================================================"
    Write-Host "Running: $benchmark"
    Write-Host "============================================================"

    Add-Content $logFile ""
    Add-Content $logFile "------------------------------------------------------------"
    Add-Content $logFile "Benchmark: $benchmark"
    Add-Content $logFile "Started: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    Add-Content $logFile "------------------------------------------------------------"

    if (-not (Test-Path $executable)) {

        Write-Host "Executable not found. Skipping."

        Add-Content $logFile "STATUS: SKIPPED"
        Add-Content $logFile "Executable not found: $executable"

        $skipped++
        continue
    }

    & $executable 2>&1 |
        Tee-Object -FilePath $logFile -Append

    $code = $LASTEXITCODE

    if ($code -eq 0) {

        Write-Host "STATUS: PASS"

        Add-Content $logFile "STATUS: PASS"

        $passed++
    }
    else {

        Write-Host "STATUS: FAIL (exit code $code)"

        Add-Content $logFile "STATUS: FAIL"
        Add-Content $logFile "Exit code: $code"

        $failed++
    }
}

Write-Host ""
Write-Host "============================================================"
Write-Host "Benchmark Suite Summary"
Write-Host "============================================================"

Write-Host "Passed:  $passed"
Write-Host "Failed:  $failed"
Write-Host "Skipped: $skipped"

Add-Content $logFile ""
Add-Content $logFile "============================================================"
Add-Content $logFile "Benchmark Suite Summary"
Add-Content $logFile "============================================================"
Add-Content $logFile "Passed:  $passed"
Add-Content $logFile "Failed:  $failed"
Add-Content $logFile "Skipped: $skipped"
Add-Content $logFile "Finished: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"

Write-Host ""
Write-Host "Log written to:"
Write-Host "benchmarks/results/benchmark_suite.log"

if ($failed -gt 0) {
    Write-Host ""
    Write-Host "Some benchmarks failed."
    return
}

Write-Host ""
Write-Host "Benchmark suite completed successfully."
