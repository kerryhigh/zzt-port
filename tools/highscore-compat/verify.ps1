param(
    [string]$PascalHiPath,
    [string]$PascalLoadDirectory,
    [string]$PascalInsertionDirectory
)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$gradleText = [IO.File]::ReadAllText((Join-Path $repoRoot 'app\build.gradle'))
$ndkMatch = [regex]::Match($gradleText, 'ndkVersion\s+[''\"]([^''\"]+)[''\"]')
if (-not $ndkMatch.Success) { throw 'Could not read ndkVersion from app/build.gradle.' }
$ndkRoot = $env:ANDROID_NDK_HOME
if (-not $ndkRoot) {
    $sdkRoot = $env:ANDROID_SDK_ROOT
    if (-not $sdkRoot) { $sdkRoot = $env:ANDROID_HOME }
    if (-not $sdkRoot) { throw 'Set ANDROID_SDK_ROOT or ANDROID_HOME to locate the Android NDK.' }
    $ndkRoot = Join-Path (Join-Path $sdkRoot 'ndk') $ndkMatch.Groups[1].Value
}
$clang = Join-Path $ndkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe'
if (-not (Test-Path -LiteralPath $clang)) { throw "NDK clang not found: $clang" }
$node = Get-Command node -ErrorAction Stop
$javac = Get-Command javac -ErrorAction Stop
$java = Get-Command java -ErrorAction Stop
$buildDir = Join-Path $repoRoot 'build\highscore-compatibility'
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
$wasm = Join-Path $buildDir 'high_scores.wasm'
$clangArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $repoRoot 'tools\serializer-compat\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-Wl,--no-entry', '-Wl,--export=hs_table_ptr', '-Wl,--export=hs_input_ptr',
    '-Wl,--export=hs_name_ptr', '-Wl,--export=hs_output_ptr', '-Wl,--export=hs_init',
    '-Wl,--export=hs_load', '-Wl,--export=hs_save', '-Wl,--export=hs_insertion_position',
    '-Wl,--export=hs_insert', '-Wl,--export=hs_score', '-Wl,--export-memory', '-Wl,--gc-sections',
    '-Wl,--initial-memory=16777216', '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576',
    (Join-Path $PSScriptRoot 'high_scores_wasm.c'),
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_high_scores.c'),
    '-o', $wasm
)
& $clang @clangArgs
if ($LASTEXITCODE -ne 0) { throw "Shared LibZoo high-score harness compilation failed with exit code $LASTEXITCODE." }

$hiPath = if ($PascalHiPath) { (Resolve-Path -LiteralPath $PascalHiPath).Path } else { '-' }
$loadPath = if ($PascalLoadDirectory) { (Resolve-Path -LiteralPath $PascalLoadDirectory).Path } else { '-' }
$insertPath = if ($PascalInsertionDirectory) { (Resolve-Path -LiteralPath $PascalInsertionDirectory).Path } else { '-' }
$roundTrip = Join-Path $buildDir 'pascal-highscores-roundtrip.HI'
& $node.Source (Join-Path $PSScriptRoot 'verify.js') $wasm `
    $hiPath $roundTrip $loadPath $buildDir $insertPath $buildDir
if ($LASTEXITCODE -ne 0) { throw "Shared LibZoo high-score compatibility checks failed with exit code $LASTEXITCODE." }

$javaSources = @(
    (Join-Path $PSScriptRoot 'ZztNative.java'),
    (Join-Path $repoRoot 'app\src\main\java\com\kerry\zzt\HighScores.java'),
    (Join-Path $PSScriptRoot 'HighScoresEncodingCompat.java')
)
& $javac.Source '-encoding' 'UTF-8' '-d' $buildDir @javaSources
if ($LASTEXITCODE -ne 0) { throw "High-score CP437 adapter harness compilation failed with exit code $LASTEXITCODE." }
& $java.Source '-cp' $buildDir 'com.kerry.zzt.HighScoresEncodingCompat'
if ($LASTEXITCODE -ne 0) { throw "High-score CP437 adapter checks failed with exit code $LASTEXITCODE." }
