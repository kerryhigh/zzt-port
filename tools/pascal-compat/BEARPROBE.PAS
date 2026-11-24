program BearProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
var
	output: text;
procedure RunCase(playerX, playerY, bearX, bearY, p1: byte);
begin
	BoardCreate;
	Board.Tiles[30][12].Element := E_EMPTY;
	Board.Stats[0].X := playerX;
	Board.Stats[0].Y := playerY;
	Board.Tiles[playerX][playerY].Element := E_PLAYER;
	Board.Tiles[playerX][playerY].Color := ElementDefs[E_PLAYER].Color;
	Board.StatCount := 1;
	Board.Stats[1].X := bearX;
	Board.Stats[1].Y := bearY;
	Board.Stats[1].P1 := p1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[bearX][bearY].Element := E_BEAR;
	Board.Tiles[bearX][bearY].Color := ElementDefs[E_BEAR].Color;
	ElementDefs[E_BEAR].TickProc(1);
	WriteLn(output, Board.Stats[1].X, ' ', Board.Stats[1].Y, ' ',
		Board.Tiles[bearX][bearY].Element, ' ', Board.Tiles[bearX][bearY].Color);
end;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	Assign(output, 'BEARPROB.OUT');
	Rewrite(output);
	RunCase(15, 13, 10, 10, 0);
	RunCase(10, 13, 10, 10, 0);
	RunCase(15, 13, 10, 10, 6);
	Close(output);
end.
