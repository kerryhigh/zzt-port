{$M 49152,163840,655360}
program RefusedEndPlayLoopReplay;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;

var
	output: text;
	outputName: string;
	firstResponse: string;
	x, y: integer;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

begin
	if ParamCount <> 1 then RunError(250);
	if ParamStr(1) = 'N' then begin
		outputName := 'ABORTN.OUT';
		firstResponse := 'NY';
	end else if ParamStr(1) = 'E' then begin
		outputName := 'ABORTE.OUT';
		firstResponse := Chr(27) + 'Y';
	end else begin
		RunError(251);
	end;

	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	RandSeed := 0;
	WorldCreate;
	World.Info.unk1 := 0;
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
	{ Use the controlled DOS timer tape so this refusal/cycle checkpoint is reproducible. }
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
	GamePlayExitRequested := false;
	InputProbeSetKeyBuffer(Chr(27));
	InputProbeSetReadKeys(firstResponse);
	InputProbeArmFollowupEscape;
	SoundProbeSetSchedule(0, 0, 0);
	GamePlayLoop(true);
	if not GamePlayExitRequested then RunError(255);
	if CurrentStatTicked <> 0 then RunError(256);
	if InputKeyPressed <> #0 then RunError(257);
	if InputProbeGetKeyBuffer <> '' then RunError(258);
	if InputProbeGetReadKeys <> '' then RunError(259);
	if InputProbeTakeFollowupEscape then RunError(260);
	x := Board.Stats[0].X;
	y := Board.Stats[0].Y;
	Assign(output, outputName);
	Rewrite(output);
	WriteLn(output,
		Ord(GamePlayExitRequested), ' ', Ord(GamePaused), ' ', Ord(InputKeyPressed), ' ',
		Length(InputProbeGetKeyBuffer), ' ', CurrentTick, ' ', CurrentStatTicked, ' ',
		TickTimeDuration, ' ', TickTimeCounter, ' ', World.Info.CurrentBoard, ' ',
		ReturnBoardId, ' ', World.Info.Health, ' ', World.Info.Ammo, ' ',
		Ord(World.Info.IsSave), ' ', Board.StatCount, ' ', x, ' ', y, ' ',
		Board.Tiles[x][y].Element, ' ', Board.Tiles[x][y].Color, ' ',
		Ord(SoundBlockQueueing), ' ', SoundBufferPos, ' ', Length(SoundBuffer), ' ',
		SoundCurrentPriority);
	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
