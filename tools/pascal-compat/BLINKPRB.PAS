program BlinkWallProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
procedure RunCase(labelText: string; rightEmpty: boolean);
begin
	BoardCreate;
	World.Info.Health := 100;
	Board.StatCount := 1;
	Board.Stats[0].X := 20;
	Board.Stats[0].Y := 11;
	Board.Tiles[20][11].Element := E_PLAYER;
	Board.Tiles[20][11].Color := ElementDefs[E_PLAYER].Color;
	Board.Stats[1].X := 20;
	Board.Stats[1].Y := 10;
	Board.Stats[1].StepX := 0;
	Board.Stats[1].StepY := 1;
	Board.Stats[1].P1 := 0;
	Board.Stats[1].P2 := 1;
	Board.Stats[1].P3 := 0;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[20][10].Element := E_BLINK_WALL;
	Board.Tiles[20][10].Color := 7;
	Board.Tiles[21][11].Element := E_SOLID;
	Board.Tiles[21][11].Color := $70;
	Board.Tiles[19][11].Element := E_SOLID;
	Board.Tiles[19][11].Color := $70;
	if rightEmpty then
		Board.Tiles[21][11].Element := E_EMPTY
	else
		Board.Tiles[19][11].Element := E_EMPTY;
	Board.Tiles[20][15].Element := E_SOLID;
	ElementDefs[E_BLINK_WALL].TickProc(1);
	WriteLn(output, labelText,
		' player ', Board.Stats[0].X, ' ', Board.Stats[0].Y,
		' origin ', Board.Tiles[20][11].Element, ' ', Board.Tiles[20][11].Color,
		' right ', Board.Tiles[21][11].Element, ' ', Board.Tiles[21][11].Color,
		' left ', Board.Tiles[19][11].Element, ' ', Board.Tiles[19][11].Color,
		' player-under ', Board.Stats[0].Under.Element, ' ', Board.Stats[0].Under.Color,
		' p3 ', Board.Stats[1].P3,
		' health ', World.Info.Health,
		' stats ', Board.StatCount,
		' timer ', Board.Stats[2].P2,
		' message ', Board.Info.Message);
end;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	TickTimeDuration := TickSpeed * 2;
	WorldCreate;
	Assign(output, 'BLINKPRB.OUT');
	Rewrite(output);
	RunCase('right-empty', true);
	RunCase('left-empty', false);
	Close(output);
end.
