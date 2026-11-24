param(
	[string]$BuildDirectory = 'build\windows-stage1'
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([System.IO.Path]::IsPathRooted($BuildDirectory)) {
	$buildDir = $BuildDirectory
} else {
	$buildDir = Join-Path $repoRoot $BuildDirectory
}

$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($null -ne $cmakeCommand) {
	$cmake = $cmakeCommand.Source
} else {
	$vsCmake = Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
	if (-not (Test-Path -LiteralPath $vsCmake)) {
		throw 'CMake was not found. Install CMake or Visual Studio 2022 C++ tools.'
	}
	$cmake = $vsCmake
}

& $cmake -S (Join-Path $repoRoot 'windows') -B $buildDir -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed with exit code $LASTEXITCODE" }

foreach ($target in @('zzt_windows', 'zzt_windows_smoke', 'zzt_renderer_test', 'zzt_sound_pcm_test')) {
	& $cmake --build $buildDir --config Release --target $target --parallel 4
	if ($LASTEXITCODE -ne 0) { throw "Build target $target failed with exit code $LASTEXITCODE" }
}

$ctestCommand = Get-Command ctest -ErrorAction SilentlyContinue
if ($null -ne $ctestCommand) {
	$ctest = $ctestCommand.Source
} else {
	$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
	if (-not (Test-Path -LiteralPath $ctest)) { throw 'CTest was not found next to CMake or on PATH.' }
}
& $ctest --test-dir $buildDir -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "CTest failed with exit code $LASTEXITCODE" }

Write-Output "Windows build and smoke test passed. Executable: $(Join-Path $buildDir 'Release\ZZT.exe')"
