program IntegratedGameplayProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

var
	output: text;
	ix, iy, statId: integer;
	script: string[32];

procedure WriteStat(statId: integer);
	begin
		with Board.Stats[statId] do begin
			Write(output, X, ' ', Y, ' ', StepX, ' ', StepY, ' ', Cycle,
				' ', P1, ' ', P2, ' ', P3, ' ', Follower, ' ', Leader,
				' ', Under.Element, ' ', Under.Color, ' ', DataPos, ' ', DataLen, ' ');
		end;
	end;

begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	RandSeed := 0;
	WorldCreate;
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
	Board.Tiles[6][5].Element := E_BOULDER;
	Board.Tiles[6][5].Color := $0E;
	Board.Tiles[7][5].Element := E_EMPTY;
	Board.Tiles[7][5].Color := $70;

	AddStat(20, 5, E_OBJECT, $0A, 1, StatTemplateDefault);
	script := '#GIVE GEMS 1' + #13 + '#IDLE' + #13;
	GetMem(Board.Stats[1].Data, Length(script));
	Move(script[1], Board.Stats[1].Data^, Length(script));
	Board.Stats[1].DataLen := Length(script);
	Board.Stats[1].DataPos := 0;

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
	InputDeltaX := 0;
	InputDeltaY := 0;
	InputKeyPressed := #0;
	InputShiftPressed := false;
	InputShiftAccepted := false;
	InputProbeSetKeyBuffer('66');
	GameLoopProbeStopAfter := 6;
	GameLoopProbeCycles := 0;
	SoundProbeSetSchedule(0, 0, 0);
	GamePlayLoop(false);

	Assign(output, 'INTEGPR.OUT');
	Rewrite(output);
	Write(output, World.Info.Ammo, ' ', World.Info.Gems, ' ', World.Info.Health,
		' ', World.Info.CurrentBoard, ' ', World.Info.Torches, ' ', World.Info.TorchTicks,
		' ', World.Info.EnergizerTicks, ' ', World.Info.Score,
		' ', World.Info.BoardTimeSec, ' ', World.Info.BoardTimeHsec,
		' ', Ord(World.Info.IsSave), ' ', Board.StatCount, ' ');
	for statId := 0 to 1 do WriteStat(statId);
	Write(output, CurrentTick, ' ', CurrentStatTicked, ' ', TickSpeed,
		' ', TickTimeDuration, ' ', TickTimeCounter, ' ', Ord(GamePaused),
		' ', Ord(GamePlayExitRequested), ' ', InputDeltaX, ' ', InputDeltaY,
		' ', Ord(InputKeyPressed), ' ', Length(InputProbeGetKeyBuffer), ' ');
	for ix := 0 to BOARD_WIDTH + 1 do
		for iy := 0 to BOARD_HEIGHT + 1 do
			Write(output, Board.Tiles[ix][iy].Element, ' ', Board.Tiles[ix][iy].Color, ' ');
	WriteLn(output);
	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
