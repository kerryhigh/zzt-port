{$M 49152,163840,655360}
program TitleLifecycleReplay;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;

var
	output: text;
	restoreScenario, generateRestoreSave: boolean;

begin
	restoreScenario := (ParamCount > 0) and (ParamStr(1) = 'R');
	generateRestoreSave := (ParamCount > 0) and (ParamStr(1) = 'G');
	New(IoTmpBuf);
	RandSeed := 0;
	WorldCreate;
	World.Info.unk1 := 0;
	if restoreScenario or generateRestoreSave then begin
		if not WorldLoad('DEMO', '.ZZT', true) then RunError(258);
		World.Info.Name := 'RESTORE';
		World.Info.IsSave := true;
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
	if generateRestoreSave then WorldSave('RESTORE', '.SAV');
	if not generateRestoreSave then begin
		{ The title monitor sees a deliberately different live state before R loads the saved game. }
		if restoreScenario then begin
			World.Info.Health := 1;
			World.Info.Ammo := 99;
		end;
	GameLoopProbeEnabled := true;
	GameLoopProbeTitleLifecycleEnabled := true;
	GameLoopProbeTitleLifecyclePlayerReturned := false;
	GameLoopProbeTitleRestoreEnabled := restoreScenario;
	GameLoopProbeSeedPauseBlink := true;
	GameLoopProbeCycleClockEnabled := false;
	GameLoopProbeStopAfter := 0;
	GameLoopProbeCycles := 0;
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeCellWriteCount := 0;
	GameLoopProbeCellWriteOverflow := false;
	GameLoopProbeCellWriteEvents := nil;
	if not restoreScenario then
		GetMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
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
	GamePlayExitRequested := false;
	Assign(output, 'INTEGPR.OUT');
	Rewrite(output);
	Close(output);
	if restoreScenario then
		InputProbeSetKeyBuffer('RR')
	else
		InputProbeSetKeyBuffer('PX');
	SoundProbeSetSchedule(0, 0, 0);
	GameTitleLoop;
	if restoreScenario and (InputProbeGetKeyBuffer <> '') then RunError(257);
	if not restoreScenario then begin
		FreeMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
		GameLoopProbeCellWriteEvents := nil;
	end;
	WorldUnload;
	end;
	Dispose(IoTmpBuf);
end.
