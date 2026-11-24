program StarPrb;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[0].X := 13;
	Board.Stats[0].Y := 10;
	Board.Tiles[13][10].Element := E_PLAYER;
	Board.Tiles[13][10].Color := ElementDefs[E_PLAYER].Color;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].P2 := 4;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_STAR;
	Board.Tiles[10][10].Color := ElementDefs[E_STAR].Color;
	Board.Tiles[11][10].Element := E_WATER;
	Board.Tiles[11][10].Color := $13;
	World.Info.EnergizerTicks := 0;
	RandSeed := 0;
	Assign(output, 'STARPRB.OUT');
	Rewrite(output);
	ElementDefs[E_STAR].TickProc(1);
	WriteLn(output, 'tick1 p2 ', Board.Stats[1].P2,
		' pos ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' step ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
		' under ', Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color,
		' tiles ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' count ', Board.StatCount, ' seed ', RandSeed);
	ElementDefs[E_STAR].TickProc(1);
	WriteLn(output, 'tick2 p2 ', Board.Stats[1].P2,
		' pos ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' step ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
		' under ', Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color,
		' tiles ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' count ', Board.StatCount, ' seed ', RandSeed);
	ElementDefs[E_STAR].TickProc(1);
	WriteLn(output, 'tick3 p2 ', Board.Stats[1].P2,
		' pos ', Board.Stats[1].X, ' ', Board.Stats[1].Y,
		' step ', Board.Stats[1].StepX, ' ', Board.Stats[1].StepY,
		' under ', Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color,
		' tiles ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' count ', Board.StatCount, ' seed ', RandSeed);
	ElementDefs[E_STAR].TickProc(1);
	WriteLn(output, 'tick4 count ', Board.StatCount,
		' tiles ', Board.Tiles[10][10].Element, ' ', Board.Tiles[10][10].Color,
		' ', Board.Tiles[11][10].Element, ' ', Board.Tiles[11][10].Color,
		' seed ', RandSeed);
	Close(output);
end.
