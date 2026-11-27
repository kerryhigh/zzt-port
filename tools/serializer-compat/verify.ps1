$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$gradleText = [IO.File]::ReadAllText((Join-Path $repoRoot 'app\build.gradle'))
$ndkMatch = [regex]::Match($gradleText, 'ndkVersion\s+[''\"]([^''\"]+)[''\"]')
if (-not $ndkMatch.Success) { throw 'Could not read ndkVersion from app/build.gradle.' }
$ndkVersion = $ndkMatch.Groups[1].Value
$ndkRoot = $env:ANDROID_NDK_HOME
if (-not $ndkRoot) {
    $sdkRoot = $env:ANDROID_SDK_ROOT
    if (-not $sdkRoot) { $sdkRoot = $env:ANDROID_HOME }
    if (-not $sdkRoot) {
        $properties = Join-Path $repoRoot 'local.properties'
        if (Test-Path -LiteralPath $properties) {
            $sdkLine = Get-Content -LiteralPath $properties | Where-Object { $_ -match '^sdk\.dir=' } | Select-Object -First 1
            if ($sdkLine) { $sdkRoot = ($sdkLine -replace '^sdk\.dir=', '').Replace('\:', ':').Replace('\\', '\') }
        }
    }
    if (-not $sdkRoot) { throw 'Set ANDROID_SDK_ROOT or ANDROID_HOME to locate the Android NDK.' }
    $ndkRoot = Join-Path (Join-Path $sdkRoot 'ndk') $ndkVersion
}
$clang = Join-Path $ndkRoot 'toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe'
if (-not (Test-Path -LiteralPath $clang)) { throw "NDK clang not found: $clang" }
$node = Get-Command node -ErrorAction Stop
$buildDir = Join-Path $repoRoot 'build\serializer-compatibility'
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
$oopTokenDir = Join-Path $buildDir 'libzoo'
New-Item -ItemType Directory -Force -Path $oopTokenDir | Out-Null
$oopTokenC = Join-Path $oopTokenDir 'zoo_oop_token.c'
$python = Get-Command python -ErrorAction Stop
& $python.Source (Join-Path $repoRoot 'third_party\libzoo\tools\tok2c.py') `
    (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_oop_token.tok') $oopTokenC
if ($LASTEXITCODE -ne 0) { throw "OOP token generation failed with exit code $LASTEXITCODE." }
$wasm = Join-Path $buildDir 'world_roundtrip.wasm'
$source = Join-Path $PSScriptRoot 'world_roundtrip.c'
$clangArgs = @(
    '--target=wasm32-unknown-unknown', '-std=c99', '-O1', '-nostdlib', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections',
    '-I', (Join-Path $PSScriptRoot 'include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\include'),
    '-I', (Join-Path $repoRoot 'third_party\libzoo\src'),
    '-I', $buildDir,
    '-Wl,--no-entry', '-Wl,--export=input_ptr', '-Wl,--export=output_ptr', '-Wl,--export=output_len',
    '-Wl,--export=run_roundtrip', '-Wl,--export=run_signed_serialization_codec', '-Wl,--export=run_pascal_boardclose_worldsave', '-Wl,--export=run_game_save_roundtrip', '-Wl,--export=run_world_read', '-Wl,--export=run_world_read_preserves_tail', '-Wl,--export=run_world_read_title_only', '-Wl,--export=run_world_load_missing_trailing', '-Wl,--export=run_world_load_partial_length', '-Wl,--export=run_world_load_unsigned_board_length', '-Wl,--export=run_flag_save_roundtrip', '-Wl,--export=run_repeated_pstring_assignment_roundtrip', '-Wl,--export=run_pstring_matches_pascal_checks', '-Wl,--export=run_world_reset_string_roundtrip', '-Wl,--export=run_board_roundtrip',
    '-Wl,--export=run_zero_count_rle', '-Wl,--export=run_final_rle_overrun', '-Wl,--export=run_board_rle_count',
    '-Wl,--export=run_board_rle_writer_boundary',
    '-Wl,--export=run_editor_brd_roundtrip', '-Wl,--export=run_editor_brd_record_roundtrip',
    '-Wl,--export=run_editor_brd_zero_count_rle', '-Wl,--export=run_editor_brd_reject_truncated', '-Wl,--export=run_editor_brd_transfer',
    '-Wl,--export=run_shared_stat_data_alias',
    '-Wl,--export-memory', '-Wl,--gc-sections', '-Wl,--initial-memory=16777216',
    '-Wl,--max-memory=16777216', '-Wl,-z,stack-size=1048576', $source, '-o', $wasm
)
& $clang @clangArgs
if ($LASTEXITCODE -ne 0) { throw "WASM serializer harness compilation failed with exit code $LASTEXITCODE." }
& $node.Source (Join-Path $PSScriptRoot 'run.js') $wasm
if ($LASTEXITCODE -ne 0) { throw "Serializer compatibility checks failed with exit code $LASTEXITCODE." }
