$ErrorActionPreference = "Stop"

$root = "C:\Users\PC\NexusFlow"
$capacityDir = "$root\benchmarks\results\capacity_sweep"
$analysisDir = "$root\benchmarks\results\analysis"

New-Item -ItemType Directory -Force -Path $analysisDir | Out-Null

Write-Host "============================================"
Write-Host "NEXUSFLOW: AUTOMATED RESULT ANALYSIS"
Write-Host "============================================"
Write-Host ""

$latest = Get-ChildItem `
    $capacityDir `
    -Filter "capacity_sweep_*.txt" `
    -File |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if ($null -eq $latest) {
    Write-Host "ERROR: No capacity sweep result found."
    Write-Host "============================================"
}
else {
    Write-Host "Latest benchmark:"
    Write-Host $latest.FullName
    Write-Host ""

    $lines = [System.IO.File]::ReadAllLines($latest.FullName)

    $records = @()

    foreach ($line in $lines) {
        if ($line -match '^\s*(\d+)\s+([\d.]+)\s+([\d.]+)\s+(\d+)\s+(\d+)\s+(PASS|FAIL)\s*$') {

            $load = [double]$Matches[1]
            $throughput = [double]$Matches[2]
            $p99 = [double]$Matches[3]
            $workers = [int]$Matches[4]
            $throttled = [int]$Matches[5]
            $integrity = $Matches[6]

            $efficiency = ($throughput / $load) * 100.0

            $records += [PSCustomObject]@{
                LoadEPS = $load
                ThroughputEPS = $throughput
                EfficiencyPct = $efficiency
                P99us = $p99
                Workers = $workers
                Throttled = $throttled
                Integrity = $integrity
            }
        }
    }

    if ($records.Count -eq 0) {
        Write-Host "ERROR: No benchmark records could be parsed."
    }
    else {
        $peak = $records |
            Sort-Object ThroughputEPS -Descending |
            Select-Object -First 1

        $lowestP99 = $records |
            Sort-Object P99us |
            Select-Object -First 1

        $maxP99 = $records |
            Sort-Object P99us -Descending |
            Select-Object -First 1

        $maxWorkers = ($records | Measure-Object Workers -Maximum).Maximum
        $maxThrottled = ($records | Measure-Object Throttled -Maximum).Maximum

        $firstBelow80 = $records |
            Where-Object { $_.EfficiencyPct -lt 80 } |
            Sort-Object LoadEPS |
            Select-Object -First 1

        $integrityPass = $true

        foreach ($record in $records) {
            if ($record.Integrity -ne "PASS") {
                $integrityPass = $false
                break
            }
        }

        Write-Host "Records analyzed: $($records.Count)"
        Write-Host "Integrity: $(if ($integrityPass) { 'PASS' } else { 'FAIL' })"
        Write-Host ""

        Write-Host "============================================"
        Write-Host "KEY RESULTS"
        Write-Host "============================================"

        Write-Host "Peak throughput:"
        Write-Host ("  {0:N2} EPS at {1:N0} EPS offered load" -f `
            $peak.ThroughputEPS, $peak.LoadEPS)

        Write-Host "Lowest P99:"
        Write-Host ("  {0:N2} us at {1:N0} EPS" -f `
            $lowestP99.P99us, $lowestP99.LoadEPS)

        Write-Host "Highest P99:"
        Write-Host ("  {0:N2} us at {1:N0} EPS" -f `
            $maxP99.P99us, $maxP99.LoadEPS)

        Write-Host "Maximum workers observed: $maxWorkers"
        Write-Host "Maximum throttled events: $maxThrottled"

        if ($null -ne $firstBelow80) {
            Write-Host "First load below 80% efficiency:"
            Write-Host ("  {0:N0} EPS -> {1:N2}% efficiency" -f `
                $firstBelow80.LoadEPS, $firstBelow80.EfficiencyPct)
        }
        else {
            Write-Host "First load below 80% efficiency: NOT OBSERVED"
        }

        Write-Host ""
        Write-Host "============================================"
        Write-Host "CAPACITY TABLE"
        Write-Host "============================================"

        $records |
            Format-Table `
                LoadEPS,
                ThroughputEPS,
                EfficiencyPct,
                P99us,
                Workers,
                Throttled,
                Integrity `
                -AutoSize

        $csvPath = "$analysisDir\capacity_sweep_analysis.csv"
        $summaryPath = "$analysisDir\capacity_sweep_analysis.txt"

        $records | Export-Csv -Path $csvPath -NoTypeInformation

        $summary = @()
        $summary += "NEXUSFLOW AUTOMATED CAPACITY ANALYSIS"
        $summary += "============================================"
        $summary += "Source: $($latest.FullName)"
        $summary += "Records analyzed: $($records.Count)"
        $summary += "Integrity: $(if ($integrityPass) { 'PASS' } else { 'FAIL' })"
        $summary += ""
        $summary += "Peak throughput: $([math]::Round($peak.ThroughputEPS,2)) EPS"
        $summary += "Peak load: $([math]::Round($peak.LoadEPS,0)) EPS"
        $summary += "Lowest P99: $([math]::Round($lowestP99.P99us,2)) us"
        $summary += "Highest P99: $([math]::Round($maxP99.P99us,2)) us"
        $summary += "Maximum workers: $maxWorkers"
        $summary += "Maximum throttled events: $maxThrottled"

        if ($null -ne $firstBelow80) {
            $summary += "First below 80% efficiency: $([math]::Round($firstBelow80.LoadEPS,0)) EPS"
        }
        else {
            $summary += "First below 80% efficiency: NOT OBSERVED"
        }

        $summary += ""
        $summary += "LoadEPS,ThroughputEPS,EfficiencyPct,P99us,Workers,Throttled,Integrity"

        foreach ($record in $records) {
            $summary += (
                "{0},{1:F2},{2:F2},{3:F2},{4},{5},{6}" -f `
                $record.LoadEPS,
                $record.ThroughputEPS,
                $record.EfficiencyPct,
                $record.P99us,
                $record.Workers,
                $record.Throttled,
                $record.Integrity
            )
        }

        [System.IO.File]::WriteAllLines(
            $summaryPath,
            $summary
        )

        Write-Host ""
        Write-Host "============================================"
        Write-Host "ANALYSIS OUTPUT"
        Write-Host "============================================"
        Write-Host "CSV:"
        Write-Host $csvPath
        Write-Host ""
        Write-Host "Summary:"
        Write-Host $summaryPath
        Write-Host ""
        Write-Host "AUTOMATED ANALYSIS: PASS"
        Write-Host "============================================"
    }
}
