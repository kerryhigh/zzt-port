program PlayerShootTickProbe;
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
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBlockQueueing := false;
	SoundBuffer := '';
end;

procedure PrintShot(labelText: string; targetX, targetY: integer; hasBullet: boolean);
var
	i: integer;
begin
	Write(output, labelText, ' ammo ', World.Info.Ammo, ' input ', InputDeltaX, ' ', InputDeltaY,
		' dir ', PlayerDirX, ' ', PlayerDirY, ' stats ', Board.StatCount,
		' target ', Board.Tiles[targetX][targetY].Element, ' ', Board.Tiles[targetX][targetY].Color,
		' sound ', SoundCurrentPriority, ' ', Length(SoundBuffer));
	if hasBullet then
		Write(output, ' bullet ', Board.Stats[Board.StatCount].X, ' ', Board.Stats[Board.StatCount].Y,
			' ', Board.Stats[Board.StatCount].P1, ' ', Board.Stats[Board.StatCount].StepX,
			' ', Board.Stats[Board.StatCount].StepY, ' ', Board.Stats[Board.StatCount].P2,
			' under ', Board.Stats[Board.StatCount].Under.Element, ' ',
			Board.Stats[Board.StatCount].Under.Color);
	for i := 1 to Length(SoundBuffer) do
		Write(output, ' ', Ord(SoundBuffer[i]));
	WriteLn(output);
	Flush(output);
end;

begin
	VideoWriteText := QuietWrite;
	New(IoTmpBuf);
	Assign(output, 'PLAYERST.OUT');
	Rewrite(output);
	SoundEnabled := false;

	SetupPlayer;
	InputShiftPressed := true;
	InputDeltaX := 1;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintShot('shoot-held-right', 11, 10, true);

	SetupPlayer;
	PlayerDirY := 1;
	InputKeyPressed := ' ';
	ElementDefs[E_PLAYER].TickProc(0);
	PrintShot('shoot-space-down', 10, 11, true);

	SetupPlayer;
	InputShiftPressed := true;
	InputDeltaX := 1;
	Board.Tiles[11][10].Element := E_SOLID;
	Board.Tiles[11][10].Color := $1F;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintShot('shoot-blocked', 11, 10, false);

	SetupPlayer;
	Board.Info.MaxShots := 1;
	AddStat(15, 12, E_BULLET, ElementDefs[E_BULLET].Color, 1, StatTemplateDefault);
	Board.Stats[Board.StatCount].P1 := SHOT_SOURCE_PLAYER;
	Board.Stats[Board.StatCount].StepX := 1;
	Board.Stats[Board.StatCount].StepY := 0;
	Board.Stats[Board.StatCount].P2 := 100;
	InputShiftPressed := true;
	InputDeltaX := 1;
	ElementDefs[E_PLAYER].TickProc(0);
	PrintShot('shoot-limit', 11, 10, false);

	SetupPlayer;
	InputKeyPressed := ' ';
	ElementDefs[E_PLAYER].TickProc(0);
	PrintShot('shoot-space-no-direction', 11, 10, false);

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
