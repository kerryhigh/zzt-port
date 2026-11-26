Warning: truncated output (original token count: 42304)
Total output lines: 2813

param(
    [string]$PascalToolchain = 'build\pascal-compat-run\Z',
    [string]$DosboxExecutable
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

if (-not [IO.Path]::IsPathRooted($PascalToolchain)) {
    $PascalToolchain = Join-Path $repoRoot $PascalToolchain
}
$PascalToolchain = (Resolve-Path -LiteralPath $PascalToolchain).Path
foreach ($requiredFile in @('TPC.EXE', 'TURBO.TPL')) {
    if (-not (Test-Path -LiteralPath (Join-Path $PascalToolchain $requiredFile) -PathType Leaf)) {
        throw "Turbo Pascal toolchain file is missing: $(Join-Path $PascalToolchain $requiredFile)"
    }
}

if (-not $DosboxExecutable) {
    $dosboxCommand = Get-Command dosbox-x.exe -ErrorAction SilentlyContinue
    if ($dosboxCommand) {
        $DosboxExecutable = $dosboxCommand.Source
    }
}
if (-not $DosboxExecutable -or -not (Test-Path -LiteralPath $DosboxExecutable -PathType Leaf)) {
    throw 'Pass -DosboxExecutable with a DOSBox-X executable path.'
}
$DosboxExecutable = (Resolve-Path -LiteralPath $DosboxExecutable).Path

$buildRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'build'))
$stageRoot = [IO.Path]::GetFullPath((Join-Path $buildRoot 'pascal-compat-verification'))
$buildPrefix = $buildRoot.TrimEnd('\') + '\'
if (-not $stageRoot.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to use a Pascal staging directory outside the workspace build folder: $stageRoot"
}
if (Test-Path -LiteralPath $stageRoot) {
    $resolvedStage = (Resolve-Path -LiteralPath $stageRoot).Path
    if (-not $resolvedStage.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to replace a Pascal staging path outside the workspace build folder: $resolvedStage"
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}

$sourceRoot = Join-Path $repoRoot 'third_party\reconstruction-of-zzt'
$stageBin = Join-Path $stageRoot 'BIN'
$stageSource = Join-Path $stageRoot 'SRC'
New-Item -ItemType Directory -Force -Path $stageBin, $stageSource | Out-Null
Copy-Item -LiteralPath (Join-Path $PascalToolchain 'TPC.EXE') -Destination $stageRoot
Copy-Item -LiteralPath (Join-Path $PascalToolchain 'TURBO.TPL') -Destination $stageRoot
Copy-Item -Path (Join-Path $sourceRoot 'SRC\*') -Destination $stageSource -Recurse -Force
Copy-Item -LiteralPath (Join-Path $sourceRoot 'RES\DEMO.ZZT') -Destination $stageRoot
$demoBytes = [IO.File]::ReadAllBytes((Join-Path $stageRoot 'DEMO.ZZT'))
if ($demoBytes.Length -lt 514) { throw 'DEMO.ZZT is too short to stage a title-only load fixture.' }
$declaredBoardCount = [BitConverter]::ToInt16($demoBytes, 2)
$firstBoardLength = [BitConverter]::ToUInt16($demoBytes, 512)
$titleOnlyLength = 514 + $firstBoardLength
if ($declaredBoardCount -le 0 -or $titleOnlyLength -ge $demoBytes.Length) {
    throw 'DEMO.ZZT does not contain a multi-board prefix suitable for the title-only load probe.'
}
$titleOnlyBytes = New-Object byte[] $titleOnlyLength
[Array]::Copy($demoBytes, $titleOnlyBytes, $titleOnlyLength)
[IO.File]::WriteAllBytes((Join-Path $stageRoot 'TITLE.ZZT'), $titleOnlyBytes)
$fullLoadBytes = New-Object byte[] $titleOnlyLength
[Array]::Copy($titleOnlyBytes, $fullLoadBytes, $titleOnlyLength)
# Keep the current board on the complete first record so the Pascal probe tests
# unchecked reads of missing trailing board records without opening stale data.
$fullLoadBytes[17] = 0
$fullLoadBytes[18] = 0
[IO.File]::WriteAllBytes((Join-Path $stageRoot 'FULLFILE.ZZT'), $fullLoadBytes)
$partialPayloadBytes = New-Object byte[] ($titleOnlyLength + 2 + 4)
[Array]::Copy($fullLoadBytes, $partialPayloadBytes, $titleOnlyLength)
$partialBoardLengthPos = $titleOnlyLength
$partialPayloadBytes[$partialBoardLengthPos] = 16
$partialPayloadBytes[$partialBoardLengthPos + 1] = 0
$partialPayloadBytes[$partialBoardLengthPos + 2] = 0x11
$partialPayloadBytes[$partialBoardLengthPos + 3] = 0x22
$partialPayloadBytes[$partialBoardLengthPos + 4] = 0x33
$partialPayloadBytes[$partialBoardLengthPos + 5] = 0x44
[IO.File]::WriteAllBytes((Join-Path $stageRoot 'PARTIAL.ZZT'), $partialPayloadBytes)
$partialLengthBytes = New-Object byte[] ($titleOnlyLength + 1)
[Array]::Copy($fullLoadBytes, $partialLengthBytes, $titleOnlyLength)
$partialLengthBytes[$titleOnlyLength] = 16
[IO.File]::WriteAllBytes((Join-Path $stageRoot 'PARTLEN.ZZT'), $partialLengthBytes)
$unsignedLengthBytes = New-Object byte[] (514 + 65535)
[Array]::Copy($demoBytes, $unsignedLengthBytes, 514 + $firstBoardLength)
$unsignedLengthBytes[2] = 0
$unsignedLengthBytes[3] = 0
$unsignedLengthBytes[17] = 0
$unsignedLengthBytes[18] = 0
$unsignedLengthBytes[512] = 0xFF
$unsignedLengthBytes[513] = 0xFF
[IO.File]::WriteAllBytes((Join-Path $stageRoot 'NEGLEN.ZZT'), $unsignedLengthBytes)
$unsignedBrdBytes = New-Object byte[] (2 + 65535)
$unsignedBrdBytes[0] = 0xFF
$unsignedBrdBytes[1] = 0xFF
[Array]::Copy($demoBytes, 514, $unsignedBrdBytes, 2, $firstBoardLength)
[IO.File]::WriteAllBytes((Join-Path $stageRoot 'NEGBRD.BRD'), $unsignedBrdBytes)

# Expose the otherwise-private short-string input buffer only in this isolated
# verification copy so a Pascal probe can seed it without changing runtime logic.
$stagedInputPath = Join-Path $stageSource 'INPUT.PAS'
$stagedInput = [IO.File]::ReadAllText($stagedInputPath)
$interfaceAnchor = "`t`tInputLastDeltaX, InputLastDeltaY: integer;"
$implementationAnchor = 'function InputInitMouse: boolean;'
$inputBufferAnchor = "`tInputKeyBuffer: string;"
if (-not $stagedInput.Contains($interfaceAnchor) -or -not $stagedInput.Contains($implementationAnchor) -or
    -not $stagedInput.Contains($inputBufferAnchor)) {
    throw 'Could not locate the Input unit probe insertion points.'
}
$stagedInput = $stagedInput.Replace(
    $interfaceAnchor,
    $interfaceAnchor + "`r`n`t`tprocedure InputProbeSetKeyBuffer(buffer: string);`r`n`t`tprocedure InputProbeSetReadKeys(keys: string);`r`n`t`tfunction InputProbeGetKeyBuffer: string;"
)
$probeHelper = @'
procedure InputProbeSetKeyBuffer(buffer: string);
begin
	InputKeyBuffer := buffer;
end;

function InputProbeGetKeyBuffer: string;
begin
	InputProbeGetKeyBuffer := InputKeyBuffer;
end;

procedure InputProbeSetReadKeys(keys: string);
begin
	InputProbeReadKeys := keys;
end;

'@
$stagedInput = $stagedInput.Replace(
    $inputBufferAnchor,
    $inputBufferAnchor + "`r`n`tInputProbeReadKeys: string[50];"
)
$stagedInput = $stagedInput.Replace($implementationAnchor, $probeHelper + $implementationAnchor)
$waitKeyPattern = '(?s)procedure InputReadWaitKey;\r?\n\s*begin\r?\n\s*repeat\r?\n\s*InputUpdate\r?\n\s*until InputKeyPressed <> #0;\r?\n\s*end;'
if (-not [regex]::IsMatch($stagedInput, $waitKeyPattern)) {
    throw 'Could not locate the InputReadWaitKey implementation for staged prompt-key injection.'
}
$waitKeyReplacement = @'
procedure InputReadWaitKey;
	begin
		if Length(InputProbeReadKeys) <> 0 then begin
			InputKeyBuffer := InputProbeReadKeys[1];
			if Length(InputProbeReadKeys) = 1 then
				InputProbeReadKeys := ''
			else
				InputProbeReadKeys := Copy(InputProbeReadKeys, 2, Length(InputProbeReadKeys) - 1);
		end;
		repeat
			InputUpdate
		until InputKeyPressed <> #0;
	end;
'@
$stagedInput = ([regex]::new($waitKeyPattern)).Replace($stagedInput, $waitKeyReplacement, 1)
$stagedInputWithoutNormalizedTape = $stagedInput
[IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))

$stagedOopPath = Join-Path $stageSource 'OOP.PAS'
$stagedOop = [IO.File]::ReadAllText($stagedOopPath)
$oopInterfaceAnchor = 'procedure OopExecute(statId: integer; var position: integer; name: TString50);'
$oopImplementationAnchor = 'procedure OopSkipLine(statId: integer; var position: integer);'
$oopInterfaceIndex = $stagedOop.IndexOf($oopInterfaceAnchor, [StringComparison]::Ordinal)
if ($oopInterfaceIndex -lt 0 -or -not $stagedOop.Contains($oopImplementationAnchor)) {
    throw 'Could not locate the OOP parser probe insertion points.'
}
$oopProbeDeclarations = "`r`n`tprocedure OopProbeReadWord(statId: integer; var position: integer);`r`n`tprocedure OopProbeReadValue(statId: integer; var position: integer);`r`n`tfunction OopProbeReadLineToEnd(statId: integer; var position: integer): string;"
$stagedOop = $stagedOop.Insert($oopInterfaceIndex + $oopInterfaceAnchor.Length, $oopProbeDeclarations)
$oopProbeWrappers = @'
procedure OopProbeReadWord(statId: integer; var position: integer);
begin
	OopReadWord(statId, position);
end;

procedure OopProbeReadValue(statId: integer; var position: integer);
begin
	OopReadValue(statId, position);
end;

function OopProbeReadLineToEnd(statId: integer; var position: integer): string;
begin
	OopProbeReadLineToEnd := OopReadLineToEnd(statId, position);
end;

'@
$oopSendImplementationAnchor = 'function OopSend(statId: integer; sendLabel: string; ignoreLock: boolean): boolean;'
$oopSendImplementationIndex = $stagedOop.LastIndexOf($oopSendImplementationAnchor, [StringComparison]::Ordinal)
if ($oopSendImplementationIndex -lt 0) {
    throw 'Could not locate the OOP line-reader probe implementation insertion point.'
}
$stagedOop = $stagedOop.Insert($oopSendImplementationIndex, $oopProbeWrappers)
[IO.File]::WriteAllText($stagedOopPath, $stagedOop, [Text.UTF8Encoding]::new($false))

function Set-ScrollPascalUnitHooks {
    # The Scroll text-window probe captures VideoWriteText output but does not
    # need host VGA memory save/restore, so stub only VideoMove in the staged unit.
    $stagedVideoPath = Join-Path $stageSource 'VIDEO.PAS'
    $stagedVideo = [IO.File]::ReadAllText($stagedVideoPath)
    $videoMoveAnchor = 'procedure VideoMove(x, y, chars: integer; data: pointer; toVideo: boolean);'
    $videoMoveStart = $stagedVideo.LastIndexOf($videoMoveAnchor, [StringComparison]::Ordinal)
    if ($videoMoveStart -lt 0) { throw 'Could not locate the Video unit VideoMove implementation for the Scroll probe.' }
    $videoMoveEnd = $stagedVideo.IndexOf('end;', $videoMoveStart, [StringComparison]::Ordinal)
    if ($videoMoveEnd -lt 0) { throw 'Could not locate the end of the Video unit VideoMove implementation.' }
    $videoMoveBody = $stagedVideo.Substring($videoMoveStart, $videoMoveEnd + 4 - $videoMoveStart)
    if (-not $videoMoveBody.Contains('VideoWriteTextCO80(0, 0, 0, ')) {
        throw 'The VideoMove implementation did not match the Scroll probe staging assumption.'
    }
    $videoMoveStub = $videoMoveAnchor + "`r`nbegin`r`nend;"
    $stagedVideo = $stagedVideo.Substring(0, $videoMoveStart) + $videoMoveStub + $stagedVideo.Substring($videoMoveEnd + 4)
    [IO.File]::WriteAllText($stagedVideoPath, $stagedVideo, [Text.UTF8Encoding]::new($false))

    # Close the modal window after the original draw routines render its title
    # and body; this avoids a real DOS keyboard wait without changing the OOP path.
    $stagedTxtWindPath = Join-Path $stageSource 'TXTWIND.PAS'
    $stagedTxtWind = [IO.File]::ReadAllText($stagedTxtWindPath)
    $newline = if ($stagedTxtWind.Contains("`r`n")) { "`r`n" } else { "`n" }
    $textWindowFlagAnchor = "`t`tTextWindowRejected: boolean;"
    $textWindowSelectAnchor = 'procedure TextWindowSelect(var state: TTextWindowState; hyperlinkAsSelect, viewingFile: boolean);'
    if (-not $stagedTxtWind.Contains($textWindowFlagAnchor) -or
        -not $stagedTxtWind.Contains($textWindowSelectAnchor)) {
        throw 'Could not locate the text-window selection probe insertion points.'
    }
    $stagedTxtWind = $stagedTxtWind.Replace(
        $textWindowFlagAnchor,
        $textWindowFlagAnchor + $newline + "`t`tTextWindowProbeAutoEscape: boolean;"
    )
    $textWindowSelectStart = $stagedTxtWind.LastIndexOf($textWindowSelectAnchor, [StringComparison]::Ordinal)
    $textWindowDrawCall = 'TextWindowDraw(state, false, viewingFile);'
    $textWindowDrawIndex = $stagedTxtWind.IndexOf($textWindowDrawCall, $textWindowSelectStart, [StringComparison]::Ordinal)
    if ($textWindowDrawIndex -lt 0) { throw 'Could not locate the initial TextWindowSelect draw call.' }
    $textWindowDrawEnd = $textWindowDrawIndex + $textWindowDrawCall.Length
    $autoEscapeShim = $newline + "`t`t`tif TextWindowProbeAutoEscape then begin" + $newline +
        "`t`t`t`tInputKeyPressed := KEY_ESCAPE;" + $newline +
        "`t`t`t`tTextWindowRejected := true;" + $newline +
        "`t`t`t`tExit;" + $newline +
        "`t`t`tend;"
    $stagedTxtWind = $stagedTxtWind.Insert($textWindowDrawEnd, $autoEscapeShim)
    [IO.File]::WriteAllText($stagedTxtWindPath, $stagedTxtWind, [Text.UTF8Encoding]::new($false))
}

function Set-TitleRestoreEscapeInputHook {
    $stagedTxtWindPath = Join-Path $stageSource 'TXTWIND.PAS'
    $stagedTxtWind = [IO.File]::ReadAllText($stagedTxtWindPath)
    $newline = if ($stagedTxtWind.Contains("`r`n")) { "`r`n" } else { "`n" }
    $textWindowFlagAnchor = "`t`tTextWindowRejected: boolean;"
    $textWindowSelectAnchor = 'procedure TextWindowSelect(var state: TTextWindowState; hyperlinkAsSelect, viewingFile: boolean);'
    if (-not $stagedTxtWind.Contains($textWindowFlagAnchor) -or
        -not $stagedTxtWind.Contains($textWindowSelectAnchor)) {
        throw 'Could not locate title restore selector input staging points.'
    }
    $stagedTxtWind = $stagedTxtWind.Replace(
        $textWindowFlagAnchor,
        $textWindowFlagAnchor + $newline + "`t`tTextWindowProbeInjectEscape: boolean;"
    )
    $textWindowSelectStart = $stagedTxtWind.LastIndexOf($textWindowSelectAnchor, [StringComparison]::Ordinal)
    $textWindowDrawCall = 'TextWindowDraw(state, false, viewingFile);'
    $textWindowDrawIndex = $stagedTxtWind.IndexOf($textWindowDrawCall, $textWindowSelectStart, [StringComparison]::Ordinal)
    if ($textWindowDrawIndex -lt 0) { throw 'Could not locate title restore selector draw call.' }
    $textWindowDrawEnd = $textWindowDrawIndex + $textWindowDrawCall.Length
    $inputUpdateCall = 'InputUpdate;'
    $inputUpdateIndex = $stagedTxtWind.IndexOf($inputUpdateCall, $textWindowDrawEnd, [StringComparison]::Ordinal)
    if ($inputUpdateIndex -lt 0) { throw 'Could not locate title restore selector input poll.' }
    $inputUpdateEnd = $inputUpdateIndex + $inputUpdateCall.Length
    $selectorEscapeShim = $newline + "`t`t`tif TextWindowProbeInjectEscape then begin" + $newline +
        "`t`t`t`tTextWindowProbeInjectEscape := false;" + $newline +
        "`t`t`t`tInputKeyPressed := KEY_ESCAPE;" + $newline +
        "`t`t`tend;"
    $stagedTxtWind = $stagedTxtWind.Insert($inputUpdateEnd, $selectorEscapeShim)
    [IO.File]::WriteAllText($stagedTxtWindPath, $stagedTxtWind, [Text.UTF8Encoding]::new($false))
}

$probeFiles = @(
    @{ Source = 'VALPROBE.PAS'; DosName = 'VALPROBE.PAS'; Program = 'VALPROBE'; CompileFlags = '/B /EBIN' },
    @{ Source = 'INPUTBUF.PAS'; DosName = 'INPUTBUF.PAS'; Program = 'INPUTBUF'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'ELDEFPRB.PAS'; DosName = 'ELDEFPRB.PAS'; Program = 'ELDEFPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'WORLDSAV.PAS'; DosName = 'WORLDSAV.PAS'; Program = 'WORLDSAV'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'PLRTORCH.PAS'; DosName = 'PLRTORCH.PAS'; Program = 'PLRTORCH'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'PLAYERST.PAS'; DosName = 'PLAYERST.PAS'; Program = 'PLAYERST'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'PLAYERMV.PAS'; DosName = 'PLAYERMV.PAS'; Program = 'PLAYERMV'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'PLAYERKEY.PAS'; DosName = 'PLAYERKY.PAS'; Program = 'PLAYERKY'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'PAUSEMV.PAS'; DosName = 'PAUSEMV.PAS'; Program = 'PAUSEMV'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'ZEROSPD.PAS'; DosName = 'ZEROSPD.PAS'; Program = 'ZEROSPD'; CompileFlags = '/B /EBIN /ISRC /USRC' },
    @{ Source = 'PLAYERDE.PAS'; DosName = 'PLAYERDE.PAS'; Program = 'PLAYERDE'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BEARPROBE.PAS'; DosName = 'BEARPROB.PAS'; Program = 'BEARPROB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'MONPRB.PAS'; DosName = 'MONPRB.PAS'; Program = 'MONPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'SHARKPRB.PAS'; DosName = 'SHARKPRB.PAS'; Program = 'SHARKPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'LIONPRB.PAS'; DosName = 'LIONPRB.PAS'; Program = 'LIONPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'RUFFPRB.PAS'; DosName = 'RUFFPRB.PAS'; Program = 'RUFFPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'STARPRB.PAS'; DosName = 'STARPRB.PAS'; Program = 'STARPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'TIGRPRB.PAS'; DosName = 'TIGRPRB.PAS'; Program = 'TIGRPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'CENTIPR.PAS'; DosName = 'CENTIPR.PAS'; Program = 'CENTIPR'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'DRAWPRB.PAS'; DosName = 'DRAWPRB.PAS'; Program = 'DRAWPRB'; CompileFlags = '/B /EBIN /ISRC /USRC' },
    @{ Source = 'MSGTIK.PAS'; DosName = 'MSGTIK.PAS'; Program = 'MSGTIK'; CompileFlags = '/B /EBIN /ISRC /USRC' },
    @{ Source = 'PUSHDUP.PAS'; DosName = 'PUSHDUP.PAS'; Program = 'PUSHDUP'; CompileFlags = '/B /EBIN /ISRC /USRC' },
    @{ Source = 'DOORPRB.PAS'; DosName = 'DOORPRB.PAS'; Program = 'DOORPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'ITEMPRB.PAS'; DosName = 'ITEMPRB.PAS'; Program = 'ITEMPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'ENERGYPR.PAS'; DosName = 'ENERGYPR.PAS'; Program = 'ENERGYPR'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'TORCHPR.PAS'; DosName = 'TORCHPR.PAS'; Program = 'TORCHPR'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BOMBPRB.PAS'; DosName = 'BOMBPRB.PAS'; Program = 'BOMBPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'TERRAIN.PAS'; DosName = 'TERRAIN.PAS'; Program = 'TERRAIN'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'DAMAGE.PAS'; DosName = 'DAMAGE.PAS'; Program = 'DAMAGE'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'PASSAGE.PAS'; DosName = 'PASSAGE.PAS'; Program = 'PASSAGE'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BOARDENT.PAS'; DosName = 'BOARDENT.PAS'; Program = 'BOARDENT'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BOARDIN.PAS'; DosName = 'BOARDIN.PAS'; Program = 'BOARDIN'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BOARDENC.PAS'; DosName = 'BOARDENC.PAS'; Program = 'BOARDENC'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BOARDDEC.PAS'; DosName = 'BOARDDEC.PAS'; Program = 'BOARDDEC'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BOARDRLE.PAS'; DosName = 'BOARDRLE.PAS'; Program = 'BOARDRLE'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'WORLDTIT.PAS'; DosName = 'WORLDTIT.PAS'; Program = 'WORLDTIT'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'FULLLOAD.PAS'; DosName = 'FULLLOAD.PAS'; Program = 'FULLLOAD'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'SHORTRD.PAS'; DosName = 'SHORTRD.PAS'; Program = 'SHORTRD'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'SHORTLEN.PAS'; DosName = 'SHORTLEN.PAS'; Program = 'SHORTLEN'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'NEGLEN.PAS'; DosName = 'NEGLEN.PAS'; Program = 'NEGLEN'; CompileFlags = '/B /EBIN /ISRC /USRC' },
    @{ Source = 'NEGBRD.PAS'; DosName = 'NEGBRD.PAS'; Program = 'NEGBRD'; CompileFlags = '/B /EBIN /ISRC /USRC' },
    @{ Source = 'BRDIOPRB.PAS'; DosName = 'BRDIOPRB.PAS'; Program = 'BRDIOPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BLINKPRB.PAS'; DosName = 'BLINKPRB.PAS'; Program = 'BLINKPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'SCROLLTK.PAS'; DosName = 'SCROLLTK.PAS'; Program = 'SCROLLTK'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'SCROLLTH.PAS'; DosName = 'SCROLLTH.PAS'; Program = 'SCROLLTH'; CompileFlags = '/B /EBIN /ISRC /USRC' },
    @{ Source = 'SPINGUN.PAS'; DosName = 'SPINGUN.PAS'; Program = 'SPINGUN'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'TRANSPRT.PAS'; DosName = 'TRANSPRT.PAS'; Program = 'TRANSPRT'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BULLETPR.PAS'; DosName = 'BULLETPR.PAS'; Program = 'BULLETPR'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'SHOTPRB.PAS'; DosName = 'SHOTPRB.PAS'; Program = 'SHOTPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'BINDPROBE.PAS'; DosName = 'BINDPROB.PAS'; Program = 'BINDPROB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'CONVPRB.PAS'; DosName = 'CONVPRB.PAS'; Program = 'CONVPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'SLIMEPRB.PAS'; DosName = 'SLIMEPRB.PAS'; Program = 'SLIMEPRB'; CompileFlags = '/B /EBIN /ISRC /USRC' },
    @{ Source = 'HISAVPRB.PAS'; DosName = 'HISAVPRB.PAS'; Program = 'HISAVPRB'; CompileFlags = '/B /EBIN /UBIN' },
    @{ Source = 'HILOADPRB.PAS'; DosName = 'HILODPRB.PAS'; Program = 'HILODPRB'; CompileFlags = '/B /EBIN /UBIN' }
)
foreach ($probe in $probeFiles) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $probe.Source) -Destination (Join-Path $stageRoot $probe.DosName)
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BINDALIA.PAS') -Destination (Join-Path $stageRoot 'BINDALIA.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BINDREUS.PAS') -Destination (Join-Path $stageRoot 'BINDREUS.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BINDORDR.PAS') -Destination (Join-Path $stageRoot 'BINDORDR.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BINDSIZE.PAS') -Destination (Join-Path $stageRoot 'BINDSIZE.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SOUNDFRQ.PAS') -Destination (Join-Path $stageRoot 'SOUNDFRQ.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BINDTAIL.PAS') -Destination (Join-Path $stageRoot 'BINDTAIL.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPIFPRB.PAS') -Destination (Join-Path $stageRoot 'OOPIFPRB.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPANYPRB.PAS') -Destination (Join-Path $stageRoot 'OOPANYP.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BINDSEL.PAS') -Destination (Join-Path $stageRoot 'BINDSEL.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BINDWIN.PAS') -Destination (Join-Path $stageRoot 'BINDWIN.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPGIV.PAS') -Destination (Join-Path $stageRoot 'OOPGIV.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPCHAR.PAS') -Destination (Join-Path $stageRoot 'OOPCHAR.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPPUT.PAS') -Destination (Join-Path $stageRoot 'OOPPUT.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPCHG.PAS') -Destination (Join-Path $stageRoot 'OOPCHG.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPFLAGS.PAS') -Destination (Join-Path $stageRoot 'OOPFLAGS.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPSEND.PAS') -Destination (Join-Path $stageRoot 'OOPSEND.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OBJTIK.PAS') -Destination (Join-Path $stageRoot 'OBJTIK.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OBJTOUCH.PAS') -Destination (Join-Path $stageRoot 'OBJTOUCH.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPFLOW.PAS') -Destination (Join-Path $stageRoot 'OOPFLOW.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'OOPPARSE.PAS') -Destination (Join-Path $stageRoot 'OOPPARSE.PAS')

# Seed the HighScoresLoad oracle with nonzero name payload bytes so a short
# typed-file Read can be compared after Pascal resets lengths and scores.
$hiSeed = New-Object byte[] (30 * 53)
for ($entry = 0; $entry -lt 30; $entry++) {
    $offset = $entry * 53
    $hiSeed[$offset] = 50
    for ($nameIndex = 0; $nameIndex -lt 50; $nameIndex++) {
        $hiSeed[$offset + 1 + $nameIndex] = [byte](65 + (($entry * 7 + $nameIndex) % 26))
    }
    $scoreBytes = [BitConverter]::GetBytes([int16](($entry * 101) - 1515))
    [Array]::Copy($scoreBytes, 0, $hiSeed, $offset + 51, 2)
}
[IO.File]::WriteAllBytes((Join-Path $stageRoot 'HISEED.HI'), $hiSeed)
foreach ($case in @(
    @{ Name = 'HIEMPT'; Length = 0 },
    @{ Name = 'HIC001'; Length = 1 },
    @{ Name = 'HIC052'; Length = 52 },
    @{ Name = 'HIC053'; Length = 53 },
    @{ Name = 'HIC1589'; Length = 1589 },
    @{ Name = 'HIFULL'; Length = 1590 },
    @{ Name = 'HILONG'; Length = 1597 }
)) {
    $length = [int]$case.Length
    $bytes = New-Object byte[] $length
    if ($length -gt 0) { [Array]::Copy($hiSeed, 0, $bytes, 0, [Math]::Min($length, $hiSeed.Length)) }
    if ($length -gt $hiSeed.Length) { [Array]::Copy($hiSeed, 0, $bytes, $hiSeed.Length, $length - $hiSeed.Length) }
    [IO.File]::WriteAllBytes((Join-Path $stageRoot ($case.Name + '.HI')), $bytes)
}

function New-DosboxConfig([string]$Name, [string[]]$Commands, [string]$Cycles = '3000') {
    $lines = [Collections.Generic.List[string]]::new()
    $lines.Add('[sdl]')
    $lines.Add('output=surface')
    $lines.Add('[dosbox]')
    $lines.Add('machine=svga_s3')
    $lines.Add('memsize=16')
    $lines.Add('[cpu]')
    $lines.Add('core=normal')
    $lines.Add(('cycles=' + $Cycles))
    $lines.Add('[autoexec]')
    $lines.Add(('mount c ' + [char]34 + $stageRoot + [char]34))
    $lines.Add('C:')
    $lines.Add('CD \')
    foreach ($command in $Commands) { $lines.Add($command) }
    $lines.Add('exit')
    $configPath = Join-Path $stageRoot ($Name + '.conf')
    [IO.File]::WriteAllLines($configPath, $lines, [Text.Encoding]::ASCII)
    return (Resolve-Path -LiteralPath $configPath).Path
}

function Invoke-DosboxSession([string]$Name, [string[]]$Commands, [string]$Cycles = '3000') {
    $configPath = New-DosboxConfig $Name $Commands $Cycles
    $process = Start-Process -FilePath $DosboxExecutable -ArgumentList @('-conf', ('"{0}"' -f $configPath)) -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(30000)) {
        Stop-Process -Id $process.Id -Force
        throw "DOSBox-X timed out in the $Name session. Review $stageRoot\$Name.LOG."
    }
    if ($process.ExitCode -ne 0) {
        throw "DOSBox-X exited with code $($process.ExitCode) in the $Name session. Review $stageRoot\$Name.LOG."
    }
}

$stagedGamePath = Join-Path $stageSource 'GAME.PAS'
$stagedGame = [IO.File]::ReadAllText($stagedGamePath)
$gameImplementationAnchor = 'implementation'
$gameImplementationIndex = $stagedGame.IndexOf($gameImplementationAnchor, [StringComparison]::Ordinal)
if ($gameImplementationIndex -lt 0) { throw 'Could not locate the Game unit implementation for zero-speed loop instrumentation.' }
$gameProbeDeclarations = @"
const
	GameLoopProbeCellWriteCapacity = 8192;
type
	TGameLoopProbeCellWriteEvent = packed record
		X, Y, Color, Character: byte;
	end;
	TGameLoopProbeCellWriteEventArray = array[1..GameLoopProbeCellWriteCapacity] of TGameLoopProbeCellWriteEvent;
	PGameLoopProbeCellWriteEventArray = ^TGameLoopProbeCellWriteEventArray;
var
	GameLoopProbeStopAfter, GameLoopProbeCycles, GameLoopProbeStartTick: integer;
	GameLoopProbeTransitionCount, GameLoopProbeLastBoard, GameLoopProbeLastState: integer;
	GameLoopProbeEnabled, GameLoopProbeSeedPauseBlink, GameLoopProbeCycleClockEnabled: boolean;
	GameLoopProbeLastPaused, GameLoopProbeCycleCompleted: boolean;
	GameLoopProbeCaptureNextBoardChange: boolean;
	GameLoopProbeTitleLifecycleEnabled, GameLoopProbeTitleLifecyclePlayerReturned: boolean;
	GameLoopProbeTitleRestoreEnabled, GameLoopProbeTitleRestoreFailureEnabled: boolean;
	GameLoopProbeTitleRestoreCancelOnceEnabled, GameLoopProbeTitleRestoreCancelled: boolean;
	GameLoopProbeCellCaptureEnabled, GameLoopProbeCellWriteOverflow: boolean;
	GameLoopProbeCellWriteCount: longint;
	GameLoopProbeCellWriteEvents: PGameLoopProbeCellWriteEventArray;
	procedure GameLoopProbeCapture(checkpointKind: char; checkpointId: integer);

"@
$stagedGame = $stagedGame.Insert($gameImplementationIndex, $gameProbeDeclarations)
$aboutScreenCallAnchor = "TextWindowDisplayFile('ABOUT.HLP', 'About ZZT...');"
if ([regex]::Matches($stagedGame, [regex]::Escape($aboutScreenCallAnchor)).Count -ne 1) {
    throw 'Could not uniquely locate the startup About screen call for the title lifecycle probe.'
}
$stagedGame = $stagedGame.Replace($aboutScreenCallAnchor,
    'if not GameLoopProbeTitleLifecycleEnabled then ' + $aboutScreenCallAnchor)
$probeUsesAnchor = 'uses Dos, Crt, Video, Sounds, Input, Elements, Editor, Oop;'
$probeUsesIndex = $stagedGame.IndexOf($probeUsesAnchor, [StringComparison]::Ordinal)
if ($probeUsesIndex -lt 0) { throw 'Could not locate the staged Game implementation uses clause for integrated checkpoint instrumentation.' }
$probeWriter = @'

procedure GameLoopProbeCapture(checkpointKind: char; checkpointId: integer);
	var
		output: text;
		ix, iy, statId, aliasId, scriptByteIndex, liveStatCount, totalScriptBytes: integer;
		checkpointFieldCount: integer;
		scriptSegment, scriptOffset: word;
		scriptPointer: pointer;
		scriptByte: byte;
		scriptIsFirst: boolean;
	begin
		liveStatCount := Board.StatCount + 1;
		totalScriptBytes := 0;
		for statId := 0 to Board.StatCount do begin
			scriptIsFirst := (Board.Stats[statId].Data <> nil) and
				(Board.Stats[statId].DataLen > 0);
			if scriptIsFirst then begin
				for aliasId := 0 to statId - 1 do
					if (Board.Stats[aliasId].Data = Board.Stats[statId].Data) and
						(Board.Stats[aliasId].DataLen > 0) then
						scriptIsFirst := false;
				if scriptIsFirst then
					totalScriptBytes := totalScriptBytes +
						Board.Stats[statId].DataLen;
			end;
		end;
		checkpointFieldCount := 13 + (14 * liveStatCount) + 11 +
			((BOARD_WIDTH + 2) * (BOARD_HEIGHT + 2) * 2) + 2 +
			(liveStatCount * liveStatCount) + totalScriptBytes + 8 + Length(SoundBuffer) + 2;
		Assign(output, 'INTEGPR.OUT');
		Append(output);
		Write(output, checkpointKind, ' ', checkpointId, ' ', checkpointFieldCount, ' ',
			World.Info.Ammo, ' ', World.Info.Gems, ' ', World.Info.Health,
			' ', World.Info.CurrentBoard, ' ', World.Info.Torches, ' ', World.Info.TorchTicks,
			' ', World.Info.EnergizerTicks, ' ', World.Info.Score,
			' ', World.Info.BoardTimeSec, ' ', World.Info.BoardTimeHsec,
			' ', Ord(World.Info.IsSave), ' ', Board.StatCount, ' ');
		if GameStateElement = E_PLAYER then
			Write(output, 1, ' ')
		else if GameStateElement = E_MONITOR then
			Write(output, 2, ' ')
		else
			Write(output, 0, ' ');
		for statId := 0 to Board.StatCount do begin
			with Board.Stats[statId] do begin
				Write(output, X, ' ', Y, ' ', StepX, ' ', StepY, ' ', Cycle,
					' ', P1, ' ', P2, ' ', P3, ' ', Follower, ' ', Leader,
					' ', Under.Element, ' ', Under.Color, ' ', DataPos, ' ', DataLen, ' ');
			end;
		end;
		Write(output, CurrentTick, ' ', CurrentStatTicked, ' ', TickSpeed,
			' ', TickTimeDuration, ' ', TickTimeCounter, ' ', Ord(GamePaused),
			' ', Ord(GamePlayExitRequested), ' ', InputDeltaX, ' ', InputDeltaY,
			' ', Ord(InputKeyPressed), ' ', Length(InputProbeGetKeyBuffer), ' ');
		for ix := 0 to BOARD_WIDTH + 1 do
			for iy := 0 to BOARD_HEIGHT + 1 do
				Write(output, Board.Tiles[ix][iy].Element, ' ', Board.Tiles[ix][iy].Color, ' ');
		Write(output, (RandSeed and $FFFF), ' ', ((RandSeed shr 16) and $FFFF), ' ');
		for statId := 0 to Board.StatCount do
			for aliasId := 0 to Board.StatCount do
				Write(output, Ord(Board.Stats[statId].Data = Board.Stats[aliasId].Data), ' ');
		for statId := 0 to Board.StatCount do begin
			scriptIsFirst := (Board.Stats[statId].Data <> nil) and
				(Board.Stats[statId].DataLen > 0);
			if scriptIsFirst then begin
				for aliasId := 0 to statId - 1 do
					if (Board.Stats[aliasId].Data = Board.Stats[statId].Data) and
						(Board.Stats[aliasId].DataLen > 0) then
						scriptIsFirst := false;
			end;
			if scriptIsFirst then begin
				scriptSegment := Seg(Board.Stats[statId].Data^);
				scriptOffset := Ofs(Board.Stats[statId].Data^);
				for scriptByteIndex := 0 to Board.Stats[statId].DataLen - 1 do begin
					scriptPointer := Ptr(scriptSegment, scriptOffset + scriptByteIndex);
					Move(scriptPointer^, scriptByte, 1);
					Write(output, scriptByte, ' ');
				end;
			end;
		end;
		Write(output, SoundCurrentPriority, ' ', SoundBufferPos, ' ', Length(SoundBuffer),
			' ', Ord(SoundIsPlaying), ' ', Ord(SoundEnabled), ' ', Ord(SoundBlockQueueing),
			' ', SoundDurationCounter, ' ', SoundDurationMultiplier, ' ');
		for scriptByteIndex := 1 to Length(SoundBuffer) do
			Write(output, Ord(SoundBuffer[scriptByteIndex]), ' ');
		Write(output, GameLoopProbeCellWriteCount, ' ', Ord(GameLoopProbeCellWriteOverflow), ' ');
		WriteLn(output);
		Close(output);
	end;
'@
$probeWriterInsertIndex = $probeUsesIndex + $probeUsesAnchor.Length
$stagedGame = $stagedGame.Insert($probeWriterInsertIndex, $probeWriter)
$boardChangeStart = $stagedGame.LastIndexOf('procedure BoardChange(boardId: integer);', [StringComparison]::Ordinal)
$boardCreateStart = $stagedGame.IndexOf('procedure BoardCreate;', $boardChangeStart, [StringComparison]::Ordinal)
if ($boardChangeStart -lt 0 -or $boardCreateStart -lt 0) { throw 'Could not isolate BoardChange for integrated board-transition capture.' }
$boardChangeBlock = $stagedGame.Substring($boardChangeStart, $boardCreateStart - $boardChangeStart)
$boardOpenAnchor = 'BoardOpen(boardId);'
if ([regex]::Matches($boardChangeBlock, [regex]::Escape($boardOpenAnchor)).Count -ne 1) {
    throw 'Could not uniquely locate BoardOpen inside BoardChange for integrated transition capture.'
}
$boardChangeCapture = @'
if GameLoopProbeEnabled and GameLoopProbeCaptureNextBoardChange then begin
	GameLoopProbeCaptureNextBoardChange := false;
	GameLoopProbeLastBoard := World.Info.CurrentBoard;
	GameLoopProbeTransitionCount := GameLoopProbeTransitionCount + 1;
	GameLoopProbeCapture('T', GameLoopProbeTransitionCount);
end;
'@
$boardChangeBlock = $boardChangeBlock.Replace($boardOpenAnchor, $boardOpenAnchor + "`r`n`t" + $boardChangeCapture.Trim())
$stagedGame = $stagedGame.Substring(0, $boardChangeStart) + $boardChangeBlock + $stagedGame.Substring($boardCreateStart)
$gamePlayLoopStart = $stagedGame.LastIndexOf('procedure GamePlayLoop(boardChanged: boolean);', [StringComparison]::Ordinal)
$gameTitleLoopStart = $stagedGame.IndexOf('procedure GameTitleLoop;', $gamePlayLoopStart, [StringComparison]::Ordinal)
if ($gamePlayLoopStart -lt 0 -or $gameTitleLoopStart -lt 0) { throw 'Could not isolate GamePlayLoop for zero-speed instrumentation.' }
$gamePlayLoop = $stagedGame.Substring($gamePlayLoopStart, $gameTitleLoopStart - $gamePlayLoopStart)
$highScoreCaptureAnchor = 'HighScoresAdd(World.Info.Score);'
if ([regex]::Matches($gamePlayLoop, [regex]::Escape($highScoreCaptureAnchor)).Count -ne 1) {
    throw 'Could not uniquely locate the GamePlayLoop high-score insertion call for F10 presentation capture filtering.'
}
$highScoreCaptureHook = "if GameLoopProbeEnabled and (World.Info.Name = 'F10ROOM') then GameLoopProbeCellCaptureEnabled := false;`r`n`t`t`t`tHighScoresAdd(World.Info.Score);"
$gamePlayLoop = $gamePlayLoop.Replace($highScoreCaptureAnchor, $highScoreCaptureHook)
$startTickAnchor = 'CurrentTick := Random(100);'
$startTickIndex = $gamePlayLoop.IndexOf($startTickAnchor, [StringComparison]::Ordinal)
if ($startTickIndex -lt 0) { throw 'Could not locate GamePlayLoop tick initialization for the zero-speed probe.' }
$startStatAnchor = 'CurrentStatTicked := Board.StatCount + 1;'
$startStatIndex = $gamePlayLoop.IndexOf($startStatAnchor, $startTickIndex + $startTickAnchor.Length, [StringComparison]::Ordinal)
if ($startStatIndex -lt 0) { throw 'Could not locate GamePlayLoop stat initialization for the integrated snapshot.' }
$startStatEnd = $startStatIndex + $startStatAnchor.Length
# GAME.PAS leaves pauseBlink uninitialized. Only F05 opts into a controlled staging seed, matching LibZoo's zero-initialized pause blink state without changing earlier replay traces.
$gamePlayLoop = $gamePlayLoop.Substring(0, $startStatEnd) + "`r`n`t`tGameLoopProbeStartTick := CurrentTick;`r`n`t`tGameLoopProbeCycles := 0;`r`n`t`tGameLoopProbeTransitionCount := 0;`r`n`t`tGameLoopProbeCycleCompleted := false;`r`n`t`tif GameLoopProbeEnabled then begin`r`n`t`t`tif GameLoopProbeSeedPauseBlink then pauseBlink := false;`r`n`t`t`tGameLoopProbeCapture('S', 0);`r`n`t`t`tGameLoopProbeLastBoard := World.Info.CurrentBoard;`r`n`t`t`tGameLoopProbeLastState := GameStateElement;`r`n`t`t`tGameLoopProbeLastPaused := GamePaused;`r`n`t`t`tGameLoopProbeCellCaptureEnabled := true;`r`n`t`tend;" + $gamePlayLoop.Substring($startStatEnd)
$titleLifecycleStartAnchor = "GameLoopProbeCapture('S', 0);"
if ([regex]::Matches($gamePlayLoop, [regex]::Escape($titleLifecycleStartAnchor)).Count -ne 1) {
    throw 'Could not uniquely locate GamePlayLoop start capture for title lifecycle checkpoints.'
}
$titleLifecycleStartProbe = @'
if GameLoopProbeTitleLifecycleEnabled then begin
	if GameStateElement = E_PLAYER then begin
		GameLoopProbeCapture('P', 0);
		GameLoopProbeStopAfter := 1;
	end else if GameLoopProbeTitleLifecyclePlayerReturned then begin
		GameLoopProbeCapture('T', 0);
		GameLoopProbeTitleLifecyclePlayerReturned := false;
		GameLoopProbeEnabled := false;
		GamePlayExitRequested := true;
		GameTitleExitRequested := true;
	end;
end;
'@
$gamePlayLoop = $gamePlayLoop.Replace($titleLifecycleStartAnchor,
    $titleLifecycleStartAnchor + "`r`n`t`t`t" + $titleLifecycleStartProbe.Trim())
$cycleAdvancePattern = '(?s)(CurrentStatTicked := 0;\s*InputUpdate;)'
if ([regex]::Matches($gamePlayLoop, $cycleAdvancePattern).Count -ne 1) {
    throw 'Could not uniquely locate the completed GamePlayLoop cycle for zero-speed instrumentation.'
}
$cycleProbe = @'
GameLoopProbeCycleCompleted := true;
if GameLoopProbeTitleLifecycleEnabled and (GameStateElement = E_MONITOR)
	and ((InputKeyPressed = 'P') or (GameLoopProbeTitleRestoreEnabled and (InputKeyPressed = 'R'))) then
	GamePlayExitRequested := true;
'@
$cycleProbeReplacement = '$1' + "`r`n`t`t`t`t`t" + $cycleProbe.Trim()
$gamePlayLoop = [regex]::Replace($gamePlayLoop, $cycleAdvancePattern, $cycleProbeReplacement, 1)
$unpauseTransitionPattern = '(?s)(GamePaused := false;\s*SidebarClearLine\(5\);\s*CurrentTick := Random\(100\);\s*CurrentStatTicked := Board.StatCount \+ 1;\s*World.Info.IsSave := true;)'
if ([regex]::Matches($gamePlayLoop, $unpauseTransitionPattern).Count -ne 1) {
    throw 'Could not uniquely locate the paused player-movement transition for the integrated replay.'
}
$unpauseTransitionProbe = @'
if GameLoopProbeEnabled then begin
	GameLoopProbeLastBoard := World.Info.CurrentBoard;
	GameLoopProbeLastState := GameStateElement;
	GameLoopProbeLastPaused := GamePaused;
	GameLoopProbeTransitionCount := GameLoopProbeTransitionCount + 1;
	GameLoopProbeCapture('T', GameLoopProbeTransitionCount);
end;
'@
$unpauseTransitionReplacement = '$1' + "`r`n`t`t`t`t`t" + $unpauseTransitionProbe.Trim()
$gamePlayLoop = [regex]::Replace($gamePlayLoop, $unpauseTransitionPattern, $unpauseTransitionReplacement, 1)
$loopEndAnchor = 'until (exitLoop or GamePlayExitRequested) and GamePlayExitRequested;'
if ([regex]::Matches($gamePlayLoop, [regex]::Escape($loopEndAnchor)).Count -ne 1) {
    throw 'Could not uniquely locate the GamePlayLoop iteration boundary for state-transition checkpoints.'
}
$iterationProbe = @'
if GameLoopProbeEnabled then begin
	if (World.Info.CurrentBoard <> GameLoopProbeLastBoard) or
		(GameStateElement <> GameLoopProbeLastState) or
		(GamePaused <> GameLoopProbeLastPaused) then begin
		GameLoopProbeLastBoard := World.Info.CurrentBoard;
		GameLoopProbeLastState := GameStateElement;
		GameLoopProbeLastPaused := GamePaused;
		GameLoopProbeTransitionCount := GameLoopProbeTransitionCount + 1;
		GameLoopProbeCapture('T', GameLoopProbeTransitionCount);
	end;
end;
if GameLoopProbeCycleCompleted then begin
	GameLoopProbeCycleCompleted := false;
	if GameLoopProbeEnabled or (GameLoopProbeStopAfter > 0) then begin
		GameLoopProbeCycles := GameLoopProbeCycles + 1;
		if GameLoopProbeCycleClockEnabled then
			TimerTicks := GameLoopProbeCycles * 2;
		if (GameLoopProbeStopAfter > 0) and
			(GameLoopProbeCycles >= GameLoopProbeStopAfter) then
			GamePlayExitRequested := true;
		if GameLoopProbeEnabled then
			GameLoopProbeCapture('C', GameLoopProbeCycles);
	end;
end;
'@
$loopEndReplacement = $iterationProbe.Trim() + "`r`n`t`t" + $loopEndAnchor
$gamePlayLoop = $gamePlayLoop.Replace($loopEndAnchor, $loopEndReplacement)
$stagedGame = $stagedGame.Substring(0, $gamePlayLoopStart) + $gamePlayLoop + $stagedGame.Substring($gameTitleLoopStart)
$gameTitleLoopStart = $stagedGame.IndexOf('procedure GameTitleLoop;', $gamePlayLoopStart + $gamePlayLoop.Length, [StringComparison]::Ordinal)
if ($gameTitleLoopStart -lt 0) { throw 'Could not relocate the GameTitleLoop implementation after staging GamePlayLoop hooks.' }
$gameTitleLoopEnd = $stagedGame.IndexOf('procedure GamePrintRegisterMessage;', $gameTitleLoopStart, [StringComparison]::Ordinal)
if ($gameTitleLoopEnd -lt 0) { throw 'Could not isolate GameTitleLoop for lifecycle return instrumentation.' }
$gameTitleLoop = $stagedGame.Substring($gameTitleLoopStart, $gameTitleLoopEnd - $gameTitleLoopStart)
$playerLoopAnchor = 'GamePlayLoop(true);'
if ([regex]::Matches($gameTitleLoop, [regex]::Escape($playerLoopAnchor)).Count -ne 1) {
    throw 'Could not uniquely locate the player-loop return in GameTitleLoop.'
}
$gameTitleLoop = $gameTitleLoop.Replace($playerLoopAnchor,
    $playerLoopAnchor + "`r`n`t`t`t`t`tif GameLoopProbeTitleLifecycleEnabled then GameLoopProbeTitleLifecyclePlayerReturned := true;")
$saveRestoreAnchor = "if GameWorldLoad('.SAV') then begin"
if ([regex]::Matches($gameTitleLoop, [regex]::Escape($saveRestoreAnchor)).Count -ne 1) {
    throw 'Could not uniquely locate the GameTitleLoop saved-game selector for deterministic restore input.'
}
# Test-only deterministic selector input. The reference GameWorldLoad and TextWindowSelect routines remain unchanged.
$restoreInputHook = "if GameLoopProbeTitleRestoreCancelOnceEnabled and not GameLoopProbeTitleRestoreCancelled then`r`n`t`t`t`t`t`tTextWindowProbeInjectEscape := true`r`n`t`t`t`t`telse if GameLoopProbeTitleRestoreEnabled then`r`n`t`t`t`t`t`tInputProbeSetKeyBuffer(KEY_ENTER);`r`n`t`t`t`t`t"
$gameTitleLoop = $gameTitleLoop.Replace($saveRestoreAnchor,
	$restoreInputHook + $saveRestoreAnchor)
$restoreFailurePattern = "(?s)if GameWorldLoad\('\.SAV'\) then begin\s+ReturnBoardId := World\.Info\.CurrentBoard;\s+BoardChange\(ReturnBoardId\);\s+startPlay := true;\s+end;"
if ([regex]::Matches($gameTitleLoop, $restoreFailurePattern).Count -ne 1) {
    throw 'Could not uniquely locate the GameTitleLoop restore result branch for malformed-save capture.'
}
$restoreFailureReplacement = @'
if GameWorldLoad('.SAV') then begin
							ReturnBoardId := World.Info.CurrentBoard;
							BoardChange(ReturnBoardId);
							startPlay := true;
						end else if GameLoopProbeTitleRestoreFailureEnabled then begin
							GameTitleExitRequested := true;
						end else if GameLoopProbeTitleRestoreCancelOnceEnabled and not GameLoopProbeTitleRestoreCancelled then begin
							GameLoopProbeTitleRestoreCancelled := true;
							GameLoopProbeCapture('X', 0);
							InputProbeSetKeyBuffer('R');
						end;
'@
$gameTitleLoop = [regex]::Replace($gameTitleLoop, $restoreFailurePattern, $restoreFailureReplacement, 1)
$stagedGame = $stagedGame.Substring(0, $gameTitleLoopStart) + $gameTitleLoop + $stagedGame.Substring($gameTitleLoopEnd)
[IO.File]::WriteAllText($stagedGamePath, $stagedGame, [Text.UTF8Encoding]::new($false))

Set-TitleRestoreEscapeInputHook
Write-Host 'Compiling the reconstructed Pascal engine...'
Invoke-DosboxSession 'ENGINE' @('TPC.EXE /B /EBIN /GD /ISRC /USRC SRC\ZZT.PAS > ENGINE.LOG')
$engineLogPath = Join-Path $stageRoot 'ENGINE.LOG'
$engineLogText = [IO.File]::ReadAllText($engineLogPath)
if ($engineLogText -match 'Error\s+\d+:') {
    throw "Turbo Pascal engine compilation reported an error. See $engineLogPath."
}
Write-Host 'Compiling and running the direct Turbo Pascal shared-#BIND-alias probe...'
# Compile against the freshly built engine units without /B. A forced unit rebuild produces
# Turbo Pascal 5.5 Error 70 (unit version mismatch) in this allocator-sensitive probe.
Invoke-DosboxSession 'BINDALIA-COMPILE' @('TPC.EXE /EBIN /UBIN BINDALIA.PAS > BINDALIA.LOG')
Invoke-DosboxSession 'BINDALIA-RUN' @('BIN\BINDALIA.EXE') 'max'
Write-Host 'Compiling and running direct Turbo Pascal #BIND allocator-reuse probe...'
Invoke-DosboxSession 'BINDREUS' @(
    'TPC.EXE /EBIN /UBIN BINDREUS.PAS > BINDREUS.LOG',
    'BIN\BINDREUS.EXE'
)
Write-Host 'Compiling and running direct Turbo Pascal #BIND multi-buffer reuse-order probe...'
Invoke-DosboxSession 'BINDORDR-COMPILE' @('TPC.EXE /EBIN /UBIN BINDORDR.PAS > BINDORDR.LOG')
Invoke-DosboxSession 'BINDORDR-RUN' @('BIN\BINDORDR.EXE') 'max'
Write-Host 'Compiling and running direct Turbo Pascal #BIND allocation-size cases...'
Invoke-DosboxSession 'BINDSIZE-COMPILE' @('TPC.EXE /EBIN /UBIN BINDSIZE.PAS > BINDSIZE.LOG')
# Each case constructs its allocation graph from scratch. Running it after the
# compiler or another case reuses DOS memory that changes the observed block
# placement, so execute every deterministic scenario from a clean runtime.
$bindSizeScenarios = @('SML', 'MED', 'LRG', 'XL', 'F41', 'F45', 'F50', 'F55', 'F60', 'F63', 'F64')
foreach ($scenario in $bindSizeScenarios) {
    Invoke-DosboxSession "BINDSIZE-$scenario" @("BIN\BINDSIZE.EXE $scenario") 'max'
}
Write-Host 'Compiling and running direct Turbo Pascal #BIND small-tail allocation cases...'
$bindTailCommands = [Collections.Generic.List[string]]::new()
$bindTailCommands.Add('TPC.EXE /EBIN /UBIN BINDTAIL.PAS > BINDTAIL.LOG')
$bindTailScenarios = @(26..64 | ForEach-Object { 'T{0}' -f $_ })
foreach ($scenario in $bindTailScenarios) { $bindTailCommands.Add(('BIN\BINDTAIL.EXE {0}' -f $scenario)) }
Invoke-DosboxSession 'BINDTAIL' $bindTailCommands.ToArray()
$bindTailRows = [Collections.Generic.List[string]]::new()
foreach ($scenario in $bindTailScenarios) {
    $casePath = Join-Path $stageRoot ($scenario + '.OUT')
    if (-not (Test-Path -LiteralPath $casePath -PathType Leaf)) {
        throw "Turbo Pascal #BIND small-tail case $scenario did not write output."
    }
    $caseLines = [IO.File]::ReadAllLines($casePath)
    if ($caseLines.Count -ne 1 -or $caseLines[0] -notmatch ('^{0} [01] -?\d+ [01]$' -f $scenario)) {
        throw "Unexpected Turbo Pascal #BIND small-tail case $scenario output: $([string]::Join('; ', $caseLines))"
    }
    $caseFields = @($caseLines[0] -split '\s+')
    $requestSize = [int]$scenario.Substring(1)
    $expectedSourceReuse = if ($requestSize -le 63) { '1' } else { '0' }
    if ($caseFields[1] -cne $expectedSourceReuse) {
        throw "Turbo Pascal #BIND source-block reuse did not match BINDSIZE at $scenario."
    }
    $expectedLargeReuse = if ($requestSize -eq 27) { '1' } else { '0' }
    if ($caseFields[3] -cne $expectedLargeReuse) {
        throw "Turbo Pascal #BIND follow-up allocation selected an unexpected original block at $scenario."
    }
    if ($requestSize -ge 26 -and $requestSize -le 62 -and [int]$caseFields[2] -ne $requestSize) {
        throw "Turbo Pascal #BIND follow-up allocation did not begin at +$requestSize."
    }
    if ($requestSize -eq 63 -and [int]$caseFields[2] -ne 85) {
        throw 'The 63-byte #BIND case did not allocate its follow-up after the live target block.'
    }
    if ($requestSize -eq 64 -and [int]$caseFields[2] -ne -85) {
        throw 'The 64-byte #BIND case did not allocate a fresh first block then reuse the earlier small block.'
    }
    $bindTailRows.Add($caseLines[0])
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'BINDTAIL.OUT'), $bindTailRows, [Text.Encoding]::ASCII)
Write-Host 'Turbo Pascal #BIND tail splits: checked contiguous follow-up addresses, exact coalesced-block reuse, and fresh-allocation fallback for requests 26 through 64.'
Write-Host 'Compiling and running direct Turbo Pascal #BIND selector cases...'
Invoke-DosboxSession 'BINDSEL' @(
    'TPC.EXE /EBIN /UBIN BINDSEL.PAS > BINDSEL.LOG',
    'BIN\BINDSEL.EXE 1',
    'BIN\BINDSEL.EXE 2',
    'BIN\BINDSEL.EXE 3',
    'BIN\BINDSEL.EXE 4',
    'BIN\BINDSEL.EXE 5'
)
$bindSelectorExpected = @(
    '1 9 0 0 21',
    '2 0 7 0 21',
    '3 9 0 0 22',
    '4 5 0 0 28',
    '5 0 13 0 22'
)
$bindSelectorRows = [Collections.Generic.List[string]]::new()
for ($caseId = 1; $caseId -le $bindSelectorExpected.Count; $caseId++) {
    $caseOutputPath = Join-Path $stageRoot ('BINDSE{0}.OUT' -f $caseId)
    if (-not (Test-Path -LiteralPath $caseOutputPath -PathType Leaf)) {
        throw "Turbo Pascal BIND selector case $caseId did not create its output file."
    }
    $caseOutput = [IO.File]::ReadAllLines($caseOutputPath)
    if ($caseOutput.Count -ne 1 -or $caseOutput[0] -cne $bindSelectorExpected[$caseId - 1]) {
        throw "Unexpected Turbo Pascal BIND selector case $caseId output: $([string]::Join('; ', $caseOutput))"
    }
    $bindSelectorRows.Add($caseOutput[0])
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'BINDSEL.OUT'), $bindSelectorRows, [Text.Encoding]::ASCII)
Write-Host 'Compiling and running direct Turbo Pascal #BIND/TextWindow allocation boundary cases...'
Invoke-DosboxSession 'BINDWIN' @(
    'TPC.EXE /EBIN /UBIN BINDWIN.PAS > BINDWIN.LOG',
    'BIN\BINDWIN.EXE 50',
    'BIN\BINDWIN.EXE 51'
)
$bindWindowExpected = @(
    '50 0 35 66 73 35 9',
    '51 1 50 48 49 50 9'
)
$bindWindowRows = [Collections.Generic.List[string]]::new()
foreach ($lineSize in @(50, 51)) {
    $bindWindowOutput = Join-Path $stageRoot ('BW{0}.OUT' -f $lineSize)
    if (-not (Test-Path -LiteralPath $bindWindowOutput -PathType Leaf)) {
        throw "Turbo Pascal #BIND/TextWindow line-size case $lineSize did not write output."
    }
    $bindWindowLine = [IO.File]::ReadAllLines($bindWindowOutput)
    if ($bindWindowLine.Count -ne 1 -or $bindWindowLine[0] -cne $bindWindowExpected[$bindWindowRows.Count]) {
        throw "Unexpected Turbo Pascal #BIND/TextWindow case $lineSize output: $([string]::Join('; ', $bindWindowLine))"
    }
    $bindWindowRows.Add($bindWindowLine[0])
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'BINDWIN.OUT'), $bindWindowRows, [Text.Encoding]::ASCII)
Write-Host 'Turbo Pascal #BIND/TextWindow: a 50-byte block is too short for the 51-byte short string; a 51-byte block is reused and overwritten.'
Write-Host 'Compiling and running the direct Turbo Pascal note-frequency probe...'
Invoke-DosboxSession 'SOUNDFRQ' @(
    'TPC.EXE /EBIN /UBIN SOUNDFRQ.PAS > SOUNDFRQ.LOG',
    'BIN\SOUNDFRQ.EXE'
)
Set-ScrollPascalUnitHooks
foreach ($probe in $probeFiles) {
    $baseName = [IO.Path]::GetFileNameWithoutExtension($probe.DosName)
    $compile = "TPC.EXE $($probe.CompileFlags) $($probe.DosName) > $baseName.LOG"
    Write-Host "Compiling and running Turbo Pascal probe: $baseName"
    # NEGLEN reads a 65,535-byte payload through the original signed-length path.
    # Compile and execute separately because the compiler's allocation history
    # can alter this unchecked malformed-file load result. Three fresh sessions
    # return the same -1 record length. Max cycles only reduces host wait time
    # for this non-clock-dependent file-load probe.
    if ($probe.Program -in @('NEGLEN', 'NEGBRD', 'STARPRB', 'CENTIPR', 'PUSHDUP', 'BINDPROB', 'SLIMEPRB', 'DRAWPRB', 'MSGTIK', 'TORCHPR', 'HILODPRB', 'BOARDRLE')) {
        $sessionPrefix = $probe.Program
        Invoke-DosboxSession "$sessionPrefix-COMPILE" @($compile) 'max'
        if (-not (Test-Path -LiteralPath (Join-Path $stageBin "$($probe.Program).EXE") -PathType Leaf)) {
            throw "Turbo Pascal did not produce BIN\$($probe.Program).EXE. See $stageRoot\$($probe.Program).LOG and $stageRoot\$sessionPrefix-COMPILE.LOG."
        }
        # Fresh maximum-speed runtime sessions avoid compiler allocation
        # history in malformed signed-length loads. STARPRB and CENTIPR also
        # need a clean DOS allocation image: STARPRB's seeded tick fixture
        # leaves StepX/StepY untouched before its first callback, while
        # CENTIPR performs a deterministic 100,001-candidate seed scan before
        # it opens the output file. PUSHDUP performs repeated board/stat
        # allocation and callback cycles, and BINDPROB checks #BIND pointer
        # alias state. SLIMEPRB rebuilds the board between five callback
        # cases, and DRAWPRB repeatedly captures draw/tick output. MSGTIK
        # captures seven message-timer render/decrement cases. TORCHPR writes
        # no output when run in the compiler's allocation history; a fresh
        # runtime session produces both expected rows. HILODPRB creates a world
        # and loads eight high-score files; the compiler's allocation history
        # likewise prevented its output files, while a fresh runtime writes all
        # eight. BOARDRLE's 256-run-count BoardOpen sweep writes no bytes in the
        # compiler's allocation history; a fresh runtime writes all 768,256.
        # All are clock-independent.
        Invoke-DosboxSession "$sessionPrefix-RUN" @("BIN\$($probe.Program).EXE") 'max'
    } else {
        Invoke-DosboxSession $baseName @($compile, "BIN\$($probe.Program).EXE") '3000'
    }
}

Write-Host 'Compiling and running isolated direct Turbo Pascal OOP #IF cases: OOPIFPRB'
$oopIfCommands = [Collections.Generic.List[string]]::new()
$oopIfCommands.Add('TPC.EXE /B /EBIN /UBIN OOPIFPRB.PAS > OOPIFPRB.LOG')
for ($caseId = 1; $caseId -le 55; $caseId++) {
    $oopIfCommands.Add(('BIN\OOPIFPRB.EXE {0}' -f $caseId))
}
Invoke-DosboxSession 'OOPIFPRB' $oopIfCommands.ToArray()
$oopIfRows = [Collections.Generic.List[string]]::new()
for ($caseId = 1; $caseId -le 55; $caseId++) {
    $caseOutputPath = Join-Path $stageRoot ('OOPIF{0:D2}.OUT' -f $caseId)
    if (-not (Test-Path -LiteralPath $caseOutputPath -PathType Leaf)) {
        throw "Turbo Pascal OOP IF case $caseId did not create its output file."
    }
    $caseOutput = [IO.File]::ReadAllLines($caseOutputPath)
    if ($caseOutput.Count -ne 2 -or $caseOutput[0] -cne 'started' -or
        $caseOutput[1] -notmatch ('^{0} [01]$' -f $caseId)) {
        throw "Unexpected Turbo Pascal OOP IF case $caseId output: $([string]::Join('; ', $caseOutput))"
    }
    $oopIfRows.Add($caseOutput[1])
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'OOPIF.OUT'), $oopIfRows, [Text.Encoding]::ASCII)

Write-Host 'Compiling and running isolated direct Turbo Pascal OOP #GIVE/#TAKE counter cases: OOPGIVPRB'
$oopGiveCommands = [Collections.Generic.List[string]]::new()
$oopGiveCommands.Add('TPC.EXE /B /EBIN /UBIN OOPGIV.PAS > OOPGIV.LOG')
for ($caseId = 1; $caseId -le 24; $caseId++) {
    $oopGiveCommands.Add(('BIN\OOPGIV.EXE {0}' -f $caseId))
}
Invoke-DosboxSession 'OOPGIVPRB' $oopGiveCommands.ToArray()
$oopGiveRows = [Collections.Generic.List[string]]::new()
for ($caseId = 1; $caseId -le 24; $caseId++) {
    $caseOutputPath = Join-Path $stageRoot ('OOPGIV{0:D2}.OUT' -f $caseId)
    if (-not (Test-Path -LiteralPath $caseOutputPath -PathType Leaf)) {
        throw "Turbo Pascal OOP #GIVE/#TAKE case $caseId did not create its output file."
    }
    $caseOutput = [IO.File]::ReadAllLines($caseOutputPath)
    if ($caseOutput.Count -ne 1 -or $caseOutput[0] -notmatch ('^{0} -?\d+ -?\d+$' -f $caseId)) {
        throw "Unexpected Turbo Pascal OOP #GIVE/#TAKE case $caseId output: $([string]::Join('; ', $caseOutput))"
    }
    $oopGiveRows.Add($caseOutput[0])
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'OOPGIV.OUT'), $oopGiveRows, [Text.Encoding]::ASCII)

Write-Host 'Compiling and running direct Turbo Pascal OOP #CHAR boundary cases: OOPCHARPRB'
$oopCharCommands = [Collections.Generic.List[string]]::new()
$oopCharCommands.Add('TPC.EXE /B /EBIN /UBIN OOPCHAR.PAS > OOPCHAR.LOG')
for ($caseId = 1; $caseId -le 8; $caseId++) {
    $oopCharCommands.Add(('BIN\OOPCHAR.EXE {0}' -f $caseId))
}
Invoke-DosboxSession 'OOPCHARPRB' $oopCharCommands.ToArray()
$oopCharRows = [Collections.Generic.List[string]]::new()
for ($caseId = 1; $caseId -le 8; $caseId++) {
    $caseOutputPath = Join-Path $stageRoot ('OOPCH{0:D2}.OUT' -f $caseId)
    if (-not (Test-Path -LiteralPath $caseOutputPath -PathType Leaf)) {
        throw "Turbo Pascal OOP #CHAR case $caseId did not create its output file."
    }
    $caseOutput = [IO.File]::ReadAllLines($caseOutputPath)
    if ($caseOutput.Count -ne 1 -or $caseOutput[0] -notmatch ('^\s*{0}\s+-?\d+\s+-?\d+\s+-?\d+$' -f $caseId)) {
        throw "Unexpected Turbo Pascal OOP #CHAR case $caseId output: $([string]::Join('; ', $caseOutput))"
    }
    $oopCharRows.Add($caseOutput[0].Trim())
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'OOPCHAR.OUT'), $oopCharRows, [Text.Encoding]::ASCII)

Write-Host 'Compiling and running direct Turbo Pascal OOP #PUT board-edge cases: OOPPUTPRB'
$oopPutCommands = [Collections.Generic.List[string]]::new()
$oopPutCommands.Add('TPC.EXE /B /EBIN /UBIN OOPPUT.PAS > OOPPUT.LOG')
for ($caseId = 1; $caseId -le 8; $caseId++) {
    $oopPutCommands.Add(('BIN\OOPPUT.EXE {0}' -f $caseId))
}
Invoke-DosboxSession 'OOPPUTPRB' $oopPutCommands.ToArray()
$oopPutRows = [Collections.Generic.List[string]]::new()
for ($caseId = 1; $caseId -le 8; $caseId++) {
    $caseOutputPath = Join-Path $stageRoot ('OOPPU{0:D2}.OUT' -f $caseId)
    if (-not (Test-Path -LiteralPath $caseOutputPath -PathType Leaf)) {
        throw "Turbo Pascal OOP #PUT case $caseId did not create its output file."
    }
    $caseOutput = [IO.File]::ReadAllLines($caseOutputPath)
    if ($caseOutput.Count -ne 1 -or $caseOutput[0] -notmatch ('^\s*{0}(?:\s+-?\d+){{9}}$' -f $caseId)) {
        throw "Unexpected Turbo Pascal OOP #PUT case $caseId output: $([string]::Join('; ', $caseOutput))"
    }
    $oopPutRows.Add($caseOutput[0].Trim())
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'OOPPUT.OUT'), $oopPutRows, [Text.Encoding]::ASCII)

Write-Host 'Compiling and running direct Turbo Pascal OOP #CHANGE board-map case: OOPCHGPRB'
Invoke-DosboxSession 'OOPCHGPRB' @(
    'TPC.EXE /B /EBIN /UBIN OOPCHG.PAS > OOPCHG.LOG',
    'BIN\OOPCHG.EXE'
)
$oopChangeOutput = Join-Path $stageRoot 'OOPCHG.OUT'
if (-not (Test-Path -LiteralPath $oopChangeOutput -PathType Leaf)) {
    throw 'Turbo Pascal OOP #CHANGE probe did not create OOPCHG.OUT.'
}

Write-Host 'Compiling and running direct Turbo Pascal OOP flag storage cases: OOPFLAGPRB'
Invoke-DosboxSession 'OOPFLAGPRB' @(
    'TPC.EXE /B /EBIN /UBIN OOPFLAGS.PAS > OOPFLAGS.LOG',
    'BIN\OOPFLAGS.EXE'
)
$oopFlagsOutput = Join-Path $stageRoot 'OOPFLAGS.OUT'
if (-not (Test-Path -LiteralPath $oopFlagsOutput -PathType Leaf)) {
    throw 'Turbo Pascal OOP flag probe did not create OOPFLAGS.OUT.'
}

Write-Host 'Compiling and running direct Turbo Pascal OOP #SEND lock and target-iteration cases: OOPSENDPRB'
Invoke-DosboxSession 'OOPSEND-COMPILE' @('TPC.EXE /B /EBIN /UBIN OOPSEND.PAS > OOPSEND.LOG') 'max'
if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'OOPSEND.EXE') -PathType Leaf)) {
    throw "Turbo Pascal did not produce BIN\OOPSEND.EXE. See $stageRoot\OOPSEND.LOG and $stageRoot\OOPSEND-COMPILE.LOG."
}
# The pointer-alias and target-iteration fixture writes no rows in the
# compiler's DOS allocation history; run the compiled direct Pascal probe in a
# fresh maximum-speed session to control allocator history.
Invoke-DosboxSession 'OOPSENDPRB' @('BIN\OOPSEND.EXE') 'max'
$oopSendOutput = Join-Path $stageRoot 'OOPSEND.OUT'
if (-not (Test-Path -LiteralPath $oopSendOutput -PathType Leaf)) {
    throw 'Turbo Pascal OOP #SEND probe did not create OOPSEND.OUT.'
}

Write-Host 'Compiling direct Turbo Pascal ElementObjectTick cases: OBJTIK'
Invoke-DosboxSession 'OBJTIK-COMPILE' @('TPC.EXE /B /EBIN /UBIN OBJTIK.PAS > OBJTIK.LOG') 'max'
if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'OBJTIK.EXE') -PathType Leaf)) {
    throw "Turbo Pascal did not produce BIN\OBJTIK.EXE. See $stageRoot\OBJTIK.LOG and $stageRoot\OBJTIK-COMPILE.LOG."
}
# Run each compiled direct handler case from a fresh DOSBox-X allocation
# history. Consecutive process exits in one DOSBox session made case 2 hang
# despite cases 2-4 completing and matching individually in isolated sessions.
# These direct ElementObjectTick calls do not consult the DOS clock.
for ($caseId = 1; $caseId -le 4; $caseId++) {
    $caseName = 'OBJTIK{0:D2}' -f $caseId
    Invoke-DosboxSession…12304 tokens truncated…-1',
        'bomb-tick 2 1 2 0 112 13 40 0 80 1 2 248 1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1',
        'bomb-tick 3 1 1 0 112 13 40 23 13 -1 0 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1',
        'bomb-tick 4 0 -1 -1 -1 0 112 0 13 -1 0 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1',
        'bomb-tick 5 1 254 0 112 13 40 0 80 1 2 248 1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1'
    )
    'CONVPRB.OUT' = @(
        'conveyor-tick 1 1 10 10 16 7 0 112 0 112 0 112 0 112 0 112 0 112 0 112 24 30',
        'conveyor-tick 2 1 10 10 17 7 24 46 0 112 0 112 0 112 0 112 0 112 0 112 0 112'
    )
    'TERRAIN.OUT' = @(
        'terrain-state',
        'invisible 22 42 37 1 0 0 20 3 4 18 1 16 1 2 -1',
        'forest-first 0 43 37 0 1 40 3 2 57 1 2 -1',
        'forest-repeat 0 43 0 0 0 3 2 57 1 2 -1',
        'fake-first 27 44 29 0 1 30 -1 0 2 -1',
        'fake-repeat 27 44 0 0 0 -1 0 2 -1',
        'water 19 45 29 1 20 3 4 64 1 80 1 2 -1'
    )
    'DAMAGE.OUT' = @(
        'damage-nonlethal health 5 player 4 127 tile 0 0 message "Ouch!" stats 1 20 sound 4 8 16 1 32 1 19 1 35 1 delta 2 -1',
        'damage-lethal health 0 player 4 127 tile 0 0 message "Ouch!" stats 1 20 sound 5 26 32 3 35 3 39 3 48 3 39 3 42 3 50 3 55 3 53 3 56 3 64 3 69 3 16 10 delta 2 -1',
        'slime-touch health 100 player 4 31 tile 23 58 message "" stats 0 sound 3 8 64 1 16 1 80 1 48 1 delta 2 -1'
    )
    'PASSAGE.OUT' = @('passage 1 player 22 17 start 22 17 spawn 0 0 first 11 42 last 11 42 paused 1 time 0 delta 0 0 sound 4 30 48 1 52 1 55 1 49 1 53 1 56 1 50 1 54 1 57 1 51 1 55 1 58 1 52 1 56 1 64 1')
    'BOARDENT.OUT' = @(
        'edge-north board 1 player 30 25 tile 4 delta 0 0 start 30 25 time 0 paused 0',
        'edge-south board 2 player 30 1 tile 4 delta 0 0 start 30 1 time 0 paused 0',
        'edge-west board 3 player 60 12 tile 4 delta 0 0 start 60 12 time 0 paused 0',
        'edge-east board 4 player 1 12 tile 4 delta 0 0 start 1 12 time 0 paused 0',
        'edge-blocked board 0 player 30 1 tile 4 delta 0 -1 start 30 1 time 42 paused 0',
        'edge-existing-player board 1 player 30 25 tile 4 delta 0 0 start 30 25 time 0 paused 0'
    )
    'BOARDIN.OUT' = @(
        'light start 12 19 time 0 hint 1 stats 0 message-length 0',
        'dark-first start 13 17 time 0 hint 0 stats 1 timer 22 message Room is dark - you need to light a torch!',
        'dark-repeat start 20 12 time 0 hint 0 stats 1 timer 22 message-length 41'
    )
    'TRANSPRT.OUT' = @(
        'transport-open player 12 10 delta 0 0 sound 3 16 48 1 66 1 52 1 70 1 56 1 74 1 64 1 82 1',
        'transport-reverse-chain player 13 10 delta 0 0 sound 3 16 48 1 66 1 52 1 70 1 56 1 74 1 64 1 82 1',
        'transport-wrong-way player 10 10 delta 0 0 sound -1 0',
        'transport-blocked player 10 10 delta 0 0 sound -1 0'
    )
    'BULLETPR.OUT' = @(
        'bullet-move stats 1 bullet 11 10 step 1 0 old 0 112 target 18 14 left 0 112 up 0 112 down 0 112 under 0 112 tick 1 sound -1 0',
        'bullet-water stats 1 bullet 11 10 step 1 0 old 0 112 target 18 78 left 0 112 up 0 112 down 0 112 under 19 77 tick 1 sound -1 0',
        'bullet-direct-ricochet stats 1 bullet 9 10 step -1 0 old 0 112 target 32 15 left 18 14 up 0 112 down 0 112 under 0 112 tick 1 sound 1 2 249 1',
        'bullet-side-down stats 1 bullet 10 9 step 0 -1 old 0 112 target 21 15 left 0 112 up 18 14 down 32 15 under 0 112 tick 1 sound 1 2 249 1',
        'bullet-side-up stats 1 bullet 10 11 step 0 1 old 0 112 target 21 15 left 0 112 up 32 15 down 18 14 under 0 112 tick 1 sound 1 2 249 1',
        'bullet-blocked stats 0 bullet 10 10 step 1 0 old 0 112 target 21 15 left 0 112 up 0 112 down 0 112 under 0 112 tick 0 sound -1 0'
    )
    'SHOTPRB.OUT' = @(
        'shot-empty result 1 stats 2 target 18 15 health 100 sound -1 0 bullet 11 10 0 1 0 100 under 0 42',
        'shot-water result 1 stats 2 target 18 31 health 100 sound -1 0 bullet 11 10 1 1 0 100 under 19 23',
        'shot-solid result 0 stats 1 target 21 31 health 100 sound -1 0',
        'shot-breakable result 1 stats 1 target 0 46 health 100 sound 2 2 16 1',
        'shot-lion-player result 1 stats 1 target 0 12 health 100 sound 2 2 16 1',
        'shot-door-player result 0 stats 1 target 9 47 health 100 sound -1 0',
        'shot-player-friendly result 0 stats 1 target 4 31 health 100 sound -1 0',
        'shot-player-enemy result 1 stats 2 target 4 127 health 90 sound 4 8 16 1 32 1 19 1 35 1',
        'shot-player-energized result 0 stats 1 target 4 31 health 100 sound -1 0'
    )
    'BINDPROB.OUT' = @('7 25 0', '9 22 0')
    'BINDALIA.OUT' = @('before', 'returned 9 22 0 27', 'follower 5', 'follower 5')
    'BINDORDR.OUT' = @('started', 'bound', 'reuse 1 0 0 1', 'followers 7 8')
    'SIZSML.OUT' = @('SML 14 1 0 0 88 35')
    'SIZMED.OUT' = @('MED 27 1 0 0 88 35')
    'SIZLRG.OUT' = @('LRG 36 1 0 0 88 69')
    'SIZXL.OUT' = @('XL 70 0 0 0 35 35')
    'SIZF41.OUT' = @('F41 41 1 0 0 88 88')
    'SIZF45.OUT' = @('F45 45 1 0 0 88 88')
    'SIZF50.OUT' = @('F50 50 1 0 0 88 88')
    'SIZF55.OUT' = @('F55 55 1 0 0 88 88')
    'SIZF60.OUT' = @('F60 60 1 0 0 88 88')
    'SIZF63.OUT' = @('F63 63 1 0 0 88 88')
    'SIZF64.OUT' = @('F64 64 0 0 0 35 35')
    'OOPIF.OUT' = @(
        '1 1', '2 0', '3 1', '4 0', '5 1', '6 0', '7 1', '8 0',
        '9 1', '10 0', '11 1', '12 0', '13 1', '14 0', '15 1', '16 0',
        '17 1', '18 0', '19 1', '20 0', '21 1', '22 0', '23 1', '24 0',
        '25 1', '26 0', '27 1', '28 0', '29 1', '30 1', '31 0', '32 1',
        '33 0', '34 1', '35 0', '36 1', '37 0', '38 1', '39 0', '40 1', '41 0',
        '42 1', '43 1', '44 1', '45 1', '46 1', '47 0', '48 0', '49 1',
        '50 1', '51 0', '52 1', '53 0', '54 1', '55 0'
    )
}
foreach ($fileName in $expectedOutputs.Keys) {
    $outputPath = Join-Path $stageRoot $fileName
    if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf)) {
        throw "Pascal probe did not write $fileName. See $stageRoot\$([IO.Path]::GetFileNameWithoutExtension($fileName)).LOG."
    }
    $actualLines = @([IO.File]::ReadAllLines($outputPath))
    $actualText = [string]::Join([char]10, $actualLines)
    $expectedText = [string]::Join([char]10, $expectedOutputs[$fileName])
    if ($actualText -cne $expectedText) {
        throw ("Unexpected Turbo Pascal output in {0}: {1}{2}" -f $fileName, [Environment]::NewLine, $actualText)
    }
    Write-Host "Turbo Pascal probe passed: $fileName"
}

$bindReuseOutput = [IO.File]::ReadAllLines((Join-Path $stageRoot 'BINDREUS.OUT'))
if ($bindReuseOutput.Count -ne 4 -or $bindReuseOutput[0] -cne 'started' -or $bindReuseOutput[1] -cne 'bound' -or
    -not (($bindReuseOutput[2] -ceq 'reused 1' -and $bindReuseOutput[3] -ceq 'follower 7') -or
        ($bindReuseOutput[2] -ceq 'reused 0' -and $bindReuseOutput[3] -ceq 'follower skipped'))) {
    throw ("Unexpected Turbo Pascal #BIND allocator trace: {0}" -f [string]::Join(' | ', $bindReuseOutput))
}
Write-Host ("Turbo Pascal #BIND allocator trace: {0} | {1}" -f $bindReuseOutput[2], $bindReuseOutput[3])

$shortReadPath = Join-Path $stageRoot 'SHORTRD.OUT'
if (-not (Test-Path -LiteralPath $shortReadPath -PathType Leaf)) {
    throw "Turbo Pascal short-board-read probe did not write SHORTRD.OUT. See $stageRoot\SHORTRD.LOG."
}
$shortReadLines = [IO.File]::ReadAllLines($shortReadPath)
if ($shortReadLines.Count -ne 1 -or $shortReadLines[0] -notmatch '^short-read 1 15 0 16 tail ([0-9]{1,3})$') {
    throw "Unexpected Turbo Pascal short-board-read output: $([string]::Join('; ', $shortReadLines))"
}
$shortReadTailByte = [int]$Matches[1]
if ($shortReadTailByte -gt 255) { throw "Turbo Pascal short-read tail was outside byte range: $shortReadTailByte" }
Write-Host "Turbo Pascal short payload read succeeds; unread tail byte $shortReadTailByte is allocator-dependent."
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'SOUNDFRQ.OUT') -PathType Leaf)) {
    throw "Turbo Pascal sound-frequency probe did not write SOUNDFRQ.OUT. See $stageRoot\SOUNDFRQ.LOG."
}

if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'PASOUT.SAV') -PathType Leaf)) {
    throw "WorldSaveProbe did not write PASOUT.SAV. See $stageRoot\WORLDSAV.LOG."
}
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'PASPROBE.HI') -PathType Leaf)) {
    throw "HiSaveProbe did not write PASPROBE.HI. See $stageRoot\HISAVPRB.LOG."
}
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'BOARDENC.BRD') -PathType Leaf)) {
    throw "BoardClose probe did not write BOARDENC.BRD. See $stageRoot\BOARDENC.LOG."
}
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'BOARDENC.ZZT') -PathType Leaf)) {
    throw "BoardClose/WorldSave probe did not write BOARDENC.ZZT. See $stageRoot\BOARDENC.LOG."
}

Push-Location $repoRoot
try {
    & '.\tools\serializer-compat\verify.ps1'
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { throw "Serializer compatibility suite failed with exit code $LASTEXITCODE." }
    & node 'tools/serializer-compat/compare-board-rle.js' `
        (Join-Path $stageRoot 'BOARDRLE.BIN') `
        (Join-Path $repoRoot 'build\serializer-compatibility\world_roundtrip.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production board RLE count comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-pascal-boardenc.js' `
        (Join-Path $stageRoot 'BOARDENC.BRD') `
        (Join-Path $repoRoot 'build\serializer-compatibility\world_roundtrip.wasm') `
        (Join-Path $repoRoot 'app\src\main\assets\DEMO.ZZT') `
        (Join-Path $stageRoot 'BOARDENC.ZZT')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal BoardClose/WorldSave byte comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-editor-brd-transfer.js' `
        $stageRoot `
        (Join-Path $repoRoot 'build\serializer-compatibility\world_roundtrip.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/shared-engine truncated .BRD editor-transfer comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-save.js' (Join-Path $stageRoot 'PASOUT.SAV')
    if ($LASTEXITCODE -ne 0) { throw "Pascal/C world-save comparison failed with exit code $LASTEXITCODE." }
    & '.\tools\highscore-compat\verify.ps1' `
        -PascalHiPath (Join-Path $stageRoot 'PASPROBE.HI') `
        -PascalLoadDirectory $stageRoot `
        -PascalInsertionDirectory $stageRoot
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { throw "High-score compatibility suite failed with exit code $LASTEXITCODE." }
    & '.\tools\runtime-compat\verify.ps1'
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { throw "Runtime compatibility suite failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-spinning-gun.js' `
        (Join-Path $stageRoot 'SPINGUN.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\spinning_gun.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Spinning Gun comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-input-buffer.js' `
        (Join-Path $stageRoot 'INPUTBUF.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\player_input.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production queued-arrow comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-player-tick.js' `
        $stageRoot `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production player-tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-game-clock.js' `
        (Join-Path $stageRoot 'SOUNDCLK.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\game_clock.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production system-clock comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-zero-speed-loop.js' `
        (Join-Path $stageRoot 'ZEROSPD.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\tick_exit.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production zero-speed loop comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-game-schedule.js' `
        (Join-Path $stageRoot 'GAMESCHD.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\tick_exit.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production deterministic game schedule comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-editor-flood-fill.js' `
        $stageRoot `
        (Join-Path $repoRoot 'build\runtime-compatibility\editor_flood_fill.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/shared LibZoo editor flood-fill comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-editor-placement.js' `
        (Join-Path $stageRoot 'EDPLACE.OUT') `
        (Join-Path $stageRoot 'EDSTAT.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\editor_flood_fill.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/Android editor placement-adapter comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-element-defs.js' `
        (Join-Path $stageRoot 'ELDEFPRB.OUT') `
        (Join-Path $repoRoot 'third_party\libzoo\src\libzoo\zoo_element_defs.inc')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production element-definition comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-sound-frequency.js' `
        (Join-Path $stageRoot 'SOUNDFRQ.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\sound_tick.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production sound-frequency comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-if.js' `
        (Join-Path $stageRoot 'OOPIF.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP IF comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-counter.js' `
        (Join-Path $stageRoot 'OOPGIV.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP #GIVE/#TAKE comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-char.js' `
        (Join-Path $stageRoot 'OOPCHAR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP #CHAR comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-put.js' `
        (Join-Path $stageRoot 'OOPPUT.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP #PUT comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-change.js' `
        $oopChangeOutput `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP #CHANGE comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-flags.js' `
        $oopFlagsOutput `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP flag-storage comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-send.js' `
        $oopSendOutput `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP #SEND comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-object-tick.js' `
        (Join-Path $stageRoot 'OBJTIK.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production ElementObjectTick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-object-touch.js' `
        (Join-Path $stageRoot 'OBJTOUCH.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production ElementObjectTouch comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-flow.js' `
        (Join-Path $stageRoot 'OOPFLOW.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP flow comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-parser.js' `
        (Join-Path $stageRoot 'OOPPARSE.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP parser comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-any.js' `
        (Join-Path $stageRoot 'OOPANY.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP ANY element-name comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-oop-bind.js' `
        (Join-Path $stageRoot 'BINDSEL.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_label.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production OOP BIND selector comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-slime-tick.js' `
        (Join-Path $stageRoot 'SLIMEPRB.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Slime tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-bomb-tick.js' `
        (Join-Path $stageRoot 'BOMB.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Bomb tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-conveyor-tick.js' `
        (Join-Path $stageRoot 'CONVPRB.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Conveyor tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-star-tick.js' `
        (Join-Path $stageRoot 'STARPRB.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Star tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-tiger-tick.js' `
        (Join-Path $stageRoot 'TIGER.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Tiger tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-centipede-tick.js' `
        (Join-Path $stageRoot 'CENTIP.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Centipede tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-element-draw.js' `
        (Join-Path $stageRoot 'DRAWPRB.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production element draw comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-message-timer.js' `
        (Join-Path $stageRoot 'MSGTIK.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production message timer comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-blink-wall-tick.js' `
        (Join-Path $stageRoot 'BLINKPRB.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Blink Wall tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-scroll-tick.js' `
        (Join-Path $stageRoot 'SCROLLTK.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Scroll tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-scroll-touch.js' `
        (Join-Path $stageRoot 'SCROLLTH.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Scroll touch comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-pusher-duplicator-tick.js' `
        (Join-Path $stageRoot 'PUSHDUP.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Pusher and Duplicator tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-ruffian-tick.js' `
        (Join-Path $stageRoot 'RUFFIAN.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Ruffian tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-bear-tick.js' `
        (Join-Path $stageRoot 'BEARPROB.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Bear tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-shark-tick.js' `
        (Join-Path $stageRoot 'SHARK.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Shark tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-lion-tick.js' `
        (Join-Path $stageRoot 'LION.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\oop_movement.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production Lion tick comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $syntheticReplayRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $syntheticReplayRoot 'INTEGSND.OUT') `
        (Join-Path $syntheticReplayRoot 'INTEGCEL.OUT') `
        (Join-Path $syntheticReplayRoot 'INTEGSAV.SAV') `
        (Join-Path $syntheticReplayRoot 'INTEGLOD.OUT') `
        (Join-Path $syntheticReplayRoot 'INTEG2SV.SAV') `
        'synthetic'
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production integrated gameplay comparison failed with exit code $LASTEXITCODE." }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $demoReplayRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $demoReplayRoot 'INTEGSND.OUT') `
        (Join-Path $demoReplayRoot 'INTEGCEL.OUT') `
        (Join-Path $demoReplayRoot 'INTEGSAV.SAV') `
        (Join-Path $demoReplayRoot 'INTEGLOD.OUT') `
        (Join-Path $demoReplayRoot 'INTEG2SV.SAV') `
        'demo-traversal' `
        (Join-Path $stageRoot 'DEMO.ZZT')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production DEMO traversal comparison failed with exit code $LASTEXITCODE." }
    Write-Host 'Compiling and running the F02 wall/pusher/duplicator gameplay replay: MOTIONIN'
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'MOTIONIN.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'MOTIONIN.EXE') -Force -ErrorAction SilentlyContinue
    Invoke-DosboxSession 'MOTIONINT-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC MOTIONIN.PAS > MOTCOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'MOTIONIN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\MOTIONIN.EXE. See $stageRoot\MOTCOMP.LOG and $stageRoot\MOTIONINT-COMPILE.LOG."
    }
    Invoke-DosboxSession 'MOTIONIN' @('BIN\MOTIONIN.EXE')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F02 integrated replay did not write $fixtureOutput. See $stageRoot\MOTIONIN.LOG."
        }
    }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'walls-pushers-duplicators'
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F02 walls/pushers/duplicators replay failed with exit code $LASTEXITCODE." }
    # The normalized tape is needed only by F03. Inject it for this fixture's
    # compilation alone: adding its static string to INPUT changes the Pascal
    # heap boundary and perturbs unrelated allocator-sensitive oracle probes.
    $stagedInput = $stagedInputWithoutNormalizedTape
    $normalizedInterfaceAnchor = 'function InputProbeGetKeyBuffer: string;'
    $normalizedInterfaceIndex = $stagedInput.IndexOf($normalizedInterfaceAnchor, [StringComparison]::Ordinal)
    if ($normalizedInterfaceIndex -lt 0) {
        throw 'Could not locate the staged Input unit interface for F03 normalized input tape injection.'
    }
    $stagedInput = $stagedInput.Insert(
        $normalizedInterfaceIndex + $normalizedInterfaceAnchor.Length,
        "`r`n`t`tprocedure InputProbeSetNormalizedTape(tape: string);`r`n`t`tfunction InputProbeGetNormalizedTape: string;"
    )
    $normalizedStateAnchor = "`tInputProbeReadKeys: string[50];"
    if (-not $stagedInput.Contains($normalizedStateAnchor)) {
        throw 'Could not locate the staged Input unit state for F03 normalized input tape injection.'
    }
    $stagedInput = $stagedInput.Replace(
        $normalizedStateAnchor,
        $normalizedStateAnchor + "`r`n`tInputProbeNormalizedTape: string[50];"
    )
    $normalizedProbeHelper = @'
procedure InputProbeSetNormalizedTape(tape: string);
begin
	InputProbeNormalizedTape := tape;
end;

function InputProbeGetNormalizedTape: string;
begin
	InputProbeGetNormalizedTape := InputProbeNormalizedTape;
end;

'@
    $stagedInput = $stagedInput.Replace($implementationAnchor, $normalizedProbeHelper + $implementationAnchor)
    $inputUpdateEndBoundary = $stagedInput.LastIndexOf('procedure InputProbeSetKeyBuffer(buffer: string);', [StringComparison]::Ordinal)
    $inputUpdateStart = if ($inputUpdateEndBoundary -ge 0) {
        $stagedInput.LastIndexOf('procedure InputUpdate;', $inputUpdateEndBoundary, [StringComparison]::Ordinal)
    } else { -1 }
    if ($inputUpdateStart -lt 0 -or $inputUpdateEndBoundary -lt 0) {
        throw 'Could not locate InputUpdate for deterministic F03 normalized input tape injection.'
    }
    $inputUpdateEnd = $stagedInput.LastIndexOf('end;', $inputUpdateEndBoundary, $inputUpdateEndBoundary - $inputUpdateStart, [StringComparison]::Ordinal)
    if ($inputUpdateEnd -lt $inputUpdateStart) { throw 'Could not locate the end of InputUpdate for F03 normalized input tape injection.' }
    $normalizedInputProbe = @'
	if Length(InputProbeNormalizedTape) <> 0 then begin
		InputDeltaX := 0;
		InputDeltaY := 0;
		InputShiftPressed := false;
		case UpCase(InputProbeNormalizedTape[1]) of
			'U': InputDeltaY := -1;
			'D': InputDeltaY := 1;
			'L': InputDeltaX := -1;
			'R': InputDeltaX := 1;
			'T': InputKeyPressed := 'T';
			'P': InputKeyPressed := 'P';
			'N': InputKeyPressed := #0;
			'F': begin
				InputDeltaX := 1;
				InputShiftPressed := true;
			end;
			'S': InputShiftPressed := true;
		end;
		if Length(InputProbeNormalizedTape) = 1 then
			InputProbeNormalizedTape := ''
		else
			InputProbeNormalizedTape := Copy(InputProbeNormalizedTape, 2, Length(InputProbeNormalizedTape) - 1);
	end else begin
		InputDeltaX := 0;
		InputDeltaY := 0;
		InputShiftPressed := false;
		InputKeyPressed := #0;
	end;
'@
    $stagedInput = $stagedInput.Insert($inputUpdateEnd, $normalizedInputProbe)
    [IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))
    Write-Host 'Compiling and running the F03 combat/key/door/pickup gameplay replay: COMBATIN'
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'COMBATIN.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'COMBATIN.EXE') -Force -ErrorAction SilentlyContinue
    Invoke-DosboxSession 'COMBAT-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC COMBATIN.PAS > COMBCOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'COMBATIN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\COMBATIN.EXE. See $stageRoot\COMBCOMP.LOG and $stageRoot\COMBAT-COMPILE.LOG."
    }
    [IO.File]::WriteAllText($stagedInputPath, $stagedInputWithoutNormalizedTape, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'COMBATIN' @('BIN\COMBATIN.EXE')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F03 integrated replay did not write $fixtureOutput. See $stageRoot\COMBATIN.LOG."
        }
    }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'combat-keys-pickups'
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F03 combat/key/door/pickup replay failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the F04 stat-order/interacting-script gameplay replay: STATORD'
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'STATORD.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'STATORD.EXE') -Force -ErrorAction SilentlyContinue
    Invoke-DosboxSession 'STATORD-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC STATORD.PAS > STATORCP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'STATORD.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\STATORD.EXE. See $stageRoot\STATORCP.LOG and $stageRoot\STATORD-COMPILE.LOG."
    }
    Invoke-DosboxSession 'STATORD' @('BIN\STATORD.EXE')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F04 integrated replay did not write $fixtureOutput. See $stageRoot\STATORD.LOG."
        }
    }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'object-stat-order'
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F04 object-stat-order replay failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the F05 board-neighbor/passage gameplay replay: BOARDPASSIN'
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'BRDPASS.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'BRDPASS.EXE') -Force -ErrorAction SilentlyContinue
    [IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'BOARDPASSIN-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC BRDPASS.PAS > BRDCOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'BRDPASS.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\BRDPASS.EXE. See $stageRoot\BRDCOMP.LOG and $stageRoot\BOARDPASSIN-COMPILE.LOG."
    }
    [IO.File]::WriteAllText($stagedInputPath, $stagedInputWithoutNormalizedTape, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'BOARDPASSIN' @('BIN\BRDPASS.EXE')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F05 integrated replay did not write $fixtureOutput. See $stageRoot\BRDPASS.LOG."
        }
    }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'board-neighbors-passages'
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F05 board-neighbor/passage replay failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the F06 dark-room/torch/Energizer/timer/death gameplay replay: DARKIN'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'DARKIN.PAS') -Destination (Join-Path $stageRoot 'DARKIN.PAS')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'DARKIN.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'DARKIN.EXE') -Force -ErrorAction SilentlyContinue
    [IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'DARKIN-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC DARKIN.PAS > DARKCOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'DARKIN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\DARKIN.EXE. See $stageRoot\DARKCOMP.LOG and $stageRoot\DARKIN-COMPILE.LOG."
    }
    [IO.File]::WriteAllText($stagedInputPath, $stagedInputWithoutNormalizedTape, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'DARKIN' @('BIN\DARKIN.EXE') 'max'
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F06 integrated replay did not write $fixtureOutput. See $stageRoot\DARKIN.LOG."
        }
    }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'dark-room-torch-energizer-timers-death'
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F06 dark-room/timer/death replay failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the F07 positive-speed/pause/delayed-input replay: PAUSEIN'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'PAUSEIN.PAS') -Destination (Join-Path $stageRoot 'PAUSEIN.PAS')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'PAUSEIN.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'PAUSEIN.EXE') -Force -ErrorAction SilentlyContinue
    [IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'PAUSEIN-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC PAUSEIN.PAS > PAUSECOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'PAUSEIN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\PAUSEIN.EXE. See $stageRoot\PAUSECOMP.LOG and $stageRoot\PAUSEIN-COMPILE.LOG."
    }
    [IO.File]::WriteAllText($stagedInputPath, $stagedInputWithoutNormalizedTape, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'PAUSEIN' @('BIN\PAUSEIN.EXE')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F07 integrated replay did not write $fixtureOutput. See $stageRoot\PAUSEIN.LOG."
        }
    }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'pause-positive-scheduler-delayed-input'
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F07 pause/scheduler/delayed-input replay failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the F08 save/restore/world-reload/board-change replay: SAVEIN'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SAVEIN.PAS') -Destination (Join-Path $stageRoot 'SAVEIN.PAS')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV', 'INTEGWRL.ZZT', 'INTEGWRL.OUT')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'SAVECOMP.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'SAVEIN.EXE') -Force -ErrorAction SilentlyContinue
    [IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'SAVEIN-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC SAVEIN.PAS > SAVECOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'SAVEIN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\SAVEIN.EXE. See $stageRoot\SAVECOMP.LOG and $stageRoot\SAVEIN-COMPILE.LOG."
    }
    [IO.File]::WriteAllText($stagedInputPath, $stagedInputWithoutNormalizedTape, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'SAVEIN' @('BIN\SAVEIN.EXE')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV', 'INTEGWRL.ZZT', 'INTEGWRL.OUT')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F08 integrated replay did not write $fixtureOutput. See $stageRoot\SAVEIN.LOG."
        }
    }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'save-restore-world-reload-board-change' `
        (Join-Path $stageRoot 'INTEGWRL.ZZT') `
        (Join-Path $stageRoot 'INTEGWRL.OUT')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F08 save/restore/world-reload/board-change replay failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the F09 editor/.BRD round-trip/gameplay replay: EDITORIN'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'EDITORIN.PAS') -Destination (Join-Path $stageRoot 'EDITORIN.PAS')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV', 'ABCGH.BRD')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'EDITORIN.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'EDITORIN.EXE') -Force -ErrorAction SilentlyContinue

    $stagedInput = $stagedInputWithoutNormalizedTape
    $editorTapeInterfaceAnchor = 'function InputProbeGetKeyBuffer: string;'
    $editorTapeInterfaceIndex = $stagedInput.IndexOf($editorTapeInterfaceAnchor, [StringComparison]::Ordinal)
    if ($editorTapeInterfaceIndex -lt 0) {
        throw 'Could not locate the staged Input unit interface for the F09 editor key tape.'
    }
    $stagedInput = $stagedInput.Insert(
        $editorTapeInterfaceIndex + $editorTapeInterfaceAnchor.Length,
        "`r`n`t`tprocedure InputProbeSetEditorTape(tape: string);`r`n`t`tfunction InputProbeGetEditorTape: string;"
    )
    $editorTapeStateAnchor = "`tInputProbeReadKeys: string[50];"
    if (-not $stagedInput.Contains($editorTapeStateAnchor)) {
        throw 'Could not locate the staged Input unit state for the F09 editor key tape.'
    }
    $stagedInput = $stagedInput.Replace($editorTapeStateAnchor,
        $editorTapeStateAnchor + "`r`n`tInputProbeEditorTape: string[50];`r`n`tInputProbeEditorTapeEnabled: boolean;")
    $editorTapeHelpers = @'
procedure InputProbeSetEditorTape(tape: string);
begin
	InputProbeEditorTape := tape;
	InputProbeEditorTapeEnabled := Length(tape) <> 0;
end;

function InputProbeGetEditorTape: string;
begin
	InputProbeGetEditorTape := InputProbeEditorTape;
end;

'@
    $stagedInput = $stagedInput.Replace($implementationAnchor, $editorTapeHelpers + $implementationAnchor)
    $inputUpdateStart = $stagedInput.LastIndexOf('procedure InputUpdate;', [StringComparison]::Ordinal)
    $inputUpdateNextRoutine = $stagedInput.IndexOf('procedure InputProbeSetKeyBuffer(buffer: string);', $inputUpdateStart, [StringComparison]::Ordinal)
    if ($inputUpdateStart -lt 0 -or $inputUpdateNextRoutine -lt 0) {
        throw 'Could not locate staged InputUpdate for the F09 editor key tape.'
    }
    $inputUpdateSource = $stagedInput.Substring($inputUpdateStart, $inputUpdateNextRoutine - $inputUpdateStart)
    $inputUpdateEndOffset = $inputUpdateSource.LastIndexOf('end;', [StringComparison]::Ordinal)
    if ($inputUpdateEndOffset -lt 0) { throw 'Could not locate the end of staged InputUpdate for F09.' }
    $editorTapeHook = @'
		if InputProbeEditorTapeEnabled then begin
			InputDeltaX := 0;
			InputDeltaY := 0;
			InputShiftPressed := false;
			if Length(InputProbeEditorTape) <> 0 then begin
				InputKeyPressed := InputProbeEditorTape[1];
				if Length(InputProbeEditorTape) = 1 then
					InputProbeEditorTape := ''
				else
					InputProbeEditorTape := Copy(InputProbeEditorTape, 2, Length(InputProbeEditorTape) - 1);
				case InputKeyPressed of
					KEY_UP: InputDeltaY := -1;
					KEY_DOWN: InputDeltaY := 1;
					KEY_LEFT: InputDeltaX := -1;
					KEY_RIGHT: InputDeltaX := 1;
				end;
			end else
				InputKeyPressed := #0;
		end;
'@
    $stagedInput = $stagedInput.Insert($inputUpdateStart + $inputUpdateEndOffset, $editorTapeHook)
    [IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'EDITORIN-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC EDITORIN.PAS > EDITORCOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'EDITORIN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\EDITORIN.EXE. See $stageRoot\EDITORCOMP.LOG and $stageRoot\EDITORIN-COMPILE.LOG."
    }
    Invoke-DosboxSession 'EDITORIN' @('BIN\EDITORIN.EXE')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV', 'ABCGH.BRD')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F09 editor/.BRD replay did not write $fixtureOutput. See $stageRoot\EDITORIN.LOG."
        }
    }
    $stagedInput = $stagedInputWithoutNormalizedTape
    [IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'editor-brd-roundtrip-play' `
        (Join-Path $stageRoot 'ABCGH.BRD')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F09 editor/.BRD round-trip/gameplay replay failed with exit code $LASTEXITCODE." }

    & node 'tools/pascal-compat/check-integrated-diagnostics.js' `
        'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $syntheticReplayRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $syntheticReplayRoot 'INTEGSND.OUT') `
        (Join-Path $syntheticReplayRoot 'INTEGCEL.OUT') `
        (Join-Path $syntheticReplayRoot 'INTEGSAV.SAV') `
        (Join-Path $syntheticReplayRoot 'INTEGLOD.OUT') `
        (Join-Path $syntheticReplayRoot 'INTEG2SV.SAV') `
        (Join-Path $stageRoot 'INTEGPR_DIAG_MUTATION.OUT')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production integrated diagnostic negative control failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the F10 malformed-load/high-score/gameplay replay: F10IN'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'F10IN.PAS') -Destination (Join-Path $stageRoot 'F10IN.PAS')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV', 'F10BAD.OUT', 'F10ROOM.HI')) {
        Remove-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -LiteralPath (Join-Path $stageRoot 'F10IN.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'F10IN.EXE') -Force -ErrorAction SilentlyContinue
    $malformedWorldBytes = New-Object byte[] 512
    $malformedWorldBytes[0] = 0xFE
    $malformedWorldBytes[1] = 0xFF
    [IO.File]::WriteAllBytes((Join-Path $stageRoot 'F10BAD.ZZT'), $malformedWorldBytes)
    $stagedInput = $stagedInputWithoutNormalizedTape
    [IO.File]::WriteAllText($stagedInputPath, $stagedInput, [Text.UTF8Encoding]::new($false))
    Invoke-DosboxSession 'F10IN-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC F10IN.PAS > F10COMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'F10IN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\F10IN.EXE. See $stageRoot\F10COMP.LOG and $stageRoot\F10IN-COMPILE.LOG."
    }
    Invoke-DosboxSession 'F10IN-RUN' @('BIN\F10IN.EXE')
    foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV', 'F10BAD.OUT', 'F10ROOM.HI')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
            throw "Pascal F10 malformed-load/high-score replay did not write $fixtureOutput. See $stageRoot\F10IN.LOG."
        }
    }
    if ((Get-Item -LiteralPath (Join-Path $stageRoot 'F10ROOM.HI')).Length -ne 1590) {
        throw 'Pascal F10 HighScoresAdd did not write the exact 1,590-byte HI record.'
    }
    & node 'tools/pascal-compat/compare-integrated-gameplay.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        (Join-Path $stageRoot 'INTEGSND.OUT') `
        (Join-Path $stageRoot 'INTEGCEL.OUT') `
        (Join-Path $stageRoot 'INTEGSAV.SAV') `
        (Join-Path $stageRoot 'INTEGLOD.OUT') `
        (Join-Path $stageRoot 'INTEG2SV.SAV') `
        'malformed-load-high-score-insertion' `
        (Join-Path $stageRoot 'F10BAD.ZZT') `
        (Join-Path $stageRoot 'F10BAD.OUT') `
        (Join-Path $stageRoot 'F10ROOM.HI')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production F10 malformed-load/high-score/gameplay replay failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the direct GameTitleLoop lifecycle replay: TITLEIN'
    Remove-Item -LiteralPath (Join-Path $stageRoot 'INTEGPR.OUT') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageRoot 'TITLEIN.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'TITLEIN.EXE') -Force -ErrorAction SilentlyContinue
    Invoke-DosboxSession 'TITLEIN-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC TITLEIN.PAS > TITLECOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'TITLEIN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\TITLEIN.EXE. See $stageRoot\TITLECOMP.LOG and $stageRoot\TITLEIN-COMPILE.LOG."
    }
    Invoke-DosboxSession 'TITLEIN' @('BIN\TITLEIN.EXE')
    if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'INTEGPR.OUT') -PathType Leaf)) {
        throw "Pascal GameTitleLoop lifecycle replay did not write INTEGPR.OUT. See $stageRoot\TITLEIN.LOG."
    }
    & node 'tools/pascal-compat/compare-title-lifecycle.js' `
        (Join-Path $stageRoot 'INTEGPR.OUT') `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production title/menu lifecycle replay failed with exit code $LASTEXITCODE." }

    $titleRestoreRoot = Join-Path $stageRoot 'TITLER'
    New-Item -ItemType Directory -Force -Path $titleRestoreRoot | Out-Null
    Copy-Item -LiteralPath (Join-Path $stageRoot 'DEMO.ZZT') -Destination (Join-Path $titleRestoreRoot 'DEMO.ZZT') -Force
    Write-Host 'Running the direct GameTitleLoop .SAV restore lifecycle replay: TITLER'
    Invoke-DosboxSession 'TITLEIN-SAVE' @('CD \TITLER', '..\BIN\TITLEIN.EXE G')
    $titleRestoreSave = Join-Path $titleRestoreRoot 'RESTORE.SAV'
    if (-not (Test-Path -LiteralPath $titleRestoreSave -PathType Leaf)) {
        throw "Pascal restore fixture generator did not create RESTORE.SAV. See $stageRoot\TITLEIN-SAVE.LOG."
    }
    Invoke-DosboxSession 'TITLEIN-RESTORE' @('CD \TITLER', '..\BIN\TITLEIN.EXE R')
    $titleRestoreOutput = Join-Path $titleRestoreRoot 'INTEGPR.OUT'
    if (-not (Test-Path -LiteralPath $titleRestoreOutput -PathType Leaf)) {
        throw "Pascal GameTitleLoop .SAV restore replay did not write INTEGPR.OUT. See $stageRoot\TITLEIN-RESTORE.LOG."
    }
    if (-not (Test-Path -LiteralPath $titleRestoreSave -PathType Leaf)) {
        throw "Pascal GameTitleLoop .SAV restore replay did not create RESTORE.SAV. See $stageRoot\TITLEIN-RESTORE.LOG."
    }
    & node 'tools/pascal-compat/compare-title-lifecycle.js' `
        $titleRestoreOutput `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        $titleRestoreSave
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production .SAV restore lifecycle replay failed with exit code $LASTEXITCODE." }

    # DOSBox's mounted DOS filesystem uses 8.3 directory names in these runs.
    $titleRetryRoot = Join-Path $stageRoot 'TITLRTY'
    New-Item -ItemType Directory -Force -Path $titleRetryRoot | Out-Null
    Copy-Item -LiteralPath (Join-Path $stageRoot 'DEMO.ZZT') -Destination (Join-Path $titleRetryRoot 'DEMO.ZZT') -Force
    $titleRetrySave = Join-Path $titleRetryRoot 'RESTORE.SAV'
    Copy-Item -LiteralPath $titleRestoreSave -Destination $titleRetrySave -Force
    Write-Host 'Running the direct GameTitleLoop restore-cancel-and-retry replay: TITLRETRY'
    Invoke-DosboxSession 'TITLEIN-RESTORE-CANCEL-RETRY' @('CD \TITLRTY', '..\BIN\TITLEIN.EXE C')
    $titleRetryOutput = Join-Path $titleRetryRoot 'INTEGPR.OUT'
    if (-not (Test-Path -LiteralPath $titleRetryOutput -PathType Leaf)) {
        throw "Pascal GameTitleLoop restore cancel/retry replay did not write INTEGPR.OUT. See $stageRoot\TITLEIN-RESTORE-CANCEL-RETRY.LOG."
    }
    & node 'tools/pascal-compat/compare-title-lifecycle.js' `
        $titleRetryOutput `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        '--restore-retry' `
        $titleRetrySave
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production title restore cancel/retry differential failed with exit code $LASTEXITCODE." }

    $titleFailureRoot = Join-Path $stageRoot 'TITLFAIL'
    New-Item -ItemType Directory -Force -Path $titleFailureRoot | Out-Null
    Write-Host 'Running the direct GameTitleLoop unsupported-version .SAV refusal replay: TITLEFAIL'
    Invoke-DosboxSession 'TITLEIN-BAD-RESTORE' @('CD \TITLFAIL', '..\BIN\TITLEIN.EXE B')
    $titleFailureOutput = Join-Path $titleFailureRoot 'TITLFAIL.OUT'
    $titleFailureSave = Join-Path $titleFailureRoot 'RESTORE.SAV'
    if (-not (Test-Path -LiteralPath $titleFailureOutput -PathType Leaf)) {
        throw "Pascal GameTitleLoop malformed .SAV replay did not write TITLFAIL.OUT. See $stageRoot\TITLEIN-BAD-RESTORE.LOG."
    }
    if (-not (Test-Path -LiteralPath $titleFailureSave -PathType Leaf)) {
        throw "Pascal GameTitleLoop malformed .SAV replay did not create RESTORE.SAV. See $stageRoot\TITLEIN-BAD-RESTORE.LOG."
    }
    & node 'tools/pascal-compat/compare-title-lifecycle.js' `
        $titleFailureOutput `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        '--restore-failure' `
        $titleFailureSave
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production malformed .SAV restore differential failed with exit code $LASTEXITCODE." }

    Write-Host 'Compiling and running the direct Pascal player-abort decision replay: ABORTIN'
    Remove-Item -LiteralPath (Join-Path $stageRoot 'ABORT.OUT') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageRoot 'TQUIT.OUT') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageRoot 'ABORTIN.LOG') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $stageBin 'ABORTIN.EXE') -Force -ErrorAction SilentlyContinue
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'ABORTIN.PAS') -Destination (Join-Path $stageRoot 'ABORTIN.PAS') -Force
    Invoke-DosboxSession 'ABORTIN-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC ABORTIN.PAS > ABORTCOMP.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'ABORTIN.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\ABORTIN.EXE. See $stageRoot\ABORTCOMP.LOG and $stageRoot\ABORTIN-COMPILE.LOG."
    }
    Invoke-DosboxSession 'ABORTIN-RUN' @('BIN\ABORTIN.EXE')
    $abortOutput = Join-Path $stageRoot 'ABORT.OUT'
    $titleQuitOutput = Join-Path $stageRoot 'TQUIT.OUT'
    if (-not (Test-Path -LiteralPath $abortOutput -PathType Leaf) -or
        -not (Test-Path -LiteralPath $titleQuitOutput -PathType Leaf)) {
        throw "Pascal end-play/title-quit replay did not write both decision captures. See $stageRoot\ABORTIN-RUN.LOG."
    }
    & node 'tools/pascal-compat/compare-player-abort.js' `
        $abortOutput `
        (Join-Path $repoRoot 'build\runtime-compatibility\integrated_gameplay.wasm') `
        $titleQuitOutput
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production player-abort decision differential failed with exit code $LASTEXITCODE." }

    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'MISSLOAD.PAS') -Destination (Join-Path $stageRoot 'MISSLOAD.PAS') -Force
    Write-Host 'Compiling and running the direct Pascal missing-world reload capture: MISSLOAD'
    Invoke-DosboxSession 'MISSLOAD-COMPILE' @(
        'TPC.EXE /B /EBIN /ISRC /USRC MISSLOAD.PAS > MISSLOAD.LOG'
    )
    if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'MISSLOAD.EXE') -PathType Leaf)) {
        throw "Turbo Pascal did not produce BIN\MISSLOAD.EXE. See $stageRoot\MISSLOAD.LOG and $stageRoot\MISSLOAD-COMPILE.LOG."
    }
    Invoke-DosboxSession 'MISSLOAD-RUN' @('BIN\MISSLOAD.EXE')
    $missingWorldOutput = Join-Path $stageRoot 'MISSLOAD.OUT'
    if (-not (Test-Path -LiteralPath $missingWorldOutput -PathType Leaf)) {
        throw "Pascal missing-world reload probe did not write MISSLOAD.OUT. See $stageRoot\MISSLOAD-RUN.LOG."
    }
    & node 'tools/pascal-compat/compare-missing-world-load.js' `
        $missingWorldOutput `
        (Join-Path $repoRoot 'build\runtime-compatibility\world_reload.wasm')
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production missing-world reload differential failed with exit code $LASTEXITCODE." }
}
finally {
    Pop-Location
}

$directProbeCount = $probeFiles.Count + 84 # Includes the previous isolated compile/run probes plus MISSLOAD-COMPILE and MISSLOAD-RUN; short-header BoardOpen remains excluded because its freed-buffer path is unstable under DOSBox-X.
Write-Host "Direct Pascal 5.5 probe sessions passed: $directProbeCount ($($probeFiles.Count) standard probes plus the existing standalone runs and MISSLOAD-COMPILE/MISSLOAD-RUN)."
Write-Host "Pascal oracle and production compatibility suites passed. Staging output: $stageRoot"
