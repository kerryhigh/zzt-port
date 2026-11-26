program TitleLifecycleReplay;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;

var
	output: text;

begin
	New(IoTmpBuf);
	RandSeed := 0;
	WorldCreate;
	World.Info.unk1 := 0;
	GameLoopProbeEnabled := true;
	GameLoopProbeTitleLifecycleEnabled := true;
	GameLoopProbeTitleLifecyclePlayerReturned := false;
	GameLoopProbeSeedPauseBlink := true;
	GameLoopProbeCycleClockEnabled := false;
	GameLoopProbeStopAfter := 0;
	GameLoopProbeCycles := 0;
	GameLoopProbeCellCaptureEnabled := false;
	GameLoopProbeCellWriteCount := 0;
	GameLoopProbeCellWriteOverflow := false;
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
	InputProbeSetKeyBuffer('PX');
	SoundProbeSetSchedule(0, 0, 0);
	GameTitleLoop;
	FreeMem(GameLoopProbeCellWriteEvents, SizeOf(TGameLoopProbeCellWriteEventArray));
	GameLoopProbeCellWriteEvents := nil;
	WorldUnload;
	Dispose(IoTmpBuf);
end.
