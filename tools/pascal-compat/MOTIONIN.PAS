program WallPusherDuplicatorReplay;
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
	loadOutput: text;
	roundTripLoaded: boolean;
	statId: integer;
	script: string[32];

begin
	VideoWriteText := CaptureCellWrite;
	New(IoTmpBuf);
	RandSeed := 0;
	WorldCreate;
	{ Match the deterministic zeroed allocation used by the production fixture. }
	World.Info.unk1 := 0;
	BoardCreate;
	World.Info.Health := 100;
	World.Info.Gems := 0;
	World.Info.Ammo := 0;
	World.Info.Score := 0;

	Board.Stats[0].X := 5;
	Board.Stats[0].Y := 5;
	Board.Stats[0].StepX := 0;
	Board.Stats[0].StepY := 0;
	Board.Stats[0].Cycle := 1;
	Board.Stats[0].P1 := 0;
	Board.Stats[0].P2 := 0;
	Board.Stats[0].P3 := 0;
	Board.Stats[0].Follower := 0;
	Board.Stats[0].Leader := 0;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := $00;
	Board.Stats[0].Data := nil;
	Board.Stats[0].DataPos := 0;
	Board.Stats[0].DataLen := 0;
	Board.Tiles[5][5].Element := E_PLAYER;
	Board.Tiles[5][5].Color := ElementDefs[E_PLAYER].Color;
	Board.Tiles[6][5].Element := E_EMPTY;
	Board.Tiles[6][5].Color := $70;
	Board.Tiles[7][5].Element := E_BOULDER;
	Board.Tiles[7][5].Color := $0E;
	Board.Tiles[8][5].Element := E_EMPTY;
	Board.Tiles[8][5].Color := $70;
	Board.Tiles[9][5].Element := E_SOLID;
	Board.Tiles[9][5].Color := $0F;

	{ Source stat precedes the pusher chain and duplicator in original stat order. }
	AddStat(21, 15, E_OBJECT, $0A, 1, StatTemplateDefault);
	statId := Board.StatCount;
	script := '#GIVE GEMS 1' + #13 + '#IDLE' + #13;
	GetMem(Board.Stats[statId].Data, Length(script));
	Move(script[1], Board.Stats[statId].Data^, Length(script));
	Board.Stats[statId].DataLen := Length(script);
	Board.Stats[statId].DataPos := 0;

	AddStat(10, 10, E_PUSHER, $0F, 1, StatTemplateDefault);
	statId := Board.StatCount;
	Board.Stats[statId].StepX := 1;
	AddStat(11, 10, E_PUSHER, $0F, 1, StatTemplateDefault);
	statId := Board.StatCount;
	Board.Stats[statId].StepX := 1;
	Board.Tiles[12][10].Element := E_BOULDER;
	Board.Tiles[12][10].Color := $0E;
	Board.Tiles[13][10].Element := E_EMPTY;
	Board.Tiles[13][10].Color := $70;
	Board.Tiles[14][10].Element := E_SOLID;
	Board.Tiles[14][10].Color := $0F;

	AddStat(20, 15, E_DUPLICATOR, $0E, 1, StatTemplateDefault);
	statId := Board.StatCount;
	Board.Stats[statId].StepX := 1;
	Board.Stats[statId].P1 := 5;
	Board.Stats[statId].P2 := 2;
	Board.Tiles[19][15].Element := E_EMPTY;
	Board.Tiles[19][15].Color := $70;

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
	GamePlayLoop(false);
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
	GameLoopProbeCapture('F', 0);
	FreeMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
	GameLoopProbeCellWriteEvents := nil;
	WorldUnload;
	Dispose(IoTmpBuf);
end.
