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
	CurrentTick := 0;
	CurrentStatTicked := 0;
	TickTimeDuration := 0;
	TickTimeCounter := 0;
	GamePlayExitRequested := false;
	Assign(output, 'INTEGPR.OUT');
	Rewrite(output);
	Close(output);
	GameLoopProbeEnabled := true;
	GameLoopProbeCapture('L', 0);
	InputProbeSetKeyBuffer('66');
	GameLoopProbeStopAfter := 6;
	GameLoopProbeCycles := 0;
	SoundProbeSetSchedule(0, 0, 0);
	GamePlayLoop(false);
	GameLoopProbeEnabled := false;
	GameLoopProbeCapture('F', 0);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
