$ErrorActionPreference = "Stop"

# Run from repo root (script is in repo root)
$CF = Join-Path $PSScriptRoot "tools\clang-format.ps1"

if (-not (Test-Path $CF)) {
  throw "ERROR: $CF not found. Did you commit tools\clang-format.ps1 and the pinned binaries?"
}

# Print version for traceability
& $CF --version | Write-Host

# Collect files (PowerShell-native; avoids xargs/find differences on Windows)
$roots = @(
  "firmware/common",
  "firmware/baseband",
  "firmware/application",
  "firmware/test/application",
  "firmware/test/baseband"
)

$files = foreach ($r in $roots) {
  if (Test-Path $r) {
    Get-ChildItem -Path $r -Recurse -File -ErrorAction SilentlyContinue |
      Where-Object { $_.Extension -in @(".h", ".hpp", ".c", ".cpp") } |
      ForEach-Object { $_.FullName }
  }
}

if (-not $files -or $files.Count -eq 0) {
  Write-Host "No matching source files found."
  exit 0
}

# Format in place
& $CF -style=file -i -- $files
exit $LASTEXITCODE
