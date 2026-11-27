{$M 49152,163840,655360}
program TitleLifecycleReplay;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;

var
	output: text;
	restoreScenario, generateRestoreSave, badRestoreScenario, cancelRetryScenario,
	savedWorldPlayScenario, emptyRestoreScenario, refusedAbortReturnScenario: boolean;
	fixtureFile: file;
	malformedHeader: array[1..512] of byte;
	malformedVersion: integer;
	ix, iy: integer;

begin
	restoreScenario := (ParamCount > 0) and (ParamStr(1) = 'R');
	generateRestoreSave := (ParamCount > 0) and (ParamStr(1) = 'G');
	badRestoreScenario := (ParamCount > 0) and (ParamStr(1) = 'B');
	cancelRetryScenario := (ParamCount > 0) and (ParamStr(1) = 'C');
	savedWorldPlayScenario := (ParamCount > 0) and (ParamStr(1) = 'S');
	emptyRestoreScenario := (ParamCount > 0) and (ParamStr(1) = 'E');
	refusedAbortReturnScenario := (ParamCount > 0) and (ParamStr(1) = 'A');
	if emptyRestoreScenario then begin
		Assign(fixtureFile, 'RESTORE.SAV');
		Rewrite(fixtureFile, 1);
		Close(fixtureFile);
	end;
	New(IoTmpBuf);
	RandSeed := 0;
	WorldCreate;
	World.Info.unk1 := 0;
	if badRestoreScenario then begin
		{ A complete 512-byte save header with unsupported world version -2. }
		FillChar(malformedHeader, SizeOf(malformedHeader), 0);
		malformedVersion := -2;
		Move(malformedVersion, malformedHeader[1], SizeOf(malformedVersion));
		Assign(fixtureFile, 'RESTORE.SAV');
		Rewrite(fixtureFile, 1);
		BlockWrite(fixtureFile, malformedHeader, SizeOf(malformedHeader));
		Close(fixtureFile);
	end;
	if restoreScenario or generateRestoreSave or cancelRetryScenario or savedWorldPlayScenario then begin
		if not WorldLoad('DEMO', '.ZZT', true) then RunError(258);
		if savedWorldPlayScenario then
			World.Info.Name := 'DEMO'
		else
			World.Info.Name := 'RESTORE';
		World.Info.IsSave := true;
		if savedWorldPlayScenario then begin
			{ Distinct live values prove that title-P reloads the original .ZZT header. }
			World.Info.Health := 1;
			World.Info.Ammo := 99;
		end else begin
			World.Info.Health := 37;
			World.Info.Ammo := 8;
			World.Info.Gems := 4;
			World.Info.Torches := 2;
			World.Info.TorchTicks := 123;
			World.Info.EnergizerTicks := 43;
			World.Info.Score := 777;
			World.Info.BoardTimeSec := 17;
			World.Info.BoardTimeHsec := 23;
			World.Info.Keys[1] := true;
		end;
	end;
	if generateRestoreSave then WorldSave('RESTORE', '.SAV');
	if not generateRestoreSave then begin
		{ The title monitor sees a deliberately different live state before R loads the saved game. }
		if restoreScenario or cancelRetryScenario or savedWorldPlayScenario then begin
			World.Info.Health := 1;
			World.Info.Ammo := 99;
		end;
	GameLoopProbeEnabled := true;
	GameLoopProbeTitleLifecycleEnabled := true;
	GameLoopProbeTitleLifecyclePlayerReturned := false;
	GameLoopProbeTitleAbortEnabled := refusedAbortReturnScenario;
	GameLoopProbeTitleRestoreEnabled := restoreScenario or badRestoreScenario or cancelRetryScenario or emptyRestoreScenario;
	GameLoopProbeTitleRestoreFailureEnabled := badRestoreScenario or emptyRestoreScenario;
	GameLoopProbeTitleRestoreEmptyEnabled := emptyRestoreScenario;
	GameLoopProbeTitleRestoreCancelOnceEnabled := cancelRetryScenario;
	GameLoopProbeTitleRestoreCancelled := false;
	GameLoopProbeSeedPauseBlink := true;
	GameLoopProbeCycleClockEnabled := false;
	GameLoopProbeStopAfter := 0;
	GameLoopProbeCycles := 0;
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeCellWriteCount := 0;
	GameLoopProbeCellWriteOverflow := false;
	GameLoopProbeCellWriteEvents := nil;
	if not (restoreScenario or cancelRetryScenario) then
		GetMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
	TickSpeed := 0;
	TickTimeCounter := 0;
	SoundTimeCheckCounter := 0;
	UseSystemTimeForElapsed := not refusedAbortReturnScenario;
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
	Assign(output, 'INTEGPR.OUT');
	Rewrite(output);
	Close(output);
	if restoreScenario then
		InputProbeSetKeyBuffer('RR')
	else if cancelRetryScenario then
		InputProbeSetKeyBuffer('R')
	else if badRestoreScenario then
		InputProbeSetKeyBuffer('R')
	else if emptyRestoreScenario then
		InputProbeSetKeyBuffer('R')
	else if refusedAbortReturnScenario then
		InputProbeSetKeyBuffer('P')
	else
		InputProbeSetKeyBuffer('PX');
	SoundProbeSetSchedule(0, 0, 0);
	GameTitleLoop;
	if refusedAbortReturnScenario then begin
		if (InputProbeGetKeyBuffer <> '') or (InputProbeGetReadKeys <> '') then RunError(259);
		if InputProbeTakeFollowupEscape then RunError(260);
	end;
	if (restoreScenario or badRestoreScenario or cancelRetryScenario or emptyRestoreScenario) and
		(InputProbeGetKeyBuffer <> '') then RunError(257);
	if cancelRetryScenario and not GameLoopProbeTitleRestoreCancelled then RunError(256);
	if badRestoreScenario or emptyRestoreScenario then begin
		Assign(output, 'TITLFAIL.OUT');
		Rewrite(output);
		{ The leading zero is GameWorldLoad's false result on the R route. }
		Write(output, 0, ' ', World.BoardCount, ' ', World.BoardLen[0],
			' ', ReturnBoardId, ' ', World.Info.CurrentBoard, ' ', World.Info.Ammo,
			' ', World.Info.Gems, ' ', World.Info.Health, ' ', World.Info.Torches,
			' ', World.Info.TorchTicks, ' ', World.Info.EnergizerTicks,
			' ', World.Info.Score, ' ', World.Info.BoardTimeSec,
			' ', World.Info.BoardTimeHsec, ' ', Ord(World.Info.IsSave),
			' ', Board.StatCount, ' ', Ord(InputKeyPressed), ' ');
		with Board.Stats[0] do
			Write(output, X, ' ', Y, ' ', StepX, ' ', StepY, ' ', Cycle,
				' ', P1, ' ', P2, ' ', P3, ' ', Follower, ' ', Leader,
				' ', Under.Element, ' ', Under.Color, ' ', DataPos, ' ', DataLen, ' ');
		for ix := 1 to 7 do Write(output, Ord(World.Info.Keys[ix]), ' ');
		for ix := 0 to BOARD_WIDTH + 1 do
			for iy := 0 to BOARD_HEIGHT + 1 do
				Write(output, Board.Tiles[ix][iy].Element, ' ', Board.Tiles[ix][iy].Color, ' ');
		WriteLn(output);
		Close(output);
	end;
	if not restoreScenario then begin
		FreeMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
		GameLoopProbeCellWriteEvents := nil;
	end;
	if not (badRestoreScenario or emptyRestoreScenario) then WorldUnload;
	end;
	Dispose(IoTmpBuf);
end.
