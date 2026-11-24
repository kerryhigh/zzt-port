program PausedMovementProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;
var
	output: text;
	ProbeExitOnPauseRedraw: boolean;
	ProbeExitOnNeutralPlayerTick: boolean;
{$F+}
procedure ProbeWrite(x, y, color: byte; text: TVideoLine);
begin
	if ProbeExitOnPauseRedraw then
		GamePlayExitRequested := true
	else if ProbeExitOnNeutralPlayerTick and not GamePaused
		and (Board.Stats[0].X = 11) and (Board.Stats[0].Y = 10)
		and (InputDeltaX = 0) and (InputDeltaY = 0)
		and (InputKeyPressed = #0) then
		{ The health-zero player tick writes its game-over message after the
		  paused move's one-shot delta has been cleared. Stop after that tick. }
		GamePlayExitRequested := true;
end;
{$F-}

begin
	VideoWriteText := ProbeWrite;
	New(IoTmpBuf);
	Assign(output, 'PAUSEMV.OUT');
	Rewrite(output);

	WorldCreate;
	Board.Tiles[30][12].Element := E_EMPTY;
	Board.Tiles[30][12].Color := 0;
	Board.Stats[0].X := 10;
	Board.Stats[0].Y := 10;
	Board.Stats[0].Cycle := 1;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := $17;
	Board.Tiles[10][10].Element := E_PLAYER;
	Board.Tiles[10][10].Color := ElementDefs[E_PLAYER].Color;
	Board.Tiles[11][10].Element := E_EMPTY;
	Board.Tiles[11][10].Color := $46;
	Board.Info.TimeLimitSec := 0;
	World.Info.Health := 0;
	World.Info.Score := 0;
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
	GameStateElement := E_PLAYER;
	GamePaused := true;
	JustStarted := false;
	InputProbeSetKeyBuffer('6');
	ProbeExitOnPauseRedraw := true;
	GamePlayLoop(false);

	WriteLn(output,
		'pause-open player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' old ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' tile ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' under ', Board.Stats[0].Under.Element, ' ', Board.Stats[0].Under.Color,
		' delta ', InputDeltaX, ' ', InputDeltaY,
		' paused ', Ord(GamePaused), ' save ', Ord(World.Info.IsSave),
		' exit ', Ord(GamePlayExitRequested), ' key ', Ord(InputKeyPressed),
		' health ', World.Info.Health);
	Flush(output);

	{ Continue in the same world until its next neutral player tick. }
	ProbeExitOnPauseRedraw := false;
	ProbeExitOnNeutralPlayerTick := true;
	InputProbeSetKeyBuffer('');
	GamePlayLoop(false);
	WriteLn(output,
		'pause-cycle player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' old ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' tile ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' under ', Board.Stats[0].Under.Element, ' ', Board.Stats[0].Under.Color,
		' delta ', InputDeltaX, ' ', InputDeltaY,
		' paused ', Ord(GamePaused), ' save ', Ord(World.Info.IsSave),
		' exit ', Ord(GamePlayExitRequested), ' key ', Ord(InputKeyPressed),
		' health ', World.Info.Health);
	Flush(output);

	{ Reset the world and compare a blocked paused move from a fresh board. }
	WorldUnload;
	WorldCreate;
	BoardCreate;
	Board.Stats[0].X := 10;
	Board.Stats[0].Y := 10;
	Board.Stats[0].Cycle := 1;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := $17;
	Board.Tiles[10][10].Element := E_PLAYER;
	Board.Tiles[10][10].Color := ElementDefs[E_PLAYER].Color;
	Board.Tiles[11][10].Element := E_SOLID;
	Board.Tiles[11][10].Color := $5F;
	Board.Info.TimeLimitSec := 0;
	World.Info.Health := 0;
	World.Info.Score := 0;
	World.Info.IsSave := false;
	TickSpeed := 0;
	SoundEnabled := false;
	SoundIsPlaying := false;
	SoundCurrentPriority := -1;
	SoundBlockQueueing := false;
	SoundBuffer := '';
	InputDeltaX := 0;
	InputDeltaY := 0;
	InputKeyPressed := #0;
	GameStateElement := E_PLAYER;
	GamePaused := true;
	JustStarted := false;
	ProbeExitOnPauseRedraw := true;
	ProbeExitOnNeutralPlayerTick := false;
	InputProbeSetKeyBuffer('6');
	GamePlayLoop(false);
	WriteLn(output,
		'pause-blocked player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' old ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' tile ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' under ', Board.Stats[0].Under.Element, ' ', Board.Stats[0].Under.Color,
		' delta ', InputDeltaX, ' ', InputDeltaY,
		' paused ', Ord(GamePaused), ' save ', Ord(World.Info.IsSave),
		' exit ', Ord(GamePlayExitRequested), ' key ', Ord(InputKeyPressed),
		' health ', World.Info.Health);
	Flush(output);

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
