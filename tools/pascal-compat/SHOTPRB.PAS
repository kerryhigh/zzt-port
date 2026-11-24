program BoardShootProbe;
uses Crt, Video, GameVars, Elements, Sounds, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
	result: boolean;

procedure SetupShot(targetElement, targetColor, source, energizer: integer);
begin
	WorldCreate;
	Board.StatCount := 1;
	Board.Stats[1] := StatTemplateDefault;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].Cycle := 1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := 0;
	Board.Tiles[10][10].Element := E_OBJECT;
	Board.Tiles[10][10].Color := ElementDefs[E_OBJECT].Color;
	Board.Tiles[11][10].Element := targetElement;
	Board.Tiles[11][10].Color := targetColor;
	Board.Stats[0].X := 30;
	Board.Stats[0].Y := 12;
	if targetElement = E_PLAYER then begin
		Board.Stats[0].X := 11;
		Board.Stats[0].Y := 10;
	end;
	World.Info.Health := 100;
	World.Info.EnergizerTicks := energizer;
	SoundCurrentPriority := -1;
	SoundIsPlaying := false;
	SoundBuffer := '';
	result := BoardShoot(E_BULLET, 10, 10, 1, 0, source);
end;

procedure PrintState(labelText: string; includeBullet: boolean);
var
	i: integer;
begin
	Write(output, labelText, ' result ', Ord(result), ' stats ', Board.StatCount,
		' target ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' health ', World.Info.Health, ' sound ', SoundCurrentPriority, ' ', Length(SoundBuffer));
	if includeBullet then
		Write(output, ' bullet ', Board.Stats[2].X, ' ', Board.Stats[2].Y, ' ',
			Board.Stats[2].P1, ' ', Board.Stats[2].StepX, ' ', Board.Stats[2].StepY, ' ',
			Board.Stats[2].P2, ' under ', Board.Stats[2].Under.Element, ' ',
			Board.Stats[2].Under.Color);
	for i := 1 to Length(SoundBuffer) do
		Write(output, ' ', Ord(SoundBuffer[i]));
	WriteLn(output);
	Flush(output);
end;

begin
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := 4;
	Assign(output, 'SHOTPRB.OUT');
	Rewrite(output);
	New(IoTmpBuf);
	SoundEnabled := false;
	SoundBlockQueueing := false;

	SetupShot(E_EMPTY, $2A, SHOT_SOURCE_PLAYER, 0);
	PrintState('shot-empty', true);
	SetupShot(E_WATER, $17, SHOT_SOURCE_ENEMY, 0);
	PrintState('shot-water', true);
	SetupShot(E_SOLID, $1F, SHOT_SOURCE_PLAYER, 0);
	PrintState('shot-solid', false);
	SetupShot(E_BREAKABLE, $2E, SHOT_SOURCE_ENEMY, 0);
	PrintState('shot-breakable', false);
	SetupShot(E_LION, $0C, SHOT_SOURCE_PLAYER, 0);
	PrintState('shot-lion-player', false);
	SetupShot(E_DOOR, $2F, SHOT_SOURCE_PLAYER, 0);
	PrintState('shot-door-player', false);
	SetupShot(E_PLAYER, ElementDefs[E_PLAYER].Color, SHOT_SOURCE_PLAYER, 0);
	PrintState('shot-player-friendly', false);
	SetupShot(E_PLAYER, ElementDefs[E_PLAYER].Color, SHOT_SOURCE_ENEMY, 0);
	PrintState('shot-player-enemy', false);

	SetupShot(E_PLAYER, ElementDefs[E_PLAYER].Color, SHOT_SOURCE_ENEMY, 1);
	PrintState('shot-player-energized', false);

	Close(output);
	WorldUnload;
	Dispose(IoTmpBuf);
end.
