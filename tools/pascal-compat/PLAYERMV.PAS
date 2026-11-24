program PlayerMovementTickProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game, Input;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;

procedure SetupPlayer;
begin
	WorldCreate;
	Board.Tiles[30][12].Element := Board.Stats[0].Under.Element;
	Board.Tiles[30][12].Color := Board.Stats[0].Under.Color;
	Board.Stats[0].X := 10;
	Board.Stats[0].Y := 10;
	Board.Stats[0].Under.Element := E_EMPTY;
	Board.Stats[0].Under.Color := $17;
	Board.Tiles[10][10].Element := E_PLAYER;
	Board.Tiles[10][10].Color := ElementDefs[E_PLAYER].Color;
	Board.Info.MaxShots := 2;
	Board.Info.TimeLimitSec := 0;
	World.Info.Health := 100;
	World.Info.Ammo := 2;
	World.Info.EnergizerTicks := 0;
	World.Info.TorchTicks := 0;
	CurrentTick := 0;
	TickSpeed := 4;
	TickTimeDuration := 4;
	InputDeltaX := 0;
	InputDeltaY := 0;
	InputShiftPressed := false;
	InputShiftAccepted := false;
	InputKeyPressed := #0;
	PlayerDirX := 0;
	PlayerDirY := 0;
	SoundEnabled := false;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBlockQueueing := false;
	SoundBuffer := '';
end;

procedure PrintMove(labelText: string);
begin
	WriteLn(output, labelText,
		' player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' delta ', InputDeltaX, ' ', InputDeltaY,
		' dir ', PlayerDirX, ' ', PlayerDirY,
		' stats ', Board.StatCount,
		' t0 ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' t1 ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' t2 ', Board.Tiles[12][10].Element, ' ', Board.Tiles[12][10].Color,
		' under ', Board.Stats[0].Under.Element, ' ', Board.Stats[0].Under.Color,
		' sound ', SoundCurrentPriority, ' ', Length(SoundBuffer));
	Flush(output);
end;

begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	Assign(output, 'PLAYERMV.OUT');
	Rewrite(output);

	SetupPlayer;
	Board.Tiles[11][10].Element := E_EMPTY;
	Board.Tiles[11][10].Color := $47;
	InputDeltaX := 1;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintMove('move-empty');

	SetupPlayer;
	Board.Tiles[11][10].Element := E_SOLID;
	Board.Tiles[11][10].Color := $5F;
	InputDeltaX := 1;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintMove('move-blocked');

	SetupPlayer;
	Board.Tiles[11][10].Element := E_BOULDER;
	Board.Tiles[11][10].Color := $0E;
	Board.Tiles[12][10].Element := E_EMPTY;
	Board.Tiles[12][10].Color := $56;
	InputDeltaX := 1;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintMove('move-push');

	SetupPlayer;
	Board.Tiles[11][10].Element := E_BOULDER;
	Board.Tiles[11][10].Color := $0E;
	Board.Tiles[12][10].Element := E_SOLID;
	Board.Tiles[12][10].Color := $5F;
	InputDeltaX := 1;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintMove('move-push-blocked');

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
