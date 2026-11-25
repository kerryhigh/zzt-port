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
	GameLoopProbeEnabled, GameLoopProbeSeedPauseBlink: boolean;
	GameLoopProbeLastPaused, GameLoopProbeCycleCompleted: boolean;
	GameLoopProbeCaptureNextBoardChange: boolean;
	GameLoopProbeCellCaptureEnabled, GameLoopProbeCellWriteOverflow: boolean;
	GameLoopProbeCellWriteCount: longint;
	GameLoopProbeCellWriteEvents: PGameLoopProbeCellWriteEventArray;
	procedure GameLoopProbeCapture(checkpointKind: char; checkpointId: integer);

"@
$stagedGame = $stagedGame.Insert($gameImplementationIndex, $gameProbeDeclarations)
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
$startTickAnchor = 'CurrentTick := Random(100);'
$startTickIndex = $gamePlayLoop.IndexOf($startTickAnchor, [StringComparison]::Ordinal)
if ($startTickIndex -lt 0) { throw 'Could not locate GamePlayLoop tick initialization for the zero-speed probe.' }
$startStatAnchor = 'CurrentStatTicked := Board.StatCount + 1;'
$startStatIndex = $gamePlayLoop.IndexOf($startStatAnchor, $startTickIndex + $startTickAnchor.Length, [StringComparison]::Ordinal)
if ($startStatIndex -lt 0) { throw 'Could not locate GamePlayLoop stat initialization for the integrated snapshot.' }
$startStatEnd = $startStatIndex + $startStatAnchor.Length
# GAME.PAS leaves pauseBlink uninitialized. Only F05 opts into a controlled staging seed, matching LibZoo's zero-initialized pause blink state without changing earlier replay traces.
$gamePlayLoop = $gamePlayLoop.Substring(0, $startStatEnd) + "`r`n`t`tGameLoopProbeStartTick := CurrentTick;`r`n`t`tGameLoopProbeCycles := 0;`r`n`t`tGameLoopProbeTransitionCount := 0;`r`n`t`tGameLoopProbeCycleCompleted := false;`r`n`t`tif GameLoopProbeEnabled then begin`r`n`t`t`tif GameLoopProbeSeedPauseBlink then pauseBlink := false;`r`n`t`t`tGameLoopProbeCapture('S', 0);`r`n`t`t`tGameLoopProbeLastBoard := World.Info.CurrentBoard;`r`n`t`t`tGameLoopProbeLastState := GameStateElement;`r`n`t`t`tGameLoopProbeLastPaused := GamePaused;`r`n`t`t`tGameLoopProbeCellCaptureEnabled := true;`r`n`t`tend;" + $gamePlayLoop.Substring($startStatEnd)
$cycleAdvancePattern = '(?s)(CurrentStatTicked := 0;\s*InputUpdate;)'
if ([regex]::Matches($gamePlayLoop, $cycleAdvancePattern).Count -ne 1) {
    throw 'Could not uniquely locate the completed GamePlayLoop cycle for zero-speed instrumentation.'
}
$cycleProbe = @'
GameLoopProbeCycleCompleted := true;
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
[IO.File]::WriteAllText($stagedGamePath, $stagedGame, [Text.UTF8Encoding]::new($false))

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
    if ($probe.Program -in @('NEGLEN', 'NEGBRD', 'STARPRB', 'CENTIPR', 'PUSHDUP', 'BINDPROB', 'SLIMEPRB', 'DRAWPRB', 'MSGTIK', 'TORCHPR')) {
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
        # runtime session produces both expected rows. All are clock-independent.
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
    Invoke-DosboxSession $caseName @(('BIN\OBJTIK.EXE {0}' -f $caseId)) 'max'
}
$objectTickRows = [Collections.Generic.List[string]]::new()
for ($caseId = 1; $caseId -le 4; $caseId++) {
    $caseOutputPath = Join-Path $stageRoot ('OBJTIK{0:D2}.OUT' -f $caseId)
    if (-not (Test-Path -LiteralPath $caseOutputPath -PathType Leaf)) {
        throw "Turbo Pascal ElementObjectTick case $caseId did not create its output file."
    }
    $caseOutput = [IO.File]::ReadAllLines($caseOutputPath)
    if ($caseOutput.Count -ne 1 -or
        $caseOutput[0] -notmatch ('^{0} -?\d+ -?\d+ -?\d+ -?\d+ -?\d+ -?\d+ -?\d+ -?\d+$' -f $caseId)) {
        throw "Unexpected Turbo Pascal ElementObjectTick case $caseId output: $([string]::Join('; ', $caseOutput))"
    }
    $objectTickRows.Add($caseOutput[0])
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'OBJTIK.OUT'), $objectTickRows, [Text.Encoding]::ASCII)

Write-Host 'Compiling and running direct Turbo Pascal Object TOUCH event cases: OBJTOUCHPRB'
Invoke-DosboxSession 'OBJTOUCHPRB' @(
    'TPC.EXE /B /EBIN /UBIN OBJTOUCH.PAS > OBJTOUCH.LOG',
    'BIN\OBJTOUCH.EXE'
)
$objectTouchOutputPath = Join-Path $stageRoot 'OBJTOUCH.OUT'
if (-not (Test-Path -LiteralPath $objectTouchOutputPath -PathType Leaf)) {
    throw 'Turbo Pascal Object TOUCH probe did not create OBJTOUCH.OUT.'
}
$objectTouchRows = [IO.File]::ReadAllLines($objectTouchOutputPath)
if ($objectTouchRows.Count -ne 4) {
    throw "Turbo Pascal Object TOUCH probe should contain four cases; found $($objectTouchRows.Count)."
}
for ($caseId = 1; $caseId -le 4; $caseId++) {
    if ($objectTouchRows[$caseId - 1] -notmatch ('^{0}(?:\s+-?\d+){{7}}$' -f $caseId)) {
        throw "Unexpected Turbo Pascal Object TOUCH case $caseId output: $($objectTouchRows[$caseId - 1])"
    }
}

Write-Host 'Compiling and running direct Turbo Pascal OOP flow-control and movement cases: OOPFLOWPRB'
Invoke-DosboxSession 'OOPFLOWPRB' @(
    'TPC.EXE /B /EBIN /UBIN OOPFLOW.PAS > OOPFLOW.LOG',
    'BIN\OOPFLOW.EXE'
)
$oopFlowOutputPath = Join-Path $stageRoot 'OOPFLOW.OUT'
if (-not (Test-Path -LiteralPath $oopFlowOutputPath -PathType Leaf)) {
    throw "Turbo Pascal OOP flow probe did not create OOPFLOW.OUT. See $stageRoot\OOPFLOW.LOG."
}
$oopFlowRows = [IO.File]::ReadAllLines($oopFlowOutputPath)
if ($oopFlowRows.Count -ne 11) {
    throw "Turbo Pascal OOP flow probe should contain 11 cases; found $($oopFlowRows.Count)."
}
for ($caseId = 1; $caseId -le 11; $caseId++) {
    if ($oopFlowRows[$caseId - 1] -notmatch ('^{0}(?:\s+-?\d+){{10}}$' -f $caseId)) {
        throw "Unexpected Turbo Pascal OOP flow case $caseId output: $($oopFlowRows[$caseId - 1])"
    }
}

Write-Host 'Compiling and running direct Turbo Pascal OOP parser and line-reader cases: OOPPARSEPRB'
# The 2,837 direct parser/line-reader cases are clock-independent. Compile and
# execute separately so the compiler's DOS allocation history cannot leave the
# oracle process with an empty partial output file; use host speed for runtime.
Invoke-DosboxSession 'OOPPARSE-COMPILE' @('TPC.EXE /B /EBIN /UBIN OOPPARSE.PAS > OOPPARSE.LOG') 'max'
if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'OOPPARSE.EXE') -PathType Leaf)) {
    throw "Turbo Pascal did not produce BIN\OOPPARSE.EXE. See $stageRoot\OOPPARSE.LOG and $stageRoot\OOPPARSE-COMPILE.LOG."
}
Invoke-DosboxSession 'OOPPARSEPRB' @('BIN\OOPPARSE.EXE') 'max'
$oopParserOutputPath = Join-Path $stageRoot 'OOPPARSE.OUT'
if (-not (Test-Path -LiteralPath $oopParserOutputPath -PathType Leaf)) {
    throw "Turbo Pascal OOP parser probe did not create OOPPARSE.OUT. See $stageRoot\OOPPARSE.LOG."
}
$oopParserRows = [IO.File]::ReadAllLines($oopParserOutputPath)
$oopWordCaseCount = 775
$oopValueCaseCount = 524
$oopLineCaseCount = 1538
if ($oopParserRows.Count -ne ($oopWordCaseCount + $oopValueCaseCount + $oopLineCaseCount)) {
    throw "Turbo Pascal OOP parser probe should contain $($oopWordCaseCount + $oopValueCaseCount + $oopLineCaseCount) rows; found $($oopParserRows.Count)."
}
for ($caseId = 1; $caseId -le $oopWordCaseCount; $caseId++) {
    if ($oopParserRows[$caseId - 1] -notmatch ('^W {0} -?\d+ \d+(?: \d+)*$' -f $caseId)) {
        throw "Unexpected Turbo Pascal OOP word-parser output for case ${caseId}: $($oopParserRows[$caseId - 1])"
    }
}
for ($caseId = 1; $caseId -le $oopValueCaseCount; $caseId++) {
    $row = $oopParserRows[$oopWordCaseCount + $caseId - 1]
    if ($row -notmatch ('^V {0} -?\d+ -?\d+$' -f $caseId)) {
        throw "Unexpected Turbo Pascal OOP value-parser output for case ${caseId}: $row"
    }
}
for ($caseId = 1; $caseId -le $oopLineCaseCount; $caseId++) {
    $row = $oopParserRows[$oopWordCaseCount + $oopValueCaseCount + $caseId - 1]
    if ($row -notmatch ('^L {0} \d+ -?\d+(?: \d+)*$' -f $caseId)) {
        throw "Unexpected Turbo Pascal OOP line-reader output for case ${caseId}: $row"
    }
}

Write-Host 'Compiling and running isolated direct Turbo Pascal OOP ANY element-name cases: OOPANYPRB'
$oopAnyCommands = [Collections.Generic.List[string]]::new()
$oopAnyCommands.Add('TPC.EXE /B /EBIN /UBIN OOPANYP.PAS > OOPANYP.LOG')
for ($elementId = 0; $elementId -le 53; $elementId++) {
    $oopAnyCommands.Add(('BIN\OOPANYP.EXE {0}' -f $elementId))
}
# All 54 named-element checks are clock-independent; keep the batch below the
# bounded DOSBox-X session window without changing the tested Pascal routines.
Invoke-DosboxSession 'OOPANYPRB' $oopAnyCommands.ToArray() 'max'
$oopAnyRows = [Collections.Generic.List[string]]::new()
$oopAnyNamedElementCount = 0
for ($elementId = 0; $elementId -le 53; $elementId++) {
    $caseOutputPath = Join-Path $stageRoot ('OOPANY{0:D2}.OUT' -f $elementId)
    if (-not (Test-Path -LiteralPath $caseOutputPath -PathType Leaf)) {
        throw "Turbo Pascal OOP ANY element case $elementId did not create its output file."
    }
    $caseOutput = [IO.File]::ReadAllLines($caseOutputPath)
    if ($caseOutput.Count -ne 2 -or $caseOutput[0] -cne 'started') {
        throw "Unexpected Turbo Pascal OOP ANY element case $elementId output: $([string]::Join('; ', $caseOutput))"
    }
    $fields = @($caseOutput[1] -split '\s+')
    if ($fields.Count -eq 2 -and $fields[0] -eq "$elementId" -and $fields[1] -ceq 'SKIP') {
        $oopAnyRows.Add("$elementId SKIP")
        continue
    }
    if ($fields.Count -ne 4 -or $fields[0] -ne "$elementId" -or
        $fields[1] -notmatch '^[A-Z0-9]{1,50}$' -or $fields[2] -cne '1' -or $fields[3] -cne '0') {
        throw "Turbo Pascal OOP ANY element $elementId did not produce a true match and false miss: $([string]::Join('; ', $caseOutput))"
    }
    $oopAnyNamedElementCount++
    $oopAnyRows.Add([string]::Join(' ', $fields))
}
if ($oopAnyNamedElementCount -ne 42) {
    throw "Expected 42 named Pascal tile elements in IDs 0 through 53, found $oopAnyNamedElementCount."
}
[IO.File]::WriteAllLines((Join-Path $stageRoot 'OOPANY.OUT'), $oopAnyRows, [Text.Encoding]::ASCII)
Write-Host "Turbo Pascal OOP ANY parsing: all $oopAnyNamedElementCount named tile elements match when present and miss when absent."

# Compile a non-interrupt wrapper around the original sound timer body only
# after the ordinary Pascal probes have run. This lets the SoundTick probe
# step the actual handler deterministically without installing its DOS vector.
$stagedSoundPath = Join-Path $stageSource 'SOUNDS.PAS'
$stagedSound = [IO.File]::ReadAllText($stagedSoundPath)
$newline = if ($stagedSound.Contains("`r`n")) { "`r`n" } else { "`n" }
$soundInterfaceAnchor = "`tprocedure SoundQueue(priority: integer; pattern: string);"
$soundTimeCallAnchor = 'GetTime(hour, minute, sec, hSec);'
$soundHandlerAnchor = 'procedure SoundTimerHandler;' + $newline + "`tinterrupt;"
$soundVectorAnchor = "`tSetIntVec(`$1C, SoundNewVector);"
if ((-not $stagedSound.Contains($soundInterfaceAnchor)) -or
    (-not $stagedSound.Contains($soundHandlerAnchor)) -or
    (-not $stagedSound.Contains($soundVectorAnchor))) {
    throw 'Could not locate the Sounds unit probe insertion points.'
}
$stagedSound = $stagedSound.Replace(
    $soundInterfaceAnchor,
    $soundInterfaceAnchor + $newline + "`tprocedure SoundProbeTick;" + $newline +
        "`tprocedure SoundProbeSetTime(hour, minute, sec, hSec: word);" + $newline +
        "`tprocedure SoundProbeSetSchedule(startHsecs, stallAfterPoll, stallExtraPits: integer);" + $newline +
        "`tprocedure SoundProbeQueueCapture(enabled: boolean);" + $newline +
        "`tprocedure SoundProbeWriteQueueEvents(filename: string);" + $newline +
        "`tfunction SoundProbeGetScheduleCycleCount: integer;" + $newline +
        "`tfunction SoundProbeGetScheduleCycleHsec(index: integer): integer;"
)
$soundProbeVarAnchor = 'uses Crt, Dos;'
if (-not $stagedSound.Contains($soundProbeVarAnchor)) {
    throw 'Could not locate the Sounds implementation declaration for clock probes.'
}
$soundProbeTimeVars = @'
uses Crt, Dos;

const
    SoundProbeQueueEventCapacity = 64;

type
    TSoundProbeQueueEvent = record
        RequestPriority, RequestLength: integer;
        RequestBytes: array[1..255] of byte;
        ResultPriority, ResultPosition, ResultLength, ResultPlaying: integer;
        ResultBytes: array[1..255] of byte;
    end;
    TSoundProbeQueueEventArray = array[1..SoundProbeQueueEventCapacity] of TSoundProbeQueueEvent;
    PSoundProbeQueueEventArray = ^TSoundProbeQueueEventArray;

var
    SoundProbeTimeEnabled: boolean;
    SoundProbeScheduleEnabled: boolean;
    SoundProbeQueueCaptureEnabled: boolean;
    SoundProbeQueueEventCount: integer;
    SoundProbeQueueEventOverflow: boolean;
    SoundProbeQueueEvents: PSoundProbeQueueEventArray;
    SoundProbeHour, SoundProbeMinute, SoundProbeSecond, SoundProbeHsec: word;
    SoundProbeScheduleStartHsecs, SoundProbeScheduleCurrentHsecs,
        SoundProbeScheduleStallAfterPoll,
        SoundProbeScheduleStallExtraPits, SoundProbeSchedulePollCount,
        SoundProbeScheduleCycleCount: integer;
    SoundProbeScheduleElapsedNanos: longint;
    SoundProbeScheduleCycleHsecs: array[0..4] of integer;

procedure SoundProbeQueueCapture(enabled: boolean);
begin
    if enabled then begin
        if SoundProbeQueueEvents = nil then
            GetMem(SoundProbeQueueEvents, SizeOf(TSoundProbeQueueEventArray));
        SoundProbeQueueEventCount := 0;
        SoundProbeQueueEventOverflow := false;
    end;
    SoundProbeQueueCaptureEnabled := enabled;
end;

procedure SoundProbeCaptureQueue(priority: integer; pattern: string);
var
    i, eventIndex: integer;
begin
    if SoundProbeQueueCaptureEnabled then begin
        if (SoundProbeQueueEventCount < SoundProbeQueueEventCapacity) and
            (Length(pattern) <= 255) and (Length(SoundBuffer) <= 255) then begin
            SoundProbeQueueEventCount := SoundProbeQueueEventCount + 1;
            eventIndex := SoundProbeQueueEventCount;
            with SoundProbeQueueEvents^[eventIndex] do begin
                RequestPriority := priority;
                RequestLength := Length(pattern);
                for i := 1 to RequestLength do
                    RequestBytes[i] := Ord(pattern[i]);
                ResultPriority := SoundCurrentPriority;
                ResultPosition := SoundBufferPos;
                ResultLength := Length(SoundBuffer);
                ResultPlaying := Ord(SoundIsPlaying);
                for i := 1 to ResultLength do
                    ResultBytes[i] := Ord(SoundBuffer[i]);
            end;
        end else
            SoundProbeQueueEventOverflow := true;
    end;
end;

procedure SoundProbeWriteQueueEvents(filename: string);
var
    output: text;
    eventIndex, byteIndex: integer;
begin
    Assign(output, filename);
    Rewrite(output);
    WriteLn(output, SoundProbeQueueEventCount, ' ', Ord(SoundProbeQueueEventOverflow));
    for eventIndex := 1 to SoundProbeQueueEventCount do begin
        with SoundProbeQueueEvents^[eventIndex] do begin
            Write(output, RequestPriority, ' ', RequestLength, ' ');
            for byteIndex := 1 to RequestLength do Write(output, RequestBytes[byteIndex], ' ');
            Write(output, ResultPriority, ' ', ResultPosition, ' ', ResultLength, ' ', ResultPlaying, ' ');
            for byteIndex := 1 to ResultLength do Write(output, ResultBytes[byteIndex], ' ');
            WriteLn(output);
        end;
    end;
    Close(output);
    if SoundProbeQueueEvents <> nil then begin
        FreeMem(SoundProbeQueueEvents, SizeOf(TSoundProbeQueueEventArray));
        SoundProbeQueueEvents := nil;
    end;
end;

procedure SoundProbeSetTime(hour, minute, sec, hSec: word);
begin
    SoundProbeTimeEnabled := true;
    SoundProbeScheduleEnabled := false;
    SoundProbeHour := hour;
    SoundProbeMinute := minute;
    SoundProbeSecond := sec;
    SoundProbeHsec := hSec;
end;

procedure SoundProbeSetSchedule(startHsecs, stallAfterPoll, stallExtraPits: integer);
var
    i, hSecs: integer;
begin
    SoundProbeTimeEnabled := true;
    SoundProbeScheduleEnabled := true;
    SoundProbeScheduleStartHsecs := startHsecs;
    SoundProbeScheduleStallAfterPoll := stallAfterPoll;
    SoundProbeScheduleStallExtraPits := stallExtraPits;
    SoundProbeSchedulePollCount := 0;
    SoundProbeScheduleCycleCount := 0;
    SoundProbeScheduleElapsedNanos := 0;
    for i := 0 to 4 do SoundProbeScheduleCycleHsecs[i] := 0;
    hSecs := startHsecs mod 6000;
    if hSecs < 0 then hSecs := hSecs + 6000;
    SoundProbeScheduleStartHsecs := hSecs;
    SoundProbeScheduleCurrentHsecs := hSecs;
    SoundProbeHour := 0;
    SoundProbeMinute := 0;
    SoundProbeSecond := hSecs div 100;
    SoundProbeHsec := hSecs mod 100;
end;

function SoundProbeGetScheduleCycleCount: integer;
begin
    SoundProbeGetScheduleCycleCount := SoundProbeScheduleCycleCount;
end;

function SoundProbeGetScheduleCycleHsec(index: integer): integer;
begin
    if (index >= 0) and (index <= 4) then
        SoundProbeGetScheduleCycleHsec := SoundProbeScheduleCycleHsecs[index]
    else
        SoundProbeGetScheduleCycleHsec := 0;
end;
'@
$stagedSound = $stagedSound.Replace($soundProbeVarAnchor, $soundProbeTimeVars.TrimEnd())
$soundQueueImplementationStart = $stagedSound.LastIndexOf('procedure SoundQueue(priority: integer; pattern: string);', [StringComparison]::Ordinal)
$soundQueueImplementationEnd = $stagedSound.IndexOf('procedure SoundClearQueue;', $soundQueueImplementationStart, [StringComparison]::Ordinal)
if ($soundQueueImplementationStart -lt 0 -or $soundQueueImplementationEnd -lt 0) {
    throw 'Could not isolate the staged SoundQueue implementation for logical event capture.'
}
$soundQueueImplementation = $stagedSound.Substring($soundQueueImplementationStart, $soundQueueImplementationEnd - $soundQueueImplementationStart)
$soundQueueFinalEnd = $soundQueueImplementation.LastIndexOf('end;', [StringComparison]::Ordinal)
if ($soundQueueFinalEnd -lt 0) { throw 'Could not locate the staged SoundQueue body end for logical event capture.' }
$soundQueueImplementation = $soundQueueImplementation.Insert(
    $soundQueueFinalEnd,
    "`t`tif SoundProbeQueueCaptureEnabled then SoundProbeCaptureQueue(priority, pattern);" + $newline
)
$stagedSound = $stagedSound.Substring(0, $soundQueueImplementationStart) +
    $soundQueueImplementation + $stagedSound.Substring($soundQueueImplementationEnd)
$elapsedRoutineAnchor = 'function SoundHasTimeElapsed(var counter: integer; duration: integer): boolean;'
$elapsedRoutineStart = $stagedSound.LastIndexOf($elapsedRoutineAnchor, [StringComparison]::Ordinal)
if ($elapsedRoutineStart -lt 0) { throw 'Could not locate SoundHasTimeElapsed for deterministic clock injection.' }
$elapsedGetTimeIndex = $stagedSound.IndexOf($soundTimeCallAnchor, $elapsedRoutineStart, [StringComparison]::Ordinal)
if ($elapsedGetTimeIndex -lt 0) { throw 'Could not locate the elapsed-check GetTime call for deterministic injection.' }
$soundProbeGetTime = @'
if SoundProbeTimeEnabled then begin
                hour := SoundProbeHour;
                minute := SoundProbeMinute;
                sec := SoundProbeSecond;
                hSec := SoundProbeHsec;
            end else GetTime(hour, minute, sec, hSec);
'@
$stagedSound = $stagedSound.Remove($elapsedGetTimeIndex, $soundTimeCallAnchor.Length).Insert(
    $elapsedGetTimeIndex, $soundProbeGetTime.TrimEnd()
)
$soundTimerStart = $stagedSound.IndexOf('procedure SoundTimerHandler;', $elapsedRoutineStart, [StringComparison]::Ordinal)
if ($soundTimerStart -lt 0) { throw 'Could not locate SoundTimerHandler after the elapsed-check routine.' }
$elapsedRoutine = $stagedSound.Substring($elapsedRoutineStart, $soundTimerStart - $elapsedRoutineStart)
$elapsedFinalEnd = $elapsedRoutine.LastIndexOf('end;', [StringComparison]::Ordinal)
if ($elapsedFinalEnd -lt 0) { throw 'Could not locate the end of SoundHasTimeElapsed for schedule instrumentation.' }
$elapsedHookLineStart = $elapsedRoutine.LastIndexOf($newline, $elapsedFinalEnd, [StringComparison]::Ordinal) + $newline.Length
$soundScheduleHook = @'
		if SoundProbeScheduleEnabled then begin
			SoundProbeSchedulePollCount := SoundProbeSchedulePollCount + 1;
			if hSecsDiff >= duration then begin
				if SoundProbeScheduleCycleCount <= 4 then
					SoundProbeScheduleCycleHsecs[SoundProbeScheduleCycleCount] := hSecsTotal;
				SoundProbeScheduleCycleCount := SoundProbeScheduleCycleCount + 1;
			end;
			SoundProbeScheduleElapsedNanos := SoundProbeScheduleElapsedNanos + 54924578;
			if (SoundProbeScheduleStallAfterPoll > 0) and
				(SoundProbeSchedulePollCount = SoundProbeScheduleStallAfterPoll) then
				SoundProbeScheduleElapsedNanos := SoundProbeScheduleElapsedNanos +
					(54924578 * SoundProbeScheduleStallExtraPits);
		SoundProbeScheduleCurrentHsecs :=
			(SoundProbeScheduleStartHsecs + (SoundProbeScheduleElapsedNanos div 10000000)) mod 6000;
		SoundProbeSecond := SoundProbeScheduleCurrentHsecs div 100;
		SoundProbeHsec := SoundProbeScheduleCurrentHsecs mod 100;
	end;
'@
$stagedSound = $stagedSound.Insert(
    $elapsedRoutineStart + $elapsedHookLineStart,
    $soundScheduleHook.TrimEnd() + $newline
)
$stagedSound = $stagedSound.Replace(
    $soundHandlerAnchor,
    'procedure SoundTimerHandler;'
)
$stagedSound = $stagedSound.Replace($soundVectorAnchor, '')
$soundProbeHelper = @'
procedure SoundProbeTick;
begin
	SoundTimerHandler;
end;

'@
$soundUninstallIndex = $stagedSound.LastIndexOf('procedure SoundUninstall;')
if ($soundUninstallIndex -lt 0) { throw 'Could not locate the Sounds unit implementation insertion point.' }
$stagedSound = $stagedSound.Insert($soundUninstallIndex, $soundProbeHelper)
[IO.File]::WriteAllText($stagedSoundPath, $stagedSound, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SOUNDTIK.PAS') -Destination (Join-Path $stageRoot 'SOUNDTIK.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SOUNDCLK.PAS') -Destination (Join-Path $stageRoot 'SOUNDCLK.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'GAMESCHD.PAS') -Destination (Join-Path $stageRoot 'GAMESCHD.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'INTEGPRB.PAS') -Destination (Join-Path $stageRoot 'INTEGPRB.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'DEMOINT.PAS') -Destination (Join-Path $stageRoot 'DEMOINT.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'MOTIONIN.PAS') -Destination (Join-Path $stageRoot 'MOTIONIN.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'COMBATIN.PAS') -Destination (Join-Path $stageRoot 'COMBATIN.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'STATORD.PAS') -Destination (Join-Path $stageRoot 'STATORD.PAS')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BOARDPASSIN.PAS') -Destination (Join-Path $stageRoot 'BRDPASS.PAS')
Write-Host 'Compiling and running direct Turbo Pascal sound-tick probe: SOUNDTIK'
Invoke-DosboxSession 'SOUNDTIK' @(
    'TPC.EXE /B /EBIN /ISRC /USRC SOUNDTIK.PAS > SOUNDTIK.LOG',
    'BIN\SOUNDTIK.EXE'
)
Write-Host 'Compiling and running the direct Turbo Pascal sound-clock probe: SOUNDCLK'
Invoke-DosboxSession 'SOUNDCLK' @(
    'TPC.EXE /EBIN /ISRC /USRC SOUNDCLK.PAS > SOUNDCLK.LOG',
    'BIN\SOUNDCLK.EXE'
)
Write-Host 'Compiling and running direct Turbo Pascal game-scheduler probe: GAMESCHD'
Invoke-DosboxSession 'GAMESCHD-COMPILE' @(
    'TPC.EXE /B /EBIN /ISRC /USRC GAMESCHD.PAS > GAMESCHD-COMPILE.LOG'
)
Invoke-DosboxSession 'GAMESCHD' @('BIN\GAMESCHD.EXE')
Write-Host 'Compiling and running integrated Turbo Pascal gameplay probe: INTEGPRB'
Invoke-DosboxSession 'INTEGPRB-COMPILE' @(
    'TPC.EXE /B /EBIN /ISRC /USRC INTEGPRB.PAS > INTEGPRB-COMPILE.LOG'
)
# Run the heap-sensitive replay in a fresh DOSBox session. Leaving Turbo Pascal's
# compiler process in the same DOS session changes the inherited DOS allocator
# history and makes the probe's post-loop capture intermittently terminate.
Invoke-DosboxSession 'INTEGPRB' @('BIN\INTEGPRB.EXE')
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'INTEGPR.OUT') -PathType Leaf)) {
    throw "Integrated Pascal gameplay probe did not write INTEGPR.OUT. See $stageRoot\INTEGPRB.LOG."
}
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'INTEGSND.OUT') -PathType Leaf)) {
    throw "Integrated Pascal gameplay probe did not write INTEGSND.OUT. See $stageRoot\INTEGPRB.LOG."
}
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'INTEGCEL.OUT') -PathType Leaf)) {
    throw "Integrated Pascal gameplay probe did not write INTEGCEL.OUT. See $stageRoot\INTEGPRB.LOG."
}
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'INTEGSAV.SAV') -PathType Leaf)) {
    throw "Integrated Pascal gameplay probe did not write INTEGSAV.SAV. See $stageRoot\INTEGPRB.LOG."
}
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'INTEGLOD.OUT') -PathType Leaf)) {
    throw "Integrated Pascal gameplay probe did not write INTEGLOD.OUT. See $stageRoot\INTEGPRB.LOG."
}
if (-not (Test-Path -LiteralPath (Join-Path $stageRoot 'INTEG2SV.SAV') -PathType Leaf)) {
    throw "Integrated Pascal gameplay probe did not write INTEG2SV.SAV after reloading INTEGSAV.SAV. See $stageRoot\INTEGPRB.LOG."
}
$syntheticReplayRoot = Join-Path $stageRoot 'integrated-synthetic'
New-Item -ItemType Directory -Force -Path $syntheticReplayRoot | Out-Null
foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
    Copy-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Destination (Join-Path $syntheticReplayRoot $fixtureOutput)
}
Write-Host 'Compiling the direct Pascal DEMO traversal replay: DEMOINT'
Invoke-DosboxSession 'DEMOINT-COMPILE' @(
    'TPC.EXE /B /EBIN /ISRC /USRC DEMOINT.PAS > DEMOINT-COMPILE.LOG'
)
Write-Host 'Running the heap-sensitive Pascal DEMO traversal replay in a fresh DOSBox-X session'
Invoke-DosboxSession 'DEMOINT' @('BIN\DEMOINT.EXE')
foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
    if (-not (Test-Path -LiteralPath (Join-Path $stageRoot $fixtureOutput) -PathType Leaf)) {
        throw "Pascal DEMO traversal replay did not write $fixtureOutput. See $stageRoot\DEMOINT.LOG."
    }
}
$demoReplayRoot = Join-Path $stageRoot 'integrated-demo'
New-Item -ItemType Directory -Force -Path $demoReplayRoot | Out-Null
foreach ($fixtureOutput in @('INTEGPR.OUT', 'INTEGSND.OUT', 'INTEGCEL.OUT', 'INTEGSAV.SAV', 'INTEGLOD.OUT', 'INTEG2SV.SAV')) {
    Copy-Item -LiteralPath (Join-Path $stageRoot $fixtureOutput) -Destination (Join-Path $demoReplayRoot $fixtureOutput)
}

# Exercise the original nested EditorFloodFill routine directly. The test-only
# EditorLoop insertion bypasses menu/input handling and returns immediately.
$stagedEditorPath = Join-Path $stageSource 'EDITOR.PAS'
$stagedEditor = [IO.File]::ReadAllText($stagedEditorPath)
$editorSourceOriginal = $stagedEditor
$editorFillCallAnchor = 'copiedTile.Color := $0F;'
$editorFillCallIndex = $stagedEditor.LastIndexOf($editorFillCallAnchor, [StringComparison]::Ordinal)
if ($editorFillCallIndex -lt 0) { throw 'Could not locate the EditorLoop state setup for the Pascal flood-fill probe.' }
$editorFillCallEnd = $editorFillCallIndex + $editorFillCallAnchor.Length
$stagedEditor = $stagedEditor.Insert(
    $editorFillCallEnd,
    "`r`n`t`tEditorFloodFill(30, 12, Board.Tiles[30][12]);`r`n`t`tExit;"
)
[IO.File]::WriteAllText($stagedEditorPath, $stagedEditor, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'EDITORFL.PAS') -Destination (Join-Path $stageRoot 'EDITORFL.PAS')
Write-Host 'Compiling and running the direct Turbo Pascal editor flood-fill probe: EDITORFL'
Invoke-DosboxSession 'EDITORFL' @(
    'TPC.EXE /B /EBIN /ISRC /USRC EDITORFL.PAS > EDITORFL.LOG',
    'BIN\EDITORFL.EXE'
)

[IO.File]::WriteAllText($stagedEditorPath, $editorSourceOriginal, [Text.UTF8Encoding]::new($false))
$stagedEditor = $editorSourceOriginal
$editorFillCallIndex = $stagedEditor.LastIndexOf($editorFillCallAnchor, [StringComparison]::Ordinal)
if ($editorFillCallIndex -lt 0) { throw 'Could not locate the EditorLoop state setup for the Pascal queue-wrap probe.' }
$editorFillCallEnd = $editorFillCallIndex + $editorFillCallAnchor.Length
$stagedEditor = $stagedEditor.Insert(
    $editorFillCallEnd,
    "`r`n`t`tEditorFloodFill(30, 1, Board.Tiles[30][1]);`r`n`t`tExit;"
)
[IO.File]::WriteAllText($stagedEditorPath, $stagedEditor, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'EDFLDOVR.PAS') -Destination (Join-Path $stageRoot 'EDFLDOVR.PAS')
Write-Host 'Compiling and running the direct Turbo Pascal editor queue-wrap probe: EDFLDOVR'
Invoke-DosboxSession 'EDFLDOVR' @(
    'TPC.EXE /B /EBIN /ISRC /USRC EDFLDOVR.PAS > EDFLDOVR.LOG',
    'BIN\EDFLDOVR.EXE'
)

[IO.File]::WriteAllText($stagedEditorPath, $editorSourceOriginal, [Text.UTF8Encoding]::new($false))
$stagedEditor = $editorSourceOriginal
$editorFillCallIndex = $stagedEditor.LastIndexOf($editorFillCallAnchor, [StringComparison]::Ordinal)
if ($editorFillCallIndex -lt 0) { throw 'Could not locate the EditorLoop state setup for the Pascal dense-queue probe.' }
$editorFillCallEnd = $editorFillCallIndex + $editorFillCallAnchor.Length
$stagedEditor = $stagedEditor.Insert(
    $editorFillCallEnd,
    "`r`n`t`tEditorFloodFill(28, 12, Board.Tiles[28][12]);`r`n`t`tExit;"
)
[IO.File]::WriteAllText($stagedEditorPath, $stagedEditor, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'EDFLDQ17.PAS') -Destination (Join-Path $stageRoot 'EDFLDQ17.PAS')
Write-Host 'Compiling and running the direct Turbo Pascal dense editor queue probe: EDFLDQ17'
Invoke-DosboxSession 'EDFLDQ17' @(
    'TPC.EXE /B /EBIN /ISRC /USRC EDFLDQ17.PAS > EDFLDQ17.LOG',
    'BIN\EDFLDQ17.EXE'
)

[IO.File]::WriteAllText($stagedEditorPath, $editorSourceOriginal, [Text.UTF8Encoding]::new($false))
$stagedEditor = $editorSourceOriginal
$editorFillCallIndex = $stagedEditor.LastIndexOf($editorFillCallAnchor, [StringComparison]::Ordinal)
if ($editorFillCallIndex -lt 0) { throw 'Could not locate the EditorLoop state setup for the Pascal full-board fill probe.' }
$editorFillCallEnd = $editorFillCallIndex + $editorFillCallAnchor.Length
$stagedEditor = $stagedEditor.Insert(
    $editorFillCallEnd,
    "`r`n`t`tEditorFloodFill(30, 12, Board.Tiles[30][12]);`r`n`t`tExit;"
)
[IO.File]::WriteAllText($stagedEditorPath, $stagedEditor, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'EDFLDFUL.PAS') -Destination (Join-Path $stageRoot 'EDFLDFUL.PAS')
Write-Host 'Compiling and running the direct Turbo Pascal full-board editor fill probe: EDFLDFUL'
Invoke-DosboxSession 'EDFLDFUL' @(
    'TPC.EXE /B /EBIN /ISRC /USRC EDFLDFUL.PAS > EDFLDFUL.LOG',
    'BIN\EDFLDFUL.EXE'
)
[IO.File]::WriteAllText($stagedEditorPath, $editorSourceOriginal, [Text.UTF8Encoding]::new($false))

# Compare the actual nested EditorPlaceTile cases against extracted Android
# placement/paste helpers, using the same board-preparation path in both.
# Run after EditorLoop initializes its locals but before the full redraw so this
# helper probe does not depend on VGA/transition state or the interactive loop.
$editorPlacementAnchor = "`t`tEditorDrawRefresh;`r`n`t	if World.BoardCount = 0 then"
$editorPlacementIndex = $editorSourceOriginal.IndexOf($editorPlacementAnchor, [StringComparison]::Ordinal)
if ($editorPlacementIndex -lt 0) { throw 'Could not locate the EditorLoop setup for the Pascal editor-placement probe.' }
$stagedEditor = $editorSourceOriginal
$editorPlacementCode = @'
		cursorPattern := 2;
		cursorColor := $0E;
		wasModified := false;
		EditorPlaceTile(30, 12);
		Board.Stats[0].P1 := Ord(wasModified);

		wasModified := false;
		EditorPlaceTile(31, 12);
		Board.Stats[0].P2 := Ord(wasModified);

		cursorPattern := EditorPatternCount + 1;
		copiedHasStat := false;
		copiedTile.Element := E_WATER;
		copiedTile.Color := $7F;
		wasModified := false;
		EditorPlaceTile(32, 12);
		Board.Stats[0].P3 := Ord(wasModified);

		Board.StatCount := MAX_STAT;
		Board.Tiles[34][12].Element := E_SOLID;
		Board.Tiles[34][12].Color := $0F;
		cursorPattern := EditorPatternCount + 1;
		copiedHasStat := true;
		copiedTile.Element := E_WATER;
		copiedTile.Color := $17;
		wasModified := false;
		EditorPlaceTile(34, 12);
		Board.Stats[0].DataPos := Ord(wasModified);

		Exit;

'@
$editorPlacementIndex = $stagedEditor.IndexOf($editorPlacementAnchor, [StringComparison]::Ordinal)
$stagedEditor = $stagedEditor.Insert($editorPlacementIndex, $editorPlacementCode)
[IO.File]::WriteAllText($stagedEditorPath, $stagedEditor, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'EDPLACE.PAS') -Destination (Join-Path $stageRoot 'EDPLACE.PAS')
Write-Host 'Compiling and running the direct Turbo Pascal editor placement/paste probe: EDPLACE'
Invoke-DosboxSession 'EDPLACE' @(
    'TPC.EXE /B /EBIN /ISRC /USRC EDPLACE.PAS > EDPLACE.LOG',
    'BIN\EDPLACE.EXE'
)
[IO.File]::WriteAllText($stagedEditorPath, $editorSourceOriginal, [Text.UTF8Encoding]::new($false))

# This separate probe calls the same Pascal board-preparation and AddStat
# routines that EditorPlaceTile invokes, keeping the successful script copy
# independent from EditorLoop's local fixture state.
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'EDSTAT.PAS') -Destination (Join-Path $stageRoot 'EDSTAT.PAS')
Write-Host 'Compiling and running the direct Turbo Pascal successful stat/script paste probe: EDSTAT'
Invoke-DosboxSession 'EDSTAT' @(
    'TPC.EXE /B /EBIN /ISRC /USRC EDSTAT.PAS > EDSTAT.LOG',
    'BIN\EDSTAT.EXE'
)
[IO.File]::WriteAllText($stagedEditorPath, $editorSourceOriginal, [Text.UTF8Encoding]::new($false))

# Exercise the original nested EditorTransferBoard routine through the real
# Pascal EditorLoop. The staging copy supplies only menu/filename choices and
# substitutes the post-Escape state at the error-dialog boundary. The modal
# acknowledgement interaction is skipped; import, IOResult checks, file reads,
# BoardCreate/BoardOpen, and editor cleanup remain reconstructed Pascal code.
$shortPayloadBrd = [byte[]]@(0x04, 0x00, 0x10, 0x20)
[IO.File]::WriteAllBytes((Join-Path $stageRoot 'SHORTPAY.BRD'), $shortPayloadBrd)
$stagedEditor = $editorSourceOriginal
$transferChoiceAnchor = "SidebarPromptChoice(true, 3, 'Transfer board:', 'Import Export', i);"
$transferFilenameAnchor = "SidebarPromptString('Import board', '.BRD', SavedBoardFileName, PROMPT_ALPHANUM);"
if (-not $stagedEditor.Contains($transferChoiceAnchor) -or -not $stagedEditor.Contains($transferFilenameAnchor)) {
    throw 'Could not locate the EditorTransferBoard prompt inputs for controlled direct execution.'
}
$stagedEditor = $stagedEditor.Replace($transferChoiceAnchor, 'i := 0;')
$stagedEditor = $stagedEditor.Replace($transferFilenameAnchor, 'SavedBoardFileName := World.Info.Name;')
$editorTransferCallIndex = $stagedEditor.LastIndexOf($editorFillCallAnchor, [StringComparison]::Ordinal)
if ($editorTransferCallIndex -lt 0) { throw 'Could not locate the EditorLoop state setup for the direct board-transfer probe.' }
$editorTransferCallEnd = $editorTransferCallIndex + $editorFillCallAnchor.Length
$stagedEditor = $stagedEditor.Insert(
    $editorTransferCallEnd,
    "`r`n`t`tEditorTransferBoard;`r`n`t`tExit;"
)
[IO.File]::WriteAllText($stagedEditorPath, $stagedEditor, [Text.UTF8Encoding]::new($false))
$stagedGamePath = Join-Path $stageSource 'GAME.PAS'
$stagedGameBeforeBrdProbe = [IO.File]::ReadAllText($stagedGamePath)
$errorDialogSelectAnchor = 'TextWindowSelect(textWindow, false, false);'
if (-not $stagedGameBeforeBrdProbe.Contains($errorDialogSelectAnchor)) {
    throw 'Could not locate the DisplayIOError acknowledgement input for the direct board-transfer probe.'
}
$stagedGameForBrdProbe = $stagedGameBeforeBrdProbe.Replace(
    $errorDialogSelectAnchor,
    'TextWindowRejected := true; InputKeyPressed := #0;'
)
[IO.File]::WriteAllText($stagedGamePath, $stagedGameForBrdProbe, [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'BRDXFER.PAS') -Destination (Join-Path $stageRoot 'BRDXFER.PAS')
Write-Host 'Compiling and running the direct Turbo Pascal short-payload editor import probe: BRDXFER'
Invoke-DosboxSession 'BRDXFER-COMPILE' @(
    'TPC.EXE /B /EBIN /ISRC /USRC BRDXFER.PAS > BRDXFER.LOG'
)
if (-not (Test-Path -LiteralPath (Join-Path $stageBin 'BRDXFER.EXE') -PathType Leaf)) {
    throw "Turbo Pascal did not produce BIN\BRDXFER.EXE. See $stageRoot\BRDXFER.LOG and $stageRoot\BRDXFER-COMPILE.LOG."
}
# The imported board's deliberately short payload exercises original
# IOResult/BoardOpen behavior. Running after the compiler in the same DOSBox-X
# allocator history can hang; the identical compiled probe succeeds in a fresh
# maximum-speed session, with no clock-dependent behavior in this operation.
Invoke-DosboxSession 'BRDXFER-RUN' @('BIN\BRDXFER.EXE SHORTPAY BRDXFER.OUT') 'max'
[IO.File]::WriteAllText($stagedEditorPath, $editorSourceOriginal, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText($stagedGamePath, $stagedGameBeforeBrdProbe, [Text.UTF8Encoding]::new($false))

$editorWrapExpected = [Collections.Generic.List[string]]::new()
$editorWrapExpected.Add('queue-wrap filled 792 empty 0')
for ($y = 1; $y -le 25; $y++) {
    $row = [char[]]::new(60)
    for ($x = 1; $x -le 60; $x++) {
        $row[$x - 1] = if (($y % 2) -eq 1) { 'F' } else { 'S' }
    }
    if (($y % 2) -eq 0) {
        $connectorX = if (([int]($y / 2) % 2) -eq 1) { 60 } else { 1 }
        $row[$connectorX - 1] = 'F'
    }
    if ($y -eq 2) { $row[29] = 'P' }
    $editorWrapExpected.Add((-join $row))
}
$editorDenseExpected = [Collections.Generic.List[string]]::new()
$editorDenseExpected.Add('dense-queue filled 289 empty 0')
$editorDenseExpected.Add(('S' * 19))
for ($row = 0; $row -lt 17; $row++) {
    $editorDenseExpected.Add(('S' + ('F' * 17) + 'S'))
}
$editorDenseExpected.Add(('S' * 19))
$editorFullExpected = [Collections.Generic.List[string]]::new()
$editorFullExpected.Add('full-board filled 1334 empty 0')
$editorFullExpected.Add(('P' + ('?' * 59)))
for ($row = 0; $row -lt 23; $row++) {
    $editorFullExpected.Add(('?' + ('F' * 58) + '?'))
}
$editorFullExpected.Add(('?' * 60))

$expectedOutputs = [ordered]@{
    'VALPROBE.OUT' = @('-1 0', '0 10', '0 10', '0 10')
    'EDITORFL.OUT' = @('enclosed-fill 9 center 21 14 outside 21 15')
    'EDFLDOVR.OUT' = $editorWrapExpected.ToArray()
    'EDFLDQ17.OUT' = $editorDenseExpected.ToArray()
    'EDFLDFUL.OUT' = $editorFullExpected.ToArray()
    'EDPLACE.OUT' = @('N 22 14 1', 'P 4 15 1', 'C 19 127 1', 'L 21 15 0 150')
    'EDSTAT.OUT' = @(
        'S 13 23 1 1 35 12 9 2 3 77 88 99 255 255 0 47 0 0 0 0 0 0 0',
        'D 13 23 1 2 36 12 9 2 3 77 88 99 255 255 0 47 4 0 66 13 0 90 1'
    )
    'INPUTBUF.OUT' = @(
        'one 65', 'one 0',
        'two 65', 'two 65', 'two 0',
        'three 65', 'three 66', 'three 0',
        'four 65', 'four 67', 'four 0',
        'repeat-four 82', 'repeat-four 82', 'repeat-four 0',
        'cp437-high 130', 'cp437-top 255',
        'arrow-up 200 0 -1 0 -1', 'arrow-left 203 -1 0 -1 0',
        'arrow-right 205 1 0 1 0', 'arrow-down 208 0 1 0 1',
        'arrow-right-burst 205 1 0 1 0', 'arrow-right-burst 205 1 0 1 0',
        'arrow-right-burst 0 0 0 1 0',
        'numeric-up 56 0 -1 0 -1', 'numeric-left 52 -1 0 -1 0',
        'numeric-right 54 1 0 1 0', 'numeric-down 50 0 1 0 1',
        'neutral 53 0 0 0 1', 'empty 0 0 0 0 1'
    )
    'SOUNDTIK.OUT' = @(
        'single 1 1 0 0',
        'odd-first 1 3 1 1', 'odd-end 2 3 0 0',
        'even-first 1 3 1 1', 'even-end 2 3 0 0',
        'zero-first 1 3 0 1', 'zero-wrapped 256 3 1 1',
        'zero-next 257 5 1 1'
    )
    'SOUNDCLK.OUT' = @(
        'before-minute-wrap 0 5990',
        'last-centisecond 1 5999',
        'minute-rollover 1 1',
        'below-threshold 0 1',
        'at-threshold 1 6',
        'zero-duration 1 1',
        'backward-clock-step 1 90'
    )
    'GAMESCHD.OUT' = @(
        'speed 1 start 0 stall 0 extra 0 cycles 5 delta 5 stat 0 counter 27 times 5 10 16 21 27',
        'speed 6 start 0 stall 0 extra 0 cycles 5 delta 5 stat 0 counter 82 times 16 32 49 65 82',
        'speed 9 start 0 stall 0 extra 0 cycles 5 delta 5 stat 0 counter 109 times 21 43 65 87 109',
        'speed 6 start 0 stall 2 extra 4 cycles 5 delta 5 stat 0 counter 98 times 32 49 65 82 98',
        'speed 9 start 5980 stall 0 extra 0 cycles 5 delta 5 stat 0 counter 89 times 1 23 45 67 89'
    )
    'ZEROSPD.OUT' = @('zero-speed cycles 5 delta 5 stat 0 timer 0 counter 0 exit 1')
    'BOARDENC.OUT' = @(
        'runs 255 255 255 255 255 225',
        'tiles 0 112 0 112 0 112 0 112 0 112 0 112'
    )
    'BOARDDEC.OUT' = @(
        'zero first 7 8 row-end 7 8 cell256 7 8 cell257 1 2 final 1 2',
        'overrun cell1275 5 15 cell1276 6 16 final 6 16 max-shots 77'
    )
    'WORLDTIT.OUT' = @('title-only 1 0 0 1 1')
    'FULLLOAD.OUT' = @('full-load 1 15 0 0 1 missing-len 0 0')
    'SHORTLEN.OUT' = @('short-len 1 15 0 272')
    'NEGLEN.OUT' = @('negative-len 1 0 0 -1')
    'NEGBRD.OUT' = @('negative-brd 1 -1')
    'BRDIOPRB.OUT' = @('partial-length 100 4610 next-IOResult 0', 'short-payload 0 100 next-IOResult 0 16 32 165 165')
    'BRDXFER.OUT' = @('board 0 255 0 4 31 30 12 0')
    'PLRTORCH.OUT' = @('0 199 30 12')
    'PLAYERST.OUT' = @(
        'shoot-held-right ammo 1 input 0 0 dir 1 0 stats 1 target 18 15 sound 2 6 bullet 11 10 0 1 0 100 under 0 0 64 1 48 1 32 1',
        'shoot-space-down ammo 1 input 0 0 dir 0 1 stats 1 target 18 15 sound 2 6 bullet 10 11 0 0 1 100 under 0 0 64 1 48 1 32 1',
        'shoot-blocked ammo 2 input 1 0 dir 1 0 stats 0 target 21 31 sound -1 0',
        'shoot-limit ammo 2 input 1 0 dir 1 0 stats 1 target 0 0 sound -1 0',
        'shoot-space-no-direction ammo 2 input 0 0 dir 0 0 stats 0 target 0 0 sound -1 0'
    )
    'PLAYERMV.OUT' = @(
        'move-empty player 11 10 delta 1 0 dir 1 0 stats 0 t0 0 23 t1 4 31 t2 0 0 under 0 71 sound -1 0',
        'move-blocked player 10 10 delta 1 0 dir 1 0 stats 0 t0 4 31 t1 21 95 t2 0 0 under 0 23 sound -1 0',
        'move-push player 11 10 delta 1 0 dir 1 0 stats 0 t0 0 23 t1 4 31 t2 24 14 under 0 14 sound 2 2',
        'move-push-blocked player 10 10 delta 1 0 dir 1 0 stats 0 t0 4 31 t1 24 14 t2 21 95 under 0 23 sound 2 2'
    )
    'PLAYERKY.OUT' = @(
        'pause-key paused 1 health 100 torch 0 energizer 0 duration 4 color 31 char 1 key 112 sound 3 6 48 1',
        'sound-key paused 0 health 100 enabled 1 playing 0 buffer 0 priority 2 key 32'
    )
    'PAUSEMV.OUT' = @(
        'pause-open player 11 10 old 0 23 tile 4 31 under 0 70 delta 1 0 paused 0 save 1 exit 1 key 54 health 0',
        'pause-cycle player 11 10 old 0 23 tile 4 31 under 0 70 delta 0 0 paused 0 save 1 exit 1 key 0 health 0',
        'pause-blocked player 10 10 old 4 31 tile 21 95 under 0 23 delta 1 0 paused 1 save 0 exit 1 key 54 health 0'
    )
    'PLAYERDE.OUT' = @(
        'dead-space player 10 10 health 0 ammo 1 input 0 0 shift FALSE dir 1 0 stats 2 tick 0 block TRUE sound -1 0 tile0 2 0 p2 0 shot 18',
        'dead-existing-stat player 10 10 health 0 ammo 2 input 0 0 shift FALSE dir 0 0 stats 1 tick 0 block TRUE sound -1 0 tile0 36 255 p2 0 shot 0'
    )
    'BEARPROB.OUT' = @('11 10 0 112', '10 11 0 112', '10 10 34 6')
    'MONPRB.OUT' = @(
        '27 1 27', '97 1 97', '101 1 101', '104 1 104', '110 1 110',
        '112 1 112', '113 1 113', '114 1 114', '115 1 115', '119 1 119',
        '65 1 65', '69 1 69', '72 1 72', '78 1 78', '80 1 80',
        '81 1 81', '82 1 82', '83 1 83', '87 1 87', '124 1 124',
        '32 0 32', '98 0 98', '102 0 102', '120 0 120', '48 0 48',
        '13 0 13', '200 0 200', '120 1 120'
    )
    'BLINKPRB.OUT' = @(
        'right-empty player 21 11 origin 43 7 right 4 127 left 21 112 player-under 0 112 p3 3 health 90 stats 2 timer 11 message Ouch!',
        'left-empty player 21 11 origin 43 7 right 4 127 left 0 112 player-under 21 112 p3 3 health 90 stats 2 timer 11 message Ouch!'
    )
    'SCROLLTH.OUT' = @(
        'default 6 83 99 114 111 108 108 2 10 70 105 114 115 116 32 108 105 110 101 11 83 101 99 111 110 100 32 108 105 110 101 4 0 0 2 -1',
        'override 19 83 99 114 111 108 108 32 99 117 115 116 111 109 32 116 105 116 108 101 2 10 70 105 114 115 116 32 108 105 110 101 11 83 101 99 111 110 100 32 108 105 110 101 4 0 0 2 -1'
    )
    'SPINGUN.OUT' = @(
        'aimed-vertical-bullet stats 2 player 10 20 gun 12 10 rng 134775814 projectile 18 12 11 step 0 1 p1 1 p2 100 under 0 0',
        'aimed-same-row-bullet stats 2 player 20 10 gun 10 12 rng 134775814 projectile 18 11 12 step 1 0 p1 1 p2 100 under 0 0',
        'vertical-bullet stats 2 player 10 20 gun 12 10 rng -596792289 projectile 18 13 10 step 1 0 p1 1 p2 100 under 0 0',
        'same-row-bullet stats 2 player 20 10 gun 10 12 rng -596792289 projectile 18 11 12 step 1 0 p1 1 p2 100 under 0 0',
        'vertical-star stats 2 player 10 20 gun 12 10 rng -596792289 projectile 15 13 10 step 1 0 p1 1 p2 100 under 0 0',
        'fire-rate-zero stats 1 player 10 20 gun 12 10 rng 1 no-projectile'
    )
    'SHARK.OUT' = @('Seed-zero Random(10)=0', '11 10 0 38 19 19', '10 10 38 0 0 112')
    'LION.OUT' = @('seed 3 draws 9 2 tick 11 10 0 0', 'seed 26 draws 9 1 1 tick 10 11 0 0', 'seed 0 draws 0 0 tick 11 10 0 0')
    'RUFFIAN.OUT' = @(
        'seeds 1 85 454',
        'wake draws 16 8 0 player 20 10 ruffian 10 10 step 1 0 under 0 112 tiles 35 13 0 0 0 0',
        'move-retain draws 0 0 0 player 20 10 ruffian 11 10 step 1 0 under 0 0 tiles 0 112 35 13 0 0',
        'move-stop draws 8 16 player 20 10 ruffian 12 10 step 0 0 under 0 0 tiles 0 112 0 0 35 13'
    )
    'STARPRB.OUT' = @(
        'tick1 p2 3 pos 10 10 step 0 0 under 0 112 tiles 15 9 19 19 count 1 seed 0',
        'tick2 p2 2 pos 11 10 step 1 0 under 19 19 tiles 0 112 15 9 count 1 seed 1',
        'tick3 p2 1 pos 11 10 step 1 0 under 19 19 tiles 0 112 15 10 count 1 seed 1',
        'tick4 count 0 tiles 0 112 19 19 seed 1'
    )
    'TIGER.OUT' = @(
        'blocked-vertical seed 330 draws 0 0 0 count 1 tiger 11 10 tiles 0 112 21 255 42 11 final-seed -1860696767',
        'star-horizontal seed 330 draws 0 0 0 count 2 tiger 10 10 tiles 42 11 0 0 15 9 projectile 11 10 1 1 0 100 final-seed -1860696767'
    )
    'CENTIP.OUT' = @(
        'follow seed 0 draws 0 count 2 s1 11 10 1 0 -1 2 4 0 s2 10 10 1 0 1 -1 4 0 row 0 0 0 112 45 15 44 15 seed-after 1',
        'reverse seed 256 draws 0 0 0 count 3 s1 10 10 0 0 2 -1 4 0 s2 9 10 1 0 3 1 4 0 s3 8 10 1 0 2 2 4 0 row 44 255 45 255 45 255 21 0 seed-after 1260005663',
        'segment-pending seed 0 draws  count 1 s1 10 10 1 0 -2 -1 4 0 row 0 0 0 0 45 255 0 112 seed-after 0',
        'segment-head seed 0 draws  count 1 s1 10 10 1 0 -2 -1 4 0 row 0 0 0 0 44 255 0 112 seed-after 0'
    )
    'PUSHDUP.OUT' = @(
        'dup-tick 1 2 10 10 1 0 21 5 2 0 -1 -1 0 112 11 10 -1 1 4 6 7 8 -1 -1 0 112 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0 112 12 14 36 42 0 0 0 0 0 0 -1 0 0 -1 -1 -1 -1 -1 -1 -1 -1',
        'dup-tick 2 3 10 10 1 0 21 0 2 0 -1 -1 0 112 11 10 -1 1 4 6 7 8 -1 -1 0 112 9 10 -1 1 4 6 7 8 -1 -1 0 112 36 42 12 14 36 42 0 0 0 0 0 0 3 1 10 48 2 50 2 52 2 53 2',
        'dup-tick 3 1 10 10 1 0 21 0 2 0 -1 -1 0 112 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 24 42 12 14 24 42 0 0 0 0 0 0 3 1 10 48 2 50 2 52 2 53 2',
        'dup-tick 4 150 10 10 1 0 21 0 2 0 -1 -1 0 112 11 10 -1 1 4 6 7 8 -1 -1 0 112 9 10 -1 1 4 6 7 8 -1 -1 0 112 36 42 12 14 36 42 0 0 0 0 0 0 3 1 10 48 2 50 2 52 2 53 2',
        'dup-tick 5 150 10 10 1 0 21 0 2 0 -1 -1 0 112 11 10 -1 1 4 6 7 8 -1 -1 0 112 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0 112 12 14 36 42 0 0 0 0 0 0 3 1 10 48 2 50 2 52 2 53 2',
        'pusher-tick 1 2 11 10 1 0 0 0 0 0 -1 -1 0 112 12 10 1 0 0 0 0 0 -1 -1 0 14 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0 0 0 112 40 15 40 15 24 14 0 112 2 1 2 21 1 -1 -1 -1 -1 -1 -1',
        'pusher-tick 2 2 10 10 1 0 0 0 0 0 -1 -1 0 112 11 10 1 0 0 0 0 0 -1 -1 0 112 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0 0 40 15 40 15 24 14 21 15 0 112 -1 0 0 -1 -1 -1 -1 -1 -1 -1 -1'
    )
    'DOORPRB.OUT' = @(
        'black 1 9 1', 'black 257 0 1', 'black 513 0 1',
        'black-key pickup 1 257 0', 'black-key duplicate 257 8',
        'color 1 wrong 9 1', 'color 1 match 0 1 0 1',
        'color 2 wrong 9 1', 'color 2 match 0 1 0 1',
        'color 3 wrong 9 1', 'color 3 match 0 1 0 1',
        'color 4 wrong 9 1', 'color 4 match 0 1 0 1',
        'color 5 wrong 9 1', 'color 5 match 0 1 0 1',
        'color 6 wrong 9 1', 'color 6 match 0 1 0 1',
        'color 7 wrong 9 1', 'color 7 match 0 1 0 1'
    )
    'ITEMPRB.OUT' = @(
        'player-state', 'ammo 12 0', 'ammo-overflow -32766 0',
        'gem 8 101 60 0', 'gem-overflow -32768 0 -32766 0',
        'torch 6 0 0'
    )
    'ENERGY.OUT' = @('energizer-state', 'energizer 0 63 75 9 112 32 3 32 3 30 0 2 0 0 22 7 3 -2')
    'TORCH.OUT' = @(
        'torch-state',
        'torch-message -32768 0 46 45 0 1 0 0 40 3 6 48 1 57 1 52 2 2 -1',
        'torch-silent 1 0 46 0 0 0 3 6 48 1 57 1 52 2 2 -1'
    )
    'BOMB.OUT' = @(
        'bomb-state',
        'bomb-start 9 13 40 15 2 0 0 40 4 10 48 1 53 1 64 1 69 1 80 1 1 0',
        'bomb-push 9 11 10 0 112 13 8 0 49 -1 0 1 0',
        'bomb-blocked 9 10 10 13 40 22 14 -1 0 1 0',
        'bomb-tick 1 1 0 0 112 13 40 0 80 -1 0 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1',
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
    if ($LASTEXITCODE -ne 0) { throw "Turbo Pascal/production positive-speed schedule comparison failed with exit code $LASTEXITCODE." }
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
}
finally {
    Pop-Location
}

$directProbeCount = $probeFiles.Count + 65 # Includes twelve separate compile/run probes, BINDALIA and BINDORDR separate compile/run sessions, eleven isolated BINDSIZE cases, the DEMO/F02/F03/F04/F05 integrated fixtures, four isolated OBJTIK cases, and isolated OOPPARSE runtime; short-header BoardOpen remains excluded because its freed-buffer path is unstable under DOSBox-X.
Write-Host "Direct Pascal 5.5 probe sessions passed: $directProbeCount ($($probeFiles.Count) standard probes plus EDITORFL, EDFLDOVR, EDFLDQ17, EDFLDFUL, EDPLACE, EDSTAT, BRDXFER-COMPILE, BRDXFER-RUN, NEGLEN-COMPILE, NEGLEN-RUN, NEGBRD-COMPILE, NEGBRD-RUN, STARPRB-COMPILE, STARPRB-RUN, CENTIPR-COMPILE, CENTIPR-RUN, PUSHDUP-COMPILE, PUSHDUP-RUN, BINDPROB-COMPILE, BINDPROB-RUN, SLIMEPRB-COMPILE, SLIMEPRB-RUN, DRAWPRB-COMPILE, DRAWPRB-RUN, MSGTIK-COMPILE, MSGTIK-RUN, TORCHPR-COMPILE, TORCHPR-RUN, SOUNDTIK, SOUNDCLK, INTEGPRB, DEMOINT, MOTIONIN, COMBATIN, STATORD, BOARDPASSIN, BINDALIA-COMPILE, BINDALIA-RUN, BINDREUS, BINDORDR-COMPILE, BINDORDR-RUN, BINDSIZE-COMPILE, BINDSIZE-SML, BINDSIZE-MED, BINDSIZE-LRG, BINDSIZE-XL, BINDSIZE-F41, BINDSIZE-F45, BINDSIZE-F50, BINDSIZE-F55, BINDSIZE-F60, BINDSIZE-F63, BINDSIZE-F64, BINDTAIL, BINDSEL, BINDWIN, SOUNDFRQ, OOPIFPRB, OOPGIVPRB, OOPCHARPRB, OOPPUTPRB, OOPCHGPRB, OOPFLAGPRB, OOPSEND-COMPILE, OOPSENDPRB, OBJTIK-COMPILE, OBJTIK01, OBJTIK02, OBJTIK03, OBJTIK04, OBJTOUCHPRB, OOPFLOWPRB, OOPPARSE-COMPILE, OOPPARSEPRB, OOPANYPRB)."
Write-Host "Pascal oracle and production compatibility suites passed. Staging output: $stageRoot"
