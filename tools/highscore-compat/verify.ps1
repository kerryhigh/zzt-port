param(
    [string]$PascalHiPath,
    [string]$PascalLoadDirectory,
    [string]$PascalInsertionDirectory
)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$javac = Get-Command javac -ErrorAction Stop
$java = Get-Command java -ErrorAction Stop
$buildDir = Join-Path $repoRoot 'build\highscore-compatibility'
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
$adapter = Join-Path $repoRoot 'app\src\main\java\com\kerry\zzt\HighScores.java'
$harness = Join-Path $PSScriptRoot 'HighScoresCompat.java'
$pascalHarness = Join-Path $PSScriptRoot 'PascalHighScoresCompare.java'
$pascalLoadHarness = Join-Path $PSScriptRoot 'PascalHighScoresLoadCompare.java'
$pascalInsertionHarness = Join-Path $PSScriptRoot 'PascalHighScoresInsertionCompare.java'

& $javac.Source '-encoding' 'UTF-8' '-d' $buildDir $adapter $harness $pascalHarness $pascalLoadHarness $pascalInsertionHarness
if ($LASTEXITCODE -ne 0) { throw "High-score compatibility harness compilation failed with exit code $LASTEXITCODE." }
& $java.Source '-cp' $buildDir 'com.kerry.zzt.HighScoresCompat'
if ($LASTEXITCODE -ne 0) { throw "High-score compatibility checks failed with exit code $LASTEXITCODE." }
if ($PascalHiPath) {
    $pascalHi = (Resolve-Path -LiteralPath $PascalHiPath).Path
    $roundTrip = Join-Path $buildDir 'pascal-highscores-roundtrip.HI'
    & $java.Source '-cp' $buildDir 'com.kerry.zzt.PascalHighScoresCompare' $pascalHi $roundTrip
    if ($LASTEXITCODE -ne 0) { throw "Pascal high-score comparison failed with exit code $LASTEXITCODE." }
}
if ($PascalLoadDirectory) {
    $loadDirectory = (Resolve-Path -LiteralPath $PascalLoadDirectory).Path
    & $java.Source '-cp' $buildDir 'com.kerry.zzt.PascalHighScoresLoadCompare' $loadDirectory
    if ($LASTEXITCODE -ne 0) { throw "Compiled Pascal high-score load comparison failed with exit code $LASTEXITCODE." }
}
if ($PascalInsertionDirectory) {
    $insertionDirectory = (Resolve-Path -LiteralPath $PascalInsertionDirectory).Path
    $scenarios = @(
        @{ Name = 'HITOP'; Score = 32767; SavesFile = $true },
        @{ Name = 'HITIE'; Score = 23000; SavesFile = $true },
        @{ Name = 'HIBOT'; Score = 15500; SavesFile = $true },
        @{ Name = 'HICUT'; Score = 15499; SavesFile = $false },
        @{ Name = 'HIZERO'; Score = 0; SavesFile = $false },
        @{ Name = 'HINEG'; Score = -32768; SavesFile = $false },
        @{ Name = 'HINAME'; Score = 25000; SavesFile = $true; EnteredName = 'KERRY' }
    )
    foreach ($scenario in $scenarios) {
        $inputPath = Join-Path $insertionDirectory ($scenario.Name + '.IN')
        $pascalOutput = Join-Path $insertionDirectory ($scenario.Name + '.OUT')
        $expected = Join-Path $buildDir ('pascal-add-' + $scenario.Name + '.HI')
        $savedPath = Join-Path $insertionDirectory ($scenario.Name + '.HI')
        foreach ($path in @($inputPath, $pascalOutput)) {
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                throw "Compiled Pascal HighScoresAdd case is missing: $path"
            }
        }
        $enteredName = if ($scenario.EnteredName) { $scenario.EnteredName } else { '-' }
        & $java.Source '-cp' $buildDir 'com.kerry.zzt.PascalHighScoresInsertionCompare' `
            $scenario.Name $inputPath $pascalOutput $scenario.Score $enteredName $expected
        if ($LASTEXITCODE -ne 0) {
            throw "Compiled Pascal HighScoresAdd comparison failed for $($scenario.Name) with exit code $LASTEXITCODE."
        }
        if ($scenario.SavesFile) {
            if (-not (Test-Path -LiteralPath $savedPath -PathType Leaf)) {
                throw "HighScoresAdd should have saved $savedPath for an accepted score."
            }
            $savedHash = (Get-FileHash -LiteralPath $savedPath -Algorithm SHA256).Hash
            $outputHash = (Get-FileHash -LiteralPath $pascalOutput -Algorithm SHA256).Hash
            if ($savedHash -ne $outputHash) {
                throw "HighScoresAdd saved table did not match its in-memory result for $($scenario.Name)."
            }
        } elseif (Test-Path -LiteralPath $savedPath) {
            throw "HighScoresAdd unexpectedly saved $savedPath for a rejected score."
        }
    }
}
