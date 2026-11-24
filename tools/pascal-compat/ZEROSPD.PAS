program ZeroSpeedGameLoopProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

var
	output: text;

begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	WorldCreate;
	World.Info.Health := 100;
	GameStateElement := E_PLAYER;
	GamePaused := false;
	JustStarted := false;
	TickSpeed := 0;
	TickTimeCounter := 0;
	TimerTicks := 0;
	SoundTimeCheckCounter := 0;
	UseSystemTimeForElapsed := false;
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
	InputProbeSetKeyBuffer('');
	GameLoopProbeStopAfter := 5;
	GameLoopProbeCycles := 0;

	GamePlayLoop(false);

	Assign(output, 'ZEROSPD.OUT');
	Rewrite(output);
	WriteLn(output,
		'zero-speed cycles ', GameLoopProbeCycles,
		' delta ', CurrentTick - GameLoopProbeStartTick,
		' stat ', CurrentStatTicked,
		' timer ', TimerTicks,
		' counter ', TickTimeCounter,
		' exit ', Ord(GamePlayExitRequested));
	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
