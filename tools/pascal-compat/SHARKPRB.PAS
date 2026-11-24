program SharkProbe;
uses Crt, Video, GameVars, Elements, Game;
{$F+}
procedure QuietWrite(x, y, color: byte; text: TVideoLine);
begin
end;
{$F-}
procedure SetupShark(water: boolean);
begin
	BoardCreate;
	Board.StatCount := 1;
	Board.Stats[0].X := 13;
	Board.Stats[0].Y := 10;
	Board.Tiles[13][10].Element := E_PLAYER;
	Board.Tiles[13][10].Color := $0F;
	Board.Stats[1].X := 10;
	Board.Stats[1].Y := 10;
	Board.Stats[1].P1 := 0;
	Board.Stats[1].Cycle := 1;
	Board.Stats[1].Under.Element := E_EMPTY;
	Board.Stats[1].Under.Color := $70;
	Board.Tiles[10][10].Element := E_SHARK;
	Board.Tiles[10][10].Color := $0E;
	if water then begin
		Board.Tiles[11][10].Element := E_WATER;
		Board.Tiles[11][10].Color := $13;
	end else begin
		Board.Tiles[11][10].Element := E_EMPTY;
		Board.Tiles[11][10].Color := $31;
	end;
	RandSeed := 0;
end;
var
	output: text;
begin
	New(IoTmpBuf);
	VideoWriteText := QuietWrite;
	TickSpeed := 4;
	WorldCreate;
	Assign(output, 'SHARK.OUT');
	Rewrite(output);
	SetupShark(true);
	WriteLn(output, 'Seed-zero Random(10)=', Random(10));
	RandSeed := 0;
	ElementDefs[E_SHARK].TickProc(1);
	Write(output, Board.Stats[1].X, ' ', Board.Stats[1].Y);
	WriteLn(output, ' ', Board.Tiles[10][10].Element, ' ', Board.Tiles[11][10].Element,
		' ', Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color);
	SetupShark(false);
	ElementDefs[E_SHARK].TickProc(1);
	Write(output, Board.Stats[1].X, ' ', Board.Stats[1].Y);
	WriteLn(output, ' ', Board.Tiles[10][10].Element, ' ', Board.Tiles[11][10].Element,
		' ', Board.Stats[1].Under.Element, ' ', Board.Stats[1].Under.Color);
	Close(output);
end.
