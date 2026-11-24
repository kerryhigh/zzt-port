program PlayerDeathTickProbe;
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
	Board.Stats[0].Under.Color := 0;
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

procedure PrintDeath(labelText: string);
begin
	WriteLn(output, labelText,
		' player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' health ', World.Info.Health, ' ammo ', World.Info.Ammo,
		' input ', InputDeltaX, ' ', InputDeltaY,
		' shift ', InputShiftPressed,
		' dir ', PlayerDirX, ' ', PlayerDirY,
		' stats ', Board.StatCount,
		' tick ', TickTimeDuration,
		' block ', SoundBlockQueueing,
		' sound ', SoundCurrentPriority, ' ', Length(SoundBuffer),
		' tile0 ', Board.Tiles[0][0].Element, ' ', Board.Tiles[0][0].Color,
		' p2 ', Board.Stats[1].P2,
		' shot ', Board.Tiles[11][10].Element);
	Flush(output);
end;

begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	Assign(output, 'PLAYERDE.OUT');
	Rewrite(output);

	SetupPlayer;
	World.Info.Health := 0;
	InputShiftPressed := true;
	InputDeltaX := 1;
	InputKeyPressed := ' ';
	PlayerDirX := 1;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintDeath('dead-space');

	SetupPlayer;
	AddStat(0, 0, E_OBJECT, ElementDefs[E_OBJECT].Color, 1, StatTemplateDefault);
	World.Info.Health := 0;
	InputShiftPressed := true;
	InputDeltaX := 1;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintDeath('dead-existing-stat');

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
