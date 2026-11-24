program GameScheduleProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;

{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}

var
	output: text;

procedure RunCase(speed, startHsecs, stallAfterPoll, stallExtraPits: integer);
	var
		i: integer;
	begin
		WorldCreate;
		World.Info.Health := 100;
		GameStateElement := E_PLAYER;
		GamePaused := false;
		JustStarted := false;
		TickSpeed := speed;
		TickTimeCounter := startHsecs;
		TimerTicks := 0;
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
		InputProbeSetKeyBuffer('');
		GameLoopProbeStopAfter := 5;
		GameLoopProbeCycles := 0;
		SoundProbeSetSchedule(startHsecs, stallAfterPoll, stallExtraPits);

		GamePlayLoop(false);

		Write(output,
			'speed ', speed,
			' start ', startHsecs,
			' stall ', stallAfterPoll,
			' extra ', stallExtraPits,
			' cycles ', GameLoopProbeCycles,
			' delta ', CurrentTick - GameLoopProbeStartTick,
			' stat ', CurrentStatTicked,
			' counter ', TickTimeCounter,
			' times');
		if SoundProbeGetScheduleCycleCount <> 5 then begin
			WriteLn(output, ' schedule-count ', SoundProbeGetScheduleCycleCount);
			Close(output);
			Halt(1);
		end;
		for i := 0 to 4 do Write(output, ' ', SoundProbeGetScheduleCycleHsec(i));
		WriteLn(output);
		WorldUnload;
	end;

begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	Assign(output, 'GAMESCHD.OUT');
	Rewrite(output);
	RunCase(1, 0, 0, 0);
	RunCase(6, 0, 0, 0);
	RunCase(9, 0, 0, 0);
	RunCase(6, 0, 2, 4);
	RunCase(9, 5980, 0, 0);
	Close(output);
	Dispose(IoTmpBuf);
end.
