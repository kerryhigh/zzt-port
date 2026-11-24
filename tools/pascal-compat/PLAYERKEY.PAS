program PlayerPauseKeyProbe;
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
	Assign(output, 'PLAYERKY.OUT');
	Rewrite(output);

	WorldCreate;
	Board.Tiles[30][12].Element := E_EMPTY;
	Board.Tiles[30][12].Color := 0;
	Board.Stats[0].X := 10;
	Board.Stats[0].Y := 10;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := $17;
	Board.Stats[0].Cycle := 1;
	Board.Tiles[10][10].Element := E_PLAYER;
	Board.Tiles[10][10].Color := ElementDefs[E_PLAYER].Color;
	ElementDefs[E_PLAYER].Character := #2;
	Board.Info.MaxShots := 2;
	Board.Info.TimeLimitSec := 0;
	World.Info.Health := 100;
	World.Info.Ammo := 2;
	World.Info.Torches := 1;
	World.Info.TorchTicks := 1;
	World.Info.EnergizerTicks := 1;
	CurrentTick := 1;
	TickSpeed := 4;
	TickTimeDuration := 4;
	InputDeltaX := 0;
	InputDeltaY := 0;
	InputShiftPressed := false;
	InputKeyPressed := 'p';
	PlayerDirX := 1;
	PlayerDirY := 0;
	GamePaused := false;
	SoundEnabled := false;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBlockQueueing := false;
	SoundBuffer := '';
	ElementDefs[E_PLAYER].TickProc(0);

	WriteLn(output, 'pause-key paused ', Ord(GamePaused),
		' health ', World.Info.Health,
		' torch ', World.Info.TorchTicks,
		' energizer ', World.Info.EnergizerTicks,
		' duration ', TickTimeDuration,
		' color ', Board.Tiles[10][10].Color,
		' char ', Ord(ElementDefs[E_PLAYER].Character),
		' key ', Ord(InputKeyPressed),
		' sound ', SoundCurrentPriority, ' ', Length(SoundBuffer),
		' ', Ord(SoundBuffer[1]), ' ', Ord(SoundBuffer[2]));
	Flush(output);

	GamePaused := false;
	World.Info.TorchTicks := 0;
	World.Info.EnergizerTicks := 0;
	TickTimeDuration := 4;
	InputKeyPressed := 'b';
	SoundEnabled := false;
	SoundCurrentPriority := 2;
	SoundIsPlaying := true;
	SoundBlockQueueing := false;
	SoundBuffer := 'old sound';
	ElementDefs[E_PLAYER].TickProc(0);

	WriteLn(output, 'sound-key paused ', Ord(GamePaused),
		' health ', World.Info.Health,
		' enabled ', Ord(SoundEnabled),
		' playing ', Ord(SoundIsPlaying),
		' buffer ', Length(SoundBuffer),
		' priority ', SoundCurrentPriority,
		' key ', Ord(InputKeyPressed));
	Flush(output);

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
