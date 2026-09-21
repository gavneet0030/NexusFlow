$ErrorActionPreference = "Stop"

$root = "C:\Users\PC\NexusFlow"
Set-Location $root

$resultsDir = "$root\benchmarks\results\capacity_sweep"
$metadataDir = "$root\benchmarks\results\metadata"

New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null
New-Item -ItemType Directory -Force -Path $metadataDir | Out-Null

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"

Write-Host "============================================"
Write-Host "NEXUSFLOW: STANDARDIZED CAPACITY BENCHMARK"
Write-Host "============================================"
Write-Host ""

Write-Host "Step 1: Building benchmark..."

& "C:\Program Files\CMake\bin\cmake.exe" `
    --build "$root\build" `
    --config Debug `
    --target adaptive_capacity_sweep_benchmark

if ($LASTEXITCODE -ne 0) {
    throw "Benchmark build failed."
}

Write-Host "BUILD: PASS"
Write-Host ""

Write-Host "Step 2: Running benchmark..."

$rawOutputFile = "$resultsDir\capacity_sweep_$timestamp.txt"

& "$root\build\Debug\adaptive_capacity_sweep_benchmark.exe" |
    Tee-Object -FilePath $rawOutputFile

if ($LASTEXITCODE -ne 0) {
    throw "Benchmark execution failed."
}

Write-Host ""
Write-Host "BENCHMARK EXECUTION: PASS"
Write-Host ""

Write-Host "Step 3: Running CTest..."

& "C:\Program Files\CMake\bin\ctest.exe" `
    --test-dir "$root\build" `
    -C Debug `
    --output-on-failure

if ($LASTEXITCODE -ne 0) {
    throw "CTest validation failed."
}

Write-Host ""
Write-Host "CTEST: PASS"
Write-Host ""

$gitCommit = "UNCOMMITTED"

try {
    $gitCommitCandidate = git rev-parse HEAD 2>$null

    if (
        $LASTEXITCODE -eq 0 -and
        -not [string]::IsNullOrWhiteSpace($gitCommitCandidate)
    ) {
        $gitCommit = $gitCommitCandidate.Trim()
    }
}
catch {
    $gitCommit = "UNCOMMITTED"
}

$gitBranch = git branch --show-current 2>$null
if ([string]::IsNullOrWhiteSpace($gitBranch)) {
    $gitBranch = "UNKNOWN"
}

$cmakeVersion = & "C:\Program Files\CMake\bin\cmake.exe" --version 2>$null |
    Select-Object -First 1

$cpu = Get-CimInstance Win32_Processor |
    Select-Object -First 1

$os = Get-CimInstance Win32_OperatingSystem

$recordFile = "$metadataDir\capacity_sweep_run_$timestamp.txt"

$record = @"
NEXUSFLOW STANDARDIZED BENCHMARK RUN
============================================

Experiment
----------
Adaptive Capacity Sweep

Benchmark
---------
adaptive_capacity_sweep_benchmark

Workload
--------
Loads EPS: 10000, 15000, 20000, 25000, 30000, 35000, 40000, 45000, 50000
Events per run: 10000
Repetitions: 3
Queue capacity: 4096
Maximum workers: 16

Build
-----
Configuration: Debug
Generator: Visual Studio 17 2022
Architecture: x64
CMake: $cmakeVersion

Environment
-----------
OS: $($os.Caption)
OS Version: $($os.Version)
CPU: $($cpu.Name)
Logical Processors: $($cpu.NumberOfLogicalProcessors)
Physical Cores: $($cpu.NumberOfCores)

Repository
----------
Git Branch: $gitBranch
Git Commit: $gitCommit

Validation
----------
Benchmark Build: PASS
Benchmark Execution: PASS
CTest: PASS

Timestamp
---------
$timestamp

Raw Output
----------
$rawOutputFile
"@

[System.IO.File]::WriteAllText(
    $recordFile,
    $record
)

Write-Host "============================================"
Write-Host "STANDARDIZED RUN COMPLETE"
Write-Host "============================================"
Write-Host ""
Write-Host "Raw benchmark:"
Write-Host $rawOutputFile
Write-Host ""
Write-Host "Run metadata:"
Write-Host $recordFile
Write-Host ""
Write-Host "RESULT: PASS"
Write-Host "============================================"