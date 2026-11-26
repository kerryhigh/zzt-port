program MalformedLoadHighScoreReplay;
{$M 49152,163840,655360}
uses Crt, Video, GameVars, Elements, Sounds, Game, Input, Editor, TxtWind;

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
	loaded: boolean;
	i, j: integer;
	longName: string[50];
	roundTripLoaded: boolean;

begin
	GameLoopProbeEnabled := false;
	VideoInstall(80, Blue);
	TextWindowInit(5, 3, 50, 18);
	VideoHideCursor;
	ClrScr;
	VideoWriteText := CaptureCellWrite;
	New(IoTmpBuf);
	RandSeed := 0;
	GenerateTransitionTable;
	WorldCreate;
	World.Info.unk1 := 0;
	loaded := WorldLoad('F10BAD', '.ZZT', false);
	Assign(loadOutput, 'F10BAD.OUT');
	Rewrite(loadOutput);
	WriteLn(loadOutput, Ord(loaded));
	Close(loadOutput);
	if loaded then RunError(259);

	{ GameTitleLoop creates a new world when its startup WorldLoad fails. }
	WorldCreate;
	World.Info.unk1 := 0;
	World.Info.Name := 'F10ROOM';
	World.Info.Health := 0;
	World.Info.Score := 1234;
	for i := 1 to HIGH_SCORE_COUNT do begin
		longName := '';
		for j := 1 to 50 do
			longName := longName + Chr(65 + ((((i - 1) * 7) + (j - 1)) mod 26));
		HighScoreList[i].Name := longName;
		HighScoreList[i].Name := '';
	end;
	{ No F10ROOM.HI exists: exercise the original failed-load reset before insertion. }
	HighScoresLoad;
	BoardEnter;

	GameStateElement := E_PLAYER;
	GamePaused := false;
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
	InputProbeSetKeyBuffer('');
	InputProbeSetReadKeys('KERRY' + #13);
	GameLoopProbeStopAfter := 2;
	GameLoopProbeCycles := 0;
	GameLoopProbeCycleClockEnabled := true;
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
	if not roundTripLoaded then RunError(260);
	GameLoopProbeCapture('T', 3);
	WorldSave('INTEG2SV', '.SAV');
	GameLoopProbeCapture('F', 0);
	FreeMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
	GameLoopProbeCellWriteEvents := nil;
	WorldUnload;
	Dispose(IoTmpBuf);
end.
