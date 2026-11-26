program EditorBoardRoundtripReplay;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input, Editor;

{$F+}
procedure CaptureCellWrite(x, y, color: byte; text: TVideoLine);
var
	charIndex, cellX: integer;
	eventIndex: longint;
begin
	if not GameLoopProbeCellCaptureEnabled then Exit;
	for charIndex := 1 to Length(text) do begin
		cellX := x + charIndex - 1;
		if (cellX >= 0) and (cellX < 60) and (y < 25) then begin
			if GameLoopProbeCellWriteEvents <> nil then begin
				eventIndex := GameLoopProbeCellWriteCount + 1;
				if eventIndex <= GameLoopProbeCellWriteCapacity then begin
					GameLoopProbeCellWriteEvents^[eventIndex].X := cellX;
					GameLoopProbeCellWriteEvents^[eventIndex].Y := y;
					GameLoopProbeCellWriteEvents^[eventIndex].Color := color;
					GameLoopProbeCellWriteEvents^[eventIndex].Character := Ord(text[charIndex]);
					GameLoopProbeCellWriteCount := eventIndex;
				end else
					GameLoopProbeCellWriteOverflow := true;
			end else
				GameLoopProbeCellWriteOverflow := true;
		end;
	end;
end;
{$F-}

procedure WriteCellWriteEvents(filename: string);
var
	cellOutput: text;
	eventIndex: longint;
begin
	Assign(cellOutput, filename);
	Rewrite(cellOutput);
	WriteLn(cellOutput, GameLoopProbeCellWriteCount, ' ', Ord(GameLoopProbeCellWriteOverflow));
	for eventIndex := 1 to GameLoopProbeCellWriteCount do begin
		with GameLoopProbeCellWriteEvents^[eventIndex] do
			WriteLn(cellOutput, X, ' ', Y, ' ', Color, ' ', Character);
	end;
	Close(cellOutput);
end;

var
	output: text;
	loadOutput: text;
	roundTripLoaded: boolean;

begin
	GameLoopProbeEnabled := false;
	GameLoopProbeCaptureNextBoardChange := false;
	VideoWriteText := CaptureCellWrite;
	New(IoTmpBuf);
	RandSeed := 0;
	GenerateTransitionTable;
	WorldCreate;
	World.Info.unk1 := 0;
	BoardClose;
	World.BoardCount := 1;
	World.Info.CurrentBoard := 1;
	BoardCreate;
	Board.Name := 'ROOM';

	{ Edit board 1 using real editor keys, then export and import its BRD record. }
	InputProbeSetEditorTape(#205 + #190 + 'A' + #13 + 'T' + #13 +
		'ABCGH' + #13 + 'T' + #203 + #13 + 'ABCGH' + #13 + 'Q' + 'N');
	EditorLoop;
	if InputProbeGetEditorTape <> '' then RunError(254);
	InputProbeSetEditorTape('');
	if (World.Info.CurrentBoard <> 1) or (Board.Name <> 'ROOM') or
		(Board.Tiles[31][12].Element <> E_TEXT_MIN + 5) or
		(Board.Tiles[31][12].Color <> Ord('A')) then RunError(255);

	BoardEnter;
	GameStateElement := E_PLAYER;
	GamePaused := true;
	JustStarted := false;
	TickSpeed := 0;
	TickTimeCounter := 0;
	SoundTimeCheckCounter := 0;
	UseSystemTimeForElapsed := false;
	TimerTicks := 0;
	SoundEnabled := false;
	SoundIsPlaying := false;
	SoundCurrentPriority := -1;
	SoundBlockQueueing := false;
	SoundBuffer := '';
	SoundBufferPos := 1;
	SoundDurationCounter := 1;
	SoundDurationMultiplier := 1;
	InputDeltaX := 0;
	InputDeltaY := 0;
	InputKeyPressed := #0;
	InputShiftPressed := false;
	InputShiftAccepted := false;
	PlayerDirX := 0;
	PlayerDirY := 0;
	CurrentTick := 0;
	CurrentStatTicked := 0;
	TickTimeDuration := 0;
	GamePlayExitRequested := false;
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeCellWriteCount := 0;
	GameLoopProbeCellWriteOverflow := false;
	GetMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));

	Assign(output, 'INTEGPR.OUT');
	Rewrite(output);
	Close(output);
	GameLoopProbeSeedPauseBlink := true;
	GameLoopProbeEnabled := true;
	GameLoopProbeCapture('L', 0);
	InputProbeSetKeyBuffer('888888');
	GameLoopProbeStopAfter := 6;
	GameLoopProbeCycles := 0;
	GameLoopProbeCycleClockEnabled := true;
	SoundProbeQueueCapture(false);
	SoundProbeQueueCapture(true);
	GamePlayLoop(true);
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeEnabled := false;
	if InputProbeGetKeyBuffer <> '' then RunError(256);
	SoundProbeQueueCapture(false);
	SoundProbeWriteQueueEvents('INTEGSND.OUT');
	WriteCellWriteEvents('INTEGCEL.OUT');
	WorldSave('INTEGSAV', '.SAV');
	GameLoopProbeCapture('T', 2);
	roundTripLoaded := WorldLoad('INTEGSAV', '.SAV', false);
	Assign(loadOutput, 'INTEGLOD.OUT');
	Rewrite(loadOutput);
	WriteLn(loadOutput, Ord(roundTripLoaded));
	Close(loadOutput);
	if not roundTripLoaded then RunError(257);
	GameLoopProbeCapture('T', 3);
	WorldSave('INTEG2SV', '.SAV');
	GameLoopProbeCapture('F', 0);
	FreeMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
	GameLoopProbeCellWriteEvents := nil;
	WorldUnload;
	Dispose(IoTmpBuf);
end.
