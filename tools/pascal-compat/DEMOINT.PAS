program DemoTraversalReplay;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;

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
			if (GameLoopProbeCellWriteEvents <> nil) and
				(GameLoopProbeCellWriteCount < GameLoopProbeCellWriteCapacity) then begin
				eventIndex := GameLoopProbeCellWriteCount + 1;
				GameLoopProbeCellWriteEvents^[eventIndex].X := cellX;
				GameLoopProbeCellWriteEvents^[eventIndex].Y := y;
				GameLoopProbeCellWriteEvents^[eventIndex].Color := color;
				GameLoopProbeCellWriteEvents^[eventIndex].Character := Ord(text[charIndex]);
				GameLoopProbeCellWriteCount := eventIndex;
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
	roundTripLoaded: boolean;
	loadOutput: text;

begin
	{ Initialize test-only capture hooks before any setup can call BoardChange. }
	GameLoopProbeEnabled := false;
	GameLoopProbeCaptureNextBoardChange := false;
	VideoWriteText := CaptureCellWrite;
	New(IoTmpBuf);
	RandSeed := 0;
	WorldCreate;
	if not WorldLoad('DEMO', '.ZZT', false) then RunError(254);
	ReturnBoardId := World.Info.CurrentBoard;
	BoardChange(ReturnBoardId);
	BoardEnter;
	GameStateElement := E_PLAYER;
	GamePaused := true;
	JustStarted := false;
	TickSpeed := 0;
	TickTimeCounter := 0;
	SoundTimeCheckCounter := 0;
	UseSystemTimeForElapsed := true;
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
	CurrentTick := 0;
	CurrentStatTicked := 0;
	TickTimeDuration := 0;
	TickTimeCounter := 0;
	GamePlayExitRequested := false;
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeCellWriteCount := 0;
	GameLoopProbeCellWriteOverflow := false;
	GetMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
	Assign(output, 'INTEGPR.OUT');
	Rewrite(output);
	Close(output);
	GameLoopProbeEnabled := true;
	GameLoopProbeCapture('L', 0);
	InputProbeSetKeyBuffer('666666');
	GameLoopProbeStopAfter := 6;
	GameLoopProbeCycles := 0;
	SoundProbeSetSchedule(0, 0, 0);
	SoundProbeQueueCapture(false);
	SoundProbeQueueCapture(true);
	GamePlayLoop(true);
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeEnabled := false;
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
	if not roundTripLoaded then RunError(255);
	GameLoopProbeCapture('T', 3);
	WorldSave('INTEG2SV', '.SAV');
	GameLoopProbeEnabled := false;
	GameLoopProbeCaptureNextBoardChange := false;
	GameLoopProbeCapture('F', 0);
	FreeMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
	GameLoopProbeCellWriteEvents := nil;
	WorldUnload;
	Dispose(IoTmpBuf);
end.
