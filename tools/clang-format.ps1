$ErrorActionPreference = "Stop"

$Ver = "18.1.8"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")

$Bin = Join-Path $Root "tools\clang-format-bin\$Ver\windows-x86_64\clang-format.exe"
if (-not (Test-Path $Bin)) {
  throw "Missing clang-format binary: $Bin"
}

& $Bin @args
exit $LASTEXITCODE
