$ErrorActionPreference = "Stop"

Write-Host "`n=== NexusFlow Release Audit ===" -ForegroundColor Cyan
Write-Host "Repository: $((Get-Location).Path)"

# Confirm repository
git rev-parse --show-toplevel
if ($LASTEXITCODE -ne 0) {
    throw "Current directory is not a Git repository."
}

Write-Host "`n--- 1. Git status ---" -ForegroundColor Yellow
git status --short --branch

Write-Host "`n--- 2. Diff whitespace check ---" -ForegroundColor Yellow
git diff --check
if ($LASTEXITCODE -ne 0) {
    Write-Warning "Diff check reported a problem."
} else {
    Write-Host "No diff whitespace errors reported." -ForegroundColor Green
}

Write-Host "`n--- 3. Change summary ---" -ForegroundColor Yellow
git diff --stat

Write-Host "`n--- 4. Deleted backup files ---" -ForegroundColor Yellow
git status --short | Select-String '^\s*D\s+.*(backup|before_final_password_fix|\.final_backup)'

Write-Host "`n--- 5. Kubernetes secret placeholders ---" -ForegroundColor Yellow
$secretFile = "deploy/kubernetes/secret.yaml"
if (Test-Path $secretFile) {
    Select-String -Path $secretFile -Pattern 'CHANGE_ME|TODO|REPLACE_ME|example-password' |
        ForEach-Object { "{0}:{1}: placeholder detected" -f $_.Path, $_.LineNumber }
    Write-Host "Review secret handling before deployment; do not put real secrets in Git."
} else {
    Write-Host "$secretFile not found."
}

Write-Host "`n--- 6. Credential-like literals (redacted) ---" -ForegroundColor Yellow
$scanFiles = @(
    "storage/postgres/postgres_store.hpp",
    "storage/postgres/postgres_store.hpp.final_backup",
    "storage/postgres/postgres_store.hpp.before_final_password_fix"
)
foreach ($file in $scanFiles) {
    if (Test-Path $file) {
        Select-String -Path $file -Pattern '(password|passwd|secret|token)\s*=\s*["''][^"'']+["'']' |
            ForEach-Object {
                "{0}:{1}: possible hard-coded credential; value hidden" -f $_.Path, $_.LineNumber
            }
    }
}

Write-Host "`n--- 7. Relevant tracked-file history ---" -ForegroundColor Yellow
git log -5 --oneline -- storage/postgres/postgres_store.hpp.final_backup deploy/kubernetes/secret.yaml

Write-Host "`n--- 8. Build configuration files ---" -ForegroundColor Yellow
$checkFiles = @(
    "CMakeLists.txt",
    "api/grpc/CMakeLists.txt",
    "observability/opentelemetry/CMakeLists.txt",
    "streaming/CMakeLists.txt"
)
foreach ($file in $checkFiles) {
    if (Test-Path $file) {
        Write-Host "OK      $file" -ForegroundColor Green
    } else {
        Write-Host "MISSING $file" -ForegroundColor Red
    }
}

Write-Host "`n--- 9. Manual review still required ---" -ForegroundColor Yellow
Write-Host "- Confirm the deleted backup files are intentionally obsolete."
Write-Host "- Review CMake changes and queue/worker concurrency changes."
Write-Host "- Confirm any credential-like literal is not a real or reused secret."
Write-Host "- Run build and tests separately; this audit does not certify correctness."
Write-Host "`nAudit finished. No files were staged, committed, reset, or pushed." -ForegroundColor Cyan
