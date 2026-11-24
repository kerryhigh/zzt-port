$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$outputDirectory = Join-Path $repoRoot 'build\android-raster-compat'
$mainSource = Join-Path $repoRoot 'app\src\main\java\com\kerry\zzt'
$uiInputHeader = Join-Path $repoRoot 'third_party\libzoo\include\zoo_ui_input.h'
$testSource = Join-Path $PSScriptRoot 'BoardRasterizerTest.java'
$javac = (Get-Command javac -ErrorAction Stop).Source
$java = (Get-Command java -ErrorAction Stop).Source
$header = Get-Content -LiteralPath $uiInputHeader -Raw
$keyBaseMatch = [regex]::Match($header, '(?m)^\s*#define\s+ZOO_KEY_CP437_BASE\s+0x([0-9a-fA-F]+)\b')
if (-not $keyBaseMatch.Success) { throw 'Could not read ZOO_KEY_CP437_BASE from zoo_ui_input.h' }
$engineCp437Base = [Convert]::ToInt32($keyBaseMatch.Groups[1].Value, 16)

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
& $javac -d $outputDirectory `
    (Join-Path $mainSource 'VideoAttributes.java') `
    (Join-Path $mainSource 'BoardRasterizer.java') `
    (Join-Path $mainSource 'DosKeyboardMapping.java') `
    $testSource
if ($LASTEXITCODE -ne 0) { throw "javac failed with exit code $LASTEXITCODE" }

& $java -cp $outputDirectory com.kerry.zzt.BoardRasterizerTest $engineCp437Base
if ($LASTEXITCODE -ne 0) { throw "BoardRasterizerTest failed with exit code $LASTEXITCODE" }
